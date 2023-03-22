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
#include <kl/memory.h>
#include "localrowcache2.h"
#include "util.h"
#include "../xdcommon/sqlite3.h"



LocalRowValue::LocalRowValue()
{
    is_null = false;
    data = NULL;
    len = 0;
}

LocalRowValue::~LocalRowValue()
{
    if (data)
    {
        unsigned char* _data = data;
        data = NULL;
        free(_data);
    }
}

void LocalRowValue::setData(unsigned char* _data, size_t _len)
{
    if (!_data)
    {
        return;
    }

    if (data)
    {
        if (len >= _len)
        {
            memcpy(data, _data, _len);
            len = _len;
            return;
        }

        free(data);
    }

    data = (unsigned char*)malloc(_len > 0 ? _len : 1);
    if (_len)
    {
        memcpy(data, _data, _len);
    }

    len = _len;

}



LocalRow2::LocalRow2()
{
    m_buf = NULL;
}

LocalRow2::~LocalRow2()
{
    for (auto el : m_values)
    {
        delete el;
    }
}


void LocalRow2::setColumnData(size_t col_idx, LocalRowValue& val)
{
    LocalRowValue* v;

    while (col_idx >= m_values.size())
    {
        v = new LocalRowValue;
        v->is_null = true;
        m_values.push_back(v);
    }

    m_values[col_idx]->setData(val.getData(), val.getDataLength());
}



LocalRowValue& LocalRow2::getColumnData(size_t col_idx)
{
    while (col_idx >= m_values.size())
    {
        LocalRowValue* v = new LocalRowValue;
        v->is_null = true;
        m_values.push_back(v);
    }

    return *m_values[col_idx];
}

size_t LocalRow2::getColumnCount() const
{
    return m_values.size();
}

void* LocalRow2::serialize(size_t* len)
{
    if (!m_buf)
    {
        m_buf = new kl::membuf;
    }

    size_t buf_alloc_len = 0;
    for (auto el : m_values)
    {
        buf_alloc_len += el->getDataLength() + sizeof(int);
    }

    m_buf->alloc(buf_alloc_len);
    m_buf->setDataSize(0);

    for (auto el : m_values)
    {
        // TODO: fill out

    }

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

