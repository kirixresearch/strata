/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2009-05-22
 *
 */


#ifndef __SDSERVLIB_HTTP_H
#define __SDSERVLIB_HTTP_H

#include <kl/memory.h>
#include "request.h"

struct mg_context;
struct mg_request_info;
struct mg_connection;


class RequestPostInfo
{
public:
    std::wstring value;
};


class ResponseCookie
{
public:
    std::string name;
    std::string value;
    time_t expire_time;
};



class HttpRequestInfo : public RequestInfo
{
public:

    HttpRequestInfo(struct mg_connection* conn, const struct mg_request_info* ri);

    virtual ~HttpRequestInfo();

    void read();
    
    bool isHTTP_1_1() const;
    std::wstring getHost();
    std::wstring getURI();
    std::wstring getQuery();
    std::wstring getValue(const std::wstring& key, const std::wstring& def = L"");
    std::wstring getGetValue(const std::wstring& key);
    std::wstring getPostValue(const std::wstring& key);
    RequestFileInfo getPostFileInfo(const std::wstring& key);
    bool movePostFile(const std::wstring& key, const std::wstring& dest_path);
    void setValue(const std::wstring& key, const std::wstring& value) { m_get[key] = value; }
    bool getValueExists(const std::wstring& key);
    bool acceptCompressed();
    
    void sendNotFoundError();
    void setStatusCode(int code, const char* msg = NULL);
    void setContentType(const char* content_type);
    void setContentLength(int length = -1);
    int getContentLength() { return m_content_length; }
    void redirect(const char* location, int http_code = 301);
    void addHeader(const char* header);
    void addCookie(ResponseCookie& cookie);
    void addCookie(const std::string& name, const std::string& value, time_t expire_time);
    
    size_t write(const void* ptr, size_t length);
    size_t write(const std::string& str);
    size_t write(const std::wstring& str);

    size_t writePiece(const void* ptr, size_t length);
    size_t writePiece(const std::string& str);
    size_t writePiece(const std::wstring& str);

    std::wstring getMethod() const;
    bool isMethodGet() const;
    bool isMethodPost() const;

protected:

    void readMultipart(const char* boundary, size_t boundary_length);
    void checkHeaderSent();

private:

    struct mg_connection* m_conn;
    const struct mg_request_info* m_req;
    std::map<std::wstring, std::wstring> m_get;
    std::map<std::wstring, RequestPostInfo> m_post;
    std::map<std::wstring, RequestFileInfo> m_files;
    std::map<std::wstring, std::wstring> m_cookies;

    std::vector<std::string> m_headers;
    std::vector<ResponseCookie> m_response_cookies;  
    std::string m_content_type;    
      
    int m_status_code;
    int m_content_length;
    bool m_header_sent;
    bool m_accept_compressed;
    
    kl::membuf m_post_data_buf;
};



class HttpServer
{
public:

    HttpServer(Sdserv* sdserv) { m_sdserv = sdserv; }
    void run(const char* options[]);

private:
    static int HttpServer::request_callback(struct mg_connection* conn);

private:

    Sdserv* m_sdserv;
};



#endif

