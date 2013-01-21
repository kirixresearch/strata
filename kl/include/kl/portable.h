/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-13
 *
 */


#ifndef __KL_PORTABLE_H
#define __KL_PORTABLE_H


#include <cstdio>
#include <ctime>


#ifdef __APPLE__
#include <wchar.h>
#include <cstring>
#include <cstdlib>
#endif


// API compatability defines

#ifdef _MSC_VER

#ifndef strcasecmp
#define strcasecmp stricmp
#endif

#ifndef strncasecmp
#define strncasecmp strnicmp
#endif

#ifndef wcscasecmp
#define wcscasecmp wcsicmp
#endif

#ifndef wcsncasecmp
#define wcsncasecmp wcsnicmp
#endif

#ifndef snprintf
#define snprintf _snprintf
#endif

#ifndef swprintf
#define swprintf _snwprintf
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif


// for all versions of MSVC less than 2008 an
#if _MSC_VER < 1600
#define max _cpp_max
#define min _cpp_min
#endif

#endif


#ifdef _MSC_VER
inline struct tm* localtime_r(const time_t* clock, struct tm* result)
{
#if _MSC_VER >= 1500
    localtime_s(result, clock);
#else
    // MSVC's localtime is thread-safe if compiling with the MT CRT
    *result = *localtime(clock);
#endif
    return result;
}
#endif



#ifdef __APPLE__

namespace kl
{


#define wcscasecmp kl::wcsCaseCmp
#define wcsncasecmp kl::wcsNCaseCmp
#define wcsdup kl::wcsDup

inline int wcsCaseCmp(const wchar_t* s1, const wchar_t* s2)
{
    wchar_t c1 = 0, c2 = 0;

    while (c1 == c2)
    {
        c1 = *s1++;
        c2 = *s2++;
        if (c1 == L'\0' || c2 == L'\0')
            break;
        if (c1 != c2)
        {
            c1 = towupper(c1);
            c2 = towupper(c2);
        }
    }

    return c1 - c2;
}

inline int wcsNCaseCmp(const wchar_t* s1, const wchar_t* s2, size_t n)
{
    wchar_t c1 = 0, c2 = 0;

    while (c1 == c2 && n > 0)
    {
        n--;
        c1 = *s1++;
        c2 = *s2++;
        if (c1 == L'\0' || c2 == L'\0')
            break;
        if (c1 != c2)
        {
            c1 = towupper(c1);
            c2 = towupper(c2);
        }
    }

    return c1 - c2;
}

inline wchar_t* wcsDup(const wchar_t* s)
{
    wchar_t* ret = (wchar_t*)malloc((wcslen(s)+1)*sizeof(wchar_t));
    wcscpy(ret, s);
    return ret;
}
}


#endif




#endif      // __KL_PORTABLE_H




