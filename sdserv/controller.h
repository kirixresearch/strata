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


#include "request.h"


class SdservSession : public ServerSessionObject
{
public:

    SdservSession()
    {
    }
    
    virtual ~SdservSession()
    {
    }

public:

    tango::IDatabasePtr db;
};


class Controller
{
public:

    Controller();
    virtual ~Controller();

    bool onRequest(RequestInfo& ri);

    bool getServerSessionObject(const std::wstring& name, ServerSessionObject** obj);
    void addServerSessionObject(const std::wstring& name, ServerSessionObject* obj);
    void removeServerSessionObject(const std::wstring& name);
    void removeAllServerSessionObjects();
    
private:

    void apiLogin(RequestInfo& req);
    void apiSelectDb(RequestInfo& req);
    void apiFolderInfo(RequestInfo& req);
                   
private:

    std::map< std::wstring , ServerSessionObject* > m_session_objects;
    tango::IDatabasePtr m_db;
};


#endif
