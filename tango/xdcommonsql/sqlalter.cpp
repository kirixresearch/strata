/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2007-04-09
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "xdcommonsql.h"
#include "../xdcommon/util.h"
#include <kl/string.h>
#include <kl/portable.h>
#include <map>


tango::IColumnInfoPtr parseColumnDescription(const std::wstring& str, ThreadErrorInfo& error);



bool sqlAlter(tango::IDatabasePtr db,
              const std::wstring& _command,
              ThreadErrorInfo& error,
              tango::IJob* job)
{
    SqlStatement stmt(_command);

    stmt.addKeyword(L"ALTER");
    stmt.addKeyword(L"TABLE");
    stmt.addKeyword(L"BEFORE");
    stmt.addKeyword(L"AFTER");
    stmt.addKeyword(L"FIRST");

    if (!stmt.getKeywordExists(L"TABLE"))
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; ALTER statement missing TABLE keyword");
        return false;
    }

    std::wstring params = stmt.getKeywordParam(L"TABLE");
    std::wstring table_name;
    
    wchar_t* params_cstr = (wchar_t*)params.c_str();
    wchar_t* blank = zl_strblank(params_cstr);
    if (!blank)
    {
        // missing parameter after table name
        error.setError(tango::errorSyntax, L"Invalid syntax; TABLE clause missing parameter");        
        return false;
    }
    
    table_name = params.substr(0, blank - params_cstr);
    params = params.substr(blank - params_cstr);

    
    dequote(table_name, '[', ']');
    
    tango::IStructurePtr structure = db->describeTable(table_name);

    if (structure.isNull())
    {
        // input set could not be opened
        wchar_t buf[1024]; // some paths might be long
        swprintf(buf, 1024, L"Unable to open table [%s]", table_name.c_str());
        error.setError(tango::errorGeneral, buf);
        return false;
    }

    
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
            
            
            tango::IColumnInfoPtr new_params = parseColumnDescription(*it, error);
            if (new_params.isNull())
            {
                // something wrong with the column description section;
                // error code set in parseColumnDescription function
                return false;
            }

            // if field name already exists, fail out
            if (structure->getColumnExist(new_params->getName()))
            {
                std::wstring msg = L"Unable to add column in ALTER statement; column '";
                msg += new_params->getName();
                msg += L"' already exists";
                
                // can't add column
                error.setError(tango::errorGeneral, msg);
                return false;
            }
            

            tango::IColumnInfoPtr colinfo = structure->createColumn();
            if (colinfo.isNull())
            {
                // can't add column
                error.setError(tango::errorGeneral, L"Unable to add column in ALTER statement");
                return false;
            }
        
            colinfo->setName(new_params->getName());
            colinfo->setType(new_params->getType());
            colinfo->setWidth(new_params->getWidth());
            colinfo->setScale(new_params->getScale());
            colinfo->setExpression(new_params->getExpression());
            colinfo->setCalculated(new_params->getCalculated());
            
            if (stmt.getKeywordExists(L"FIRST"))
                colinfo->setColumnOrdinal(0);
                 else
                colinfo->setColumnOrdinal(structure->getColumnCount());


            if (stmt.getKeywordExists(L"BEFORE"))
            {
                std::wstring before = stmt.getKeywordParam(L"BEFORE");
                tango::IColumnInfoPtr col = structure->getColumnInfo(before);
                if (col.isNull())
                {
                    std::wstring msg = L"Unable to add column in ALTER statement; column '";
                    msg += new_params->getName();
                    msg += L"' specified in BEFORE clause does not exist";
                }
                
                colinfo->setColumnOrdinal(col->getColumnOrdinal());
            }
            
            if (stmt.getKeywordExists(L"AFTER"))
            {
                std::wstring after = stmt.getKeywordParam(L"AFTER");
                tango::IColumnInfoPtr col = structure->getColumnInfo(after);
                if (col.isNull())
                {
                    std::wstring msg = L"Unable to add column in ALTER statement; column '";
                    msg += new_params->getName();
                    msg += L"' specified in AFTER clause does not exist";
                }
                
                colinfo->setColumnOrdinal(col->getColumnOrdinal()+1);
            }
        }
         else if (verb == L"DROP")
        {
            std::wstring column = popToken(*it);
            
            if (0 == wcscasecmp(column.c_str(), L"COLUMN"))
                column = popToken(*it);
            
            dequote(column, '[', ']');
            
            if (!structure->deleteColumn(column))
            {
                // column didn't exist in the first place
                wchar_t buf[1024];
                swprintf(buf, 1024, L"Unable to drop column [%ls] in ALTER statement", column.c_str());
                error.setError(tango::errorGeneral, buf);               
                return false;
            }
        }
         else if (verb == L"RENAME")
        {
            std::wstring column = popToken(*it);
            
            if (0 == wcscasecmp(column.c_str(), L"COLUMN"))
                column = popToken(*it);
            
            std::wstring to = popToken(*it);
            if (0 != wcscasecmp(to.c_str(), L"TO"))
            {
                // missing TO
                error.setError(tango::errorSyntax, L"Invalid syntax; RENAME clause missing TO keyword");
                return false;
            }
            
            std::wstring new_name = popToken(*it);
            if (new_name.empty())
            {
                // missing new name
                error.setError(tango::errorSyntax, L"Invalid syntax; RENAME clause missing new name parameter");
                return false;
            }
            
            dequote(column, '[', ']');
            dequote(new_name, '[', ']');

            tango::IColumnInfoPtr colinfo = structure->modifyColumn(column);
            if (colinfo.isNull())
            {
                // column doesn't exist
                wchar_t buf[1024];
                swprintf(buf, 1024, L"Unable to rename column [%ls] in ALTER statement", column.c_str());
                error.setError(tango::errorGeneral, buf);
                return false;
            }
            
            colinfo->setName(new_name);
        }
         else if (verb == L"ALTER")
        {
            std::wstring tok = peekToken(*it);
            
            if (0 == wcscasecmp(tok.c_str(), L"COLUMN"))
                popToken(*it);
            
            
            tango::IColumnInfoPtr new_params = parseColumnDescription(*it, error);
            if (new_params.isNull())
            {
                // something wrong with the column description section;
                // error code set in parseColumnDescription function
                return false;
            }

            std::wstring colname = new_params->getName();
            tango::IColumnInfoPtr colinfo = structure->modifyColumn(colname);
            if (colinfo.isNull())
            {
                // column doesn't exist
                wchar_t buf[1024];
                swprintf(buf, 1024, L"Unable to alter column [%ls] in ALTER statement", colname.c_str());
                error.setError(tango::errorGeneral, buf);                 
                return false;
            }
        
            colinfo->setType(new_params->getType());
            colinfo->setWidth(new_params->getWidth());
            colinfo->setScale(new_params->getScale());
            colinfo->setExpression(new_params->getExpression());
            colinfo->setCalculated(new_params->getCalculated());
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
                    error.setError(tango::errorSyntax, L"Invalid syntax; missing original column in MODIFY clause of ALTER statement");
                    return false;
                }
                
                
                std::wstring to = popToken(*col_it);
                if (0 != wcscasecmp(to.c_str(), L"TO"))
                {
                    // missing TO
                    error.setError(tango::errorSyntax, L"Invalid syntax; missing TO keyword in MODIFY clause of ALTER statement");
                    return false;
                }
                
                tango::IColumnInfoPtr new_params = parseColumnDescription(*col_it, error);
                if (new_params.isNull())
                {
                    // something wrong with the column description section;
                    // error code set in parseColumnDescription function
                    return false;
                }
                
                tango::IColumnInfoPtr colinfo = structure->modifyColumn(old_name);
                if (colinfo.isNull())
                {
                    // column doesn't exist
                    wchar_t buf[1024];
                    swprintf(buf, 1024, L"Unable to modify column [%ls] in ALTER statement", old_name.c_str());
                    error.setError(tango::errorGeneral, buf);                    
                    return false;
                }
            
                colinfo->setName(new_params->getName());
                colinfo->setType(new_params->getType());
                colinfo->setWidth(new_params->getWidth());
                colinfo->setScale(new_params->getScale());
                colinfo->setExpression(new_params->getExpression());
                colinfo->setCalculated(new_params->getCalculated());
            }
        }
    }    
    

    db->modifyStructure(table_name, structure, NULL);

    return true;
}


