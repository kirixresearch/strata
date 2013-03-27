/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-22
 *
 */


#include "xdclient.h"
#include "request.h"
#include "stream.h"



const wchar_t* xdclient_keywords =
                L"ADD,ALL,ALTER,AND,ANY,AS,ASC,AVG,BEGIN,BETWEEN,BOOL,BOOLEAN,"
                L"BOTH,BREAK,BY,CASE,CHAR,CHARACTER,CHECK,CLOSE,COLLATE,"
                L"COLUMN,COMMIT,CONNECT,CONTINUE,CREATE,CURRENT,CURSOR,"
                L"DATE,DATETIME,DECIMAL,DECLARE,DEFAULT,DELETE,DESC,"
                L"DESCRIBE,DISTINCT,DO,DOUBLE,DROP,ELSE,ELSEIF,END,EXISTS,"
                L"FALSE,FETCH,FLOAT,FOR,FOREIGN,FROM,FULL,FUNCTION,GOTO,"
                L"GRANT,GROUP,HAVING,IF,IN,INDEX,INNER,INSERT,INT,INTEGER,"
                L"INTERSECT,INTO,IS,JOIN,KEY,LEFT,LEVEL,LIKE,LONG,MATCH,MIN,MAX,NEW,"
                L"NOT,NULL,NUMERIC,OF,ON,OPEN,OPTION,OR,ORDER,OUTER,PRIMARY,"
                L"PRIVATE,PRECISION,PRIVILEGES,PROCEDURE,PROTECTED,PUBLIC,"
                L"READ,RESTRICT,RETURN,REVOKE,RIGHT,ROWS,SELECT,SESSION,SET,"
                L"SIZE,SHORT,SIGNED,SMALLINT,SOME,SUM,SWITCH,TABLE,THEN,THIS,TO,"
                L"TRUE,UNION,UNIQUE,UNSIGNED,UPDATE,USER,USING,VALUES,VARCHAR,"
                L"VARYING,VIEW,VOID,WHEN,WHENEVER,WHERE,WHILE,WITH";

const wchar_t* xdclient_invalid_column_chars =
                             L"~!@#$%^&*()+{}|:\"<>?`-=[]\\;',./";
const wchar_t* xdclient_invalid_column_starting_chars =
                             L"~!@#$%^&*()+{}|:\"<>?`-=[]\\;',./ 0123456789";
const wchar_t* xdclient_invalid_object_chars =
                             L"~!@#$%^&*()+{}|:\"<>?`-=[]\\;',/";
const wchar_t* xdclient_invalid_object_starting_chars =
                             L"~!@#$%^&*()+{}|:\"<>?`-=[]\\;',./ 0123456789";



ClientDatabase::ClientDatabase()
{
    m_last_job = 0;
    m_connection_thread_id = 0;

    m_attr = static_cast<tango::IAttributes*>(new DatabaseAttributes);
    m_attr->setStringAttribute(tango::dbattrKeywords, xdclient_keywords);
    m_attr->setIntAttribute(tango::dbattrColumnMaxNameLength, 80);
    m_attr->setIntAttribute(tango::dbattrTableMaxNameLength, 80);
    m_attr->setStringAttribute(tango::dbattrColumnInvalidChars,
                               xdclient_invalid_column_chars);
    m_attr->setStringAttribute(tango::dbattrColumnInvalidStartingChars,
                               xdclient_invalid_column_starting_chars);
    m_attr->setStringAttribute(tango::dbattrTableInvalidChars,
                               xdclient_invalid_object_chars);
    m_attr->setStringAttribute(tango::dbattrTableInvalidStartingChars,
                               xdclient_invalid_object_starting_chars);
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteOpenChar, L"[");
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteCloseChar, L"]");
}

ClientDatabase::~ClientDatabase()
{
    std::vector<JobInfo*>::iterator it;
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
    {
        (*it)->unref();
    }
}

