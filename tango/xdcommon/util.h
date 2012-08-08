/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-09-22
 *
 */


#ifndef __XDCOMMON_UTIL_H
#define __XDCOMMON_UTIL_H


// -- forward declarations --

namespace kscript
{
    class ExprParser;
};


// -- platform definitions --

#ifdef WIN32
#define PATH_SEPARATOR_CHAR    L'\\'
#define PATH_SEPARATOR_STR     L"\\"
#else
#define PATH_SEPARATOR_CHAR    L'/'
#define PATH_SEPARATOR_STR     L"/"
#endif



// -- string and conversion functions --

wchar_t* zl_strblank(wchar_t* str);
wchar_t* zl_strchr(
             wchar_t* str,
             wchar_t ch,
             const wchar_t* open_parens = L"(",
             const wchar_t* close_parens = L")");
wchar_t* zl_stristr(wchar_t* str,
                    const wchar_t* search_str,
                    bool single_word = false,
                    bool forward = true);




// -- utility functions --

std::wstring getUniqueString();

void doubleQuoteCopy(std::wstring& output,
                    const wchar_t* input,
                    int input_length);

void doubleQuoteCopy(std::wstring& output, 
                    const char* input,
                    int input_length);

void requote(std::wstring& str, wchar_t ch1, wchar_t ch2,
                                wchar_t new_ch1, wchar_t new_ch2);

void dequote(std::wstring& str, wchar_t ch1, wchar_t ch2);

std::wstring makePathName(const std::wstring& base_dir,
                          const std::wstring& subdir,
                          const std::wstring& filename = L"",
                          const std::wstring& ext = L"");

std::wstring getTablenameFromOfsPath(const std::wstring& path);
std::wstring getTablenameFromFilesystemPath(const std::wstring& path);

void crc64(const unsigned char* s,
           int length,
           tango::tango_uint64_t* result);

void bindExprParser(kscript::ExprParser* parser);
kscript::ExprParser* createExprParser();

int kscript2tangoType(int type);
int tango2kscriptType(int type);

tango::IIndexInfoPtr lookupIndex(tango::IIndexInfoEnumPtr enums,
                                 const std::wstring& expr,
                                 bool exact_column_order);

bool parseDateTime(const std::wstring& input,
                   int* year,
                   int* month,
                   int* day,
                   int* hour,
                   int* minute,
                   int* second);

tango::datetime_t str2datetime(const char* str,
                               const char* fmt = "YMDhms");

tango::tango_uint64_t hex2uint64(const wchar_t* _code);


tango::rowid_t bufToRowid(unsigned char* buf);
void rowidToBuf(unsigned char* bytes, tango::rowid_t r);

int levenshtein(const wchar_t* s, const wchar_t* t);

// -- inline utilities --


