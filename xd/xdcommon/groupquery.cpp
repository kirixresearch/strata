/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-02-21
 *
 */


#include <set>
#include <ctime>
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/math.h>
#include <xd/xd.h>
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/keylayout.h"
#include "../xdcommon/util.h"
#include "../../kscript/kscript.h"


namespace xdcommon
{



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



static void getDefaultExprWidthAndScale(const std::wstring& expr, int type, int* width, int* scale)
{
    // default
    *width = 10;
    *scale = 0;
    
    // TODO: for an expression, we don't always know what the width
    // or scale are, so use reasonable values
    switch (type)
    {
        case xd::typeCharacter:
        case xd::typeWideCharacter:
        {
            *width = 160;
            *scale = 0;
        }
        break;

        case xd::typeDouble:
        {
            *width = 8;
            *scale = 4;
        }
        break;
            
        case xd::typeNumeric:
        {
            *width = 18;
            *scale = 4;
        }
        break;

        case xd::typeInteger:
        {
            *width = 4;
            *scale = 0;
        }
        break;

        case xd::typeBoolean:
        {
            *width = 1;
            *scale = 0;
        }
        break;

        case xd::typeDate:
        {
            *width = 4;
            *scale = 0;
        }                        
        break;

        case xd::typeDateTime:
        {
            *width = 8;
            *scale = 0;
        }
        break;
    }
}

class GroupResult
{
public:
    int m_group_func;
    std::wstring m_param_text;

    std::string m_val_string;
    std::wstring m_val_wstring;
    double m_val_double;
    bool m_val_bool;
    xd::datetime_t m_val_datetime;
    
    std::vector<std::wstring> m_arr_string;

    int m_param_type;
    int m_param_width;
    
    int m_offset;
    int m_type;
    int m_width;
    int m_scale;
    xd::objhandle_t m_handle;
};


class GroupIndexInputInfo
{
public:
    
    xd::objhandle_t m_input_handle;
    std::wstring m_name;
    int m_type;
    int m_width;
    int m_scale;
    int m_offset;
};


class GroupOutputInfo
{
public:

    // info for the output file
    xd::objhandle_t m_output_handle;  // handle for the row inserter
    std::wstring m_name;                 // field name
    int m_type;                          // field type
    int m_width;                         // field width
    int m_scale;                         // field scale
    

    std::wstring m_detail_source_field;  // detail source field
    int m_store_offset;                  // source store offset (-1 if not used)
    GroupResult* m_result;               // source group result (NULL if not used)
    xd::objhandle_t m_input_handle;   // source handle (0 if not used)
};


class GroupResultExprElement : public kscript::ExprElement
{
public:
    
    GroupResult* m_result;

    int eval(kscript::ExprEnv*, kscript::Value* retval)
    {
        switch (m_result->m_type)
        {
            case xd::typeCharacter:
            {
                std::wstring s = kl::towstring(m_result->m_val_string);
                retval->setString(s.c_str());
            }
            break;

            case xd::typeWideCharacter:
                retval->setString(m_result->m_val_wstring.c_str());
                break;
            case xd::typeNumeric:
            case xd::typeDouble:
            case xd::typeInteger:
                retval->setDouble(m_result->m_val_double);
                break;
            case xd::typeBoolean:
                retval->setBoolean(m_result->m_val_bool);
                break;
            case xd::typeDate:
            case xd::typeDateTime:
                retval->setDateTime((unsigned int)(m_result->m_val_datetime >> 32),
                                    (unsigned int)(m_result->m_val_datetime & 0xffffffff));
                break;
        }

        return kscript::errorNone;
    }

