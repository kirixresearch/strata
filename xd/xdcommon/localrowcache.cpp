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
#include "localrowcache.h"
#include "util.h"


// all values are machine byte order (depends on the platform)
// this is ok because these files are just temporary files


// 8-bit column flags
//    * 0x01 = column size = 8 bits
//    * 0x02 = column size = 16 bits
//    * 0x04 = column size = 24 bits
//    * 0x08 = column size = 32 bits
//    * 0x10 = column is null


// -- row format --

// 32-bit size of row buffer
// 32-bit row ordinal (first row = row 0)

// column 0 flags (8 bits)
// column 0 data size bytes (8, 16, 24, or 32 bits)
// column 0 data

// column 1 flags (8 bits)
// column 1 data size bytes (8, 16, 24, or 32 bits)
// column 1 data

// 8-bit 0x00000000 terminator
// 32-bit size of row buffer


// -- LocalRow implementation --

LocalRow::LocalRow()
{
    m_buf_alloc_size = 0;
    m_buf = NULL;
}

LocalRow::~LocalRow()
{
    delete[] m_buf;
}

// helper functions
inline unsigned char* getColumnDataPtr(unsigned char* col_ptr)
{
    if (*col_ptr & 0x01)
    {
        return (col_ptr + 2);
    }
     else if (*col_ptr & 0x02)
    {
        return (col_ptr + 3);
    }
     else if (*col_ptr & 0x04)
    {
        return (col_ptr + 4);
    }
     else if (*col_ptr & 0x08)
    {
        return (col_ptr + 5);
    }
     else
    {
        return (col_ptr + 1);
    }
}

inline int getColumnDataLength(unsigned char* col_ptr)
{    
    if (*col_ptr & 0x10)
    {
        // column null;
        return 0;
    }
        
    if (*col_ptr & 0x01)
    {
        // 8-bit column data size
        return *(col_ptr+1);
    }
     else if (*col_ptr & 0x02)
    {
        // 16-bit column data size
        return *((unsigned short*)(col_ptr+1));
    }
     else if (*col_ptr & 0x04)
    {
        // 24-bit column data size
        return *((unsigned int*)(col_ptr+1)) & 0x00ffffff;
    }
     else if (*col_ptr & 0x08)
    {
        // 32-bit column data size
        return *((unsigned int*)(col_ptr+1));
    }
    
    return 0;
}

unsigned char* LocalRow::getColumnData(int col_idx, unsigned int* size, bool* is_null)
{
    if (col_idx < 0 ||(size_t)col_idx >= m_col_ptrs.size())
        return NULL;
    
    unsigned char* ptr = m_col_ptrs[col_idx];
    
    bool null = (*ptr & 0x10) ? true : false;
    
    if (is_null)
    {
        *is_null = null;
    }
    
    if (null)
    {
        if (size)
            *size = 0;
        return NULL;
    }
    
    if (size)
    {
        *size = getColumnDataLength(ptr);
    }
    
    return getColumnDataPtr(ptr);
}

size_t LocalRow::getColumnCount() const
{
    return m_col_ptrs.size();
}


void LocalRow::reserveBuffer(unsigned int new_size)
{
    if (new_size <= m_buf_alloc_size)
        return;
    
    unsigned char* new_data = new unsigned char[new_size];
    memset(new_data, 0, new_size);
    if (m_buf)
        memcpy(new_data, m_buf, m_buf_alloc_size);
    
    delete[] m_buf;
    m_buf = new_data;
    m_buf_alloc_size = new_size;
}





// -- LocalRowCache implementation --


LocalRowCache::LocalRowCache()
{
    m_f = 0;
    m_row_count = 0;
    m_curpos = 0;
    m_row_count = 0;
    
    m_append_allocsize = 0;
    m_append_data = NULL;
    m_append_rownum = 0;
    
    m_read_reclen = 0;
    m_read_currow = (xd::rowpos_t)-1;
}

LocalRowCache::~LocalRowCache()
{
    delete[] m_append_data;

    if (m_f)
    {
        xf_close(m_f);
        xf_remove(m_filename);
    }
}

bool LocalRowCache::init()
{
    m_filename = xf_get_temp_path();
    m_filename += getUniqueString();
    m_filename += L".lrc";
    
    m_f = xf_open(m_filename, xfCreate, xfReadWrite, xfShareNone);
    if (!m_f)
        return false;

    return true;
}

bool LocalRowCache::isOk() const
{
    return m_f ? true : false;
}


xd::rowpos_t LocalRowCache::getRowCount()
{
    return m_row_count;
}


