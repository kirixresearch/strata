/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-21
 *
 */


#include <xd/xd.h>
#include "localrowcache2.h"
#include "util.h"
#include "../xdcommon/sqlite3.h"



LocalRowValue& LocalRow2::getColumnData(size_t col_idx)
{
    if (col_idx >= m_values.size())
    {
        m_values.resize(col_idx);
    }

    return m_values[col_idx];
}

size_t LocalRow2::getColumnCount() const
{
    return m_values.size();
}

void* LocalRow2::serialize(size_t* len)
{
    return NULL;
}

void LocalRow2::unserialize(const void* buf, size_t len)
{
}


// -- LocalRowCache implementation --


LocalRowCache2::LocalRowCache2()
{
    m_sqlite = NULL;
}

LocalRowCache2::~LocalRowCache2()
{
    
}

bool LocalRowCache2::init()
{
    sqlite3* db = NULL;

    if (SQLITE_OK != sqlite3_open(":memory:", &db))
    {
        // database could not be opened
        return false;
    }



    std::string sql = "create table rows (id int primary key, data blob)";
    if (SQLITE_OK != sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL))
    {
        return false;
    }


    m_sqlite = (void*)db;


    return true;
}

bool LocalRowCache2::getRow(long long rowid, LocalRow2& row)
{
    sqlite3* db = (sqlite3*)m_sqlite;

    std::string sql = "SELECT data from rows WHERE id=";

    char rowid_str[80];
    snprintf(rowid_str, 80, "%lld", rowid);
    sql += rowid_str;

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(db, sql.c_str(), sql.length(), &stmt, NULL);
    if (!stmt)
    {
        return false;
    }

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return false;
    }

    const void* blob = sqlite3_column_blob(stmt, 0);
    int blob_len = sqlite3_column_bytes(stmt, 0);
    if (blob_len < 0)
    {
        sqlite3_finalize(stmt);
        return false;
    }

    row.unserialize(blob, (size_t)blob_len);

    sqlite3_finalize(stmt);
    return true;
}

bool LocalRowCache2::putRow(long long rowid, LocalRow2& row)
{
    sqlite3* db = (sqlite3*)m_sqlite;

    // now write the block we were called to write
    char sql[255];
    snprintf(sql, 254, "INSERT INTO rows (id, data) VALUES (%lld, ?) ON CONFLICT(id) DO UPDATE SET data=excluded.data", rowid);

    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL))
    {
        return false;
    }

    size_t size = 0;
    const void* data = row.serialize(&size);

    sqlite3_bind_blob(stmt, 1, data, size, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return true;
}