    int getType()
    {
        switch (m_result->m_type)
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


class GroupQueryParams
{

public:

    GroupQueryParams()
    {
        m_store_size = 0;
    }

    ~GroupQueryParams()
    {
        std::vector<GroupResult*>::iterator it;
        
        for (it = m_results.begin(); it != m_results.end(); ++it)
        {
            delete (*it);
        }

        std::vector<kscript::ExprParser*>::iterator dit;
        for (dit = m_to_destroy.begin(); dit != m_to_destroy.end(); ++dit)
        {
            delete (*dit);
        }
    }

    GroupResult* getResultObject(const std::wstring& _expr)
    {
        int group_func;

        std::wstring expr = _expr;
        kl::trim(expr);

        std::wstring func_name;
        func_name = kl::beforeFirst(expr, L'(');
        kl::trim(func_name);
        kl::makeUpper(func_name);

        if (func_name == L"FIRST")
            group_func = GroupFunc_First;
        else if (func_name == L"LAST")
            group_func = GroupFunc_Last;
        else if (func_name == L"MIN")
            group_func = GroupFunc_Min;
        else if (func_name == L"MAX")
            group_func = GroupFunc_Max;
        else if (func_name == L"SUM")
            group_func = GroupFunc_Sum;
        else if (func_name == L"AVG")
            group_func = GroupFunc_Avg;
        else if (func_name == L"COUNT")
            group_func = GroupFunc_Count;
        else if (func_name == L"STDDEV")
            group_func = GroupFunc_Stddev;
        else if (func_name == L"VARIANCE")
            group_func = GroupFunc_Variance;            
        else if (func_name == L"MERGE")
            group_func = GroupFunc_Merge;            
        else if (func_name == L"GROUPID")
            group_func = GroupFunc_GroupID;
        else if (func_name == L"MAXDISTANCE")
            group_func = GroupFunc_MaxDistance;
        else return NULL;
        
        
        std::wstring param;
        param = kl::afterFirst(expr, L'(');
        if (param.find_last_of(L')') == -1)
            return NULL;
        param = kl::beforeLast(param, L')');

        kl::trim(param);

        // now attempt to find an existing, suitable result object

        std::vector<GroupResult*>::iterator it;
        for (it = m_results.begin(); it != m_results.end(); ++it)
        {
            if ((*it)->m_group_func == group_func &&
                !wcscasecmp((*it)->m_param_text.c_str(), param.c_str()))
            {
                return *it;
            }
        }

        // get the parameter expression type
        int param_type = m_db->validateExpression(param, m_set_structure).type;

        // check if the parameter is a valid column
        if (group_func != GroupFunc_Count &&
            group_func != GroupFunc_GroupID)
        {
            if (param_type == xd::typeInvalid)
                return NULL;
        }

        // certain aggregate functions demand a numeric parameter
        if (group_func == GroupFunc_Sum ||
            group_func == GroupFunc_Avg ||
            group_func == GroupFunc_Stddev ||
            group_func == GroupFunc_Variance)
        {
            if (param_type == xd::typeInvalid)
                return NULL;

            if (param_type != xd::typeInteger &&
                param_type != xd::typeDouble &&
                param_type != xd::typeNumeric)
            {
                return NULL;
            }
        }
        
        // not found, so create a new group result object
        GroupResult* gr = new GroupResult;
        gr->m_param_text = param;
        gr->m_group_func = group_func;

        m_results.push_back(gr);
        return gr;
    }






    // index value store functions
    bool addStoreCountField(xd::IIteratorPtr iter,
                            const std::wstring& count_filter,
                            int* out_offset)
    {
        // get the handle of the count condition
        xd::objhandle_t h = 0;
        
        if (count_filter.length() > 0)
        {
            h = iter->getHandle(count_filter);

            if (h)
            {
                GroupIndexInputInfo ii;
                ii.m_input_handle = h;
                ii.m_name = L"";
                ii.m_type = xd::typeBoolean;
                ii.m_width = 1;
                ii.m_scale = 0;
                ii.m_offset = m_store_size;

                m_store_fields.push_back(ii);

                if (out_offset)
                {
                    *out_offset = m_store_size;
                }

                m_store_size += 1;
                
                return true;
            }
        }
        
        return false;
    }

    bool addOrLookupStoreField(xd::IIteratorPtr iter,
                               const std::wstring& _field,
                               int* out_offset)
    {
        std::wstring field = _field;
        
        // if the 'field' parameter represented a valid column name, quoted or not,
        // just use that in its dequoted form to avoid parsing it as an expression
        // (this solves a problem of type confusion between date and datetime)
        // problem sql was: select inv_date from ap_hist group by inv_date
        std::wstring deq_field = field;
        dequote(deq_field, '[', ']');
        if (m_iter_structure.getColumnExist(deq_field))
            field = deq_field;


        std::wstring uppercase_field = field;
        kl::makeUpper(uppercase_field);

        if (m_unique_store_fields.find(uppercase_field) == m_unique_store_fields.end())
        {
            m_unique_store_fields.insert(uppercase_field);

            xd::objhandle_t handle = iter->getHandle(field);
            if (!handle)
                return false;

            GroupIndexInputInfo ii;
            ii.m_input_handle = handle;
            ii.m_offset = m_store_size;

            const xd::ColumnInfo& colinfo = m_iter_structure.getColumnInfo(field);
            if (colinfo.isOk())
            {
                // if we have a regular column, get the info
                ii.m_name = colinfo.name;
                ii.m_type = colinfo.type;
                ii.m_width = colinfo.width;
                ii.m_scale = colinfo.scale;
            }
            else            
            {
                // the "field" is actually an expression; get the expression
                // type and suitable widths/scales depending on the expression
                ii.m_name = field;
                ii.m_type = iter->getType(handle);
                getDefaultExprWidthAndScale(field, ii.m_type, &ii.m_width, &ii.m_scale);
            }

            if (out_offset)
            {
                *out_offset = m_store_size;
            }
            
            
            if (ii.m_type == xd::typeWideCharacter)
            {
                m_store_size += (ii.m_width * 2);
            }
             else if (ii.m_type == xd::typeNumeric)
            {
                m_store_size += sizeof(double);
            }
             else if (ii.m_type == xd::typeDateTime)
            {
                m_store_size += 8;
            }
             else if (ii.m_type == xd::typeDate)
            {
                m_store_size += 4;
            }
             else
            {
                m_store_size += ii.m_width;
            }

            m_store_fields.push_back(ii);
        }
         else
        {
            // lookup the offset
            std::vector<GroupIndexInputInfo>::iterator sf_it;
            for (sf_it = m_store_fields.begin();
                 sf_it != m_store_fields.end();
                 ++sf_it)
            {
                if (0 == wcscasecmp(sf_it->m_name.c_str(), field.c_str()))
                {
                    if (out_offset)
                    {
                        *out_offset = sf_it->m_offset;
                    }

                    break;
                }
            }
        }
        
        return true;
    }
    
    
    void releaseStoreFieldHandles(xd::IIteratorPtr iter)
    {
        std::vector<GroupIndexInputInfo>::iterator it;
        for (it = m_store_fields.begin(); it != m_store_fields.end(); ++it)
            iter->releaseHandle(it->m_input_handle);
    }


public:

    std::vector<GroupResult*> m_results;
    
    std::vector<GroupIndexInputInfo> m_store_fields;
    std::set<std::wstring> m_unique_store_fields;
    int m_store_size;
    
    GroupResult* m_last_result;
    xd::Structure m_set_structure;
    xd::Structure m_iter_structure;
    std::vector<kscript::ExprParser*> m_to_destroy;

    xd::IDatabase* m_db;
};




bool group_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    std::wstring text = hook_info.expr_text;

    if (hook_info.element_type == kscript::ExprParseHookInfo::typeIdentifier)
    {
        std::wstring upper_expr_text = hook_info.expr_text;
        kl::makeUpper(upper_expr_text);
        
        wchar_t ch = 0;
        if (hook_info.expr_text.length() > 0)
            ch = hook_info.expr_text[0];
        
        if (ch == L'"' || ch == L'\'' || iswdigit(ch) || upper_expr_text == L"TRUE" || upper_expr_text == L"FALSE")
        {
            // it's a constant -- we want to let the
            // expression parser handle it
            return false;
        }

    
        // identifiers by themselves are considered FIRST()
        std::wstring deq = hook_info.expr_text;
        dequote(deq, '[', ']');

        text = L"FIRST([";
        text += deq;
        text += L"])";
    }



    GroupQueryParams* info = (GroupQueryParams*)hook_info.hook_param;
    
    GroupResult* result =  info->getResultObject(text);
    
    if (!result)
    {
        return false;
    }

    result->m_param_type = 0;
    result->m_param_width = 0;
    

    // try to look up column info
    if (result->m_group_func == GroupFunc_Count ||
        result->m_group_func == GroupFunc_GroupID)
    {
        // NUMERIC(10,0) to accomodate tables >= 10 billion rows
        result->m_type = xd::typeNumeric;
        result->m_width = 10;
        result->m_scale = 0;
    }
     else
    {
        std::wstring lookup_colname = result->m_param_text;
        dequote(lookup_colname, '[', ']');

        const xd::ColumnInfo& colinfo = info->m_set_structure.getColumnInfo(lookup_colname);

        if (colinfo.isNull())
        {
            result->m_type = xd::typeInvalid;
            result->m_type = info->m_db->validateExpression(result->m_param_text, info->m_set_structure).type;
            getDefaultExprWidthAndScale(result->m_param_text, result->m_type, &result->m_width, &result->m_scale);
        }
        else
        {
            result->m_type = colinfo.type;
            result->m_width = colinfo.width;
            result->m_scale = colinfo.scale;
        }


        result->m_param_type = result->m_type;
        result->m_param_width = result->m_width;
        
        
        if (result->m_group_func == GroupFunc_MaxDistance)
        {
            // DISTANCE() always returns a number
            result->m_type = xd::typeNumeric;
            result->m_width = 10;
            result->m_scale = 0;
        }

        // adjust the width if we're summing on a numeric type
        if (result->m_type == xd::typeNumeric && (result->m_group_func == GroupFunc_Sum ||
                                                     result->m_group_func == GroupFunc_Stddev ||
                                                     result->m_group_func == GroupFunc_Variance))
        {
            result->m_width = 18;
        }

         if (result->m_group_func == GroupFunc_Merge)
         {
            // CHAR(1024,0) to accomodate the merged values of a field
            // across the rows in a group
            result->m_width = 1024;
        }        
    }

    info->m_last_result = result;


    GroupResultExprElement* e = new GroupResultExprElement;
    e->m_result = result;
    hook_info.res_element = e;

    return result ? true : false;
}





class GroupInsertProgress : public IIndexProgress
{

public:

