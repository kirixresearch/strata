/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-11-09
 *
 */


#ifndef __XDNATIVE_DATABASE_H
#define __XDNATIVE_DATABASE_H


#include <map>
#include <set>
#include "xdnative_private.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "../xdcommon/errorinfo.h"


class JobInfo;
class OfsFile;

class XdnativeDatabase : public tango::IDatabase,
                         public tango::IRelationSchema,
                         public IXdsqlDatabase

{
    friend class XdnativeFileInfo;
    friend class TableSet;

    XCM_CLASS_NAME("xdnative.Database")
    XCM_BEGIN_INTERFACE_MAP(XdnativeDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
        XCM_INTERFACE_ENTRY(IXdsqlDatabase)
        XCM_INTERFACE_ENTRY(tango::IRelationSchema)
    XCM_END_INTERFACE_MAP()

public:

    XdnativeDatabase();
    virtual ~XdnativeDatabase();

    std::wstring getActiveUid();
    tango::IAttributesPtr getAttributes();

    bool createDatabase(const std::wstring& db_name,
                        const std::wstring& base_dir);

    bool openDatabase(const std::wstring& base_dir,
                      const std::wstring& uid,
                      const std::wstring& password);

    void close();


    // database user
    
    bool createUser(const std::wstring& uid,
                    const std::wstring& password);
                    
    bool deleteUser(const std::wstring& uid);
    
    bool checkPassword(
                    const std::wstring& uid,
                    const std::wstring& password);
    
    bool getUserExist(const std::wstring& uid);
    

    // database configuration

    bool setBaseDirectory(const std::wstring& base_dir);


    ITablePtr openTableByOrdinal(tango::tableord_t ordinal);
    std::wstring getTempFilename();
    std::wstring getUniqueFilename();
    std::wstring getTempOfsPath();
    std::wstring getTempPath();
    std::wstring getDefinitionPath();
    std::wstring getBasePath();
    std::wstring getOfsPath();
    std::wstring ofsToPhysFilename(const std::wstring& ofs_path, bool folder);
    IJobInternalPtr createJobEntry();

    bool lockSet(const std::wstring& set_id);
    bool unlockSet(const std::wstring& set_id);
    bool getSetLocked(const std::wstring& set_id);

    tango::tableord_t allocOrdinal();
    tango::tableord_t getMaximumOrdinal();
    bool deleteOrdinal(tango::tableord_t);
    bool setOrdinalTable(tango::tableord_t ordinal,
                         const std::wstring& table_filename);

    bool setFileType(const std::wstring& path, int type);
    bool getFileType(const std::wstring& path, int* type, bool* is_mount);

    void addFileToTrash(const std::wstring& filename);
    void emptyTrash();

    void lockObjectRegistryMutex() { m_objregistry_mutex.lock(); }
    void unlockObjectRegistryMutex() { m_objregistry_mutex.unlock(); } 

    void registerNodeFile(OfsFile* set);
    void unregisterNodeFile(OfsFile* set);

    void registerSet(IXdnativeSet* set);
    void unregisterSet(IXdnativeSet* set);

    void registerTable(ITable* table);
    void unregisterTable(ITable* table);

    bool deleteTempData();
    bool cleanup();


    // tango::IDatabase

    int getDatabaseType();
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    tango::IJobPtr createJob();

    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_location);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool copyData(const tango::CopyInfo* info, tango::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getLocalFileExist(const std::wstring& path);
    bool getFileExist(const std::wstring& path);

    tango::IFileInfoPtr getFileInfo(const std::wstring& path);
    tango::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);

    tango::IDatabasePtr getMountDatabase(const std::wstring& path);

    bool setMountPoint(const std::wstring& path,
                       const std::wstring& connection_str,
                       const std::wstring& remote_path);
                                  
    bool getMountPoint(const std::wstring& path,
                       std::wstring& connection_str,
                       std::wstring& remote_path);

    bool createFolder(const std::wstring& path);
    INodeValuePtr createNodeFile(const std::wstring& path);
    INodeValuePtr openNodeFile(const std::wstring& path);
    INodeValuePtr openLocalNodeFile(const std::wstring& path);

    tango::IStructurePtr createStructure();
    bool createStream(const std::wstring& path, const std::wstring& mime_type);
    bool createTable(const std::wstring& path, tango::IStructurePtr structure, tango::FormatInfo* format_info);
    
    IXdnativeSetPtr openSetById(const std::wstring& set_id);
    std::wstring getSetIdFromPath(const std::wstring& set_path);
    std::wstring getSetPathFromId(const std::wstring& set_id);
    tango::IStreamPtr openStream(const std::wstring& path);
    IXdsqlTablePtr openTable(const std::wstring& path);

    bool deleteSet(const std::wstring& ofs_path);
    bool deleteStream(const std::wstring& ofs_path);
    
    tango::IIteratorPtr query(const tango::QueryParams& qp);

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

    bool detectMountPoint(const std::wstring& path,
                          std::wstring& connection_str,
                          std::wstring& remote_path);
                          
    tango::IDatabasePtr lookupOrOpenMountDb(const std::wstring& connection);

    tango::IDatabasePtr getPassThroughMount(const std::vector<std::wstring>& tables);
    
    bool checkCircularMount(const std::wstring& path,
                           tango::IDatabasePtr remote_db, 
                           const std::wstring remote_path);
    bool checkCircularMountInternal( // (helper)
                           std::set<std::wstring, kl::cmp_nocase>& bad_paths,
                           tango::IDatabasePtr remote_db, 
                           const std::wstring remote_path);

    long long getFileSize(const std::wstring& path);
    std::wstring getFileMimeType(const std::wstring& path);
    bool renameOfsFile(const std::wstring& path, const std::wstring& new_name);
    bool moveOfsFile(const std::wstring& path, const std::wstring& new_location);
    bool deleteOfsFile(const std::wstring& path);
    
    void updateSetReference(const std::wstring& ofs_path);
    void recursiveReferenceUpdate(const std::wstring& folder_path);
    std::wstring getTableFilename(tango::tableord_t table_ordinal);
    std::wstring getStreamFilename(const std::wstring& ofs_path);
    IXdnativeSet* lookupSet(const std::wstring& set_id);
    void getFolderUsedOrdinals(const std::wstring& path,
                               std::set<int>& used_ordinals);


private:

    xcm::mutex m_objregistry_mutex;
    xcm::mutex m_mountdbs_mutex;
    xcm::mutex m_jobs_mutex;
    xcm::mutex m_lockedsets_mutex;

    tango::IAttributesPtr m_attr;
    
    tango::IDatabaseMgrPtr m_db_mgr;
    std::map<std::wstring, tango::IDatabasePtr> m_mounted_dbs;
    
    std::vector<IXdnativeSet*> m_sets;
    std::vector<ITable*> m_tables;
    std::vector<OfsFile*> m_ofs_files;
    std::vector<JobInfo*> m_jobs;
    std::vector<std::wstring> m_locked_sets;

    std::wstring m_base_dir;
    std::wstring m_ofs_root;
    std::wstring m_ordinal_counter_path;
    std::wstring m_uid;
    tango::jobid_t m_last_job;
    
    ThreadErrorInfo m_error;
};



#endif

