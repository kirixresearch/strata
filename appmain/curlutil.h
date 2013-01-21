/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-09
 *
 */


#ifndef __APP_CURL_H
#define __APP_CURL_H


typedef void CURL;


CURL* curlCreateHandle();
void curlDestroyHandle(CURL* curl);


size_t curl_xffile_writer(void* ptr, size_t size, size_t nmemb, void *stream);
size_t curl_string_writer(void* ptr, size_t size, size_t nmemb, void *stream);


class CurlAutoDestroy
{
public:
    CurlAutoDestroy(CURL* _h) : h(_h) { }
    ~CurlAutoDestroy() { curlDestroyHandle(h); }
    CURL* h;
};


#endif

