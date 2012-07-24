/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2001-08-25
 *
 */


#ifndef __KL_STRING_H
#define __KL_STRING_H


// remove VC9 warnings - eventually commenting this
// out and implementing specific fixes for these warnings
// is a good idea.
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


#include <cctype>
#include <cwctype>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include <functional>


#ifdef __APPLE__
#include "portable.h"
#endif


namespace kl
{


inline void trimRight(std::string& s)
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

inline void trimRight(std::wstring& s)
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

inline void trimLeft(std::string& s)
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

inline void trimLeft(std::wstring& s)
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

inline void trim(std::string& s)
{
    trimRight(s);
    trimLeft(s);
}

inline void trim(std::wstring& s)
{
    trimRight(s);
    trimLeft(s);
}

inline void makeUpper(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), towupper);
}

inline void makeUpper(std::wstring& s)
{
    std::transform(s.begin(), s.end(), s.begin(), towupper);
}

inline void makeUpper(wchar_t* p)
{
    while (*p)
    {
        *p = towupper(*p);
        ++p;
    }
}

inline void makeUpper(char* p)
{
    while (*p)
    {
        *p = (char)(unsigned char)towupper((wchar_t)*p);
        ++p;
    }
}

inline void makeLower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), towlower);
}

inline void makeLower(std::wstring& s)
{
    std::transform(s.begin(), s.end(), s.begin(), towlower);
}

inline void makeLower(wchar_t* p)
{
    while (*p)
    {
        *p = towlower(*p);
        ++p;
    }
}

inline void makeLower(char* p)
{
    while (*p)
    {
        *p = (char)(unsigned char)towlower((wchar_t)*p);
        ++p;
    }
}

inline std::string beforeFirst(const std::string& str, char ch)
{
    size_t pos = str.find(ch);
    if (pos == str.npos)
        return str;
    return str.substr(0, pos);
}

inline std::wstring beforeFirst(const std::wstring& str, wchar_t ch)
{
   size_t pos = str.find(ch);
    if (pos == str.npos)
        return str;
    return str.substr(0, pos);
}

inline std::string afterFirst(const std::string& str, char ch)
{
    size_t pos = str.find(ch);
    if (pos == str.npos)
        return str;
    return str.substr(pos+1);
}

inline std::wstring afterFirst(const std::wstring& str, wchar_t ch)
{
    size_t pos = str.find(ch);
    if (pos == str.npos)
        return str;
    return str.substr(pos+1);
}

inline std::string beforeLast(const std::string& str, char ch)
{
    size_t pos = str.find_last_of(ch);
    if (pos == str.npos)
        return str;
    return str.substr(0, pos);
}

inline std::wstring beforeLast(const std::wstring& str, wchar_t ch)
{
    size_t pos = str.find_last_of(ch);
    if (pos == str.npos)
        return str;
    return str.substr(0, pos);
}

inline std::string afterLast(const std::string& str, char ch)
{
    size_t pos = str.find_last_of(ch);
    if (pos == str.npos)
        return str;
    return str.substr(pos+1);
}

inline std::wstring afterLast(const std::wstring& str, wchar_t ch)
{
    size_t pos = str.find_last_of(ch);
    if (pos == str.npos)
        return str;
    return str.substr(pos+1);
}


inline size_t replaceStr(std::wstring& str,
                         const std::wstring& search,
                         const std::wstring& replace,
                         bool global = true)
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