bool ClientDatabase::open(const std::wstring& host,
                          int port,
                          const std::wstring& database, 
                          const std::wstring& uid, 
                          const std::wstring& password)
{
    m_connection_thread_id = xcm::get_current_thread_id();

    m_host = host;
    m_port = kl::itowstring(port);
    m_database = database;
    m_uid = uid;
    m_password = password;


    ServerCallParams params;
    params.setParam(L"username", L"admin");
    params.setParam(L"password", L"");
    std::wstring sres = serverCall(L"/api/login", &params, false, 15 /*seconds*/);
    kl::JsonNode response;
    response.fromString(sres);

    if (response["success"].getBoolean())
    {
        m_session_id = response["session_id"];
        if (m_session_id.length() == 0)
            return false;

        ServerCallParams params;
        params.setParam(L"database", m_database);
        std::wstring sres = serverCall(L"/api/selectdb", &params);
        kl::JsonNode response;
        response.fromString(sres);
        if (!response["success"].getBoolean())
            return false;

        std::wstring url = L"sdserv://";
        url += m_host;
        url += L":";
        url += m_port;
        url += L"/";
        url += m_database;
        m_attr->setStringAttribute(tango::dbattrDatabaseUrl, url);


        return true;
    }
     else
    {
        return false;
    }

}

std::wstring ClientDatabase::getRequestPath()
{
    std::wstring path;

    if (m_port == L"4820")
        path.append(L"https://");
         else
        path.append(L"http://");

    path.append(m_host);
    if (m_port.length() > 0)
    {
        path.append(L":");
        path.append(m_port); 
    }
      
    return path;
}

HttpRequest* ClientDatabase::getHttpObject()
{
    XCM_AUTO_LOCK(m_http_mutex);

    xcm::threadid_t thread_id = xcm::get_current_thread_id();

    std::map<xcm::threadid_t, HttpRequest*>::iterator it;
    it = m_http_objects.find(thread_id);
    if (it != m_http_objects.end())
    {
        return it->second;
    }
     else
    {
        HttpRequest* req = new HttpRequest;
        m_http_objects[thread_id] = req;
        return req;
    }
}


std::wstring ClientDatabase::serverCall(const std::wstring& call_path,
                                        const ServerCallParams* params,
                                        bool use_multipart,
                                        int timeout)
{
    std::vector<std::pair<std::wstring, std::wstring> >::const_iterator it;

    HttpRequest* http = getHttpObject();

    http->setTimeout(timeout);
    http->resetPostParameters();
    if (use_multipart)
        http->useMultipartPost();
    http->setLocation(getRequestPath() + call_path);

    if (params)
    {
        for (it = params->m_params.begin(); it != params->m_params.end(); ++it)
            http->setPostValue(it->first, it->second);
    }

    if (m_session_id.length() > 0)
        http->setPostValue(L"sid", m_session_id);
    
    http->send();

    std::wstring result = http->getResponseString();

    if (xcm::get_current_thread_id() != m_connection_thread_id)
    {
        // keep connection open only on main database connection
        http->close();
    }

    return result;
}

tango::IStructurePtr ClientDatabase::jsonToStructure(kl::JsonNode& node)
{
    Structure* s = new Structure;

    kl::JsonNode columns = node["columns"];
    size_t i = 0, cnt = columns.getChildCount();

    for (i = 0; i < cnt; ++i)
    {
        kl::JsonNode column = columns[i];

        tango::IColumnInfoPtr col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
        col->setName(column["name"]);
        col->setType(tango::stringToDbtype(column["type"]));
        col->setWidth(column["width"].getInteger());
        col->setScale(column["scale"].getInteger());
        col->setColumnOrdinal(i);
        col->setExpression(column["expression"]);
        col->setCalculated(column["expression"].getString().length() > 0 ? true : false);

        s->addColumn(col);
    }

    return static_cast<tango::IStructure*>(s);
}


void ClientDatabase::columnToJsonNode(tango::IColumnInfoPtr info, kl::JsonNode& column)
{  
    column["name"] = info->getName();
    column["type"] = tango::dbtypeToString(info->getType());
    column["width"].setInteger(info->getWidth());
    column["scale"].setInteger(info->getScale());   
    column["expression"] = info->getExpression();
    column["calculated"].setBoolean(info->getCalculated());
}




