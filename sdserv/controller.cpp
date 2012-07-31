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
#include <kl/md5.h>
#include <kl/portable.h>

Controller::Controller()
{
}

Controller::~Controller()
{
    removeAllServerSessionObjects();
}

bool Controller::onRequest(RequestInfo& req)
{
    std::wstring uri = req.getURI();
    uri = kl::beforeFirst(uri, '?');
    if (uri.length() > 0 && uri[uri.length()-1] == '/')
       uri = uri.substr(0, uri.length()-1);
    
         if (uri == L"/api/login")      apiLogin(req);
    else if (uri == L"/api/selectdb")   apiSelectDb(req);
    else if (uri == L"/api/folderinfo") apiFolderInfo(req);

    else return false;

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
    char buf[255];
    snprintf(buf, 255, "%d.%d.%d", (int)time(NULL), (int)clock(), rand());
    std::wstring session_id = kl::towstring(kl::md5str(buf));

    // create a new session
    SdservSession* session = new SdservSession;
    addServerSessionObject(session_id, session);

    // return success and session information to caller
    JsonNode response;
    response["success"] = true;
    response["session_id"] = session_id;
    req.write(response.toString());
}

void Controller::apiSelectDb(RequestInfo& req)
{
    std::wstring sid = req.getValue(L"sid");
    SdservSession* session = NULL;
    if (!getServerSessionObject(sid, (ServerSessionObject**)&session))
    {
        JsonNode response;
        response["success"] = false;
        response["msg"] = "Invalid session id";
        req.write(response.toString());
        return;
    }

    if (session->db.isNull())
    {
        JsonNode response;
        response["success"] = false;
        response["msg"] = "No database selected";
        req.write(response.toString());
        return;
    }


    tango::IDatabaseMgrPtr dbmgr = tango::getDatabaseMgr();;
    session->db = dbmgr->open(L"xdprovider=xdnative;database=C:\\Users\\bwilliams\\Documents\\Gold Prairie Projects\\Default Project;user id=admin;password=;");

    if (session->db)
    {
        // return success to caller
        JsonNode response;
        response["success"] = true;
        req.write(response.toString());
    } 
     else
    {
        // return failure to caller
        JsonNode response;
        response["success"] = false;
        response["msg"] = "Database could not be opened";
        req.write(response.toString());
    }
}


void Controller::apiFolderInfo(RequestInfo& req)
{
    std::wstring sid = req.getValue(L"sid");
    SdservSession* session = NULL;
    if (!getServerSessionObject(sid, (ServerSessionObject**)&session))
    {
        JsonNode response;
        response["success"] = false;
        response["msg"] = "Invalid session id";
        req.write(response.toString());
        return;
    }

    if (session->db.isNull())
    {
        JsonNode response;
        response["success"] = false;
        response["msg"] = "No database selected";
        req.write(response.toString());
        return;
    }


}
