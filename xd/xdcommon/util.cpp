/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-09-22
 *
 */



#include <vector>
#include <string>
#include <ctime>
#include <kl/xcm.h>
#include <kl/portable.h>
#include <kl/string.h>
#include <xd/xd.h>
#include "util.h"
#include "../../kscript/kscript.h"
#include "../../kscript/functions.h"
#include "exprfuncs.h"



void doubleQuoteCopy(std::wstring& output,
                     const wchar_t* input,
                     int input_length)
{
    output.reserve((input_length*2)+10);
    int i;


    while (input_length > 0 && iswspace(*(input+input_length-1)))
    {
        input_length--;
    }


    output = L"'";

    for (i = 0; i < input_length; ++i)
    {
        if (!*(input+i))
            break;

        output += *(input+i);

        if (*(input+i) == L'\'')
        {
            output += L'\'';
        }
    }

    output += L'\'';
}

void doubleQuoteCopy(std::wstring& output, const char* input, int input_length)
{
    output.reserve((input_length*2)+10);
    int i;


    while (input_length > 0 && isspace((unsigned char)*(input+input_length-1)))
    {
        input_length--;
    }


    output = L"'";

    for (i = 0; i < input_length; ++i)
    {
        if (!*(input+i))
            break;

        output += *(input+i);

        if (*(input+i) == L'\'')
        {
            output += L'\'';
        }
    }

    output += L'\'';
}

void requote(std::wstring& str, wchar_t ch1, wchar_t ch2,
                                wchar_t new_ch1, wchar_t new_ch2)
{
    const wchar_t* expr = str.c_str();
    wchar_t* start = (wchar_t*)expr;
    wchar_t* ch = NULL;
    int nesting_level = 0;

    while (1)
    {
        wchar_t search_str = ch1;
        if (nesting_level > 0)
            search_str = ch2;
    
        ch = zl_strchr((wchar_t*)expr, search_str, L"", L"");

        if (!ch)
            break;

        // if we're on the opening quote character, replace it
        // with new_char1; we have to check the nesting level 
        // because the opening and closing quote characters
        // might be the same, such as with "`", in which case
        // the only way to distinguish between the opening
        // and closing quote is the nesting level

        if (*ch == ch1 && nesting_level == 0)
        {
            str[ch - start] = new_ch1;
            nesting_level++;
        }

        if (*ch == ch2 && nesting_level == 1)
        {
            str[ch - start] = new_ch2;
            nesting_level--;
        }

        expr = ch + 1;
    }
}

void dequote(std::wstring& str, wchar_t ch1, wchar_t ch2)
{
    size_t len = str.length();
    if (len > 1)
    {
        if (str[0] == ch1 && str[len-1] == ch2)
        {
            str.erase(len-1, 1);
            str.erase(0, 1);
        }
    }
}

void dequoteIfField(const xd::Structure& structure, std::wstring& str, wchar_t ch1, wchar_t ch2)
{
    size_t len = str.length();
    if (len > 1)
    {
        if (str[0] == ch1 && str[len-1] == ch2)
        {
            std::wstring deq = str;
            dequote(deq, ch1, ch2);
            if (structure.getColumnExist(deq))
            {
                str = deq;
            }
        }
    }
}



