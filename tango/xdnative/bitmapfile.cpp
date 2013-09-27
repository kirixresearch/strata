/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-15
 *
 */


#include <xd/xd.h>
#include "bitmapfile.h"
#include "util.h"


// ------------------ FILE FORMAT: Tango Native Table --------------------
//
// -- file header --
// offset   00:   (uint32) signature 0xa2d022e4;
// offset   04:   (uint32) version (== 01)
// offset   08:   (uint32) data begin offset
// offset   12:   (uint64) set-bit count
// (data begin offset): bitmap



const int bitmap_file_page_size = 512;
const unsigned char bit_values[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };



BitmapFileScroller::BitmapFileScroller()
{
    m_buf = new unsigned char[bitmap_file_page_size];
    m_data_offset = 0;
    m_buf_offset = (unsigned long long)-1;
    m_file = NULL;
    m_bmp_file = NULL;
    m_buf_valid = false;

    m_locked = false;
    m_dirty = false;
    m_modify_set_bit_count = 0;
}


BitmapFileScroller::~BitmapFileScroller()
{
    if (m_buf)
    {
        delete[] m_buf;
    }
}

bool BitmapFileScroller::_flush()
{
    if (m_dirty)
    {
        xf_seek(m_file, m_buf_offset+m_data_offset, xfSeekSet);
        xf_write(m_file, m_buf, bitmap_file_page_size, 1);
        m_dirty = false;
    }

    if (m_locked)
    {
        xf_unlock(m_file,
                  m_buf_offset + m_data_offset,
                  bitmap_file_page_size);
        m_locked = false;
    }

    if (m_modify_set_bit_count != 0)
    {
        unsigned char buf[20];

        if (xf_trylock(m_file, 0, bitmap_file_page_size, 10000))
        {
            xf_seek(m_file, 0, xfSeekSet);
            xf_read(m_file, buf, 20, 1);

            long long count = bufToInt64(buf+12);
            count += m_modify_set_bit_count;
            int64ToBuf(buf+12, count);

            xf_seek(m_file, 0, xfSeekSet);
            xf_write(m_file, buf, 20, 1);

            m_modify_set_bit_count = 0;

            xf_unlock(m_file, 0, bitmap_file_page_size);
        }
    }

    return true;
}

bool BitmapFileScroller::_goBlock(unsigned long long block_number,
                                  bool lock,
                                  bool pad)
{
    // flush last block
    _flush();

    xf_off_t new_offset = block_number * bitmap_file_page_size;

    if (!xf_seek(m_file, new_offset+m_data_offset, xfSeekSet))
        return false;

    if (lock)
    {
        if (!xf_trylock(m_file,
                        new_offset+m_data_offset,
                        bitmap_file_page_size,
                        10000))
        {
            // could not get lock
            return false;
        }
    }

    if (xf_read(m_file, m_buf, bitmap_file_page_size, 1) != 1)
    {
        memset(m_buf, 0, bitmap_file_page_size);

        if (!pad)
        {
            m_buf_valid = false;
            if (lock)
            {
                xf_unlock(m_file, new_offset+m_data_offset, bitmap_file_page_size);
            }
            return false;
        }

        // we are beyond the end, so we must set
        // the intervening bytes to zero

        if (!xf_seek(m_file, 0, xfSeekEnd))
            return false;

        int cur_block = (xf_get_file_pos(m_file)-m_data_offset) / bitmap_file_page_size;
        cur_block = block_number - cur_block + 1;
        while (cur_block)
        {
            xf_write(m_file, m_buf, bitmap_file_page_size, 1);
            --cur_block;
        }

        // now reseek to position the caller wanted, and try
        // to read the buffer in again
        if (!xf_seek(m_file, new_offset+m_data_offset, xfSeekSet))
            return false;

        if (xf_read(m_file, m_buf, bitmap_file_page_size, 1) != 1)
            return false;

    }

    m_buf_offset = new_offset;
    m_buf_valid = true;

    if (lock)
    {
        m_locked = true;
    }

    return true;
}


bool BitmapFileScroller::getState(unsigned long long offset)
{
    unsigned int byte_offset = offset/8;
    unsigned int bit_offset = offset % 8;

    if (m_buf_offset == (unsigned long long)-1 ||
        byte_offset < m_buf_offset ||
        byte_offset >= m_buf_offset+bitmap_file_page_size)
    {
        if (!_goBlock(byte_offset/bitmap_file_page_size, false, false))
            return false;
        _flush();
    }

    return (m_buf[byte_offset-m_buf_offset] & bit_values[bit_offset]) ? true : false;
}

bool BitmapFileScroller::findPrev(unsigned long long* offset,
                                  bool state)
{
    unsigned int byte_offset = (*offset)/8;
    unsigned int bit_offset = (*offset) % 8;
    unsigned int byte_check = state ?  0x00000000 : 0xffffffff;

    while (1)
    {
        if (m_buf_offset == (unsigned long long)-1 ||
            byte_offset < m_buf_offset ||
            byte_offset >= m_buf_offset+bitmap_file_page_size)
        {
            if (!_goBlock(byte_offset/bitmap_file_page_size, false, false))
                return false;
            _flush();
        }
        
        // looking for a bit that is set (backward direction)
        while (m_buf[byte_offset-m_buf_offset] == byte_check)
        {
            if (byte_offset == m_buf_offset)
            {
                // go to the previous block
                break;
            }

            byte_offset--;
            bit_offset = 7;
        }

        while (1)
        {
            if ((m_buf[byte_offset-m_buf_offset] & bit_values[bit_offset] ? true : false) == state)
            {
                *offset = (byte_offset*8) + bit_offset;
                return true;
            }

            if (bit_offset == 0)
            {
                if (byte_offset == 0)
                {
                    *offset = 0;
                    // bof condition
                    return false;
                }

                byte_offset--;
                bit_offset = 7;
                if (byte_offset < m_buf_offset)
                {
                    // go to the previous block
                    break;
                }
            }
             else
            {
                bit_offset--;
            }
        }
    }

    return false;
}

