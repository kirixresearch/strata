/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-04-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <kl/string.h>
#include <kl/portable.h>
#include <map>
#include <ctime>
#include "tango.h"
#include "sqlcommon.h"
#include "util.h"
#include "jobinfo.h"



struct BaseSetReplaceInfo
{
    std::wstring col_text;
    int col_type;
    tango::objhandle_t col_handle;

    std::wstring replace_text;
    int replace_type;
    tango::objhandle_t replace_handle;
};


static int doUpdate(tango::ISetPtr set,
                    const std::wstring& filter,
                    const std::wstring& _params,
                    ThreadErrorInfo& error,
                    tango::IJob* job)
{
    tango::ColumnUpdateInfo* col_update;
    std::vector<BaseSetReplaceInfo> replace;
    std::vector<BaseSetReplaceInfo>::iterator rit;

    // -- parse the replace params --
    std::vector<std::wstring> params;
    std::vector<std::wstring>::iterator p_it;
    kl::parseDelimitedList(_params, params, L',', true);

    if (params.size() == 0)
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; no replace parameters were specified");
        return -1;
    }

    BaseSetReplaceInfo ri;
    
    for (p_it = params.begin(); p_it != params.end(); ++p_it)
    {
        kl::trim(*p_it);

        int eq_pos = p_it->find(L'=');
        if (eq_pos < 1)
        {
            // -- parse error --
            error.setError(tango::errorSyntax, L"Invalid syntax; missing '=' in replace parameters");
            return -1;
        }

        eq_pos++;

        ri.col_type = 0;
        ri.replace_type = 0;
        ri.col_text = p_it->substr(0, eq_pos-1);
        kl::trim(ri.col_text);
        dequote(ri.col_text, '[', ']');
        ri.col_handle = 0;
        ri.replace_text = p_it->substr(eq_pos);
        kl::trim(ri.replace_text);
        ri.replace_handle = 0;

        replace.push_back(ri);
    }

    // -- get set update interface --
    tango::ISetRowUpdatePtr sp_set_update = set;
    tango::ISetRowUpdate* set_update = sp_set_update.p;
    if (sp_set_update.isNull())
    {
        // could not get a row update interface
        error.setError(tango::errorGeneral, L"Unable to process the UPDATE statement");
        return -1;
    }


    // -- create physical iterator and initialize expression handles --
    tango::IIteratorPtr sp_iter = set->createIterator(L"", L"", NULL);
    tango::IIterator* iter = sp_iter.p;
    if (sp_iter.isNull())
    {
        // could not create an unordered iterator
        error.setError(tango::errorGeneral, L"Unable to process the UPDATE statement");        
        return -1;
    }
    
    
    // -- create an SqlIterator object --
    
    SqlIterator* s_iter = SqlIterator::createSqlIterator(sp_iter, filter, job);
    if (!s_iter)
    {
        error.setError(tango::errorGeneral, L"Unable to process the UPDATE statement");    
        return -1;
    }

    // -- get structure --
    tango::IStructurePtr structure = set->getStructure();




    // -- perpare the tango::ColumnUpdateInfo array; this involves
    //    getting all writer handles --
    
    
    col_update = new tango::ColumnUpdateInfo[replace.size()];
    size_t i;
    
    tango::IColumnInfoPtr colinfo;

    bool success = true;
    for (rit = replace.begin(), i = 0; rit != replace.end(); ++rit, ++i)
    {
        colinfo = structure->getColumnInfo(rit->col_text);
        if (!colinfo)
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Invalid syntax: Column [%ls] does not exist", (rit->col_text).c_str());
            error.setError(tango::errorSyntax, buf);
            
            success = false;
            break;
        }

        rit->col_type = colinfo->getType();
        rit->col_handle = iter->getHandle(rit->col_text);
        if (rit->col_handle == NULL)
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Invalid syntax: Unable to get handle for column [%ls]", (rit->col_text).c_str());
            error.setError(tango::errorSyntax, buf);

            success = false;
            break;
        }

        rit->replace_handle = iter->getHandle(rit->replace_text);
        if (rit->replace_handle == NULL)
        {
            error.setError(tango::errorSyntax, L"Invalid syntax: invalid replace expression");        

            success = false;
            break;
        }

        rit->replace_type = iter->getType(rit->replace_handle);

        if (rit->col_type != rit->replace_type)
        {
            if (!tango::isTypeCompatible(rit->col_type, rit->replace_type))
            {
                error.setError(tango::errorSyntax, L"Invalid syntax: replace expression type incompatible with column type being replaced");              
            
                success = false;
                break;
            }
        }


        col_update[i].handle = rit->col_handle;
    }

    if (!success)
    {
        for (rit = replace.begin(); rit != replace.end(); ++rit)
        {
            if (rit->col_handle)
                iter->releaseHandle(rit->col_handle);

            if (rit->replace_handle)
                iter->releaseHandle(rit->replace_handle);
        }

        if (job)
        {
            IJobInternalPtr ijob = job;
            if (ijob)
            {
                ijob->setStatus(tango::jobFailed);
            }
        }

        return -1;
    }


    // -- initialize the job with starting time, max_count, etc. --
    IJobInternalPtr ijob = job;
    if (ijob)
    {
        tango::rowpos_t max_count = 0;
        
        if (set->getSetFlags() & tango::sfFastRowCount)
        {
            max_count = set->getRowCount();
        }

        ijob->setMaxCount(max_count);
        ijob->setCurrentCount(0);
        ijob->setStatus(tango::jobRunning);
        ijob->setStartTime(time(NULL));
    }


    // -- start at the top --
    s_iter->goFirst();

    // -- do the replacing --
    int update_count = 0;
    size_t col;
    size_t col_count = replace.size();

    while (!s_iter->isDone())
    {
        for (col = 0; col < col_count; ++col)
        {
            if (iter->isNull(replace[col].replace_handle))
            {
                col_update[col].null = true;
                continue;
            }
             else
            {
                col_update[col].null = false;
            }
            
            switch (replace[col].col_type)
            {
                case tango::typeCharacter:
                    col_update[col].str_val = iter->getString(replace[col].replace_handle);
                    break;

                case tango::typeWideCharacter:
                    col_update[col].wstr_val = iter->getWideString(replace[col].replace_handle);
                    break;

                case tango::typeNumeric:
                case tango::typeDouble:
                    col_update[col].dbl_val = iter->getDouble(replace[col].replace_handle);
                    break;

                case tango::typeInteger:
                    col_update[col].int_val = iter->getInteger(replace[col].replace_handle);
                    break;

                case tango::typeDate:
                case tango::typeDateTime:
                    col_update[col].date_val = iter->getDateTime(replace[col].replace_handle);
                    break;

                case tango::typeBoolean:
                    col_update[col].bool_val = iter->getBoolean(replace[col].replace_handle);
                    break;
            }
        }

        if (set_update->updateRow(iter->getRowId(), col_update, col_count))
        {
            update_count++;
        }


        s_iter->goNext();
        
        if (s_iter->isCancelled())
        {
            break;
        }
    }

    for (rit = replace.begin(); rit != replace.end(); ++rit)
    {
        if (rit->col_handle)
        {
            iter->releaseHandle(rit->col_handle);
        }
        if (rit->replace_handle)
        {
            iter->releaseHandle(rit->replace_handle);
        }
    }
    
    if (job)
    {
        if (!job->getCancelled())
        {
            ijob->setCurrentCount(ijob->getMaxCount());
            ijob->setStatus(tango::jobFinished);
            ijob->setFinishTime(time(NULL));
        }
    }

    delete[] col_update;
    delete s_iter;
    
    return update_count;
 }
 
 
