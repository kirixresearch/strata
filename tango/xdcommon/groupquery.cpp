/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2012-04-18
 *
 */

#include <kl/portable.h>
#include <kl/math.h>
#include <kl/string.h>
#include <xd/xd.h>
#include "groupquery.h"
#include "jobinfo.h"
#include "util.h"
#include "../../kscript/kscript.h"
#include <ctime>


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
            delete (*it);

        std::vector<kscript::ExprParser*>::iterator dit;
        for (dit = m_to_destroy.begin(); dit != m_to_destroy.end(); ++dit)
            delete (*dit);
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
        int param_type = m_structure->getExprType(param);

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
                               const std::wstring& field,
                               int* out_offset)
    {
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

            xd::IColumnInfoPtr colinfo = m_structure->getColumnInfo(field);
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
    xd::IStructurePtr m_structure;
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

        // identifiers by themselves are considered FIRST()
        text = L"FIRST(";
        text += hook_info.expr_text;
        text += L")";
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
        xd::IColumnInfoPtr colinfo;
        colinfo = info->m_structure->getColumnInfo(result->m_param_text);

        if (colinfo.isNull())
        {
            result->m_type = info->m_structure->getExprType(result->m_param_text);
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


bool runGroupQuery(xd::IDatabasePtr db,
                   xd::IIteratorPtr sp_iter,
                   const std::wstring& output_path,
                   const std::wstring& group,
                   const std::wstring& output,
                   const std::wstring& condition,
                   const std::wstring& having,
                   xd::IJob* job)
{
    if (db.isNull() || sp_iter.isNull())
        return false;

    GroupQueryParams gi;
    std::vector<GroupOutputInfo> output_fields;
    kscript::ExprParser* having_parser = NULL;
    bool copy_detail = false;
    bool store_detail = false;
    xd::IIterator* iter = sp_iter.p;
    xd::IStructurePtr structure;
    xd::rowpos_t row_count = 0;
    xd::rowpos_t current_row = 0;

    // try to get the row count
    {
        if (sp_iter->getIteratorFlags() & xd::ifFastRowCount)
        {
            row_count = sp_iter->getRowCount();
        }
    }

    gi.m_structure = iter->getStructure();


    // parse output fields

    std::vector<std::wstring> tmp_outputfields;
    kl::parseDelimitedList(output, tmp_outputfields, L',', true);
    int tmp_i;

    for (tmp_i = 0; tmp_i < (int)tmp_outputfields.size(); ++tmp_i)
    {
        kl::trim(tmp_outputfields[tmp_i]);

        if (0 == wcscasecmp(tmp_outputfields[tmp_i].c_str(),  L"[DETAIL]"))
        {
            if (copy_detail)
                return false; // '[DETAIL]' was specified more than once. return error
            copy_detail = true;

            // determine if the detail records must be stored in the index's
            // value side.  This is the case when the iterator is forward only,
            // or can't support goRow()
            if (iter->getIteratorFlags() & xd::ifForwardOnly)
                store_detail = true;

            int colcount = structure->getColumnCount();
            for (int i = 0; i < colcount; ++i)
            {
                xd::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);

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

            continue;
        }
         else if (tmp_outputfields[tmp_i] == L"*")
        {
            int i, colcount = structure->getColumnCount();
            for (i = colcount-1; i >= 0; --i)
            {
                std::wstring expr = structure->getColumnName(i);
                expr += L"=FIRST(";
                expr += structure->getColumnName(i);
                expr += L")";

                tmp_outputfields.insert(tmp_outputfields.begin() + tmp_i + 1, expr);
            }

            continue;
        }
         else
        {
            std::wstring str = tmp_outputfields[tmp_i];
            std::wstring part1;
            std::wstring part2;

            // try to find an equals sign
            int eq_pos = (int)str.find(L'=');
            
            // if it's just a field name, make a FIRST(<field_name>) out of it
            if (eq_pos == -1)
            {
                if (!structure->getColumnExist(str))
                    return false; // column doesn't exist

                part1 = str;
                part2 = L"FIRST(";
                part2 += str;
                part2 += L")";
            }
             else
            {
                // parse the <output_field>=FUNC(<input_field>) expression
                part1 = str.substr(0, eq_pos);
                part2 = str.substr(eq_pos+1);

                kl::trim(part1);
                kl::trim(part2);

                if (structure->getColumnExist(part2))
                {
                    std::wstring temps = L"FIRST(";
                    temps += part2;
                    temps += L")";
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



    IJobInternalPtr ijob;
    if (job)
    {
        ijob = job;
        if (!ijob)
            return xcm::null;

        ijob->setStartTime(time(NULL));
        ijob->setStatus(xd::jobRunning);
        ijob->setMaxCount(row_count);
        ijob->setCurrentCount(0);
    }



    iter->goFirst();
    current_row = 0;

    while (!iter->eof())
    {

        iter->skip(1);
        current_row++;


        if ((current_row % 1) == 0 && job)
        {
            ijob->setCurrentCount(current_row);
            if (job->getCancelled())
            {
                break;
            }
        }
    }

    return true;
}





}; // namespace xdcommon

