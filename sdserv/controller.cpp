/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */

#include "sdserv.h"
#include "controller.h"
#include "request.h"
#include "jsonconfig.h"
#include <kl/string.h>


Controller::Controller()
{
}

Controller::~Controller()
{
}

bool Controller::onRequest(RequestInfo& req)
{
    std::wstring uri = req.getURI();
    uri = kl::beforeFirst(uri, '?');
    if (uri.length() > 0 && uri[uri.length()-1] == '/')
       uri = uri.substr(0, uri.length()-1);
    
    if (uri == L"/api/login")
    {
        apiLogin(req);
        return true;
    }
     else
    {
        return false;
    }


    return true;
}

bool Controller::getServerSessionObject(const std::wstring& name, ServerSessionObject** obj)
{
    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();

    it = m_session_objects.find(name);
    if (it == it_end)
        return false;

    *obj = it->second;
    return true;
}

void Controller::addServerSessionObject(const std::wstring& name, ServerSessionObject* obj)
{
    m_session_objects[name] = obj;
}

void Controller::removeServerSessionObject(const std::wstring& name)
{
    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();

    it = m_session_objects.find(name);
    if (it == it_end)
        return;
        
    m_session_objects.erase(it);
}

void Controller::removeAllServerSessionObjects()
{
    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();
    
    for (it = m_session_objects.begin(); it != it_end; ++it)
    {
        // free the session objects
        delete it->second;
    }
    
    m_session_objects.clear();
}














void Controller::apiLogin(RequestInfo& req)
{
    JsonNode response;
    response["success"] = true;
    response["session_id"] = 123;
    req.write(response.toString());
}
