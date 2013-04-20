/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2009-05-22
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

    void setType(const std::string& type) { m_type = type; }
    const std::string& getType() const { return m_type; }
    bool isType(const char* type) const { return m_type == type; }

private:

    std::string m_type;
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
    std::wstring getValue(const std::wstring& key, const std::wstring& def = L"");
    std::wstring getGetValue(const std::wstring& key);
    std::wstring getPostValue(const std::wstring& key);
    void setGetValue(const std::wstring& key, const std::wstring& value) { m_get[key] = value; }
    bool getValueExists(const std::wstring& key) const;
    bool acceptCompressed();
    
    std::map<std::wstring, std::wstring>& getCookies() { return m_cookies; }
    std::map<std::wstring, std::wstring>& getGetValues() { return m_get; }
    std::map<std::wstring, RequestPostInfo>& getPostValues() { return m_post; }
    std::map<std::wstring, RequestPostInfo>& getFileValues() { return m_files; }
    
    //void writeResultCode(int code, const char* mime_type = NULL);
    void setStatusCode(int code, const char* msg = NULL);
    void setContentType(const char* content_type);
    void setContentLength(int length = -1);
    int getContentLength() const { return m_content_length; }
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
    int m_content_length;
    bool m_header_sent;
    bool m_accept_compressed;
    
    kl::membuf m_post_data_buf;
};





#endif

