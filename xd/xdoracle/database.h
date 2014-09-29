/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-04-18
 *
 */


#ifndef __XDORACLE_DATABASE_H
#define __XDORACLE_DATABASE_H


#include "../xdcommon/errorinfo.h"


// utility functions


inline bool oraSucceeded(sword res)
{
    return (res == OCI_SUCCESS || res == OCI_SUCCESS_WITH_INFO) ? true : false;
}

int oracle2xdType(int oracle_type, int oracle_charset = SQLCS_IMPLICIT);
int xd2oracleType(int xd_type);

std::wstring createOracleFieldString(const std::wstring& name,
                                     int type,
                                     int width,
                                     int scale,
                                     bool null);
bool isSafeFieldname(const std::wstring& s);

xd::ColumnInfo createColInfo(const std::wstring& col_name,
                             int oracle_type,
                             int oracle_charset,
                             int width,
                             int precision,
                             int scale,
                             const std::wstring& expr = L"");

xcm_interface IOracleDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdoracle.IOracleDatabase")

public:

    virtual std::wstring getServer() = 0;
    virtual std::wstring getDatabase() = 0;
};

XCM_DECLARE_SMARTPTR(IOracleDatabase)




class OracleDatabase : public xd::IDatabase,
                       public IOracleDatabase
{
    friend class OracleSet;
    friend class OracleIterator;
    friend class OracleRowInserter;

    XCM_CLASS_NAME("xdoracle.Database")
    XCM_BEGIN_INTERFACE_MAP(OracleDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
        XCM_INTERFACE_ENTRY(IOracleDatabase);
    XCM_END_INTERFACE_MAP()

public:

    OracleDatabase();
    ~OracleDatabase();
    
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
    bool moveFile(const std::wstring& path, const std::wstring& new_folder);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool copyData(const xd::CopyParams* info, xd::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    xd::IFileInfoPtr getFileInfo(const std::wstring& path);
    xd::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);
    std::wstring getPrimaryKey(const std::wstring& table);

    xd::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, xd::IStructurePtr struct_config, const xd::FormatDefinition* format_info);
    xd::IStreamPtr openStream(const std::wstring& path);
    bool createStream(const std::wstring& path, const std::wstring& mime_type);

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

    bool modifyStructure(const std::wstring& path, xd::IStructurePtr struct_config, xd::IJob* job);

    bool execute(const std::wstring& command,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 xd::IJob* job);

    bool groupQuery(xd::GroupQueryParams* info, xd::IJob* job);

private:

    std::wstring getServer() { return m_server; }
    std::wstring getDatabase() { return m_database; }
    
    sword checkerr(OCIError* err, sword status);
    
private:

    xd::IAttributesPtr m_attr;
    
    // -- OCI variables --
    OCISession* m_auth;
    OCIEnv* m_env;
    OCIServer* m_srv;
    OCISvcCtx* m_svc;
    OCIError* m_err;

    std::wstring m_db_name;

    std::wstring m_server;
    int m_port;
    std::wstring m_database;
    std::wstring m_username;
    std::wstring m_password;
    
    ThreadErrorInfo m_error;
};


#endif
