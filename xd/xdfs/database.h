/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
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
};
XCM_DECLARE_SMARTPTR(IFsDatabase)




class FsDatabase : public xd::IDatabase,
                   public IFsDatabase
{
    XCM_CLASS_NAME("xdfs.Database")
    XCM_BEGIN_INTERFACE_MAP(FsDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
        XCM_INTERFACE_ENTRY(IFsDatabase)
    XCM_END_INTERFACE_MAP()

public:

    FsDatabase();
    ~FsDatabase();
    bool open(const std::wstring& path);
    
    // xd::IDatabase
    
    bool getSetFormat(const std::wstring& path,
                      FsSetFormatInfo* info,
                      int info_mask);
    std::wstring getTempFileDirectory();
    std::wstring getDefinitionDirectory();
    
    
    // xd::IDatabase

    void close();

    int getDatabaseType();
    xd::IAttributesPtr getAttributes();
    std::wstring getActiveUid();
    
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    double getFreeSpace();
    double getUsedSpace();
    bool cleanup();

    xd::IJobPtr createJob();

    IXdfsSetPtr openSetEx(const std::wstring& path, const xd::FormatInfo& fi);

    bool createFolder(const std::wstring& path);
    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_folder);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool copyData(const xd::CopyParams* info, xd::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    bool getLocalFileExist(const std::wstring& path);
    xd::IFileInfoPtr getFileInfo(const std::wstring& path);
    xd::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);

    xd::IDatabasePtr getMountDatabase(const std::wstring& path);

    bool setMountPoint(const std::wstring& path,
                       const std::wstring& connection_str,
                       const std::wstring& remote_path);
                                  
    bool getMountPoint(const std::wstring& path,
                       std::wstring& connection_str,
                       std::wstring& remote_path);

    xd::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, xd::IStructurePtr struct_config, xd::FormatInfo* format_info);
    xd::IStreamPtr openStream(const std::wstring& ofs_path);
    bool createStream(const std::wstring& ofs_path, const std::wstring& mime_type);

    xd::IIteratorPtr query(const xd::QueryParams& qp);

    xd::IIndexInfoPtr createIndex(const std::wstring& path,
                                     const std::wstring& name,
                                     const std::wstring& expr,
                                     xd::IJob* job);
    bool renameIndex(const std::wstring& path,
                     const std::wstring& name,
                     const std::wstring& new_name);
    bool deleteIndex(const std::wstring& path,
                     const std::wstring& name);
    xd::IIndexInfoEnumPtr getIndexEnum(const std::wstring& path);

    xd::IRowInserterPtr bulkInsert(const std::wstring& path);

    xd::IStructurePtr describeTable(const std::wstring& path);

    bool modifyStructure(const std::wstring& path, xd::IStructurePtr struct_config, xd::IJob* job);

    bool execute(const std::wstring& command,
                 unsigned int flags,
                 xcm::IObjectPtr& result,
                 xd::IJob* job);

    bool groupQuery(xd::GroupQueryParams* info, xd::IJob* job);

private:

    std::wstring makeFullPath(const std::wstring& _path);
    
    bool detectMountPoint(const std::wstring& path,
                          std::wstring& connection_str,
                          std::wstring& remote_path);
                          
    xd::IDatabasePtr lookupOrOpenMountDb(const std::wstring& connection);
    
    bool checkCircularMount(const std::wstring& path,
                            xd::IDatabasePtr remote_db, 
                            const std::wstring remote_path);

private:

    xcm::mutex m_obj_mutex;
    std::map<std::wstring, xd::IDatabasePtr> m_mounted_dbs;
    xd::IDatabaseMgrPtr m_db_mgr;

    std::wstring m_base_path;
    int m_last_job;
    std::vector<JobInfo*> m_jobs;
    xd::IAttributesPtr m_attr;
    
    ThreadErrorInfo m_error;
};



#endif



