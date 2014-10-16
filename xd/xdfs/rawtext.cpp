/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2007-01-22
 *
 */


#include <cmath>
#include <cstring>
#include "rawtext.h"



BufferedFile::BufferedFile()
{
    m_file = NULL;
    m_filesize = 0;
    m_buf_offset = 0;
    m_buf_size = 0;
    m_buf = NULL;
    m_byte_order = boDefault;
    m_skip_bytes = 0;
}

BufferedFile::~BufferedFile()
{
    if (isOpen())
        closeFile();
}

bool BufferedFile::isUnicode()
{
    return (m_byte_order != boDefault) ? true : false;
}

bool BufferedFile::openFile(const std::wstring& filename, int open_flags, int share_flags)
{
    // try to open the file
    m_file = xf_open(filename, xfOpen, open_flags, share_flags);
    if (m_file == NULL)
        return false;
        
    m_filename = filename;
    m_filesize = xf_get_file_size(filename);
    
    // create the buffer
    m_buf = new unsigned char[bfBufSize];
    
    // do an initial read of the file into our buffer
    memset(m_buf, 0, bfBufSize);
    m_buf_size = xf_read(m_file, m_buf, 1, bfBufSize);
    m_buf_offset = 0;
    
    // look for a byte order mark
    if (getByte(0) == 0xff && getByte(1) == 0xfe)
    {
        m_byte_order = boUCS2LE;
        m_skip_bytes += 2;
    }
    
    return true;
}

void BufferedFile::closeFile()
{
    if (!isOpen())
        return;
    
    if (m_buf)
    {
        delete[] m_buf;
        m_buf = NULL;
        m_buf_size = 0;
        m_buf_offset = 0;
        m_skip_bytes = 0;
    }
            
    if (m_file)
    {
        xf_close(m_file);
        m_file = NULL;
    }
}

bool BufferedFile::rewind()
{
    xf_seek(m_file, 0, xfSeekSet);
    memset(m_buf, 0, bfBufSize);
    m_buf_size = xf_read(m_file, m_buf, 1, bfBufSize);
    m_buf_offset = 0;
    return true;
}


wchar_t BufferedFile::getChar(xf_off_t offset, bool* eof)
{
    if (m_byte_order == boDefault)
    {
        return (wchar_t)getByte(offset+m_skip_bytes, eof);
    }
     else if (m_byte_order == boUCS2LE)
    {
        offset *= 2;
        offset += m_skip_bytes;
        unsigned char c1 = getByte(offset, eof);
        wchar_t res = getByte(offset+1, eof);
        res <<= 8;
        res |= c1;
        return res;
    }
    
    return 0;
}


bool BufferedFile::appendData(wchar_t* buf, size_t char_count)
{
    // go to the end of the file
    xf_seek(m_file, 0, xfSeekEnd);
    
    if (m_byte_order == boDefault)
    {
        unsigned char* dbuf = (unsigned char*)buf;
        for (size_t i = 0; i < char_count; ++i)
            dbuf[i] = (unsigned char)buf[i];
        
        xf_write(m_file, dbuf, 1, char_count);
        m_filesize = xf_get_file_size(m_filename);
    }
     else if (m_byte_order == boUCS2LE)
    {
        unsigned char* dbuf = (unsigned char*)buf;
        
        #ifdef WIN32
            // this assumes that wchar_t is 2 bytes, little endian
            // (which is a good assumption on windows ;-)
            xf_write(m_file, dbuf, 1, char_count * 2);
            
            m_filesize = xf_get_file_size(m_filename);
        #else
        
            wchar_t ch;
            for (size_t i = 0; i < char_count; ++i)
            {
                ch = buf[i];
                dbuf[i*2] = (ch & 0xff);
                dbuf[(i+2)+1] = ((ch >> 8) & 0xff);
            }
        
            // this assumes that wchar_t is 2 bytes, little endian
            // (which is a good assumption on windows ;-)
            xf_write(m_file, dbuf, 1, char_count * 2);
            
            m_filesize = xf_get_file_size(m_filename);
    
        #endif
    }
    
    return true;
}


bool BufferedFile::isPositionEof(xf_off_t offset)
{
    if (m_byte_order == boDefault)
    {
        return (offset >= m_filesize) ? true : false;
    }
     else if (m_byte_order == boUCS2LE)
    {
        offset *= 2;
        offset += m_skip_bytes;
        offset++;
        return (offset >= m_filesize) ? true : false;
    }
    
    return false;
}



xf_off_t BufferedFile::getFileSize()
{
    return m_filesize;
}
    
