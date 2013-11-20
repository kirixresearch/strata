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


#include "http.h"


struct SessionQueryResultColumn
{
    xd::objhandle_t handle;
    std::wstring name;
    int type;
    int width;
    int scale;
};

struct SessionQueryResult : public ServerSessionObject
{
    SessionQueryResult() : ServerSessionObject() { setType("SessionQueryResult"); }

    xd::IIteratorPtr iter;
    std::vector<SessionQueryResultColumn> columns;
    xd::rowpos_t rowpos;
    xd::rowpos_t rowcount;
};

struct SessionStream : public ServerSessionObject
{
    SessionStream() : ServerSessionObject() { setType("SessionStream"); }

    xd::IStreamPtr stream;
};


struct SessionRowInserterColumn
{
    xd::objhandle_t handle;
    int type;
};

struct SessionRowInserter : public ServerSessionObject
{
    SessionRowInserter() : ServerSessionObject() { setType("SessionRowInserter"); }

    xd::IRowInserterPtr inserter;
    std::vector<SessionRowInserterColumn> columns;
};



class Controller
{
public:

    Controller();
    virtual ~Controller();

    bool onRequest(RequestInfo& ri);
    void setConnectionString(const std::wstring& cstr) { m_connection_string = cstr; }

private:

    ServerSessionObject* getServerSessionObject(const std::wstring& name, const char* type_check = NULL);
    void addServerSessionObject(const std::wstring& name, ServerSessionObject* obj);
    void removeServerSessionObject(const std::wstring& name);
    void removeAllServerSessionObjects();
    std::wstring createHandle() const;
    
private:

    void returnApiError(RequestInfo& req, const char* msg, const char* code = "ERR0000");
    xd::IDatabasePtr getSessionDatabase(RequestInfo& req);

    void apiFolderInfo(RequestInfo& req);
    void apiFileInfo(RequestInfo& req);
    void apiCreateStream(RequestInfo& req);
    void apiCreateTable(RequestInfo& req);
    void apiCreateFolder(RequestInfo& req);
    void apiMoveFile(RequestInfo& req);
    void apiRenameFile(RequestInfo& req);
    void apiDeleteFile(RequestInfo& req);
    void apiCopyData(RequestInfo& req);
    void apiOpenStream(RequestInfo& req);
    void apiReadStream(RequestInfo& req);
    void apiWriteStream(RequestInfo& req);
    void apiQuery(RequestInfo& req);
    void apiGroupQuery(RequestInfo& req);
    void apiDescribeTable(RequestInfo& req);
    void apiGetData(RequestInfo& req);
    void apiRead(RequestInfo& req);
    void apiInsertRows(RequestInfo& req);
    void apiClone(RequestInfo& req);
    void apiClose(RequestInfo& req);
    void apiAlter(RequestInfo& req);
    void apiLoad(RequestInfo& req);
    void apiImportUpload(RequestInfo& req);
    void apiImportLoad(RequestInfo& req);
    void apiJobInfo(RequestInfo& req);

private:

    std::map< std::wstring , ServerSessionObject* > m_session_objects;
    xcm::mutex m_session_object_mutex;

    std::map< std::wstring , xd::IDatabasePtr > m_databases;
    xcm::mutex m_databases_object_mutex;
    xd::IDatabasePtr m_database;
    std::wstring m_connection_string;

    xcm::mutex m_job_info_mutex;
    std::vector<jobs::IJobInfoPtr> m_job_info_vec;
};

extern Controller g_controller;

#endif
