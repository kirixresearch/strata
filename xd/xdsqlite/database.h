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


#include "sqlite3.h"
#include "../xdcommon/errorinfo.h"

std::wstring sqliteGetTablenameFromPath(const std::wstring& path);

class JobInfo;
class SlDatabase : public xd::IDatabase
{
    friend class SlRowInserter;
    friend class SlIterator;

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

    // xd::IDatabase interface

    void close();

    void setDatabaseName(const std::wstring& name);
    std::wstring getDatabaseName();
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

    xd::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, xd::IStructurePtr struct_config, xd::FormatInfo* format_info);
 
    xd::IStreamPtr openStream(const std::wstring& path);

    xd::IIteratorPtr query(const xd::QueryParams& qp);

    xd::IIndexInfoPtr createIndex(const std::wstring& path,
                                     const std::wstring& name,
                                     const std::wstring& expr,
                                     xd::IJob* job);
    bool renameIndex(const std::wstring& path,
                     const std::wstring& name,
                     const std::wstring& new_name);
    bool deleteIndex(const std::wstring& path,
                     const std::wstring& name);
    xd::IIndexInfoEnumPtr getIndexEnum(const std::wstring& path);


    xd::IDatabasePtr getMountDatabase(const std::wstring& path);
    
    bool setMountPoint(const std::wstring& path,
                       const std::wstring& connection_str,
                       const std::wstring& remote_path);
                                  
    bool getMountPoint(const std::wstring& path,
                       std::wstring& connection_str,
                       std::wstring& remote_path);
    
    xd::IRowInserterPtr bulkInsert(const std::wstring& path);
          
    xd::IStructurePtr describeTable(const std::wstring& path);

    bool modifyStructure(const std::wstring& path, xd::IStructurePtr struct_config, xd::IJob* job);

    bool execute(const std::wstring& command,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 xd::IJob* job);
    
    bool groupQuery(xd::GroupQueryParams* info, xd::IJob* job);

private:

    std::wstring m_path;
    std::wstring m_db_name;

    sqlite3* m_sqlite;
    int m_last_job;
    std::vector<JobInfo*> m_jobs;
    
    ThreadErrorInfo m_error;
};




#endif
