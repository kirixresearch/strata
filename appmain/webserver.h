/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#ifndef __APP_WEBSERVER_H
#define __APP_WEBSERVER_H


struct mg_context;
struct mg_request_info;
struct mg_connection;


class RequestPostInfo
{
public:
    RequestPostInfo();
    bool saveToFile(const std::wstring& path);
    
public:
    const char* data;
    size_t length;
    std::wstring str;
};


class ResponseCookie
{
public:
    std::string name;
    std::string value;
    time_t expire_time;
};


class ServerSessionObject
{
// session object for storing information that
// can be accessed across multiple calls

public:

    ServerSessionObject() {}
    virtual ~ServerSessionObject() {}

    void setType(const std::wstring& type) {m_type = type;}
    std::wstring getType() {return m_type;}

private:

    std::wstring m_type;
};


class RequestInfo
{
public:

    RequestInfo(struct mg_connection* conn,
                const struct mg_request_info* ri);
    virtual ~RequestInfo();

    void parse();
    
    std::wstring getURI();
    std::wstring getQuery();
    std::wstring getValue(const std::wstring& key);
    std::wstring getGetValue(const std::wstring& key);
    std::wstring getPostValue(const std::wstring& key);
    bool acceptCompressed();
    
    std::map<std::wstring, std::wstring>& getCookies() { return m_cookies; }
    std::map<std::wstring, std::wstring>& getGetValues() { return m_get; }
    std::map<std::wstring, RequestPostInfo>& getPostValues() { return m_post; }
    std::map<std::wstring, RequestPostInfo>& getFileValues() { return m_files; }
    
    //void writeResultCode(int code, const char* mime_type = NULL);
    void setStatusCode(int code, const char* msg = NULL);
    void setContentType(const char* content_type);
    void redirect(const char* location, int http_code = 301);
    void addHeader(const char* header);
    void addCookie(ResponseCookie& cookie);
    
    size_t write(const void* ptr, size_t length);
    
    std::wstring getMethod() const;
    bool isMethodGet() const;
    bool isMethodPost() const;

protected:

    bool parsePart(const char* p,
                   const char* boundary,
                   size_t boundary_size,
                   const char* endp,
                   const char** next);

    void checkHeaderSent();

private:

    struct mg_connection* m_conn;
    const struct mg_request_info* m_req;
    std::map<std::wstring, std::wstring> m_get;
    std::map<std::wstring, RequestPostInfo> m_post;
    std::map<std::wstring, RequestPostInfo> m_files;
    std::map<std::wstring, std::wstring> m_cookies;

    std::vector<std::string> m_headers;
    std::vector<ResponseCookie> m_response_cookies;  
    std::string m_content_type;    
      
    int m_status_code;
    bool m_header_sent;
    bool m_accept_compressed;
};



class WebServer
{
public:

    WebServer();
    virtual ~WebServer();
    
    void start();
    void stop();
    bool isRunning() const;

protected:

    void onRequest(RequestInfo& ri);
    
    // this static callback routes the handler to onRequest()
    static void request_handler(
                   struct mg_connection* conn,
                   const struct mg_request_info* ri,
                   void* user_data);

    bool getServerSessionObject(const std::wstring& name, ServerSessionObject** obj);
    void addServerSessionObject(const std::wstring& name, ServerSessionObject* obj);
    void removeServerSessionObject(const std::wstring& name);
    void removeAllServerSessionObjects();
                   
private:

    void handleFolderResponse(const std::wstring& uri, RequestInfo& ri);
    void handleTableResponse(const std::wstring& uri, RequestInfo& ri);
    void handleHtmlResponse(const std::wstring& uri, RequestInfo& ri);
    void handleScriptResponse(const std::wstring& uri, RequestInfo& ri);
    void handleStreamResponse(const std::wstring& uri, RequestInfo& ri);
                   
private:

    std::map< std::wstring , ServerSessionObject* > m_session_objects;
    struct mg_context* m_ctx;
};


#endif

