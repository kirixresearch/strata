/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#ifndef __XDSQLITE_DATABASE_H
#define __XDSQLITE_DATABASE_H

#define SQLITE_THREADSAFE 1
#include "sqlite3.h"
#include "../xdcommon/errorinfo.h"

std::wstring sqliteGetTablenameFromPath(const std::wstring& path, bool quote = false);

class JobInfo;
class SlConnectionPool;

class SlDatabase : public xd::IDatabase
{
    friend class SlRowInserter;
    friend class SlIterator;
    friend class SlStream;

    XCM_CLASS_NAME("xdsqlite.Database")
    XCM_BEGIN_INTERFACE_MAP(SlDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
    XCM_END_INTERFACE_MAP()

public:

    SlDatabase();
    ~SlDatabase();

    bool createDatabase(const std::wstring& path);

    bool openDatabase(const std::wstring& path,
                      const std::wstring& username,
                      const std::wstring& password);

    sqlite3* getPoolDatabase();
    void freePoolDatabase(sqlite3* db);

    // xd::IDatabase interface

    void close();

    int getDatabaseType();
    xd::IAttributesPtr getAttributes();
    std::wstring getActiveUid();
    
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    bool cleanup();

    xd::IJobPtr createJob();

    bool createFolder(const std::wstring& path);
    bool createStream(const std::wstring& path, const std::wstring& mime_type);
    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_location);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool copyData(const xd::CopyParams* info, xd::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    xd::IFileInfoPtr getFileInfo(const std::wstring& path);
    xd::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);

    bool createTable(const std::wstring& path, const xd::FormatDefinition& format_definition);
 
    xd::IStreamPtr openStream(const std::wstring& path);

    xd::IIteratorPtr query(const xd::QueryParams& qp);

    xd::IndexInfo createIndex(const std::wstring& path,
                              const std::wstring& name,
                              const std::wstring& expr,
                              xd::IJob* job);
    bool renameIndex(const std::wstring& path,
                     const std::wstring& name,
                     const std::wstring& new_name);
    bool deleteIndex(const std::wstring& path,
                     const std::wstring& name);
    xd::IndexInfoEnum getIndexEnum(const std::wstring& path);


    xd::IDatabasePtr getMountDatabase(const std::wstring& path);
    
    bool setMountPoint(const std::wstring& path,
                       const std::wstring& connection_str,
                       const std::wstring& remote_path);
                                  
    bool getMountPoint(const std::wstring& path,
                       std::wstring& connection_str,
                       std::wstring& remote_path);
    
    xd::IRowInserterPtr bulkInsert(const std::wstring& path);

    xd::Structure describeTable(const std::wstring& path);   

    bool modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job);

    bool execute(const std::wstring& command,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 xd::IJob* job);
    
    bool groupQuery(xd::GroupQueryParams* info, xd::IJob* job);

private:

    SlConnectionPool* m_connection_pool;

    std::wstring m_path;
    std::wstring m_db_name;

    sqlite3* m_sqlite;
    int m_last_job;
    std::vector<JobInfo*> m_jobs;
    
    ThreadErrorInfo m_error;
};




#endif

