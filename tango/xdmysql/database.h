/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-04-16
 *
 */


#ifndef __XDMYSQL_DATABASE_H
#define __XDMYSQL_DATABASE_H


#include <string>
#include "../xdcommon/errorinfo.h"


// -- utility functions --
int mysql2tangoType(int mysql_type);
std::wstring createMySqlFieldString(const std::wstring& name,
                                    int type,
                                    int width,
                                    int scale,
                                    bool null);


xcm_interface IMySqlDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdmysql.IMySqlDatabase")

public:

    virtual MYSQL* open() = 0;
    virtual MYSQL* getMySqlPtr() = 0;
    virtual std::wstring getServer() = 0;
    virtual std::wstring getDatabase() = 0;
};

XCM_DECLARE_SMARTPTR(IMySqlDatabase)



class MySqlDatabase : public tango::IDatabase,
                      public IMySqlDatabase
{
    XCM_CLASS_NAME("xdmysql.Database")
    XCM_BEGIN_INTERFACE_MAP(MySqlDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
        XCM_INTERFACE_ENTRY(IMySqlDatabase)
    XCM_END_INTERFACE_MAP()

public:

    MySqlDatabase();
    ~MySqlDatabase();
    
    bool open(const std::wstring& server,
              int port,
              const std::wstring& database,
              const std::wstring& username,
              const std::wstring& password);

    MYSQL* open();
    MYSQL* getMySqlPtr();
    std::wstring getServer();
    std::wstring getDatabase();
    
    // -- tango::IDatabase interface --

    void close();

    void setDatabaseName(const std::wstring& name);
    std::wstring getDatabaseName();
    int getDatabaseType();
    std::wstring getActiveUid();
    tango::IAttributesPtr getAttributes();

    double getFreeSpace();
    double getUsedSpace();
    bool cleanup();

    bool storeObject(xcm::IObject* obj, const std::wstring& ofs_path);

    tango::IJobPtr createJob();
    tango::IJobPtr getJob(tango::jobid_t job_id);

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
    std::wstring getPrimaryKey(const std::wstring table);    

    tango::IDatabasePtr getMountDatabase(const std::wstring& path);

    bool setMountPoint(const std::wstring& path,
                       const std::wstring& connection_str,
                       const std::wstring& remote_path);

    bool getMountPoint(const std::wstring& path,
                       std::wstring& connection_str,
                       std::wstring& remote_path);

    tango::IStructurePtr createStructure();
    tango::ISetPtr createSet(const std::wstring& path, tango::IStructurePtr struct_config, tango::FormatInfo* format_info);
    tango::IStreamPtr openStream(const std::wstring& path);
    tango::IStreamPtr createStream(const std::wstring& path, const std::wstring& mime_type);
    tango::ISetPtr openSet(const std::wstring& path);
    tango::ISetPtr openSetEx(const std::wstring& path,
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

    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

private:

    tango::IAttributesPtr m_attr;
    
    MYSQL* m_data;
    MYSQL_RES* m_res;

    std::wstring m_db_name;

    std::wstring m_server;
    int m_port;
    std::wstring m_database;
    std::wstring m_username;
    std::wstring m_password;
    
    ThreadErrorInfo m_error;

};






#endif




