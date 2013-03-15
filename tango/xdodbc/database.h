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


#if _MSC_VER < 1300
#define SQLLEN SQLINTEGER
#define SQLULEN SQLUINTEGER
#endif


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
SQLSMALLINT tango2sqlType(int tango_type);
SQLSMALLINT tango2sqlCType(int tango_type);
int sql2tangoType(SQLSMALLINT sql_type);

std::wstring createOdbcFieldString(const std::wstring& name,
                                   int type,
                                   int width,
                                   int scale,
                                   bool allow_nulls,
                                   int db_type);

tango::IColumnInfoPtr createColInfo(int db_type,
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



class OdbcDatabase : public tango::IDatabase,
                     public IOdbcDatabase
{
    XCM_CLASS_NAME("xdodbc.Database")
    XCM_BEGIN_INTERFACE_MAP(OdbcDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
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

    void setDatabaseName(const std::wstring& name);
    std::wstring getDatabaseName();
    int getDatabaseType();
    tango::IAttributesPtr getAttributes();
    std::wstring getActiveUid();
    
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    double getFreeSpace();
    double getUsedSpace();
    bool cleanup();

    bool storeObject(xcm::IObject* obj, const std::wstring& path);

    tango::IJobPtr createJob();
    tango::IJobPtr getJob(tango::jobid_t job_id);

    tango::IDatabasePtr getMountDatabase(const std::wstring& path);

    bool setMountPoint(const std::wstring& path,
                       const std::wstring& connection_str,
                       const std::wstring& remote_path);
                                  
    bool getMountPoint(const std::wstring& path,
                       std::wstring& connection_str,
                       std::wstring& remote_path);

    bool createFolder(const std::wstring& path);
    tango::INodeValuePtr createNodeFile(const std::wstring& path);
    tango::INodeValuePtr openNodeFile(const std::wstring& path);
    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_location);
    bool copyFile(const std::wstring& path, const std::wstring& dest_path);
    bool copyData(const tango::CopyInfo* info, tango::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    tango::IFileInfoPtr getFileInfo(const std::wstring& path);
    tango::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);
    std::wstring getPrimaryKey(const std::wstring table);
    
    tango::IStructurePtr createStructure();
    tango::ISetPtr createTable(const std::wstring& path,
                             tango::IStructurePtr struct_config,
                             tango::FormatInfo* format_info);
    tango::IStreamPtr openStream(const std::wstring& path);
    tango::IStreamPtr createStream(const std::wstring& path, const std::wstring& mime_type);
    tango::ISetPtr openSet(const std::wstring& path);
    tango::ISetPtr openSetEx(const std::wstring& path,
                             int format);

    tango::IIteratorPtr createIterator(const std::wstring& path,
                                       const std::wstring& columns,
                                       const std::wstring& sort,
                                       tango::IJob* job);

    tango::IRelationPtr createRelation(const std::wstring& tag,
                                       const std::wstring& left_set_path,
                                       const std::wstring& right_set_path,
                                       const std::wstring& left_expr,
                                       const std::wstring& right_expr);
    bool deleteRelation(const std::wstring& relation_id);
    tango::IRelationPtr getRelation(const std::wstring& relation_id);
    tango::IRelationEnumPtr getRelationEnum(const std::wstring& path);

    tango::IIndexInfoPtr createIndex(const std::wstring& path,
                                     const std::wstring& name,
                                     const std::wstring& expr,
                                     tango::IJob* job);
    bool renameIndex(const std::wstring& path,
                     const std::wstring& name,
                     const std::wstring& new_name);
    bool deleteIndex(const std::wstring& path,
                     const std::wstring& name);
    tango::IIndexInfoEnumPtr getIndexEnum(const std::wstring& path);


    tango::IStructurePtr describeTable(const std::wstring& path);

    bool execute(const std::wstring& command,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 tango::IJob* job);

    tango::ISetPtr runGroupQuery(tango::ISetPtr set,
                                 const std::wstring& group,
                                 const std::wstring& output,
                                 const std::wstring& where,
                                 const std::wstring& having,
                                 tango::IJob* job);

private:

    tango::IFileInfoEnumPtr getTreeFolderInfo(const std::wstring& path);
 
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

    tango::IAttributesPtr m_attr;
    
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




