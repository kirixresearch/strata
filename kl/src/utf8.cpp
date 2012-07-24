/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-04-24
 *
 */


#include <cwchar>
#include <cstring>
#include <string>
#include "kl/utf8.h"


namespace kl
{


static const unsigned long offsets_from_utf8[6] =
{
    0x00000000UL,
    0x00003080UL,
    0x000E2080UL,
    0x03C82080UL,
    0xFA082080UL,
    0x82082080UL
};

static const char trailing_bytes_for_utf8[256] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

#define isutf(c) (((c) & 0xc0) != 0x80)



// utf8_utf8toucs4() - converts a utf8 string into a ucs4 string
//   dest      - destination buffer
//   dest_size - destination buffer size
//   src       - source buffer
//   src_size  - source buffer size, or -1 if null terminated

template<class T>
size_t utf8_fromutf8(T* dest,
                     size_t dest_size,
                     const char* src,
                     size_t src_size)
{
    const char *src_end = src + src_size;
    unsigned long ch;
    int num_bytes;
    size_t i = 0;

    if (dest_size == 0)
        return 0;

    while (i < dest_size - 1)
    {
        if (src_size == -1)
        {
            if (*src == 0)
                break;
            num_bytes = trailing_bytes_for_utf8[(unsigned char)*src];
        }
         else
        {
            if (src >= src_end)
                break;
            num_bytes = trailing_bytes_for_utf8[(unsigned char)*src];
            if (src + num_bytes >= src_end)
                break;
        }
        
        ch = 0;
        
        switch (num_bytes)
        {
            case 5: ch += (unsigned char)*src++; ch <<= 6;
            case 4: ch += (unsigned char)*src++; ch <<= 6;
            case 3: ch += (unsigned char)*src++; ch <<= 6;
            case 2: ch += (unsigned char)*src++; ch <<= 6;
            case 1: ch += (unsigned char)*src++; ch <<= 6;
            case 0: ch += (unsigned char)*src++;
        }
        
        ch -= offsets_from_utf8[num_bytes];
        
        dest[i++] = (T)ch;
    }
    
    dest[i] = 0;
    return i;
}




// utf8_ucs4toutf8() - converts a ucs4 string into a utf8 string
//   dest      - destination buffer
//   dest_size - destination buffer size
//   src       - source buffer
//   src_size  - source buffer size, or -1 if null terminated

template<class T>
size_t utf8_toutf8(char* dest,
                   size_t dest_size,
                   const T* src,
                   size_t src_size,
                   size_t* output_buf_size)
{
    char* dest_start = dest;
    char* dest_end = dest + dest_size;
    unsigned long ch;
    size_t i = 0;

    if (dest_size == 0)
    {
        if (output_buf_size)
            *output_buf_size = (dest-dest_start);

        return 0;
    }

    while (src_size < 0 ? src[i] != 0 : i < src_size)
    {
        ch = (unsigned long)src[i];
        
        if (ch < 0x80)
        {
            if (dest >= dest_end)
            {
                if (output_buf_size)
                    *output_buf_size = (dest-dest_start);

                return i;
            }
            
            *dest++ = (char)ch;
        }
         else if (ch < 0x800)
        {
            if (dest >= dest_end-1)
            {
                if (output_buf_size)
                    *output_buf_size = (dest-dest_start);
                return i;
            }
            *dest++ = (char)((ch>>6) | 0xC0);
            *dest++ = (char)((ch & 0x3F) | 0x80);
        }
         else if (ch < 0x10000)
        {
            if (dest >= dest_end-2)
            {
                if (output_buf_size)
                    *output_buf_size = (dest-dest_start);
                return i;
            }
            
            *dest++ = (char)((ch>>12) | 0xE0);
            *dest++ = (char)(((ch>>6) & 0x3F) | 0x80);
            *dest++ = (char)((ch & 0x3F) | 0x80);
        }
         else if (ch < 0x200000)
        {
            if (dest >= dest_end-3)
            {
                if (output_buf_size)
                    *output_buf_size = (dest-dest_start);
                return i;
            }
            
            *dest++ = (char)((ch>>18) | 0xF0);
            *dest++ = (char)(((ch>>12) & 0x3F) | 0x80);
            *dest++ = (char)(((ch>>6) & 0x3F) | 0x80);
            *dest++ = (char)((ch & 0x3F) | 0x80);
        }
        
        i++;
    }
    
    if (dest < dest_end)
    {
        *dest = '\0';
        dest++;
    }

    if (output_buf_size)
        *output_buf_size = (dest-dest_start);
    
    return i;
}



size_t utf8_utf8toa(   char* dest,
                       size_t dest_size,
                       const char *src,
                       size_t src_size)
{
    return utf8_fromutf8<unsigned char>((unsigned char*)dest, dest_size, src, src_size);
}

size_t utf8_utf8tow(wchar_t* dest,
                    size_t dest_size,
                    const char* src,
                    size_t src_size)
{
    return utf8_fromutf8<wchar_t>(dest, dest_size, src, src_size);
}

size_t utf8_utf8toucs2(unsigned short* dest,
                       size_t dest_size,
                       const char* src,
                       size_t src_size)
{
    return utf8_fromutf8<unsigned short>(dest, dest_size, src, src_size);
}
      
size_t utf8_utf8toucs4(unsigned long* dest,
                       size_t dest_size,
                       const char* src,
                       size_t src_size)
{
    return utf8_fromutf8<unsigned long>(dest, dest_size, src, src_size);
}
    
std::wstring utf8_utf8towstr(
                       const char* src,
                       size_t src_size)
{
    if (src_size == (size_t)-1)
    {
        src_size = 0;
        while (*(src+src_size))
            src_size++;
    }
    
    wchar_t* buf = new wchar_t[src_size+1];
    utf8_utf8tow(buf, src_size+1, src, src_size);
    std::wstring res = buf;
    delete[] buf;
    return res;
}



size_t utf8_atoutf8(   char* dest,
                       size_t dest_size,
                       const char *src,
                       size_t src_size,
                       size_t* output_buf_size)
{
    return utf8_toutf8<unsigned char>(dest, dest_size, (const unsigned char*)src, src_size, output_buf_size);
}

size_t utf8_wtoutf8(char* dest,
                    size_t dest_size,
                    const wchar_t* src,
                    size_t src_size,
                    size_t* output_buf_size)
{
    return utf8_toutf8<wchar_t>(dest, dest_size, src, src_size, output_buf_size);
}

size_t utf8_ucs2toutf8(char* dest,
                       size_t dest_size,
                       const unsigned short *src,
                       size_t src_size,
                       size_t* output_buf_size)
{
    return utf8_toutf8<unsigned short>(dest, dest_size, src, src_size, output_buf_size);
}
                          
size_t utf8_ucs4toutf8(char* dest,
                       size_t dest_size,
                       const unsigned long *src,
                       size_t src_size,
                       size_t* output_buf_size)
{
    return utf8_toutf8<unsigned long>(dest, dest_size, src, src_size, output_buf_size);
}


// reads the next character from a utf8 string
unsigned long utf8_nextchar(const char* s, size_t* i)
{
    unsigned long ch = 0;
    int sz = 0;

    do
    {
        ch <<= 6;
        ch += (unsigned char)s[*i];
        (*i)++;
        sz++;
    } while (s[*i] && !isutf(s[*i]));
    
    if (sz > 6)
        return 0;
        
    ch -= offsets_from_utf8[sz-1];

    return ch;
}

// number of characters
size_t utf8_strlen(const char *s)
{
    size_t count = 0;
    size_t i = 0;

    while (utf8_nextchar(s, &i) != 0)
        count++;

    return count;
}



std::wstring fromUtf8(const char* src, size_t src_size)
{
    if (src_size == (size_t)-1)
        src_size = strlen(src);
    wchar_t* buf = new wchar_t[src_size+1];
    utf8_utf8tow(buf, src_size+1, src, src_size);
    std::wstring r = buf;
    delete[] buf;
    return r;
}





toUtf8::toUtf8(const wchar_t* s)
{
    size_t src_len = wcslen(s);
    size_t utf8_size = (src_len*4)+1;
    m_s = new char[utf8_size];
    size_t output_buf_size = 0;
    utf8_toutf8(m_s, utf8_size, s, src_len, &output_buf_size);
}

toUtf8::toUtf8(const std::wstring& s)
{
    size_t utf8_size = (s.length()*4)+1;
    m_s = new char[utf8_size];
    size_t output_buf_size = 0;
    utf8_toutf8(m_s, utf8_size, s.c_str(), s.length(), &output_buf_size);
}

toUtf8::~toUtf8()
{
    delete[] m_s;
}






};
