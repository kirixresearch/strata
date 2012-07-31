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

void Controller::onRequest(RequestInfo& ri)
{
    // get the intial time at the start of the request
    clock_t time_start = ::clock();

    std::wstring uri = L"";
    std::wstring ext = kl::afterLast(uri, '.');

    tango::IDatabasePtr db;

    tango::IFileInfoPtr file;
    if (db)
        file = db->getFileInfo(uri);

    if (file.isNull())
    {
        ri.setStatusCode(404); // 404 Not Found
    }
    else if (file->getMimeType() == L"text/html" || ext == L"html" || ext == L"htm")
    {
        handleHtmlResponse(uri, ri);
    }
    else if (ext == L"sjs")
    {
        handleScriptResponse(uri, ri);
    }
    else if (ext == L"pdf")
    {
        // TODO: remove when file->getFileType() is fixed for
        // externally mounted PDFs

        // default handler is stream; but right now, pdfs in external
        // folders return filetype of tango::filetypeSet, and we don't
        // want to serve these as tables, so we need to preempt the
        // table handler
        handleStreamResponse(uri, ri);
    }
    else if (file->getType() == tango::filetypeFolder)
    {
        handleFolderResponse(uri, ri);
    }
    else if (file->getType() == tango::filetypeSet)
    {
        handleTableResponse(uri, ri);
    }
    else
    {
        handleStreamResponse(uri, ri);
    }

    // get the intial time at the end of the request
    clock_t time_end = ::clock();
    double total_time = (double)(time_end - time_start)/CLOCKS_PER_SEC;
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

void Controller::handleFolderResponse(const std::wstring& uri, RequestInfo& ri)
{
    ri.setStatusCode(200);
    ri.setContentType("text/html");
    ri.write("");
}

void Controller::handleTableResponse(const std::wstring& uri, RequestInfo& ri)
{
    ri.setStatusCode(200);
    ri.setContentType("text/html");
    ri.write("");
}

void Controller::handleHtmlResponse(const std::wstring& uri, RequestInfo& ri)
{
    ri.setStatusCode(200);
    ri.setContentType("text/html");
    ri.write("");
}

void Controller::handleScriptResponse(const std::wstring& uri, RequestInfo& ri)
{
    ri.setStatusCode(200);
    ri.setContentType("text/html");
    ri.write("");
}

void Controller::handleStreamResponse(const std::wstring& uri, RequestInfo& ri)
{
    ri.setStatusCode(200);
    ri.setContentType("text/html");
    ri.write("");
}
