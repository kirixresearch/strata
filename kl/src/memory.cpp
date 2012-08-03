/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-31
 *
 */


#ifdef WIN32
#include <windows.h>
#endif


#include "kl/memory.h"


namespace kl
{


// -- memory api --

#ifdef WIN32

    unsigned long getTotalPhysMemory()
    {
        MEMORYSTATUS memStatus;
        memStatus.dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus(&memStatus);
        return memStatus.dwTotalPhys;
    }

    unsigned long getFreePhysMemory()
    {
        MEMORYSTATUS memStatus;
        memStatus.dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus(&memStatus);
        return memStatus.dwAvailPhys;
    }

#else

    unsigned long getTotalPhysMemory()
    {
        return 268435456;
    }

    unsigned long getFreePhysMemory()
    {
        return 268435456;
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
    
bool membuf::alloc(size_t size)
{
    m_buf = (unsigned char*)realloc(m_buf, size);
    if (!m_buf)
    {
        m_alloc_size = 0;
        return false;
    }

    m_alloc_size = size;
    return true;
}

size_t membuf::getAllocSize() const
{
    return m_alloc_size;
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

unsigned char* membuf::getData()
{
    return m_buf;
}

size_t membuf::getDataSize() const
{
    return m_size;
}


    
};