inline int dateToJulian(int year, int month, int day)
{
int a, y, m;

    a = (14-month)/12;
    y = year + 4800 - a;
    m = month + (12 * a) - 3;
    return (day + (((153*m)+2)/5) + (365*y) +
            (y/4) - (y/100) + (y/400) - 32045);
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

inline void setTimeStamp(int time_stamp, int* hour,
                                         int* minute,
                                         int* second,
                                         int* millisecond)
{
    time_stamp++;

    *hour = time_stamp / 3600000;
    time_stamp -= (*hour * 3600000);

    *minute = time_stamp / 60000;
    time_stamp -= (*minute * 60000);

    *second = time_stamp / 1000;
    time_stamp -= (*second * 1000);

    *millisecond = time_stamp;
}
    
inline int getTimeStamp(int hour,
                        int minute,
                        int second,
                        int millisecond)
{
    int time_stamp = 0;
    time_stamp += (hour * 3600000);
    time_stamp += (minute * 60000);
    time_stamp += (second * 1000);
    time_stamp += millisecond;
    return time_stamp;
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




// -- integer <-> buffer conversions --

inline void int2buf(unsigned char* buf, unsigned int i)
{
    *(buf)   = (i & 0x000000ff);
    *(buf+1) = (i & 0x0000ff00) >> 8;
    *(buf+2) = (i & 0x00ff0000) >> 16;
    *(buf+3) = (i & 0xff000000) >> 24;
}

inline unsigned int buf2int(const unsigned char* buf)
{
#ifdef WIN32
    return *((unsigned int*)buf);
#else
    return buf[0] + (buf[1]*256) + (buf[2]*65536) + (buf[3] * 16777216);
#endif
}


inline tango::tango_int64_t bufToInt64(unsigned char* buf)
{
#ifdef WIN32
    return *((tango::tango_int64_t*)buf);
#else
    tango::tango_int64_t result, tempv;
    tempv = buf[0];
    result = tempv;
    tempv = buf[1];
    result |= (tempv << 8);
    tempv = buf[2];
    result |= (tempv << 16);
    tempv = buf[3];
    result |= (tempv << 24);
    tempv = buf[4];
    result |= (tempv << 32);
    tempv = buf[5];
    result |= (tempv << 40);
    tempv = buf[6];
    result |= (tempv << 48);
    tempv = buf[7];
    result |= (tempv << 56);
    return result;
#endif
}

inline void int64ToBuf(unsigned char* bytes,
                       tango::tango_int64_t i)
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


inline void invert_rowid_endianness(unsigned char* out, const unsigned char* in)
{
    out[0] = in[3];
    out[1] = in[2];
    out[2] = in[1];
    out[3] = in[0];

    out[4] = in[7];
    out[5] = in[6];
    out[6] = in[5];
    out[7] = in[4];
}


// -- helper class to delete ptr after scope exits --

template <class T>
class DelArrPtr
{
public:
    T* ptr;
    DelArrPtr(T* t) : ptr(t) { }
    ~DelArrPtr() { delete[] ptr; };
};


// -- type stuff --

inline bool isTypeCompatible(int type1, int type2)
{
    if (type1 == type2)
        return true;

    switch (type1)
    {
        case tango::typeCharacter:
        case tango::typeWideCharacter:
            if (type2 == tango::typeCharacter ||
                type2 == tango::typeWideCharacter)
            {
                return true;
            }
            break;

        case tango::typeNumeric:
        case tango::typeDouble:
        case tango::typeInteger:
            if (type2 == tango::typeNumeric ||
                type2 == tango::typeDouble ||
                type2 == tango::typeInteger)
            {
                return true;
            }
            break;

        case tango::typeDate:
        case tango::typeDateTime:
            if (type2 == tango::typeDate ||
                type2 == tango::typeDateTime)
            {
                return true;
            }
            break;
    }

    return false;
}


// -- character-encoding conversion functions --


inline unsigned char ascii2ebcdic(unsigned char c)
{
    static unsigned char a2e[256] =
    {
          0,  1,  2,  3, 55, 45, 46, 47, 22,  5, 37, 11, 12, 13, 14, 15,
         16, 17, 18, 19, 60, 61, 50, 38, 24, 25, 63, 39, 28, 29, 30, 31,
         64, 79,127,123, 91,108, 80,125, 77, 93, 92, 78,107, 96, 75, 97,
        240,241,242,243,244,245,246,247,248,249,122, 94, 76,126,110,111,
        124,193,194,195,196,197,198,199,200,201,209,210,211,212,213,214,
        215,216,217,226,227,228,229,230,231,232,233, 74,224, 90, 95,109,
        121,129,130,131,132,133,134,135,136,137,145,146,147,148,149,150,
        151,152,153,162,163,164,165,166,167,168,169,192,106,208,161,  7,
         32, 33, 34, 35, 36, 21,  6, 23, 40, 41, 42, 43, 44,  9, 10, 27,
         48, 49, 26, 51, 52, 53, 54,  8, 56, 57, 58, 59,  4, 20, 62,225,
         65, 66, 67, 68, 69, 70, 71, 72, 73, 81, 82, 83, 84, 85, 86, 87,
         88, 89, 98, 99,100,101,102,103,104,105,112,113,114,115,116,117,
        118,119,120,128,138,139,140,141,142,143,144,154,155,156,157,158,
        159,160,170,171,172,173,174,175,176,177,178,179,180,181,182,183,
        184,185,186,187,188,189,190,191,202,203,204,205,206,207,218,219,
        220,221,222,223,234,235,236,237,238,239,250,251,252,253,254,255
    };

    return a2e[c];
}


inline unsigned char ebcdic2ascii(unsigned char c)
{
    static unsigned char e2a[256] =
    {
          0,  1,  2,  3,156,  9,134,127,151,141,142, 11, 12, 13, 14, 15,
         16, 17, 18, 19,157,133,  8,135, 24, 25,146,143, 28, 29, 30, 31,
        128,129,130,131,132, 10, 23, 27,136,137,138,139,140,  5,  6,  7,
        144,145, 22,147,148,149,150,  4,152,153,154,155, 20, 21,158, 26,
         32,160,161,162,163,164,165,166,167,168, 91, 46, 60, 40, 43, 33,
         38,169,170,171,172,173,174,175,176,177, 93, 36, 42, 41, 59, 94,
         45, 47,178,179,180,181,182,183,184,185,124, 44, 37, 95, 62, 63,
        186,187,188,189,190,191,192,193,194, 96, 58, 35, 64, 39, 61, 34,
        195, 97, 98, 99,100,101,102,103,104,105,196,197,198,199,200,201,
        202,106,107,108,109,110,111,112,113,114,203,204,205,206,207,208,
        209,126,115,116,117,118,119,120,121,122,210,211,212,213,214,215,
        216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,
        123, 65, 66, 67, 68, 69, 70, 71, 72, 73,232,233,234,235,236,237,
        125, 74, 75, 76, 77, 78, 79, 80, 81, 82,238,239,240,241,242,243,
         92,159, 83, 84, 85, 86, 87, 88, 89, 90,244,245,246,247,248,249,
         48, 49, 50, 51, 52, 53, 54, 55, 56, 57,250,251,252,253,254,255
    };

    return e2a[c];
}


#endif
