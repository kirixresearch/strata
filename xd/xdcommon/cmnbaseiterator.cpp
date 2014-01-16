/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-12-23
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <xd/xd.h>
#include "xd_private.h"
#include <kl/portable.h>
#include <kl/math.h>
#include "xdcommon.h"
#include "cmnbaseiterator.h"
#include "keylayout.h"
#include "util.h"
#include "../../kscript/kscript.h"

enum
{
    GroupFunc_None = 0,
    GroupFunc_First = 1,
    GroupFunc_Last = 2,
    GroupFunc_Min = 3,
    GroupFunc_Max = 4,
    GroupFunc_Sum = 5,
    GroupFunc_Avg = 6,
    GroupFunc_Count = 7,
    GroupFunc_MaxDistance = 8,
    GroupFunc_Stddev = 9,
    GroupFunc_Variance = 10,
    GroupFunc_Merge = 11,
    GroupFunc_GroupID = 12
};

class CommonBaseIteratorBindInfo
{
public:
    xd::IIterator* iter;
    xd::objhandle_t handle;
};


CommonBaseIterator::CommonBaseIterator()
{
}

CommonBaseIterator::~CommonBaseIterator()
{
    // clear out binding info structures that were created in our parse hook
    std::vector<CommonBaseIteratorBindInfo*>::iterator it;
    for (it = m_bindings.begin();
         it != m_bindings.end(); ++it)
    {
        delete (*it);
    }
}

static void _bindFieldString(kscript::ExprEnv*,
                             void* param,
                             kscript::Value* retval)
{
    CommonBaseIteratorBindInfo* info = (CommonBaseIteratorBindInfo*)param;
    if (info->iter->isNull(info->handle))
    {
        retval->setNull();
        return;
    }
    retval->setString(info->iter->getWideString(info->handle));
}


static void _bindFieldDouble(kscript::ExprEnv*,
                             void* param,
                             kscript::Value* retval)
{
    CommonBaseIteratorBindInfo* info = (CommonBaseIteratorBindInfo*)param;
    if (info->iter->isNull(info->handle))
    {
        retval->setNull();
        return;
    }
    retval->setDouble(info->iter->getDouble(info->handle));
}

static void _bindFieldInteger(kscript::ExprEnv*,
                              void* param,
                              kscript::Value* retval)
{
    CommonBaseIteratorBindInfo* info = (CommonBaseIteratorBindInfo*)param;
    if (info->iter->isNull(info->handle))
    {
        retval->setNull();
        return;
    }
    retval->setInteger(info->iter->getInteger(info->handle));
}

static void _bindFieldDateTime(kscript::ExprEnv*,
                               void* param,
                               kscript::Value* retval)
{
    CommonBaseIteratorBindInfo* info = (CommonBaseIteratorBindInfo*)param;
    if (info->iter->isNull(info->handle))
    {
        retval->setNull();
        return;
    }
    xd::datetime_t dt, d, t;
    dt = info->iter->getDateTime(info->handle);
    d = dt >> 32;
    t = dt & 0xffffffff;
    retval->setDateTime((unsigned int)d, (unsigned int)t);
}

static void _bindFieldBoolean(kscript::ExprEnv*,
                              void* param,
                              kscript::Value* retval)
{
    CommonBaseIteratorBindInfo* info = (CommonBaseIteratorBindInfo*)param;
    if (info->iter->isNull(info->handle))
    {
        retval->setNull();
        return;
    }
    retval->setBoolean(info->iter->getBoolean(info->handle));
}

xd::IStructurePtr CommonBaseIterator::getParserStructure()
{
    // default is to just use the normal iterator structure
    // as source fields for the expression.  This can, however,
    // be overridden by the derived class.  Just as long as the field
    // handles can be gotten from getHandle with the names
    // from this structure
    
    return getStructure();
}





















