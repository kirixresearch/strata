/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-04-24
 *
 */


#ifndef __KL_UTF8_H
#define __KL_UTF8_H


#include <string>


namespace kl
{


// utf8_utf8to*() - converts to a string from a utf8 string
//   dest      - destination buffer
//   dest_size - destination buffer size
//   src       - source buffer (utf8)
//   src_size  - source buffer size, or -1 if null terminated

size_t utf8_utf8toa(   char* dest,
                       size_t dest_size,
                       const char *src,
                       size_t src_size);

                       
size_t utf8_utf8tow(   wchar_t* dest,
                       size_t dest_size,
                       const char* src,
                       size_t src_size);
                       
size_t utf8_utf8toucs2(unsigned short* dest,
                       size_t dest_size,
                       const char* src,
                       size_t src_size);
                       
size_t utf8_utf8toucs4(unsigned long* dest,
                       size_t dest_size,
                       const char* src,
                       size_t src_size);

std::wstring utf8_utf8towstr(
                       const char* src,
                       size_t src_size = (size_t)-1);
                        


// utf8_*toutf8() - converts a string into a utf8 string
//   dest      - destination buffer (utf8)
//   dest_size - destination buffer size
//   src       - source buffer
//   src_size  - source buffer size, or -1 if null terminated
//   output_buf_size - optional ptr to output byte count
//                     (including terminating null, if there
//                     was enough space in the output buffer)

size_t utf8_atoutf8(   char* dest,
                       size_t dest_size,
                       const char *src,
                       size_t src_size,
                       size_t* output_buf_size = NULL);
                       
size_t utf8_wtoutf8(   char* dest,
                       size_t dest_size,
                       const wchar_t* src,
                       size_t src_size,
                       size_t* output_buf_size = NULL);
                       
size_t utf8_ucs2toutf8(char* dest,
                       size_t dest_size,
                       const unsigned short *src,
                       size_t src_size,
                       size_t* output_buf_size = NULL);
                       
size_t utf8_ucs4toutf8(char* dest,
                       size_t dest_size,
                       const unsigned long *src,
                       size_t src_size,
                       size_t* output_buf_size = NULL);
                       
unsigned long utf8_nextchar(const char* s, size_t* i);

size_t utf8_strlen(const char* s);




std::wstring fromUtf8(const char* src, size_t src_size = (size_t)-1);


class toUtf8
{
public:

    toUtf8(const wchar_t* s);
    toUtf8(const std::wstring& s);
    
    ~toUtf8();
 
    operator const char*() const { return m_s; }
    
public:

    char* m_s;
};


};


#endif


