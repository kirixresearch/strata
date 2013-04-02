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


class JobInfo;
class SlDatabase : public tango::IDatabase
{
    friend class SlSet;
    friend class SlIterator;

    XCM_CLASS_NAME("xdsqlite.Database")
    XCM_BEGIN_INTERFACE_MAP(SlDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
    XCM_END_INTERFACE_MAP()

public:

    SlDatabase();
    ~SlDatabase();

    bool createDatabase(const std::wstring& path,
                        const std::wstring& db_name);

    bool openDatabase(const std::wstring& path,
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

    bool cleanup();

    tango::IJobPtr createJob();

    bool createFolder(const std::wstring& path);
    tango::IStreamPtr createStream(const std::wstring& path, const std::wstring& mime_type);
    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_location);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool copyData(const tango::CopyInfo* info, tango::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    tango::IFileInfoPtr getFileInfo(const std::wstring& path);
    tango::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);

    tango::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, tango::IStructurePtr struct_config, tango::FormatInfo* format_info);
    tango::ISetPtr createDynamicSet(tango::ISetPtr base_set);
    
    tango::IStreamPtr openStream(const std::wstring& path);
    tango::ISetPtr openSet(const std::wstring& path);
    tango::ISetPtr openSetEx(const std::wstring& path, int format);

    tango::IIteratorPtr createIterator(const std::wstring& path,
                                       const std::wstring& columns,
                                       const std::wstring& sort,
                                       tango::IJob* job);

    tango::IRelationEnumPtr getRelationEnum(const std::wstring& path);
    tango::IRelationPtr getRelation(const std::wstring& relation_id);
    tango::IRelationPtr createRelation(const std::wstring& tag,
                                       const std::wstring& left_set_path,
                                       const std::wstring& right_set_path,
                                       const std::wstring& left_expr,
                                       const std::wstring& right_expr);
    bool deleteRelation(const std::wstring& relation_id);


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


    tango::IDatabasePtr getMountDatabase(const std::wstring& path);
    
    bool setMountPoint(const std::wstring& path,
                       const std::wstring& connection_str,
                       const std::wstring& remote_path);
                                  
    bool getMountPoint(const std::wstring& path,
                       std::wstring& connection_str,
                       std::wstring& remote_path);
    
    tango::IRowInserterPtr bulkInsert(const std::wstring& path);
          
    tango::IStructurePtr describeTable(const std::wstring& path);

    bool modifyStructure(const std::wstring& path, tango::IStructurePtr struct_config, tango::IJob* job);

    bool execute(const std::wstring& command,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 tango::IJob* job);
    
    bool groupQuery(tango::GroupQueryInfo* info, tango::IJob* job);

    tango::IStructurePtr getStructureFromPath(std::wstring& path);
    
private:

    std::wstring m_path;
    std::wstring m_db_name;

    sqlite3* m_sqlite;
    int m_last_job;
    std::vector<JobInfo*> m_jobs;
    
    ThreadErrorInfo m_error;
};




#endif

