/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-23
 *
 */


#include "request.h"
#include <curl/curl.h>
#include <kl/string.h>
#include <kl/utf8.h>


HttpRequest g_httprequest;


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








RpcConnectionHttp::RpcConnectionHttp()
{
    m_response_bytes = 0;
    
    // initialize CURL
    if (0 != curl_global_init(CURL_GLOBAL_ALL))
        return;

    m_curl = curl_easy_init();

    if (m_curl == NULL)
        return;

    CURLcode curl_result;

    // from the curl documentation:  "When using multiple threads you 
    // should set the CURLOPT_NOSIGNAL option to TRUE for all handles. 
    // Everything will or might work fine except that timeouts are not 
    // honored during the DNS lookup ... On some platforms, libcurl simply 
    // will not function properly multi-threaded unless this option is set."
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, TRUE);
        
    // see http://curl.haxx.se/lxr/source/docs/examples/https.c
    // these are necessary unless we want to include a certificate bundle
    curl_result = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, FALSE);
    curl_result = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0);
    
    // follow redirects
    curl_result = curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, TRUE);
    
    // when redirecting, set the referer
    curl_result = curl_easy_setopt(m_curl, CURLOPT_AUTOREFERER, TRUE);
    
    // set the maximimum number of redirects to infinity
    curl_result = curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, -1);
    
    // set the default proxy type
    //curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
    
    // set the default authentication methods
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    
    // allow cookies
    curl_result = curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");
    
    // include headers in body output
    //curl_result = curl_easy_setopt(m_curl, CURLOPT_HEADER, TRUE);
    
    curl_version_info_data* a = curl_version_info(CURLVERSION_NOW);
}

RpcConnectionHttp::~RpcConnectionHttp()
{
    if (m_curl != NULL)
        curl_easy_cleanup(m_curl);
        
    freeResponsePieces();
}

void RpcConnectionHttp::freeResponsePieces()
{
    std::list<HttpResponsePiece>::iterator it, it_end;
    it_end = m_response_pieces.end();
    for (it = m_response_pieces.begin(); it != it_end; ++it)
        delete[] it->buf;
    m_response_pieces.clear();
    m_response_bytes = 0;
}

MethodResponse RpcConnectionHttp::call(const MethodCall& call)
{
    MethodResponse m;
    return m;
}






HttpRequest::HttpRequest()
{
    m_response_bytes = 0;
    
    // initialize CURL
    if (0 != curl_global_init(CURL_GLOBAL_ALL))
        return;

    m_curl = curl_easy_init();

    if (m_curl == NULL)
        return;

    CURLcode curl_result;

    // from the curl documentation:  "When using multiple threads you 
    // should set the CURLOPT_NOSIGNAL option to TRUE for all handles. 
    // Everything will or might work fine except that timeouts are not 
    // honored during the DNS lookup ... On some platforms, libcurl simply 
    // will not function properly multi-threaded unless this option is set."
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, TRUE);
        
    // see http://curl.haxx.se/lxr/source/docs/examples/https.c
    // these are necessary unless we want to include a certificate bundle
    curl_result = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, FALSE);
    curl_result = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0);
    
    // follow redirects
    curl_result = curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, TRUE);
    
    // when redirecting, set the referer
    curl_result = curl_easy_setopt(m_curl, CURLOPT_AUTOREFERER, TRUE);
    
    // set the maximimum number of redirects to infinity
    curl_result = curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, -1);
    
    // set the default proxy type
    //curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
    
    // set the default authentication methods
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_result = curl_easy_setopt(m_curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    
    // allow cookies
    curl_result = curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");
    
    // include headers in body output
    //curl_result = curl_easy_setopt(m_curl, CURLOPT_HEADER, TRUE);
    
    curl_version_info_data* a = curl_version_info(CURLVERSION_NOW);        
}

HttpRequest::~HttpRequest()
{
    if (m_curl != NULL)
        curl_easy_cleanup(m_curl);
        
    freeResponsePieces();        
}

void HttpRequest::send()
{
    CURLcode curl_result;
    
    // free any previous request
    freeResponsePieces();


    curl_result = curl_easy_setopt(m_curl, CURLOPT_URL, m_location.c_str());
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

    // set the GET option
    curl_result = curl_easy_setopt(m_curl, CURLOPT_HTTPGET, TRUE);
    if (curl_result != CURLE_OK)
        return;
    
    // retrieve the data from the URL
    curl_result = curl_easy_perform(m_curl);
    if (curl_result != CURLE_OK)
        return;
}

void HttpRequest::setLocation(const std::string& location)
{
    m_location = location;
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

    // -- return the number of bytes actually taken care of; if it differs from
    //    the amount passed to the function, it will signal an error to the library,
    //    abort the transfer and return CURLE_WRITE_ERROR --
    return total_size;
}

// static
size_t HttpRequest::http_response_writer(void* ptr, size_t size, size_t nmemb, void *stream)
{
    HttpRequest* pThis = (HttpRequest*)stream;
    size_t total_size = 0;
    
    HttpResponsePiece piece;
    piece.len = size*nmemb;
    piece.curpos = 0;
    piece.buf = new unsigned char[piece.len];
    memcpy(piece.buf, ptr, piece.len);

    pThis->m_response_pieces.push_back(piece);
    pThis->m_response_bytes += piece.len;
    
    return piece.len;
}

