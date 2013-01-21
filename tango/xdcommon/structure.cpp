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


#include "tango.h"
#include "structure.h"
#include "columninfo.h"
#include "util.h"
#include "../../kscript/kscript.h"
#include <kl/string.h>
#include <kl/portable.h>



// -- modify structure helper functions --

static void modColumn(tango::IColumnInfoPtr target_col,
                      const std::wstring& name,
                      int type,
                      int width,
                      int scale,
                      const std::wstring& expr,
                      int offset,
                      int encoding,
                      int ordinal)
{
    if (name.length() > 0)
    {
        target_col->setName(name);
    }

    if (type != -1)
    {
        target_col->setType(type);
    }
    
    if (width != -1)
    {
        target_col->setWidth(width);
    }

    if (scale != -1)
    {
        target_col->setScale(scale);
    }

    if (expr.length() > 0)
    {
        target_col->setExpression(expr);
        target_col->setCalculated(true);
    }

    if (offset != -1)
    {
        target_col->setOffset(offset);
    }
    
    if (encoding != -1)
    {
        target_col->setEncoding(encoding);
    }
     
    if (ordinal != -1)
    {
        // new column position
        target_col->setColumnOrdinal(ordinal);
    }


    // -- if type changed, make sure width and scale conform --
    if (type != -1)
    {
        switch (type)
        {
            case tango::typeDate:
                target_col->setWidth(4);
                target_col->setScale(0);
                break;
            case tango::typeInteger:
                target_col->setWidth(4);
                target_col->setScale(0);
                break;
            case tango::typeDouble:
                target_col->setWidth(8);
                break;
            case tango::typeBoolean:
                target_col->setWidth(1);
                target_col->setScale(0);
                break;
            case tango::typeDateTime:
                target_col->setWidth(8);
                target_col->setScale(0);
                break;
        }
    }
}



bool calcfieldsModifyStructure(std::vector<StructureAction>& actions,
                               tango::IStructurePtr _mod_struct,
                               std::vector<tango::IColumnInfoPtr>* calc_fields,
                               bool* done_flag)
{
    IStructureInternalPtr mod_struct = _mod_struct;


    *done_flag = false;
    unsigned int processed_action_count = 0;

    std::vector<StructureAction>::iterator it;
    std::vector<tango::IColumnInfoPtr>::iterator cit;

    // -- handle delete --
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action == StructureAction::actionDelete)
        {
            bool processed = false;

            if (calc_fields)
            {
                for (cit = calc_fields->begin();
                     cit != calc_fields->end();
                     ++cit)
                {
                    if (!wcscasecmp((*cit)->getName().c_str(),
                                    it->m_colname.c_str()))
                    {
                        calc_fields->erase(cit);
                        processed_action_count++;
                        processed = true;
                        break;
                    }
                }
            }

            if (mod_struct.isOk() && !processed)
            {
                if (mod_struct->removeColumn(it->m_colname))
                {
                    processed_action_count++;
                }
            }
        }
    }

    // -- handle modify --
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action == StructureAction::actionModify)
        {
            bool processed = false;

            if (calc_fields)
            {
                for (cit = calc_fields->begin();
                     cit != calc_fields->end();
                     ++cit)
                {
                    if (!wcscasecmp((*cit)->getName().c_str(), it->m_colname.c_str()))
                    {
                        if (!it->m_params->getCalculated())
                        {
                            // caller wants this field to be permanent,
                            // so we won't do anything here

                            continue;
                        }

                        modColumn(*cit,
                                  it->m_params->getName(),
                                  it->m_params->getType(),
                                  it->m_params->getWidth(),
                                  it->m_params->getScale(),
                                  it->m_params->getExpression(),
                                  it->m_params->getOffset(),
                                  it->m_params->getEncoding(),
                                  -1 /* don't allow calc fields to be moved */);

                        processed_action_count++;
                        processed = true;
                        break;
                    }
                }
            }

            if (mod_struct.isOk() && !processed)
            {
                if (mod_struct->modifyColumn(it->m_colname,
                                             it->m_params->getName(),
                                             it->m_params->getType(),
                                             it->m_params->getWidth(),
                                             it->m_params->getScale(),
                                             it->m_params->getExpression(),
                                             it->m_params->getOffset(),
                                             it->m_params->getEncoding(),
                                             it->m_params->getColumnOrdinal()))
                {
                    processed_action_count++;
                }
            }
        }
    }

    // -- handle create --
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action == StructureAction::actionCreate)
        {
            if (it->m_params->getExpression().length() > 0)
            {
                if (calc_fields)
                {
                    tango::IColumnInfoPtr col = it->m_params->clone();
                    col->setCalculated(true);
                    calc_fields->push_back(col);
                }
                 else
                {
                    mod_struct->addColumn(it->m_params->clone());
                }

                processed_action_count++;
            }
        }
    }

    // -- handle insert --
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action == StructureAction::actionInsert)
        {
            if (it->m_params->getExpression().length() > 0)
            {
                if (calc_fields)
                {
                    tango::IColumnInfoPtr col = it->m_params->clone();
                    col->setCalculated(true);
                    calc_fields->push_back(col);
                }
                 else
                {
                    int insert_idx = it->m_pos;
                    mod_struct->internalInsertColumn(it->m_params->clone(),
                                                     insert_idx);
                }

                processed_action_count++;
            }
        }
    }

    if (processed_action_count == actions.size())
    {
        // -- we have handled all actions, so we're done --
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

std::vector<StructureAction>& Structure::getStructureActions()
{
    return m_actions;
}

void Structure::addColumn(tango::IColumnInfoPtr col)
{
    m_cols.push_back(col);

    if (!m_map.empty())
    {
        m_map.clear();
    }
}

bool Structure::internalInsertColumn(tango::IColumnInfoPtr col, int insert_idx)
{
    // we're just adding a column to the end of the structure
    if (insert_idx == -1)
    {
        addColumn(col);
        return true;
    }
    
    // the insert index is out-of-bounds
    if (insert_idx < 0 || insert_idx >= (int)m_cols.size())
        return false;

    m_cols.insert(m_cols.begin()+insert_idx, col);

    if (!m_map.empty())
    {
        m_map.clear();
    }
    
    return true;
}

bool Structure::internalMoveColumn(const std::wstring& column_name, int new_idx)
{
    std::vector<tango::IColumnInfoPtr>::iterator it;

    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        if (!wcscasecmp((*it)->getName().c_str(), column_name.c_str()))
        {
            // -- save the element so we can reinsert it into our vector --
            tango::IColumnInfoPtr insert_col = *it;
            
            // -- remove the element from our vector and clear the map
            //    to force it to renumber the elements --
            m_cols.erase(it);
            m_cols.insert(m_cols.begin()+new_idx, insert_col);
            m_map.clear();
            return true;
        }
    }

    return false;
}

