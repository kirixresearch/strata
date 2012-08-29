/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
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
#include "../xdcommon/errorinfo.h"


// -- forward declarations --
class JobInfo;
class OfsFile;

class Database : public tango::IDatabase,
                 public IDatabaseInternal,
                 public IOfsEvents
{
    XCM_CLASS_NAME("xdnative.Database")
    XCM_BEGIN_INTERFACE_MAP(Database)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
        XCM_INTERFACE_ENTRY(IDatabaseInternal)
        XCM_INTERFACE_ENTRY(IOfsEvents)
    XCM_END_INTERFACE_MAP()

public:

    Database();
    virtual ~Database();

    std::wstring getActiveUid();
    tango::IAttributesPtr getAttributes();

    bool createDatabase(const std::wstring& db_name,
                        const std::wstring& base_dir);

    bool openDatabase(const std::wstring& base_dir,
                      const std::wstring& uid,
                      const std::wstring& password);

    void close();


    // -- database user --
    
    bool createUser(const std::wstring& uid,
                    const std::wstring& password);
                    
    bool deleteUser(const std::wstring& uid);
    
    bool checkPassword(
                    const std::wstring& uid,
                    const std::wstring& password);
    
    bool getUserExist(const std::wstring& uid);
    
    // -- database configuration --
    bool setBaseDirectory(const std::wstring& base_dir);
    std::wstring getBaseDirectory();

    // -- IDatabaseInternal --
    ITablePtr openTableByOrdinal(tango::tableord_t ordinal);
    std::wstring getTempFilename();
    std::wstring getUniqueFilename();
    std::wstring getTempOfsPath();
    std::wstring getTempPath();
    std::wstring getDefinitionPath();
    std::wstring getBasePath();
    std::wstring getOfsPath();
    std::wstring ofsToPhysFilename(const std::wstring& ofs_path, bool folder);
    std::wstring urlToOfsFilename(const std::wstring& url);
    IJobInternalPtr createJobEntry();

    bool lockSet(const std::wstring& set_id);
    bool unlockSet(const std::wstring& set_id);
    bool getSetLocked(const std::wstring& set_id);

    tango::tableord_t allocOrdinal();
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

    void registerSet(tango::ISet* set);
    void unregisterSet(tango::ISet* set);

    void registerTable(ITable* table);
    void unregisterTable(ITable* table);

    bool deleteTempData();
    bool cleanup();

    // -- IDatabase --

    void setDatabaseName(const std::wstring& name);
    std::wstring getDatabaseName();
    int getDatabaseType();
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    double getFreeSpace();
    double getUsedSpace();

    tango::IJobPtr createJob();
    tango::IJobPtr getJob(tango::jobid_t job_id);

    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_location);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
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
    tango::INodeValuePtr createNodeFile(const std::wstring& path);
    tango::INodeValuePtr openNodeFile(const std::wstring& path);
    tango::INodeValuePtr openLocalNodeFile(const std::wstring& path);

    tango::IStructurePtr createStructure();
    tango::IStreamPtr createStream(const std::wstring& path, const std::wstring& mime_type);
    tango::ISetPtr createSet(const std::wstring& path, tango::IStructurePtr structure, tango::FormatInfo* format_info);
    
    tango::ISetPtr openSetById(const std::wstring& set_id);
    std::wstring getSetIdFromPath(const std::wstring& set_path);
    std::wstring getSetPathFromId(const std::wstring& set_id);
    tango::IStreamPtr openStream(const std::wstring& path);
    tango::ISetPtr openSet(const std::wstring& path);
    tango::ISetPtr openSetEx(const std::wstring& path,
                             int format);
    bool storeObject(xcm::IObject* _obj, const std::wstring& ofs_path);
    bool deleteSet(const std::wstring& ofs_path);
    bool deleteStream(const std::wstring& ofs_path);
    
    tango::IRelationEnumPtr getRelationEnum(const std::wstring& path);
    tango::IRelationPtr getRelation(const std::wstring& relation_id);
    bool deleteRelation(const std::wstring& relation_id);
    tango::IRelationPtr createRelation(const std::wstring& tag,
                                       const std::wstring& left_set_path,
                                       const std::wstring& right_set_path,
                                       const std::wstring& left_expr,
                                       const std::wstring& right_expr);

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

    tango::tango_int64_t getFileSize(const std::wstring& path);
    std::wstring getFileMimeType(const std::wstring& path);
    bool renameOfsFile(const std::wstring& path, const std::wstring& new_name);
    bool moveOfsFile(const std::wstring& path, const std::wstring& new_location);
    bool deleteOfsFile(const std::wstring& path);
    
    void updateSetReference(const std::wstring& ofs_path);
    void recursiveReferenceUpdate(const std::wstring& folder_path);
    std::wstring getTableFilename(tango::tableord_t table_ordinal);
    std::wstring getStreamFilename(const std::wstring& ofs_path);
    tango::ISet* lookupSet(const std::wstring& set_id);
    void getFolderUsedOrdinals(const std::wstring& path,
                               std::set<int>& used_ordinals);

    // -- Ofs Event Handlers --
    void onOfsPostRenameFile(std::wstring ofs_path, std::wstring new_name);
    void onOfsPostMoveFile(std::wstring ofs_path, std::wstring new_path);

private:

    xcm::mutex m_object_mutex;
    xcm::mutex m_relations_mutex;
    xcm::mutex m_objregistry_mutex;

    tango::IAttributesPtr m_attr;
    
    tango::IDatabaseMgrPtr m_db_mgr;
    std::map<std::wstring, tango::IDatabasePtr> m_mounted_dbs;
    
    std::vector<tango::ISet*> m_sets;
    std::vector<ITable*> m_tables;
    std::vector<OfsFile*> m_ofs_files;
    std::vector<JobInfo*> m_jobs;
    std::vector<std::wstring> m_locked_sets;

    std::wstring m_base_dir;
    std::wstring m_ofs_root;
    std::wstring m_dbname;
    std::wstring m_uid;
    tango::jobid_t m_last_job;
    
    ThreadErrorInfo m_error;

};



#endif

