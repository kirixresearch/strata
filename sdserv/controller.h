/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#ifndef __SDSERV_CONTROLLER_H
#define __SDSERV_CONTROLLER_H

class RequestInfo;
class ServerSessionObject;

class Controller
{
public:

    Controller();
    virtual ~Controller();

    void onRequest(RequestInfo& ri);

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
};


#endif
