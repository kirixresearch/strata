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

bool isFileUrl(const std::wstring& s);


// transforms a file:// url into a normal filename

std::wstring urlToFilename(const std::wstring& file_url);


// transforms a filename into file:/// url

std::wstring filenameToUrl(const std::wstring& filename);

// url escape/unescape functions

std::wstring url_escape(const std::wstring& input);
std::wstring url_unescape(const std::wstring& input);

std::wstring url_encodeURI(const std::wstring& input);
std::wstring url_decodeURI(const std::wstring& input);

std::wstring url_encodeURIComponent(const std::wstring& input);
std::wstring url_decodeURIComponent(const std::wstring& input);


};



#endif

