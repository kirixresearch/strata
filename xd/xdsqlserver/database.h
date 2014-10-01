/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-03-17
 *
 */


#ifndef __XDSQLSERVER_DATABASE_H
#define __XDSQLSERVER_DATABASE_H


#include "../xdcommon/errorinfo.h"


// utility functions

std::wstring createSqlServerFieldString(const std::wstring& name,
                                        int type,
                                        int width,
                                        int scale,
                                        bool null);


class SqlServerDatabase : public xd::IDatabase
{
    XCM_CLASS_NAME("xdsqlserver.Database")
    XCM_BEGIN_INTERFACE_MAP(SqlServerDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
    XCM_END_INTERFACE_MAP()

public:

    SqlServerDatabase();
    ~SqlServerDatabase();
    
    bool open(const std::wstring& server,
              int port,
              const std::wstring& database,
              const std::wstring& username,
              const std::wstring& password);

    // xd::IDatabase

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
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool copyData(const xd::CopyParams* info, xd::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    xd::IFileInfoPtr getFileInfo(const std::wstring& path);
    xd::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);

    xd::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, const xd::FormatDefinition& format_info);
    xd::IStreamPtr openStream(const std::wstring& ofs_path);
    bool createStream(const std::wstring& ofs_path, const std::wstring& mime_type);

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

    xd::IRowInserterPtr bulkInsert(const std::wstring& path);

    xd::IStructurePtr describeTable(const std::wstring& path);

    bool modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job);

    bool execute(const std::wstring&,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 xd::IJob* job);

private:

    xd::IAttributesPtr m_attr;
    
    TDSLOGIN* m_login;
    TDSCONTEXT* m_context;
    TDSCONNECTION* m_connect_info;

    std::wstring m_server;
    int m_port;
    std::wstring m_database;
    std::wstring m_username;
    std::wstring m_password;

    ThreadErrorInfo m_error;
};


#endif



