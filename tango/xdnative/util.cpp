/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-03-17
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <kl/file.h>
#include <kl/string.h>
#include <kl/portable.h>
#include <kl/math.h>
#include <set>
#include "tango.h"
#include "util.h"
#include "../xdcommon/util.h"
#include "../../kscript/kscript.h"


std::wstring combineOfsPath(const std::wstring& base_folder,
                            const std::wstring& file_name)
{
    std::wstring result;
    result = base_folder;

    if (result.empty() || result[result.length()-1] != L'/')
    {
        result += L'/';
    }


    const wchar_t* f = file_name.c_str();
    while (*f == L'/')
        f++;

    result += f;

    return result;
}


bool getOrderExpressionMatch(const std::wstring& expr1,
                             const std::wstring& expr2)
{
    std::vector<std::wstring> elements1;
    std::vector<std::wstring> elements2;

    kl::parseDelimitedList(expr1, elements1, L',');
    kl::parseDelimitedList(expr2, elements2, L',');

    int e1s = elements1.size();
    
    if (e1s != elements2.size())
        return false;

    int i;
    for (i = 0; i < e1s; ++i)
    {
        kl::trim(elements1[i]);
        kl::trim(elements2[i]);

        if (0 != wcscasecmp(elements1[i].c_str(), elements2[i].c_str()))
            return false;
    }

    return true;
}








struct FindFieldInfo
{
    std::set<std::wstring> found_fields;
    tango::IStructurePtr structure;
};