// getByte is used internally to fetch a byte from the file
unsigned char BufferedFile::getByte(xf_off_t offset, bool* eof)
{
    // the offset is out of bounds
    if (offset < 0 || offset > m_filesize)
    {
        if (eof)
            *eof = true;
        return 0;
    }
    
    // the char is inside our buffer
    if (offset >= m_buf_offset &&
        offset < m_buf_offset+m_buf_size)
    {
        return m_buf[offset-m_buf_offset];
    }
    
    
    // the char is outside our buffer, so skip to
    // the appropriate location, read the data into
    // the buffer and return the appropriate char
    xf_off_t buf_offset_mod = offset % (bfBufSize-3000);
    xf_off_t new_buf_offset = offset-buf_offset_mod;
    
    if (!xf_seek(m_file, new_buf_offset, xfSeekSet))
    {
        if (eof)
            *eof = true;
        return 0;
    }
    
    memset(m_buf, 0, bfBufSize);
    m_buf_size = xf_read(m_file, m_buf, 1, bfBufSize);
    m_buf_offset = new_buf_offset;
    
    return m_buf[offset-m_buf_offset];
}
    











static const unsigned char utf8_sequence_lengths[256] =
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

#define UTF8_SEQLEN(ch) utf8_sequence_lengths[(ch)];



const xf_off_t BufferedTextFile::BUFFER_SIZE = 1000000;
const xf_off_t BufferedTextFile::WRITE_BUFFER_SIZE = 50000;
const xf_off_t BufferedTextFile::SEEK_BUFFER_SIZE = 1024;
const xf_off_t BufferedTextFile::FILE_BOF = -1;
const xf_off_t BufferedTextFile::FILE_EOF = -2;

BufferedTextFile::BufferedTextFile()
{
    m_file = (xf_file_t)0;
    m_data_start_offset = 0;
    m_buf_start_offset = 0;
    m_buf = NULL;
    m_buf_length = 0;
    m_buf_offset = 0;
    m_encoding = encodingDefault;
    m_write_buf = NULL;
    m_write_buf_length = 0;
    m_read_only = false;
}

BufferedTextFile::~BufferedTextFile()
{
    if (m_file)
        xf_close(m_file);
    delete[] m_buf;
    delete[] m_write_buf;
}

bool BufferedTextFile::openFile(const std::wstring& filename, int encoding)
{
    if (m_file)
        close();
    
    m_buf = new unsigned char[BUFFER_SIZE];
    if (!m_buf)
        return false;

    m_read_only = false;
    m_file = xf_open(filename, xfOpen, xfReadWrite, xfShareReadWrite);
    if (!m_file)
    {
        m_file = xf_open(filename, xfOpen, xfRead, xfShareReadWrite);
        if (!m_file)
            return false;
        m_read_only = true;
    }

    m_encoding = encoding;
    
    int bom_encoding = encodingDefault;
    unsigned char bom[4];
    memset(bom, 0, 4);
    xf_read(m_file, bom, 1, 4);
    if (bom[0] == 0xef && bom[1] == 0xbb && bom[2] == 0xbf)
    {
        bom_encoding = encodingUTF8;
        m_data_start_offset = 3;
    }
     else if (bom[0] == 0xff && bom[1] == 0xfe)
    {
        bom_encoding = encodingUTF16LE;
        m_data_start_offset = 2;
    }
     else if (bom[0] == 0xfe && bom[1] == 0xff)
    {
        bom_encoding = encodingUTF16BE;
        m_data_start_offset = 2;
    }
     else
    {
        // there is no byte order mark. 
        m_data_start_offset = 0;
    }
    
    
    if (m_encoding == encodingDefault)
    {
        if (bom_encoding == encodingDefault)
        {
            // try to detect the file encoding
            m_encoding = detectEncoding();
        }
         else
        {
            // use encoding specified in the byte-order mark
            m_encoding = bom_encoding;
        }
    }
    
    
    
    
    rewind();
    return true;
}

void BufferedTextFile::close()
{
    if (m_file)
        xf_close(m_file);
    delete[] m_buf;
    delete[] m_write_buf;
    
    m_file = (xf_file_t)0;
    m_data_start_offset = 0;
    m_buf_start_offset = 0;
    m_buf = NULL;
    m_buf_length = 0;
    m_buf_offset = 0;
    m_encoding = encodingDefault;
    m_write_buf = NULL;
    m_write_buf_length = 0;
}

int BufferedTextFile::detectEncoding()
{
    int encoding = encodingISO88591;
            
    xf_seek(m_file, m_data_start_offset, xfSeekSet);
    int r = xf_read(m_file, m_buf, 1, 16384);
    if (r <= 1)
        return encoding;
    
    // check for utf8
    int cnt_utf8 = 0;
    int cnt_8bit = 0;
    for (int i = 0; i < r-1; ++i)
    {
        if (m_buf[i] >= 0x80)
            cnt_8bit++;
        
        if ((m_buf[i] & 0xc0) == 0xc0)
        {
            if ((m_buf[i+1] & 0xc0) == 0x80)
            {
                cnt_utf8++;
            }
             else
            {
                // invalid utf8 character, set utf8 count to zero and break
                cnt_utf8 = 0;
                break;
            }
        }
    }
        
    if (cnt_utf8 > 0)
        return encodingUTF8;
    
    if (cnt_8bit > 0)
        return encodingISO88591;
    
    return encoding;
}

