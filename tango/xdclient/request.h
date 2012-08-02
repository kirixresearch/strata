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


class MethodCall
{
public:

    MethodCall(const std::wstring& method) : m_method(method) { }
    
    void addParam(const std::wstring& param, const std::wstring& value) {
        m_params.push_back(std::pair<std::wstring,std::wstring>(param,value));
    }
    
private:

    std::wstring m_method;
    std::vector< std::pair<std::wstring, std::wstring> > m_params;
};


class MethodResponse
{
public:

    MethodResponse() { }
    
private:

    std::map<std::wstring, std::wstring> m_properties;
};


class RpcConnection
{
public:

    virtual MethodResponse call(const MethodCall& call) = 0;
};



typedef void CURL;


class HttpResponsePiece
{
public:
    unsigned char* buf;
    size_t curpos;
    size_t len;
};



class RpcConnectionHttp
{
public:

    RpcConnectionHttp();
    virtual ~RpcConnectionHttp();
    
    void setUrl(const std::wstring& url) { m_url = url; }
    MethodResponse call(const MethodCall& call);

private:

    void freeResponsePieces();

    static size_t http_header_writer(void* ptr, size_t size, size_t nmemb, void *stream);
    static size_t http_response_writer(void* ptr, size_t size, size_t nmemb, void *stream);

private:

    CURL* m_curl;

    std::list<HttpResponsePiece> m_response_pieces;
    std::string m_response_header;
    size_t m_response_bytes;

    std::wstring m_url;
};






class HttpRequest
{
public:

    HttpRequest();
    virtual ~HttpRequest();

    void setLocation(const std::wstring& location);
    void setPostValue(const std::wstring& key, const std::wstring& value);
    void resetPostParameters();

    void send();
    std::wstring getResponseString();
    size_t getResponseSize();
    void freeResponsePieces();
    void useMultipartPost();

private:

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
};


extern HttpRequest g_httprequest;


#endif

