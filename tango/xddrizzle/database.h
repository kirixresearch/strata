/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-04-16
 *
 */


#ifndef __XDDRIZZLE_DATABASE_H
#define __XDDRIZZLE_DATABASE_H


#include <string>
#include "../xdcommon/errorinfo.h"


// -- utility functions --
int drizzle2tangoType(int mysql_type);
std::wstring createDrizzleFieldString(const std::wstring& name,
                                    int type,
                                    int width,
                                    int scale,
                                    bool null);


xcm_interface IDrizzleDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("xddrizzle.IDrizzleDatabase")

public:

    virtual drizzle_con_st* open() = 0;
    virtual std::wstring getServer() = 0;
    virtual std::wstring getDatabase() = 0;
    virtual void setError(int error_code, const std::wstring& error_string) = 0;

};

XCM_DECLARE_SMARTPTR(IDrizzleDatabase)



class DrizzleDatabase : public tango::IDatabase,
                        public IDrizzleDatabase
{
    XCM_CLASS_NAME("xddrizzle.Database")
    XCM_BEGIN_INTERFACE_MAP(DrizzleDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
        XCM_INTERFACE_ENTRY(IDrizzleDatabase)
    XCM_END_INTERFACE_MAP()

public:

    DrizzleDatabase();
    ~DrizzleDatabase();
    
    bool open(const std::wstring& server,
              int port,
              const std::wstring& database,
              const std::wstring& username,
              const std::wstring& password);

    drizzle_con_st* open();
    std::wstring getServer();
    std::wstring getDatabase();
    
    // tango::IDatabase

    void close();

    int getDatabaseType();
    std::wstring getActiveUid();
    tango::IAttributesPtr getAttributes();

    bool cleanup();

    tango::IJobPtr createJob();

    bool createFolder(const std::wstring& path);
    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_folder);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool copyData(const tango::CopyInfo* info, tango::IJob* job);
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
    bool createTable(const std::wstring& path, tango::IStructurePtr struct_config, tango::FormatInfo* format_info);
    tango::IStreamPtr openStream(const std::wstring& path);
    bool createStream(const std::wstring& path, const std::wstring& mime_type);

    tango::IIteratorPtr createIterator(const std::wstring& path,
                                       const std::wstring& columns,
                                       const std::wstring& wherec,
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
    
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

private:

    tango::IAttributesPtr m_attr;
    
    drizzle_st m_drizzle;

    std::wstring m_db_name;

    std::wstring m_server;
    int m_port;
    std::wstring m_database;
    std::wstring m_username;
    std::wstring m_password;
    
    ThreadErrorInfo m_error;
};







#endif