bool BitmapFileScroller::findNext(unsigned long long* offset,
                                  bool state)
{
    unsigned int byte_offset = (*offset)/8;
    unsigned int bit_offset = (*offset) % 8;
    unsigned int byte_check = state ?  0x00000000 : 0xffffffff;

    while (1)
    {
        if (m_buf_offset == (unsigned long long)-1 ||
            byte_offset < m_buf_offset ||
            byte_offset >= m_buf_offset+bitmap_file_page_size)
        {
            if (!_goBlock(byte_offset/bitmap_file_page_size, false, false))
                return false;
            _flush();
        }
        
        // looking for a bit that is set (forward direction)
        while (m_buf[byte_offset-m_buf_offset] == byte_check)
        {
            byte_offset++;
            bit_offset = 0;
            if (byte_offset >= m_buf_offset+bitmap_file_page_size)
            {
                break;
            }
        }

        while (1)
        {
            if (byte_offset >= m_buf_offset+bitmap_file_page_size)
            {
                // go to the next block
                break;
            }

            if ((m_buf[byte_offset-m_buf_offset] & bit_values[bit_offset] ? true : false) == state)
            {
                *offset = (byte_offset*8) + bit_offset;
                return true;
            }

            bit_offset++;
            if (bit_offset == 8)
            {
                byte_offset++;
                bit_offset = 0;
            }
        }
    }

    return false;
}


void BitmapFileScroller::startModify()
{
    m_modify_set_bit_count = 0;
}

void BitmapFileScroller::endModify()
{
    _flush();
}


void BitmapFileScroller::setState(unsigned long long offset,
                                  bool state)
{
    unsigned byte_offset = offset/8;
    unsigned bit_offset = offset % 8;

    if (m_buf_offset == (unsigned long long)-1 ||
        byte_offset < m_buf_offset ||
        byte_offset >= m_buf_offset+bitmap_file_page_size)
    {
        _goBlock(byte_offset/bitmap_file_page_size, true, true);
    }

    bool cur_state = (m_buf[byte_offset-m_buf_offset] & bit_values[bit_offset]) ? true : false;
    if (cur_state == state)
        return;

    if (state)
    {
        m_modify_set_bit_count++;
        m_buf[byte_offset-m_buf_offset] |= bit_values[bit_offset];
    }
     else
    {
        m_modify_set_bit_count--;
        m_buf[byte_offset-m_buf_offset] &= (~bit_values[bit_offset]);
    }
    m_dirty = true;
}





BitmapFile::BitmapFile()
{
    m_file = NULL;
    m_data_offset = 0;
}


BitmapFile::~BitmapFile()
{
    if (m_file)
    {
        xf_close(m_file);
    }
}


bool BitmapFile::open(const std::wstring& filename)
{
    bool create = false;
    
    if (!xf_get_file_exist(filename))
        create = true;
    
    m_file = xf_open(filename,
                     xfOpenCreateIfNotExist,
                     xfReadWrite,
                     xfShareReadWrite);
    
    if (m_file == NULL)
    {
        return false;
    }

    if (!create)
    {                 
        unsigned char buf[20];
        if (xf_read(m_file, buf, 20, 1) != 1)
        {
            return false;
        }

        unsigned int signature, version;
        signature = buf2int(buf);
        version = buf2int(buf+4);
        m_data_offset = buf2int(buf+8);
    
        if (signature != 0xa2d022e4 || version != 1)
        {
            return false;
        }
    }
     else
    {
        unsigned char buf[bitmap_file_page_size];

        m_data_offset = bitmap_file_page_size;

        memset(buf, 0, bitmap_file_page_size);
        int2buf(buf, 0xa2d022e4);     // signature
        int2buf(buf+4, 1);            // version
        int2buf(buf+8, m_data_offset); // data begin
        int2buf(buf+12, 0);            // set-bit count (lower 32 bits)
        int2buf(buf+16, 0);            // set-bit count (upper 32 bits)

        if (xf_write(m_file, buf, bitmap_file_page_size, 1) != 1)
        {
            return false;
        }
    }

    return true;
}

bool BitmapFile::close()
{
    if (m_file == NULL)
    {
        return false;
    }

    xf_close(m_file);
    m_file = NULL;

    return true;
}

bool BitmapFile::isOpen()
{
    return (m_file != NULL) ? true : false;
}


unsigned long long BitmapFile::getSetBitCount()
{
    if (!xf_trylock(m_file, 0, bitmap_file_page_size, 10000))
        return 0;

    unsigned char buf[8];
    unsigned long long result;

    memset(buf, 0, 8);
    xf_seek(m_file, 12, xfSeekSet);
    xf_read(m_file, buf, 8, 1);

    result = bufToInt64(buf);

    xf_unlock(m_file, 0, bitmap_file_page_size);

    return result;
}


BitmapFileScroller* BitmapFile::createScroller()
{
    BitmapFileScroller* bfs = new BitmapFileScroller;
    bfs->m_bmp_file = this;
    bfs->m_file = m_file;
    bfs->m_data_offset = m_data_offset;
    return bfs;
}





