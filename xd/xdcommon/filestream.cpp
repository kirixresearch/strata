/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2007-01-31
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <xd/xd.h>
#include "filestream.h"



FileStream::FileStream()
{
    m_file = 0;
}

FileStream::~FileStream()
{
    if (m_file)
    {
        xf_close(m_file);
    }
}


bool FileStream::create(const std::wstring& filename)
{
    m_file = xf_open(filename, xfCreate, xfReadWrite, xfShareReadWrite);
    if (!m_file)
    {
        return false;
    }
    
    return true;
}

bool FileStream::open(const std::wstring& filename)
{
    m_file = xf_open(filename, xfOpen, xfReadWrite, xfShareReadWrite);
    if (!m_file)
    {
        // try read-only
        m_file = xf_open(filename, xfOpen, xfRead, xfShareReadWrite);
        if (!m_file)
            return false;
    }
    
    return true;
}


bool FileStream::read(void* buf,
          unsigned long read_size,
          unsigned long* read_count)
{
    unsigned long r = (unsigned long)xf_read(m_file, buf, 1, read_size);
    if (read_count)
    {
        *read_count = r;
    }
    
    if (read_size > 0 && r == 0)
        return false;

    return true;
}
                  
bool FileStream::write(const void* buf,
          unsigned long write_size,
          unsigned long* written_count)
{
    unsigned long w = (unsigned long)xf_write(m_file, buf, 1, write_size);
    if (written_count)
    {
        *written_count = w;
    }
    
    return true;
}


bool FileStream::seek(long long seek_pos, int whence)
{
    int xf_whence;

    switch (whence)
    {
        default:          return false; // invalid seek value
        case xd::seekSet: xf_whence = xfSeekSet; break;
        case xd::seekCur: xf_whence = xfSeekCur; break;
        case xd::seekEnd: xf_whence = xfSeekEnd; break;
    }

    return xf_seek(m_file, seek_pos, xf_whence);
}
