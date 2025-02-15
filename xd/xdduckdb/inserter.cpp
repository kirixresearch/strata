/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#include <kl/klib.h>
#include <kl/md5.h>
#include <kl/utf8.h>
#include <xd/xd.h>
#include "database.h"
#include "inserter.h"
#include "../xdcommon/util.h"
#include "util.h"





// DuckdbRowInserter class implementation

DuckdbRowInserter::DuckdbRowInserter(DuckdbDatabase* db, const std::wstring& table)
{
    m_inserting = false;
    m_stmt = NULL;
    m_table = table;

    m_database = db;
    m_database->ref();
}

DuckdbRowInserter::~DuckdbRowInserter()
{
    if (m_stmt)
    {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    std::vector<DuckdbRowInserterData>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        delete[] it->str_data;
    }

    m_database->unref();
}

xd::objhandle_t DuckdbRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<DuckdbRowInserterData>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (0 == wcscasecmp(column_name.c_str(),
                            it->name.c_str()))
        {
            return (xd::objhandle_t)(&(*it));
        }
    }

    return 0;
}

bool DuckdbRowInserter::startInsert(const std::wstring& col_list)
{
    std::vector<std::wstring> cols;

    xd::Structure structure = m_database->describeTable(m_table);
    if (structure.isNull())
        return false;

    size_t i, col_count;

    if (col_list.empty() || col_list == L"*")
    {
        //  inserting every field
        col_count = structure.getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            cols.push_back(structure.getColumnName(i));
        }
    }
     else
    {
        kl::parseDelimitedList(col_list, cols, L',', true);
    }


    col_count = cols.size();

    // make sure all of the columns are real

    for (i = 0; i < col_count; ++i)
    {
        if (!structure.getColumnExist(cols[i]))
            return false;
    }


    // create the SQL insert statement

    std::wstring sql;
    sql = L"INSERT INTO ";
    sql += xdGetTablenameFromPath(m_table);
    sql += L" (";

    for (i = 0; i < col_count; ++i)
    {
        sql += cols[i];
        if (i+1 < col_count)
            sql += L","; 
    }

    sql += L") VALUES (";

    for (i = 0; i < col_count; ++i)
    {
        sql += L"?";
        if (i+1 < col_count)
            sql += L","; 
    }
    
    sql += L");";





    // begin a transaction
    sqlite3_exec(m_database->m_sqlite, "BEGIN TRANSACTION;", NULL, NULL, NULL);





    std::string ascsql;
    ascsql = kl::tostring(sql);


    if (SQLITE_OK != sqlite3_prepare(m_database->m_sqlite, 
                                     ascsql.c_str(),  // stmt
                                     ascsql.length(),
                                     &m_stmt,
                                     0))
    {
        return false;
    }


    for (i = 0; i < col_count; ++i)
    {
        DuckdbRowInserterData data;

        data.colinfo = structure.getColumnInfo(cols[i]);
        data.type = data.colinfo.type;
        data.length = data.colinfo.width;
        data.idx = i+1;
        data.name = cols[i];
        
        data.buf_len = (data.length * 5) + 1;
        if (data.type == xd::typeDate || 
            data.type == xd::typeDateTime)
        {
            data.buf_len = 30;
        }
        
        data.str_data = new char[data.buf_len];

        m_fields.push_back(data);
    }

    return true;
}

void DuckdbRowInserter::finishInsert()
{
    m_inserting = false;

    if (m_stmt)
    {
        sqlite3_finalize(m_stmt);
        m_stmt = NULL;
    }

    // commit this transaction
    sqlite3_exec(m_database->m_sqlite, "COMMIT;", NULL, NULL, NULL);

}

bool DuckdbRowInserter::insertRow()
{
    int rc;

    while (1)
    {
        rc = sqlite3_step(m_stmt);
        if (rc == SQLITE_BUSY)
        {
            continue;
        }
         else if (rc == SQLITE_ERROR)
        {
            sqlite3_reset(m_stmt);
            return false;
        }
         else
        {
            break;
        }
    }

    sqlite3_reset(m_stmt);
    return true;
}

bool DuckdbRowInserter::flush()
{
    return false;
}

bool DuckdbRowInserter::putRawPtr(xd::objhandle_t column_handle,
                              const unsigned char* value,
                              int length)
{
    return false;
}

