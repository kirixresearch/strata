/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#ifndef __XDPGSQL_DATABASE_H
#define __XDPGSQL_DATABASE_H



#include "../xdcommon/errorinfo.h"

std::wstring pgsqlGetTablenameFromPath(const std::wstring& path);

xd::IColumnInfoPtr pgsqlCreateColInfo(const std::wstring& col_name,
                                         int col_odbc_type,
                                         int col_width,
                                         int col_scale,
                                         const std::wstring& col_expr,
                                         int datetime_sub);

int pgsqlToXdType(int pg_type);

std::wstring pgsqlQuoteIdentifier(const std::wstring& str);
std::wstring pgsqlQuoteIdentifierIfNecessary(const std::wstring& str);



class PgsqlJobInfo;


class PgsqlDatabase : public xd::IDatabase
{
friend class PgsqlRowInserter;
friend class PgsqlIterator;
friend class PgsqlStream;
friend class PgsqlFileInfo;

    XCM_CLASS_NAME("xdpgsql.Database")
    XCM_BEGIN_INTERFACE_MAP(PgsqlDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
    XCM_END_INTERFACE_MAP()

public:

    PgsqlDatabase();
    ~PgsqlDatabase();
    
    bool open(const std::wstring& server,
              int port,
              const std::wstring& database,
              const std::wstring& username,
              const std::wstring& password);

    void close();

    int getDatabaseType();
    xd::IAttributesPtr getAttributes();
    std::wstring getActiveUid();
    
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    bool cleanup();

    xd::IJobPtr createJob();

    xd::IDatabasePtr getMountDatabase(const std::wstring& path);

    bool setMountPoint(const std::wstring& path,
                       const std::wstring& connection_str,
                       const std::wstring& remote_path);
                                  
    bool getMountPoint(const std::wstring& path,
                       std::wstring& connection_str,
                       std::wstring& remote_path);

    bool createFolder(const std::wstring& path);
    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_location);
    bool copyFile(const std::wstring& path, const std::wstring& dest_path);
    bool copyData(const xd::CopyParams* info, xd::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    xd::IFileInfoPtr getFileInfo(const std::wstring& path);
    xd::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);
    std::wstring getPrimaryKey(const std::wstring table);
    
    xd::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, xd::IStructurePtr struct_config, xd::FormatInfo* format_info);
    xd::IStreamPtr openStream(const std::wstring& path);
    bool createStream(const std::wstring& path, const std::wstring& mime_type);

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

    xd::IRowInserterPtr bulkInsert(const std::wstring& path);

    xd::IStructurePtr describeTable(const std::wstring& path);

    bool modifyStructure(const std::wstring& path, xd::IStructurePtr struct_config, xd::IJob* job);

    bool execute(const std::wstring& command,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 xd::IJob* job);

    bool groupQuery(xd::GroupQueryParams* info, xd::IJob* job);

private:
 
    std::wstring getServer();
    std::wstring getPath();
    
    std::wstring getTempFileDirectory();
    std::wstring getDefinitionDirectory();
    
    PGconn* createConnection();
    void closeConnection(PGconn* conn);

private:

    xd::IAttributesPtr m_attr;

    int m_port;
    std::wstring m_server;
    std::wstring m_database;
    std::wstring m_username;
    std::wstring m_password;
    std::wstring m_path;
    
    xcm::mutex m_jobs_mutex;
    int m_last_job;
    std::vector<PgsqlJobInfo*> m_jobs;

    ThreadErrorInfo m_error;
};




#endif




