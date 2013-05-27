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
    swprintf(buf, 64, L"%016llX", num);
    return buf;
}

unsigned long long hexToUint64(const std::wstring& _code)
{
    static const wchar_t* hexchars = L"0123456789ABCDEF";

    wchar_t* code = wcsdup(_code.c_str());
    wcsrev(code);

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


