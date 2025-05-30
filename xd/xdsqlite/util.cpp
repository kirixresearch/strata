/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#include <xd/xd.h>
#include <kl/string.h>
#include <kl/portable.h>
#include "../xdcommon/util.h"
#include "util.h"


// TODO: peekToken and popToken should factor
// out common code

static std::wstring peekToken(const std::wstring& str)
{
    const wchar_t* start = str.c_str();
    const wchar_t* p = start;
    std::wstring ret;
    int chars = 0;

    while (iswspace(*p))
    {
        p++;
        chars++;
    }

    if (*p == '[')
    {
        // identifier quotation
        const wchar_t* close = wcschr(p, ']');
        if (close)
        {
            ret.assign(p, close-p+1);
            return ret;
        }
    }
    
    while (*p)
    {
        if (0 != wcschr(L" \t\n\r!@#$%^&*-=|/+,()[]{}:'\"", *p))
        {
            if (ret.empty())
            {
                ret = *p;
                return ret;
            }
             else
            {
                break;
            }
        }

        ret += *p;
        ++p;
        ++chars;
    }

    return ret;
}


// WARNING: until peekToken and popToken
// are factored, if you make a change to popToken,
// make sure to make the change to the function above

static std::wstring popToken(std::wstring& str)
{
    const wchar_t* start = str.c_str();
    const wchar_t* p = start;
    std::wstring ret;
    int chars = 0;

    while (iswspace(*p))
    {
        p++;
        chars++;
    }

    if (*p == '[')
    {
        // identifier quotation
        const wchar_t* close = wcschr(p, ']');
        if (close)
        {
            ret.assign(p, close-p+1);
            str.erase(0, close-start+1);
            return ret;
        }
    }
    
    while (*p)
    {
        if (0 != wcschr(L" \t\n\r!@#$%^&*-=|/+,()[]{}:'\"", *p))
        {
            if (ret.empty())
            {
                ret = *p;
                str.erase(0,chars+1);
                return ret;
            }
             else
            {
                break;
            }
        }

        ret += *p;
        ++p;
        ++chars;
    }

    str.erase(0, chars);
    return ret;
}


// this code is from xdcommon/sqlcreate.cpp
xd::ColumnInfo parseSqliteColumnDescription(const std::wstring& _col_desc)
{
    std::wstring col_desc = _col_desc;
    
    // get field name
    std::wstring field = popToken(col_desc);
    kl::trim(field);
    if (field.empty())
    {
        // empty field name (probably a trailing comma);
        return xd::ColumnInfo();
    }
    
    // get field type
    std::wstring type = popToken(col_desc);
    kl::makeUpper(type);
    kl::trim(type);
    kl::trim(col_desc);

    std::wstring expr;
    int width = 0;
    int scale = 0;
    bool generated = false;

    if (type == L"GENERATED")
    {
        type = L"TEXT";
        std::wstring next_token = peekToken(col_desc);
        kl::makeUpper(next_token);
        if (next_token == L"ALWAYS")
            popToken(col_desc);

        expr = col_desc;
        generated = true;
    }
    else
    {
        if (type == L"DOUBLE")
        {
            // "DOUBLE PRECISION" -- pop off the second word
            std::wstring next_token = peekToken(col_desc);
            kl::makeUpper(next_token);
            if (next_token == L"PRECISION")
                popToken(col_desc);
        }


        expr = col_desc;

        // get type parameters (width, scale) etc

        if (!col_desc.empty() && col_desc[0] == L'(')
        {
            std::wstring type_params = kl::beforeFirst(col_desc.substr(1), ')');

            std::wstring w = kl::beforeFirst(type_params, L',');
            std::wstring s = L"0";
            if (type_params.find(L',') != type_params.npos)
                s = kl::afterFirst(type_params, L',');
            width = kl::wtoi(w);
            scale = kl::wtoi(s);

            expr = kl::afterFirst(col_desc, L')');
        }


    }



    // make column info
    xd::ColumnInfo colinfo;

    colinfo.name = field;

    if (type == L"VARCHAR" ||
        type == L"CHAR" ||
        type == L"CHARACTER" ||
        type == L"TEXT")
    {
        colinfo.type = xd::typeCharacter;
        if (scale != 0)
            return xd::ColumnInfo();
        if (width <= 0)
            width = 255;
    }
     else if (type == L"INTEGER" ||
              type == L"INT" ||
              type == L"SMALLINT")
    {
        colinfo.type = xd::typeInteger;
        if (scale != 0)
            return xd::ColumnInfo();
    }
     else if (type == L"DEC" ||
              type == L"DECIMAL" ||
              type == L"NUMERIC" ||
              type == L"NUMBER" ||
              type == L"NUM")
    {
        colinfo.type = xd::typeNumeric;
    }
     else if (type == L"DOUBLE" ||
              type == L"FLOAT" ||
              type == L"REAL")
    {  
        colinfo.type = xd::typeDouble;
    }
     else if (type == L"DATE")
    {
        colinfo.type = xd::typeDate;
    }
     else if (type == L"DATETIME")
    {
        colinfo.type = xd::typeDateTime;
    }
     else if (type == L"BOOLEAN" || type == L"BOOL")
    {
        colinfo.type = xd::typeBoolean;
    }
     else
    {
        // bad type
        return xd::ColumnInfo();
    }

    colinfo.width = width;
    colinfo.scale = scale;
    
    
    if (!expr.empty())
    {
        std::wstring next = popToken(expr);

        if (0 == wcscasecmp(next.c_str(), L"GENERATED"))
        {
            if (0 == wcscasecmp(peekToken(expr).c_str(), L"ALWAYS"))
                popToken(expr);

            next = popToken(expr);
            generated = true;
        }

        if (0 != wcscasecmp(next.c_str(), L"AS"))
        {
            // missing AS
            return xd::ColumnInfo();
        }
        
        kl::trim(expr);

        // trim off trailing VIRTUAL
        if (expr.length() > 7 && (0 == wcscasecmp(expr.substr(expr.length() - 7).c_str(), L"VIRTUAL") ||
                                  0 == wcscasecmp(expr.substr(expr.length() - 6).c_str(), L"STORED")))
        {
            expr = expr.substr(0, expr.length()-7); // " STORED" will take the preceding space with it
            kl::trim(expr);
        }

        // sqlite syntax surrounds that expression in parentheses
        if (expr.length() >= 2 && expr[0] == '(' && expr[expr.length() - 1] == ')')
        {
            expr = expr.substr(1, expr.length()-2);
        }

        if (expr.empty())
        {
            // empty expression
            return xd::ColumnInfo();
        }
        
        colinfo.expression = expr;
    }

    if (generated)
        colinfo.calculated = true;
    
    return colinfo;
}



xd::Structure parseCreateStatement(const std::wstring& create)
{
    std::wstring params = kl::afterFirst(create, L'(');
    std::wstring columns = kl::beforeLast(params, L')');
    
    std::vector<std::wstring> colvec;
    kl::parseDelimitedList(columns, colvec, L',', true);

    xd::Structure s;

    std::vector<std::wstring>::iterator it;
    for (it = colvec.begin(); it != colvec.end(); ++it)
    {
        xd::ColumnInfo col = parseSqliteColumnDescription(*it);
        if (col.isNull())
            return xd::Structure();
            
        s.createColumn(col);
    }

    return s;
}
