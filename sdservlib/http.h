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
friend class HttpServer;

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
    void setPostHook(PostHookBase* hook) { m_post_hook = hook; } // will take ownership of hook object
    bool isMultipart() { return (m_boundary_length > 0) ? true : false; }

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

    void setError(bool error) { m_error = true; }
    bool getError() { return m_error; }

protected:

    void readPost();
    void readMultipart();
    void checkHeaderSent();
    void checkReadRequestBody();

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
    std::wstring m_uri; 
      
    int m_status_code;
    int m_content_length;
    bool m_header_sent;
    bool m_accept_compressed;

    PostHookBase* m_post_hook;
    bool m_request_post_read_invoked;
    int m_request_content_length;
    int m_request_bytes_read;

    char* m_boundary;
    size_t m_boundary_length;

    int m_strip_path;

    bool m_error;
};


struct HttpListeningPort
{
    int port;
    bool ssl;
};

class HttpServer
{
public:

    HttpServer(Sdserv* sdserv);
    bool run();
    const std::vector<HttpListeningPort>& getListeningPorts() { return m_listening_ports; }

private:
    static int request_callback(struct mg_connection* conn);

private:

    Sdserv* m_sdserv;
    char m_ports[255];
    char m_ssl_cert_file[255];

    const char* m_options[255];
    size_t m_options_arr_size;

    int m_strip_path;

    std::vector<HttpListeningPort> m_listening_ports;
};



#endif

