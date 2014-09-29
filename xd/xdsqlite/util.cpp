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
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
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
    
    
    if (type == L"DOUBLE")
    {
        // "DOUBLE PRECISION" -- pop off the second word
        std::wstring next_token = peekToken(col_desc);
        kl::makeUpper(next_token);
        if (next_token == L"PRECISION")
            popToken(col_desc);
    }
          
    
    std::wstring expr = col_desc;
    
    // get type parameters (width, scale) etc
    int width = 0;
    int scale = 0;
    if (!col_desc.empty() && col_desc[0] == L'(')
    {
        std::wstring type_params = kl::beforeFirst(col_desc.substr(1), ')');
        
        std::wstring w = kl::beforeFirst(type_params, L',');
        std::wstring s = L"0";
        if (type_params.find(L',') != -1)
            s = kl::afterFirst(type_params, L',');
        width = kl::wtoi(w);
        scale = kl::wtoi(s);    
        
        expr = kl::afterFirst(col_desc, L')');
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
              type == L"NUMBER")
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
     else if (type == L"BOOLEAN")
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
        std::wstring as = popToken(expr);
        if (0 != wcscasecmp(as.c_str(), L"AS"))
        {
            // missing AS
            return xd::ColumnInfo();
        }
        
        kl::trim(expr);
        
        if (expr.empty())
        {
            // empty expression
            return xd::ColumnInfo();
        }
        
        colinfo.expression = expr;
        colinfo.type = true;
    }
    
    return colinfo;
}



xd::IStructurePtr parseCreateStatement(const std::wstring& create)
{
    std::wstring params = kl::afterFirst(create, L'(');
    std::wstring columns = kl::beforeLast(params, L')');
    
    std::vector<std::wstring> colvec;
    kl::parseDelimitedList(columns, colvec, L',', true);

    Structure* s = new Structure;
    xd::IStructurePtr sp = static_cast<xd::IStructure*>(s);

    std::vector<std::wstring>::iterator it;
    for (it = colvec.begin();
         it != colvec.end();
         ++it)
    {
        xd::ColumnInfo col = parseSqliteColumnDescription(*it);
        if (col.isNull())
            return xcm::null;
            
        s->addColumn(col);
    }

    return static_cast<xd::IStructure*>(s);
}
