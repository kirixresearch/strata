/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Security Library
 * Author:   Benjamin I. Williams
 * Created:  2003-04-30
 *
 */


#ifndef __PALADIN_UTIL_H
#define __PALADIN_UTIL_H


#include <cstdio>
#include <cctype>
#include <cwctype>
#include <cstring>
#include <cstdlib>


#include "crypt.h"


#ifdef _MSC_VER
#define PAL_INLINE __forceinline
#else
#define PAL_INLINE inline
#endif


namespace paladin
{


PAL_INLINE int dateToJulian(int year, int month, int day)
{
    int a, y, m;

    a = (14-month)/12;
    y = year + 4800 - a;
    m = month + (12 * a) - 3;
    return (day + (((153*m)+2)/5) + (365*y) + (y/4) - (y/100) + (y/400) - 32045);
}


PAL_INLINE void int64unpack(unsigned char* bytes, const paladin_int64_t& i)
{
    bytes[0] = (unsigned char)((i) & 0xff);
    bytes[1] = (unsigned char)((i >> 8) & 0xff);
    bytes[2] = (unsigned char)((i >> 16) & 0xff);
    bytes[3] = (unsigned char)((i >> 24) & 0xff);
    bytes[4] = (unsigned char)((i >> 32) & 0xff);
    bytes[5] = (unsigned char)((i >> 40) & 0xff);
    bytes[6] = (unsigned char)((i >> 48) & 0xff);
    bytes[7] = (unsigned char)((i >> 56) & 0xff);
}

PAL_INLINE void int64pack(paladin_int64_t& i, const unsigned char* bytes)
{
    paladin_int64_t tempv;
    tempv = bytes[0];
    i = tempv;
    tempv = bytes[1];
    i |= (tempv << 8);
    tempv = bytes[2];
    i |= (tempv << 16);
    tempv = bytes[3];
    i |= (tempv << 24);
    tempv = bytes[4];
    i |= (tempv << 32);
    tempv = bytes[5];
    i |= (tempv << 40);
    tempv = bytes[6];
    i |= (tempv << 48);
    tempv = bytes[7];
    i |= (tempv << 56);
}


PAL_INLINE void rot13(char* s)
{
    while (*s)
    {
        if (!isalpha(*s))
        {
            ++s;
            continue;
        }

        int offset = isupper(*s) ? 'A' : 'a';
        *s = offset + ((*s - offset + 13) % 26);
        ++s;
    }
}


PAL_INLINE void rot13(wchar_t* s)
{
    while (*s)
    {
        if (!iswalpha(*s))
        {
            ++s;
            continue;
        }

        int offset = iswupper(*s) ? L'A' : L'a';
        *s = offset + ((*s - offset + 13) % 26);
        ++s;
    }
}


PAL_INLINE void reverse_string(char* str)
{
    int inlen = strlen(str);
    char* temps = new char[inlen+1];
    char* c = str+inlen-1;
    int i = 0;
    while (c >= str)
    {
        *(temps+i) = *c;
        i++;
        c--;
    }
    *(temps+i) = 0;

    strcpy(str, temps);
    delete[] temps;
}


PAL_INLINE paladin_int64_t getCodeFromString(const char* _code)
{
    static const char* hexchars = "0123456789ABCDEF";

    char* act_code = strdup(_code);
    reverse_string(act_code);

    paladin::actcode_t retval = 0;
    paladin::actcode_t multiplier = 1;

    char* p = act_code;
    while (*p)
    {
        char c = toupper(*p);
        p++;

        if (isspace(c) || c == '-' || c == '.')
            continue;

        const char* pos = strchr(hexchars, c);
        if (!pos)
        {
            return 0;
        }

        paladin::actcode_t a = pos-hexchars;
        retval += (a*multiplier);
        multiplier *= 16;
    }

    free(act_code);

    return retval;
}


PAL_INLINE void getStringFromCode(const paladin_int64_t& actcode,
                                  char* output_string,
                                  bool format = true)
{
    unsigned int i0 = (unsigned int)((actcode >> 56) & 0xff);
    unsigned int i1 = (unsigned int)((actcode >> 48) & 0xff);
    unsigned int i2 = (unsigned int)((actcode >> 40) & 0xff);
    unsigned int i3 = (unsigned int)((actcode >> 32) & 0xff);
    unsigned int i4 = (unsigned int)((actcode >> 24) & 0xff);
    unsigned int i5 = (unsigned int)((actcode >> 16) & 0xff);
    unsigned int i6 = (unsigned int)((actcode >> 8) & 0xff);
    unsigned int i7 = (unsigned int)((actcode) & 0xff);

    if (format)
    {
        sprintf(output_string, "%02X%02X %02X%02X %02X%02X %02X%02X",
                i0, i1, i2, i3, i4, i5, i6, i7);
    }
     else
    {
        sprintf(output_string, "%02X%02X%02X%02X%02X%02X%02X%02X",
                i0, i1, i2, i3, i4, i5, i6, i7);
    }
}



PAL_INLINE void int64crypt(paladin::paladin_int64_t& i, const unsigned char* key, bool encrypt)
{
    unsigned char bytes[8];
    bytes[0] = (unsigned char)((i) & 0xff);
    bytes[1] = (unsigned char)((i >> 8) & 0xff);
    bytes[2] = (unsigned char)((i >> 16) & 0xff);
    bytes[3] = (unsigned char)((i >> 24) & 0xff);
    bytes[4] = (unsigned char)((i >> 32) & 0xff);
    bytes[5] = (unsigned char)((i >> 40) & 0xff);
    bytes[6] = (unsigned char)((i >> 48) & 0xff);
    bytes[7] = (unsigned char)((i >> 56) & 0xff);

    Des d;
    d.setKey(key);
    d.crypt(bytes, 8, encrypt);

    i = 0;

    paladin::paladin_int64_t tempv;
    tempv = bytes[0];
    i |= tempv;
    tempv = bytes[1];
    i |= (tempv << 8);
    tempv = bytes[2];
    i |= (tempv << 16);
    tempv = bytes[3];
    i |= (tempv << 24);
    tempv = bytes[4];
    i |= (tempv << 32);
    tempv = bytes[5];
    i |= (tempv << 40);
    tempv = bytes[6];
    i |= (tempv << 48);
    tempv = bytes[7];
    i |= (tempv << 56);
}



};


#endif
