/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-02-14
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <xd/xd.h>
#include "structure.h"
#include "util.h"
#include "../../kscript/kscript.h"
#include <kl/string.h>
#include <kl/portable.h>


static xd::ColumnInfo g_invalid_colinfo;


// -- modify structure helper functions --

static void modColumn(xd::ColumnInfo& target_col, const xd::ColumnInfo& params)
{
    if (params.mask & xd::ColumnInfo::maskName)
    {
        target_col.name = params.name;
    }

    if (params.mask & xd::ColumnInfo::maskType)
    {
        target_col.type = params.type;
    }
    
    if (params.mask & xd::ColumnInfo::maskWidth)
    {
        target_col.width = params.width;
    }

    if (params.mask & xd::ColumnInfo::maskScale)
    {
        target_col.scale = params.scale;
    }

    if (params.mask & xd::ColumnInfo::maskExpression)
    {
        target_col.expression = params.expression;
        target_col.calculated = true;
    }

    if (params.mask & xd::ColumnInfo::maskSourceOffset)
    {
        target_col.source_offset = params.source_offset;
    }
    
    if (params.mask & xd::ColumnInfo::maskSourceEncoding)
    {
        target_col.source_encoding = params.source_encoding;
    }
     
    if (params.mask & xd::ColumnInfo::maskColumnOrdinal)
    {
        // new column position
        target_col.column_ordinal = params.column_ordinal;
    }


    // if type changed, make sure width and scale conform
    if (params.mask & xd::ColumnInfo::maskType)
    {
        switch (target_col.type)
        {
            case xd::typeDate:
                target_col.width = 4;
                target_col.scale = 0;
                break;
            case xd::typeInteger:
                target_col.width = 4;
                target_col.scale = 0;
                break;
            case xd::typeDouble:
                target_col.width = 8;
                break;
            case xd::typeBoolean:
                target_col.width = 1;
                target_col.scale = 0;
                break;
            case xd::typeDateTime:
                target_col.width = 8;
                target_col.scale = 0;
                break;
        }
    }
}



bool calcfieldsModifyStructure(const xd::StructureModify& mod_params,
                               std::vector<xd::ColumnInfo>& calc_fields,
                               bool* done_flag)
{

    *done_flag = false;
    unsigned int processed_action_count = 0;

    std::vector<xd::StructureModify::Action>::const_iterator it;
    std::vector<xd::ColumnInfo>::iterator cit;

    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionDelete)
        {
            bool processed = false;

            for (cit = calc_fields.begin(); cit != calc_fields.end(); ++cit)
            {
                if (kl::iequals(cit->name, it->column))
                {
                    calc_fields.erase(cit);
                    processed_action_count++;
                    processed = true;
                    break;
                }
            }
        }
    }


    // handle modify
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionModify)
        {
            bool processed = false;

            for (cit = calc_fields.begin(); cit != calc_fields.end(); ++cit)
            {
                if (kl::iequals(cit->name, it->column))
                {
                    if ((it->params.mask & xd::ColumnInfo::maskCalculated) && !it->params.calculated)
                    {
                        // caller wants this field to be permanent,
                        // so we won't do anything here

                        continue;
                    }

                    modColumn(*cit, it->params);

                    processed_action_count++;
                    processed = true;
                    break;
                }
            }
        }
    }

    // handle create
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionCreate)
        {
            if (it->params.expression.length() > 0)
            {
                xd::ColumnInfo colinfo = it->params;
                colinfo.calculated = true;
                calc_fields.push_back(colinfo);

                processed_action_count++;
            }
        }
    }

    // handle insert
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionInsert)
        {
            if (it->params.expression.length() > 0)
            {
                xd::ColumnInfo colinfo = it->params;
                colinfo.calculated = true;
                calc_fields.push_back(colinfo);

                processed_action_count++;
            }
        }
    }

    if (processed_action_count == mod_params.actions.size())
    {
        // we have handled all actions, so we're done
        *done_flag = true;
    }

    return true;
}



// -- Structure class implementation --


Structure::Structure()
{
}

Structure::~Structure()
{
}


void Structure::fromStructure(const xd::Structure& s)
{
    m_cols = s.columns;
    m_map.clear();
}

xd::Structure Structure::toStructure()
{
    xd::Structure s;
    s.columns = m_cols;
    return s;
}