bool DuckdbRowInserter::putString(xd::objhandle_t column_handle,
                              const std::string& value)
{
    DuckdbRowInserterData* r = (DuckdbRowInserterData*)column_handle;
    if (!r)
        return false;

    strncpy(r->str_data, value.c_str(), r->length);
    r->str_data[r->length] = 0;
    
    if (SQLITE_OK != sqlite3_bind_text(m_stmt,
                                       r->idx,
                                       r->str_data,
                                       value.length(),
                                       SQLITE_STATIC
                                       ))
    {
        return false;
    }

    return true;
}

bool DuckdbRowInserter::putWideString(xd::objhandle_t column_handle,
                                  const std::wstring& value)
{
    DuckdbRowInserterData* r = (DuckdbRowInserterData*)column_handle;
    if (!r)
        return false;

    size_t output_buf_size = 0;
    kl::utf8_wtoutf8(r->str_data, r->buf_len, value.c_str(), value.length(), &output_buf_size);
        
    if (SQLITE_OK != sqlite3_bind_text(m_stmt,
                                       r->idx,
                                       r->str_data,
                                       output_buf_size,
                                       SQLITE_STATIC
                                       ))
    {
        return false;
    }

    return true;
}

bool DuckdbRowInserter::putDouble(xd::objhandle_t column_handle,
                              double value)
{
    DuckdbRowInserterData* r = (DuckdbRowInserterData*)column_handle;
    if (!r)
        return false;
        
    if (SQLITE_OK != sqlite3_bind_double(m_stmt,
                                       r->idx,
                                       value))
    {
        return false;
    }

    return true;
}

bool DuckdbRowInserter::putInteger(xd::objhandle_t column_handle,
                               int value)
{
    DuckdbRowInserterData* r = (DuckdbRowInserterData*)column_handle;
    if (!r)
        return false;
        
    if (SQLITE_OK != sqlite3_bind_int(m_stmt,
                                      r->idx,
                                      value))
    {
        return false;
    }

    return true;
}

bool DuckdbRowInserter::putBoolean(xd::objhandle_t column_handle,
                               bool value)
{
    DuckdbRowInserterData* r = (DuckdbRowInserterData*)column_handle;
    if (!r)
        return false;
    r->str_data[0] = value ? '0':'1';
    r->str_data[1] = 0;
    
    if (SQLITE_OK != sqlite3_bind_text(m_stmt,
                                   r->idx,
                                   r->str_data,
                                   -1,
                                   SQLITE_STATIC
                                   ))
    {
        return false;
    }
    
    return true;
}

bool DuckdbRowInserter::putDateTime(xd::objhandle_t column_handle,
                                xd::datetime_t datetime)
{
    DuckdbRowInserterData* r = (DuckdbRowInserterData*)column_handle;
    if (!r)
        return false;

    int y = 0, m = 0, d = 0,
        hh = 0, mm = 0, ss = 0;

    if (datetime > 0)
    {
        julianToDate(datetime >> 32, &y, &m, &d);

        xd::datetime_t time_stamp = (datetime >> 32);
        if (time_stamp)
        {
            hh = (int)(time_stamp / 3600000);
            time_stamp -= (hh * 3600000);
            mm = (int)(time_stamp / 60000);
            time_stamp -= (mm * 60000);
            ss = (int)(time_stamp / 1000);
            time_stamp -= (ss * 1000);
        }
    }



    if (r->type == xd::typeDate)
    {
        sprintf(r->str_data,
                "%04d-%02d-%02d",
                y, m, d);
    }
     else
    {
        sprintf(r->str_data,
                "%04d-%02d-%02d %02d:%02d:%02d",
                y, m, d, hh, mm, ss);
    }
    
    if (SQLITE_OK != sqlite3_bind_text(m_stmt,
                                       r->idx,
                                       r->str_data,
                                       -1,
                                       SQLITE_STATIC
                                       ))
    {
        return false;
    }

    return true;
}

bool DuckdbRowInserter::putNull(xd::objhandle_t column_handle)
{
    DuckdbRowInserterData* r = (DuckdbRowInserterData*)column_handle;
    if (!r)
        return false;
        
    if (SQLITE_OK != sqlite3_bind_null(m_stmt, r->idx))
    {
        return false;
    }

    return true;
}