void LocalRowCache::createRow()
{
    // at the very beginning, we will write the row length;
    // we need to reserve 8 bytes for this
    reserveAppendBuffer(8);
    m_append_cursize = 8;
}

void LocalRowCache::appendNullColumn()
{
    int needed_space = m_append_cursize + 1;
    reserveAppendBuffer(needed_space);
    
    *(m_append_data + m_append_cursize) = 0x10;
    m_append_cursize++;
}

void LocalRowCache::copyDataToBuffer(unsigned char* dest,
                                     const unsigned char* src,
                                     unsigned int size)
{
    unsigned char* p = dest;
    
    *p = 0;
    if (size < 256)
    {
        *p |= (0x01);
        *(p+1) = (unsigned char)size;
        p += 2;
    }
    else if (size >= 256 && size < 65536)
    {
        *p |= (0x02);
        *(p+1) = size & 0xff;
        *(p+2) = (size >> 8) & 0xff;
        p += 3;
    }
    else if (size >= 65536 && size < 16777216)
    {
        *p |= (0x03);
        *(p+1) = size & 0xff;
        *(p+2) = (size >> 8) & 0xff;
        *(p+3) = (size >> 16) & 0xff;
        p += 4;
    }
    else if (size >= 16777216)
    {
        *p |= (0x04);
        *(p+1) = size & 0xff;
        *(p+2) = (size >> 8) & 0xff;
        *(p+3) = (size >> 16) & 0xff;
        *(p+4) = (size >> 24) & 0xff;
        p += 5;
    }
    
    memcpy(p, src, size);
}
    
void LocalRowCache::appendColumnData(unsigned char* data, unsigned int size)
{
    int needed_space = m_append_cursize + size + 5;
    reserveAppendBuffer(needed_space);

    unsigned char* p = m_append_data + m_append_cursize;
    
    *p = 0;
    if (size < 256)
    {
        *p |= (0x01);
        *(p+1) = (unsigned char)size;
        p += 2;
    }
    else if (size >= 256 && size < 65536)
    {
        *p |= (0x02);
        *(p+1) = size & 0xff;
        *(p+2) = (size >> 8) & 0xff;
        p += 3;
    }
    else if (size >= 65536 && size < 16777216)
    {
        *p |= (0x03);
        *(p+1) = size & 0xff;
        *(p+2) = (size >> 8) & 0xff;
        *(p+3) = (size >> 16) & 0xff;
        p += 4;
    }
    else if (size >= 16777216)
    {
        *p |= (0x04);
        *(p+1) = size & 0xff;
        *(p+2) = (size >> 8) & 0xff;
        *(p+3) = (size >> 16) & 0xff;
        *(p+4) = (size >> 24) & 0xff;
        p += 5;
    }
    
    memcpy(p, data, size);
    p += size;
    
    m_append_cursize = (p - m_append_data);
}

void LocalRowCache::finishRow()
{
    // append terminator and total row length
    reserveAppendBuffer(m_append_cursize + 5);
    unsigned char* p = m_append_data + m_append_cursize;
    
    m_append_cursize += 5;
    
    // write terminator
    *p = 0;
    
    // write row data length at the beginning and end
    int2buf(m_append_data, (unsigned int)m_append_cursize);
    int2buf(m_append_data+4, (unsigned int)m_append_rownum);
    int2buf(p+1, (unsigned int)m_append_cursize);
    
    m_append_rownum++;
    
    // write out row data
    xf_seek(m_f, 0, xfSeekEnd);
    xf_write(m_f, m_append_data, m_append_cursize, 1);
    
    m_row_count++;
}

void LocalRowCache::reserveAppendBuffer(unsigned int new_size)
{
    if (new_size <= m_append_allocsize)
        return;
    
    unsigned char* new_data = new unsigned char[new_size];
    memset(new_data, 0, new_size);
    
    if (!m_append_data)
    {
        m_append_data = new_data;
        m_append_allocsize = new_size;
        return;
    }
   
    memcpy(new_data, m_append_data, m_append_allocsize);
    
    // reposition the m_append_ptr
    if (m_append_colptr)
        m_append_colptr = new_data + (m_append_colptr - m_append_data);
    
    delete[] m_append_data;
    m_append_data = new_data;
    m_append_allocsize = new_size;
}


void LocalRowCache::goFirst()
{
    m_eof = false;
    m_offset = 0;
    
    xf_seek(m_f, 0, xfSeekSet);
    
    unsigned char buf[8];
    if (1 != xf_read(m_f, buf, 8, 1))
    {
        m_eof = true;
        return;
    }
    
    m_read_reclen = buf2int(buf);
    m_read_currow = buf2int(buf+4);
}

