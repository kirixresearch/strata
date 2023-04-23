/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#ifndef H_XDEXCEL_DATABASE_H
#define H_XDEXCEL_DATABASE_H


#include <map>
#include <kl/string.h>
#include "../xdcommon/errorinfo.h"

class JobInfo;

namespace kl { class xmlnode; };
void xdkpgStructureToXml(const xd::Structure& structure, kl::xmlnode& node);
xd::Structure xdkpgXmlToStructure(kl::xmlnode& node);

namespace xlnt { class workbook; };

class ExcelDatabase : public xd::IDatabase
{
friend class ExcelSet;
friend class ExcelRowInserter;
friend class ExcelIterator;
friend class ExcelStream;
friend class ExcelFileInfo;

    XCM_CLASS_NAME("xdexcel.Database")
    XCM_BEGIN_INTERFACE_MAP(ExcelDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
    XCM_END_INTERFACE_MAP()

public:

    ExcelDatabase();
    ~ExcelDatabase();
    
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
    
    bool createTable(const std::wstring& path, const xd::FormatDefinition& format_info);
    xd::IStreamPtr openStream(const std::wstring& path);
    bool createStream(const std::wstring& path, const std::wstring& mime_type);

    xd::IIteratorPtr query(const xd::QueryParams& qp);

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

    bool groupQuery(xd::GroupQueryParams* info, xd::IJob* job);

private:

    bool getStreamInfoBlock(const std::wstring& stream_name, std::wstring& output);

    std::wstring getPath();

    std::wstring getTempFileDirectory();
    std::wstring getDefinitionDirectory();
    
private:

    kl::mutex m_obj_mutex;

    xd::IAttributesPtr m_attr;

    std::wstring m_db_name;
    std::wstring m_path;

    xlnt::workbook* m_wb;

    kl::mutex m_jobs_mutex;
    int m_last_job;
    std::vector<JobInfo*> m_jobs;

    std::map<std::wstring, xd::FormatDefinition, kl::cmp_nocase> m_create_tables;
    std::map<std::wstring, std::wstring /*mime type*/, kl::cmp_nocase> m_create_streams;

    ThreadErrorInfo m_error;
};




#endif




