/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-04-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "sqlcommon.h"
#include "jobinfo.h"
#include "util.h"
#include "tango_private.h"
#include <kl/portable.h>
#include <map>
#include <ctime>



static int doDelete(tango::ISetPtr set,
                    const std::wstring& filter,
                    tango::IJob* job)
{
    return 0;

/*
    int delete_count = 0;

    // get a row deleter from this object

    tango::IRowDeleterPtr sp_row_deleter = set->getRowDeleter();
    if (sp_row_deleter.isNull())
        return -1;

    // create physical iterator and initialize expression handles

    tango::IIteratorPtr sp_iter = set->createIterator(L"", L"", NULL);
    if (sp_iter.isNull())
        return -1;


    // create an SqlIterator object
    
    SqlIterator* s_iter = SqlIterator::createSqlIterator(sp_iter, filter, job);
    if (!s_iter)
    {
        return -1;
    }


    tango::IIterator* iter = sp_iter.p;
    tango::IRowDeleter* row_deleter = sp_row_deleter.p;

    // initialize job
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



    // start at the top
    s_iter->goFirst();

    // do the deleting
    bool cancelled = false;
    row_deleter->startDelete();

    while (!s_iter->isDone())
    {
        if (row_deleter->deleteRow(s_iter->getIterator()->getRowId()))
        {
            delete_count++;
        }

        s_iter->goNext();
        
        if (s_iter->isCancelled())
        {
            cancelled = true;
            break;
        }
    }

    if (cancelled)
        row_deleter->cancelDelete();
         else
        row_deleter->finishDelete();
    
    delete s_iter;
    
    if (job)
    {
        if (!job->getCancelled())
        {
            ijob->setCurrentCount(ijob->getMaxCount());
            ijob->setStatus(tango::jobFinished);
            ijob->setFinishTime(time(NULL));
        }
    }
    
    return delete_count;
    */
}



bool sqlDelete(tango::IDatabasePtr db,
               const std::wstring& _command,
               ThreadErrorInfo& error,
               tango::IJob* job)
{
    SqlStatement stmt(_command);

    stmt.addKeyword(L"DELETE");
    stmt.addKeyword(L"FROM");
    stmt.addKeyword(L"WHERE");
    stmt.addKeyword(L"RESTORE");

    if (!stmt.getKeywordExists(L"DELETE"))
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; DELETE statement missing DELETE clause");
        return false;
    }

    if (!stmt.getKeywordExists(L"FROM"))
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; DELETE statement missing FROM clause");
        return false;
    }

    if (!stmt.getKeywordExists(L"WHERE") && !stmt.getKeywordExists(L"RESTORE"))
    {
        error.setError(tango::errorSyntax, L"Invalid syntax; DELETE statement missing WHERE clause");
        return false;
    }

    std::wstring table = stmt.getKeywordParam(L"FROM");
    std::wstring filter = stmt.getKeywordParam(L"WHERE");

    dequote(table, '[', ']');
    
    tango::ISetPtr set = db->openSet(table);
    if (set.isNull())
    {
        wchar_t buf[1024]; // some paths might be long
        swprintf(buf, 1024, L"Unable to delete rows because table [%ls] cannot be opened", table.c_str());
        error.setError(tango::errorGeneral, buf);
        return false;
    }

    if (stmt.getKeywordExists(L"RESTORE"))
    {
        ISetRestoreDeletedPtr sp = set;
        if (!sp->restoreDeleted())
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to restore deleted rows in table [%ls]", table.c_str());
            error.setError(tango::errorGeneral, buf);
            return false;
        }
        
        return true;
    }

    int result = doDelete(set, filter, job);
    if (result == -1)
    {
        error.setError(tango::errorGeneral, L"Unable to process DELETE statement");
        return false;
    }

    return true;
}