void BufferedTextFile::rewind()
{
    if (m_buf)
    {
        memset(m_buf, 0, BUFFER_SIZE);
    }
    
    if (xf_seek(m_file, m_data_start_offset, xfSeekSet))
    {
        m_buf_length = (size_t)xf_read(m_file, m_buf, 1, BUFFER_SIZE);
        m_buf_offset = 0;
        m_buf_start_offset = 0;
    }
     else
    {
        m_buf_start_offset = 0;
        m_buf_offset = 0;
        m_buf_length = 0;
    }
}
    
wchar_t BufferedTextFile::getChar()
{
    if (bof() || eof())
        return 0;
    
    if (m_encoding == encodingUTF8)
    {
        unsigned char ch = getByte();
        
        if (ch <= 127)
        {
            return (wchar_t)ch;
        }
         else
        {
            unsigned char buf[6];
            unsigned char* p = buf;
            unsigned char utf8_len = UTF8_SEQLEN(ch);
            
            if (getBytes(buf, utf8_len) != utf8_len)
                return 0; // premature eof; invalid utf character
                          
            unsigned int res = 0;
            switch (utf8_len)
            {
                case 6: res += *p++; res <<= 6;
                case 5: res += *p++; res <<= 6;
                case 4: res += *p++; res <<= 6;
                case 3: res += *p++; res <<= 6;
                case 2: res += *p++; res <<= 6;
                case 1: res += *p;
            }

            static const unsigned int utf8_offsets[7] =
            {
                0,
                0x00000000UL,
                0x00003080UL,
                0x000E2080UL,
                0x03C82080UL,
                0xFA082080UL,
                0x82082080UL
            };
            
            return (wchar_t)(res - utf8_offsets[utf8_len]);
        }
    }
     else if (m_encoding == encodingUTF16LE || m_encoding == encodingUTF16BE)
    {
        // for now support just the basic multilingual plane
        return (wchar_t)getUTF16Char();
    }
     else
    {
        return (wchar_t)getByte();
    }
}


void BufferedTextFile::skip(int delta)
{
    if (delta == 0)
        return;
    
    if (bof())
    {
        if (delta < 0)
            return;
        if (delta > 0)
        {
            skipByte(1);
            delta--;
            if (delta == 0)
                return;
        }   
    }
    
    if (m_encoding == encodingDefault || m_encoding == encodingISO88591)
    {
        skipByte(delta);
    }
     else if (m_encoding == encodingUTF8)
    {
        int i;
        if (delta > 0)
        {
            for (i = 0; i < delta; ++i)
            {
                if (eof())
                    return;
                unsigned char utf8_len = UTF8_SEQLEN(getByte());
                skipByte(utf8_len);
            }
        }
         else
        {
            unsigned char ch;
            
            for (i = 0; i < -delta; ++i)
            {
                if (bof())
                    return;
                
                skipByte(-1);
                ch = getByte();
                
                while (!bof() && ((ch & 0xc0) == 0x80))
                {
                    skipByte(-1);
                    ch = getByte();
                }
            }
        }
    }
     else if (m_encoding == encodingUTF16LE || m_encoding == encodingUTF16BE)
    {
        // for now support just the basic multilingual plane
        skipByte(delta*2);
    }
}

size_t BufferedTextFile::getBytes(unsigned char* p, size_t len)
{
    if (m_buf_offset + (int)len > m_buf_length)
    {
        // out of buffer range -- fetch bytes from disk
        if (!xf_seek(m_file, m_data_start_offset + m_buf_start_offset + m_buf_offset, xfSeekSet))
            return 0;
        return xf_read(m_file, p, 1, len);
    }
     else
    {
        // in buffer range -- fetch bytes from buffer
        memcpy(p, m_buf + m_buf_offset, len);
        return len;
    }
}

unsigned int BufferedTextFile::getUTF16Char()
{
    unsigned int ret;
    unsigned char buf[2];
    if (2 != getBytes(buf, 2))
        return 0;
    
    if (m_encoding == encodingUTF16LE)
    {
        ret = buf[1];
        ret <<= 8;
        ret |= buf[0];
    }
     else
    {
        ret = buf[0];
        ret <<= 8;
        ret |= buf[1];
    }

    return ret;
}
  
