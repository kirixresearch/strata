/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-22
 *
 */


#ifndef __XDCLIENT_DATABASE_H
#define __XDCLIENT_DATABASE_H


#include "../xdcommon/errorinfo.h"


// forward declarations
class HttpRequest;
class JobInfo;

namespace kscript { class JsonNode; }
typedef kscript::JsonNode JsonNode;


xcm_interface IClientDatabase : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdclient.IClientDatabase")

public:

    virtual std::wstring getRequestPath() = 0;
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
class ClientDatabase : public tango::IDatabase,
                     public IClientDatabase
{
    friend class ClientSet;

    XCM_CLASS_NAME("xdclient.Database")
    XCM_BEGIN_INTERFACE_MAP(ClientDatabase)
        XCM_INTERFACE_ENTRY(tango::IDatabase)
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

    std::wstring getRequestPath();
    std::wstring serverCall(const std::wstring& call_path, const ServerCallParams* params = NULL, bool use_multipart = false);

    // tango::IDatabase interface

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
    tango::IStreamPtr openStream(const std::wstring& path);
    tango::IStreamPtr createStream(const std::wstring& path, const std::wstring& mime_type);
    tango::ISetPtr openSet(const std::wstring& path);
    tango::ISetPtr openSetEx(const std::wstring& path,
                             int format);
    tango::IRelationEnumPtr getRelationEnum();

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

public:

    tango::IStructurePtr jsonToStructure(JsonNode& node);
    std::wstring structureToJson(tango::IStructurePtr structure);

private:

    std::wstring m_host;
    std::wstring m_port;
    std::wstring m_database;
    std::wstring m_uid;
    std::wstring m_password;

    std::wstring m_session_id;

    std::vector<JobInfo*> m_jobs;
    xcm::mutex m_obj_mutex;
    int m_last_job;

    tango::IAttributesPtr m_attr;
    ThreadErrorInfo m_error;
};


#endif

