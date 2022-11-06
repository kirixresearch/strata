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


bool isUrl(const std::wstring& s)
{
    return (s.find(L"://") != s.npos ? true : false);
}


bool isFileUrl(const std::wstring& s)
{
    std::wstring temps = s;
    kl::trim(temps);
    
    if (0 == wcscasecmp(temps.substr(0,5).c_str(), L"file:"))
    {
        return true;
    }
    
    return false;
}



// transforms a file:// url into a normal filename

std::wstring urlToFilename(const std::wstring& file_url)
{
    std::wstring result = file_url;
    
    kl::trim(result);
    
    if (0 != wcscasecmp(result.substr(0,5).c_str(), L"file:"))
    {
        // there is no file: part in the url,
        // just return the filename as is
        return result;
    }
    
    // get rid of the 'file:'
    result.erase(0, 5);
    
    // convert all slashes to the '/' (for now)
    size_t i, len = result.length();
    for (i = 0; i < len; ++i)
    {
        if (result[i] == L'\\' || result[i] == L'/')
            result[i] = L'/';
    }
    
    // replace all %20's with spaces
    kl::replaceStr(result, L"%20", L" ");
    
    // replace all %3A's with colons
    kl::replaceStr(result, L"%3A", L":");
    kl::replaceStr(result, L"%3a", L":");
    
    // replace all %26's with ampersands
    kl::replaceStr(result, L"%26", L"&");
    
    // replace all %25's with percents
    kl::replaceStr(result, L"%25", L"%");
    
    
    // count how many slashes we have
    size_t num_slashes = 0;
    while (num_slashes < result.length() && result[num_slashes] == L'/')
    {
        num_slashes++;
    }
    
    
    
    if (num_slashes >= 4)
    {
        // because there are more than four slahes, it was
        // a UNC path.  Leave just two of the slashes
        result.erase(0, num_slashes-2);
    }
     else
    {
        result.erase(0, num_slashes);
        
        // if the first character is a letter and the second
        // a | sign, this can be a form of indicating a DOS-drive
        // (example: file:///c|/file.txt).  The pipe sign will be
        // changed to a colon
        if (result.length() >= 2 && iswalpha(result[0]) && result[1] == L'|')
            result[1] = L':';
        
        // if there is no drive, place an initial root slash at the beginning
        if (result.length() < 2 || result[1] != L':')
            result = L"/" + result;
    }
    
    
    // convert all the slashes to the platforms path separator
#ifdef WIN32
    len = result.length();
    for (i = 0; i < len; ++i)
    {
        if (result[i] == L'/')
            result[i] = L'\\';
    }
#endif
    
    return result;
}


// transforms a filename into file:/// url

std::wstring filenameToUrl(const std::wstring& _filename)
{
    std::wstring result = L"file://";
    
    std::wstring filename = _filename;
    kl::trim(filename);

    if (filename.empty() || filename[0] != '/')
        result += L'/';

    result += filename;

    // replace all %20's with spaces, etc.
    kl::replaceStr(result,  L"%", L"%25");
    kl::replaceStr(result,  L"&", L"%26");
    kl::replaceStr(result,  L" ", L"%20");
    kl::replaceStr(result,  L"\\", L"/");

    return result;
}


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
                swprintf(buf, 63, L"%%%02X", c);
                buf[63] = 0;
                result += buf;
            }
             else
            {
                swprintf(buf, 63, L"%%u%04X", c);
                buf[63] = 0;
                result += buf;
            }
            
            ch++;
            continue;
        }
        
        if (c <= 0x1f || wcschr(to_encode, *ch))
        {
            swprintf(buf, 63, L"%%%02X", c);
            buf[63] = 0;
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
         else if (*ch == '+')
        {
            result += ' ';
            ++ch;
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

std::wstring url_encodeURIComponent(const std::wstring& input)
{
    return doEncode(input, L"%@&$#{}[]<>=:/,;?+\\|^`\" ");
}

std::wstring url_decodeURIComponent(const std::wstring& input)
{
    return doDecode(input);
}


};


