/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-31
 *
 */


#ifndef __KL_MEMORY_H
#define __KL_MEMORY_H


namespace kl
{


unsigned long long getTotalPhysMemory();
unsigned long long getFreePhysMemory();




class membuf
{

public:

    membuf();
    ~membuf();
    
    bool alloc(size_t size);
    size_t getAllocSize() const;

    unsigned char* append(const unsigned char* buf, size_t len);

    unsigned char* getData();
    size_t getDataSize() const;
    
    unsigned char* takeOwnership();

private:

    unsigned char* m_buf;
    size_t m_alloc_size;
    size_t m_size;

};



};


#endif

