/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-23
 *
 */

#include "xdclient.h"
#include "request.h"
#include <curl/curl.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/portable.h>
#include <kl/url.h>




static bool isUtf8(const std::string& header, const std::string& content)
{
    // poor man's detect utf-8 routine.  This really should be implemented
    // better by parsing for charset= etc.
    
    std::string h = header;
    kl::makeLower(h);
    std::string c = content.substr(0, 500);
    kl::makeLower(c);
    
    if (h.find("utf-8") != -1)
        return true;
    if (c.find("utf-8") != -1)
        return true;
    return false;
}



static std::wstring multipartEncode(const std::wstring& input)
{
    std::wstring result;
    result.reserve(input.length() + 10);
    
    const wchar_t* ch = input.c_str();
    unsigned int c;
    
    wchar_t buf[80];

    while ((c = (unsigned int)*ch))
    {
        if (c > 255)
        {
            swprintf(buf, 80, L"&#%d;", c);
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





HttpRequest::HttpRequest()
{
    m_response_bytes = 0;
    m_post_string = "";
    m_post_multipart = false;
    m_formfields = NULL;
    m_formfieldslast = NULL;
    m_curl = NULL;

    // initialize CURL
    static bool global_init = true;
    if (global_init)
    {
        if (0 != curl_global_init(CURL_GLOBAL_ALL))
            return;
        global_init = false;
    }
}

HttpRequest::~HttpRequest()
{
    close();
}

void HttpRequest::init()
{
    m_curl = curl_easy_init();

    if (m_curl == NULL)
        return;

    //CURLcode curl_result;

    // from the curl documentation:  "When using multiple threads you 
    // should set the CURLOPT_NOSIGNAL option to TRUE for all handles. 
    // Everything will or might work fine except that timeouts are not 
    // honored during the DNS lookup ... On some platforms, libcurl simply 
    // will not function properly multi-threaded unless this option is set."
    (void)curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, TRUE);
        
    // see http://curl.haxx.se/lxr/source/docs/examples/https.c
    // these are necessary unless we want to include a certificate bundle
    (void)curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, FALSE);
    (void)curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0);
    
    // follow redirects
    (void)curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, TRUE);
    
    // when redirecting, set the referer
    (void)curl_easy_setopt(m_curl, CURLOPT_AUTOREFERER, TRUE);
    
    // set the maximimum number of redirects to infinity
    (void)curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, -1);
    
    // set the default proxy type
    //(void)curl_easy_setopt(m_curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
    
    // set the default authentication methods
    (void)curl_easy_setopt(m_curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    (void)curl_easy_setopt(m_curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    
    // allow cookies
    if (m_cookie_file_path.length() > 0)
    {
        (void)curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, m_cookie_file_path.c_str());
        (void)curl_easy_setopt(m_curl, CURLOPT_COOKIEJAR, m_cookie_file_path.c_str());
    }
     else
    {
        (void)curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");
    }

    // include headers in body output
    //(void)curl_easy_setopt(m_curl, CURLOPT_HEADER, TRUE);
    
    curl_version_info_data* a = curl_version_info(CURLVERSION_NOW);
}


void HttpRequest::close()
{
    resetPostParameters();

    if (m_curl != NULL)
        curl_easy_cleanup(m_curl);
        
    freeResponsePieces();
    
    m_curl = NULL;
}


void HttpRequest::setCookieFilePath(const std::wstring& location)
{
    m_cookie_file_path = kl::tostring(location);
}


void HttpRequest::send()
{
    CURLcode curl_result;
    
    // free any previous request
    freeResponsePieces();
    m_response_bytes = 0;

    if (m_curl == NULL)
        init();

    (void)curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_1_0);

    curl_result = curl_easy_setopt(m_curl, CURLOPT_URL, m_location.c_str());
    if (curl_result != CURLE_OK)
        return;

    curl_result = curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);
    if (curl_result != CURLE_OK)
        return;

    // set the result functions
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void*)this);
    if (curl_result != CURLE_OK)
        return;
    
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, (void*)http_response_writer);
    if (curl_result != CURLE_OK)
        return;
        
    // result header
    std::string response_header;
    curl_result = curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &response_header);
    if (curl_result != CURLE_OK)
        return;
        
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, (void*)http_header_writer);
    if (curl_result != CURLE_OK)
        return;

    // get the full body
    curl_result = curl_easy_setopt(m_curl, CURLOPT_NOBODY, FALSE);
    if (curl_result != CURLE_OK)
        return;


    if (m_post_multipart)
    {
        // set the post multipart parameters
        curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, m_formfields);
        if (curl_result != CURLE_OK)
            return;
    }
     else
    {
        if (m_post_string.length() == 0)
        {
            // set the GET option
            curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPGET, TRUE);
            if (curl_result != CURLE_OK)
                return;
        }
         else
        {
            // set the POST option
            curl_result = curl_easy_setopt(m_curl, CURLOPT_POST, TRUE);
            if (curl_result != CURLE_OK)
                return;
                
            curl_result = curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, (const char*)m_post_string.c_str());
            if (curl_result != CURLE_OK)
                return;

            (void)curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, (long)m_post_string.length());
        }
    }

    // retrieve the data from the URL
    curl_result = curl_easy_perform(m_curl);
    if (curl_result != CURLE_OK)
        return;
}

