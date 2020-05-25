/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
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
#include <vector>


#ifdef __APPLE__
#include "portable.h"
#endif


namespace kl
{


void trimRight(std::string& s);
void trimRight(std::wstring& s);
void trimLeft(std::string& s);
void trimLeft(std::wstring& s);

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


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#endif

inline void makeLower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), towlower);
}

inline void makeLower(std::wstring& s)
{
    std::transform(s.begin(), s.end(), s.begin(), towlower);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif


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

std::wstring itowstring(int val);
std::string itostring(int val);

std::string stdsprintf(const char* fmt, ...);
std::wstring stdswprintf(const wchar_t* fmt, ...);

std::wstring formattedNumber(double d, int dec = -1);
std::wstring getUniqueString();

size_t stringFrequency(const std::wstring& haystack, wchar_t needle);

std::wstring joinList(std::vector<std::wstring>& vec, const std::wstring join_str = L",");

bool iequals(const std::wstring& s1, const std::wstring& s2);
bool iequals(const std::string& s1, const std::string& s2);
bool iequals(const std::wstring& s1, const std::string& s2);

bool icontains(const std::wstring& s1, const std::wstring& s2);
bool icontains(const std::string& s1, const std::string& s2);

size_t ifind(const std::string& s1, const std::string& s2);
size_t ifind(const std::wstring& s1, const std::wstring& s2);


void parseDelimitedList(const std::wstring& s,
                        std::vector<std::wstring>& vec,
                        wchar_t delimiter,
                        bool zero_level = false);


size_t replaceStr(std::wstring& str,
                         const std::wstring& search,
                         const std::wstring& replace,
                         bool global = true);


size_t replaceStrNoCase(std::wstring& str,
                        const std::wstring& search,
                        const std::wstring& replace);

double nolocale_atof(const char* num);

inline double nolocale_atof(const std::string& num)
{
    return nolocale_atof(num.c_str());
}

int wtoi(const wchar_t* s);
double wtof(const wchar_t* s);
double nolocale_wtof(const wchar_t* num);

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



// string conversion helper functions

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

void string2ucsle(unsigned char* dest,
                  const std::string& src,
                  int pad_len);

void wstring2ucsle(unsigned char* dest,
                   const std::wstring& src,
                   int pad_len);

void ucsle2wstring(std::wstring& dest,
                   const unsigned char* src,
                   int buf_len);

void ucsle2string(std::string& dest,
                  const unsigned char* src,
                  int buf_len);

void wstring2ucsbe(unsigned char* dest,
                   const std::wstring& src,
                   int pad_len);

void ucsbe2wstring(std::wstring& dest,
                   const unsigned char* src,
                   int buf_len);

bool is_unicode_string(const std::wstring& val);

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
