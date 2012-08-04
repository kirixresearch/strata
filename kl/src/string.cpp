/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2012-08-04
 *
 */


#include "kl/string.h"


namespace kl
{


void trimRight(std::string& s)
{
    size_t orig_len = s.length();
    size_t len = orig_len;

    while (len > 0)
    {
        if (!isspace((unsigned char)s[len-1]))
            break;
        --len;
    }

    if (orig_len != len)
    {
        s.resize(len);
    }
}

void trimRight(std::wstring& s)
{
    size_t orig_len = s.length();
    size_t len = orig_len;

    while (len > 0)
    {
        if (!iswspace(s[len-1]))
            break;
        --len;
    }

    if (orig_len != len)
    {
        s.resize(len);
    }
}

void trimLeft(std::string& s)
{
    const char* c = s.c_str();
    int cnt = 0;

    while (*c && isspace((unsigned char)*c))
    {
        ++cnt;
        ++c;
    }

    if (cnt > 0)
    {
        s.erase(0, cnt);
    }
}

void trimLeft(std::wstring& s)
{
    const wchar_t* c = s.c_str();
    int cnt = 0;

    while (*c && iswspace(*c))
    {
        ++cnt;
        ++c;
    }

    if (cnt > 0)
    {
        s.erase(0, cnt);
    }
}


std::wstring itowstring(int val)
{
    wchar_t buf[80];
    swprintf(buf, L"%d", val);
    return buf;
}


size_t replaceStr(std::wstring& str,
                  const std::wstring& search,
                  const std::wstring& replace,
                  bool global)
{
    size_t offset = 0;
    size_t search_len = search.length();
    size_t replace_len = replace.length();
    size_t num_replaces = 0;
    
    while (1)
    {
        size_t pos = str.find(search, offset);
        if (pos == str.npos)
            break;
            
        str.erase(pos, search_len);
        str.insert(pos, replace);
        
        offset = (pos+replace_len);
        num_replaces++;
        
        if (search_len == 0 || !global)
            break;
    }
    
    return num_replaces;
}



size_t replaceStrNoCase(std::wstring& str,
                        const std::wstring& search,
                        const std::wstring& replace)
{
    size_t offset = 0;
    size_t search_len = search.length();
    size_t replace_len = replace.length();
    size_t num_replaces = 0;
    
    std::wstring uc_search = search;
    std::wstring uc_str;
    kl::makeUpper(uc_search);
    
    while (1)
    {
        uc_str = str;
        kl::makeUpper(uc_str);
        size_t pos = uc_str.find(uc_search, offset);

        if (pos == str.npos)
            break;
            
        str.erase(pos, search_len);
        str.insert(pos, replace);
        
        offset = pos+replace_len;
        num_replaces++;
    }
    
    return num_replaces;
}


double nolocale_atof(const char* num)
{
    static char search_ch = 0;
    static char replace_ch = 0;

    if (!search_ch)
    {
        lconv* l = localeconv();
        replace_ch = (unsigned char)*l->decimal_point;
        if (replace_ch == ',')
        {
            search_ch = '.';
        }
         else
        {
            search_ch = ',';
        }
    }

    char buf[128];
    int i = 0;

    while (1)
    {
        if (i == sizeof(buf)-1)
        {
            buf[i] = 0;
            break;
        }

        if (*num == search_ch)
            buf[i++] = replace_ch;
             else
            buf[i++] = *num;

        if (*num == 0)
            break;
        num++;
    }

    return atof(buf);
}




int wtoi(const wchar_t* s)
{
    char buf[64];
    int i = 0;

    while (*s)
    {
        buf[i] = (char)*s;
        ++s;
        if (i > 60)
            break;
        ++i;
    }

    buf[i] = 0;

    return atoi(buf);
}

double wtof(const wchar_t* s)
{
    char buf[64];
    int i = 0;

    while (*s)
    {
        buf[i] = (char)*s;
        ++s;
        if (i > 60)
            break;
        ++i;
    }

    buf[i] = 0;

    return atof(buf);
}

double nolocale_wtof(const wchar_t* num)
{
    static wchar_t search_ch = 0;
    static wchar_t replace_ch = 0;

    if (!search_ch)
    {
        lconv* l = localeconv();
        replace_ch = (unsigned char)*l->decimal_point;
        if (replace_ch == L',')
        {
            search_ch = L'.';
        }
         else
        {
            search_ch = L',';
        }
    }

    char buf[128];
    int i = 0;

    while (1)
    {
        if (i == sizeof(buf)-1)
        {
            buf[i] = 0;
            break;
        }

        if (*num == search_ch)
            buf[i++] = (char)replace_ch;
             else
            buf[i++] = (char)*num;

        if (*num == 0)
            break;
        num++;
    }

    return atof(buf);
}





// these functions convert a std::(w)string to a UCS-2 buffer and vice versa.
// Big-endian and little-endian versions are provided

void string2ucsle(unsigned char* dest,
                  const std::string& src,
                  int pad_len)
{
    size_t i = 0;
    size_t str_len = src.length();
    wchar_t ch;

    while (pad_len)
    {
        if (i < str_len)
            ch = (unsigned char)src[i];
             else
            ch = 0;

        *dest = (ch & 0xff);
        ++dest;
        *dest = (ch >> 8) & 0xff;
        ++dest;
        ++i;
        --pad_len;
    }
}


void wstring2ucsle(unsigned char* dest,
                   const std::wstring& src,
                   int pad_len)
{
    size_t i = 0;
    size_t str_len = src.length();
    wchar_t ch;

    while (pad_len)
    {
        if (i < str_len)
            ch = src[i];
             else
            ch = 0;

        *dest = (ch & 0xff);
        ++dest;
        *dest = (ch >> 8) & 0xff;
        ++dest;
        ++i;
        --pad_len;
    }
}

void ucsle2wstring(std::wstring& dest,
                   const unsigned char* src,
                   int buf_len)
{
#ifdef WIN32
    const wchar_t* wsrc = (const wchar_t*)src;
    int len;
    for (len = 0; len < buf_len; ++len)
    {
        if (!*(wsrc+len))
            break;
    }

    dest.assign(wsrc, len);
#else
    int i;
    wchar_t ch;

    dest.resize(buf_len);
    for (i = 0; i < buf_len; ++i)
    {
        ch = (wchar_t)*src;
        ++src;
        ch |= (((wchar_t)*src) << 8);
        ++src;

        if (ch == 0)
        {
            dest.resize(i);
            break;
        }

        dest[i] = ch;
    }
#endif
}

void ucsle2string(std::string& dest,
                  const unsigned char* src,
                  int buf_len)
{
    int i;
    wchar_t ch;

    dest.resize(buf_len);
    for (i = 0; i < buf_len; ++i)
    {
        ch = (wchar_t)*src;
        ++src;
        ch |= (((wchar_t)*src) << 8);
        ++src;

        if (ch == 0)
        {
            dest.resize(i);
            break;
        }

        if (ch <= 255)
        {
            dest[i] = (char)(unsigned char)ch;
        } 
         else
        {
            dest[i] = '?';
        }
    }
}


void wstring2ucsbe(unsigned char* dest,
                   const std::wstring& src,
                   int pad_len)
{
    size_t i = 0;
    size_t str_len = src.length();
    wchar_t ch;

    while (pad_len)
    {
        if (i < str_len)
            ch = src[i];
             else
            ch = 0;

        *dest = ((ch >> 8) & 0xff);
        ++dest;
        *dest = ch & 0xff;
        ++dest;
        ++i;
        --pad_len;
    }
}

void ucsbe2wstring(std::wstring& dest,
                   const unsigned char* src,
                   int buf_len)
{
    int i;
    wchar_t ch;

    dest.resize(buf_len);
    for (i = 0; i < buf_len; ++i)
    {
        ch = ((wchar_t)*src) << 8;
        ++src;
        ch |= (wchar_t)*src;
        ++src;

        if (ch == 0)
        {
            dest.resize(i);
            break;
        }

        dest[i] = ch;
    }
}




};

