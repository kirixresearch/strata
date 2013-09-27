/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-22
 *
 */


#ifndef __XDCLIENT_DATABASE_H
#define __XDCLIENT_DATABASE_H



// forward declarations
class HttpRequest;
class JobInfo;


xcm_interface IClientDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdclient.IClientDatabase")

public:

    virtual std::wstring getRequestPath(const std::wstring& path, const std::wstring& method) = 0;
};

XCM_DECLARE_SMARTPTR(IClientDatabase)



class ServerCallParams
{
public:

    void setParam(const std::wstring& param, const std::wstring& value)
    {
        m_params.push_back(std::pair<std::wstring, std::wstring>(param, value));
    }

    std::vector<std::pair<std::wstring, std::wstring> > m_params;
};


// database class definition
class ClientDatabase : public xd::IDatabase,
                       public IClientDatabase
{
    XCM_CLASS_NAME("xdclient.Database")
    XCM_BEGIN_INTERFACE_MAP(ClientDatabase)
        XCM_INTERFACE_ENTRY(xd::IDatabase)
        XCM_INTERFACE_ENTRY(IClientDatabase)
    XCM_END_INTERFACE_MAP()

public:

    ClientDatabase();
    ~ClientDatabase();

    bool open(const std::wstring& host,
              int port,
              const std::wstring& database, 
              const std::wstring& uid, 
              const std::wstring& password);

    std::wstring getRequestPath(const std::wstring& path, const std::wstring& method);
    HttpRequest* getHttpObject();

    std::wstring serverCall(const std::wstring& path,
                            const std::wstring& method,
                            const ServerCallParams* params = NULL,
                            bool use_multipart = false,
                            int timeout = 0);
    void close();

    int getDatabaseType();
    xd::IAttributesPtr getAttributes();
    std::wstring getActiveUid();
    
    std::wstring getErrorString();
    int getErrorCode();
    void setError(int error_code, const std::wstring& error_string);

    bool cleanup();

    xd::IJobPtr createJob();

    bool createFolder(const std::wstring& path);
    bool renameFile(const std::wstring& path, const std::wstring& new_name);
    bool moveFile(const std::wstring& path, const std::wstring& new_folder);
    bool copyFile(const std::wstring& src_path, const std::wstring& dest_path);
    bool copyData(const xd::CopyParams* info, xd::IJob* job);
    bool deleteFile(const std::wstring& path);
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

    xd::IStructurePtr createStructure();
    bool createTable(const std::wstring& path, xd::IStructurePtr struct_config, xd::FormatInfo* format_info);
    bool createStream(const std::wstring& path, const std::wstring& mime_type);
    xd::IStreamPtr openStream(const std::wstring& path);

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

    bool executePost(const std::wstring& command);

    bool execute(const std::wstring& command,
                unsigned int flags,
                xcm::IObjectPtr& result,
                xd::IJob* job);

    bool groupQuery(xd::GroupQueryParams* info, xd::IJob* job);

    void clearDescribeTableCache(const std::wstring& path);

public:

    xd::IStructurePtr jsonToStructure(kl::JsonNode& node);
    std::wstring structureToJson(xd::IStructurePtr structure);
    void columnToJsonNode(xd::IColumnInfoPtr colinfo, kl::JsonNode& column);
    bool getCallCacheResult(const std::wstring& path,
                            const std::wstring& method,
                            const ServerCallParams* params,
                            std::wstring& hash,
                            std::wstring& result);
    void addCallCacheResult(const std::wstring& hash, const std::wstring& value);

private:

    std::wstring m_host;
    std::wstring m_port;
    std::wstring m_database;
    std::wstring m_uid;
    std::wstring m_password;
    std::wstring m_cookie_file;

    std::vector<JobInfo*> m_jobs;
    xcm::mutex m_obj_mutex;
    int m_last_job;

    xd::IAttributesPtr m_attr;
    ThreadErrorInfo m_error;

    xcm::mutex m_http_mutex;
    std::map<xcm::threadid_t, HttpRequest*> m_http_objects;

    xcm::mutex m_describe_mutex;
    std::map<std::wstring, xd::IStructurePtr> m_describe;

    xcm::mutex m_call_cache_mutex;
    std::map<std::wstring, std::pair<time_t /*age*/, std::wstring > > m_call_cache;

    xcm::threadid_t m_connection_thread_id;
};


#endif