bool parseDateTime(const std::wstring& input,
                   int* year,
                   int* month,
                   int* day,
                   int* hour,
                   int* minute,
                   int* second)
{
    if (year)
        *year = -1;
    if (month)
        *month = -1;
    if (day)
        *day = -1;
    if (hour)
        *hour = -1;
    if (minute)
        *minute = -1;
    if (second)
        *second = -1;

    std::vector<int> parts;

    // parse out the parts
    wchar_t part[80];
    int idx = 0;

    part[0] = 0;

    const wchar_t* p = input.c_str();
    while (1)
    {
        if (*p == 0 || wcschr(L"/-.: ", *p))
        {
            part[idx] = 0;
            if (wcslen(part) > 0)
                parts.push_back(kl::wtoi(part));
            
            if (!*p)
                break;

            part[0] = 0;
            idx = 0;
            ++p;
            continue;
        }

        part[idx] = *p;
        ++idx;
        ++p;
    }

    // extract information
    int part_count = parts.size();
    if (part_count < 3)
    {
        // if we only have one part, and we have 8 digits,
        // assume form of YYYYMMDD without delimiters,
        // otherwise, we're done
        if (part_count != 1 || input.length() != 8)
            return false;

        parts.clear();
        parts.push_back(kl::wtoi(input.substr(0,4)));
        parts.push_back(kl::wtoi(input.substr(4,2)));
        parts.push_back(kl::wtoi(input.substr(6,2)));
    }
    
    // try dates of the form param1/param2/param3, where the 
    // three parameters are interpreted as follows:
    //     1) MM/DD/YYYY:  0 <= param1 < 70 and param3 >= 100
    //     2) MM/DD/YY:    0 <= param1 < 70 and param3 < 100
    //     3) YY/MM/DD:    70 <= param1 < 100 and param2 < 70
    //     4) YYYY/MM/DD:  param1 >= 100 and param2 < 70
    // other combinations are invalid;
    // note: parallels logic in kscript parseDate()

    int y = 0, m = 0, d = 0, hh = -1, mm = -1, ss = 0;
    
    int p1, p2, p3;
    p1 = parts[0];
    p2 = parts[1];
    p3 = parts[2];

    // MM/DD/YYYY:  0 <= param1 < 70 and param3 >= 100
    if (p1 >= 0 && p1 < 70 && p3 >= 100)
    {
        y = p3;
        m = p1;
        d = p2;
    }

    // MM/DD/YY:    0 <= param1 < 70 and param3 < 100
    if (p1 >= 0 && p1 < 70 && p3 < 100)
    {
        y = p3 + 1900;
        m = p1;
        d = p2;
    }

    // YY/MM/DD:    70 <= param1 < 100 and param2 < 70
    if (p1 >= 70 && p1 < 100 && p2 < 70)
    {
        y = p1 + 1900;
        m = p2;
        d = p3;
    }

    // YYYY/MM/DD:  param1 >= 100 and param2 < 70
    if (p1 >= 100 && p2 < 70)
    {
        y = p1;
        m = p2;
        d = p3;
    }

    // handle additional parts
    if (part_count == 4)
        return false;

    if (part_count >= 5)
    {
        // time is included
        hh = parts[3];
        mm = parts[4];

        if (part_count > 5)
            ss = parts[5];
    }

    // verify month the month and the days in the month
    if (m <= 0 || m > 12)
        return false;

    int month_days[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (d <= 0 || d > month_days[m-1])
        return false;

    if (part_count >= 5)
    {
        if (hh == 24)
            hh = 0;

        if (hh < 0 || hh >= 24)
            return false;

        if (m < 0 || mm >= 60)
            return false;
        
        if (part_count > 5)
        {
            if (ss < 0 || ss >= 60)
                return false;
        }
    }

    // return the date info
    if (year)
        *year = y;
    if (month)
        *month = m;
    if (day)
        *day = d;
    if (hour)
        *hour = hh;
    if (minute)
        *minute = mm;
    if (second)
        *second = ss;

    return true;
}



xd::datetime_t str2datetime(const char* str, const char* fmt)
{
    const char* p;
    const char* d;
    char piece[255];
    bool last;

    int arr[7];
    int arr_size = 0;
    int Y,M,D,h,m,s,ms;

    // find next delimiter
    p = str;
    last = false;
    while (1)
    {
        d = strpbrk(p, "/-.: ");
        if (d)
        {
            strncpy(piece, p, d-p);
            piece[d-p] = 0;
            p = d+1;
        }
         else
        {
            strcpy(piece, p);
            last = true;
        }

        arr[arr_size] = atoi(piece);
        arr_size++;

        // if this is the last value, we are done
        if (last)
            break;

        // if we've read 7 values, we are done
        if (arr_size == 7)
            break;
    }

    // assign the appropriate values from the array
    // based on the format string

    p = fmt;
    while (*p)
    {
        // we do not have this many values in the array
        // so we're done

        if ((p-fmt) > arr_size)
            break;

        // locate the appropriate value in the integer
        // array based on the format string

        switch (*p)
        {
            case 'Y':
                Y = arr[p-fmt];
                if (Y < 70 && Y >= 0)
                    Y += 2000;
                if (Y >= 70 && Y < 100)
                    Y += 1900;
                break;
            case 'M':
                M = arr[p-fmt];
                break;
            case 'D':
                D = arr[p-fmt];
                break;
            case 'h':
                h = arr[p-fmt];
                break;
            case 'm':
                m = arr[p-fmt];
                break;
            case 's':
                s = arr[p-fmt];
                break;
            case 'l':
                ms = arr[p-fmt];
                break;
        }

        p++;
    }

    xd::datetime_t dt;
    xd::datetime_t ts;

    dt = dateToJulian(Y, M, D);
    dt <<= 32;

    ts = (h*3600000) + (m*60000) + (s*1000) + ms;
    dt |= ts;

    return dt;
}


std::wstring makePathName(const std::wstring& base_dir,
                          const std::wstring& subdir,
                          const std::wstring& filename,
                          const std::wstring& ext)
{
    std::wstring final;

    wchar_t sl;
    #ifdef WIN32
    sl = L'\\';
    #else
    sl = L'/';
    #endif

    final = base_dir;
    if (final.length() == 0)
        final = sl;

    if (final[final.length()-1] != sl)
        final += sl;
    
    if (!subdir.empty())
    {
        const wchar_t* s = subdir.c_str();

        if (*s == L'/' || *s == L'\\')
        {
            ++s;
        }

        final += s;

        // remove trailing slash, if any

        wchar_t last_char = final[final.length()-1];
        if (last_char == L'/' || last_char == L'\\')
        {
            final.erase(final.begin() + final.length() - 1);
        }
    }

    if (!filename.empty())
    {
        if (final[final.length()-1] != sl)
            final += sl;

        final += filename;
    }

    if (!ext.empty())
    {
        const wchar_t* s = ext.c_str();

        if (*s != L'.')
            final += L'.';

        final += s;
    }
    
    
    int pos = 0;
    while (1)
    {
        #ifdef WIN32
        pos = final.find(L'/', pos);
        #else
        pos = final.find(L'\\', pos);
        #endif
        
        if (pos == -1)
            break;
            
        final[pos] = sl;
        pos++;
    }
    
    return final;
}




std::wstring getTablenameFromOfsPath(const std::wstring& _path)
{
    std::wstring path = _path;
    kl::trim(path);

    if (path.empty())
    {
        return path;
    }

    if (path[0] == L'/')
    {
        path = path.substr(1);
    }
    

    int len = path.length();
    for (int i = 0; i < len; ++i)
    {
        if (path[i] == L'/')
        {
            path[i] = L'.';
        }
    }

    return path;
}


std::wstring getTablenameFromFilesystemPath(const std::wstring& _path)
{
    std::wstring path = _path;
    kl::trim(path);

    if (path.empty())
    {
        return path;
    }

    int i, length = (int)path.length();
    
    for (i = (length-1); i >= 0; --i)
    {
        if (path[i] == PATH_SEPARATOR_CHAR)
        break;
    }
    
    path = path.substr(i+1);
    
    // now that we have the last chunk of the filesystem path, remove
    // the file extension, if it exists
    
    length = path.length();
    for (i = 0; i < length; ++i)
    {
        if (path[i] == L'.')
        {
            path = path.substr(0,i);
            break;
        }
    }
    
    return path;
}


bool isSamePath(const std::wstring& path1, const std::wstring& path2)
{
    std::wstring s1 = path1, s2 = path2;
    kl::makeLower(s1);
    kl::makeLower(s2);
    if (s1.length() > 0 && s1[0] == '/')
        s1.erase(0,1);
    if (s2.length() > 0 && s2[0] == '/')
        s2.erase(0,1);

    return (s1 == s2) ? true : false;
}



wchar_t* zl_strblank(wchar_t* str)
{
    wchar_t *a, *b, *c, *d, *r;
    a = zl_strchr(str, L' ',  L"([", L")]");
    b = zl_strchr(str, L'\t', L"([", L")]");
    c = zl_strchr(str, L'\n', L"([", L")]");
    d = zl_strchr(str, L'\r', L"([", L")]");
    
    r = a;
    if (r == NULL || (b != NULL && b < r))
        r = b;
    if (r == NULL || (c != NULL && c < r))
        r = c;
    if (r == NULL || (d != NULL && d < r))
        r = d;
    
    return r;
}


wchar_t* zl_strchr(wchar_t* str,
                   wchar_t ch,
                   const wchar_t* open_parens,
                   const wchar_t* close_parens)
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





wchar_t* zl_stristr(wchar_t* str,
                    const wchar_t* search_str,
                    bool single_word,
                    bool forward)
{
    int str_len = wcslen(search_str);
    int paren_level = 0;
    int bracket_level = 0;
    wchar_t quote_char = 0;
    wchar_t* ptr = NULL;
    wchar_t* ch;

    ch = str;
    while (*ch)
    {
        if (*ch == quote_char)
        {
            quote_char = 0;
            ch++;
            continue;
        }

        if (*ch == L'\'' && !quote_char)
        {
            quote_char = L'\'';
        }
        
        if (*ch == L'"' && !quote_char)
        {
            quote_char = L'\"';
        }

        if (!quote_char)
        {
            if (*ch == L'(')
                paren_level++;
            if (*ch == L')')
                paren_level--;
            if (*ch == L'[')
                bracket_level++;
            if (*ch == L']')
                bracket_level--;
                
            if (paren_level == 0 && bracket_level == 0 && !wcsncasecmp(ch, search_str, str_len))
            {
                if (single_word)
                {
                    bool valid = true;
                    if (ch > str)
                    {
                        if (!iswspace(*(ch-1)))
                            valid = false;
                    }
                    if (*(ch+str_len))
                    {
                        if (!iswspace(*(ch+str_len)))
                            valid = false;
                    }
                    if (valid)
                    {
                        ptr = ch;
                    }
                     else
                    {
                        ch++;
                        continue;
                    }
                }
                 else
                {
                    ptr = ch;
                }

                if (forward)
                {
                    return ptr;
                }
            }
        }

        ch++;
    }

    return ptr;
}




void crc64(const unsigned char* s, int length, unsigned long long* result)
{
    static unsigned long long crc_table[256];
    unsigned long long crc = 0;
    static int init = 0;

    if (!init)
    {
        int i;
        init = 1;
        for (i = 0; i <= 255; i++)
        {
            int j;
            unsigned long long part = i;
            for (j = 0; j < 8; j++)
            {
                if (part & 1)
#ifdef _MSC_VER
                    part = (part >> 1) ^ ((unsigned long long)0xd800000000000000);
#else
                    part = (part >> 1) ^ 0xd800000000000000ULL;
#endif
                     else
                    part >>= 1;
            }
            crc_table[i] = part;
        }
    }

    while (length--)
    {
        unsigned long long temp1 = crc >> 8;
        unsigned long long temp2 = crc_table[(crc ^ (unsigned long long)*s) & 0xff];
        crc = temp1 ^ temp2;
        s++;
    }

    *result = crc;
}




bool getIndexExpressionMatch(const std::wstring& expr1, const std::wstring& expr2)
{
    std::vector<std::wstring> elements1;
    std::vector<std::wstring> elements2;

    kl::parseDelimitedList(expr1, elements1, L',');
    kl::parseDelimitedList(expr2, elements2, L',');

    int e1s = elements1.size();
    
    if (e1s != elements2.size())
        return false;

    int i;
    for (i = 0; i < e1s; ++i)
    {
        kl::trim(elements1[i]);
        kl::trim(elements2[i]);

        dequote(elements1[i], '[', ']');
        dequote(elements2[i], '[', ']');

        if (!kl::iequals(elements1[i], elements2[i]))
            return false;
    }

    return true;
}

xd::IndexInfo xdLookupIndex(const xd::IndexInfoEnum& idx_enum,
                            const std::wstring& expr,
                            bool exact_column_order)
{
    if (idx_enum.empty())
        return xd::IndexInfo();

    std::vector<std::wstring> expr_cols;
    size_t i, idx_count = idx_enum.size();
    xd::IndexInfo result;

    kl::parseDelimitedList(expr, expr_cols, L',', true);
    
    for (i = 0; i < idx_count; ++i)
    {
        std::vector<std::wstring> idx_cols;
        kl::parseDelimitedList(idx_enum[i].expression, idx_cols, L',', true);

        if (idx_cols.size() != expr_cols.size())
            continue;

        if (exact_column_order)
        {
            int col_count = idx_cols.size();
            int j;
            bool match = true;

            for (j = 0; j < col_count; ++j)
            {
                if (0 != wcscasecmp(idx_cols[j].c_str(), expr_cols[j].c_str()))
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                return idx_enum[i];
            }
        }
         else
        {
            int col_count = idx_cols.size();
            int j, k;
            bool match = true;

            for (j = 0; j < col_count; ++j)
            {
                // try to find it in the idx columns
                
                bool found = false;

                for (k = 0; k < col_count; ++k)
                {
                    if (0 == wcscasecmp(idx_cols[j].c_str(),
                                        expr_cols[k].c_str()))
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    match = false;
                }

            }

            if (match)
            {
                return idx_enum[i];
            }

        }
    }

    return result;
}



// this function is also used for creating unique temporary fieldnames,
// and as such, should not ever create strings that are longer
// than the max fieldname length (currently 80 characters)
std::wstring getUniqueString()
{
    static unsigned int seed = (unsigned)time(NULL);
    srand(++seed);

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




int xd2kscriptType(int type)
{
    switch (type)
    {
        case xd::typeCharacter:
        case xd::typeWideCharacter:
            return kscript::Value::typeString;

        case xd::typeNumeric:
        case xd::typeDouble:
            return kscript::Value::typeDouble;

        case xd::typeInteger:
            return kscript::Value::typeInteger;

        case xd::typeDate:
        case xd::typeDateTime:
            return kscript::Value::typeDateTime;

        case xd::typeBoolean:
            return kscript::Value::typeBoolean;
        
        default:
            return kscript::Value::typeUndefined;
    }
}

int kscript2xdType(int type)
{
    switch (type)
    {
        case kscript::Value::typeNull:       return xd::typeInvalid;
        case kscript::Value::typeUndefined:  return xd::typeUndefined;
        case kscript::Value::typeBoolean:    return xd::typeBoolean;
        case kscript::Value::typeInteger:    return xd::typeInteger;
        case kscript::Value::typeDouble:     return xd::typeNumeric;
        case kscript::Value::typeString:     return xd::typeCharacter;
        case kscript::Value::typeDateTime:   return xd::typeDateTime;
        case kscript::Value::typeBinary:     return xd::typeBinary;
    }

    return xd::typeInvalid;
}



#ifndef WIN32
wchar_t* wcsrev(wchar_t* str)
{
    wchar_t* ret = str;
    wchar_t* end = str + wcslen(str) - 1;
    wchar_t t;

    while (end > str)
    {
        t = *end;
        *end-- = *str;
        *str++ = t;
    }

    return ret;
}
#endif



void bindExprParser(kscript::ExprParser* parser)
{
    // numeric functions
    parser->addFunction(L"abs", false, func_abs, false, L"f(f);i(i)");
    parser->addFunction(L"sign", false, func_sign, false, L"i(n)");
    parser->addFunction(L"exp", false, func_exp, false, L"f(n)");
    parser->addFunction(L"ln", false, func_ln, false, L"f(n)");
    parser->addFunction(L"power", false, func_power, false, L"f(nn)");
    parser->addFunction(L"log", false, func_log, false, L"f(n[n])");
    parser->addFunction(L"cos", false, func_cos, false, L"f(n)");
    parser->addFunction(L"acos", false, func_acos, false, L"f(n)");
    parser->addFunction(L"sin", false, func_sin, false, L"f(n)");
    parser->addFunction(L"asin", false, func_asin, false, L"f(n)");
    parser->addFunction(L"tan", false, func_tan, false, L"f(n)");
    parser->addFunction(L"atan", false, func_atan, false, L"f(n)");
    parser->addFunction(L"rand", false, func_rand, false, L"f()");
    parser->addFunction(L"pi", false, func_pi, false, L"f()");
    parser->addFunction(L"mod", false, kscript::oper_mod, false, L"f(nn)");
    parser->addFunction(L"ceiling", false, func_ceiling, false, L"f(n[n])");
    parser->addFunction(L"floor", false, func_floor, false, L"f(n[n])");
    parser->addFunction(L"trunc", false, func_trunc, false, L"f(n[n])");
    parser->addFunction(L"round", false, func_round, false, L"f(n[n])");

    // string functions
    parser->addFunction(L"chr", false, func_chr, false, L"s(i)");
    parser->addFunction(L"contains", false, func_contains, false, L"b(ss)");
    parser->addFunction(L"ltrim", false, func_ltrim, false, L"s(s[s])");
    parser->addFunction(L"rtrim", false, func_rtrim, false, L"s(s[s])");
    parser->addFunction(L"trim", false, func_trim, false, L"s(s[s])");
    parser->addFunction(L"lpad", false, func_lpad, false, L"s(sn[s])");
    parser->addFunction(L"rpad", false, func_rpad, false, L"s(sn[s])");
    parser->addFunction(L"repeat", false, func_repeat, false, L"s(si)");
    parser->addFunction(L"left", false, func_left, false, L"s(sn)");
    parser->addFunction(L"right", false, func_right, false, L"s(sn)");
    parser->addFunction(L"substr", false, func_substr, false, L"s(sn[n])");
    parser->addFunction(L"strpart", false, func_strpart, false, L"s(sn[sb])");
    parser->addFunction(L"length", false, func_len, false, L"i(s)");
    parser->addFunction(L"lfind", false, func_lfind, false, L"i(ss[n])");
    parser->addFunction(L"rfind", false, func_rfind, false, L"i(ss[n])");
    parser->addFunction(L"instr", false, func_lfind, false, L"i(ss[n])");
    parser->addFunction(L"translate", false, func_translate, false, L"s(sss)");
    parser->addFunction(L"replace", false, func_replace, false, L"s(sss)");
    parser->addFunction(L"lower", false, func_lower, false, L"s(s)");
    parser->addFunction(L"upper", false, func_upper, false, L"s(s)");
    parser->addFunction(L"proper", false, func_initcap, false, L"s(s)");
    parser->addFunction(L"initcap", false, func_initcap, false, L"s(s)");   // more standard name for proper()
    parser->addFunction(L"reverse", false, func_reverse, false, L"s(s)");
    parser->addFunction(L"soundex", false, func_soundex, false, L"s(s[n])");
    parser->addFunction(L"binhex", false, func_binhex, false, L"s(x)");              // internal
    parser->addFunction(L"crc64", false, func_crc64, false, L"s(s);s(x)");           // internal
    parser->addFunction(L"md5", false, func_md5, false, L"s(s);s(x)");               // internal
    parser->addFunction(L"clean", false, func_clean, false, L"s(s);s(sn);s(snb)");   // internal
    parser->addFunction(L"sortchars", false, func_sortchars, false, L"s(s)");        // internal
    parser->addFunction(L"filterdigits", false, func_filterdigits, false, L"s(s)");  // internal
    parser->addFunction(L"filterchars", false, func_filterchars, false, L"s(ss)");  // internal
    parser->addFunction(L"stripchars", false, func_stripchars, false, L"s(ss)");  // internal
    parser->addFunction(L"strdistance", false, func_strdistance, false, L"i(ss)");  // internal

    // date functions
    parser->addFunction(L"year", false, func_year, false, L"i(d)");
    parser->addFunction(L"month", false, func_month, false, L"i(d)");
    parser->addFunction(L"monthname", false, func_monthname, false, L"s(d)");
    parser->addFunction(L"week", false, func_week, false, L"i(d)");
    parser->addFunction(L"day", false, func_day, false, L"i(d)");
    parser->addFunction(L"dayname", false, func_dayname, false, L"s(d)");
    parser->addFunction(L"dayofweek", false, func_dayofweek, false, L"i(d)");
    parser->addFunction(L"hour", false, func_hour, false, L"i(d)");
    parser->addFunction(L"minute", false, func_minute, false, L"i(d)");
    parser->addFunction(L"second", false, func_second, false, L"i(d)");
    parser->addFunction(L"curdate", false, func_curdate, false, L"d()");
    parser->addFunction(L"curdatetime", false, func_curdatetime, false, L"d()");
    parser->addFunction(L"striptime", false, func_striptime, false, L"d(d)");  // internal

    // conversion functions
    parser->addFunction(L"date", false, func_date, false, L"d(s);d(ss);d(n);d(nnn);d(nnnnnn)");
    parser->addFunction(L"str", false, func_str, false, L"s(n[n]);s(d);s(b)");
    parser->addFunction(L"val", false, func_val, false, L"f(s)");
    parser->addFunction(L"to_char", false, func_tochar, false, L"s(ds);s(ns);s(bs)");  // internal
    
    // list functions
    parser->addFunction(L"min", false, func_min, false, L"f(f)f;s(s)s;d(d)d");
    parser->addFunction(L"max", false, func_max, false, L"f(f)f;s(s)s;d(d)d");
    parser->addFunction(L"sum", false, func_sum, false, L"f(n)n");
    parser->addFunction(L"avg", false, func_avg, false, L"f(n)n");
    parser->addFunction(L"stddev", false, func_stddev, false, L"f(n)n");
    parser->addFunction(L"variance", false, func_variance, false, L"f(n)n");

    // index functions
    parser->addFunction(L"ascend", false, func_ascend, true, L"x(sn);x(dn);x(nn);x(in);x(bn)");    // internal
    parser->addFunction(L"descend", false, func_descend, true, L"x(sn);x(dn);x(nn);x(in);x(bn)");  // internal
    parser->addFunction(L"key", false, func_ascend, true, L"x(sn);x(dn);x(nn);x(in);x(bn)");       // internal
    parser->addFunction(L"asckey", false, func_asckey, true, L"x(sn);x(dn);x(nn);x(in);x(bn)");    // internal

    // other functions
    parser->addFunction(L"iif", false, func_iif, false, L"s(bss);f(bnn);b(bbb);d(bdd)");
    parser->addFunction(L"empty", false, func_empty, false, L"b(s);b(n);b(d)");
    parser->addFunction(L"isnull", false, func_isnull, false, L"b(s);b(n);b(d)");
    parser->addFunction(L"hostname", false, func_hostname, false, L"s(s)");  // internal

    // regex functions
    parser->addFunction(L"regexp", false, func_regexp, false, L"s(ss);s(ssn);s(sss)");
    parser->addFunction(L"regexp_m", false, func_regexp_m, false, L"s(ss);s(ssn);s(sss)");  // internal
    parser->addFunction(L"regexp_replace", false, func_regexp_replace, false, L"s(sss);");  // internal
    parser->addFunction(L"regexp_match", false, func_regexp_match, false, L"b(ss);");       // internal
    
    // javascript/json functions
    parser->addFunction(L"json_extract", false, func_json_extract, false, L"s(ss)");  // internal
}







class DbExprParser : public kscript::ExprParser
{

public:

    DbExprParser() : kscript::ExprParser(kscript::optionLanguageGeneric |
                                         kscript::optionEpsilonNumericCompare)
    {
        bindExprParser(this);
    }
    
    
    kscript::ExprElement* onParseElement(
                     kscript::ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc = NULL)
    {
        // try to parse date and timestamp literals
        if (*expr == '{')
        {
            expr++;
            while (iswspace(*expr))
                ++expr;
            
            if ((*expr == 'd') ||
                ((*expr == 't' && *(expr+1) == 's')))
            {
                wchar_t* start = NULL;
                wchar_t* end = NULL;
                
                wchar_t* p = expr;
                while (*p)
                {
                    if (*p == '\'' || *p == '"')
                    {
                        if (start)
                        {
                            end = p;
                            break;
                        }
                         else
                        {
                            start = p;
                        }
                    }
                    ++p;
                }

                if (start && end)
                {
                    start++;
                    if (end > start)
                    {
                        std::wstring str(start, end-start);
                        int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
                        if (::parseDateTime(str, &year, &month, &day, &hour, &minute, &second))
                        {
                            if (hour == -1)
                            {
                                hour = 0;
                                minute = 0;
                                second = 0;
                            }
                            
                            kscript::Value* retval = new kscript::Value;
                            retval->setDateTime(year, month, day, hour, minute, second, 0);
                            return retval;
                        }
                    }
                }

            }
            

            m_error_loc = expr; // end
            m_error_code = kscript::errorMalformedStatement;
            m_error_text = L"";
            if (*endloc)
                *endloc = expr;
        }
        
        // allow default processing to continue
        return NULL;
    }
};



kscript::ExprParser* createExprParser()
{
    kscript::ExprParser* parser = new DbExprParser;
    return parser;
}







struct FindFieldInfo
{
    std::set<std::wstring> found_fields;
    xd::Structure structure;
};

static bool findfield_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    FindFieldInfo* info = (FindFieldInfo*)hook_info.hook_param;

    hook_info.res_element = NULL;

    if (hook_info.element_type == kscript::ExprParseHookInfo::typeOperator)
    {
        // not handled
        return false;
    }

    std::wstring expr_text = hook_info.expr_text;
    kl::makeUpper(expr_text);

    if (hook_info.element_type == kscript::ExprParseHookInfo::typeFunction)
    {
        std::wstring func_name = expr_text;

        std::wstring param;
        param = kl::afterFirst(expr_text, L'(');
        param = kl::beforeLast(param, L')');
        kl::trim(param);

        // check for aggregate functions
        if (func_name == L"SUM" ||
            func_name == L"AVG" ||
            func_name == L"STDDEV" ||
            func_name == L"VARIANCE")
        {
            const xd::ColumnInfo& colinfo = info->structure.getColumnInfo(param);
            if (colinfo.isNull())
                return true;

            if (colinfo.type != xd::typeNumeric &&
                colinfo.type != xd::typeInteger &&
                colinfo.type != xd::typeDouble)
            {
                // fail
                return true;
            }

            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"MERGE")
        {
            const xd::ColumnInfo& colinfo = info->structure.getColumnInfo(param);
            if (colinfo.isNull())
                return true;
        
            if (colinfo.type != xd::typeCharacter &&
                colinfo.type != xd::typeWideCharacter)
            {
                // fail
                return true;
            }

            kscript::Value* v = new kscript::Value;
            v->setString(L"");
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"COUNT")
        {
            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"GROUPID")
        {
            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"MAXDISTANCE")
        {
            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"FIRST" ||
                  func_name == L"LAST" ||
                  func_name == L"MIN" ||
                  func_name == L"MAX")
        {
            // continue below
            expr_text = param;
        }
         else
        {
            // not handled
            return false;
        }
    }


    const xd::ColumnInfo& colinfo = info->structure.getColumnInfo(expr_text);

    if (colinfo.isNull())
    {
        // not handled
        return false;
    }

    kscript::Value* v = new kscript::Value;

    switch (colinfo.type)
    {
        case xd::typeCharacter:
        case xd::typeWideCharacter:
            v->setString(L"");
            break;
        case xd::typeNumeric:
        case xd::typeDouble:
            v->setDouble(0.0);
            break;
        case xd::typeInteger:
            v->setInteger(0);
            break;
        case xd::typeBoolean:
            v->setBoolean(true);
            break;
        case xd::typeDateTime:
        case xd::typeDate:
            v->setDateTime(0,0);
            break;
        case xd::typeBinary:
            v->setType(kscript::Value::typeBinary);
            break;
        default:
            return true;
    }

    hook_info.res_element = v;

    info->found_fields.insert(expr_text);

    return true;
}




static void _findFieldsInExpr(const std::wstring& expr,
                              const xd::Structure& s,
                              bool recurse_calc_fields,
                              std::set<std::wstring>& fields)
{
    FindFieldInfo info;
    info.structure = s;

    std::vector<std::wstring> parts;
    std::vector<std::wstring>::iterator pit;

    kl::parseDelimitedList(expr, parts, L',', true);

    for (pit = parts.begin(); pit != parts.end(); ++pit)
    {
        kscript::ExprParser* parser = createExprParser();
        parser->setParseHook(kscript::ExprParseHookInfo::typeFunction |
                             kscript::ExprParseHookInfo::typeIdentifier,
                             findfield_parse_hook,
                             &info);
                             
        if (!parser->parse(*pit))
        {
            delete parser;
            return;
        }
        delete parser;
    }


    std::set<std::wstring>::iterator it;

    // remove fields that we already found
    // (this will prevent infinite recursion problems)

    for (it = fields.begin();
         it != fields.end();
         ++it)
    {
        info.found_fields.erase(*it);
    }


    // add the fields we found to the result set
    for (it = info.found_fields.begin();
         it != info.found_fields.end();
         ++it)
    {
        fields.insert(*it);
    }


    if (recurse_calc_fields)
    {
        for (it = info.found_fields.begin();
             it != info.found_fields.end();
             ++it)
        {
            xd::ColumnInfo colinfo = s.getColumnInfo(*it);
            if (colinfo.isNull())
                continue;
            if (!colinfo.calculated)
                continue;
            
            _findFieldsInExpr(colinfo.expression,
                              s,
                              recurse_calc_fields,
                              fields);
        }

    }
}

std::vector<std::wstring> getFieldsInExpr(const std::wstring& expr,
                                          const xd::Structure& s,
                                          bool recurse_calcfields)
{
    std::set<std::wstring> flds;
    _findFieldsInExpr(expr, s, recurse_calcfields, flds);

    std::vector<std::wstring> result;

    std::set<std::wstring>::iterator it;
    for (it = flds.begin();
         it != flds.end();
         ++it)
    {
        result.push_back(*it);
    }

    return result;
}

bool findFieldInExpr(const std::wstring& _field,
                     const std::wstring& expr,
                     const xd::Structure& s,
                     bool recurse_calcfields)
{
    std::wstring field = _field;
    kl::makeUpper(field);


    std::set<std::wstring> flds;
    _findFieldsInExpr(expr, s, recurse_calcfields, flds);

    if (flds.find(field) != flds.end())
        return true;

    return false;
}








static void modColumn(xd::ColumnInfo& target_col, const xd::ColumnInfo& params)
{
    if (params.mask & xd::ColumnInfo::maskName)
    {
        target_col.name = params.name;
    }

    if (params.mask & xd::ColumnInfo::maskType)
    {
        target_col.type = params.type;
    }
    
    if (params.mask & xd::ColumnInfo::maskWidth)
    {
        target_col.width = params.width;
    }

    if (params.mask & xd::ColumnInfo::maskScale)
    {
        target_col.scale = params.scale;
    }

    if (params.mask & xd::ColumnInfo::maskExpression)
    {
        target_col.expression = params.expression;
        target_col.calculated = true;
    }

    if (params.mask & xd::ColumnInfo::maskSourceOffset)
    {
        target_col.source_offset = params.source_offset;
    }
    
    if (params.mask & xd::ColumnInfo::maskSourceEncoding)
    {
        target_col.source_encoding = params.source_encoding;
    }
     
    if (params.mask & xd::ColumnInfo::maskColumnOrdinal)
    {
        // new column position
        target_col.column_ordinal = params.column_ordinal;
    }


    // if type changed, make sure width and scale conform
    if (params.mask & xd::ColumnInfo::maskType)
    {
        switch (target_col.type)
        {
            case xd::typeDate:
                target_col.width = 4;
                target_col.scale = 0;
                break;
            case xd::typeInteger:
                target_col.width = 4;
                target_col.scale = 0;
                break;
            case xd::typeDouble:
                target_col.width = 8;
                break;
            case xd::typeBoolean:
                target_col.width = 1;
                target_col.scale = 0;
                break;
            case xd::typeDateTime:
                target_col.width = 8;
                target_col.scale = 0;
                break;
        }
    }
}

bool calcfieldsModifyStructure(const xd::StructureModify& mod_params,
                               std::vector<xd::ColumnInfo>& calc_fields,
                               bool* done_flag)
{

    *done_flag = false;
    unsigned int processed_action_count = 0;

    std::vector<xd::StructureModify::Action>::const_iterator it;
    std::vector<xd::ColumnInfo>::iterator cit;

    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionDelete)
        {
            bool processed = false;

            for (cit = calc_fields.begin(); cit != calc_fields.end(); ++cit)
            {
                if (kl::iequals(cit->name, it->column))
                {
                    calc_fields.erase(cit);
                    processed_action_count++;
                    processed = true;
                    break;
                }
            }
        }
    }


    // handle modify
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionModify)
        {
            bool processed = false;

            for (cit = calc_fields.begin(); cit != calc_fields.end(); ++cit)
            {
                if (kl::iequals(cit->name, it->column))
                {
                    if ((it->params.mask & xd::ColumnInfo::maskCalculated) && !it->params.calculated)
                    {
                        // caller wants this field to be permanent,
                        // so we won't do anything here

                        continue;
                    }

                    modColumn(*cit, it->params);

                    processed_action_count++;
                    processed = true;
                    break;
                }
            }
        }
    }

    // handle create
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionCreate)
        {
            if (it->params.expression.length() > 0)
            {
                xd::ColumnInfo colinfo = it->params;
                colinfo.calculated = true;
                calc_fields.push_back(colinfo);

                processed_action_count++;
            }
        }
    }

    // handle insert
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionInsert)
        {
            if (it->params.expression.length() > 0)
            {
                xd::ColumnInfo colinfo = it->params;
                colinfo.calculated = true;
                calc_fields.push_back(colinfo);

                processed_action_count++;
            }
        }
    }

    if (processed_action_count == mod_params.actions.size())
    {
        // we have handled all actions, so we're done
        *done_flag = true;
    }

    return true;
}









