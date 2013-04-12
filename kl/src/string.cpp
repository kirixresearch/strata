/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2012-08-04
 *
 */


#include "kl/string.h"
#include "kl/math.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstring>

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
    swprintf(buf, 80, L"%d", val);
    return buf;
}



std::string itostring(int val)
{
    char buf[80];
    snprintf(buf, 80, "%d", val);
    return buf;
}


std::string stdsprintf(const char* fmt, ...)
{
    char str[512];

    va_list args;
    va_start(args, fmt);
    vsnprintf(str, 512, fmt, args);
    va_end(args);

    return str;
}


std::wstring stdswprintf(const wchar_t* fmt, ...)
{
    wchar_t str[512];

    va_list args;
    va_start(args, fmt);
    vswprintf(str, 512, fmt, args);
    va_end(args);

    return str;
}

std::wstring formattedNumber(double d, int dec_places)
{
    // the first time we are run, get locale information about
    // the system's decimal point character and thousands
    // separator character

    static wchar_t thousands_sep = 0;
    static wchar_t decimal_point = 0;
    if (!decimal_point)
    {
        struct lconv* l = localeconv();
        thousands_sep = (unsigned char)*(l->thousands_sep);
        decimal_point = (unsigned char)*(l->decimal_point);

        if (thousands_sep == 0)
            thousands_sep = ',';
        if (decimal_point == 0)
            decimal_point = '.';
    }

    if (dec_places == -1)
    {
        // TODO: it'd be nice to sense the number of decimal places if dec == -1
        dec_places = 0;
    }

    // initialize result area
    wchar_t result[128];
    wchar_t* string_start;
    memset(result, 0, sizeof(wchar_t)*128);

    double decp, intp;
    int digit;
    bool negative = false;

    if (d < 0.0)
    {
        negative = true;
        d = fabs(d);
    }

    // split the number up into integer and fraction portions
    d = kl::dblround(d, dec_places);
    decp = modf(d, &intp);

    int i = 0;

    int pos = 40;
    while (1)
    {
        digit = (int)((modf(intp/10, &intp)+0.01)*10);

        result[pos] = L'0' + digit;
        pos--;

        if (intp < 1.0)
        {
            break;
        }

        if (++i == 3)
        {
            result[pos] = thousands_sep;
            pos--;
            i = 0;
        }
    }

    if (negative)
    {
        result[pos] = L'-';
        --pos;
    }


    string_start = result+pos+1;


    if (dec_places > 0)
    {
        pos = 41;
        result[pos] = decimal_point;
        pos++;
        swprintf(result+pos, 40, L"%0*.0f", dec_places, decp * kl::pow10(dec_places));
    }

    return string_start;
}



// this function is also used for creating unique strings;
// the current length of the strings this function returns
// is presently set at 32

std::wstring getUniqueString()
{
    int i;
    wchar_t temp[33];
    memset(temp, 0, 33 * sizeof(wchar_t));
    
    for (i = 0; i < 8; i++)
    {
        temp[i] = L'a' + (rand() % 26);
    }

    unsigned int t = (unsigned int)time(NULL);
    unsigned int div = 308915776;    // 26^6;
    for (i = 8; i < 15; i++)
    {
        temp[i] = L'a' + (t/div);
        t -= ((t/div)*div);
        div /= 26;
    }

    return temp;
}

size_t stringFrequency(const std::wstring& haystack, wchar_t needle)
{
    size_t cnt = 0, i, len = haystack.length();
    for (i = 0; i < len; ++i)
    {
        if (haystack[i] == needle)
            cnt++;
    }
    return cnt;
}

static wchar_t* zl_strchr(wchar_t* str,
                          wchar_t ch,
                          const wchar_t* open_parens = L"(",
                          const wchar_t* close_parens = L")")
{
    int paren_level = 0;
    wchar_t quote_char = 0;
    wchar_t* start = str;

    while (*str)
    {
        if (quote_char)
        {
            if (*str == quote_char)
            {            
                if (*(str+1) == quote_char)
                {
                    // double quote ex. "Test "" String";
                    str += 2;
                    continue;
                }

                quote_char = 0;
                str++;
                continue;
            }
        }
         else
        {
            if (*str == L'\'')
            {
                quote_char = L'\'';
            }
             else if (*str == L'"')
            {
                quote_char = L'\"';
            }

            if (open_parens && close_parens)
            {
                if (wcschr(open_parens, *str))
                    paren_level++;
                else if (wcschr(close_parens, *str))
                    paren_level--;
            }

            if (paren_level == 0 && *str == ch)
                return str;
        }
        
        str++;
    }

    return NULL;
}

std::wstring joinList(std::vector<std::wstring>& vec, const std::wstring join_str)
{
    std::wstring res;
    std::vector<std::wstring>::iterator it;

    for (it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin())
            res += join_str;
        res += *it;
    }

    return res;
}


void parseDelimitedList(const std::wstring& s,
                        std::vector<std::wstring>& vec,
                        wchar_t delimiter,
                        bool zero_level)
{
    const wchar_t* piece = s.c_str();
    const wchar_t* comma;

    while (1)
    {
        while (iswspace(*piece))
            piece++;

        if (zero_level)
        {
            comma = zl_strchr((wchar_t*)piece, delimiter);
        }
         else
        {
            comma = wcschr(piece, delimiter);
        }

        if (!comma)
        {
            std::wstring out = piece;
            kl::trimRight(out);
            vec.push_back(out);
            return;
        }
         else
        {
            std::wstring out(piece, comma-piece);
            kl::trimRight(out);
            vec.push_back(out);
            piece = comma+1;
        }
    }
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