static bool findfield_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    FindFieldInfo* info = (FindFieldInfo*)hook_info.hook_param;

    hook_info.res_element = NULL;

    if (hook_info.element_type == kscript::ExprParseHookInfo::typeOperator)
    {
        // not handled
        return false;
    }

    std::wstring expr_text = hook_info.expr_text;
    kl::makeUpper(expr_text);

    if (hook_info.element_type == kscript::ExprParseHookInfo::typeFunction)
    {
        std::wstring func_name = expr_text;

        std::wstring param;
        param = kl::afterFirst(expr_text, L'(');
        param = kl::beforeLast(param, L')');
        kl::trim(param);

        // check for aggregate functions
        if (func_name == L"SUM" ||
            func_name == L"AVG" ||
            func_name == L"STDDEV" ||
            func_name == L"VARIANCE")
        {
            tango::IColumnInfoPtr colinfo = info->structure->getColumnInfo(param);
            if (colinfo.isNull())
                return true;
        
            int type = colinfo->getType();

            if (type != tango::typeNumeric &&
                type != tango::typeInteger &&
                type != tango::typeDouble)
            {
                // fail
                return true;
            }

            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"MERGE")
        {
            tango::IColumnInfoPtr colinfo = info->structure->getColumnInfo(param);
            if (colinfo.isNull())
                return true;
        
            int type = colinfo->getType();

            if (type != tango::typeCharacter &&
                type != tango::typeWideCharacter)
            {
                // fail
                return true;
            }

            kscript::Value* v = new kscript::Value;
            v->setString(L"");
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"COUNT")
        {
            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"GROUPID")
        {
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
         else if (func_name == L"FIRST" ||
                  func_name == L"LAST" ||
                  func_name == L"MIN" ||
                  func_name == L"MAX")
        {
            // continue below
            expr_text = param;
        }
         else
        {
            // not handled
            return false;
        }
    }


    tango::IColumnInfoPtr colinfo;
    colinfo = info->structure->getColumnInfo(expr_text);

    if (colinfo.isNull())
    {
        // not handled
        return false;
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

    info->found_fields.insert(expr_text);

    return true;
}




static void _findFieldsInExpr(const std::wstring& expr,
                              tango::IStructurePtr s,
                              bool recurse_calc_fields,
                              std::set<std::wstring>& fields)
{
    FindFieldInfo info;
    info.structure = s;

    std::vector<std::wstring> parts;
    std::vector<std::wstring>::iterator pit;

    kl::parseDelimitedList(expr, parts, L',', true);

    for (pit = parts.begin(); pit != parts.end(); ++pit)
    {
        kscript::ExprParser* parser = createExprParser();
        parser->setParseHook(kscript::ExprParseHookInfo::typeFunction |
                             kscript::ExprParseHookInfo::typeIdentifier,
                             findfield_parse_hook,
                             &info);
                             
        if (!parser->parse(*pit))
        {
            delete parser;
            return;
        }
        delete parser;
    }


    std::set<std::wstring>::iterator it;

    // remove fields that we already found
    // (this will prevent infinite recursion problems)

    for (it = fields.begin();
         it != fields.end();
         ++it)
    {
        info.found_fields.erase(*it);
    }


    // add the fields we found to the result set
    for (it = info.found_fields.begin();
         it != info.found_fields.end();
         ++it)
    {
        fields.insert(*it);
    }


    if (recurse_calc_fields)
    {
        for (it = info.found_fields.begin();
             it != info.found_fields.end();
             ++it)
        {
            tango::IColumnInfoPtr colinfo;
            colinfo = s->getColumnInfo(*it);
            if (colinfo.isNull())
                continue;
            if (!colinfo->getCalculated())
                continue;
            
            _findFieldsInExpr(colinfo->getExpression(),
                              s,
                              recurse_calc_fields,
                              fields);
        }

    }
}

std::vector<std::wstring> getFieldsInExpr(const std::wstring& expr,
                                          tango::IStructurePtr s,
                                          bool recurse_calcfields)
{
    std::set<std::wstring> flds;
    _findFieldsInExpr(expr, s, recurse_calcfields, flds);

    std::vector<std::wstring> result;

    std::set<std::wstring>::iterator it;
    for (it = flds.begin();
         it != flds.end();
         ++it)
    {
        result.push_back(*it);
    }

    return result;
}

bool findFieldInExpr(const std::wstring& _field,
                     const std::wstring& expr,
                     tango::IStructurePtr s,
                     bool recurse_calcfields)
{
    std::wstring field = _field;
    kl::makeUpper(field);


    std::set<std::wstring> flds;
    _findFieldsInExpr(expr, s, recurse_calcfields, flds);

    if (flds.find(field) != flds.end())
        return true;

    return false;
}



double decstr2dbl(const char* c, int width, int scale)
{
    double res = 0;
    double d = kl::pow10(width-scale-1);
    bool neg = false;
    while (width)
    {
        if (*c == '-')
            neg = true;

        if (*c >= '0' && *c <= '9')
        {
            res += d * (*c - '0');
        }

        d /= 10;
        c++;
        width--;
    }

    if (neg)
    {
        res *= -1.0;
    }

    return res;
}


void dbl2decstr(char* dest, double d, int width, int scale)
{
    double intpart;

    // check for negative
    if (d < 0.0)
    {
        *dest = '-';
        dest++;
        width--;
        d = fabs(d);
    }

    // rounding
    d += (0.5/kl::pow10(scale));

    // put everything to the right of the decimal
    d /= kl::pow10(width-scale);

    while (width)
    {
        d *= 10;
        d = modf(d, &intpart);
        if (intpart > 9.1)
            intpart = 0.0;

        *dest = int(intpart) + '0';
        dest++;
        width--;
    }
}



// conversion between set id's and table ord's

std::wstring getTableSetId(tango::tableord_t table_ordinal)
{
    wchar_t buf[64];
    swprintf(buf, 64, L"tbl%08u", table_ordinal);
    return buf;
}

tango::tableord_t getSetTableOrd(const std::wstring& set_id)
{
    // check to make sure that set id indeed refers to an ordinal
    const wchar_t* s = set_id.c_str();
    if (0 != wcsncasecmp(s, L"tbl", 3))
        return 0;

    return kl::wtoi(s+3);
}


