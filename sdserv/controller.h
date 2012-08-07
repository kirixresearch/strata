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



struct QueryResult
{
    tango::IIteratorPtr iter;
    std::vector<tango::objhandle_t> handles;
    tango::rowpos_t rowpos;
};

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
    std::map<std::wstring, tango::IStreamPtr> streams;
    std::map<std::wstring, tango::IRowInserterPtr> inserters;
    std::map<std::wstring, QueryResult> iters;
};


class Controller
{
public:

    Controller();
    virtual ~Controller();

    bool onRequest(RequestInfo& ri);

private:

    ServerSessionObject* getServerSessionObject(const std::wstring& name);
    void addServerSessionObject(const std::wstring& name, ServerSessionObject* obj);
    void removeServerSessionObject(const std::wstring& name);
    void removeAllServerSessionObjects();
    std::wstring createHandle() const;
    
private:

    void returnApiError(RequestInfo& req, const char* msg, const char* code = "ERR0000");
    tango::IDatabasePtr getSessionDatabase(RequestInfo& req);
    SdservSession* getSdservSession(RequestInfo& req);
    
    void apiLogin(RequestInfo& req);
    void apiSelectDb(RequestInfo& req);
    void apiFolderInfo(RequestInfo& req);
    void apiFileInfo(RequestInfo& req);
    void apiCreateStream(RequestInfo& req);
    void apiCreateTable(RequestInfo& req);
    void apiOpenStream(RequestInfo& req);
    void apiReadStream(RequestInfo& req);
    void apiWriteStream(RequestInfo& req);
    void apiQuery(RequestInfo& req);
    void apiDescribeTable(RequestInfo& req);
    void apiFetchRows(RequestInfo& req);
    void apiStartBulkInsert(RequestInfo& req);
    void apiFinishBulkInsert(RequestInfo& req);
    void apiBulkInsert(RequestInfo& req);

private:

    std::map< std::wstring , ServerSessionObject* > m_session_objects;
    xcm::mutex m_session_object_mutex;

    tango::IDatabasePtr m_db;
};


#endif
