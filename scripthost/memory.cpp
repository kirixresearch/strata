/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2006-12-24
 *
 */


#include "scripthost.h"
#include "memory.h"
#include <kl/string.h>



namespace scripthost
{


// (CLASS) MemoryBuffer
// Category: IO
// Description: A class that encapsulates read/write operations to the memory.
// Remarks: The MemoryBuffer class allows an arbitrary amount of data to be
//     stored in a memory buffer. MemoryBuffer automatically manages the
//     deallocation of the memory as soon as the object is destroyed.

MemoryBuffer::MemoryBuffer()
{
    m_buf = NULL;
    m_size = 0;
}

MemoryBuffer::~MemoryBuffer()
{
    if (m_buf)
    {
        ::free(m_buf);
    }
}


void MemoryBuffer::alloc(size_t size)
{
    m_buf = (unsigned char*)malloc(size);
    memset(m_buf, 0, (size_t)size);
    m_size = size;
}


unsigned char* MemoryBuffer::getBuffer()
{    
    return m_buf;
}

size_t MemoryBuffer::getBufferSize()
{
    return m_size;
}


void MemoryBuffer::setMember(const std::wstring& name, kscript::Value* value)
{
    if (name.length() == 0)
        return;
        
    if (iswdigit(name[0]))
    {
        int idx = kl::wtoi(name);
        if (idx < 0 || idx >= (int)m_size)
            return;
            
        m_buf[idx] = (unsigned int)value->getInteger();
        return;
    }
    
    ValueObject::setMember(name, value);
}

kscript::Value* MemoryBuffer::getMember(const std::wstring& name)
{
    if (name.length() == 0)
    {
        m_val.setNull();
        return &m_val;
    }
        
    if (iswdigit(name[0]))
    {
        int idx = kl::wtoi(name);
        if (idx < 0 || idx >= (int)m_size)
        {
            m_val.setNull();
            return &m_val;
        }
            
        m_val.setInteger(m_buf[idx]);
        return &m_val;
    }
    
    return ValueObject::getMember(name);
}




// (CONSTRUCTOR) MemoryBuffer.constructor
// Description: Constructor for MemoryBuffer class.
// 
// Syntax: MemoryBuffer(size : Integer)
//
// Remarks: Constructs a MemoryBuffer object and sets the initial size in bytes.
// Param(size): The new size of the buffer.


void MemoryBuffer::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    int size = 0;
    
    if (env->getParamCount() >= 1)
        size = env->getParam(0)->getInteger();
        
    if (size > 0)
    {
        m_buf = (unsigned char*)malloc((size_t)size);
        memset(m_buf, 0, (size_t)size);
        m_size = (size_t)size;
    }
}


// (METHOD) MemoryBuffer.setSize
// Description: Sets the size of the buffer.
//
// Syntax: function MemoryBuffer.setSize(size : Integer) : Boolean
//
// Remarks: Ensures that the buffer has at least |size| bytes available.
//
// Param(size): The new size of the buffer.
// Returns: True if the buffer allocation succeeded, false upon failure

void MemoryBuffer::setSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    // if no parameters are specified, return failure
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    // get size from parameter
    int size = env->getParam(0)->getInteger();
    
    // if specified size is less than zero, return failure
    if (size < 0)
    {
        retval->setBoolean(false);
        return;
    }
    
    retval->setBoolean(setSizeInternal(size));
}



// (METHOD) MemoryBuffer.getSize
// Description: Retrieves the size of the buffer.
//
// Syntax: function MemoryBuffer.getSize(size : Integer) : Integer
//
// Remarks: Returns the present size of the memory buffer in bytes.
//
// Returns: Returns the present size of the memory buffer

void MemoryBuffer::getSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger((int)m_size);
}




// (METHOD) MemoryBuffer.clear
//
// Description: Clears the memory buffer
//
// Syntax: function MemoryBuffer.clear() : Undefined
//
// Remarks: Calling clear() erases all the data in the buffer,
//     setting all bytes to zero.  The buffer size is not affected
//
// Returns: Undefined

void MemoryBuffer::clear(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_buf)
    {
        memset(m_buf, 0, m_size);
    }
    
    retval->setUndefined();
}

