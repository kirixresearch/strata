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


#include <map>
#include <kl/string.h>
#include "../xdcommon/errorinfo.h"

class JobInfo;
class PkgFile;

namespace kl { class xmlnode; };
void xdkpgStructureToXml(xd::IStructurePtr s, kl::xmlnode& node);
xd::IStructurePtr xdkpgXmlToStructure(kl::xmlnode& node);

class KpgDatabase : public xd::IDatabase
{
friend class KpgSet;
friend class KpgRowInserter;
friend class KpgIterator;
friend class KpgStream;

    XCM_CLASS_NAME("xdkpg.Database")
    XCM_BEGIN_INTERFACE_MAP(KpgDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
    XCM_END_INTERFACE_MAP()

public:

    KpgDatabase();
    ~KpgDatabase();
    
    bool open(const std::wstring& path);
    void close();

    int getDatabaseType() { return xd::dbtypeKpg; }
    void setDatabaseName(const std::wstring& name);
    std::wstring getDatabaseName();
    xd::IAttributesPtr getAttributes();
    std::wstring getActiveUid();
    
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    bool cleanup();

    xd::IJobPtr createJob();

    xd::IDatabasePtr getMountDatabase(const std::wstring& path);

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
    bool copyData(const xd::CopyParams* info, xd::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    xd::IFileInfoPtr getFileInfo(const std::wstring& path);
    xd::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);
    std::wstring getPrimaryKey(const std::wstring table);
    
    xd::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, xd::IStructurePtr struct_config,  xd::FormatInfo* format_info);
    xd::IStreamPtr openStream(const std::wstring& path);
    bool createStream(const std::wstring& path, const std::wstring& mime_type);

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

    bool getStreamInfoBlock(const std::wstring& stream_name, std::wstring& output);

    std::wstring getPath();

    std::wstring getTempFileDirectory();
    std::wstring getDefinitionDirectory();
    
private:

    xcm::mutex m_obj_mutex;

    xd::IAttributesPtr m_attr;

    std::wstring m_db_name;
    std::wstring m_path;
    PkgFile* m_kpg;

    xcm::mutex m_jobs_mutex;
    int m_last_job;
    std::vector<JobInfo*> m_jobs;

    std::map<std::wstring, xd::IStructurePtr, kl::cmp_nocase> m_create_tables;
    std::map<std::wstring, std::wstring /*mime type*/, kl::cmp_nocase> m_create_streams;

    ThreadErrorInfo m_error;
};




#endif




