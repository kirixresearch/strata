/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2013-11-15
 *
 */

 
#ifndef __SDSERV_WEBSOCKETS_H
#define __SDSERV_WEBSOCKETS_H

#include <queue>

struct libwebsocket;
struct libwebsocket_context;


class WebSocketsClient;
class WebSocketsRequestInfo : public RequestInfo
{
    friend class WebSocketsClient;

public:

    WebSocketsRequestInfo(WebSocketsClient* client);

    std::wstring getURI();
    std::wstring getValue(const std::wstring& key, const std::wstring& def = L"");
    bool getValueExists(const std::wstring& key);
    void setValue(const std::wstring& key, const std::wstring& value);

    int getContentLength();

    RequestFileInfo getPostFileInfo(const std::wstring& key);
    
    void setContentType(const char* content_type);
    size_t write(const void* ptr, size_t length);
    size_t write(const std::string& str);
    size_t write(const std::wstring& str);

    void sendNotFoundError();

private:

    WebSocketsClient* m_client;
    std::map<std::wstring, std::wstring> m_params;
    std::wstring m_uri;
    std::string m_token;
    std::string m_content_type;
    int m_content_length;
};



class WebSocketsClient
{
public:

    bool run(const std::string& server, int port = 80, bool ssl = false);

public:

    void onMessage(const std::string& msg);
    void send(const std::string& msg);

public:

    struct libwebsocket_context* m_context;
    struct libwebsocket* m_wsi;
    std::queue<std::string> m_write_bufs;
};



#endif