// (METHOD) MemoryBuffer.free
//
// Description: Frees the memory buffer
//
// Syntax: function MemoryBuffer.free() : Undefined
//
// Remarks: Calling free releases all memory associated with the
//     memory buffer and sets the new size of the buffer to zero
//
// Returns: Undefined

void MemoryBuffer::free(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_buf)
    {
        ::free(m_buf);
    }
    
    m_size = 0;
    
    retval->setUndefined();
}


// (METHOD) MemoryBuffer.copy
//
// Description: Copies either all or a portion of the buffer
//     to a new MemoryBuffer object.
//
// Syntax: function MemoryBuffer.copy() : MemoryBuffer
// Syntax: function MemoryBuffer.copy(offset : Integer,
//                                    length : Integer) : MemoryBuffer
//
// Remarks: Copies a portion of the buffer specified by the |offset|
//     and |length| parameters.  If the |length| parameter is
//     omitted, the remainder of the buffer is copied.  If no parameters
//     are specified, the entire object is copied.
//
// Param(offset): Offset at which the copy should start.
// Param(length): Number of bytes to copy.
// Returns: A new MemoryBuffer object containing the data requested

void MemoryBuffer::copy(kscript::ExprEnv* env, kscript::Value* retval)
{
    int offset;
    int length;
    
    if (env->getParamCount() < 1)
    {
        offset = 0;
        length = (int)m_size;
    }
     else
    {
        if (env->getParamCount() >= 1)
        {
            offset = env->getParam(0)->getInteger();
            if (offset < 0)
            {
                retval->setNull();
                return;
            }
        }
        
        if (env->getParamCount() >= 2)
        {
            length = env->getParam(1)->getInteger();
            if (offset < 0)
            {
                retval->setNull();
                return;
            }
        }
         else
        {
            length = 2147483647;
        }
        
        if ((size_t)offset + (size_t)length > m_size)
            length = (int)(m_size - (size_t)offset);
    }
    
    
    
    MemoryBuffer* ret = MemoryBuffer::createObject(env);
    
    if (length == 0)
    {
        retval->setObject(ret);
        return;
    }
    
    unsigned char* buf = (unsigned char*)malloc(length);
    if (!buf)
    {
        delete ret;
        retval->setNull();
        return;
    }
    
    ret->m_buf = buf;
    ret->m_size = (size_t)length;
    
    memcpy(ret->m_buf, m_buf + offset, (size_t)length);
    
    retval->setObject(ret);
}



// (METHOD) MemoryBuffer.toAsciiString
//
// Description: Copies either all or a portion of the buffer
//     to a new MemoryBuffer object.
//
// Syntax: function MemoryBuffer.toAsciiString(offset : Integer,
//                                             length : Integer) : String
//
// Remarks: Copies a portion of the buffer specified by the |offset|
//     and |length| parameters into an ascii string.  If the |length| parameter
//     is omitted, the remainder of the buffer is copied.  If no parameters
//     are specified, the entire object is copied.
//
// Param(offset): Offset at which the copy should start.
// Param(length): Number of bytes to copy.
// Returns: A new MemoryBuffer object containing the data requested


void MemoryBuffer::toAsciiString(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();

    int offset = 0;
    int length = -1;

    if (env->getParamCount() > 0)
        offset = env->getParam(0)->getInteger();
    if (env->getParamCount() > 1)
        length = env->getParam(1)->getInteger();

    if (offset < 0)
        return;

    if (length == -1)
        length = (int)(m_size - (size_t)offset);

    std::string str(((const char*)m_buf)+offset, length);

    retval->setString(str);
}


bool MemoryBuffer::setSizeInternal(int size)
{
    if (size < 0)
        return false;
        
    // if we already have the required amount, do
    // nothing and return success
    if ((size_t)size == m_size)
        return true;
    
    size_t old_size = m_size;
    
    // reallocate buffer
    m_buf = (unsigned char*)realloc(m_buf, (size_t)size);
    m_size = (size_t)size;
    
    // if the reallocation failed, return failure
    if (!m_buf)
    {
        m_size = 0;
        return false;
    }
    
    // realloc preserves data, and new size is less than old size
    // so we're already done
    if ((size_t)size <= m_size)
        return true;
    
    // fill new bytes with zero
    memset(m_buf + old_size, 0, m_size - old_size);
    
    return true;
}



}; // namespace scripthost