bool sqlUpdate(tango::IDatabasePtr db,
               const std::wstring& _command,
               ThreadErrorInfo& error,
               tango::IJob* job)
{
    SqlStatement stmt(_command);

    stmt.addKeyword(L"UPDATE");
    stmt.addKeyword(L"SET");
    stmt.addKeyword(L"WHERE");

    if (!stmt.getKeywordExists(L"UPDATE"))
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; UPDATE statement missing UPDATE clause");
        return false;
    }

    if (!stmt.getKeywordExists(L"SET"))
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; UPDATE statement missing SET clause");
        return false;
    }

    std::wstring update = stmt.getKeywordParam(L"UPDATE");
    std::wstring params = stmt.getKeywordParam(L"SET");
    std::wstring filter;
    
    if (stmt.getKeywordExists(L"WHERE"))
    {
        filter = stmt.getKeywordParam(L"WHERE");
    }
    
    dequote(update, '[', ']');
    
    tango::ISetPtr set = db->openSet(update);
    if (set.isNull())
    {
        wchar_t buf[1024]; // some paths might be long
        swprintf(buf, 1024, L"Unable to update rows because table [%ls] cannot be opened", update.c_str());
        error.setError(tango::errorGeneral, buf);
        return false;
    }

    int result = doUpdate(set, filter, params, error, job);
    if (result == -1)
        return false;
        
    return true;
}


