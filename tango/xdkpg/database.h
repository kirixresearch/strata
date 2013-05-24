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
void xdkpgStructureToXml(tango::IStructurePtr s, kl::xmlnode& node);
tango::IStructurePtr xdkpgXmlToStructure(kl::xmlnode& node);

class KpgDatabase : public tango::IDatabase
{
friend class KpgSet;
friend class KpgRowInserter;
friend class KpgIterator;
friend class KpgStream;

    XCM_CLASS_NAME("xdkpg.Database")
    XCM_BEGIN_INTERFACE_MAP(KpgDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
    XCM_END_INTERFACE_MAP()

public:

    KpgDatabase();
    ~KpgDatabase();
    
    bool open(const std::wstring& path);
    void close();

    int getDatabaseType() { return tango::dbtypeKpg; }
    void setDatabaseName(const std::wstring& name);
    std::wstring getDatabaseName();
    tango::IAttributesPtr getAttributes();
    std::wstring getActiveUid();
    
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    bool cleanup();

    tango::IJobPtr createJob();

    tango::IDatabasePtr getMountDatabase(const std::wstring& path);

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
    bool copyData(const tango::CopyParams* info, tango::IJob* job);
    bool deleteFile(const std::wstring& path);
    bool getFileExist(const std::wstring& path);
    tango::IFileInfoPtr getFileInfo(const std::wstring& path);
    tango::IFileInfoEnumPtr getFolderInfo(const std::wstring& path);
    std::wstring getPrimaryKey(const std::wstring table);
    
    tango::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, tango::IStructurePtr struct_config,  tango::FormatInfo* format_info);
    tango::IStreamPtr openStream(const std::wstring& path);
    bool createStream(const std::wstring& path, const std::wstring& mime_type);

    tango::IIteratorPtr query(const tango::QueryParams& qp);

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

    bool groupQuery(tango::GroupQueryParams* info, tango::IJob* job);

private:

    bool getStreamInfoBlock(const std::wstring& stream_name, std::wstring& output);

    std::wstring getPath();

    std::wstring getTempFileDirectory();
    std::wstring getDefinitionDirectory();
    
private:

    xcm::mutex m_obj_mutex;

    tango::IAttributesPtr m_attr;

    std::wstring m_db_name;
    std::wstring m_path;
    PkgFile* m_kpg;

    xcm::mutex m_jobs_mutex;
    int m_last_job;
    std::vector<JobInfo*> m_jobs;

    std::map<std::wstring, tango::IStructurePtr, kl::cmp_nocase> m_create_tables;
    std::map<std::wstring, std::wstring /*mime type*/, kl::cmp_nocase> m_create_streams;

    ThreadErrorInfo m_error;
};




#endif