std::wstring ClientDatabase::structureToJson(tango::IStructurePtr structure)
{
    // set the total number of items
    int idx, count = structure->getColumnCount();

    // set the items
    kl::JsonNode columns;
    columns.setArray();
    for (idx = 0; idx < count; ++idx)
    {
        kl::JsonNode column = columns.appendElement();
            
        tango::IColumnInfoPtr info = structure->getColumnInfoByIdx(idx);
        column["name"] = info->getName();
        column["type"] = tango::dbtypeToString(info->getType());
        column["width"].setInteger(info->getWidth());
        column["scale"].setInteger(info->getScale());
        column["expression"] = info->getExpression();
    }

    return columns.toString();
}


void ClientDatabase::close()
{
}

void ClientDatabase::setDatabaseName(const std::wstring& name)
{
}

std::wstring ClientDatabase::getDatabaseName()
{
    return m_database;
}

int ClientDatabase::getDatabaseType()
{
    return tango::dbtypeClient;
}

tango::IAttributesPtr ClientDatabase::getAttributes()
{
    return m_attr;
}

std::wstring ClientDatabase::getActiveUid()
{
    return m_uid;
}

std::wstring ClientDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int ClientDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void ClientDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code);
}

double ClientDatabase::getFreeSpace()
{
    return 0.0f;
}

double ClientDatabase::getUsedSpace()
{
    return 0.0f;
}

bool ClientDatabase::cleanup()
{
    return false;
}

bool ClientDatabase::storeObject(xcm::IObject* obj, const std::wstring& path)
{
    tango::ISetPtr sp_set = obj;
    if (sp_set.isNull())
        return false;

    ClientSet* set = (ClientSet*)obj;
    
    if (moveFile(set->m_path, path))
    {
        set->m_path = path;
        return true;
    }

    return false;
}

tango::IJobPtr ClientDatabase::createJob()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<tango::IJob*>(job);
}

tango::IJobPtr ClientDatabase::getJob(tango::jobid_t job_id)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::vector<JobInfo*>::iterator it;
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
    {
        if ((*it)->getJobId() == job_id)
        {
            return static_cast<tango::IJob*>(*it);
        }
    }

    return xcm::null;
}

bool ClientDatabase::createFolder(const std::wstring& path)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    std::wstring sres = serverCall(L"/api/createfolder", &params);
    kl::JsonNode response;
    response.fromString(sres);

    return response["success"].getBoolean();
}

bool ClientDatabase::renameFile(const std::wstring& path, const std::wstring& new_name)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    params.setParam(L"new_name", new_name);
    std::wstring sres = serverCall(L"/api/renamefile", &params);
    kl::JsonNode response;
    response.fromString(sres);

    return response["success"].getBoolean();
}

bool ClientDatabase::moveFile(const std::wstring& path, const std::wstring& destination_folder)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    params.setParam(L"destination", destination_folder);
    std::wstring sres = serverCall(L"/api/movefile", &params);
    kl::JsonNode response;
    response.fromString(sres);

    return response["success"].getBoolean();
}

bool ClientDatabase::copyFile(const std::wstring& src_path, const std::wstring& dest_path)
{
    return false;
}

bool ClientDatabase::copyData(const tango::CopyInfo* info, tango::IJob* job)
{
    return false;
}

bool ClientDatabase::deleteFile(const std::wstring& path)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    std::wstring sres = serverCall(L"/api/deletefile", &params);
    kl::JsonNode response;
    response.fromString(sres);

    return response["success"].getBoolean();
}

bool ClientDatabase::getFileExist(const std::wstring& path)
{
    return getFileInfo(path).isOk();
}

bool ClientDatabase::getLocalFileExist(const std::wstring& path)
{
    return false;
}

