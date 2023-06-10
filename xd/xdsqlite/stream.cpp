/*!
 *
 * Copyright (c) 2007-2022, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2023-02-19
 *
 */


#include <kl/string.h>
#include <kl/utf8.h>
#include <xd/xd.h>
#include "database.h"
#include "stream.h"
#include <cstdlib>


SlStream::SlStream(SlDatabase* database)
{
    m_mime_type = L"application/octet-stream";
    m_offset = 0;
    m_block_size = SlStream::DEFAULT_BLOCK_SIZE;

    m_database = database;
    m_database->ref();
}

SlStream::~SlStream()
{
    m_database->unref();
}

bool SlStream::init(const std::wstring& stream_object_name, const std::wstring& mime_type, int block_size)
{
    m_mime_type = mime_type;
    m_stream = kl::toUtf8(xdGetTablenameFromPath(stream_object_name, true));
    m_block_size = block_size;
    return true;
}

bool SlStream::readBlock(long long block, void* buf, unsigned long* read_count)
{
    std::string sql = "SELECT data from " + m_stream + " WHERE block_id=";

    char block_str[80];
    snprintf(block_str, 80, "%lld", block);
    sql += block_str;

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(m_database->m_sqlite, sql.c_str(), sql.length(), &stmt, NULL);
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

    if (blob_len > m_block_size)
    {
        blob_len = m_block_size;
    }


    if (blob)
    {
        *read_count = (unsigned long)blob_len;
        memcpy(buf, blob, blob_len);
    }
    else
    {
        *read_count = 0;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SlStream::writeBlock(long long block, const void* buf, unsigned long size)
{
    // first, find out the largest numbered block already in the table
    std::string sql = "SELECT max(block_id) as max_block_id from " + m_stream;

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(m_database->m_sqlite, sql.c_str(), -1, &stmt, NULL);
    if (!stmt)
    {
        return false;
    }

    long long max_block_id = 0;
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        max_block_id = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    // now create blocks up to the block we want to write to
    for (long long block_i = max_block_id+1; block_i < block; ++block_i)
    {
        sql = kl::stdsprintf("INSERT INTO %s (block_id, data) VALUES (%lld, ZEROBLOCK(%d))", m_stream.c_str(), block_i, m_block_size);
        if (SQLITE_OK != sqlite3_exec(m_database->m_sqlite, sql.c_str(), NULL, NULL, NULL))
            return false;
    }

    // now write the block we were called to write
    sql = kl::stdsprintf("INSERT INTO %s (block_id, data) VALUES (%lld, ?) ON CONFLICT(block_id) DO UPDATE SET data=excluded.data", m_stream.c_str(), block, m_block_size);

    if (sqlite3_prepare_v2(m_database->m_sqlite, sql.c_str(), -1, &stmt, NULL))
    {
        return false;
    }

    if (block < max_block_id && size < (unsigned long)m_block_size)
    {
        // need to pad
        unsigned char* writebuf = new unsigned char[m_block_size];
        memset(writebuf, 0, m_block_size);
        memcpy(writebuf, buf, size);
        sqlite3_bind_blob(stmt, 1, writebuf, m_block_size, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        delete[] writebuf;
    }
    else
    {
        sqlite3_bind_blob(stmt, 1, buf, size, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    return true;
}


bool SlStream::read(void* buf,
                    unsigned long read_size,
                    unsigned long* read_count)
{
    unsigned char* cbuf = (unsigned char*)buf;
    long long block = (m_offset / m_block_size) + 1;
    int block_offset = (int)(m_offset % m_block_size);
    char* data = new char[m_block_size];
    unsigned long data_size;
    bool done = false;

    *read_count = 0;

    while (!done)
    {
        data_size = 0;
        done = !readBlock(block, data, &data_size);
        
        if (read_size < data_size)
            data_size = read_size;

        if (block_offset > 0)
        {
            if ((unsigned long)block_offset < data_size)
            {
                data_size -= block_offset;
                memcpy(cbuf, data+block_offset, data_size);
            }
            
            block_offset = 0;
        }
        else
        {
            memcpy(cbuf, data, data_size);
        }

        block++;
        read_size -= data_size;
        *read_count += data_size;
        cbuf += data_size;
        m_offset += data_size;
    }


    delete[] data;
    return true;
}
                  
bool SlStream::write(const void* buf,
                     unsigned long write_size,
                     unsigned long* written_count)
{
    const unsigned char* cbuf = (const unsigned char*)buf;
    long long block = (m_offset / m_block_size) + 1;
    int block_offset = (int)(m_offset % m_block_size);
    long max_write;
    char* data = new char[m_block_size];
    unsigned long data_size;

    while (write_size > 0)
    {
        max_write = (long)write_size;
        if (max_write > m_block_size - block_offset)
        {
            max_write = m_block_size - block_offset;
        }

        if (block_offset > 0)
        {
            // writing a partial block; first read in the existing block
            memset(data, 0, m_block_size);
            readBlock(block, data, &data_size);
            memcpy(data + block_offset, cbuf, max_write);
            writeBlock(block, data, m_block_size);
        }
        else
        {
            memset(data, 0, m_block_size);
            memcpy(data + block_offset, cbuf, max_write);
            writeBlock(block, data, max_write);
        }

        cbuf += max_write;
        write_size -= (unsigned long)max_write;
        ++block;
    }

    delete[] data;
    return true;
}

bool SlStream::seek(long long seek_pos, int whence)
{
    switch (whence)
    {
        case xd::seekSet:
        {
            m_offset = seek_pos;
        }
        break;

        case xd::seekCur:
        {
            m_offset += seek_pos;
        }
        break;

        case xd::seekEnd:
        {
            m_offset = this->getSize() + seek_pos;
        }
        break;
    }

    return true;
}


long long SlStream::getSize()
{
    // first, find out the largest numbered block already in the table
    std::string sql = "select sum(length(data)) from " + m_stream;

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(m_database->m_sqlite, sql.c_str(), -1, &stmt, NULL);
    if (!stmt)
    {
        return 0;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        return sqlite3_column_int64(stmt, 0);
    }
    else
    {
        return 0;
    }
}

std::wstring SlStream::getMimeType()
{
    return m_mime_type;
}
