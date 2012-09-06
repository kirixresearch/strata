/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-02-21
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <set>
#include <ctime>
#include <kl/portable.h>
#include <kl/math.h>
#include "tango.h"
#include "database.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/keylayout.h"
#include "../xdcommon/util.h"
#include "../../kscript/kscript.h"


static void getDefaultExprWidthAndScale(const std::wstring& expr, int type, int* width, int* scale)
{
    // default
    *width = 10;
    *scale = 0;
    
    // TODO: for an expression, we don't always know what the width
    // or scale are, so use reasonable values
    switch (type)
    {
        case tango::typeCharacter:
        case tango::typeWideCharacter:
        {
            *width = 160;
            *scale = 0;
        }
        break;

        case tango::typeDouble:
        {
            *width = 8;
            *scale = 4;
        }
        break;
            
        case tango::typeNumeric:
        {
            *width = 18;
            *scale = 4;
        }
        break;

        case tango::typeInteger:
        {
            *width = 4;
            *scale = 0;
        }
        break;

        case tango::typeBoolean:
        {
            *width = 1;
            *scale = 0;
        }
        break;

        case tango::typeDate:
        {
            *width = 4;
            *scale = 0;
        }                        
        break;

        case tango::typeDateTime:
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
    tango::datetime_t m_val_datetime;
    
    std::vector<std::wstring> m_arr_string;

    int m_param_type;
    int m_param_width;
    
    int m_offset;
    int m_type;
    int m_width;
    int m_scale;
    tango::objhandle_t m_handle;
};


class GroupIndexInputInfo
{
public:
    
    tango::objhandle_t m_input_handle;
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
    tango::objhandle_t m_output_handle;  // handle for the row inserter
    std::wstring m_name;                 // field name
    int m_type;                          // field type
    int m_width;                         // field width
    int m_scale;                         // field scale
    

    std::wstring m_detail_source_field;  // detail source field
    int m_store_offset;                  // source store offset (-1 if not used)
    GroupResult* m_result;               // source group result (NULL if not used)
    tango::objhandle_t m_input_handle;   // source handle (0 if not used)
};


class GroupResultExprElement : public kscript::ExprElement
{
public:
    
    GroupResult* m_result;

    int eval(kscript::ExprEnv*, kscript::Value* retval)
    {
        switch (m_result->m_type)
        {
            case tango::typeCharacter:
            {
                std::wstring s = kl::towstring(m_result->m_val_string);
                retval->setString(s.c_str());
            }
            break;

            case tango::typeWideCharacter:
                retval->setString(m_result->m_val_wstring.c_str());
                break;
            case tango::typeNumeric:
            case tango::typeDouble:
            case tango::typeInteger:
                retval->setDouble(m_result->m_val_double);
                break;
            case tango::typeBoolean:
                retval->setBoolean(m_result->m_val_bool);
                break;
            case tango::typeDate:
            case tango::typeDateTime:
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
            case tango::typeInvalid:       return kscript::Value::typeNull; 
            case tango::typeUndefined:     return kscript::Value::typeUndefined;
            case tango::typeBoolean:       return kscript::Value::typeBoolean;
            case tango::typeNumeric:       return kscript::Value::typeDouble;
            case tango::typeInteger:       return kscript::Value::typeInteger;
            case tango::typeDouble:        return kscript::Value::typeDouble;
            case tango::typeCharacter:     return kscript::Value::typeString;
            case tango::typeWideCharacter: return kscript::Value::typeString;
            case tango::typeDateTime:      return kscript::Value::typeDateTime;
            case tango::typeDate:          return kscript::Value::typeDateTime;
            case tango::typeBinary:        return kscript::Value::typeBinary;
        }
    }
};


class GroupQueryInfo
{

public:

    GroupQueryInfo()
    {
        m_store_size = 0;
    }

    ~GroupQueryInfo()
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
        dequote(param, '[', ']');

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
        int param_type = m_set_structure->getExprType(param);

        // check if the parameter is a valid column
        if (group_func != GroupFunc_Count &&
            group_func != GroupFunc_GroupID)
        {
            if (param_type == tango::typeInvalid)
                return NULL;
        }

        // certain aggregate functions demand a numeric parameter
        if (group_func == GroupFunc_Sum ||
            group_func == GroupFunc_Avg ||
            group_func == GroupFunc_Stddev ||
            group_func == GroupFunc_Variance)
        {
            if (param_type == tango::typeInvalid)
                return NULL;

            if (param_type != tango::typeInteger &&
                param_type != tango::typeDouble &&
                param_type != tango::typeNumeric)
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
    bool addStoreCountField(tango::IIteratorPtr iter,
                            const std::wstring& count_filter,
                            int* out_offset)
    {
        // get the handle of the count condition
        tango::objhandle_t h = 0;
        
        if (count_filter.length() > 0)
        {
            h = iter->getHandle(count_filter);

            if (h)
            {
                GroupIndexInputInfo ii;
                ii.m_input_handle = h;
                ii.m_name = L"";
                ii.m_type = tango::typeBoolean;
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

    bool addOrLookupStoreField(tango::IIteratorPtr iter,
                               const std::wstring& field,
                               int* out_offset)
    {
        std::wstring uppercase_field = field;
        kl::makeUpper(uppercase_field);

        if (m_unique_store_fields.find(uppercase_field) == m_unique_store_fields.end())
        {
            m_unique_store_fields.insert(uppercase_field);

            tango::objhandle_t handle = iter->getHandle(field);
            if (!handle)
                return false;

            GroupIndexInputInfo ii;
            ii.m_input_handle = handle;
            ii.m_offset = m_store_size;

            tango::IColumnInfoPtr colinfo = m_iter_structure->getColumnInfo(field);
            if (colinfo.isOk())
            {
                // if we have a regular column, get the info
                ii.m_name = colinfo->getName();
                ii.m_type = colinfo->getType();
                ii.m_width = colinfo->getWidth();
                ii.m_scale = colinfo->getScale();
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
            
            
            if (ii.m_type == tango::typeWideCharacter)
            {
                m_store_size += (ii.m_width * 2);
            }
             else if (ii.m_type == tango::typeNumeric)
            {
                m_store_size += sizeof(double);
            }
             else if (ii.m_type == tango::typeDateTime)
            {
                m_store_size += 8;
            }
             else if (ii.m_type == tango::typeDate)
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
    
    
    void releaseStoreFieldHandles(tango::IIteratorPtr iter)
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
    tango::IStructurePtr m_set_structure;
    tango::IStructurePtr m_iter_structure;
    std::vector<kscript::ExprParser*> m_to_destroy;
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

        std::wstring expr_text = hook_info.expr_text;
        dequote(expr_text, '[', ']');
        
        // identifiers by themselves are considered FIRST()
        text = L"FIRST([";
        text += expr_text;
        text += L"])";
    }



    GroupQueryInfo* info = (GroupQueryInfo*)hook_info.hook_param;
    
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
        result->m_type = tango::typeNumeric;
        result->m_width = 10;
        result->m_scale = 0;
    }
     else
    {
        std::wstring lookup_colname = result->m_param_text;
        dequote(lookup_colname, '[', ']');

        tango::IColumnInfoPtr colinfo;
        colinfo = info->m_set_structure->getColumnInfo(lookup_colname);

        if (colinfo.isNull())
        {
            result->m_type = info->m_set_structure->getExprType(result->m_param_text);
            getDefaultExprWidthAndScale(result->m_param_text, result->m_type, &result->m_width, &result->m_scale);
        }
        else
        {
            result->m_type = colinfo->getType();
            result->m_width = colinfo->getWidth();
            result->m_scale = colinfo->getScale();
        }


        result->m_param_type = result->m_type;
        result->m_param_width = result->m_width;
        
        
        if (result->m_group_func == GroupFunc_MaxDistance)
        {
            // DISTANCE() always returns a number
            result->m_type = tango::typeNumeric;
            result->m_width = 10;
            result->m_scale = 0;
        }

        // adjust the width if we're summing on a numeric type
        if (result->m_type == tango::typeNumeric && (result->m_group_func == GroupFunc_Sum ||
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

    void updateProgress(tango::rowpos_t cur_count,
                        tango::rowpos_t max_count,
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
                    ijob->setStatus(tango::jobFailed);
                    *cancel = true;
                    cancelled = true;
                }
            }
        }
    }

public:
    tango::IJob* job;
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



tango::ISetPtr Database::runGroupQuery(tango::ISetPtr set,
                                       const std::wstring& group,
                                       const std::wstring& output,
                                       const std::wstring& where,
                                       const std::wstring& having,
                                       tango::IJob* job)
{

    if (group.empty() &&
        !where.empty())
    {
        // this scenario isn't supported yet
        return xcm::null;
    }


    // check if we are running this job on an xdclient mount
    std::wstring set_path = set->getObjectPath();
    std::wstring cstr, rpath;
    if (detectMountPoint(set_path, cstr, rpath))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;
        
        tango::IAttributesPtr attr = db->getAttributes();
        if (attr->getStringAttribute(tango::dbattrDatabaseUrl).substr(0,6) == L"sdserv")
        {
            tango::ISetPtr rset = db->openSet(rpath);
            if (rset.isOk())
            {
                tango::ISetPtr result_rset = db->runGroupQuery(rset, group, output, where, having, job);
                if (result_rset.isNull())
                    return xcm::null;

                std::wstring remote_output_path = L"/.temp/" + getUniqueString();
                if (!db->storeObject(result_rset, remote_output_path))
                    return xcm::null;

                std::wstring link_output_path;
                if (link_output_path.length() == 0)
                    link_output_path = L"/.temp/" + getUniqueString();

                if (!setMountPoint(link_output_path, cstr, remote_output_path))
                    return xcm::null;

                return openSet(link_output_path);
            }
        }
    }




    GroupQueryInfo gi;
    std::vector<GroupOutputInfo> output_fields;
    kscript::ExprParser* having_parser = NULL;
    bool copy_detail = false;
    bool detail_in_index = false;
    tango::IIteratorPtr sp_iter;
    tango::IIterator* iter;
    tango::IStructurePtr structure;
    tango::rowpos_t row_count = 0;


    // create an iterator for the input file

    sp_iter = set->createIterator(L"", L"", NULL);
    if (sp_iter.isNull())
    {
        // iterator can't be made for some reason
        // usually this is an out of disk space error
        return xcm::null;
    }
    

    iter = sp_iter.p;
    structure = iter->getStructure();
    gi.m_iter_structure = iter->getStructure();
    gi.m_set_structure = set->getStructure();

    // try to get the row count
    if (set->getSetFlags() & tango::sfFastRowCount)
    {
        row_count = set->getRowCount();
    }


    // parse output fields

    std::vector<std::wstring> output_columns;
    std::vector<std::wstring>::iterator outcol_it;
    kl::parseDelimitedList(output, output_columns, L',', true);

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
                return xcm::null;
            }

            copy_detail = true;

            
            // determine if the detail records must be stored
            // in the index's value side.  This is the case
            // when the iterator is forward only, or can't support goRow()
            if (iter->getIteratorFlags() & tango::ifForwardOnly)
            {
                detail_in_index = true;
            }

            int colcount = structure->getColumnCount();
            for (int i = 0; i < colcount; ++i)
            {
                tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);

                GroupOutputInfo of;
                of.m_detail_source_field = colinfo->getName();
                of.m_result = NULL;
                of.m_store_offset = -1;
                of.m_input_handle = 0;
                of.m_output_handle = 0;
                of.m_name = colinfo->getName();
                of.m_type = colinfo->getType();
                of.m_width = colinfo->getWidth();
                of.m_scale = colinfo->getScale();

                output_fields.push_back(of);
            }
        }
         else if (*outcol_it == L"*")
        {
            // '*' is equal to the first() of every column

            tango::IColumnInfoPtr colinfo;

            int colcount = structure->getColumnCount();
            for (int i = 0; i < colcount; ++i)
            {
                colinfo = structure->getColumnInfoByIdx(i);

                std::wstring part2 = L"FIRST([";
                part2 += colinfo->getName();
                part2 += L"])";

                // parse a group expr - this will add a GroupResult object to |gi.m_results|
                kscript::ExprParser* parser = createExprParser();
                parser->setParseHook(kscript::ExprParseHookInfo::typeIdentifier |
                                     kscript::ExprParseHookInfo::typeFunction,
                                     group_parse_hook, &gi);
                gi.m_to_destroy.push_back(parser);

                if (!parser->parse(part2))
                {
                    return xcm::null;
                }

                // add an output field in the output file definition
                GroupOutputInfo of;
                of.m_name = colinfo->getName();
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
                if (structure->getColumnExist(*outcol_it))
                {
                    part1 = *outcol_it;
                    part2 = L"FIRST([";
                    part2 += *outcol_it;
                    part2 += L"])";
                }
                 else
                {
                    // syntax error
                    return xcm::null;
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
                
                if (structure->getColumnExist(part2))
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
                return xcm::null;
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
            return xcm::null;
        }
    }
    
    
    // parse where (pre-filter) expression
    tango::objhandle_t where_handle = 0;

    if (!where.empty())
    {
        where_handle = iter->getHandle(where);
        if (iter->getType(where_handle) != tango::typeBoolean)
        {
            return xcm::null;
        }
    }


    // create a key layout for the index's key side
    
    KeyLayout kl;

    if (!group.empty())
    {
        if (!kl.setKeyExpr(sp_iter, group))
            return xcm::null;
    }


    // create the layout for the index value side by
    // going through all result objects

    std::vector<GroupResult*>::iterator it;

    if (copy_detail)
    {
        // if we are copying detail rows, we must copy the rowid into the
        // value side of the index entries; this will leave space for that

        gi.m_store_size += sizeof(tango::rowid_t);
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

                    if (gi.addStoreCountField(iter,
                                              count_filter,
                                              &store_offset))
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
    tango::IStructurePtr output_struct = createStructure();
    std::set<std::wstring> unique_output_fields;
    std::vector<GroupOutputInfo>::iterator out_it;

    for (out_it = output_fields.begin();
         out_it != output_fields.end();
         ++out_it)
    {
        // check to make sure that there are no duplicate output fields 
        std::wstring f = out_it->m_name;
        kl::makeUpper(f);
        if (unique_output_fields.find(f) != unique_output_fields.end())
        {
            // duplicate output field name found, no good
            return xcm::null;
        }
        unique_output_fields.insert(f);


        // create the output column
        tango::IColumnInfoPtr info = output_struct->createColumn();
        info->setName(out_it->m_name);
        info->setType(out_it->m_type);
        info->setWidth(out_it->m_width);
        info->setScale(out_it->m_scale);
    }

    tango::ISetPtr output_set = createSet(L"", output_struct, NULL);
    if (output_set.isNull())
        return xcm::null;

    tango::IRowInserterPtr output_inserter = output_set->getRowInserter();
    if (output_inserter.isNull())
        return xcm::null;


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
            return xcm::null;
        }

        ijob->setStartTime(time(NULL));
        ijob->setStatus(tango::jobRunning);
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
    
    if (!group.empty())
    {
        index_filename = getTempFilename();

        idx = new ExIndex;
        idx->setTempFilePath(getTempPath());


        keylen = kl.getKeyLength();
        vallen = gi.m_store_size;

        if (!idx->create(index_filename, keylen, vallen, true))
        {
            delete idx;
            return xcm::null;
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
                    tango::rowid_t rowid = iter->getRowId();
                    memcpy(valuebuf, &rowid, sizeof(rowid));
                }

                for (sf_it = begin_sf_it; sf_it != end_sf_it; ++sf_it)
                {
                    switch (sf_it->m_type)
                    {
                        case tango::typeCharacter:
                        {
                            const std::string& s = iter->getString(sf_it->m_input_handle);
                            memset(valuebuf+sf_it->m_offset, 0, sf_it->m_width);
                            memcpy(valuebuf+sf_it->m_offset, s.c_str(), s.length());
                        }
                        break;

                        case tango::typeWideCharacter:
                        {
                            const std::wstring& s = iter->getWideString(sf_it->m_input_handle);
                            kl::wstring2ucsle(valuebuf+sf_it->m_offset, s, sf_it->m_width);
                        }
                        break;

                        case tango::typeDouble:
                        case tango::typeNumeric:
                        {
                            double d = iter->getDouble(sf_it->m_input_handle);
                            memcpy(valuebuf+sf_it->m_offset,
                                    &d, sizeof(double));
                        }
                        break;

                        case tango::typeInteger:
                        {
                            int i = iter->getInteger(sf_it->m_input_handle);
                            memcpy(valuebuf+sf_it->m_offset,
                                    &i, sizeof(int));
                        }
                        break;

                        case tango::typeBoolean:
                        {
                            bool b = iter->getBoolean(sf_it->m_input_handle);
                            memcpy(valuebuf+sf_it->m_offset,
                                    &b, sizeof(bool));
                        }
                        break;

                        case tango::typeDate:
                        {
                            tango::datetime_t dt = iter->getDateTime(sf_it->m_input_handle);
                            unsigned int d = ((unsigned int)(dt >> 32));
                            memcpy(valuebuf+sf_it->m_offset,
                                    &d, sizeof(int));
                        }
                        break;

                        case tango::typeDateTime:
                        {
                            tango::datetime_t dt = iter->getDateTime(sf_it->m_input_handle);
                            memcpy(valuebuf+sf_it->m_offset,
                                    &dt, sizeof(tango::datetime_t));
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
                    ijob->setStatus(tango::jobCancelled);

                if (failed)
                    ijob->setStatus(tango::jobFailed);
            }

            delete[] keybuf;
            delete[] valuebuf;

            return xcm::null;
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
                ijob->setStatus(tango::jobCancelled);
            }

            delete[] keybuf;
            delete[] valuebuf;

            return xcm::null;
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

    kl::Statistics stats;

    unsigned char* valptr = NULL;

    std::string val_string;
    std::wstring val_wstring;
    double val_double;
    bool val_bool;
    tango::datetime_t val_datetime;

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
        return output_set;
    }



    current_row = 0;
    output_inserter->startInsert(L"*");


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
                    case tango::typeCharacter:
                        buf2str(val_string, (char*)valptr+(*it)->m_offset, (*it)->m_param_width);
                        break;
                    case tango::typeWideCharacter:
                        kl::ucsle2wstring(val_wstring, valptr+(*it)->m_offset, (*it)->m_param_width);
                        break;
                    case tango::typeNumeric:
                    case tango::typeDouble:
                        val_double = *((double*)(valptr+(*it)->m_offset));
                        break;
                    case tango::typeInteger:
                        val_double = *((int*)(valptr+(*it)->m_offset));
                        break;
                    case tango::typeDate:
                        val_datetime = *((unsigned int*)(valptr+(*it)->m_offset));
                        val_datetime <<= 32;
                        break;
                    case tango::typeDateTime:
                        val_datetime = *((tango::datetime_t*)(valptr+(*it)->m_offset));
                        break;
                    case tango::typeBoolean:
                        val_bool = *((bool*)(valptr+(*it)->m_offset));
                        break;
                }
            }
             else
            {
                switch ((*it)->m_param_type)
                {
                    case tango::typeCharacter:
                        val_string = iter->getString((*it)->m_handle);
                        break;
                    case tango::typeWideCharacter:
                        val_wstring = iter->getWideString((*it)->m_handle);
                        break;
                    case tango::typeInteger:
                    case tango::typeNumeric:
                    case tango::typeDouble:
                        val_double = iter->getDouble((*it)->m_handle);
                        break;
                    case tango::typeDate:
                    case tango::typeDateTime:
                        val_datetime = iter->getDateTime((*it)->m_handle);
                        break;
                    case tango::typeBoolean:
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
                            case tango::typeCharacter:
                                (*it)->m_val_string = val_string;
                                break;
                            case tango::typeWideCharacter:
                                (*it)->m_val_wstring = val_wstring;
                                break;
                            case tango::typeInteger:
                            case tango::typeNumeric:
                            case tango::typeDouble:
                                (*it)->m_val_double = val_double;
                                break;
                            case tango::typeDate:
                            case tango::typeDateTime:
                                (*it)->m_val_datetime = val_datetime;
                                break;
                            case tango::typeBoolean:
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
                        case tango::typeCharacter:
                            (*it)->m_val_string += val_string;
                            break;

                        case tango::typeWideCharacter:
                            (*it)->m_val_wstring += val_wstring;
                            break;
                    }
                }
                break;
                
                case GroupFunc_MaxDistance:
                {
                    switch ((*it)->m_param_type)
                    {
                        case tango::typeCharacter:
                            (*it)->m_arr_string.push_back(kl::towstring(val_string));
                            break;

                        case tango::typeWideCharacter:
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
                        case tango::typeCharacter:
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

                        case tango::typeWideCharacter:
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

                        case tango::typeInteger:
                        case tango::typeNumeric:
                        case tango::typeDouble:
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

                        case tango::typeDate:
                        case tango::typeDateTime:
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

                        case tango::typeBoolean:
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
                            case tango::typeCharacter:
                                output_inserter->putString(out_it->m_output_handle, out_it->m_result->m_val_string);
                                break;
                            case tango::typeWideCharacter:
                                output_inserter->putWideString(out_it->m_output_handle, out_it->m_result->m_val_wstring);
                                break;
                            case tango::typeDouble:
                            case tango::typeNumeric:
                                output_inserter->putDouble(out_it->m_output_handle, out_it->m_result->m_val_double);
                                break;
                            case tango::typeInteger:
                                output_inserter->putInteger(out_it->m_output_handle, (int)out_it->m_result->m_val_double);
                                break;
                            case tango::typeDate:
                            case tango::typeDateTime:
                                output_inserter->putDateTime(out_it->m_output_handle, out_it->m_result->m_val_datetime);
                                break;
                            case tango::typeBoolean:
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
                                tango::rowid_t* rowid = (tango::rowid_t*)valptr;
                                iter->goRow(*rowid);
                            }
                        }

                        for (out_it = output_fields.begin(); out_it != output_fields.end(); ++out_it)
                        {
                            if (out_it->m_result)
                            {
                                switch (out_it->m_type)
                                {
                                    case tango::typeCharacter:
                                        output_inserter->putString(out_it->m_output_handle, out_it->m_result->m_val_string);
                                        break;
                                    case tango::typeWideCharacter:
                                        output_inserter->putWideString(out_it->m_output_handle, out_it->m_result->m_val_wstring);
                                        break;
                                    case tango::typeDouble:
                                    case tango::typeNumeric:
                                        output_inserter->putDouble(out_it->m_output_handle, out_it->m_result->m_val_double);
                                        break;
                                    case tango::typeInteger:
                                        output_inserter->putInteger(out_it->m_output_handle, (int)out_it->m_result->m_val_double);
                                        break;
                                    case tango::typeDate:
                                    case tango::typeDateTime:
                                        output_inserter->putDateTime(out_it->m_output_handle, out_it->m_result->m_val_datetime);
                                        break;
                                    case tango::typeBoolean:
                                        output_inserter->putBoolean(out_it->m_output_handle, out_it->m_result->m_val_bool);
                                        break;
                                }
                            }
                             else if (out_it->m_input_handle)
                            {
                                switch (out_it->m_type)
                                {
                                    case tango::typeCharacter:
                                        output_inserter->putString(out_it->m_output_handle, iter->getString(out_it->m_input_handle));
                                        break;
                                    case tango::typeWideCharacter:
                                        output_inserter->putWideString(out_it->m_output_handle, iter->getWideString(out_it->m_input_handle));
                                        break;
                                    case tango::typeDouble:
                                    case tango::typeNumeric:
                                        output_inserter->putDouble(out_it->m_output_handle, iter->getDouble(out_it->m_input_handle));
                                        break;
                                    case tango::typeInteger:
                                        output_inserter->putInteger(out_it->m_output_handle, iter->getInteger(out_it->m_input_handle));
                                        break;
                                    case tango::typeDate:
                                    case tango::typeDateTime:
                                        output_inserter->putDateTime(out_it->m_output_handle, iter->getDateTime(out_it->m_input_handle));
                                        break;
                                    case tango::typeBoolean:
                                        output_inserter->putBoolean(out_it->m_output_handle, iter->getBoolean(out_it->m_input_handle));
                                        break;
                                }
                            }
                             else if (out_it->m_store_offset != -1)
                            {
                                switch (out_it->m_type)
                                {
                                    case tango::typeCharacter:
                                        buf2str(val_string, (char*)valptr+out_it->m_store_offset, out_it->m_width);
                                        output_inserter->putString(out_it->m_output_handle, val_string);
                                        break;
                                    case tango::typeWideCharacter:
                                        kl::ucsle2wstring(val_wstring, valptr+out_it->m_store_offset, out_it->m_width);
                                        output_inserter->putWideString(out_it->m_output_handle, val_wstring);
                                        break;
                                    case tango::typeNumeric:
                                    case tango::typeDouble:
                                        val_double = *((double*)(valptr+out_it->m_store_offset));
                                        output_inserter->putDouble(out_it->m_output_handle, val_double);
                                        break;
                                    case tango::typeInteger:
                                        output_inserter->putInteger(out_it->m_output_handle, *((int*)(valptr+out_it->m_store_offset)));
                                        break;
                                    case tango::typeDate:
                                        val_datetime = *((unsigned int*)(valptr+out_it->m_store_offset));
                                        val_datetime <<= 32;
                                        output_inserter->putDateTime(out_it->m_output_handle, val_datetime);
                                        break;
                                    case tango::typeDateTime:
                                        val_datetime = *((tango::datetime_t*)(valptr+out_it->m_store_offset));
                                        output_inserter->putDateTime(out_it->m_output_handle, val_datetime);
                                        break;
                                    case tango::typeBoolean:
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

    if (idx)
    {
        idx_iter->unref();
        idx->unref();
        xf_remove(index_filename);
    }


    if (cancelled)
    {
        return xcm::null;
    }
     else
    {
        // set job to finished
        if (job)
        {
            ijob->setCurrentCount(current_row);
            ijob->setStatus(tango::jobFinished);
            ijob->setFinishTime(time(NULL));
        }
    }

    return output_set;
}




