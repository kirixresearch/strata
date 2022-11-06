/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2013-05-22
 *
 */

#define _CRT_SECURE_NO_WARNINGS
#include "kl/portable.h"
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <string>

namespace kl
{

std::wstring Uint64ToHex(unsigned long long num)
{
    wchar_t buf[64];
    swprintf(buf, 63, L"%016llX", num);
    buf[63] = 0;
    return buf;
}

// wcsrev is not available on linux, so we'll use our own
static wchar_t* klwcsrev(wchar_t* s)
{
    wchar_t *p1, *p2;
    wchar_t tmp;

    if (!s || !*s)
        return s;

    for (p1 = s, p2 = s + wcslen(s) - 1; p2 > p1; ++p1, --p2)
    {
        tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
    }

    return s;
}

unsigned long long hexToUint64(const std::wstring& _code)
{
    static const wchar_t* hexchars = L"0123456789ABCDEF";

    wchar_t* code = wcsdup(_code.c_str());
    klwcsrev(code);

    unsigned long long retval = 0;
    unsigned long long multiplier = 1;

    wchar_t* p = code;
    while (*p)
    {
        wchar_t c = toupper(*p);
        p++;

        const wchar_t* pos = wcschr(hexchars, c);
        if (!pos)
        {
            free(code);
            return 0;
        }

        unsigned long long a = pos-hexchars;
        retval += (a*multiplier);
        multiplier *= 16;
    }

    free(code);

    return retval;
}




};


