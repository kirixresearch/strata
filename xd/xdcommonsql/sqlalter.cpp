/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2007-04-09
 *
 */


#include <xd/xd.h>
#include "xdcommonsql.h"
#include "../xdcommon/util.h"
#include "../xdcommon/errorinfo.h"
#include <kl/string.h>
#include <kl/portable.h>
#include <map>


xd::ColumnInfo parseColumnDescription(const std::wstring& str, ThreadErrorInfo& error);



bool sqlAlter(xd::IDatabasePtr db,
              const std::wstring& _command,
              ThreadErrorInfo& error,
              xd::IJob* job)
{
    std::wstring command = _command.substr(6); // chop off "ALTER "

    SqlStatement stmt(command);

    stmt.addKeyword(L"TABLE");
    stmt.addKeyword(L"BEFORE");
    stmt.addKeyword(L"AFTER");
    stmt.addKeyword(L"FIRST");

    if (!stmt.getKeywordExists(L"TABLE"))
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; ALTER statement missing TABLE keyword");
        return false;
    }

    std::wstring params = stmt.getKeywordParam(L"TABLE");
    std::wstring table_name;
    
    wchar_t* params_cstr = (wchar_t*)params.c_str();
    wchar_t* blank = zl_strblank(params_cstr);
    if (!blank)
    {
        // missing parameter after table name
        error.setError(xd::errorSyntax, L"Invalid syntax; TABLE clause missing parameter");        
        return false;
    }
    
    table_name = params.substr(0, blank - params_cstr);
    params = params.substr(blank - params_cstr);

    
    dequote(table_name, '[', ']');
    
    xd::Structure structure = db->describeTable(table_name);

    if (structure.isNull())
    {
        // input set could not be opened
        wchar_t buf[1024]; // some paths might be long
        swprintf(buf, 1024, L"Unable to open table [%s]", table_name.c_str());
        error.setError(xd::errorGeneral, buf);
        return false;
    }

    

    xd::StructureModify mod_params;



    std::vector<std::wstring> commands;
    std::vector<std::wstring>::iterator it;
    kl::parseDelimitedList(params, commands, ',', true);

    for (it = commands.begin(); it != commands.end(); ++it)
    {
        std::wstring verb = popToken(*it);
        
        kl::trim(verb);
        kl::makeUpper(verb);
        
        if (verb == L"ADD")
        {
            std::wstring tok = peekToken(*it);
            if (0 == wcscasecmp(tok.c_str(), L"COLUMN"))
                popToken(*it);
            
            
            xd::ColumnInfo new_params = parseColumnDescription(*it, error);
            if (new_params.isNull())
            {
                // something wrong with the column description section;
                // error code set in parseColumnDescription function
                return false;
            }

            // if field name already exists, fail out
            if (structure.getColumnExist(new_params.name))
            {
                std::wstring msg = L"Unable to add column in ALTER statement; column '";
                msg += new_params.name;
                msg += L"' already exists";
                
                // can't add column
                error.setError(xd::errorGeneral, msg);
                return false;
            }
            

            xd::ColumnInfo colinfo;

            colinfo.name = new_params.name;
            colinfo.type = new_params.type;
            colinfo.width = new_params.width;
            colinfo.scale = new_params.scale;
            colinfo.expression = new_params.expression;
            colinfo.calculated = new_params.calculated;
            
            if (stmt.getKeywordExists(L"FIRST"))
                colinfo.column_ordinal = 0;
                 else
                colinfo.column_ordinal = structure.getColumnCount();


            if (stmt.getKeywordExists(L"BEFORE"))
            {
                std::wstring before = stmt.getKeywordParam(L"BEFORE");
                const xd::ColumnInfo& col = structure.getColumnInfo(before);
                if (col.isNull())
                {
                    std::wstring msg = L"Unable to add column in ALTER statement; column '";
                    msg += new_params.name;
                    msg += L"' specified in BEFORE clause does not exist";
                }
                
                colinfo.column_ordinal = col.column_ordinal;
            }
            
            if (stmt.getKeywordExists(L"AFTER"))
            {
                std::wstring after = stmt.getKeywordParam(L"AFTER");
                const xd::ColumnInfo& col = structure.getColumnInfo(after);
                if (col.isNull())
                {
                    std::wstring msg = L"Unable to add column in ALTER statement; column '";
                    msg += new_params.name;
                    msg += L"' specified in AFTER clause does not exist";
                }
                
                colinfo.column_ordinal = col.column_ordinal+1;
            }


            mod_params.createColumn(colinfo);
        }
         else if (verb == L"DROP")
        {
            std::wstring column = popToken(*it);
            
            if (0 == wcscasecmp(column.c_str(), L"COLUMN"))
                column = popToken(*it);
            
            dequote(column, '[', ']');
            
            if (!structure.getColumnExist(column))
            {
                // column didn't exist in the first place
                wchar_t buf[1024];
                swprintf(buf, 1024, L"Unable to drop column [%ls] in ALTER statement", column.c_str());
                error.setError(xd::errorGeneral, buf);               
                return false;
            }

            mod_params.deleteColumn(column);
        }
         else if (verb == L"RENAME")
        {
            std::wstring column = popToken(*it);
            
            if (kl::iequals(column, L"COLUMN"))
                column = popToken(*it);
            
            std::wstring to = popToken(*it);
            if (0 != wcscasecmp(to.c_str(), L"TO"))
            {
                // missing TO
                error.setError(xd::errorSyntax, L"Invalid syntax; RENAME clause missing TO keyword");
                return false;
            }
            
            std::wstring new_name = popToken(*it);
            if (new_name.empty())
            {
                // missing new name
                error.setError(xd::errorSyntax, L"Invalid syntax; RENAME clause missing new name parameter");
                return false;
            }
            
            dequote(column, '[', ']');
            dequote(new_name, '[', ']');

            if (!structure.getColumnExist(column))
            {
                // column doesn't exist
                wchar_t buf[1024];
                swprintf(buf, 1024, L"Unable to rename column [%ls] in ALTER statement", column.c_str());
                error.setError(xd::errorGeneral, buf);
                return false;
            }

            xd::ColumnInfo colinfo;
            colinfo.mask = xd::ColumnInfo::maskName;
            colinfo.name = new_name;

            mod_params.modifyColumn(column, colinfo);
        }
         else if (verb == L"ALTER")
        {
            std::wstring tok = peekToken(*it);
            
            if (0 == wcscasecmp(tok.c_str(), L"COLUMN"))
                popToken(*it);
            
            
            xd::ColumnInfo new_params = parseColumnDescription(*it, error);
            if (new_params.isNull())
            {
                // something wrong with the column description section;
                // error code set in parseColumnDescription function
                return false;
            }

            std::wstring colname = new_params.name;

            new_params.mask = xd::ColumnInfo::maskType | xd::ColumnInfo::maskWidth |
                              xd::ColumnInfo::maskScale | xd::ColumnInfo::maskExpression;

            if (!structure.getColumnExist(colname))
            {
                // column doesn't exist
                wchar_t buf[1024];
                swprintf(buf, 1024, L"Unable to alter column [%ls] in ALTER statement", colname.c_str());
                error.setError(xd::errorGeneral, buf);                 
                return false;
            }

            mod_params.modifyColumn(colname, new_params);
        }
         else if (verb == L"MODIFY")
        {
            std::wstring params = kl::afterFirst(*it, L'(');
            std::wstring columns = kl::beforeLast(params, L')');
            
            std::vector<std::wstring> colvec;
            std::vector<std::wstring>::iterator col_it;
            kl::parseDelimitedList(columns, colvec, L',', true);

            for (col_it = colvec.begin(); col_it != colvec.end(); ++col_it)
            {
                // get original column name
                std::wstring old_name = popToken(*col_it);
                if (old_name.empty())
                {
                    // missing original column name
                    error.setError(xd::errorSyntax, L"Invalid syntax; missing original column in MODIFY clause of ALTER statement");
                    return false;
                }
                dequote(old_name, '[', ']');
                
                std::wstring to = popToken(*col_it);
                if (0 != wcscasecmp(to.c_str(), L"TO"))
                {
                    // missing TO
                    error.setError(xd::errorSyntax, L"Invalid syntax; missing TO keyword in MODIFY clause of ALTER statement");
                    return false;
                }
                
                xd::ColumnInfo new_params = parseColumnDescription(*col_it, error);
                if (new_params.isNull())
                {
                    // something wrong with the column description section;
                    // error code set in parseColumnDescription function
                    return false;
                }
                
                new_params.mask = xd::ColumnInfo::maskType | xd::ColumnInfo::maskWidth |
                                  xd::ColumnInfo::maskScale | xd::ColumnInfo::maskExpression;

                if (structure.getColumnExist(old_name))
                {
                    // column doesn't exist
                    wchar_t buf[1024];
                    swprintf(buf, 1024, L"Unable to modify column [%ls] in ALTER statement", old_name.c_str());
                    error.setError(xd::errorGeneral, buf);                    
                    return false;
                }

                mod_params.modifyColumn(old_name, new_params);
            }
        }
    }    
    

    db->modifyStructure(table_name, mod_params, NULL);

    return true;
}