void HttpRequest::setLocation(const std::wstring& location)
{
    m_location = kl::tostring(location);
}

void HttpRequest::setTimeout(int timeout)
{
    m_timeout = timeout;
}

void HttpRequest::resetPostParameters()
{
    // free the form fields
    if (m_formfields != NULL)
    {
        curl_formfree(m_formfields);
        m_formfields = NULL;
        m_formfieldslast = NULL;
    }

    m_post_string = "";
    m_post_multipart = false;
}

void HttpRequest::useMultipartPost()
{
    m_post_multipart = true;
}

void HttpRequest::setPostValue(const std::wstring& key, const std::wstring& value)
{
    // append the value to our post structure (note
    // this is only used in the case of multipart posts)
    curl_formadd(&m_formfields, &m_formfieldslast,
                 CURLFORM_COPYNAME, (const char*)kl::tostring(multipartEncode(key)).c_str(),
                 CURLFORM_COPYCONTENTS, (const char*)kl::tostring(multipartEncode(value)).c_str(),
                 CURLFORM_END);

    if (m_post_multipart)
        return; // only use multipart

    // append the value to our post string (regular, non-multipart post)
    if (m_post_string.length() > 0)
        m_post_string += "&";
    m_post_string += kl::tostring(kl::url_escape(key));
    m_post_string += "=";
    m_post_string += kl::tostring(kl::url_escape(value));
}

std::wstring HttpRequest::getResponseString()
{
    std::string response_a;
    
    std::list<HttpResponsePiece>::iterator it, it_end;
    it_end = m_response_pieces.end();
    for (it = m_response_pieces.begin(); it != it_end; ++it)
    {
        response_a.append((const char*)it->buf, it->len);
        if (response_a.length() > 5000000)
            break;
    }
    
    if (isUtf8(m_response_header, response_a))
        return kl::utf8_utf8towstr(response_a.c_str());
         else
        return kl::towstring(response_a);
}

size_t HttpRequest::getResponseSize()
{
    return m_response_bytes;
}

void HttpRequest::freeResponsePieces()
{
    std::list<HttpResponsePiece>::iterator it, it_end;
    it_end = m_response_pieces.end();
    for (it = m_response_pieces.begin(); it != it_end; ++it)
        delete[] it->buf;
    m_response_pieces.clear();
    m_response_bytes = 0;
}

// static
size_t HttpRequest::http_header_writer(void* ptr, size_t size, size_t nmemb, void *stream)
{
    std::string* str = (std::string*)stream;
    size_t total_size = 0;
    
    // limit string length to 5 MB
    if (str->length() < 5000000)
    {
        total_size = size*nmemb;
        
        // only keep the last http header
        if (0 == strncmp((const char*)ptr, "HTTP/", 5))
            *str = "";
        
        str->append((const char*)ptr, total_size);
    }

    // return the number of bytes actually taken care of; if it differs from
    // the amount passed to the function, it will signal an error to the library,
    // abort the transfer and return CURLE_WRITE_ERROR
    return total_size;
}

// static
size_t HttpRequest::http_response_writer(void* ptr, size_t size, size_t nmemb, void *stream)
{
    HttpRequest* pThis = (HttpRequest*)stream;

    HttpResponsePiece piece;
    piece.len = size*nmemb;
    piece.curpos = 0;
    piece.buf = new unsigned char[piece.len];
    memcpy(piece.buf, ptr, piece.len);

    pThis->m_response_pieces.push_back(piece);
    pThis->m_response_bytes += piece.len;
    
    return piece.len;
}

