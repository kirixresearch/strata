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
#include <kl/utf8.h>
#include <kl/memory.h>
#include "localrowcache2.h"
#include "../xdcommon/sqlite3.h"



LocalRowValue::LocalRowValue()
{
    is_null = false;
    data = NULL;
    len = 0;
    type = LocalRowValue::typeBinary;
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

void LocalRowValue::setNull()
{
    if (data)
    {
        free(data);
        data = NULL;
    }

    len = 0;
    is_null = true;
}

bool LocalRowValue::setData(const unsigned char* _data, size_t _len)
{
    if (!_data)
    {
        setNull();
        return true;
    }

    if (data)
    {
        if (len >= _len)
        {
            memcpy(data, _data, _len);
            len = _len;
            is_null = false;
            return true;
        }

        free(data);
        data = NULL;
    }

    unsigned char* newbuf = (unsigned char*)malloc(_len > 0 ? _len : 1);
    if (!newbuf)
    {
        return false;
    }

    data = newbuf;

    if (_len)
    {
        memcpy(data, _data, _len);
    }

    len = _len;
    is_null = false;
    return true;
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

    delete m_buf;
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
    m_values[col_idx]->setType(val.getType());
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

const unsigned char* LocalRow2::serialize(size_t* len)
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

    unsigned char col_flags;
    unsigned char length_code[4];
    size_t col_length;

    for (auto el : m_values)
    {
        col_flags = (el->getType() << 3);

        if (el->isNull())
        {
            col_flags |= 0x05;
            m_buf->append(&col_flags, 1);
        }
        else
        {
            col_length = el->getDataLength();
  
            if (col_length <= 0xff)
            {
                col_flags |= 0x01;
                m_buf->append(&col_flags, 1);
                length_code[0] = (unsigned char)(col_length & 0x000000ff);
                m_buf->append(length_code, 1);
            }
            else if (col_length <= 0xffff)
            {
                col_flags |= 0x02;
                m_buf->append(&col_flags, 1);
                length_code[0] = (unsigned char)((col_length & 0x0000ff00) >> 8);
                length_code[1] = (unsigned char)(col_length & 0x000000ff);
                m_buf->append(length_code, 2);
            }
            else if (col_length <= 0xffffff)
            {
                col_flags |= 0x03;
                m_buf->append(&col_flags, 1);
                length_code[0] = (unsigned char)((col_length & 0x00ff0000) >> 16);
                length_code[1] = (unsigned char)((col_length & 0x0000ff00) >> 8);
                length_code[2] = (unsigned char)(col_length & 0x000000ff);
                m_buf->append(length_code, 3);
            }
            else
            {
                col_flags |= 0x04;
                m_buf->append(&col_flags, 1);
                length_code[0] = (unsigned char)((col_length & 0xff000000) >> 24);
                length_code[1] = (unsigned char)((col_length & 0x00ff0000) >> 16);
                length_code[2] = (unsigned char)((col_length & 0x0000ff00) >> 8);
                length_code[3] = (unsigned char)(col_length & 0x000000ff);
                m_buf->append(length_code, 4);
            }

            m_buf->append(el->getData(), el->getDataLength());
        }
    }

    col_flags = 0;
    m_buf->append(&col_flags, 1);

    *len = m_buf->getDataSize();
    return m_buf->getData();
}

void LocalRow2::unserialize(const unsigned char* buf, size_t len)
{
    size_t col_idx = 0;
    LocalRowValue* v = new LocalRowValue;
    size_t col_length;
    unsigned char length_ind;

    while (*buf)
    {
        if (col_idx >= m_values.size())
        {
            v = new LocalRowValue;
            m_values.push_back(v);
        }
        else
        {
            v = m_values[col_idx];
        }

        col_length = 0;
        length_ind = *buf & 0b111;

        v->setType(*buf >> 3);

        if (length_ind == 0x05)
        {
            ++buf;
            v->setNull();
        }
        else if (length_ind == 0x01)
        {
            ++buf;
            col_length |= (size_t)*buf;
            ++buf;
            v->setData(buf, col_length);
            buf += col_length;
        }
        else if (length_ind == 0x02)
        {
            ++buf;
            col_length |= ((size_t)*buf) << 8;
            ++buf;
            col_length |= ((size_t)*buf);
            ++buf;
            v->setData(buf, col_length);
            buf += col_length;
        }
        else if (length_ind == 0x03)
        {
            ++buf;
            col_length |= ((size_t)*buf) << 16;
            ++buf;
            col_length |= ((size_t)*buf) << 8;
            ++buf;
            col_length |= ((size_t)*buf);
            ++buf;
            v->setData(buf, col_length);
            buf += col_length;
        }
        else if (length_ind == 0x04)
        {
            ++buf;
            col_length |= ((size_t)*buf) << 24;
            ++buf;
            col_length |= ((size_t)*buf) << 16;
            ++buf;
            col_length |= ((size_t)*buf) << 8;
            ++buf;
            col_length |= ((size_t)*buf);
            ++buf;
            v->setData(buf, col_length);
            buf += col_length;
        }

        ++col_idx;
    }

    if (col_idx < m_values.size())
    {
        for (size_t i = col_idx; i < m_values.size(); ++i)
        {
            delete m_values[i];
        }

        m_values.resize(col_idx);
    }
}


// -- LocalRowCache implementation --

LocalRowCache2::LocalRowCache2()
{
    m_sqlite = NULL;
}

LocalRowCache2::~LocalRowCache2()
{
    if (m_sqlite)
    {
        sqlite3* sqlite = (sqlite3*)m_sqlite;
        m_sqlite = NULL;
        sqlite3_close(sqlite);
    }

    if (m_path.length() > 0)
    {
        xf_remove(m_path);
    }
}

bool LocalRowCache2::init(const std::wstring& path)
{
    m_path = path;

    std::string utf8_path;

    if (path.length() == 0)
    {
        utf8_path = ":memory:";
    }
    else
    {
        utf8_path = kl::toUtf8(path);
    }


    sqlite3* db = NULL;

    if (SQLITE_OK != sqlite3_open(utf8_path.c_str(), &db))
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
    if (m_sqlite == NULL)
    {
        if (!init())
        {
            return false;
        }
    }

    sqlite3* db = (sqlite3*)m_sqlite;

    std::string sql = "SELECT data from rows WHERE id=";

    char rowid_str[80];
    snprintf(rowid_str, 80, "%lld", rowid);
    sql += rowid_str;

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(db, sql.c_str(), (int)sql.length(), &stmt, NULL);
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

    row.unserialize((unsigned char*)blob, (size_t)blob_len);

    sqlite3_finalize(stmt);
    return true;
}

bool LocalRowCache2::putRow(long long rowid, LocalRow2& row)
{
    if (m_sqlite == NULL)
    {
        if (!init())
        {
            return false;
        }
    }

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

    sqlite3_bind_blob(stmt, 1, data, (int)size, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return true;
}