bool CommonBaseIterator::refreshRelInfo(CommonBaseIteratorRelInfo& info)
{
    xd::IDatabase* database = cmniterGetDatabase();

    // make sure we can get a relationship schema
    if (m_relschema.isNull())
    {
        m_relschema = database;
        if (m_relschema.isNull())
            return false;
    }


    // free old expression
    if (info.kl)
    {
        delete info.kl;
        info.kl = NULL;
    }
    info.tag = L"";
    



    xd::IRelationPtr rel;

    // try to find the correct relation id
    xd::IRelationEnumPtr rel_enum = m_relschema->getRelationEnum(getTable());
    size_t i, rel_count = rel_enum->size();
    for (i = 0; i < rel_count; ++i)
    {
        rel = rel_enum->getItem(i);
        if (rel->getRelationId() == info.relation_id)
        {
            info.tag = rel->getTag();
            break;
        }
    }

    if (info.tag.length() == 0)
        return false;

    xd::IStructurePtr right_structure = database->describeTable(rel->getRightTable());
    if (right_structure.isNull())
        return false;

    // lookup the index on the right set
    xd::IIndexInfoEnumPtr idx_enum = database->getIndexEnum(rel->getRightTable());
    xd::IIndexInfoPtr idx = xdLookupIndex(idx_enum, rel->getRightExpression(), false);
    if (!idx)
        return false;

    // construct final left expression.  This will be based off of the order
    // of the fields in the right set's index expression.  The order of the
    // fields in the index expression may be arbitrary, for example:
    // field1, field2 -or- field2, field1

    std::vector<std::wstring> left_list, right_list, idx_list;

    kl::parseDelimitedList(rel->getLeftExpression(), left_list, L',');
    kl::parseDelimitedList(rel->getRightExpression(), right_list, L',');
    kl::parseDelimitedList(idx->getExpression(), idx_list, L',');

    if (left_list.size() != right_list.size() ||
        right_list.size() != idx_list.size())
    {
        return false;
    }

    size_t x, j, count = left_list.size();


    info.kl = new KeyLayout;
    info.kl->setIterator(static_cast<xd::IIterator*>(this), false);


    for (x = 0; x < count; ++x)
    {
        for (j = 0; j < count; ++j)
        {
            if (!wcscasecmp(right_list[j].c_str(), idx_list[x].c_str()))
            {
                xd::IColumnInfoPtr colinfo;

                colinfo = right_structure->getColumnInfo(right_list[j]);
                if (colinfo.isNull())
                {
                    delete info.kl;
                    info.kl = NULL;
                    return false;
                }

                info.kl->addKeyPart(left_list[j],
                                    colinfo->getType(),
                                    colinfo->getWidth());

                break;
            }
        }    
    }

    if (info.right_iter.isNull())
    {
        info.right_iter = database->query(rel->getRightTable(), L"", L"", idx->getExpression(), NULL);
        info.right_iter_int = info.right_iter;
    }

    return true;
}




class CommonAggregateResult
{
public:
   
    CommonAggregateResult()
    {
        m_expr = L"";
        m_agg_func = GroupFunc_None;
        m_ref_count = 1;
        m_valid_rowid = 0;
        m_handle = 0;
    }

    ~CommonAggregateResult()
    {
    }

    bool init(xd::IDatabase* database,
              const std::wstring& path,
              int agg_func,
              const std::wstring& expr)
    {
        xd::IRelationSchemaPtr schema = database;
        if (schema.isNull())
            return false;

        if (path.empty())
            return false;

        m_agg_func = agg_func;

        std::wstring column = L"";

        // find out the link tag
        if (m_agg_func == GroupFunc_Count)
        {
            m_link_tag = expr;
            kl::trim(m_link_tag);
            dequote(m_link_tag, '[', ']');

            if (m_link_tag.empty())
                return false;
        }
         else
        {
            const wchar_t* pstr = expr.c_str();
            const wchar_t* pperiod = zl_strchr((wchar_t*)pstr, '.', L"[", L"]");

            int period_pos = pperiod ? (pperiod-pstr) : -1;

            // if no period was found, or it's in the wrong
            // place, the parse was bad
            if (period_pos <= 0)
                return false;

            m_link_tag.assign(expr.c_str(), period_pos);
            kl::trim(m_link_tag);
            dequote(m_link_tag, '[', ']');

            column = expr.substr(period_pos+1);
            kl::trim(column);
            dequote(column, '[', ']');
        }

        xd::IRelationEnumPtr rel_enum;
        xd::IRelationPtr rel;

        rel_enum = schema->getRelationEnum(path);
        size_t i, rel_count = rel_enum->size();
        for (i = 0; i < rel_count; ++i)
        {
            rel = rel_enum->getItem(i);
            if (kl::iequals(rel->getTag(), m_link_tag))
                break;
        }

        if (i >= rel_count)
            return false;

        if (agg_func == GroupFunc_Count)
        {
            m_expr_type = xd::typeNumeric;
            
            // if it's just a count, we're done
            return true;
        }



        xd::IStructurePtr s = database->describeTable(rel->getRightTable());
        if (s.isNull())
            return false;

        xd::IColumnInfoPtr colinfo = s->getColumnInfo(column);
        if (colinfo.isNull())
            return false;

        m_expr = column;
        m_expr_type = colinfo->getType();

        return true;
    }

public:

