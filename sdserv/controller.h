/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Server
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#ifndef __SDSERV_CONTROLLER_H
#define __SDSERV_CONTROLLER_H


#include "request.h"


struct SessionQueryResultColumn
{
    tango::objhandle_t handle;
    int type;
    int width;
    int scale;
};

struct SessionQueryResult : public ServerSessionObject
{
    tango::IIteratorPtr iter;
    std::vector<SessionQueryResultColumn> columns;
    tango::rowpos_t rowpos;
};


struct SessionRowInserterColumn : public ServerSessionObject
{
    tango::objhandle_t handle;
    int type;
};

struct SessionRowInserter : public ServerSessionObject
{
    tango::IRowInserterPtr inserter;
    std::vector<SessionRowInserterColumn> columns;
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
    tango::IIteratorPtr getSessionIterator(RequestInfo& req);

    void apiFolderInfo(RequestInfo& req);
    void apiFileInfo(RequestInfo& req);
    void apiCreateStream(RequestInfo& req);
    void apiCreateTable(RequestInfo& req);
    void apiCreateFolder(RequestInfo& req);
    void apiMoveFile(RequestInfo& req);
    void apiRenameFile(RequestInfo& req);
    void apiDeleteFile(RequestInfo& req);
    void apiOpenStream(RequestInfo& req);
    void apiReadStream(RequestInfo& req);
    void apiWriteStream(RequestInfo& req);
    void apiQuery(RequestInfo& req);
    void apiGroupQuery(RequestInfo& req);
    void apiDescribeTable(RequestInfo& req);
    void apiFetchRows(RequestInfo& req);
    void apiInsertRows(RequestInfo& req);
    void apiClone(RequestInfo& req);
    void apiClose(RequestInfo& req);
    void apiAlter(RequestInfo& req);
    void apiRefresh(RequestInfo& req);
    void apiStartBulkInsert(RequestInfo& req);
    void apiFinishBulkInsert(RequestInfo& req);
    void apiBulkInsert(RequestInfo& req);

private:

    std::map< std::wstring , ServerSessionObject* > m_session_objects;
    xcm::mutex m_session_object_mutex;

    std::map< std::wstring , tango::IDatabasePtr > m_databases;
    xcm::mutex m_databases_object_mutex;
};


#endif
