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

tango::IColumnInfoPtr pgsqlCreateColInfo(const std::wstring& col_name,
                                         int col_odbc_type,
                                         int col_width,
                                         int col_scale,
                                         const std::wstring& col_expr,
                                         int datetime_sub);

int pgsqlToTangoType(int pg_type);

std::wstring pgsqlQuoteIdentifier(const std::wstring& str);
std::wstring pgsqlQuoteIdentifierIfNecessary(const std::wstring& str);



class PgsqlJobInfo;


class PgsqlDatabase : public tango::IDatabase
{
friend class PgsqlSet;
friend class PgsqlRowInserter;
friend class PgsqlIterator;
friend class PgsqlStream;

    XCM_CLASS_NAME("xdpgsql.Database")
    XCM_BEGIN_INTERFACE_MAP(PgsqlDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
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

    void setDatabaseName(const std::wstring& name);
    std::wstring getDatabaseName();
    int getDatabaseType();
    tango::IAttributesPtr getAttributes();
    std::wstring getActiveUid();
    
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    bool cleanup();

    tango::IJobPtr createJob();

    tango::IDatabasePtr getMountDatabase(const std::wstring& path);

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
    bool copyData(const tango::CopyInfo* info, tango::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    tango::IFileInfoPtr getFileInfo(const std::wstring& path);
    tango::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);
    std::wstring getPrimaryKey(const std::wstring table);
    
    tango::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, tango::IStructurePtr struct_config, tango::FormatInfo* format_info);
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

    tango::IRowInserterPtr bulkInsert(const std::wstring& path);

    tango::IStructurePtr describeTable(const std::wstring& path);

    bool modifyStructure(const std::wstring& path, tango::IStructurePtr struct_config, tango::IJob* job);

    bool execute(const std::wstring& command,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 tango::IJob* job);

    bool groupQuery(tango::GroupQueryInfo* info, tango::IJob* job);

private:
 
    std::wstring getServer();
    std::wstring getPath();
    
    std::wstring getTempFileDirectory();
    std::wstring getDefinitionDirectory();
    
    PGconn* createConnection();
    void closeConnection(PGconn* conn);

private:

    tango::IAttributesPtr m_attr;

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
    
    xcm::mutex m_jobs_mutex;
    int m_last_job;
    std::vector<PgsqlJobInfo*> m_jobs;

    ThreadErrorInfo m_error;
};




#endif