    int m_ref_count;

    xd::objhandle_t m_handle;

    std::wstring m_link_tag;
    std::wstring m_expr;
    int m_expr_type;
    int m_agg_func;

    std::wstring m_str_result;
    double m_dbl_result;
    bool m_bool_result;
    xd::datetime_t m_dt_result;

    // rowid for which this result is valid
    xd::rowid_t m_valid_rowid;
};



class CommonAggregateExprElement : public kscript::ExprElement
{
public:
    CommonAggregateResult* m_agg_result;
    CommonBaseIterator* m_iter;

    CommonAggregateExprElement(CommonBaseIterator* iter, CommonAggregateResult* agg_res)
    {
        m_iter = iter;
        m_agg_result = agg_res;
    }

    ~CommonAggregateExprElement()
    {
        if (m_iter && m_agg_result)
        {
            m_iter->releaseAggResultObject(m_agg_result);
        }
    }

    int eval(kscript::ExprEnv*, kscript::Value* retval)
    {
        if (m_agg_result->m_valid_rowid != m_iter->getRowId())
        {
            m_iter->recalcAggResults();
        }

        switch (m_agg_result->m_expr_type)
        {
            case xd::typeWideCharacter:
            case xd::typeCharacter:
                retval->setString(m_agg_result->m_str_result.c_str());
                break;
            case xd::typeNumeric:
            case xd::typeDouble:
            case xd::typeInteger:
                retval->setDouble(m_agg_result->m_dbl_result);
                break;
            case xd::typeBoolean:
                retval->setBoolean(m_agg_result->m_bool_result);
                break;
            case xd::typeDate:
            case xd::typeDateTime:
                retval->setDateTime((unsigned int)(m_agg_result->m_dt_result >> 32),
                                    (unsigned int)(m_agg_result->m_dt_result & 0xffffffff));
                break;
        }

        return kscript::errorNone;
    }

    int getType()
    {
        switch (m_agg_result->m_expr_type)
        {
            default:
            case xd::typeInvalid:       return kscript::Value::typeNull;
            case xd::typeUndefined:     return kscript::Value::typeUndefined;
            case xd::typeBoolean:       return kscript::Value::typeBoolean;
            case xd::typeNumeric:       return kscript::Value::typeDouble;
            case xd::typeInteger:       return kscript::Value::typeInteger;
            case xd::typeDouble:        return kscript::Value::typeDouble;
            case xd::typeCharacter:     return kscript::Value::typeString;
            case xd::typeWideCharacter: return kscript::Value::typeString;
            case xd::typeDateTime:      return kscript::Value::typeDateTime;
            case xd::typeDate:          return kscript::Value::typeDateTime;
            case xd::typeBinary:        return kscript::Value::typeBinary;
        }
    }
};


struct CommonBaseIteratorParseHookInfo
{
    CommonBaseIterator* iter;
    xd::IStructurePtr structure;
};


