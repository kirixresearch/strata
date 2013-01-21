/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-09
 *
 */


#include "appmain.h"
#include <curl/curl.h>


// -- curl machinery --

CURL* g_curl = (CURL*)0;

class CurlInit
{
public:

    CurlInit()
    {
        // -- initialize curl --
        if (0 != curl_global_init(CURL_GLOBAL_ALL))
        {
            // failure
            return;
        }
        
        // -- initialize curl_easy_* functions --
        g_curl = curl_easy_init();

        if (g_curl != NULL)
        {
            // -- from the curl documentation:  "When using multiple threads you 
            //    should set the CURLOPT_NOSIGNAL option to TRUE for all handles. 
            //    Everything will or might work fine except that timeouts are not 
            //    honored during the DNS lookup ... On some platforms, libcurl simply 
            //    will not function properly multi-threaded unless this option is set." --
            curl_easy_setopt(g_curl, CURLOPT_NOSIGNAL, TRUE);
        }
    }
    
    ~CurlInit()
    {
        if (g_curl == NULL)
            return;

        curl_easy_cleanup(g_curl);
        g_curl = (CURL*)0;
    }
};

CurlInit g_curl_init;


CURL* curlCreateHandle()
{
    if (g_curl != NULL)
    {
        CURL* curl = curl_easy_duphandle(g_curl);
        
        return curl;
    }  

    return (CURL*)0;
}

void curlDestroyHandle(CURL* curl)
{
    curl_easy_cleanup(curl);
}






size_t curl_xffile_writer(void* ptr, size_t size, size_t nmemb, void *stream)
{
    unsigned int amt = xf_write((xf_file_t)stream, ptr, size, nmemb);
    return amt*size;
}

size_t curl_string_writer(void* ptr, size_t size, size_t nmemb, void *stream)
{
    std::string* str = (std::string*)stream;
    size_t total_size = 0;
    
    // limit string length to 5 MB
    if (str->length() < 5000000)
    {
        total_size = size*nmemb;
        str->append((const char*)ptr, total_size);
    }

    // -- return the number of bytes actually taken care of; if it differs from
    //    the amount passed to the function, it will signal an error to the library,
    //    abort the transfer and return CURLE_WRITE_ERROR --
    return total_size;
}
