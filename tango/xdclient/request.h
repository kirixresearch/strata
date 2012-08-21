/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-25
 *
 */


#ifndef __XDCLIENT_REQUEST_H
#define __XDCLIENT_REQUEST_H


#include <string>
#include <list>
#include <map>
#include <vector>


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

    void setLocation(const std::wstring& location);
    void setTimeout(int timeout);
    void setPostValue(const std::wstring& key, const std::wstring& value);
    void resetPostParameters();

    void send();
    std::wstring getResponseString();
    size_t getResponseSize();
    void freeResponsePieces();
    void useMultipartPost();

    void close();

private:

    void init();

    static size_t http_header_writer(void* ptr, size_t size, size_t nmemb, void *stream);
    static size_t http_response_writer(void* ptr, size_t size, size_t nmemb, void *stream);

private:

    CURL* m_curl;

    std::list<HttpResponsePiece> m_response_pieces;
    std::string m_response_header;
    size_t m_response_bytes;

    std::string m_location;

    struct curl_httppost* m_formfields;      // only used by multipart post
    struct curl_httppost* m_formfieldslast;  // only used by multipart post
    std::string m_post_string;               // only used by regular post
    bool m_post_multipart;
    int m_timeout;
};


#endif