// static
bool CommonBaseIterator::script_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    CommonBaseIterator* iter = ((CommonBaseIteratorParseHookInfo*)hook_info.hook_param)->iter;

    if (hook_info.element_type == kscript::ExprParseHookInfo::typeIdentifier)
    {
        xd::IStructurePtr& structure = ((CommonBaseIteratorParseHookInfo*)hook_info.hook_param)->structure;
        
        if (hook_info.expr_text.length() > 1 &&
            hook_info.expr_text[0] == '[' &&
            hook_info.expr_text[hook_info.expr_text.length()-1] == ']')
        {
            // remove brackets from beginning and end e.g. [Field Name] => Field Name
            hook_info.expr_text = hook_info.expr_text.substr(0, hook_info.expr_text.length()-1);
            hook_info.expr_text.erase(0, 1);
        }


        xd::IColumnInfoPtr colinfo = structure->getColumnInfo(hook_info.expr_text);
        if (colinfo.isNull())
            return false;
        
        xd::objhandle_t handle = iter->getHandle(hook_info.expr_text);
        
        if (handle == 0)
            return false;
       
        CommonBaseIteratorBindInfo* bind_info = new CommonBaseIteratorBindInfo;
        bind_info->iter = iter;
        bind_info->handle = handle;
        iter->m_bindings.push_back(bind_info); // free up the object later
        
        
        kscript::Value* val = new kscript::Value;
        switch (colinfo->getType())
        {
            case xd::typeCharacter:
            case xd::typeWideCharacter:
                val->setGetVal(kscript::Value::typeString,
                               _bindFieldString,
                               (void*)bind_info);
                break;

            case xd::typeNumeric:
            case xd::typeDouble:
                val->setGetVal(kscript::Value::typeDouble,
                               _bindFieldDouble,
                               (void*)bind_info);
                break;

            case xd::typeInteger:
                val->setGetVal(kscript::Value::typeInteger,
                               _bindFieldInteger,
                               (void*)bind_info);
                break;

            case xd::typeDate:
            case xd::typeDateTime:
                val->setGetVal(kscript::Value::typeDateTime,
                               _bindFieldDateTime,
                               (void*)bind_info);
                break;

            case xd::typeBoolean:
                val->setGetVal(kscript::Value::typeBoolean,
                               _bindFieldBoolean,
                               (void*)bind_info);
                break;
            
            default:
                delete bind_info;
                delete val;
                return false;
        }
        
        hook_info.res_element = val;
        return true;
    }
     else if (hook_info.element_type == kscript::ExprParseHookInfo::typeFunction)
    {
        size_t len = hook_info.expr_text.length();
        if (len == 0)
            return false;

        const wchar_t* expr_text = hook_info.expr_text.c_str();

        if (*(expr_text+len-1) != ')')
            return false;

        const wchar_t* open_paren = wcschr(expr_text, L'(');
        if (!open_paren)
            return false;

        open_paren++;
        while (iswspace(*open_paren))
            open_paren++;

        std::wstring expr;
        expr.assign(open_paren, (expr_text + len - open_paren - 1));
        kl::trimRight(expr);

        int agg_func = GroupFunc_None;
        if (!wcsncasecmp(expr_text, L"FIRST(", 6))
            agg_func = GroupFunc_First;
        else if (!wcsncasecmp(expr_text, L"LAST(", 5))
            agg_func = GroupFunc_Last;
        else if (!wcsncasecmp(expr_text, L"MIN(", 4))
            agg_func = GroupFunc_Min;
        else if (!wcsncasecmp(expr_text, L"MAX(", 4))
            agg_func = GroupFunc_Max;
        else if (!wcsncasecmp(expr_text, L"SUM(", 4))
            agg_func = GroupFunc_Sum;
        else if (!wcsncasecmp(expr_text, L"AVG(", 4))
            agg_func = GroupFunc_Avg;
        else if (!wcsncasecmp(expr_text, L"COUNT(", 6))
            agg_func = GroupFunc_Count;
        else if (!wcsncasecmp(expr_text, L"STDDEV(", 7))
            agg_func = GroupFunc_Stddev;
        else if (!wcsncasecmp(expr_text, L"VARIANCE(", 9))
            agg_func = GroupFunc_Variance;
        else if (!wcsncasecmp(expr_text, L"MERGE(", 6))
            agg_func = GroupFunc_Merge;

        if (agg_func == GroupFunc_None)
            return false;


        CommonAggregateResult* agg_res = iter->getAggResultObject(agg_func, expr);
        if (!agg_res)
            return false;

        CommonAggregateExprElement* e = new CommonAggregateExprElement(iter, agg_res);
        hook_info.res_element = static_cast<kscript::ExprElement*>(e);

        return true;
    }
    
    return false;
}