tango::IFileInfoPtr ClientDatabase::getFileInfo(const std::wstring& path)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    std::wstring sres = serverCall(L"/api/fileinfo", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    kl::JsonNode file_info = response["file_info"];

    xdcommon::FileInfo* f = new xdcommon::FileInfo;
    f->name = file_info["name"];
    
    std::wstring type = file_info["type"];
         if (type == L"folder")          f->type = tango::filetypeFolder;
    else if (type == L"node")            f->type = tango::filetypeNode;
    else if (type == L"set")             f->type = tango::filetypeSet;
    else if (type == L"table")           f->type = tango::filetypeSet;
    else if (type == L"stream")          f->type = tango::filetypeStream;
    else f->type = tango::filetypeSet;

    std::wstring format = file_info["format"];
         if (format == L"native")          f->format = tango::formatNative;
    else if (format == L"delimitedtext")   f->format = tango::formatDelimitedText;
    else if (format == L"fixedlengthtext") f->format = tango::formatFixedLengthText;
    else if (format == L"text")            f->format = tango::formatText;
    else if (format == L"xbase")           f->format = tango::formatXbase;            
    else f->format = tango::formatNative;

    f->mime_type = file_info["mime_type"];
    f->is_mount = file_info["is_mount"].getBoolean();
    f->primary_key = file_info["primary_key"];
    f->size = (tango::tango_int64_t)file_info["size"].getDouble();

    return static_cast<tango::IFileInfo*>(f);
}



tango::IFileInfoEnumPtr ClientDatabase::getFolderInfo(const std::wstring& path)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    std::wstring sres = serverCall(L"/api/folderinfo", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    size_t i, count;
    kl::JsonNode items = response["items"];
    count = items.getChildCount();

    xcm::IVectorImpl<tango::IFileInfoPtr>* retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;

    for (i = 0; i < count; ++i)
    {
        kl::JsonNode item = items[i];
    
        xdcommon::FileInfo* f = new xdcommon::FileInfo;
        f->name = item["name"];
        
        std::wstring type = item["type"];
             if (type == L"folder")          f->type = tango::filetypeFolder;
        else if (type == L"node")            f->type = tango::filetypeNode;
        else if (type == L"set")             f->type = tango::filetypeSet;
        else if (type == L"table")           f->type = tango::filetypeSet;
        else if (type == L"stream")          f->type = tango::filetypeStream;
        else f->type = tango::filetypeSet;


        std::wstring format = item["format"];
             if (format == L"native")          f->format = tango::formatNative;
        else if (format == L"delimitedtext")   f->format = tango::formatDelimitedText;
        else if (format == L"fixedlengthtext") f->format = tango::formatFixedLengthText;
        else if (format == L"text")            f->format = tango::formatText;
        else if (format == L"xbase")           f->format = tango::formatXbase;            
        else f->format = tango::formatNative;

        
        f->mime_type = item["mime_type"];
        f->is_mount = item["is_mount"].getBoolean();
        f->primary_key = item["primary_key"];
        f->size = (tango::tango_int64_t)item["size"].getDouble();

        retval->append(f);
    }

    return retval;
}


tango::IRowInserterPtr ClientDatabase::bulkInsert(const std::wstring& path)
{
    return xcm::null;
}

tango::IStructurePtr ClientDatabase::describeTable(const std::wstring& path)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    std::wstring sres = serverCall(L"/api/describetable", &params);

    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    return jsonToStructure(response);
}

bool ClientDatabase::modifyStructure(const std::wstring& path, tango::IStructurePtr struct_config, tango::IJob* job)
{
    return false;
}



void ClientDatabase::clearDescribeTableCache(const std::wstring& path)
{
}



tango::IDatabasePtr ClientDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool ClientDatabase::setMountPoint(const std::wstring& path,
                                   const std::wstring& connection_str,
                                   const std::wstring& remote_path)
{
    return false;
}
                              
bool ClientDatabase::getMountPoint(const std::wstring& path,
                                   std::wstring& connection_str,
                                   std::wstring& remote_path)
{
    return false;
}

tango::IStructurePtr ClientDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<tango::IStructure*>(s);
}