inline size_t replaceStrNoCase(std::wstring& str,
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


inline double nolocale_atof(const char* num)
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


inline double nolocale_atof(const std::string& num)
{
    return nolocale_atof(num.c_str());
}

inline int wtoi(const wchar_t* s)
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

inline double wtof(const wchar_t* s)
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

inline double nolocale_wtof(const wchar_t* num)
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

inline double nolocale_wtof(const std::wstring& num)
{
    return nolocale_wtof(num.c_str());
}

inline int wtoi(const std::wstring& s)
{
    return wtoi(s.c_str());
}

inline double wtof(const std::wstring& s)
{
    return wtof(s.c_str());
}


// -- string conversion helpers --

inline std::wstring towstring(const char* src)
{
    std::wstring res;
    size_t i, src_len = strlen(src);

    res.resize(src_len);
    for (i = 0; i < src_len; ++i)
        res[i] = (wchar_t)(unsigned char)(*(src+i));

    return res;
}

inline std::wstring towstring(const wchar_t* src)
{
    std::wstring res = src;
    return res;
}

inline std::wstring towstring(const std::string& src)
{
    std::wstring res;
    size_t i, src_len = src.length();

    res.resize(src_len);
    for (i = 0; i < src_len; ++i)
        res[i] = (wchar_t)(unsigned char)src[i];

    return res;
}

inline std::wstring towstring(const std::wstring& src)
{
    return src;
}

inline std::string tostring(const std::wstring& src)
{
    std::string res;
    wchar_t c;
    size_t i, src_len = src.length();

    res.resize(src_len);
    
    for (i = 0; i < src_len; ++i)
    {
        c = src[i];
        if (c > 255)
            c = '?';
        res[i] = (char)(unsigned char)c;
    }

    return res;
}

inline std::string tostring(const std::string& src)
{
    return src;
}

inline std::string tostring(const wchar_t* src)
{
    std::string res;
    size_t i, src_len = wcslen(src);
    res.resize(src_len);
    
    for (i = 0; i < src_len; ++i)
    {
        if (*(src+i) > 255)
            res[i] = '?';
             else
            res[i] = (char)(unsigned char)(*(src+i));
    }
    
    return res;
}


#ifdef _UNICODE
#define totstring towstring
#else
#define totstring tostring
#endif


inline void towstring(std::wstring& dest, const char* src, size_t len = (size_t)-1)
{
    size_t i;

    if (len == (size_t)-1)
        len = strlen(src);

    dest.resize(len);
    for (i = 0; i < len; ++i)
    {
        if (src[i] == 0)
        {
            dest.resize(i);
            return;
        }

        dest[i] = (unsigned char)src[i];
    }
}




// these functions convert a std::(w)string to a UCS-2 buffer and vice versa.
// Big-endian and little-endian versions are provided

inline void string2ucsle(unsigned char* dest,
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


inline void wstring2ucsle(unsigned char* dest,
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

inline void ucsle2wstring(std::wstring& dest,
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

inline void ucsle2string(std::string& dest,
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


inline void wstring2ucsbe(unsigned char* dest,
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

inline void ucsbe2wstring(std::wstring& dest,
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


class tstr
{
public:

    tstr(const std::string& s)
    {
        size_t len = s.length();
        m_s = new char[len+1];
        m_ws = new wchar_t[len+1];

        strcpy(m_s, s.c_str());
        mbstowcs(m_ws, m_s, len+1);
    }

    tstr(const std::wstring& ws)
    {
        size_t len = ws.length();
        m_s = new char[len+1];
        m_ws = new wchar_t[len+1];

        wcscpy(m_ws, ws.c_str());
        wcstombs(m_s, m_ws, len+1);
    }

    tstr(const char* s)
    {
        size_t len = strlen(s);
        m_s = new char[len+1];
        m_ws = new wchar_t[len+1];

        strcpy(m_s, s);
        mbstowcs(m_ws, m_s, len+1);
    }

    tstr(const wchar_t* ws)
    {
        size_t len = wcslen(ws);
        m_s = new char[len+1];
        m_ws = new wchar_t[len+1];

        wcscpy(m_ws, ws);
        wcstombs(m_s, m_ws, len+1);
    }

    ~tstr()
    {
        delete[] m_s;
        delete[] m_ws;
    }
    
    operator wchar_t*()
    {
        return m_ws;
    }

    operator char*()
    {
        return m_s;
    }

#ifdef _UNICODE
    wchar_t* c_str()
    {
        return m_ws;
    }
#else
    char* c_str()
    {
        return m_s;
    }
#endif

private:
    char* m_s;
    wchar_t* m_ws;
};




struct cmp_nocase : std::binary_function<const std::wstring&,
                                         const std::wstring&,
                                         bool>
{
    bool operator()(const std::wstring& lhs, 
                    const std::wstring& rhs) const
    {
#ifdef _MSC_VER
        return (wcsicmp(lhs.c_str(), rhs.c_str()) < 0);
#else
        return (wcscasecmp(lhs.c_str(), rhs.c_str()) < 0);
#endif
    }
};


};


#endif

