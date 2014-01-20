/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-03-17
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <kl/file.h>
#include <kl/string.h>
#include <kl/portable.h>
#include <kl/math.h>
#include <set>
#include <xd/xd.h>
#include "util.h"
#include "../xdcommon/util.h"
#include "../../kscript/kscript.h"


std::wstring combineOfsPath(const std::wstring& base_folder,
                            const std::wstring& file_name)
{
    std::wstring result;
    result = base_folder;

    if (result.empty() || result[result.length()-1] != L'/')
    {
        result += L'/';
    }


    const wchar_t* f = file_name.c_str();
    while (*f == L'/')
        f++;

    result += f;

    return result;
}



double decstr2dbl(const char* c, int width, int scale)
{
    double res = 0;
    double d = kl::pow10(width-scale-1);
    bool neg = false;
    while (width)
    {
        if (*c == '-')
            neg = true;

        if (*c >= '0' && *c <= '9')
        {
            res += d * (*c - '0');
        }

        d /= 10;
        c++;
        width--;
    }

    if (neg)
    {
        res *= -1.0;
    }

    return res;
}


void dbl2decstr(char* dest, double d, int width, int scale)
{
    double intpart;

    // check for negative
    if (d < 0.0)
    {
        *dest = '-';
        dest++;
        width--;
        d = fabs(d);
    }

    // rounding
    d += (0.5/kl::pow10(scale));

    // put everything to the right of the decimal
    d /= kl::pow10(width-scale);

    while (width)
    {
        d *= 10;
        d = modf(d, &intpart);
        if (intpart > 9.1)
            intpart = 0.0;

        *dest = int(intpart) + '0';
        dest++;
        width--;
    }
}



// conversion between set id's and table ord's

std::wstring getTableSetId(xd::tableord_t table_ordinal)
{
    wchar_t buf[64];
    swprintf(buf, 64, L"tbl%08u", table_ordinal);
    return buf;
}

