/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2007-01-22
 *
 */

#include <xd/xd.h>
#include <cmath>
#include <cstring>
#include "rawtext.h"
#include "../xdcommon/filestream.h"




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

#define UTF8_SEQLEN(ch) utf8_sequence_lengths[(ch)]



const long long BufferedTextFile::BUFFER_SIZE = 1000000;
const long long BufferedTextFile::WRITE_BUFFER_SIZE = 50000;
const long long BufferedTextFile::SEEK_BUFFER_SIZE = 1024;
const long long BufferedTextFile::FILE_BOF = -1;
const long long BufferedTextFile::FILE_EOF = -2;

BufferedTextFile::BufferedTextFile()
{
    m_stream = NULL;
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
    if (m_stream)
        m_stream->unref();
    delete[] m_buf;
    delete[] m_write_buf;
}

bool BufferedTextFile::open(const std::wstring& filename, int encoding)
{
    if (m_stream)
        close();
    
    FileStream* f = new FileStream;
    if (!f->open(filename))
        return false;

    bool res = open(static_cast<xd::IStream*>(f));
    if (!res)
    {
        delete f;
        return false;
    }

    return true;
}


bool BufferedTextFile::open(xd::IStream* stream, int encoding)
{
    m_buf = new unsigned char[BUFFER_SIZE];
    if (!m_buf)
        return false;

    m_stream = stream;
    m_stream->ref();
    m_read_only = false;
    m_encoding = encoding;
    


    int bom_encoding = encodingDefault;
    unsigned char bom[4];
    memset(bom, 0, 4);
    m_stream->seek(0, xd::seekSet);
    m_stream->read(bom, 4, NULL);
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
    if (m_stream)
    {
        m_stream->unref();
        m_stream = NULL;
    }

    delete[] m_buf;
    delete[] m_write_buf;
    
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
    
    if (!m_stream)
        return encoding;
            
    m_stream->seek(m_data_start_offset, xd::seekSet);

    unsigned long bytes_read = 0;
    m_stream->read(m_buf, 16384, &bytes_read);
    if (bytes_read <= 1)
        return encoding;

    // check for utf8
    int cnt_utf8 = 0;
    int cnt_8bit = 0;
    for (unsigned long i = 0; i < bytes_read-1; ++i)
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
    
    if (m_stream->seek(m_data_start_offset, xd::seekSet))
    {
        unsigned long bytes_read = 0;

        m_stream->read(m_buf, BUFFER_SIZE, &bytes_read);
        m_buf_length = bytes_read;
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
        // requested data is out of buffer range -- fetch a new block from stream
        if (!m_stream->seek(m_data_start_offset + m_buf_start_offset + m_buf_offset, xd::seekSet))
            return 0;

        unsigned long bytes_read = 0;
        m_stream->read(p, (unsigned long)len, &bytes_read);
        return bytes_read;
    }
     else
    {
        // requested data is in buffer range -- copy bytes from buffer
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
        
        long long new_file_offset = m_buf_start_offset + (long long)new_offset;
        long long buf_offset_mod = new_file_offset % BUFFER_SIZE;
        long long new_buf_offset = (new_file_offset / BUFFER_SIZE) * BUFFER_SIZE;


        memset(m_buf, 0, BUFFER_SIZE);
        
        if (new_file_offset < 0)
        {
            // bof condition
            m_buf_length = 0;
            m_buf_offset = -1;
            m_buf_start_offset = 0;
            return;
        }
        
        if (m_stream->seek(m_data_start_offset + new_buf_offset, xd::seekSet))
        {
            unsigned long bytes_read = 0;
            m_stream->read(m_buf, BUFFER_SIZE, &bytes_read);
            
            m_buf_length = bytes_read;
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

long long BufferedTextFile::getOffset() const
{
    return m_buf_start_offset + (long long)m_buf_offset;
}

void BufferedTextFile::goOffset(long long offset)
{
    if (m_stream->seek(m_data_start_offset + offset, xd::seekSet))
    {
        unsigned long bytes_read = 0;
        m_stream->read(m_buf, SEEK_BUFFER_SIZE, &bytes_read);
        
        m_buf_length = bytes_read;
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
            m_stream->seek(0, xd::seekEnd);

            unsigned long bytes_written = 0;
            bool res = m_stream->write(m_write_buf, m_write_buf_length, &bytes_written);

            if (res && m_write_buf_length == bytes_written)
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