CommonAggregateResult* CommonBaseIterator::getAggResultObject(int agg_func, const std::wstring& expr)
{
    // first, try to see if a suitable aggregate result object exists
    std::vector<CommonAggregateResult*>::iterator it;
    for (it = m_aggregate_results.begin();
         it != m_aggregate_results.end(); ++it)
    {
        if ((*it)->m_agg_func == agg_func &&  kl::iequals((*it)->m_expr, expr))
        {
            (*it)->m_ref_count++;
            return (*it);
        }
    }

    xd::IDatabase* db = cmniterGetDatabase();

    if (m_relschema.isNull())
        m_relschema = db;    

    // no suitable aggregate result object was found, so initialize a new one
    CommonAggregateResult* agg_res = new CommonAggregateResult;
    if (!agg_res->init(db, getTable(), agg_func, expr))
    {
        delete agg_res;
        return NULL;
    }
    
    m_aggregate_results.push_back(agg_res);
    
    return agg_res;
}

void CommonBaseIterator::releaseAggResultObject(CommonAggregateResult* agg_res)
{
    std::vector<CommonAggregateResult*>::iterator it;
    it = std::find(m_aggregate_results.begin(),
                   m_aggregate_results.end(),
                   agg_res);
    
    if (it == m_aggregate_results.end())
        return;
    
    (*it)->m_ref_count--;
    if ((*it)->m_ref_count == 0)
    {
        delete (*it);
        m_aggregate_results.erase(it);
    }

}


xd::IIteratorPtr CommonBaseIterator::getChildIterator(xd::IRelationPtr relation)
{
    CommonBaseIteratorRelInfo* info = NULL;

    // lookup the relationship info
    std::vector<CommonBaseIteratorRelInfo>::iterator it;
    for (it = m_relations.begin(); it != m_relations.end(); ++it)
    {
        if (it->relation_id == relation->getRelationId())
        {
            info = &(*it);
            break;
        }
    }

    // if the relationship info was not found, attempt
    // to initialize a new structure and add it to our
    // m_relations array

    if (!info || !info->kl)
    {
        CommonBaseIteratorRelInfo i;
        i.relation_id = relation->getRelationId();
        i.kl = NULL;

        if (!refreshRelInfo(i))
        {
            return xcm::null;
        }

        m_relations.push_back(i);
        info = &m_relations.back();
    }

    // sanity check
    if (!info->kl)
        return xcm::null;

    // get the left key
    const unsigned char* left_key = info->kl->getKey();
    int left_keylen = info->kl->getKeyLength();

    // if the left key was truncated at all, that means that no record
    // can be found on the right side which satisfies the left expression

    if (info->kl->getTruncation())
        return xcm::null;

    info->right_iter_int->setKeyFilter(NULL, 0);

    if (!info->right_iter->seek(left_key, left_keylen, false))
        return xcm::null;
    
    return info->right_iter;
}


xd::IIteratorPtr CommonBaseIterator::getFilteredChildIterator(xd::IRelationPtr relation)
{
    CommonBaseIteratorRelInfo* info = NULL;

    // lookup the relationship info
    std::vector<CommonBaseIteratorRelInfo>::iterator it;
    for (it = m_relations.begin(); it != m_relations.end(); ++it)
    {
        if (it->relation_id == relation->getRelationId())
        {
            info = &(*it);
            break;
        }
    }

    // if the relationship info was not found, attempt to initialize
    // a new structure and add it to our m_relations array

    if (!info || !info->kl)
    {
        CommonBaseIteratorRelInfo i;
        i.relation_id = relation->getRelationId();
        i.kl = NULL;

        if (!refreshRelInfo(i))
            return xcm::null;

        m_relations.push_back(i);
        info = &m_relations.back();
    }

    // sanity check
    if (!info->kl)
        return xcm::null;

    // get the left key
    const unsigned char* left_key = info->kl->getKey();
    int left_keylen = info->kl->getKeyLength();

    // if the left key was truncated at all, that means that no record
    // can be found on the right side which satisfies the left expression

    if (info->kl->getTruncation())
        return xcm::null;

    // check if the iterator is already positioned on the current key

    const void* cur_key_filter;
    int cur_key_filter_len;
    info->right_iter_int->getKeyFilter(&cur_key_filter, &cur_key_filter_len);
    if (cur_key_filter && cur_key_filter_len == left_keylen)
    {
        if (memcmp(cur_key_filter, left_key, left_keylen) == 0)
        {
            info->right_iter->goFirst();
            return info->right_iter;
        }
    }

    info->right_iter_int->setKeyFilter(NULL, 0);

    if (!info->right_iter->seek(left_key, left_keylen, false))
        return xcm::null;

    info->right_iter_int->setKeyFilter(left_key, left_keylen);
    info->right_iter_int->setFirstKey();

    return info->right_iter;
}

