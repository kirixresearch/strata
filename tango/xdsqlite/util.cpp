/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#include "tango.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/util.h"
#include "util.h"


std::wstring popToken(std::wstring& str);
std::wstring peekToken(const std::wstring& str);


// this code is from xdcommon/sqlcreate.cpp
tango::IColumnInfoPtr parseSqliteColumnDescription(const std::wstring& _col_desc)
{
    std::wstring col_desc = _col_desc;
    
    // get field name
    std::wstring field = popToken(col_desc);
    kl::trim(field);
    if (field.empty())
    {
        // empty field name (probably a trailing comma);
        return xcm::null;
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
    
    


    // -- make column info --
    ColumnInfo* colinfo = new ColumnInfo;

    colinfo->setName(field);

    if (type == L"VARCHAR" ||
        type == L"CHAR" ||
        type == L"CHARACTER" ||
        type == L"TEXT")
    {
        colinfo->setType(tango::typeCharacter);
        if (scale != 0)
            return xcm::null;
        if (width <= 0)
            width = 255;
    }
     else if (type == L"INTEGER" ||
              type == L"INT" ||
              type == L"SMALLINT")
    {
        colinfo->setType(tango::typeInteger);
        if (scale != 0)
            return xcm::null;
    }
     else if (type == L"DEC" ||
              type == L"DECIMAL" ||
              type == L"NUMERIC" ||
              type == L"NUMBER")
    {
        colinfo->setType(tango::typeNumeric);
    }
     else if (type == L"DOUBLE" ||
              type == L"FLOAT" ||
              type == L"REAL")
    {  
        colinfo->setType(tango::typeDouble);
    }
     else if (type == L"DATE")
    {
        colinfo->setType(tango::typeDate);
    }
     else if (type == L"DATETIME")
    {
        colinfo->setType(tango::typeDateTime);
    }
     else if (type == L"BOOLEAN")
    {
        colinfo->setType(tango::typeBoolean);
    }
     else
    {
        // -- bad type --
        return xcm::null;
    }

    colinfo->setWidth(width);
    colinfo->setScale(scale);
    
    
    if (!expr.empty())
    {
        std::wstring as = popToken(expr);
        if (0 != wcscasecmp(as.c_str(), L"AS"))
        {
            // missing AS
            return xcm::null;
        }
        
        kl::trim(expr);
        
        if (expr.empty())
        {
            // empty expression
            return xcm::null;
        }
        
        colinfo->setExpression(expr);
        colinfo->setCalculated(true);
    }
    
    return static_cast<tango::IColumnInfo*>(colinfo);
}



tango::IStructurePtr parseCreateStatement(const std::wstring& create)
{
    std::wstring params = kl::afterFirst(create, L'(');
    std::wstring columns = kl::beforeLast(params, L')');
    
    std::vector<std::wstring> colvec;
    kl::parseDelimitedList(columns, colvec, L',', true);


    Structure* s = new Structure;
    tango::IStructurePtr sp = static_cast<tango::IStructure*>(s);

    std::vector<std::wstring>::iterator it;
    for (it = colvec.begin();
         it != colvec.end();
         ++it)
    {
        tango::IColumnInfoPtr col = parseSqliteColumnDescription(*it);
        if (col.isNull())
            return xcm::null;
            
        s->addColumn(col);
    }

    return static_cast<tango::IStructure*>(s);
}