void BufferedTextFile::skipByte(int delta)
{
    if (delta == 0)
        return;
    
    int new_offset = (m_buf_offset + delta);
    
    if (new_offset > 0 && new_offset < m_buf_length)
    {
        m_buf_offset = new_offset;
    }
     else
    {
        // the char is outside our buffer, so skip to
        // the appropriate location, read the data into
        // the buffer and return the appropriate char
        
        xf_off_t new_file_offset = m_buf_start_offset + (xf_off_t)new_offset;
        xf_off_t buf_offset_mod = new_file_offset % BUFFER_SIZE;
        xf_off_t new_buf_offset = (new_file_offset / BUFFER_SIZE) * BUFFER_SIZE;


        memset(m_buf, 0, BUFFER_SIZE);
        
        if (new_file_offset < 0)
        {
            // bof condition
            m_buf_length = 0;
            m_buf_offset = -1;
            m_buf_start_offset = 0;
            return;
        }
        
        if (xf_seek(m_file, m_data_start_offset + new_buf_offset, xfSeekSet))
        {
            m_buf_length = (size_t)xf_read(m_file, m_buf, 1, BUFFER_SIZE);
            m_buf_offset = (int)buf_offset_mod;
            m_buf_start_offset = new_buf_offset;
        }
         else
        {
            // seek failed -- set length to zero
            m_buf_start_offset = new_buf_offset;
            m_buf_offset = 0;
            m_buf_length = 0;
        }
    }
}

xf_off_t BufferedTextFile::getOffset() const
{
    return m_buf_start_offset + (xf_off_t)m_buf_offset;
}

void BufferedTextFile::goOffset(xf_off_t offset)
{
    if (xf_seek(m_file, m_data_start_offset + offset, xfSeekSet))
    {
        m_buf_length = (size_t)xf_read(m_file, m_buf, 1, SEEK_BUFFER_SIZE);
        m_buf_offset = 0;
        m_buf_start_offset = offset;
    }
     else
    {
        // seek failed -- set length to zero
        m_buf_start_offset = offset;
        m_buf_offset = 0;
        m_buf_length = 0;
    }
}


bool BufferedTextFile::write(const std::wstring& str)
{
    if (!m_write_buf)
        m_write_buf = new unsigned char[WRITE_BUFFER_SIZE];
    
    size_t i, cnt = str.size();
    unsigned long ch;
    unsigned char* dest = m_write_buf;
    m_write_buf_length = 0;
    
    for (i = 0; i < cnt; ++i)
    {
        ch = (unsigned long)str[i];
        
        if (m_encoding == encodingISO88591 || m_encoding == encodingDefault)
        {
            if (ch <= 255)
                *dest++ = (unsigned char)ch;
                 else
                *dest++ = '?';
            m_write_buf_length++;
        }
         else if (m_encoding == encodingUTF8)
        {
            if (ch < 0x80)
            {
                *dest++ = (unsigned char)ch;
                m_write_buf_length++;
            }
             else if (ch < 0x800)
            {
                *dest++ = (unsigned char)((ch >> 6) | 0xc0);
                *dest++ = (unsigned char)((ch & 0x3f) | 0x80);
                m_write_buf_length += 2;
            }
             else if (ch < 0x10000)
            {
                *dest++ = (unsigned char)((ch >> 12) | 0xe0);
                *dest++ = (unsigned char)(((ch >> 6) & 0x3f) | 0x80);
                *dest++ = (unsigned char)((ch & 0x3f) | 0x80);
                m_write_buf_length += 3;
            }
             else if (ch < 0x200000)
            {
                *dest++ = (unsigned char)((ch >> 18) | 0xf0);
                *dest++ = (unsigned char)(((ch >> 12) & 0x3f) | 0x80);
                *dest++ = (unsigned char)(((ch >> 6) & 0x3f) | 0x80);
                *dest++ = (unsigned char)((ch & 0x3f) | 0x80);
                m_write_buf_length += 4;
            }
        }
         else if (m_encoding == encodingUTF16LE)
        {
            *dest++ = (ch & 0xff);
            *dest++ = (unsigned char)(ch >> 16);
            m_write_buf_length += 2;
        }
         else if (m_encoding == encodingUTF16BE)
        {
            *dest++ = (unsigned char)(ch >> 16);
            *dest++ = (ch & 0xff);
            m_write_buf_length += 2;
        }
        
        if (i+1 >= cnt || (m_write_buf_length > WRITE_BUFFER_SIZE-10))
        {
            xf_seek(m_file, 0, xfSeekEnd);
            if (m_write_buf_length == xf_write(m_file, m_write_buf, 1, m_write_buf_length))
            {
                m_write_buf_length = 0;
                dest = m_write_buf;
            }
             else
            {
                m_write_buf_length = 0;
                return false;
            }
        }
    }

    m_write_buf_length = 0;
    return true;
}
