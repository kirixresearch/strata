/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2013-11-15
 *
 */

 
#ifndef __SDSERVLIB_WEBSOCKETS_H
#define __SDSERVLIB_WEBSOCKETS_H

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
    std::wstring getGetValue(const std::wstring& key) { return getValue(key); }
    std::wstring getValue(const std::wstring& key, const std::wstring& def = L"");
    bool getValueExists(const std::wstring& key);
    void setValue(const std::wstring& key, const std::wstring& value);
    int getContentLength();

    void setPostHook(PostHookBase* hook) { }
    void readPost() { }
    bool isMultipart() { return false; }

    RequestFileInfo getPostFileInfo(const std::wstring& key);
    
    void setContentType(const char* content_type);
    void setContentLength(int length) { }
    size_t writePiece(const void* ptr, size_t length);
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

    WebSocketsClient(Sdserv* sdserv)
    {
        m_sdserv = sdserv;
        m_publish_flag = 0;
        m_write_flag = 0;
    }

    bool run(const std::string& server, int port = 80, bool ssl = false, const std::string& path = "/");

public:

    void onMessage(const std::string& msg);
    void send(const std::string& msg);

    void updateAssetInformation();

public:

    Sdserv* m_sdserv;
    struct libwebsocket_context* m_context;
    struct libwebsocket* m_wsi;
    kl::mutex m_write_bufs_mutex;
    std::queue<std::string> m_write_bufs;
    int m_publish_flag;
    int m_write_flag;
    std::wstring m_session;
    std::wstring m_connection;

private:

    void doPublishAssets();

};



#endif