void Structure::createColumn(const xd::ColumnInfo& col)
{
    m_cols.push_back(col);

    if (!m_map.empty())
    {
        m_map.clear();
    }
}

xd::IStructurePtr Structure::clone()
{
    Structure* s = new Structure;
    s->m_cols = m_cols;

    return static_cast<xd::IStructure*>(s);
}

int Structure::getColumnIdx(const std::wstring& name)
{
    if (m_map.empty())
    {
        std::vector<xd::ColumnInfo>::iterator it;
        int i = 0;

        for (it = m_cols.begin(); it != m_cols.end(); ++it)
        {
            m_map[it->name] = i;
            i++;
        }
    }

    std::map<std::wstring, int, kl::cmp_nocase>::iterator it;
    it = m_map.find(name);
    if (it == m_map.end())
        return -1;
    return it->second;
}

int Structure::getColumnCount()
{
    return m_cols.size();
}

const std::wstring& Structure::getColumnName(int idx)
{
    return m_cols[idx].name;
}

const xd::ColumnInfo& Structure::getColumnInfoByIdx(int idx)
{
    if (idx < 0 || (size_t)idx >= m_cols.size())
        return g_invalid_colinfo;
    
    return m_cols[idx];
}

const xd::ColumnInfo& Structure::getColumnInfo(const std::wstring& column_name)
{
    int idx = getColumnIdx(column_name);
    if (idx == -1)
        return g_invalid_colinfo;

    return getColumnInfoByIdx(idx);
}

bool Structure::getColumnExist(const std::wstring& column_name)
{
    return (getColumnIdx(column_name) != -1) ? true : false;
}