bool Structure::modifyColumn(const std::wstring& column_name,
                             const std::wstring& name,
                             int type,
                             int width,
                             int scale,
                             const std::wstring& expr,
                             int offset,
                             int encoding,
                             int ordinal)
{
    std::vector<tango::IColumnInfoPtr>::iterator it;
    for (it = m_cols.begin(); it != m_cols.end(); ++it)
    {
        if (!wcscasecmp((*it)->getName().c_str(), column_name.c_str()))
        {
            modColumn(*it, name, type, width, scale, expr, offset, encoding, ordinal);
            m_map.clear();
            return true;
        }
    }

    return false;
}

bool Structure::removeColumn(const std::wstring& column_name)
{
    bool found = false;
    
    int i = 0, col_count = m_cols.size();
    for (i = 0; i < col_count; ++i)
    {
        if (!wcscasecmp(m_cols[i]->getName().c_str(), column_name.c_str()))
        {
            m_cols.erase(m_cols.begin() + i);
            m_map.erase(column_name);
            found = true;
            i--;
            col_count--;
        }
    }

    return found;
}


Structure* Structure::internalClone()
{
    Structure* s = new Structure;
    std::vector<tango::IColumnInfoPtr>::iterator it, it_end;

    s->m_cols.reserve(m_cols.size());

    it_end = m_cols.end();
    for (it = m_cols.begin(); it != it_end; ++it)
        s->m_cols.push_back((*it)->clone());

    s->m_actions = m_actions;

    return s;
}

tango::IStructurePtr Structure::clone()
{
    Structure* s = internalClone();
    return static_cast<tango::IStructure*>(s);
}

