/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-04-27
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "xdcommonsql.h"
#include "../xdcommon/util.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/errorinfo.h"
#include <kl/string.h>
#include <kl/portable.h>
#include <map>


tango::IColumnInfoPtr parseColumnDescription(const std::wstring& _col_desc, ThreadErrorInfo& error)
{
    std::wstring col_desc = _col_desc;
    
    // get field name
    std::wstring field = popToken(col_desc);
    kl::trim(field);
    if (field.empty())
    {
        // empty field name (probably a trailing comma);
        error.setError(tango::errorSyntax, L"Invalid syntax; empty field name in the column parameters");
        return xcm::null;
    }
    
    // if the field name is quoted with brackets,
    // dequote it to get the real field name
    dequote(field, '[', ']');
    
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
        type == L"CHARACTER")
    {
        colinfo->setType(tango::typeCharacter);
        if (width <= 0)
        {
            error.setError(tango::errorSyntax, L"Invalid syntax; character field has a width <= 0 in the column parameters");
            return xcm::null;
        }
        
        if (scale != 0)
        {
            error.setError(tango::errorSyntax, L"Invalid syntax; character field has a scale != 0 in the column parameters");        
            return xcm::null;
        }
    }
     else if (type == L"NVARCHAR" ||
              type == L"NCHAR")
    {
        colinfo->setType(tango::typeWideCharacter);
        if (width <= 0)
        {
            error.setError(tango::errorSyntax, L"Invalid syntax; character field has a width <= 0 in the column parameters");
            return xcm::null;
        }
        
        if (scale != 0)
        {
            error.setError(tango::errorSyntax, L"Invalid syntax; character field has a scale != 0 in the column parameters");        
            return xcm::null;
        }
    }
     else if (type == L"INTEGER" ||
              type == L"INT" ||
              type == L"SMALLINT")
    {
        colinfo->setType(tango::typeInteger);
        if (scale != 0)
        {
            error.setError(tango::errorSyntax, L"Invalid syntax; integer field has a scale != 0 in the column parameters");        
            return xcm::null;
        }
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
        wchar_t buf[1024]; // some paths might be long
        swprintf(buf, 1024, L"Invalid syntax; invalid column type [%ls] in the column parameters", type.c_str()); 
        error.setError(tango::errorSyntax, buf);        
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
            error.setError(tango::errorSyntax, L"Invalid syntax; missing AS in the column parameters");
            return xcm::null;
        }
        
        kl::trim(expr);
        
        if (expr.empty())
        {
            // empty expression
            error.setError(tango::errorSyntax, L"Invalid syntax; empty expression in the column parameters");            
            return xcm::null;
        }
        
        colinfo->setExpression(expr);
        colinfo->setCalculated(true);
    }
    
    return static_cast<tango::IColumnInfo*>(colinfo);
}




bool sqlCreate(tango::IDatabasePtr db,
               const std::wstring& _command,
               xcm::IObjectPtr& result_obj,
               ThreadErrorInfo& error,
               tango::IJob* job)
{
    result_obj.clear();
    
    SqlStatement stmt(_command);

    stmt.addKeyword(L"CREATE");
    stmt.addKeyword(L"TABLE");
    stmt.addKeyword(L"INDEX");
    stmt.addKeyword(L"DIRECTORY");
    stmt.addKeyword(L"MOUNT");
    stmt.addKeyword(L"AS");
    stmt.addKeyword(L"PATH");

    if (!stmt.getKeywordExists(L"CREATE"))
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; CREATE statement missing CREATE clause");    
        return false;
    }

    if (stmt.getKeywordExists(L"DIRECTORY"))
    {
        // create directory
        std::wstring param = stmt.getKeywordParam(L"DIRECTORY");
        dequote(param, '[', ']');

        if (!db->createFolder(param))
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to create directory [%ls]", param.c_str());
            error.setError(tango::errorGeneral, buf);
            return false;
        }
        
        return true;
    }
     else if (stmt.getKeywordExists(L"MOUNT"))
    {
        std::wstring mount_path = stmt.getKeywordParam(L"MOUNT");
        std::wstring connection_string = stmt.getKeywordParam(L"AS");
        std::wstring path = stmt.getKeywordParam(L"PATH");
        if (connection_string.empty() && path.empty())
        {
            error.setError(tango::errorSyntax, L"Invalid syntax; MOUNT clause missing connection string and path");
            return false;
        }
        
        dequote(mount_path, '[', ']');

        if (!db->setMountPoint(mount_path, connection_string, path))
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to create mount [%ls]", mount_path.c_str());
            error.setError(tango::errorGeneral, buf);
            return false;
        }
        
        return true;
    }
     else if (stmt.getKeywordExists(L"INDEX"))
    {
        std::wstring command = _command;
        std::wstring token = popToken(command);
        token = popToken(command);
        if (0 != wcscasecmp(token.c_str(), L"INDEX"))
        {
            // missing INDEX keyword
            error.setError(tango::errorSyntax, L"Invalid syntax; INDEX clause missing INDEX keyword");
            return false;
        }
        
        std::wstring name = popToken(command);
        dequote(name, L'[', L']');
        
        std::wstring on = popToken(command);
        if (0 != wcscasecmp(on.c_str(), L"ON"))
        {
            // missing ON keyword
            error.setError(tango::errorSyntax, L"Invalid syntax; INDEX clause missing ON keyword");            
            return false;
        }
        
        std::wstring table = popToken(command);
        dequote(table, L'[', L']');
        
        std::wstring paren = popToken(command);
        if (paren != L"(")
        {
            // missing open parenthesis
            // CREATE INDEX idx ON table(col1)
            //                          ^^
            error.setError(tango::errorSyntax, L"Invalid syntax; INDEX clause missing open parenthesis");             
            return false;
        }
        
        std::wstring col_list = kl::beforeLast(command, L')');
        
        

        tango::IIndexInfoPtr index;
        
        index = db->createIndex(table, name, col_list, job);
        
        if (index.isNull())
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to create index [%ls] on table [%ls]", name.c_str(), table.c_str()); 
            error.setError(tango::errorGeneral, buf);         
            return false;
        }
        
        return true;
    }
     else if (stmt.getKeywordExists(L"TABLE"))
    {
        std::wstring params = stmt.getKeywordParam(L"TABLE");
        
        const wchar_t* start = params.c_str();
        wchar_t* left_paren = zl_strchr((wchar_t*)start, '(', L"[", L"]");
        
        if (!left_paren)
        {
            // missing left parenthesis
            error.setError(tango::errorSyntax, L"Invalid syntax; TABLE clause missing open parenthesis");
            return false;
        }
        
        std::wstring table_name = params.substr(0, left_paren-start);
            
        kl::trim(table_name);
        dequote(table_name, '[', ']');

        params = kl::afterFirst(params, L'(');
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
            tango::IColumnInfoPtr col = parseColumnDescription(*it, error);
            if (col.isNull())
                return false;
                
            s->addColumn(col);
        }

        if (db->getFileExist(table_name))
        {
            // object or table already exists
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to create table [%ls] because table already exists", table_name.c_str()); 
            error.setError(tango::errorGeneral, buf);  
            return false;
        }
        
        bool res = db->createTable(table_name, sp, NULL);
        if (!res)
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to create table [%ls]", table_name.c_str()); 
            error.setError(tango::errorGeneral, buf);
            return false;
        }
    }
     else
    {
        error.setError(tango::errorGeneral, L"Unable to process CREATE statement");
        return false;
    }
    
    
    return true;
}

