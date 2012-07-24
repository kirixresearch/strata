/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2005-01-10
 *
 */


#ifndef __XDSL_DATABASE_H
#define __XDSL_DATABASE_H


#include <sqlite3.h>


class JobInfo;
class SlDatabase : public tango::IDatabase
{
    friend class SlSet;
    friend class SlIterator;

	XCM_CLASS_NAME("xdsl.Database")
	XCM_BEGIN_INTERFACE_MAP(SlDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
	XCM_END_INTERFACE_MAP()

public:

    SlDatabase();
    ~SlDatabase();

    bool createDatabase(const std::wstring& path,
                        const std::wstring& db_name);

    bool openDatabase(int type,
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
    tango::IDatabaseAttributesPtr getAttributes();
    std::wstring getActiveUid();
    std::wstring getErrorString();
    int getErrorCode();
    double getFreeSpace();
    double getUsedSpace();
    bool cleanup();

    bool storeObject(xcm::IObject* obj, const std::wstring& ofs_path);

    tango::IJobPtr createJob();
    tango::IJobPtr getJob(tango::jobid_t job_id);

    bool createFolder(const std::wstring& path);
    tango::IOfsValuePtr createFile(const std::wstring& ofs_path);
    tango::IOfsValuePtr openFile(const std::wstring& ofs_path);
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
    tango::ISetPtr createBookmarkSet(tango::rowid_t rowid);
    tango::IUnionPtr createUnionSet();
    tango::ISetPtr openSet(const std::wstring& path);

    tango::IRelationEnumPtr getRelationEnum();

    tango::IIteratorPtr runQuery(const std::wstring& command, tango::IJob* job);
    bool execute(const std::wstring& command, tango::IJob* job);
    tango::ISetPtr runGroupQuery(tango::ISetPtr set,
                                 const std::wstring& group,
                                 const std::wstring& output,
                                 const std::wstring& filter,
                                 tango::IJob* job);

    // -- internal database functions --

    tango::IStructurePtr getStructureFromOrdinal(int ordinal);

private:

    int getCatalogEntry(const std::wstring& path,
                        int* catalog_id,
                        int* ofs_type,
                        std::wstring* obj_name);

    int createCatalogEntry(const std::wstring& path,
                           int ofs_type,
                           const std::wstring& name);

    bool updateCatalogEntry(int ordinal,
                            int folder_id,
                            int ofstype,
                            const wchar_t* name,
                            const wchar_t* obj_name);

    bool deleteCatalogEntry(int ordinal);

    std::wstring getCatalogPath(int ordinal);

private:

    std::wstring m_path;
    std::wstring m_db_name;

    sqlite3* m_db;
    int m_last_job;
    std::vector<JobInfo*> m_jobs;
};




#endif

