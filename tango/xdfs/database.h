/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-12-28
 *
 */


#ifndef __XDFS_DATABASE_H
#define __XDFS_DATABASE_H


#include <map>
#include <kl/string.h>
#include "../xdcommon/errorinfo.h"


// forward declarations
class JobInfo;


// small container class for the set's format info

class FsSetFormatInfo
{
public:

    enum
    {
        maskFormat     = 1 << 0,
        maskDelimiters = 1 << 1
    };

    FsSetFormatInfo()
    {
        format = -1;
        delimiters = L"";
    }
    
public:

    int format;
    std::wstring delimiters;
};



xcm_interface IFsDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IFsDatabase")

public:

    virtual bool getSetFormat(const std::wstring& path,
                              FsSetFormatInfo* info,
                              int info_mask) = 0;
    virtual std::wstring getDefinitionDirectory() = 0;
    virtual std::wstring getTempFileDirectory() = 0;
    
    virtual void registerSet(std::wstring path, tango::ISet* set) = 0;
    virtual void unregisterSet(tango::ISet* set) = 0;
};
XCM_DECLARE_SMARTPTR(IFsDatabase)




class FsDatabase : public tango::IDatabase,
                   public IFsDatabase
{
    XCM_CLASS_NAME("xdfs.Database")
    XCM_BEGIN_INTERFACE_MAP(FsDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
        XCM_INTERFACE_ENTRY(IFsDatabase)
    XCM_END_INTERFACE_MAP()

public:

    FsDatabase();
    ~FsDatabase();
    bool open(const std::wstring& path);
    
    // -- tango::IFsDatabase interface --
    
    bool getSetFormat(const std::wstring& path,
                      FsSetFormatInfo* info,
                      int info_mask);
    std::wstring getTempFileDirectory();
    std::wstring getDefinitionDirectory();
    
    void registerSet(std::wstring path, tango::ISet* set);
    void unregisterSet(tango::ISet* set);
    
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

    double getFreeSpace();
    double getUsedSpace();
    bool cleanup();

    bool storeObject(xcm::IObject* obj, const std::wstring& path);

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
    bool getLocalFileExist(const std::wstring& path);
    tango::IFileInfoPtr getFileInfo(const std::wstring& path);
    tango::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);

    tango::IDatabasePtr getMountDatabase(const std::wstring& path);

    bool setMountPoint(const std::wstring& path,
                       const std::wstring& connection_str,
                       const std::wstring& remote_path);
                                  
    bool getMountPoint(const std::wstring& path,
                       std::wstring& connection_str,
                       std::wstring& remote_path);

    tango::IStructurePtr createStructure();
    tango::ISetPtr createSet(const std::wstring& path, tango::IStructurePtr struct_config, tango::FormatInfo* format_info);
    tango::IStreamPtr openStream(const std::wstring& ofs_path);
    tango::IStreamPtr createStream(const std::wstring& ofs_path, const std::wstring& mime_type);
    tango::ISetPtr openSet(const std::wstring& ofs_path);
    tango::ISetPtr openSetEx(const std::wstring& ofs_path,
                             int format);

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

    
    tango::IStructurePtr describeTable(const std::wstring& path);

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

    std::wstring makeFullPath(const std::wstring& _path);
    
    bool detectMountPoint(const std::wstring& path,
                          std::wstring& connection_str,
                          std::wstring& remote_path);
                          
    tango::IDatabasePtr lookupOrOpenMountDb(const std::wstring& connection);
    
    bool checkCircularMount(const std::wstring& path,
                            tango::IDatabasePtr remote_db, 
                            const std::wstring remote_path);

private:

    xcm::mutex m_obj_mutex;
    std::map<std::wstring, tango::ISet*, kl::cmp_nocase> m_set_map;
    std::map<std::wstring, tango::IDatabasePtr> m_mounted_dbs;
    tango::IDatabaseMgrPtr m_db_mgr;

    std::wstring m_base_path;
    int m_last_job;
    std::vector<JobInfo*> m_jobs;
    tango::IAttributesPtr m_attr;
    
    ThreadErrorInfo m_error;
};



#endif




