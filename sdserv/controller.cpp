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
    
         if (uri == L"/api/login")         apiLogin(req);
    else if (uri == L"/api/selectdb")      apiSelectDb(req);
    else if (uri == L"/api/folderinfo")    apiFolderInfo(req);
    else if (uri == L"/api/fileinfo")      apiFileInfo(req);
    else return false;

    return true;
}

bool Controller::getServerSessionObject(const std::wstring& name, ServerSessionObject** obj)
{
    XCM_AUTO_LOCK(m_session_object_mutex);

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
    XCM_AUTO_LOCK(m_session_object_mutex);

    m_session_objects[name] = obj;
}

void Controller::removeServerSessionObject(const std::wstring& name)
{
    XCM_AUTO_LOCK(m_session_object_mutex);

    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();

    it = m_session_objects.find(name);
    if (it == it_end)
        return;
        
    m_session_objects.erase(it);
}

void Controller::removeAllServerSessionObjects()
{
    XCM_AUTO_LOCK(m_session_object_mutex);

    std::map< std::wstring, ServerSessionObject* >::iterator it, it_end;
    it_end = m_session_objects.end();
    
    for (it = m_session_objects.begin(); it != it_end; ++it)
    {
        // free the session objects
        delete it->second;
    }
    
    m_session_objects.clear();
}









void Controller::returnApiError(RequestInfo& req, const char* msg, const char* code)
{
    JsonNode response;
    response["success"].setBoolean(false);
    response["error_code"] = code;
    response["msg"] = msg;
    req.write(response.toString());
}

tango::IDatabasePtr Controller::getSessionDatabase(RequestInfo& req)
{
    std::wstring sid = req.getValue(L"sid");
    SdservSession* session = NULL;
    if (!getServerSessionObject(sid, (ServerSessionObject**)&session))
    {
        returnApiError(req, "Invalid session id");
        return xcm::null;
    }

    if (session->db.isNull())
    {
        returnApiError(req, "No database selected");
        return xcm::null;
    }

    return session->db;
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
    response["success"].setBoolean(true);
    response["session_id"] = session_id;
    req.write(response.toString());
}

void Controller::apiSelectDb(RequestInfo& req)
{
    std::wstring sid = req.getValue(L"sid");
    SdservSession* session = NULL;
    if (!getServerSessionObject(sid, (ServerSessionObject**)&session))
    {
        returnApiError(req, "Invalid session id");
        return;
    }

    tango::IDatabaseMgrPtr dbmgr = tango::getDatabaseMgr();
    if (dbmgr.isNull())
    {
        returnApiError(req, "Missing dbmgr component");
        return;
    }

    session->db = dbmgr->open(L"xdprovider=xdnative;database=C:\\Users\\bwilliams\\Documents\\Gold Prairie Projects\\Default Project;user id=admin;password=;");

    if (session->db)
    {
        // return success to caller
        JsonNode response;
        response["success"].setBoolean(true);
        req.write(response.toString());
    } 
     else
    {
        returnApiError(req, "Database could not be opened");
        return;
    }
}


void Controller::apiFolderInfo(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;

    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    

    // return success to caller
    JsonNode response;
    response["success"].setBoolean(true);
    JsonNode items = response["items"];

    tango::IFileInfoEnumPtr folder_info = db->getFolderInfo(path);
    if (folder_info.isOk())
    {
        size_t i, cnt = folder_info->size();
        for (i = 0; i < cnt; ++i)
        {
            JsonNode item = items.appendElement();

            tango::IFileInfoPtr finfo = folder_info->getItem(i);
            item["name"] = finfo->getName();

            switch (finfo->getType())
            {
                case tango::filetypeFolder: item["type"] = "folder"; break;
                case tango::filetypeNode: item["type"] = "node";     break;
                case tango::filetypeSet: item["type"] = "table";     break;
                case tango::filetypeStream: item["type"] = "stream"; break;
                default: continue;
            }
        }
    }
     else
    {
        returnApiError(req, "Path does not exist");
        return;
    }

    req.write(response.toString());
}



void Controller::apiFileInfo(RequestInfo& req)
{
    tango::IDatabasePtr db = getSessionDatabase(req);
    if (db.isNull())
        return;
    
    if (!req.getValueExists(L"path"))
    {
        returnApiError(req, "Missing path parameter");
        return;
    }
    
    std::wstring path = req.getValue(L"path");
    

    // return success to caller
    JsonNode response;
    response["success"].setBoolean(true);
    JsonNode file_info = response["file_info"];

    tango::IFileInfoPtr finfo = db->getFileInfo(path);
    if (finfo.isOk())
    {
        file_info["name"] = finfo->getName();
        
        switch (finfo->getType())
        {
            case tango::filetypeFolder: file_info["type"] = "folder"; break;
            case tango::filetypeNode: file_info["type"] = "node";     break;
            case tango::filetypeSet: file_info["type"] = "table";     break;
            case tango::filetypeStream: file_info["type"] = "stream"; break;
            default: file_info["type"] = "unknown"; break;
        }

        switch (finfo->getFormat())
        {
            case tango::formatNative:          file_info["format"] = "native";            break;
            case tango::formatXbase:           file_info["format"] = "xbase";             break;
            case tango::formatDelimitedText:   file_info["format"] = "delimited_text";    break;
            case tango::formatFixedLengthText: file_info["format"] = "fixedlength_text";  break;
            case tango::formatText:            file_info["format"] = "fixed_length_text"; break;
            default: file_info["type"] = "unknown"; break;
        }

        file_info["mime_type"] = finfo->getMimeType();
        file_info["is_mount"].setBoolean(finfo->isMount());
        file_info["primary_key"] = finfo->getPrimaryKey();
        file_info["size"] = (double)finfo->getSize();
    }
     else
    {
        returnApiError(req, "Path does not exist");
        return;
    }

    req.write(response.toString());
}
