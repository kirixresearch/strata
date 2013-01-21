/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2007-06-22
 *
 */


#ifndef __KL_URL_H
#define __KL_URL_H


#include <kl/string.h>


namespace kl
{


// determines whether a string is a file url or not

inline bool isFileUrl(const std::wstring s)
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

inline std::wstring urlToFilename(const std::wstring& file_url)
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


// transforms a filename into file:// url

inline std::wstring filenameToUrl(const std::wstring& _filename)
{
    std::wstring result = L"file://";
    
    std::wstring filename = _filename;
    kl::trim(filename);

    result += filename;

    // replace all %20's with spaces, etc.
    kl::replaceStr(result,  L"%", L"%25");
    kl::replaceStr(result,  L"&", L"%26");
    kl::replaceStr(result,  L" ", L"%20");
    kl::replaceStr(result,  L"\\", L"/");

    return result;
}

// url escape/unescape functions

std::wstring url_escape(const std::wstring& input);
std::wstring url_unescape(const std::wstring& input);

std::wstring url_encodeURI(const std::wstring& input);
std::wstring url_decodeURI(const std::wstring& input);

std::wstring url_encodeURIComponent(const std::wstring input);
std::wstring url_decodeURIComponent(const std::wstring input);


};



#endif

