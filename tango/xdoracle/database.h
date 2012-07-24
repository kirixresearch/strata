/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
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

int oracle2tangoType(int oracle_type, int oracle_charset = SQLCS_IMPLICIT);
int tango2oracleType(int tango_type);

std::wstring createOracleFieldString(const std::wstring& name,
                                     int type,
                                     int width,
                                     int scale,
                                     bool null);
bool isSafeFieldname(const std::wstring& s);

tango::IColumnInfoPtr createColInfo(const std::wstring& col_name,
                                    int oracle_type,
                                    int oracle_charset,
                                    int width,
                                    int precision,
                                    int scale,
                                    const std::wstring& expr = L"");

sword checkerr(OCIError* err,
               sword status,
               char* msg = NULL,
               size_t msgbuf_size = 0);
               

xcm_interface IOracleDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdoracle.IOracleDatabase")

public:

    virtual std::wstring getServer() = 0;
    virtual std::wstring getDatabase() = 0;
};

XCM_DECLARE_SMARTPTR(IOracleDatabase)




class OracleDatabase : public tango::IDatabase,
                       public IOracleDatabase
{
    XCM_CLASS_NAME("xdoracle.Database")
    XCM_BEGIN_INTERFACE_MAP(OracleDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
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

    // -- tango::IDatabase interface --

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

    bool storeObject(xcm::IObject* obj, const std::wstring& ofs_path);

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
    bool moveFile(const std::wstring& path, const std::wstring& new_folder);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    tango::IFileInfoPtr getFileInfo(const std::wstring& path);
    tango::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);
    std::wstring getPrimaryKey(const std::wstring& table);

    tango::IStructurePtr createStructure();
    tango::ISetPtr createSet(const std::wstring& path, tango::IStructurePtr struct_config, tango::FormatInfo* format_info);
    tango::IStreamPtr openStream(const std::wstring& path);
    tango::IStreamPtr createStream(const std::wstring& path, const std::wstring& mime_type);
    tango::ISetPtr openSet(const std::wstring& path);
    tango::ISetPtr openSetEx(const std::wstring& ofs_path,
                             int format);
    tango::IRelationEnumPtr getRelationEnum();
    
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

    std::wstring getServer() { return m_server; }
    std::wstring getDatabase() { return m_database; }
    
    sword checkerr(OCIError* err, sword status);
    
private:

    tango::IAttributesPtr m_attr;
    
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