void LocalRowCache::goRow(xd::rowpos_t row)
{
    if (m_read_currow == (xd::rowpos_t)-1)
    {
        goFirst();
    }

    if (row == 0)
    {
        goFirst();
        return;
    }
    
    while (row < m_read_currow)
    {
        goPrev();
        if (eof())
            return;
    }
    
    while (row > m_read_currow)
    {
        goNext();
        if (eof())
            return;
    }
}

void LocalRowCache::goPrev()
{
    if (m_read_currow == (xd::rowpos_t)-1)
    {
        goFirst();
    }
    
    if (m_offset <= 4)
    {
        return;
    }
    
    m_eof = false;
    
    xf_seek(m_f, m_offset-4, xfSeekSet);

    unsigned char buf[8];
    if (1 != xf_read(m_f, buf, 4, 1))
    {
        m_eof = true;
        return;
    }
    
    int last_rec_len = buf2int(buf);
    m_offset -= last_rec_len;
    
    xf_seek(m_f, m_offset, xfSeekSet);
    
    

    if (1 != xf_read(m_f, buf, 8, 1))
    {
        m_eof = true;
        return;
    }
    
    m_read_reclen = buf2int(buf);
    m_read_currow = buf2int(buf+4);
}


void LocalRowCache::goNext()
{
    if (m_read_currow == (xd::rowpos_t)-1)
    {
        goFirst();
    }
    
    m_offset += m_read_reclen;
    
    xf_seek(m_f, m_offset, xfSeekSet);

    unsigned char buf[8];
    if (1 != xf_read(m_f, buf, 8, 1))
    {
        m_eof = true;
        return;
    }
    
    m_read_reclen = buf2int(buf);
    m_read_currow = buf2int(buf+4);
}

bool LocalRowCache::eof()
{
    return m_eof;
}

bool LocalRowCache::getRow(LocalRow& row)
{
    if (m_eof)
        return false;
        
    xf_seek(m_f, m_offset, xfSeekSet);

    row.reserveBuffer(m_read_reclen);
    if (1 != xf_read(m_f, row.m_buf, m_read_reclen, 1))
    {
        *row.m_buf = 0;
        return false;
    }
    

    // process the row buffer
    
    row.m_col_ptrs.clear();
    
    int row_buffer_size = buf2int(row.m_buf);
    int row_ordinal = buf2int(row.m_buf + 4);
    
    bool col_null = false;
    
    unsigned char* p = row.m_buf + 8;
    
    int col = 0;
    while (1)
    {
        // check for terminator
        if (!*p)
            break;

        row.m_col_ptrs.push_back(p);
        p = getColumnDataPtr(p) + getColumnDataLength(p);
        
        // a value was updated in memory
        unsigned char* col_buf = getUpdateValue(col);
        if (col_buf)
            row.m_col_ptrs[row.m_col_ptrs.size()-1] = col_buf;
        
        col++;
    }

    return true;
}



void LocalRowCache::updateValue(xd::rowpos_t row,
                                int col,
                                unsigned char* data,
                                unsigned int size)
{
    LocalRowUpdates* update_row = NULL;
    
    std::map<xd::rowpos_t, LocalRowUpdates*>::iterator it;
    it = m_updates.find(row);
    if (it == m_updates.end())
    {
        update_row = new LocalRowUpdates;
        m_updates[row] = update_row;
    }
     else
    {
        update_row = it->second;
    }
    
    // get rid of whatever was there before
    unsigned char* update_data = update_row->m_data[col];
    delete[] update_data;
    
    // create and assign new value
    update_data = new unsigned char[size+5];
    
    copyDataToBuffer(update_data, data, size);
    
    update_row->m_data[col] = update_data;
}

void LocalRowCache::clearUpdateValues()
{
    std::map<xd::rowpos_t, LocalRowUpdates*>::iterator it, it_end;
    it_end = m_updates.end();
    for (it = m_updates.begin(); it != it_end; ++it)
        delete it->second;
    
    m_updates.clear();
}

unsigned char* LocalRowCache::getUpdateValue(int col)
{
    if (m_updates.size() == 0)
        return NULL;
        
    if (m_read_currow == (xd::rowpos_t)-1)
        return NULL;
    
    std::map<xd::rowpos_t, LocalRowUpdates*>::iterator it;
    it = m_updates.find(m_read_currow);
    if (it == m_updates.end())
        return NULL;
        
    std::map<int, unsigned char*>::iterator it2;
    it2 = it->second->m_data.find(col);
    if (it2 == it->second->m_data.end())
        return NULL;
        
    return it2->second;
}

