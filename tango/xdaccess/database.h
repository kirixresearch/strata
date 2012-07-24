/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2005-01-10
 *
 */


#ifndef __XDACCESS_DATABASE_H
#define __XDACCESS_DATABASE_H


extern "C"
{
#include "mdbtools.h"
};


int access2tangoType(int);
MdbTableDef* findTableInCatalog(MdbHandle* mdb, gchar* table_name);


class AccessDatabase : public tango::IDatabase
{
	XCM_CLASS_NAME("xdaccess.Database")
	XCM_BEGIN_INTERFACE_MAP(AccessDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
	XCM_END_INTERFACE_MAP()

public:

    AccessDatabase();
    ~AccessDatabase();
    bool open(int type,
              const std::wstring& server,
              int port,
              const std::wstring& database,
              const std::wstring& username,
              const std::wstring& password,
              const std::wstring& path = L"");

    // -- tango::IDatabase interface --

    void close();

    void setDatabaseName(const std::wstring& name);
    std::wstring getDatabaseName();
    int getDatabaseType();
    tango::IAttributesPtr getAttributes();
    std::wstring getActiveUid();
    double getFreeSpace();
    double getUsedSpace();
    bool cleanup();

    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

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
    tango::IStreamPtr createStream(const std::wstring& path, const std::wstring& mime_type);
    tango::IStreamPtr openStream(const std::wstring& path);
    tango::INodeValuePtr createNodeFile(const std::wstring& path);
    tango::INodeValuePtr openNodeFile(const std::wstring& path);
    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_location);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    tango::IFileInfoPtr getFileInfo(const std::wstring& path);
    tango::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);

    tango::IStructurePtr createStructure();
    tango::ISetPtr createSet(const std::wstring& path, tango::IStructurePtr struct_config, int format);
    tango::ISetPtr createDynamicSet(tango::ISetPtr base_set);
    tango::ISetPtr createFilterSet(tango::ISetPtr base_set, const std::wstring& condition);
    tango::ISetPtr openSet(const std::wstring& path);
    tango::ISetPtr openSetEx(const std::wstring& path,
                             int format);

    tango::IRelationEnumPtr getRelationEnum();

    tango::IIteratorPtr runQuery(const std::wstring& command,
                                 unsigned int flags,
                                 tango::IJob* job);

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

    MdbHandle* m_mdb;
    std::wstring m_path;
    std::wstring m_db_name;
};




#endif