void CommonBaseIterator::recalcAggResults()
{
    if (m_relschema.isNull())
        return;

    if (m_aggregate_results.size() == 0)
        return;

    std::vector<CommonAggregateResult*>::iterator it, it_end = m_aggregate_results.end();

    xd::rowid_t rowid = getRowId();

    for (it = m_aggregate_results.begin(); it != it_end; ++it)
    {
        // initialize group results
        (*it)->m_str_result = L"";
        (*it)->m_dbl_result = 0.0;
        (*it)->m_bool_result = false;
        (*it)->m_dt_result = 0;

        (*it)->m_valid_rowid = rowid;
    }


    if (m_relenum.isNull())
        m_relenum = m_relschema->getRelationEnum(getTable());

    size_t rel_count = m_relenum->size();
    if (rel_count == 0)
        return;


    std::vector<CommonAggregateResult*> results;
    results.resize(m_aggregate_results.size());

    int r, result_count;

    xd::IRelationPtr rel;
    xd::IIteratorPtr sp_iter;
    xd::IIterator* iter;

    bool only_first;
    xd::objhandle_t obj_handle;




    for (size_t i = 0; i < rel_count; ++i)
    {
        rel = m_relenum->getItem(i);
        
        only_first = true;

        result_count = 0;

        for (it = m_aggregate_results.begin(); it != it_end; ++it)
        {
            if (kl::iequals((*it)->m_link_tag, rel->getTag()))
            {
                if ((*it)->m_agg_func != GroupFunc_First)
                    only_first = false;

                results[result_count] = (*it);
                result_count++;
            }
        }

        if (result_count == 0)
            continue;

        sp_iter = getFilteredChildIterator(rel);
        if (sp_iter.isNull())
            continue;

        iter = sp_iter.p;

        if (iter->eof())
            continue;
        
        if (only_first)
        {
            for (r = 0; r < result_count; ++r)
            {
                if (!results[r]->m_handle)
                {
                    results[r]->m_handle = iter->getHandle(results[r]->m_expr);
                }

                obj_handle = results[r]->m_handle;

                if (!obj_handle)
                    continue;

                switch (results[r]->m_expr_type)
                {
                    case xd::typeWideCharacter:
                    case xd::typeCharacter:
                        results[r]->m_str_result = iter->getWideString(obj_handle);
                        break;
                    case xd::typeNumeric:
                    case xd::typeInteger:
                    case xd::typeDouble:
                        results[r]->m_dbl_result = iter->getDouble(obj_handle);
                        break;
                    case xd::typeBoolean:
                        results[r]->m_bool_result = iter->getBoolean(obj_handle);
                        break;
                    case xd::typeDate:
                    case xd::typeDateTime:
                        results[r]->m_dt_result = iter->getDateTime(obj_handle);
                        break;
                }
            }
        }
         else
        {
            bool first_row = true;
            double group_row_count = 0.0;
            kl::statistics stats;

            iter->goFirst();
            while (!iter->eof())
            {
                group_row_count++;

                for (r = 0; r < result_count; ++r)
                {
                    if (!results[r]->m_handle && results[r]->m_agg_func != GroupFunc_Count)
                    {
                        results[r]->m_handle = iter->getHandle(results[r]->m_expr);
                    }
                    obj_handle = results[r]->m_handle;

                    switch (results[r]->m_agg_func)
                    {
                        case GroupFunc_Min:
                        case GroupFunc_Max:
                        {
                            std::wstring str_result;
                            double dbl_result;
                            bool bool_result;
                            xd::datetime_t dt_result;
                            int agg_func = results[r]->m_agg_func;

                            switch (results[r]->m_expr_type)
                            {
                                case xd::typeWideCharacter:
                                case xd::typeCharacter:
                                    str_result = iter->getWideString(obj_handle);

                                    if (first_row ||
                                        (agg_func == GroupFunc_Min && str_result < results[r]->m_str_result) ||
                                        (agg_func == GroupFunc_Max && str_result > results[r]->m_str_result))
                                    {
                                        results[r]->m_str_result = str_result;
                                    }
                                    break;
                                case xd::typeNumeric:
                                case xd::typeInteger:
                                case xd::typeDouble:
                                    dbl_result = iter->getDouble(obj_handle);
                                    if (first_row ||
                                        (agg_func == GroupFunc_Min && dbl_result < results[r]->m_dbl_result) ||
                                        (agg_func == GroupFunc_Max && dbl_result > results[r]->m_dbl_result))
                                    {
                                        results[r]->m_dbl_result = dbl_result;
                                    }
                                    break;
                                case xd::typeBoolean:
                                    bool_result = iter->getBoolean(obj_handle);
                                    if (first_row ||
                                        (agg_func == GroupFunc_Min && !bool_result) ||
                                        (agg_func == GroupFunc_Max && bool_result))
                                    {
                                        results[r]->m_bool_result = bool_result;
                                    }
                                    break;
                                case xd::typeDate:
                                case xd::typeDateTime:
                                    dt_result = iter->getDateTime(obj_handle);
                                    if (first_row ||
                                        (agg_func == GroupFunc_Min && dt_result < results[r]->m_dt_result) ||
                                        (agg_func == GroupFunc_Max && dt_result > results[r]->m_dt_result))
                                    {
                                        results[r]->m_dt_result = dt_result;
                                    }
                                    break;
                            }
                        }
                        break;

                        case GroupFunc_First:
                            if (!first_row)
                                break;
                        
                        case GroupFunc_Last:
                            switch (results[r]->m_expr_type)
                            {
                                case xd::typeWideCharacter:
                                case xd::typeCharacter:
                                    results[r]->m_str_result = iter->getWideString(obj_handle);
                                    break;
                                case xd::typeNumeric:
                                case xd::typeInteger:
                                case xd::typeDouble:
                                    results[r]->m_dbl_result = iter->getDouble(obj_handle);
                                    break;
                                case xd::typeBoolean:
                                    results[r]->m_bool_result = iter->getBoolean(obj_handle);
                                    break;
                                case xd::typeDate:
                                case xd::typeDateTime:
                                    results[r]->m_dt_result = iter->getDateTime(obj_handle);
                                    break;
                            }
                            break;

                        case GroupFunc_Avg:
                        case GroupFunc_Sum:
                            results[r]->m_dbl_result += iter->getDouble(obj_handle);
                            break;

                        case GroupFunc_Stddev:
                        case GroupFunc_Variance:
                            stats.push(iter->getDouble(obj_handle));
                            break;

                        case GroupFunc_Merge:
                            results[r]->m_str_result += iter->getWideString(obj_handle);
                            break;
                    }
                }

                first_row = false;
                iter->skip(1);
            }

            for (r = 0; r < result_count; ++r)
            {
                switch (results[r]->m_agg_func)
                {
                    case GroupFunc_Count:
                        results[r]->m_dbl_result = group_row_count;
                        break;
                    case GroupFunc_Avg:
                        if (kl::dblcompare(group_row_count, 0.0) == 0)
                            results[r]->m_dbl_result = 0.0;
                             else
                            results[r]->m_dbl_result /= group_row_count;
                        break;
                    case GroupFunc_Stddev:
                        results[r]->m_dbl_result = stats.stddev();
                        break;
                    case GroupFunc_Variance:
                        results[r]->m_dbl_result = stats.variance();
                        break;
                }
            }
        }
    }
}



kscript::ExprParser* CommonBaseIterator::parse(const std::wstring& expr)
{
    if (expr.length() == 0)
        return NULL;
        
    kscript::ExprParser* parser = createExprParser();


    CommonBaseIteratorParseHookInfo info;
    info.iter = this;
    info.structure = getParserStructure();
    
    parser->setParseHook(kscript::ExprParseHookInfo::typeIdentifier | kscript::ExprParseHookInfo::typeFunction,
                         &CommonBaseIterator::script_parse_hook,
                         (void*)&info);
    
    if (!parser->parse(expr))
    {
        delete parser;
        return NULL;
    }

    return parser;
}




