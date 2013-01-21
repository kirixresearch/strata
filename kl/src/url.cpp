/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2009-05-30
 *
 */


#include "kl/portable.h"
#include "kl/url.h"


namespace kl
{


static std::wstring doEncode(const std::wstring& input, const wchar_t* to_encode)
{
    std::wstring result;
    
    const wchar_t* ch = input.c_str();
    unsigned int c;
    
    wchar_t buf[64];
    
    while ((c = *ch))
    {
        if (c >= 128)
        {
            // TODO: the below works as expected with escape();
            // My understanding is that, for the other functions,
            // we need to utf-8 encode this character per RFC-3986
 
            if (c <= 255)
            {
                swprintf(buf, 64, L"%%%02X", c);
                result += buf;
            }
             else
            {
                swprintf(buf, 64, L"%%u%04X", c);
                result += buf;
            }
            
            ch++;
            continue;
        }
        
        if (c <= 0x1f || wcschr(to_encode, *ch))
        {
            swprintf(buf, 64, L"%%%02X", c);
            result += buf;
        }
         else
        {
            result += *ch;
        }
        
        ++ch;
    }

    return result;
}
      
static std::wstring doDecode(const std::wstring& input)
{
    std::wstring result;
    
    const wchar_t* ch = input.c_str();
    const wchar_t* d;
    static const wchar_t* hexchars = L"0123456789ABCDEF";
    
    while (*ch)
    {
        if (*ch == '%')
        {
            if (towupper(*(ch+1)) == 'U')
            {
                if (*(ch+2) && *(ch+3) && *(ch+4) && *(ch+5))
                {
                    const wchar_t* s = ch+1;
                    const wchar_t* res;
                    unsigned int i, mul = 4096, c = 0;
                    
                    for (i = 1; i <= 4; ++i)
                    {
                        res = wcschr(hexchars, ::towupper(*(s+i)));
                        if (!res)
                            break;
                        c += (res-hexchars)*mul;
                        mul /= 16;
                    }

                    if (i == 5)
                    {
                        result += (wchar_t)c;
                        ch += 6;
                        continue;
                    }
                }
            }
             else if (*(ch+1) && *(ch+2))
            {
                int digit1, digit2;
                d = wcschr(hexchars, towupper(*(ch+1)));
                if (d)
                {
                    digit1 = d-hexchars;
                    d = wcschr(hexchars, towupper(*(ch+2)));
                    if (d)
                    {
                        digit2 = d-hexchars;
                        wchar_t r = (digit1*16) + digit2;
                        result += r;
                        ch += 3;
                        continue;
                    }
                }
            }
        }
        
        result += *ch;
        ++ch;
    }

    return result;
}


// encode-decode functions
            

std::wstring url_escape(const std::wstring& input)
{
    return doEncode(input, L"%!&$#(){}[]<>=:,;?\\|^~`'\" ");
}

std::wstring url_unescape(const std::wstring& input)
{
    return doDecode(input);
}

std::wstring url_encodeURI(const std::wstring& input)
{
    return doEncode(input, L"%&{}[]<>\\|^`\" ");
}

std::wstring url_decodeURI(const std::wstring& input)
{
    return doDecode(input);
}

std::wstring url_encodeURIComponent(const std::wstring input)
{
    return doEncode(input, L"%@&$#{}[]<>=:/,;?+\\|^`\" ");
}

std::wstring url_decodeURIComponent(const std::wstring input)
{
    return doDecode(input);
}


};