int Structure::getColumnIdx(const std::wstring& name)
{
    if (m_map.empty())
    {
        std::vector<tango::IColumnInfoPtr>::iterator it;
        int i = 0;

        for (it = m_cols.begin(); it != m_cols.end(); ++it)
        {
            m_map[(*it)->getName()] = i;
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

std::wstring Structure::getColumnName(int idx)
{
    return m_cols[idx]->getName();
}

tango::IColumnInfoPtr Structure::getColumnInfoByIdx(int idx)
{
    if (idx < 0 || (size_t)idx >= m_cols.size())
        return xcm::null;
        
    return m_cols[idx];
}

tango::IColumnInfoPtr Structure::getColumnInfo(const std::wstring& column_name)
{
    int idx = getColumnIdx(column_name);
    if (idx == -1)
        return xcm::null;

    return getColumnInfoByIdx(idx);
}

bool Structure::getColumnExist(const std::wstring& column_name)
{
    return (getColumnIdx(column_name) != -1) ? true : false;
}


bool Structure::deleteColumn(const std::wstring& column_name)
{
    StructureAction action;
    action.m_action = StructureAction::actionDelete;
    action.m_params = xcm::null;
    action.m_colname = column_name;
    action.m_pos = -1;
    m_actions.push_back(action);
    return true;
}

bool Structure::moveColumn(const std::wstring& column_name, int new_idx)
{
    StructureAction action;
    action.m_action = StructureAction::actionMove;
    action.m_params = xcm::null;
    action.m_colname = column_name;
    action.m_pos = new_idx;
    m_actions.push_back(action);
    return true;
}

tango::IColumnInfoPtr Structure::modifyColumn(const std::wstring& column_name)
{
    int idx = getColumnIdx(column_name);
    if (idx == -1)
        return xcm::null;

    ColumnInfo* action_params = new ColumnInfo;
    action_params->setName(L"");
    action_params->setType(-1);
    action_params->setWidth(-1);
    action_params->setScale(-1);
    action_params->setExpression(L"");
    action_params->setOffset(-1);
    action_params->setColumnOrdinal(-1);
    action_params->setEncoding(-1);
    action_params->setCalculated(m_cols[idx]->getCalculated());

    StructureAction action;
    action.m_action = StructureAction::actionModify;
    action.m_params = action_params;
    action.m_colname = column_name;
    action.m_pos = -1;
    m_actions.push_back(action);

    return action_params;
}

tango::IColumnInfoPtr Structure::createColumn()
{
    ColumnInfo* action_params = new ColumnInfo;
    action_params->setName(L"");
    action_params->setType(tango::typeInvalid);
    action_params->setWidth(0);
    action_params->setScale(0);

    StructureAction action;
    action.m_action = StructureAction::actionCreate;
    action.m_params = action_params;
    action.m_colname = L"";
    action.m_pos = -1;
    m_actions.push_back(action);

    m_cols.push_back(action_params);

    return action_params;
}

tango::IColumnInfoPtr Structure::insertColumn(int idx)
{
    // both of these options are the same as calling createColumn()
    if (idx == -1 || idx == (int)m_cols.size())
        return createColumn();

    if (idx < 0 || idx > (int)m_cols.size())
        return xcm::null;

    ColumnInfo* action_params = new ColumnInfo;
    action_params->setName(L"");
    action_params->setType(tango::typeInvalid);
    action_params->setWidth(0);
    action_params->setScale(0);

    StructureAction action;
    action.m_action = StructureAction::actionInsert;
    action.m_params = action_params;
    action.m_colname = L"";
    action.m_pos = idx;
    m_actions.push_back(action);

    m_cols.insert(m_cols.begin()+idx, action_params);

    return action_params;
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


        tango::IColumnInfoPtr col = structure->getColumnInfo(hook_info.expr_text);
        if (col.isNull())
            return false;
        
        kscript::Value* v = new kscript::Value;
        v->setGetVal(tango2kscriptType(col->getType()),
                     NULL,
                     NULL);
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
            tango::IColumnInfoPtr colinfo;
            
            colinfo = structure->getColumnInfo(param);
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
            
            switch (colinfo->getType())
            {
                case tango::typeCharacter:
                case tango::typeWideCharacter:
                    v->setString(L"");
                    break;
                case tango::typeNumeric:
                case tango::typeDouble:
                    v->setDouble(0.0);
                    break;
                case tango::typeInteger:
                    v->setInteger(0);
                    break;
                case tango::typeBoolean:
                    v->setBoolean(true);
                    break;
                case tango::typeDateTime:
                case tango::typeDate:
                    v->setDateTime(0,0);
                    break;
                case tango::typeBinary:
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
            tango::IColumnInfoPtr colinfo;
            
            colinfo = structure->getColumnInfo(param);

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
            
            int type = colinfo->getType();

            if (type != tango::typeNumeric &&
                type != tango::typeInteger &&
                type != tango::typeDouble)
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
            tango::IColumnInfoPtr colinfo;
            
            colinfo = structure->getColumnInfo(param);

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
            
            int type = colinfo->getType();

            if (type != tango::typeCharacter &&
                type != tango::typeWideCharacter)
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

    // -- not handled --
    return false;
}




int Structure::getExprType(const std::wstring& expression)
{
    // if the expression is a column name, simply look it up via getColumnInfo
    std::wstring dequoted_expression = expression;
    dequote(dequoted_expression, '[', ']');
    tango::IColumnInfoPtr colinfo = getColumnInfo(dequoted_expression);
    if (colinfo.isOk())
        return colinfo->getType();


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
        return tango::typeInvalid;
    }

    int type = kscript2tangoType(parser->getType());
    delete parser;

    return type;
}


bool Structure::isValid()
{
    return true;
}



