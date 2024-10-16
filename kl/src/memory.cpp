/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-31
 *
 */


#ifdef WIN32
#include <windows.h>
#endif


#include <cstring>
#include <cstdlib>
#include <cstddef>
#include "kl/memory.h"


namespace kl
{


// memory api

#ifdef WIN32

    unsigned long long getTotalPhysMemory()
    {
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memStatus);
        return memStatus.ullTotalPhys;
    }

    unsigned long long getFreePhysMemory()
    {
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memStatus);
        return memStatus.ullAvailPhys;
    }

#else

    unsigned long long getTotalPhysMemory()
    {
        return 1073741824;
    }

    unsigned long long getFreePhysMemory()
    {
        return 536870912;
    }
    
#endif




// membuf class implementation

membuf::membuf()
{
    m_size = 0;
    m_alloc_size = 0;
    m_buf = NULL;
}

membuf::~membuf()
{
    if (m_buf)
    {
        free(m_buf);
        m_buf = NULL;
    }
}
  
membuf::membuf(const membuf& c)
{
    if (c.m_buf)
    {
        m_size = c.m_size;
        m_alloc_size = 0;
        m_buf = NULL;

        alloc(c.m_alloc_size);

        if (m_buf)
        {
            memcpy(m_buf, c.m_buf, c.m_alloc_size);
        }
    }
     else
    {
        m_size = 0;
        m_alloc_size = 0;
        m_buf = NULL;
    }
}

    
bool membuf::alloc(size_t size)
{
    if (size <= m_alloc_size)
        return true;

    void* new_buf = realloc(m_buf, size);
    if (!new_buf)
    {
        // realloc() failed -- don't change m_buf and return failure
        return false;
    }

    m_buf = (unsigned char*)new_buf;
    m_alloc_size = size;
    return true;
}


void membuf::setDataSize(size_t s)
{
    alloc(s);
    m_size = s;
}

unsigned char* membuf::append(const unsigned char* buf, size_t len)
{
    if (len == 0)
    {
        if (!m_buf)
            alloc(10);
        return m_buf + m_size;
    }
    
    size_t required_size = m_size + len;
    if (required_size > m_alloc_size)
    {
        if (!alloc(required_size))
            return NULL;
    }

    unsigned char* ptr = m_buf + m_size;

    if (buf)
        memcpy(ptr, buf, len);
         else
        memset(ptr, 0, len);

    m_size += len;

    return ptr;
}


void membuf::popData(size_t len)
{
    // remove len bytes from beginning, shift remaining data

    if (len >= m_size)
    {
        // set data length to zero and return
        m_size = 0;
        return;
    }

    size_t remaining = m_size - len;
    memmove(m_buf, m_buf + len, remaining);
    m_size = remaining;
}


unsigned char* membuf::takeOwnership()
{
    unsigned char* buf = m_buf;

    m_size = 0;
    m_alloc_size = 0;
    m_buf = NULL;

    return buf;
}

    
};
