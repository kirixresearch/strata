/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-11-09
 *
 */


#ifndef H_XDNATIVE_DATABASE_H
#define H_XDNATIVE_DATABASE_H


#include <map>
#include <set>
#include "xdnative_private.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "../xdcommon/errorinfo.h"


class JobInfo;
class OfsFile;

class XdnativeDatabase : public xd::IDatabase,
                         public xd::IRelationSchema,
                         public IXdsqlDatabase

{
    friend class XdnativeFileInfo;
    friend class TableSet;

    XCM_CLASS_NAME("xdnative.Database")
    XCM_BEGIN_INTERFACE_MAP(XdnativeDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
        XCM_INTERFACE_ENTRY(IXdsqlDatabase)
        XCM_INTERFACE_ENTRY(xd::IRelationSchema)
    XCM_END_INTERFACE_MAP()

public:

    XdnativeDatabase();
    virtual ~XdnativeDatabase();

    std::wstring getActiveUid();
    xd::IAttributesPtr getAttributes();

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


    ITablePtr openTableByOrdinal(xd::tableord_t ordinal);
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

    xd::tableord_t allocOrdinal();
    xd::tableord_t getMaximumOrdinal();
    bool deleteOrdinal(xd::tableord_t);
    bool setOrdinalTable(xd::tableord_t ordinal,
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


    // xd::IDatabase

    int getDatabaseType();
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    xd::IJobPtr createJob();

    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_location);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool copyData(const xd::CopyParams* info, xd::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getLocalFileExist(const std::wstring& path);
    bool getFileExist(const std::wstring& path);

    xd::IFileInfoPtr getFileInfo(const std::wstring& path);
    xd::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);

    xd::IDatabasePtr getMountDatabase(const std::wstring& path);

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

    bool createStream(const std::wstring& path, const std::wstring& mime_type);
    bool createTable(const std::wstring& path, const xd::FormatDefinition& format_definition);
    
    bool detectStreamFormat(const std::wstring& path, xd::FormatDefinition* format_info, const xd::FormatDefinition* defaults = NULL, xd::IJob* job = NULL);
    bool loadDefinition(const std::wstring& path, xd::FormatDefinition* format_info);
    bool saveDefinition(const std::wstring& path, const xd::FormatDefinition& format_info);

    IXdnativeSetPtr openSetById(const std::wstring& set_id);
    std::wstring getSetIdFromPath(const std::wstring& set_path);
    std::wstring getSetPathFromId(const std::wstring& set_id);
    xd::IStreamPtr openStream(const std::wstring& path);
    IXdsqlTablePtr openTable(const std::wstring& path, const xd::FormatDefinition& format_definition = xd::FormatDefinition(), xd::IJob* job = NULL);

    bool deleteSet(const std::wstring& ofs_path);
    bool deleteStream(const std::wstring& ofs_path);
    
    xd::IIteratorPtr query(const xd::QueryParams& qp);
    bool groupQuery(xd::GroupQueryParams* info, xd::IJob* job);

    xd::IRelationPtr createRelation(const std::wstring& tag,
                                       const std::wstring& left_set_path,
                                       const std::wstring& right_set_path,
                                       const std::wstring& left_expr,
                                       const std::wstring& right_expr);
    bool deleteRelation(const std::wstring& relation_id);
    xd::IRelationPtr getRelation(const std::wstring& relation_id);
    xd::IRelationEnumPtr getRelationEnum(const std::wstring& path);

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

    xd::Structure describeTable(const std::wstring& path);

    bool modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job);

    bool execute(const std::wstring& command,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 xd::IJob* job);

    xd::ColumnInfo validateExpression(const std::wstring& expr, const xd::Structure& structure = xd::Structure(), const std::wstring& path_context = L"");
    bool assignDefinition(const std::wstring& path, const xd::FormatDefinition& fd);

private:

    bool detectMountPoint(const std::wstring& path,
                          std::wstring* connection_str = NULL,
                          std::wstring* remote_path = NULL,
                          std::wstring* mount_root = NULL);
                          
    xd::IDatabasePtr lookupOrOpenMountDb(const std::wstring& connection);

    xd::IDatabasePtr getPassThroughMount(const std::vector<std::wstring>& tables);
    
    bool checkCircularMount(const std::wstring& path,
                           xd::IDatabasePtr remote_db, 
                           const std::wstring& remote_path);
    bool checkCircularMountInternal( // (helper)
                           std::set<std::wstring, kl::cmp_nocase>& bad_paths,
                           xd::IDatabasePtr remote_db, 
                           const std::wstring& remote_path);

    long long getFileSize(const std::wstring& path);
    xd::rowpos_t getRowCount(const std::wstring& path);

    std::wstring getFileMimeType(const std::wstring& path);
    bool renameOfsFile(const std::wstring& path, const std::wstring& new_name);
    bool moveOfsFile(const std::wstring& path, const std::wstring& new_location);
    bool deleteOfsFile(const std::wstring& path);
    
    void updateSetReference(const std::wstring& ofs_path);
    void recursiveReferenceUpdate(const std::wstring& folder_path);
    std::wstring getTableFilename(xd::tableord_t table_ordinal);
    std::wstring getStreamFilename(const std::wstring& ofs_path);
    IXdnativeSet* lookupSet(const std::wstring& set_id);
    void getFolderUsedOrdinals(const std::wstring& path,
                               std::set<int>& used_ordinals);


private:

    kl::mutex m_objregistry_mutex;
    kl::mutex m_mountdbs_mutex;
    kl::mutex m_jobs_mutex;
    kl::mutex m_lockedsets_mutex;

    xd::IAttributesPtr m_attr;
    
    xd::IDatabaseMgrPtr m_db_mgr;
    std::map<std::wstring, xd::IDatabasePtr> m_mounted_dbs;
    
    std::vector<IXdnativeSet*> m_sets;
    std::vector<ITable*> m_tables;
    std::vector<OfsFile*> m_ofs_files;
    std::vector<JobInfo*> m_jobs;
    std::vector<std::wstring> m_locked_sets;

    std::wstring m_base_dir;
    std::wstring m_ofs_root;
    std::wstring m_ordinal_counter_path;
    std::wstring m_uid;
    xd::jobid_t m_last_job;
    
    ThreadErrorInfo m_error;
};



#endif