static bool group_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    Structure* structure = (Structure*)hook_info.hook_param;
    
    if (hook_info.element_type == kscript::ExprParseHookInfo::typeIdentifier)
    {
        if (hook_info.expr_text.length() > 1 &&
            hook_info.expr_text[0] == '[' &&
            hook_info.expr_text[hook_info.expr_text.length()-1] == ']')
        {
            // remove brackets from identifier; i.e.:
            //     [field] => field
            //     table.[field] => table.field
            //     [table].field => table.field
            //     [table].[field] => table.field
            
            // get the two parts separated by the period, if there is one
            std::wstring part1, part2;

            const wchar_t* pstr = hook_info.expr_text.c_str();
            const wchar_t* pperiod = zl_strchr((wchar_t*)pstr, '.', L"[", L"]");
            int period_pos = pperiod ? (pperiod-pstr) : -1;

            if (period_pos == -1)
            {
                part1 = hook_info.expr_text;
                dequote(part1, '[', ']');
                hook_info.expr_text = part1;
            }
            else
            {
                part1 = hook_info.expr_text.substr(0, period_pos);
                part2 = hook_info.expr_text.substr(period_pos + 1);
                dequote(part1, '[', ']');
                dequote(part2, '[', ']');
                hook_info.expr_text = part1 + L"." + part2;
            }
        }


        const xd::ColumnInfo& col = structure->getColumnInfo(hook_info.expr_text);
        if (col.isNull())
            return false;
        
        kscript::Value* v = new kscript::Value;
        v->setGetVal(xd2kscriptType(col.type), NULL, NULL);
        hook_info.res_element = v;
        
        return true;
    }
     else if (hook_info.element_type == kscript::ExprParseHookInfo::typeFunction)
    {
        hook_info.res_element = NULL;
        
        std::wstring str = hook_info.expr_text;
        kl::trim(str);

        int len = str.length();
        if (len == 0)
            return true;

        if (str[len-1] != L')')
            return true;

        str.resize(len-1);
        len--;

        if (len == 0)
            return true;

        std::wstring func_name = kl::beforeFirst(str, L'(');
        std::wstring param = kl::afterFirst(str, L'(');

        kl::trim(func_name);
        kl::makeUpper(func_name);

        if (func_name == L"FIRST" ||
            func_name == L"LAST" ||
            func_name == L"MIN" ||
            func_name == L"MAX")
        {
            xd::ColumnInfo colinfo = structure->getColumnInfo(param);
            if (colinfo.isNull())
            {
                // try the parameter dequoted
                std::wstring dequoted_param = param;
                dequote(dequoted_param, '[', ']');
                colinfo = structure->getColumnInfo(dequoted_param);
            }

            if (colinfo.isNull())
            {
                kscript::Value* v = new kscript::Value;
                if (param.find(L'.') != -1)
                {
                    // let child fields be valid,
                    // even if we can't determine their type
                    // assume character
                    v->setString(L"");
                    hook_info.res_element = v;
                }
                
                return true;
            }

            kscript::Value* v = new kscript::Value;
            
            switch (colinfo.type)
            {
                case xd::typeCharacter:
                case xd::typeWideCharacter:
                    v->setString(L"");
                    break;
                case xd::typeNumeric:
                case xd::typeDouble:
                    v->setDouble(0.0);
                    break;
                case xd::typeInteger:
                    v->setInteger(0);
                    break;
                case xd::typeBoolean:
                    v->setBoolean(true);
                    break;
                case xd::typeDateTime:
                case xd::typeDate:
                    v->setDateTime(0,0);
                    break;
                case xd::typeBinary:
                    v->setType(kscript::Value::typeBinary);
                    break;
                default:
                    return true;
            }

            hook_info.res_element = v;

            return true;
        }
         else if (func_name == L"SUM" ||
                  func_name == L"AVG" ||
                  func_name == L"STDDEV" ||
                  func_name == L"VARIANCE")
        {
            xd::ColumnInfo colinfo = structure->getColumnInfo(param);

            if (colinfo.isNull())
            {
                // try the parameter dequoted
                std::wstring dequoted_param = param;
                dequote(dequoted_param, '[', ']');
                colinfo = structure->getColumnInfo(dequoted_param);
            }

            if (colinfo.isNull())
            {
                if (param.find(L'.') != -1)
                {
                    // let child fields be valid
                    kscript::Value* v = new kscript::Value;
                    v->setDouble(0.0);
                    hook_info.res_element = v;
                }
                
                return true;
            }

            if (colinfo.type != xd::typeNumeric &&
                colinfo.type != xd::typeInteger &&
                colinfo.type != xd::typeDouble)
            {
                return true;
            }

            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"MAXDISTANCE")
        {
            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"MERGE")
        {
            xd::ColumnInfo colinfo  = structure->getColumnInfo(param);

            if (colinfo.isNull())
            {
                // try the parameter dequoted
                std::wstring dequoted_param = param;
                dequote(dequoted_param, '[', ']');
                colinfo = structure->getColumnInfo(dequoted_param);
            }

            if (colinfo.isNull())
            {
                if (param.find(L'.') != -1)
                {
                    // let child fields be valid
                    kscript::Value* v = new kscript::Value;
                    v->setString(L"");
                    hook_info.res_element = v;
                }
                
                return true;
            }
            
            if (colinfo.type != xd::typeCharacter &&
                colinfo.type != xd::typeWideCharacter)
            {
                return true;
            }

            kscript::Value* v = new kscript::Value;
            v->setString(L"");
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"COUNT" ||
                  func_name == L"GROUPID")
        {
            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
    }

    // not handled
    return false;
}




int Structure::getExprType(const std::wstring& expression)
{
    // if the expression is a column name, simply look it up via getColumnInfo
    std::wstring dequoted_expression = expression;
    dequote(dequoted_expression, '[', ']');
    const xd::ColumnInfo& colinfo = getColumnInfo(dequoted_expression);
    if (colinfo.isOk())
        return colinfo.type;


    kscript::ExprParser* parser = createExprParser();
    
    // we added this here so that the expression
    // parser in structuredoc would recognize this as a valid function;
    // these are from xdnative/baseiterator.cpp
    //parser->setParseHook(parse_hook, this);
    parser->addFunction(L"recno", false, NULL, false, L"f()", this);
    parser->addFunction(L"rownum", false, NULL, false, L"f()", this);
    parser->addFunction(L"recid", false, NULL, false, L"s()", this);
    parser->addFunction(L"reccrc", false, NULL, false, L"x()", this);
    
    
    // create field bindings and add them to the expression parser

    parser->setParseHook(kscript::ExprParseHookInfo::typeFunction |
                         kscript::ExprParseHookInfo::typeIdentifier,
                         group_parse_hook,
                         (void*)this);

    if (!parser->parse(expression))
    {
        delete parser;
        return xd::typeInvalid;
    }

    int type = kscript2xdType(parser->getType());
    delete parser;

    return type;
}


bool Structure::isValid()
{
    return true;
}