tango::ISetPtr ClientDatabase::createTable(const std::wstring& path,
                                         tango::IStructurePtr structure,
                                         tango::FormatInfo* format_info)
{

    std::wstring columns = structureToJson(structure);


    ServerCallParams params;
    params.setParam(L"path", path);
    params.setParam(L"columns", columns);
    std::wstring sres = serverCall(L"/api/createtable", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;


    ClientSet* set = new ClientSet(this);

    if (!set->init(response["path"]))
    {
        delete set;
        return xcm::null;
    }

    return static_cast<tango::ISet*>(set);
}

tango::IStreamPtr ClientDatabase::openStream(const std::wstring& path)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    std::wstring sres = serverCall(L"/api/openstream", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    return static_cast<tango::IStream*>(new ClientStream(this, response["handle"]));
}

tango::IStreamPtr ClientDatabase::createStream(const std::wstring& path, const std::wstring& mime_type)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    params.setParam(L"mime_type", mime_type);
    std::wstring sres = serverCall(L"/api/createstream", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    return static_cast<tango::IStream*>(new ClientStream(this, response["handle"]));

}

tango::ISetPtr ClientDatabase::openSet(const std::wstring& path)
{
    ClientSet* set = new ClientSet(this);


    if (!set->init(path))
    {
        delete set;
        return xcm::null;
    }

    return static_cast<tango::ISet*>(set);
}

tango::ISetPtr ClientDatabase::openSetEx(const std::wstring& path, int format)
{
    return openSet(path);
}


tango::IIteratorPtr ClientDatabase::createIterator(const std::wstring& path,
                                                   const std::wstring& columns,
                                                   const std::wstring& sort,
                                                   tango::IJob* job)
{
    tango::ISetPtr set = openSet(path);
    if (set.isNull())
        return xcm::null;
    return set->createIterator(columns, sort, job);
}


tango::IRelationEnumPtr ClientDatabase::getRelationEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}


tango::IRelationPtr ClientDatabase::getRelation(const std::wstring& relation_id)
{
    return xcm::null;
}

tango::IRelationPtr ClientDatabase::createRelation(const std::wstring& tag,
                                                   const std::wstring& left_set_path,
                                                   const std::wstring& right_set_path,
                                                   const std::wstring& left_expr,
                                                   const std::wstring& right_expr)
{
    return xcm::null;
}

bool ClientDatabase::deleteRelation(const std::wstring& relation_id)
{
    return false;
}



tango::IIndexInfoPtr ClientDatabase::createIndex(const std::wstring& path,
                                                 const std::wstring& name,
                                                 const std::wstring& expr,
                                                 tango::IJob* job)
{
    return xcm::null;
}


bool ClientDatabase::renameIndex(const std::wstring& path,
                                 const std::wstring& name,
                                 const std::wstring& new_name)
{
    return false;
}


bool ClientDatabase::deleteIndex(const std::wstring& path,
                                 const std::wstring& name)
{
    return false;
}


tango::IIndexInfoEnumPtr ClientDatabase::getIndexEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IIndexInfoEnumPtr>* vec;
    vec = new xcm::IVectorImpl<tango::IIndexInfoEnumPtr>;

    return vec;
}

bool ClientDatabase::execute(const std::wstring& command,
                             unsigned int flags,
                             xcm::IObjectPtr& result,
                             tango::IJob* job)
{
    m_error.clearError();
    result.clear();


    ServerCallParams params;
    params.setParam(L"mode", L"sql");
    params.setParam(L"sql", command);


    std::wstring sres = serverCall(L"/api/query", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
    {
        return false;
    }



    // initialize a placeholder set
    std::wstring path = getTableNameFromSql(command);
    ClientSet* set = new ClientSet(this);
    if (!set->init(path))
    {
        return xcm::null;
    }

    if (response["row_count"].isOk())
    {
        set->m_known_row_count = (tango::rowpos_t)response["row_count"].getDouble();
    }

    // initialize the iterator
    ClientIterator* iter = new ClientIterator(this, set);
    if (!iter->init(response["handle"], L""))
    {
        delete iter;
        return xcm::null;
    }
    

    tango::IIteratorPtr sp_iter = static_cast<tango::IIterator*>(iter);
    result = sp_iter;
    return true;
}

bool ClientDatabase::groupQuery(tango::GroupQueryInfo* info, tango::IJob* job)
{
    return false;
}

