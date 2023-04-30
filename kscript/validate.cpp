/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2001-11-06
 *
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include "kscript.h"
#include <cctype>
#include <cwctype>

#ifdef _MSC_VER
#define wcsdup _wcsdup
#endif

#ifdef __APPLE__
#include "../kl/include/kl/portable.h"
#include "../kl/include/kl/string.h"
#define isnan std::isnan
#endif




namespace kscript
{


const int Value_AnyNumber = 100;


static int _typeFromChar(wchar_t ctype)
{
    switch (ctype)
    {
        case L's': return Value::typeString;
        case L'i': return Value::typeInteger;
        case L'f': return Value::typeDouble;
        case L'n': return Value_AnyNumber;
        case L'b': return Value::typeBoolean;
        case L'd': return Value::typeDateTime;
        case L'x': return Value::typeBinary;
        case L'*': return Value::typeUndefined;
    }
    return Value::typeNull;
}



bool _getValueTypesCompatible(int type1, int type2)
{
    if (type1 == type2)
        return true;

    switch (type1)
    {
        case Value_AnyNumber:
        case Value::typeDouble:
        case Value::typeInteger:

            if (type2 == Value::typeDouble ||
                type2 == Value::typeInteger)
            {
                return true;
            }
            break;
    }


    switch (type2)
    {
        case Value_AnyNumber:
        case Value::typeDouble:
        case Value::typeInteger:

            if (type1 == Value::typeDouble ||
                type1 == Value::typeInteger)
            {
                return true;
            }
            break;
    }


    return false;
}



static int _getSingleRetValType(const wchar_t* format,
                                ExprElement* elements[],
                                int element_count)
{
    // prototype variables

    int pttypes[255];
    int ptcount;
    int ptretval;
    int opt_offset = -1;
    int ptvarparam = Value::typeNull;

    ptretval = _typeFromChar(*format);
    
    if (ptretval == Value::typeNull)
        return Value::typeNull;
    
    format++;

    if (*format != L'(')
        return Value::typeNull;

    format++;

    // ... means variable params
    if (wcsncmp(format, L"...", 3) == 0)
        return ptretval;


    ptcount = 0;
    while (*format != L')')
    {
        if (!*format)
            return Value::typeNull;

        if (*format == L'[')
        {
            opt_offset = ptcount;
            format++;
            continue;
        }

        if (*format == L']')
        {
            format++;
            continue;
        }

        pttypes[ptcount] = _typeFromChar(*format);
        if (pttypes[ptcount] == Value::typeNull)
            return Value::typeNull;
        ptcount++;
        format++;
    }

    format++;
    if (*format)
    {
        ptvarparam = _typeFromChar(*format);
        if (ptvarparam == Value::typeNull)
            return Value::typeNull;
    }

    // ok, now that we have parsed the prototype, we can see if the
    // parameters passed match up to the prototype

    if (ptvarparam == Value::typeNull)
    {
        // check to see if we have too many parameters
        if (element_count > ptcount)
        {
            return Value::typeNull;
        }

        if (opt_offset == -1)
        {
            // if there are no optional parameters, the param count
            // must match up exactly

            if (ptcount != element_count)
                return Value::typeNull;
        }
         else
        {
            // check to see if there are at least the required number
            // of obligitory parameters

            if (element_count < opt_offset)
                return Value::typeNull;
        }
    }
     else
    {
        // there is a variable number of parameters, but we must
        // make sure that there are _at least_ the number of required
        // parameters
        if (element_count < ptcount)
            return Value::typeNull;
    }
    
    int etype;

    for (int i = 0; i < element_count; i++)
    {
        etype = Value::typeNull;
        if (elements[i])
        {
            etype = elements[i]->getType();
        }

        if (i < ptcount)
        {
            if (pttypes[i] == Value::typeUndefined)
                continue;
            if (etype == Value::typeUndefined || etype == Value::typeNull)
                continue;
            if (_getValueTypesCompatible(etype, pttypes[i]))
                continue;
            return Value::typeNull;
        }
         else
        {
            if (ptvarparam == Value::typeNull)
                return Value::typeNull;
            if (!_getValueTypesCompatible(etype, ptvarparam))
                return Value::typeNull;
        }
    }

    // everything checked out ok
    return ptretval;
}


// this function returns 0 if parameters were not compatible with
// the format definition.  Otherwise, it returns the return value
// type.

int getReturnValType(const std::wstring& format,
                     ExprElement* elements[],
                     int element_count)
{
    wchar_t* str = wcsdup(format.c_str());
    wchar_t* start = str;
    wchar_t* s;
    wchar_t tempc;
    int val;

    while (1)
    {
        while (iswspace(*start))
            start++;

        s = wcschr(start, L';');
        if (!s)
            s = start+wcslen(start);

        if (s == start)
            break;

        tempc = *s;
        *s = 0;
         val = _getSingleRetValType(start, elements, element_count);
        *s = tempc;

        if (val)
        {
            free(str);
            return val;
        }
        if (!*s)
        {
            break;
        }
    
        start = s+1;
    }

    free(str);
    return 0;
}



};