    void updateProgress(xd::rowpos_t cur_count,
                        xd::rowpos_t max_count,
                        bool* cancel)
    {
        if (cur_count == 0)
        {
            if (ijob)
            {
                ijob->startPhase();
            }
        }

        if (cur_count % 1 == 0)
        {
            if (ijob)
            {
                ijob->setCurrentCount(cur_count);
                if (job->getCancelled())
                {
                    *cancel = true;
                    cancelled = true;
                }
            }
        }

        if (cur_count % 10000 == 0)
        {
            if (xf_get_free_disk_space(filename) < 50000000)
            {
                if (ijob)
                {
                    ijob->setStatus(xd::jobFailed);
                    *cancel = true;
                    cancelled = true;
                }
            }
        }
    }

public:
    xd::IJob* job;
    IJobInternal* ijob;
    std::wstring filename;
    bool cancelled;
};




void buf2str(std::string& str, char* ptr, int len)
{
    for (int ii = 0; ii < len; ++ii)
    {
        if (!*(ptr+ii))
        {
            len = ii;
            break;
        }
    }
    
    str.assign(ptr, len);
}



bool runGroupQuery(xd::IDatabasePtr db, xd::GroupQueryParams* info, xd::IJob* job)
{
    std::wstring input = info->input;
    std::wstring group = info->group;
    std::wstring columns = info->columns;
    std::wstring where = info->where;
    std::wstring having = info->having;


    GroupQueryParams gi;
    gi.m_db = db.p;

    std::vector<GroupOutputInfo> output_fields;
    kscript::ExprParser* having_parser = NULL;
    bool copy_detail = false;
    bool detail_in_index = false;
    xd::IIteratorPtr sp_iter;
    xd::IIterator* iter;
    xd::Structure structure;
    xd::rowpos_t row_count = 0;


    xd::IFileInfoPtr finfo = db->getFileInfo(input);
    if (finfo.isNull())
        return false;

    // create an iterator for the input file

    xd::QueryParams qp;
    qp.from = input;

    sp_iter = db->query(qp);
    if (sp_iter.isNull())
    {
        // iterator can't be made for some reason
        // usually this is an out of disk space error
        return false;
    }
    

    iter = sp_iter.p;
    structure = iter->getStructure();
    gi.m_iter_structure = iter->getStructure();
    gi.m_set_structure = gi.m_iter_structure;

    // try to get the row count
    if (finfo->getFlags() & xd::sfFastRowCount)
    {
        row_count = finfo->getRowCount();
    }


    // parse output fields

    std::vector<std::wstring> output_columns;
    std::vector<std::wstring>::iterator outcol_it;
    kl::parseDelimitedList(columns, output_columns, L',', true);

    for (outcol_it = output_columns.begin();
         outcol_it != output_columns.end();
         ++outcol_it)
    {
        kl::trim(*outcol_it);


        if (!wcscasecmp(outcol_it->c_str(), L"[DETAIL]"))
        {
            if (copy_detail)
            {
                // '[DETAIL]' was specified more than once. return error
                return false;
            }

            copy_detail = true;

            
            // determine if the detail records must be stored
            // in the index's value side.  This is the case
            // when the iterator is forward only, or can't support goRow()
            if (iter->getIteratorFlags() & xd::ifForwardOnly)
            {
                detail_in_index = true;
            }

            size_t i, col_count = structure.getColumnCount();
            for (i = 0; i < col_count; ++i)
            {
                const xd::ColumnInfo& colinfo = structure.getColumnInfoByIdx(i);

                GroupOutputInfo of;
                of.m_detail_source_field = colinfo.name;
                of.m_result = NULL;
                of.m_store_offset = -1;
                of.m_input_handle = 0;
                of.m_output_handle = 0;
                of.m_name = colinfo.name;
                of.m_type = colinfo.type;
                of.m_width = colinfo.width;
                of.m_scale = colinfo.scale;

                output_fields.push_back(of);
            }
        }
         else if (*outcol_it == L"*")
        {
            // '*' is equal to the first() of every column

            size_t i, col_count = structure.getColumnCount();
            for (i = 0; i < col_count; ++i)
            {
                const xd::ColumnInfo& colinfo = structure.getColumnInfoByIdx(i);

                std::wstring part2 = L"FIRST([";
                part2 += colinfo.name;
                part2 += L"])";

                // parse a group expr - this will add a GroupResult object to |gi.m_results|
                kscript::ExprParser* parser = createExprParser();
                parser->setParseHook(kscript::ExprParseHookInfo::typeIdentifier |
                                     kscript::ExprParseHookInfo::typeFunction,
                                     group_parse_hook, &gi);
                gi.m_to_destroy.push_back(parser);

                if (!parser->parse(part2))
                {
                    return false;
                }

                // add an output field in the output file definition
                GroupOutputInfo of;
                of.m_name = colinfo.name;
                of.m_store_offset = -1;
                of.m_input_handle = 0;
                of.m_output_handle = 0;
                of.m_result = gi.m_last_result;
                of.m_type = gi.m_last_result->m_type;
                of.m_width = gi.m_last_result->m_width;
                of.m_scale = gi.m_last_result->m_scale;

                output_fields.push_back(of);
            }
        }
         else
        {
            std::wstring part1;
            std::wstring part2;

            // try to find an equals sign
            int eq_pos = outcol_it->find(L'=');
            
            // if it's just a field name, make a FIRST(<field_name>) out of it
            if (eq_pos == -1)
            {
                if (structure.getColumnExist(*outcol_it))
                {
                    part1 = *outcol_it;
                    part2 = L"FIRST([";
                    part2 += *outcol_it;
                    part2 += L"])";
                }
                 else
                {
                    // syntax error
                    return false;
                }
            }

            // parse the <output_field>=FUNC(<input_field>) expression
            if (part1.empty())
            {
                part1 = outcol_it->substr(0, eq_pos);
                part2 = outcol_it->substr(eq_pos+1);

                kl::trim(part1);
                kl::trim(part2);

                dequote(part1, '[', ']');
                
                dequoteIfField(structure, part2, '[', ']');

                if (structure.getColumnExist(part2))
                {
                    std::wstring temps;
                    temps = L"FIRST([";
                    temps += part2;
                    temps += L"])";
                    part2 = temps;
                }
            }


            // parse a group expr - this will add a GroupResult object to |gi.m_results|
            kscript::ExprParser* parser = createExprParser();
            parser->setParseHook(kscript::ExprParseHookInfo::typeIdentifier |
                                 kscript::ExprParseHookInfo::typeFunction,
                                 group_parse_hook, &gi);
            gi.m_to_destroy.push_back(parser);
            if (!parser->parse(part2))
            {
                return false;
            }
            
            
            // add an output field in the output file definition
            GroupOutputInfo of;
            of.m_name = part1;
            of.m_store_offset = -1;
            of.m_input_handle = 0;
            of.m_output_handle = 0;
            of.m_result = gi.m_last_result;
            of.m_type = gi.m_last_result->m_type;
            of.m_width = gi.m_last_result->m_width;
            of.m_scale = gi.m_last_result->m_scale;

            output_fields.push_back(of);
        }
    }


    // parse having expression
    if (having.length() > 0)
    {
        having_parser = createExprParser();
        having_parser->setParseHook(kscript::ExprParseHookInfo::typeIdentifier |
                             kscript::ExprParseHookInfo::typeFunction,
                             group_parse_hook, &gi);
        gi.m_to_destroy.push_back(having_parser);

        if (!having_parser->parse(having))
        {
            return false;
        }
    }
    
    
    // parse where (pre-filter) expression
    xd::objhandle_t where_handle = 0;

    if (!where.empty())
    {
        where_handle = iter->getHandle(where);
        if (iter->getType(where_handle) != xd::typeBoolean)
        {
            return false;
        }
    }


    // create a key layout for the index's key side
    
    KeyLayout kl;

    if (!group.empty())
    {
        if (!kl.setKeyExpr(sp_iter, group))
            return false;
    }


    // create the layout for the index value side by
    // going through all result objects

    std::vector<GroupResult*>::iterator it;

    if (copy_detail)
    {
        // if we are copying detail rows, we must copy the rowid into the
        // value side of the index entries; this will leave space for that

        gi.m_store_size += sizeof(xd::rowid_t);
    }



    for (it = gi.m_results.begin(); it != gi.m_results.end(); ++it)
    {
        (*it)->m_handle = 0;
        (*it)->m_offset = -1;

        if ((*it)->m_group_func == GroupFunc_GroupID)
        {
        }
         else if ((*it)->m_group_func == GroupFunc_Count)
        {
            std::wstring count_filter = (*it)->m_param_text;
            
            if (count_filter.length() > 0)
            {
                if (!group.empty())
                {
                    int store_offset = 0;

                    if (gi.addStoreCountField(iter, count_filter, &store_offset))
                    {
                        // this only happens if there was a valid count filter,
                        // otherwise addStoreCountField() does nothing
                        (*it)->m_offset = store_offset;
                    }
                }
                 else
                {
                    (*it)->m_handle = iter->getHandle(count_filter);
                }
            }
        }
         else
        {
            std::wstring field = (*it)->m_param_text;
            
            if (!group.empty())
            {
                int store_offset = 0;          
                
                if (gi.addOrLookupStoreField(iter, 
                                             field, 
                                             &store_offset))
                {
                    (*it)->m_offset = store_offset;
                }
            }
             else
            {
                (*it)->m_handle = iter->getHandle(field);
            }
        }
    }


    if (copy_detail)
    {
        std::vector<GroupOutputInfo>::iterator out_it;
        
        for (out_it = output_fields.begin();
             out_it != output_fields.end();
             ++out_it)
        {
            if (out_it->m_detail_source_field.length() > 0)
            {
                if (detail_in_index)
                {
                    // forward-only iterators put their
                    // detail in the index, because only
                    // one iteration is allowed
                    int store_offset = 0;
                    gi.addOrLookupStoreField(iter, out_it->m_detail_source_field, &store_offset);
                    out_it->m_store_offset = store_offset;
                }
                 else
                {
                    out_it->m_input_handle = iter->getHandle(out_it->m_detail_source_field);
                }
            }
        }
    }


    
    // create output structure
    xd::FormatDefinition output_struct;
    std::set<std::wstring> unique_output_fields;
    std::vector<GroupOutputInfo>::iterator out_it;

    for (out_it = output_fields.begin(); out_it != output_fields.end(); ++out_it)
    {
        // check to make sure that there are no duplicate output fields 
        std::wstring f = out_it->m_name;
        kl::makeUpper(f);
        if (unique_output_fields.find(f) != unique_output_fields.end())
        {
            // duplicate output field name found, no good
            return false;
        }
        unique_output_fields.insert(f);


        // create the output column
        xd::ColumnInfo info;
        
        info.name = out_it->m_name;
        info.type = out_it->m_type;
        info.width = out_it->m_width;
        info.scale = out_it->m_scale;

        output_struct.createColumn(info);
    }

    if (!db->createTable(info->output, output_struct))
        return false;

    xd::IRowInserterPtr output_inserter = db->bulkInsert(info->output);
    if (output_inserter.isNull())
        return false;
    if (!output_inserter->startInsert(L"*"))
        return false;



    for (out_it = output_fields.begin(); out_it != output_fields.end(); ++out_it)
    {
        out_it->m_output_handle = output_inserter->getHandle(out_it->m_name);
    }


    // initialize job information

    int current_row = 0;
    bool cancelled = false;
    bool failed = false;

    IJobInternalPtr ijob;
    if (job)
    {
        ijob = job;
        if (!ijob)
        {
            return false;
        }

        ijob->setStartTime(time(NULL));
        ijob->setStatus(xd::jobRunning);
        ijob->setMaxCount(row_count);
        ijob->setCurrentCount(0);

        if (!group.empty())
        {
            int phase_pcts[] = { 30, 30, 40 };
            ijob->setPhases(3, phase_pcts);
        }
    }




    // PHASE 1: first put keys and values into new index

    int keylen = 0;
    int vallen = 0;
    unsigned char* keybuf = NULL;
    unsigned char* valuebuf = NULL;

    std::wstring index_filename;

    ExIndex* idx = NULL;
    

    std::wstring temp_path = db->getAttributes()->getStringAttribute(xd::dbattrTempDirectory);

    if (!group.empty())
    {
        index_filename = temp_path;
        index_filename += PATH_SEPARATOR_STR;
        index_filename += kl::getUniqueString();
        index_filename += L".idx";


        idx = new ExIndex;
        idx->setTempFilePath(temp_path);


        keylen = kl.getKeyLength();
        vallen = gi.m_store_size;

        if (!idx->create(index_filename, keylen, vallen, true))
        {
            delete idx;
            return false;
        }

        if (ijob)
        {
            ijob->startPhase();
        }

        idx->startBulkInsert(row_count);

        keybuf = new unsigned char[keylen];
        valuebuf = new unsigned char[vallen];

        std::vector<GroupIndexInputInfo>::iterator sf_it;
        std::vector<GroupIndexInputInfo>::iterator begin_sf_it = gi.m_store_fields.begin();
        std::vector<GroupIndexInputInfo>::iterator end_sf_it = gi.m_store_fields.end();

        bool add_record;

        iter->goFirst();
        while (!iter->eof())
        {   
            add_record = true;

            if (where_handle)
            {
                if (!iter->getBoolean(where_handle))
                {
                    add_record = false;
                }
            }


            if (add_record)
            {
                if (copy_detail)
                {
                    xd::rowid_t rowid = iter->getRowId();
                    memcpy(valuebuf, &rowid, sizeof(rowid));
                }

                for (sf_it = begin_sf_it; sf_it != end_sf_it; ++sf_it)
                {
                    switch (sf_it->m_type)
                    {
                        case xd::typeCharacter:
                        {
                            const std::string& s = iter->getString(sf_it->m_input_handle);
                            memset(valuebuf+sf_it->m_offset, 0, sf_it->m_width);
                            memcpy(valuebuf+sf_it->m_offset, s.c_str(), s.length());
                        }
                        break;

                        case xd::typeWideCharacter:
                        {
                            const std::wstring& s = iter->getWideString(sf_it->m_input_handle);
                            kl::wstring2ucsle(valuebuf+sf_it->m_offset, s, sf_it->m_width);
                        }
                        break;

                        case xd::typeDouble:
                        case xd::typeNumeric:
                        {
                            double d = iter->getDouble(sf_it->m_input_handle);
                            memcpy(valuebuf+sf_it->m_offset,
                                    &d, sizeof(double));
                        }
                        break;

                        case xd::typeInteger:
                        {
                            int i = iter->getInteger(sf_it->m_input_handle);
                            memcpy(valuebuf+sf_it->m_offset,
                                    &i, sizeof(int));
                        }
                        break;

                        case xd::typeBoolean:
                        {
                            bool b = iter->getBoolean(sf_it->m_input_handle);
                            memcpy(valuebuf+sf_it->m_offset,
                                    &b, sizeof(bool));
                        }
                        break;

                        case xd::typeDate:
                        {
                            xd::datetime_t dt = iter->getDateTime(sf_it->m_input_handle);
                            unsigned int d = ((unsigned int)(dt >> 32));
                            memcpy(valuebuf+sf_it->m_offset,
                                    &d, sizeof(int));
                        }
                        break;

                        case xd::typeDateTime:
                        {
                            xd::datetime_t dt = iter->getDateTime(sf_it->m_input_handle);
                            memcpy(valuebuf+sf_it->m_offset,
                                    &dt, sizeof(xd::datetime_t));
                        }
                        break;
                    }
                }

                if (idx->insert(kl.getKey(), keylen,
                               valuebuf, vallen) != idxErrSuccess)
                {
                    idx->cancelBulkInsert();
                    failed = true;
                    break;
                }
            }

            if ((current_row % 1) == 0 && job)
            {
                ijob->setCurrentCount(current_row);
                if (job->getCancelled())
                {
                    idx->cancelBulkInsert();
                    cancelled = true;
                    break;
                }
            }

            iter->skip(1);
            current_row++;
        }

        if (cancelled || failed)
        {
            if (job)
            {
                if (cancelled)
                    ijob->setStatus(xd::jobCancelled);

                if (failed)
                    ijob->setStatus(xd::jobFailed);
            }

            delete[] keybuf;
            delete[] valuebuf;

            return false;
        }


        GroupInsertProgress gip;
        gip.job = job;
        gip.ijob = ijob;
        gip.filename = index_filename;
        gip.cancelled = false;

        idx->finishBulkInsert(&gip);

        if (gip.cancelled)
        {
            if (job)
            {
                ijob->setStatus(xd::jobCancelled);
            }

            delete[] keybuf;
            delete[] valuebuf;

            return false;
        }
    }

    gi.releaseStoreFieldHandles(iter);

    if (where_handle)
    {
        iter->releaseHandle(where_handle);
    }




    // PHASE 2: iterator through the index and process groups

    int group_id = 1;
    int group_row_count = 0;
    bool first_row = true;
    bool group_changed = false;
    cancelled = false;
    bool eof;

    kl::statistics stats;

    unsigned char* valptr = NULL;

    std::string val_string;
    std::wstring val_wstring;
    double val_double;
    bool val_bool;
    xd::datetime_t val_datetime;

    std::vector<GroupResult*>::iterator begin_it = gi.m_results.begin();
    std::vector<GroupResult*>::iterator end_it = gi.m_results.end();


    IIndexIterator* idx_iter = NULL;
    if (job && idx)
    {
        ijob->startPhase();
    }

    if (idx)
    {
        idx_iter = idx->createIterator();
        idx_iter->goFirst();
        eof = idx_iter->isEof();
        if (!eof)
        {
            memcpy(keybuf, idx_iter->getKey(), keylen);
        }
    }
     else
    {
        iter->goFirst();
        eof = iter->eof();
    }

    
    // if we are already at eof, then this is an empty file
    if (eof)
    {
        return true;
    }



    current_row = 0;

    for (it = begin_it; it != end_it; ++it)
    {
        (*it)->m_val_double = 0.0;
        (*it)->m_arr_string.clear();
    }

    while (!eof)
    {
        if (idx)
        {
            valptr = (unsigned char*)idx_iter->getValue();
        }

        for (it = begin_it; it != end_it; ++it)
        {
            if ((*it)->m_group_func == GroupFunc_Count)
            {
                if ((*it)->m_offset != -1)
                {
                    // in the index store, a one-byte flag stores
                    // whether the count condition was true
                    // for that record
                    val_bool = *((bool*)(valptr+(*it)->m_offset));
                }
                 else if ((*it)->m_handle != 0)
                {
                    val_bool = iter->getBoolean((*it)->m_handle);
                }
                 else
                {
                    val_bool = true;
                }
            }
             else if ((*it)->m_group_func == GroupFunc_GroupID)
            {
            }
             else if (idx)
            {
                switch ((*it)->m_param_type)
                {
                    case xd::typeCharacter:
                        buf2str(val_string, (char*)valptr+(*it)->m_offset, (*it)->m_param_width);
                        break;
                    case xd::typeWideCharacter:
                        kl::ucsle2wstring(val_wstring, valptr+(*it)->m_offset, (*it)->m_param_width);
                        break;
                    case xd::typeNumeric:
                    case xd::typeDouble:
                        val_double = *((double*)(valptr+(*it)->m_offset));
                        break;
                    case xd::typeInteger:
                        val_double = *((int*)(valptr+(*it)->m_offset));
                        break;
                    case xd::typeDate:
                        val_datetime = *((unsigned int*)(valptr+(*it)->m_offset));
                        val_datetime <<= 32;
                        break;
                    case xd::typeDateTime:
                        val_datetime = *((xd::datetime_t*)(valptr+(*it)->m_offset));
                        break;
                    case xd::typeBoolean:
                        val_bool = *((bool*)(valptr+(*it)->m_offset));
                        break;
                }
            }
             else
            {
                switch ((*it)->m_param_type)
                {
                    case xd::typeCharacter:
                        val_string = iter->getString((*it)->m_handle);
                        break;
                    case xd::typeWideCharacter:
                        val_wstring = iter->getWideString((*it)->m_handle);
                        break;
                    case xd::typeInteger:
                    case xd::typeNumeric:
                    case xd::typeDouble:
                        val_double = iter->getDouble((*it)->m_handle);
                        break;
                    case xd::typeDate:
                    case xd::typeDateTime:
                        val_datetime = iter->getDateTime((*it)->m_handle);
                        break;
                    case xd::typeBoolean:
                        val_bool = iter->getBoolean((*it)->m_handle);
                        break;
                }
            }


            switch ((*it)->m_group_func)
            {
                case GroupFunc_Count:
                {
                    if (val_bool)
                    {
                        (*it)->m_val_double++;
                    }
                }
                break;

                case GroupFunc_GroupID:
                {
                    if (first_row)
                    {
                        (*it)->m_val_double = group_id;
                    }
                }
                break;

                case GroupFunc_Last:
                case GroupFunc_First:
                {
                    if (first_row || (*it)->m_group_func == GroupFunc_Last)
                    {
                        switch ((*it)->m_type)
                        {
                            case xd::typeCharacter:
                                (*it)->m_val_string = val_string;
                                break;
                            case xd::typeWideCharacter:
                                (*it)->m_val_wstring = val_wstring;
                                break;
                            case xd::typeInteger:
                            case xd::typeNumeric:
                            case xd::typeDouble:
                                (*it)->m_val_double = val_double;
                                break;
                            case xd::typeDate:
                            case xd::typeDateTime:
                                (*it)->m_val_datetime = val_datetime;
                                break;
                            case xd::typeBoolean:
                                (*it)->m_val_bool = val_bool;
                                break;
                        }
                    }
                }
                break;

                case GroupFunc_Merge:
                {
                    switch ((*it)->m_param_type)
                    {
                        case xd::typeCharacter:
                            (*it)->m_val_string += val_string;
                            break;

                        case xd::typeWideCharacter:
                            (*it)->m_val_wstring += val_wstring;
                            break;
                    }
                }
                break;
                
                case GroupFunc_MaxDistance:
                {
                    switch ((*it)->m_param_type)
                    {
                        case xd::typeCharacter:
                            (*it)->m_arr_string.push_back(kl::towstring(val_string));
                            break;

                        case xd::typeWideCharacter:
                            (*it)->m_arr_string.push_back(val_wstring);
                            break;
                    }
                }
                break;

                case GroupFunc_Min:
                case GroupFunc_Max:
                case GroupFunc_Sum:
                case GroupFunc_Avg:
                case GroupFunc_Stddev:
                case GroupFunc_Variance:
                {
                    switch ((*it)->m_param_type)
                    {
                        case xd::typeCharacter:
                            if ((*it)->m_group_func == GroupFunc_Min)
                            {
                                if (first_row || (*it)->m_val_string > val_string)
                                    (*it)->m_val_string = val_string;
                            }
                             else if ((*it)->m_group_func == GroupFunc_Max)
                            {
                                if (first_row || (*it)->m_val_string < val_string)
                                    (*it)->m_val_string = val_string;
                            }
                            break;

                        case xd::typeWideCharacter:
                            if ((*it)->m_group_func == GroupFunc_Min)
                            {
                                if (first_row || (*it)->m_val_wstring > val_wstring)
                                    (*it)->m_val_wstring = val_wstring;
                            }
                             else if ((*it)->m_group_func == GroupFunc_Max)
                            {
                                if (first_row || (*it)->m_val_wstring < val_wstring)
                                    (*it)->m_val_wstring = val_wstring;
                            }
                            break;

                        case xd::typeInteger:
                        case xd::typeNumeric:
                        case xd::typeDouble:
                            if ((*it)->m_group_func == GroupFunc_Min)
                            {
                                if (first_row || (*it)->m_val_double > val_double)
                                    (*it)->m_val_double = val_double;
                            }
                             else if ((*it)->m_group_func == GroupFunc_Max)
                            {
                                if (first_row || (*it)->m_val_double < val_double)
                                    (*it)->m_val_double = val_double;
                            }
                             else
                            {
                                // sum and avg will accumulate
                                (*it)->m_val_double += val_double;
                                
                                // accumulate values for stddev and variance
                                stats.push(val_double);
                            }
                            break;

                        case xd::typeDate:
                        case xd::typeDateTime:
                            if ((*it)->m_group_func == GroupFunc_Min)
                            {
                                if (first_row || (*it)->m_val_datetime > val_datetime)
                                    (*it)->m_val_datetime = val_datetime;
                            }
                             else if ((*it)->m_group_func == GroupFunc_Max)
                            {
                                if (first_row || (*it)->m_val_datetime < val_datetime)
                                    (*it)->m_val_datetime = val_datetime;
                            }
                            break;

                        case xd::typeBoolean:
                            if ((*it)->m_group_func == GroupFunc_Min)
                            {
                                if (first_row || (*it)->m_val_bool > val_bool)
                                    (*it)->m_val_bool = val_bool;
                            }
                             else if ((*it)->m_group_func == GroupFunc_Max)
                            {
                                if (first_row || (*it)->m_val_bool < val_bool)
                                    (*it)->m_val_bool = val_bool;
                            }

                            break;
                    }
                }
                break;
            }
        }

        
        group_row_count++;
        first_row = false;

        if (idx)
        {
            idx_iter->skip(1);
            eof = idx_iter->isEof();

            if (!eof)
            {
                if (0 != memcmp(keybuf, idx_iter->getKey(), keylen))
                {
                    group_changed = true;
                    first_row = true;
                    memcpy(keybuf, idx_iter->getKey(), keylen);
                }
            }
        }
         else
        {
            iter->skip(1);
            eof = iter->eof();
        }




        if (group_changed || eof)
        {
            group_changed = false;

            for (it = begin_it; it != end_it; ++it)
            {
                if ((*it)->m_group_func == GroupFunc_Avg)
                {
                    (*it)->m_val_double /= group_row_count;
                }
                 else if ((*it)->m_group_func == GroupFunc_MaxDistance)
                {
                    // calculate max distance (levenstein) -- unfortunately O(n^2)
                    int td, distance = 0;
                    
                    std::vector<std::wstring>::iterator it1,it2,arrbegin,arrend;
                    arrbegin = (*it)->m_arr_string.begin();
                    arrend = (*it)->m_arr_string.end();
                    for (it1 = arrbegin; it1 != arrend; ++it1)
                    {
                        for (it2 = arrbegin; it2 != arrend; ++it2)
                        {
                            // don't compare anything to 'ourself'
                            if (it2 != it1)
                            {
                                td = levenshtein(it1->c_str(), it2->c_str());
                                if (td > distance)
                                    distance = td;
                            }
                        }
                    }
                    
                    (*it)->m_val_double = distance;
                }
                 else if ((*it)->m_group_func == GroupFunc_Stddev)
                {
                    (*it)->m_val_double = stats.stddev();
                }
                 else if ((*it)->m_group_func == GroupFunc_Variance)
                {
                    (*it)->m_val_double = stats.variance();
                }
            }


            // if there is a 'having expression', evaluate it
            // to see if the group's results belong in the output file
            
            bool copy_result_to_output = true;

            if (having_parser)
            {
                kscript::Value val;
                having_parser->eval(&val);

                if (!val.getBoolean())
                    copy_result_to_output = false;
            }


            // put results in output file, if 'having expression' was true
            // (or if there wasn't any 'having expression'
            
            if (copy_result_to_output)
            {
                // increment group id for the next group
                group_id += 1;

                if (!copy_detail)
                {
                    // normal summary output; output one line per each group
                    // which contains the summary information of the detail
                    
                    for (out_it = output_fields.begin(); out_it != output_fields.end(); ++out_it)
                    {
                        switch (out_it->m_type)
                        {
                            case xd::typeCharacter:
                                output_inserter->putString(out_it->m_output_handle, out_it->m_result->m_val_string);
                                break;
                            case xd::typeWideCharacter:
                                output_inserter->putWideString(out_it->m_output_handle, out_it->m_result->m_val_wstring);
                                break;
                            case xd::typeDouble:
                            case xd::typeNumeric:
                                output_inserter->putDouble(out_it->m_output_handle, out_it->m_result->m_val_double);
                                break;
                            case xd::typeInteger:
                                output_inserter->putInteger(out_it->m_output_handle, (int)out_it->m_result->m_val_double);
                                break;
                            case xd::typeDate:
                            case xd::typeDateTime:
                                output_inserter->putDateTime(out_it->m_output_handle, out_it->m_result->m_val_datetime);
                                break;
                            case xd::typeBoolean:
                                output_inserter->putBoolean(out_it->m_output_handle, out_it->m_result->m_val_bool);
                                break;
                        }
                    }

                    output_inserter->insertRow();
                }
                 else
                {
                    // need to copy the detail rows
                    if (idx)
                    {
                        idx_iter->skip(-group_row_count);
                    }
                     else
                    {
                        iter->skip(-group_row_count);
                    }

                    for (int i = 0; i < group_row_count; ++i)
                    {
                        if (idx)
                        {
                            valptr = (unsigned char*)idx_iter->getValue();
                            
                            if (!detail_in_index)
                            {
                                xd::rowid_t* rowid = (xd::rowid_t*)valptr;
                                iter->goRow(*rowid);
                            }
                        }

                        for (out_it = output_fields.begin(); out_it != output_fields.end(); ++out_it)
                        {
                            if (out_it->m_result)
                            {
                                switch (out_it->m_type)
                                {
                                    case xd::typeCharacter:
                                        output_inserter->putString(out_it->m_output_handle, out_it->m_result->m_val_string);
                                        break;
                                    case xd::typeWideCharacter:
                                        output_inserter->putWideString(out_it->m_output_handle, out_it->m_result->m_val_wstring);
                                        break;
                                    case xd::typeDouble:
                                    case xd::typeNumeric:
                                        output_inserter->putDouble(out_it->m_output_handle, out_it->m_result->m_val_double);
                                        break;
                                    case xd::typeInteger:
                                        output_inserter->putInteger(out_it->m_output_handle, (int)out_it->m_result->m_val_double);
                                        break;
                                    case xd::typeDate:
                                    case xd::typeDateTime:
                                        output_inserter->putDateTime(out_it->m_output_handle, out_it->m_result->m_val_datetime);
                                        break;
                                    case xd::typeBoolean:
                                        output_inserter->putBoolean(out_it->m_output_handle, out_it->m_result->m_val_bool);
                                        break;
                                }
                            }
                             else if (out_it->m_input_handle)
                            {
                                switch (out_it->m_type)
                                {
                                    case xd::typeCharacter:
                                        output_inserter->putString(out_it->m_output_handle, iter->getString(out_it->m_input_handle));
                                        break;
                                    case xd::typeWideCharacter:
                                        output_inserter->putWideString(out_it->m_output_handle, iter->getWideString(out_it->m_input_handle));
                                        break;
                                    case xd::typeDouble:
                                    case xd::typeNumeric:
                                        output_inserter->putDouble(out_it->m_output_handle, iter->getDouble(out_it->m_input_handle));
                                        break;
                                    case xd::typeInteger:
                                        output_inserter->putInteger(out_it->m_output_handle, iter->getInteger(out_it->m_input_handle));
                                        break;
                                    case xd::typeDate:
                                    case xd::typeDateTime:
                                        output_inserter->putDateTime(out_it->m_output_handle, iter->getDateTime(out_it->m_input_handle));
                                        break;
                                    case xd::typeBoolean:
                                        output_inserter->putBoolean(out_it->m_output_handle, iter->getBoolean(out_it->m_input_handle));
                                        break;
                                }
                            }
                             else if (out_it->m_store_offset != -1)
                            {
                                switch (out_it->m_type)
                                {
                                    case xd::typeCharacter:
                                        buf2str(val_string, (char*)valptr+out_it->m_store_offset, out_it->m_width);
                                        output_inserter->putString(out_it->m_output_handle, val_string);
                                        break;
                                    case xd::typeWideCharacter:
                                        kl::ucsle2wstring(val_wstring, valptr+out_it->m_store_offset, out_it->m_width);
                                        output_inserter->putWideString(out_it->m_output_handle, val_wstring);
                                        break;
                                    case xd::typeNumeric:
                                    case xd::typeDouble:
                                        val_double = *((double*)(valptr+out_it->m_store_offset));
                                        output_inserter->putDouble(out_it->m_output_handle, val_double);
                                        break;
                                    case xd::typeInteger:
                                        output_inserter->putInteger(out_it->m_output_handle, *((int*)(valptr+out_it->m_store_offset)));
                                        break;
                                    case xd::typeDate:
                                        val_datetime = *((unsigned int*)(valptr+out_it->m_store_offset));
                                        val_datetime <<= 32;
                                        output_inserter->putDateTime(out_it->m_output_handle, val_datetime);
                                        break;
                                    case xd::typeDateTime:
                                        val_datetime = *((xd::datetime_t*)(valptr+out_it->m_store_offset));
                                        output_inserter->putDateTime(out_it->m_output_handle, val_datetime);
                                        break;
                                    case xd::typeBoolean:
                                        val_bool = *((bool*)(valptr+out_it->m_store_offset));
                                        output_inserter->putBoolean(out_it->m_output_handle, val_bool);
                                        break;
                                }
                            }
                        }

                        output_inserter->insertRow();
                        
                        if (idx)
                        {
                            idx_iter->skip(1);
                        }
                         else
                        {
                            iter->skip(1);
                        }
                    }
                }
            }

            // clear out values in preparation for the next group
            first_row = true;

            for (it = begin_it; it != end_it; ++it)
            {
                (*it)->m_val_string = "";
                (*it)->m_val_wstring = L"";
                (*it)->m_val_double = 0.0;
                (*it)->m_arr_string.clear();
                
                // clear out the statistics accumulator
                stats.init();
            }

            group_row_count = 0;
        }


        // increment row counter and update job info
        current_row++;

        if ((current_row % 1) == 0 && job)
        {
            ijob->setCurrentCount(current_row);
            if (job->getCancelled())
            {
                cancelled = true;
                break;
            }
        }
    }

    delete[] keybuf;
    delete[] valuebuf;

    output_inserter->finishInsert();
    output_inserter.clear();

    if (idx)
    {
        idx_iter->unref();
        idx->unref();
        xf_remove(index_filename);
    }


    if (cancelled)
    {
        db->deleteFile(info->output);
        return false;
    }
     else
    {
        // set job to finished
        if (job)
        {
            ijob->setCurrentCount(current_row);
            ijob->setStatus(xd::jobFinished);
            ijob->setFinishTime(time(NULL));
        }
    }

    return true;
}




};
