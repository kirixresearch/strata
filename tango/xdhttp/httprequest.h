/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-25
 *
 */


#ifndef __XDHTTP_HTTPREQUEST_H
#define __XDHTTP_HTTPREQUEST_H


#include <string>
#include <list>


typedef void CURL;


class HttpResponsePiece
{
public:
    unsigned char* buf;
    size_t curpos;
    size_t len;
};

class HttpRequest
{
public:

    HttpRequest();
    virtual ~HttpRequest();

    void setLocation(const std::string& location);

    void send();
    std::wstring getResponseString();
    size_t getResponseSize();
    void freeResponsePieces();

private:

    static size_t http_header_writer(void* ptr, size_t size, size_t nmemb, void *stream);
    static size_t http_response_writer(void* ptr, size_t size, size_t nmemb, void *stream);

private:

    CURL* m_curl;

    std::list<HttpResponsePiece> m_response_pieces;
    std::string m_response_header;
    size_t m_response_bytes;

    std::string m_location;
};


extern HttpRequest g_httprequest;


#endif

