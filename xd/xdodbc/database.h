/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-09-03
 *
 */


#ifndef __XDODBC_DATABASE_H
#define __XDODBC_DATABASE_H


#ifdef WIN32
#include <xcm/xcmwin32.h>
#endif

#include <kl/string.h>
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/errorinfo.h"


extern "C" {

#include <sql.h>
#include <sqlext.h>
};


#ifdef _UNICODE
    #define sqlt(s)    ((SQLWCHAR*)(s.c_str()))
    #define _t(s)      (SQLWCHAR*)L ## s
#else
    #define sqlt(s)    ((SQLCHAR*)(kl::tostring(s).c_str()))
    #define _t(s)      (SQLCHAR*)s
#endif


inline std::wstring sql2wstring(const SQLCHAR* s)
{
    return kl::towstring((char*)s);
}

inline std::wstring sql2wstring(const SQLWCHAR* s)
{
    return kl::towstring((wchar_t*)s);
}

void testSqlStmt(HSTMT stmt);
void testSqlConn(HDBC hdbc);
SQLSMALLINT tango2sqlType(int xd_type);
SQLSMALLINT tango2sqlCType(int xd_type);
int sql2tangoType(SQLSMALLINT sql_type);

std::wstring createOdbcFieldString(const std::wstring& name,
                                   int type,
                                   int width,
                                   int scale,
                                   bool allow_nulls,
                                   int db_type);

xd::IColumnInfoPtr createColInfo(int db_type,
                                    const std::wstring& col_name,
                                    int col_odbc_type,
                                    int col_width,
                                    int col_scale,
                                    const std::wstring& col_expr,
                                    int datetime_sub);




xcm_interface IOdbcDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdodbc.IOdbcDatabase")

public:

    virtual SQLRETURN connect(HDBC conn) = 0;
    virtual bool getUsingDsn() = 0;
    virtual std::wstring getServer() = 0;
    virtual std::wstring getPath() = 0;
    
    virtual std::wstring getTempFileDirectory() = 0;
    virtual std::wstring getDefinitionDirectory() = 0;
    
    virtual void errorSqlStmt(HSTMT stmt) = 0;
    virtual void errorSqlConn(HDBC hdbc) = 0;
};

XCM_DECLARE_SMARTPTR(IOdbcDatabase)



class OdbcDatabase : public xd::IDatabase,
                     public IOdbcDatabase
{
    friend class OdbcRowInserter;
    friend class OdbcIterator;

    XCM_CLASS_NAME("xdodbc.Database")
    XCM_BEGIN_INTERFACE_MAP(OdbcDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
        XCM_INTERFACE_ENTRY(IOdbcDatabase)
    XCM_END_INTERFACE_MAP()

public:

    OdbcDatabase();
    ~OdbcDatabase();
    
    bool open(int type,
              const std::wstring& server,
              int port,
              const std::wstring& database,
              const std::wstring& username,
              const std::wstring& password,
              const std::wstring& path = L"");

    void close();

    std::wstring getDatabaseName();
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

    xd::IFileInfoEnumPtr getTreeFolderInfo(const std::wstring& path);
 
    void errorSqlStmt(HSTMT stmt);
    void errorSqlConn(HDBC hdbc);
    
    SQLRETURN connect(HDBC conn);
    HDBC createConnection(SQLRETURN* retval = NULL);
    void closeConnection(HDBC conn);
    
    bool getUsingDsn();
    std::wstring getServer();
    std::wstring getPath();
    
    std::wstring getTempFileDirectory();
    std::wstring getDefinitionDirectory();
    
    void setAttributes(HDBC connection = NULL);
    
private:

    xcm::mutex m_obj_mutex;

    int m_last_job;
    std::vector<JobInfo*> m_jobs;

    xd::IAttributesPtr m_attr;
    
    HENV m_env;

    std::wstring m_db_name;
    std::wstring m_conn_str;

    bool m_using_dsn;
    int m_db_type;
    int m_port;
    std::wstring m_server;
    std::wstring m_database;
    std::wstring m_username;
    std::wstring m_password;
    std::wstring m_path;
    
    ThreadErrorInfo m_error;
};




#endif




