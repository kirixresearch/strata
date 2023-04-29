/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-04-28
 *
 */


#include <xd/xd.h>
#include "xdcommonsql.h"
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/util.h"
#include "../xdcommon/errorinfo.h"
#include <kl/portable.h>
#include <kl/string.h>
#include <ctime>



static int doDelete(IXdsqlDatabasePtr db,
                    const std::wstring& path,
                    const std::wstring& filter,
                    xd::IJob* job)
{
    int delete_count = 0;

    IXdsqlTablePtr tbl = db->openTable(path);
    if (tbl.isNull())
        return -1;

    // get a row deleter from this object
    IXdsqlRowDeleterPtr sp_row_deleter = tbl->getRowDeleter();
    if (sp_row_deleter.isNull())
        return -1;

    // create physical iterator and initialize expression handles
    xd::IDatabasePtr tdb = db;
    if (tdb.isNull())
        return -1;

    xd::IIteratorPtr sp_iter = tdb->query(path, L"", L"", L"", NULL);
    if (sp_iter.isNull())
        return -1;

    // create an SqlIterator object
    
    SqlIterator* s_iter = SqlIterator::createSqlIterator(sp_iter, filter, job);
    if (!s_iter)
    {
        return -1;
    }


    xd::IIterator* iter = sp_iter.p;
    IXdsqlRowDeleter* row_deleter = sp_row_deleter.p;

    // initialize job
    IJobInternalPtr ijob = job;
    if (ijob)
    {
        xd::rowpos_t max_count = 0;

        xd::IFileInfoPtr finfo = tdb->getFileInfo(path);
        if (finfo.isOk() && (finfo->getFlags() & xd::sfFastRowCount))
        {
            max_count = finfo->getRowCount();
        }

        ijob->setMaxCount(max_count);
        ijob->setCurrentCount(0);
        ijob->setStatus(xd::jobRunning);
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
            ijob->setStatus(xd::jobFinished);
            ijob->setFinishTime(time(NULL));
        }
    }
    
    return delete_count;
}



bool sqlDelete(xd::IDatabasePtr db,
               const std::wstring& _command,
               ThreadErrorInfo& error,
               xd::IJob* job)
{
    IXdsqlDatabasePtr xdb = db;
    if (xdb.isNull())
        return false;

    SqlStatement stmt(_command);

    stmt.addKeyword(L"DELETE");
    stmt.addKeyword(L"FROM");
    stmt.addKeyword(L"WHERE");
    stmt.addKeyword(L"RESTORE");

    if (!stmt.getKeywordExists(L"DELETE"))
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; DELETE statement missing DELETE clause");
        return false;
    }

    if (!stmt.getKeywordExists(L"FROM"))
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; DELETE statement missing FROM clause");
        return false;
    }

    if (!stmt.getKeywordExists(L"WHERE") && !stmt.getKeywordExists(L"RESTORE"))
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; DELETE statement missing WHERE clause");
        return false;
    }

    std::wstring table = stmt.getKeywordParam(L"FROM");
    std::wstring filter = stmt.getKeywordParam(L"WHERE");

    kl::trim(filter);

    if (stmt.getKeywordExists(L"WHERE") && filter.length() == 0)
    {
        error.setError(xd::errorSyntax, L"Invalid syntax; DELETE statement missing WHERE condition");
        return false;
    }
        
    dequote(table, '[', ']');
    
    xd::IFileInfoPtr finfo = db->getFileInfo(table);
    if (finfo.isNull() || finfo->getType() != xd::filetypeTable)
    {
        wchar_t buf[1024]; // some paths might be long
        swprintf(buf, 1024, L"Unable to delete rows because table [%ls] cannot be opened", table.c_str());
        buf[1023] = 0;
        error.setError(xd::errorGeneral, buf);
        return false;
    }

    if (stmt.getKeywordExists(L"RESTORE"))
    {
        IXdsqlTablePtr tbl = xdb->openTable(table);

        if (tbl.isNull() || !tbl->restoreDeleted())
        {
            wchar_t buf[1024]; // some paths might be long
            swprintf(buf, 1024, L"Unable to restore deleted rows in table [%ls]", table.c_str());
            buf[1023] = 0;
            error.setError(xd::errorGeneral, buf);
            return false;
        }
        
        return true;
    }

    int result = doDelete(db, table, filter, job);
    if (result == -1)
    {
        error.setError(xd::errorGeneral, L"Unable to process DELETE statement");
        return false;
    }

    return true;
}