xd::rowid_t bufToRowid(unsigned char* buf)
{
    long long result, tempv;
    tempv = buf[7];
    result = tempv;
    tempv = buf[6];
    result |= (tempv << 8);
    tempv = buf[5];
    result |= (tempv << 16);
    tempv = buf[4];
    result |= (tempv << 24);
    tempv = buf[3];
    result |= (tempv << 32);
    tempv = buf[2];
    result |= (tempv << 40);
    tempv = buf[1];
    result |= (tempv << 48);
    tempv = buf[0];
    result |= (tempv << 56);
    return result;
}

void rowidToBuf(unsigned char* bytes, xd::rowid_t r)
{
    bytes[7] = (unsigned char)((r) & 0xff);
    bytes[6] = (unsigned char)((r >> 8) & 0xff);
    bytes[5] = (unsigned char)((r >> 16) & 0xff);
    bytes[4] = (unsigned char)((r >> 24) & 0xff);
    bytes[3] = (unsigned char)((r >> 32) & 0xff);
    bytes[2] = (unsigned char)((r >> 40) & 0xff);
    bytes[1] = (unsigned char)((r >> 48) & 0xff);
    bytes[0] = (unsigned char)((r >> 56) & 0xff);
}




#define DPOS(x, y) d[((y)*(m+1))+(x)]

inline int min3(int a, int b, int c)
{
    int d = (a < b ? a : b);
    return (c < d ? c : d);
}

int levenshtein(const wchar_t* s, const wchar_t* t)
{
    int m = wcslen(s);
    int n = wcslen(t);
    int i, j;

    int* d = new int[(m+1)*(n+1)];
    for (i = 0; i < (m+1)*(n+1); ++i)
        d[i] = 0;
    for (i = 0; i <= m; ++i)
        DPOS(i,0) = i;
    for (j = 0; j <= n; ++j)
        DPOS(0,j) = j;

    for (j = 1; j <= n; ++j)
    {
        for (i = 1; i <= m; ++i)
        {
            if (s[i-1] == t[j-1])
            {
                DPOS(i,j) = DPOS(i-1,j-1);
            }
             else
            {
                DPOS(i,j) = min3(
                                DPOS(i-1, j) + 1,
                                DPOS(i, j-1) + 1,
                                DPOS(i-1, j-1) + 1
                            );
            }
        }
    }

    int res = DPOS(m, n);
    delete[] d;
    return res;
}
