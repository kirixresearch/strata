/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-10
 *
 */


#ifndef __KSCRIPT_UTIL_H
#define __KSCRIPT_UTIL_H


#include <cwctype>
#include <ctime>
#include <cmath>
#include <cfloat>
#include <algorithm>


namespace kscript
{


#ifdef _MSC_VER
#define isnan _isnan
#define isinf(d) (0 == _finite(d))
#endif


#ifdef __APPLE__
#define isnan std::isnan
#define isinf std::isinf
#endif


double nolocale_wtof(const wchar_t* num);

std::wstring xtrim(const std::wstring& var);


int dblcompare(double x1, double x2);
int dblcompare_strict(double x1, double x2);


std::wstring dbltostr(double d);

bool parseHexOctConstant(const wchar_t* ch, int base, Value* retval, const wchar_t** offending);
bool parseDecimalConstant(const wchar_t* ch, Value* retval, const wchar_t** offending);
bool parseNumericConstant(const wchar_t* ch, Value* retval, const wchar_t** offending, bool allow_octal = false);

wchar_t jsToLower(wchar_t ch);
wchar_t jsToUpper(wchar_t ch);


inline bool isWhiteSpaceOrLS(wchar_t ch)
{
    if (ch < 0x80)
        return (ch == ' ' || ch == 0x0d || ch == 0x0a || ch == 0x09 || ch == 0x0b || ch == 0x0c);
         else
        return (ch == 0xa0 || (ch >= 0x2000 && ch <= 0x200b) || ch == 0x202f || ch == 0x205f || ch == 0x3000 || iswspace(ch));
}

inline void skipWhiteSpaceOrLS(wchar_t*& ch)
{
    if (!ch) return;
    while (isWhiteSpaceOrLS(*ch))
        ++ch;
}

inline bool isWhiteSpace(wchar_t ch)
{
    if (ch < 0x80)
        return (ch == ' ' || ch == 0x09 || ch == 0x0b || ch == 0x0c);
         else
        return (ch == 0xa0 || (ch >= 0x2000 && ch <= 0x200b) || ch == 0x202f || ch == 0x205f || ch == 0x3000 || iswspace(ch));
}

inline bool isLineSeparator(wchar_t ch)
{
    if (ch == 0x0d || ch == 0x0a || ch == 0x2028 || ch == 0x2029)
        return true;
    return false;
}


struct SetCharOnExitScope
{
    SetCharOnExitScope() { ptr = NULL; }
    SetCharOnExitScope(wchar_t* p, wchar_t ch_onexit) { ptr = p; ch = ch_onexit; }
    SetCharOnExitScope(wchar_t* p, wchar_t ch_onexit, wchar_t ch_now)
    {
        ptr = p;
        ch = ch_onexit;
        if (ptr)
            *ptr = ch_now;
    }
    ~SetCharOnExitScope() { if (ptr) *ptr = ch; }
    wchar_t* ptr;
    wchar_t ch;
};

bool isValidIdentifierFirstChar(wchar_t ch);
bool isValidIdentifierChar(wchar_t ch);
bool isValidIdentifier(const std::wstring& identifier);

bool isBigEndian();
int getDoubleSign(double* d);

inline double pow10(int r)
{
    static double d[21] = { 1.0e0,  1.0e1,  1.0e2,  1.0e3,  1.0e4,  1.0e5,  1.0e6,  1.0e7,
                            1.0e8,  1.0e9,  1.0e10, 1.0e11, 1.0e12, 1.0e13, 1.0e14, 1.0e15,
                            1.0e16, 1.0e17, 1.0e18, 1.0e19, 1.0e20 };

    if (r < 0 || r > 20)
        return pow((double)10.0, r);
    
    return d[r];
}

inline double dblround(double d, int dec)
{
    double p = pow10(dec);
    d = d * p;
    d = d + 0.5;
    d = floor(d);
    d = d / p;
    return d;
}

inline int doubleToInt32(double d)
{
    // implements ECMA-262 section 9.5
    if (d == 0.0 || isnan(d) || isinf(d))
    {
        return 0;
    }
    
    if (d >= 0.0)
        d = floor(d);
         else
        d = -floor(fabs(d));
        
    return (int)(unsigned int)d;
}


// date routines

inline int dateToJulian(int year, int month, int day)
{
    int a, y, m;

    a = (14-month)/12;
    y = year + 4800 - a;
    m = month + (12 * a) - 3;
    return (day + (((153*m)+2)/5) + (365*y) + (y/4) - (y/100) + (y/400) - 32045);
}

inline void julianToDate(int julian, int* year, int* month, int* day)
{
    int a, b, c, d, e, f;

    a = julian + 32044;
    b = ((4*a)+3)/146097;
    c = a - ((146097*b) / 4);
    d = ((4*c)+3)/1461;
    e = c - ((1461*d))/4;
    f = ((5*e)+2)/153;

    *day = e - (((153*f)+2)/5) + 1;
    *month = f + 3 - (12 * (f/10));
    *year = (100*b) + d - 4800 + (f/10);
}

inline bool isLeapYear( int year )
{
    if ((year % 4 == 0) && (year % 100 != 0) || (year % 400 == 0))
    {
        return true;
    }

    return false;
}

inline int getDayOfWeek(int year, int month, int day)
{
    int a, y, m;

    a = (14-month)/12;
    y = year - a;
    m = month + (12*a) - 2;
    return ((day + y + (y/4) - (y/100) + (y/400) + ((31*m)/12)) % 7);
}


// string routines

inline std::wstring towstring(const std::string& src)
{
    std::wstring res;
    size_t i, src_len = src.length();

    res.resize(src_len);
    for (i = 0; i < src_len; ++i)
        res[i] = (unsigned char)src[i];

    return res;
}

inline std::string tostring(const std::wstring& src)
{
    std::string res;
    size_t i, src_len = src.length();

    res.resize(src_len);
    for (i = 0; i < src_len; ++i)
        res[i] = (char)src[i];

    return res;
}


inline int wtoi(const wchar_t* s)
{
    char buf[64];
    size_t i = 0;

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
    size_t i = 0;

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


inline void makeLower(std::wstring& s)
{
    std::transform(s.begin(), s.end(), s.begin(), towlower);
}
 



};


#endif

