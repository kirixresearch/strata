/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
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


// utility functions
int mysql2xdType(int mysql_type);
std::wstring createMySqlFieldString(const std::wstring& name,
                                    int type,
                                    int width,
                                    int scale,
                                    bool null);

std::wstring mysqlGetTablenameFromPath(const std::wstring& path);
std::wstring mysqlQuoteIdentifier(const std::wstring& str);
std::wstring mysqlQuoteIdentifierIfNecessary(const std::wstring& str);


class MysqlDatabase : public xd::IDatabase
{
    XCM_CLASS_NAME("xdmysql.Database")
    XCM_BEGIN_INTERFACE_MAP(MysqlDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
    XCM_END_INTERFACE_MAP()

public:

    MysqlDatabase();
    ~MysqlDatabase();
    
    bool open(const std::wstring& server,
              int port,
              const std::wstring& database,
              const std::wstring& username,
              const std::wstring& password);

    void close();

    MYSQL* open();
    MYSQL* getMySqlPtr();
    std::wstring getServer();
    std::wstring getDatabase();
    
    void setDatabaseName(const std::wstring& name);
    std::wstring getDatabaseName();
    int getDatabaseType();
    std::wstring getActiveUid();
    xd::IAttributesPtr getAttributes();

    double getFreeSpace();
    double getUsedSpace();
    bool cleanup();

    bool storeObject(xcm::IObject* obj, const std::wstring& ofs_path);

    xd::IJobPtr createJob();
    xd::IJobPtr getJob(xd::jobid_t job_id);

    bool createFolder(const std::wstring& path);
    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_folder);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool copyData(const xd::CopyParams* info, xd::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    xd::IFileInfoPtr getFileInfo(const std::wstring& path);
    xd::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);
    std::wstring getPrimaryKey(const std::wstring table);    

    xd::IDatabasePtr getMountDatabase(const std::wstring& path);

    bool setMountPoint(const std::wstring& path,
                       const std::wstring& connection_str,
                       const std::wstring& remote_path);

    bool getMountPoint(const std::wstring& path,
                       std::wstring& connection_str,
                       std::wstring& remote_path);

    xd::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, const xd::FormatDefinition& format_definition);
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

    bool modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job);

    bool execute(const std::wstring& command,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 xd::IJob* job);
    
    bool groupQuery(xd::GroupQueryParams* info, xd::IJob* job);

    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

private:

    xd::IAttributesPtr m_attr;
    
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




