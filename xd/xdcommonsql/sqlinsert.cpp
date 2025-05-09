/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-04-26
 *
 */


#include <kl/portable.h>
#include <kl/string.h>
#include <xd/xd.h>
#include "xdcommonsql.h"
#include "../xdcommon/util.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/errorinfo.h"

struct InsertFieldInfo
{
public:

    int type;
    xd::objhandle_t handle;
};



bool sqlInsert(xd::IDatabasePtr db,
               const std::wstring& _command,
               ThreadErrorInfo& error,
               xd::IJob* job)
{
    SqlStatement stmt(_command);

    stmt.addKeyword(L"INSERT");
    stmt.addKeyword(L"INTO");
    stmt.addKeyword(L"VALUES");
    stmt.addKeyword(L"SELECT");

    if (!stmt.getKeywordExists(L"INSERT"))
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; INSERT statement missing INSERT clause");
        return false;
    }

    if (!stmt.getKeywordExists(L"INTO"))
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; INSERT statement missing INTO clause");
        return false;
    }

    bool value_insert = stmt.getKeywordExists(L"VALUES");
    bool select_insert = stmt.getKeywordExists(L"SELECT");
    
    if ((value_insert && select_insert) ||
        (!value_insert && !select_insert))
    {
        // make sure it's either a INSERT INTO tbl (...) VALUES (...)" statement
        // or a INSERT INTO tbl SELECT statement
        error.setError(xd::errorSyntax, L"Invalid syntax; INSERT statement missing VALUES or SELECT clause");        
        return false;
    }

    std::wstring table;
    std::wstring fields;
    std::wstring values;

    // fetch the table name
    std::wstring temps = stmt.getKeywordParam(L"INTO");
    table = kl::beforeFirst(temps, L'(');
    kl::trim(table);
    dequote(table, '[', ']');
    
    // fetch the field names
    if (table != temps)
    {
        temps = kl::afterFirst(temps, L'(');
        fields = kl::beforeLast(temps, L')');
    }
    kl::trim(fields);

    if (select_insert)
    {
        size_t pos = stmt.getKeywordPosition(L"SELECT");
        if (pos == (size_t)-1)
        {
            error.setError(xd::errorSyntax, L"Invalid syntax; INSERT statement missing SELECT clause");          
            return false;
        }
            
        std::wstring select_stmt = _command.substr(pos);
        
        xd::IFileInfoPtr finfo = db->getFileInfo(table);
        if (finfo.isNull() || finfo->getType() != xd::filetypeTable)
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to insert rows because table [%ls] cannot be accessed", table.c_str());
            buf[1023] = 0;
            error.setError(xd::errorGeneral, buf);
            return false;
        }
        
        xcm::IObjectPtr result;
        db->execute(select_stmt, 0, result, NULL);
        xd::IIteratorPtr src_iter = result;
        
        if (src_iter.isNull())
        {
            error.setError(xd::errorGeneral, L"Unable to insert rows because select clause could not be processed");        
            return false;
        }

        src_iter->goFirst();

        std::vector<std::pair<std::wstring,std::wstring> > default_columns;
        xdcmnInsert(db, src_iter, table, default_columns, L"",  0,  NULL);

        return true;
    }

    

    temps = stmt.getKeywordParam(L"VALUES");
    kl::trim(temps);

    if (temps.empty() || temps[0] != L'(' || temps[temps.length()-1] != L')')
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; VALUES clause missing parameters");          
        return false;
    }

    temps = kl::afterFirst(temps, L'(');
    values = kl::beforeLast(temps, L')');
    kl::trim(values);


    // parse lists
    
    std::vector<InsertFieldInfo> insertvec;
    std::vector<std::wstring> fieldvec;
    std::vector<std::wstring> valuesvec;

    kl::parseDelimitedList(fields, fieldvec, L',', true);
    kl::parseDelimitedList(values, valuesvec, L',', true);

    if (fieldvec.size() != valuesvec.size())
    {
        // field list/value list size mismatch
        error.setError(xd::errorSyntax, L"Invalid syntax; number of values to insert differs from number of fields to insert into");
        return false;
    }

    // try to open the target set

    xd::Structure structure = db->describeTable(table);

    if (structure.isNull())
    {
        wchar_t buf[1024]; // some paths might be long
        swprintf(buf, 1024, L"Unable to insert values because table [%ls] could not be accessed", table.c_str());
        error.setError(xd::errorGeneral, buf);
        return false;
    }



    // do the insert

    xd::IRowInserterPtr inserter = db->bulkInsert(table);


    // get field info
    insertvec.reserve(fieldvec.size());

    std::vector<std::wstring>::iterator it;
    for (it = fieldvec.begin(); it != fieldvec.end(); ++it)
    {
        dequote(*it, '[', ']');
        
        const xd::ColumnInfo& colinfo = structure.getColumnInfo(*it);
        if (colinfo.isNull())
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Invalid syntax: Column [%ls] does not exist", (*it).c_str());
            error.setError(xd::errorSyntax, buf);
            return false;
        }

        InsertFieldInfo fi;
        fi.type = colinfo.type;
        fi.handle = inserter->getHandle(*it);

        if (fi.handle == 0)
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Invalid syntax: Unable to get handle for column [%ls]", (*it).c_str());
            buf[1023] = 0;
            error.setError(xd::errorSyntax, buf);        
            return false;
        }

        insertvec.push_back(fi);
    }


    if (!inserter->startInsert(fields))
    {
        error.setError(xd::errorGeneral, L"Unable to process INSERT statement");    
        return false;
    }

    int idx = 0;

    std::vector<InsertFieldInfo>::iterator insert_it, insert_end;
    insert_end = insertvec.end();
    
    for (insert_it = insertvec.begin();
         insert_it != insert_end;
         ++insert_it, ++idx)
    {
        std::wstring& s = valuesvec[idx];
        kl::trim(s);
        int len = (int)s.length();

        if (insert_it->type == xd::typeCharacter ||
            insert_it->type == xd::typeWideCharacter)
        {
            if (s.length() < 2)
            {
                // since character fields require two quotes, length should be
                // at least two            
                inserter->finishInsert();
                error.setError(xd::errorSyntax, L"Invalid syntax: missing character value; make sure list of values to insert contains properly quoted values");
                return false;
            }

            wchar_t quote_char = s[0];
            if (quote_char != '"' && quote_char != '\'')
            {
                // missing quotation mark -- either " or ' required
                inserter->finishInsert();
                error.setError(xd::errorSyntax, L"Invalid syntax: missing quote around character value; make sure list of values to insert contains properly quoted values");
                return false;
            }

            if (s[len-1] != quote_char)
            {
                // close quote is not the same as the open quote
                inserter->finishInsert();
                error.setError(xd::errorSyntax, L"Invalid syntax: closing and opening quote differ for some of the values to insert");                
                return false;
            }            

            std::wstring result;
            result.reserve(len);

            for (int i = 1; i < len-1; ++i)
            {
                result += s[i];
                if (s[i] == quote_char)
                {
                    if (s[i+1] != quote_char)
                    {
                        inserter->finishInsert();
                        error.setError(xd::errorSyntax, L"Invalid syntax: missing quote around character value; make sure list of values to insert contains properly quoted values");
                        return false;
                    }

                    ++i;
                }
            }

            inserter->putWideString(insert_it->handle, result);
        }
         else if (insert_it->type == xd::typeInteger ||
                  insert_it->type == xd::typeNumeric ||
                  insert_it->type == xd::typeDouble)
        {
            double d = kl::wtof(s);
            inserter->putDouble(insert_it->handle, d);
        }
         else if (insert_it->type == xd::typeDateTime ||
                  insert_it->type == xd::typeDate)
        {
            int y,m,d,hh,mm,ss;

            if (s.length() < 2)
            {
                // since date/datetime fields require two quotes, length should be
                // at least two            
                inserter->finishInsert();
                error.setError(xd::errorSyntax, L"Invalid syntax: missing date value; make sure list of values to insert contains properly quoted values");
                return false;
            }

            wchar_t quote_char = s[0];
            if (quote_char != '"' && quote_char != '\'')
            {
                // missing quotation mark -- either " or ' required
                inserter->finishInsert();
                error.setError(xd::errorSyntax, L"Invalid syntax: missing quote around date value; make sure list of values to insert contains properly quoted values");
                return false;
            }
            
            if (s[len-1] != quote_char)
            {
                // close quote is not the same as the open quote
                inserter->finishInsert();
                error.setError(xd::errorSyntax, L"Invalid syntax: closing and opening quote differ for some of the values to insert");
                return false;
            }

            std::wstring temps = kl::afterFirst(s, quote_char);
            temps = kl::beforeLast(temps, quote_char);

            if (!parseDateTime(temps, &y, &m, &d, &hh, &mm, &ss))
            {
                inserter->finishInsert();
                error.setError(xd::errorSyntax, L"Invalid syntax: date value not properly formatted; date values should be inserted in the form YYYY-MM-DD; datetime values should be inserted in the form YYYY-MM-DD HH:MM:SS");
                return false;
            }

            if (hh == -1)
            {
                hh = 0;
                mm = 0;
                ss = 0;
            }

            xd::DateTime dt(y, m, d, hh, mm, ss);
            inserter->putDateTime(insert_it->handle, dt.getDateTime());
        }
         else if (insert_it->type == xd::typeBoolean)
        {
            bool b = false;

            if (wcscasecmp(s.c_str(), L"true") == 0)
            {
                b = true;
            }
             else if (wcscasecmp(s.c_str(), L"false") == 0)
            {
                b = false;
            }
             else
            {
                inserter->finishInsert();
                error.setError(xd::errorSyntax, L"Invalid syntax: boolean value not properly formatted; boolean values should be inserted using values of 'true' or 'false'");
                return false;
            }

            inserter->putBoolean(insert_it->handle, b);
        }
    }

    inserter->insertRow();
    inserter->finishInsert();

    return true;
}


