/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-05-12
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "libpq-fe.h"
#include "libpq/libpq-fs.h"
#include <xd/xd.h>
#include "database.h"
#include "stream.h"


PgsqlStream::PgsqlStream(PgsqlDatabase* database)
{
    m_conn = NULL;
    m_fd = -1;

    m_database = database;
    m_database->ref();
}

PgsqlStream::~PgsqlStream()
{
    if (m_fd != -1)
        lo_close(m_conn, m_fd);


    if (m_conn)
    {
        PQexec(m_conn, "COMMIT");

        m_database->closeConnection(m_conn);
    }

    m_database->unref();
}

bool PgsqlStream::init(Oid id, PGconn* conn)
{
    if (conn)
    {
        m_conn = conn;
    }
     else
    {
        m_conn = m_database->createConnection();
        if (!m_conn)
            return false;
    }

    m_fd = lo_open(m_conn, id, INV_READ | INV_WRITE);
    if (m_fd == -1)
    {
        m_database->closeConnection(m_conn);
        m_conn = NULL;
        return false;
    }

    return true;
}


bool PgsqlStream::read(void* buf,
                       unsigned long read_size,
                       unsigned long* read_count)
{
    int r = lo_read(m_conn, m_fd, (char*)buf, read_size);

    if (r < 0)
        return false;

    if (read_count)
        *read_count = (unsigned long)r;

    return true;
}
                  
bool PgsqlStream::write(const void* buf,
          unsigned long write_size,
          unsigned long* written_count)
{
    int w = lo_write(m_conn, m_fd, (const char*)buf, write_size);
    if (w < 0)
        return false;

    if (written_count)
        *written_count = (unsigned long)w;
    
    return true;
}

bool PgsqlStream::seek(long long seek_pos, int whence)
{
    int pg_whence;

    switch (whence)
    {
        default:          return false; // invalid seek value
        case xd::seekSet: pg_whence = 0; break;
        case xd::seekCur: pg_whence = 1; break;
        case xd::seekEnd: pg_whence = 2; break;
    }

    // TODO: needs to be lo_lseek64()
    int res = lo_lseek(m_conn, m_fd, (int)seek_pos, pg_whence);
    if (res < 0)
        return false;
        
    return true;
}
