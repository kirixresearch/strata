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

    std::wstring url = L"sdserv://";
    url += m_host;
    url += L":";
    url += m_port;
    url += L"/";
    url += (m_database.length() > 0 && m_database[0] == '/') ? m_database.substr(1) : m_database;
    m_attr->setStringAttribute(tango::dbattrDatabaseUrl, url);

    return true;
}

std::wstring ClientDatabase::getRequestPath(const std::wstring& path, const std::wstring& method)
{
    std::wstring res;

    if (path.find(L"://") != path.npos)
    {
        res = path;
        kl::replaceStr(res, L"sdservs:", L"https:");
        kl::replaceStr(res, L"sdserv:", L"http:");
        if (method.length() > 0)
            res += (L"?m=" + method);
        return res;
    }


    if (m_port == L"4820")
        res.append(L"https://");
         else
        res.append(L"http://");

    res.append(m_host);
    if (m_port.length() > 0 && m_port != L"0")
    {
        res.append(L":");
        res.append(m_port); 
    }

    if (path.length() > 0)
    {
        if (path[0] != '/')
            res += '/';
            
        res += path;
    }

    if (method.length() > 0)
        res += (L"?m=" + method);

    return res;
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


std::wstring ClientDatabase::serverCall(const std::wstring& path,
                                        const std::wstring& method,
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

    std::wstring full_path = getRequestPath(path, method);

    http->setLocation(full_path);

    if (params)
    {
        for (it = params->m_params.begin(); it != params->m_params.end(); ++it)
            http->setPostValue(it->first, it->second);
    }

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

bool ClientDatabase::cleanup()
{
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


bool ClientDatabase::createFolder(const std::wstring& path)
{
    ServerCallParams params;
    std::wstring sres = serverCall(path, L"createfolder", &params);
    kl::JsonNode response;
    response.fromString(sres);

    return response["success"].getBoolean();
}

bool ClientDatabase::renameFile(const std::wstring& path, const std::wstring& new_name)
{
    ServerCallParams params;
    params.setParam(L"new_name", new_name);
    std::wstring sres = serverCall(path, L"renamefile", &params);
    kl::JsonNode response;
    response.fromString(sres);

    return response["success"].getBoolean();
}

bool ClientDatabase::moveFile(const std::wstring& path, const std::wstring& destination_folder)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    params.setParam(L"destination", destination_folder);
    std::wstring sres = serverCall(L"", L"movefile", &params);
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
    std::wstring handle;
    IClientIteratorPtr iter = info->iter_input;
    if (iter.isOk())
        handle = iter->getHandle();

    ServerCallParams params;
    if (handle.length() > 0)
        params.setParam(L"input_iter", handle);
         else
        params.setParam(L"input", info->input);
    params.setParam(L"output", info->output);
    params.setParam(L"order", info->order);
    params.setParam(L"where", info->where);
    params.setParam(L"limit", kl::itowstring(info->limit));

    std::wstring sres = serverCall(info->input, L"copydata", &params);
    kl::JsonNode response;
    response.fromString(sres);

    return response["success"].getBoolean();
}

bool ClientDatabase::deleteFile(const std::wstring& path)
{
    ServerCallParams params;
    std::wstring sres = serverCall(path, L"deletefile", &params);
    kl::JsonNode response;
    response.fromString(sres);

    return response["success"].getBoolean();
}

bool ClientDatabase::getFileExist(const std::wstring& path)
{
    return getFileInfo(path).isOk();
}


tango::IFileInfoPtr ClientDatabase::getFileInfo(const std::wstring& path)
{
    ServerCallParams params;
    std::wstring sres = serverCall(path, L"fileinfo", &params);
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
    else if (type == L"set")             f->type = tango::filetypeTable;
    else if (type == L"table")           f->type = tango::filetypeTable;
    else if (type == L"stream")          f->type = tango::filetypeStream;
    else f->type = tango::filetypeTable;

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
    f->size = (long long)file_info["size"].getDouble();
    f->object_id = file_info["object_id"];

    return static_cast<tango::IFileInfo*>(f);
}



tango::IFileInfoEnumPtr ClientDatabase::getFolderInfo(const std::wstring& path)
{
    ServerCallParams params;
    std::wstring sres = serverCall(path, L"folderinfo", &params);
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
        else if (type == L"set")             f->type = tango::filetypeTable;
        else if (type == L"table")           f->type = tango::filetypeTable;
        else if (type == L"stream")          f->type = tango::filetypeStream;
        else f->type = tango::filetypeTable;


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
        f->size = (long long)item["size"].getDouble();

        retval->append(f);
    }

    return retval;
}


tango::IRowInserterPtr ClientDatabase::bulkInsert(const std::wstring& path)
{
    return static_cast<tango::IRowInserter*>(new ClientRowInserter(this, path));
}

tango::IStructurePtr ClientDatabase::describeTable(const std::wstring& path)
{
    ServerCallParams params;
    std::wstring sres = serverCall(path, L"describetable", &params);

    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    return jsonToStructure(response);
}


bool ClientDatabase::modifyStructure(const std::wstring& path, tango::IStructurePtr struct_config, tango::IJob* job)
{
/*
    if (!struct_config)
        return false;

    tango::IStructurePtr orig_structure = getStructure();
    if (orig_structure.isNull())
        return false;

    m_database->clearDescribeTableCache(m_path);


    tango::IStructurePtr structure = struct_config;
    IStructureInternalPtr struct_internal = structure;
    if (struct_internal.isNull())
        return false;


    kl::JsonNode json_actions;
    json_actions.setArray();

    std::vector<StructureAction>& actions = struct_internal->getStructureActions();
    std::vector<StructureAction>::iterator it;
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        kl::JsonNode json_action = json_actions.appendElement();
        
        switch (it->m_action)
        {
            case StructureAction::actionCreate:  json_action[L"action"] = L"create"; break;
            case StructureAction::actionModify:  json_action[L"action"] = L"modify"; break;
            case StructureAction::actionDelete:  json_action[L"action"] = L"delete"; break;
            case StructureAction::actionMove:    json_action[L"action"] = L"move";   break;
            case StructureAction::actionInsert:  json_action[L"action"] = L"insert"; break;
        }

        if (it->m_action == StructureAction::actionModify ||
            it->m_action == StructureAction::actionDelete ||
            it->m_action == StructureAction::actionMove ||
            it->m_action == StructureAction::actionInsert)
        {
            json_action[L"target_column"] = it->m_colname;
        }

        if (it->m_action == StructureAction::actionInsert)
        {
            json_action[L"position"] = it->m_pos;
        }


        if (it->m_params.isOk())
        {
            tango::IColumnInfoPtr orig_colinfo = orig_structure->getColumnInfo(it->m_colname);

            if (it->m_params->getName().length() > 0)
                json_action["params"]["name"] = it->m_params->getName();

            if (it->m_params->getType() != -1)
                json_action["params"]["type"] = tango::dbtypeToString(it->m_params->getType());

            if (it->m_params->getWidth() != -1)
                json_action["params"]["width"].setInteger(it->m_params->getWidth());

            if (it->m_params->getScale() != -1)
                json_action["params"]["scale"].setInteger(it->m_params->getScale());   

            if (it->m_params->getExpression().length() > 0)
                json_action["params"]["expression"] = it->m_params->getExpression();

            if (orig_colinfo.isNull() || it->m_params->getCalculated() != orig_colinfo->getCalculated())
                json_action["params"]["calculated"].setBoolean(it->m_params->getCalculated());
        }
    }



    ServerCallParams params;
    params.setParam(L"path", m_path);
    params.setParam(L"actions", json_actions.toString());
    
    std::wstring sres = m_database->serverCall(L"", L"alter", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    return true;
    */



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

bool ClientDatabase::createTable(const std::wstring& path,
                                 tango::IStructurePtr structure,
                                 tango::FormatInfo* format_info)
{
    std::wstring columns = structureToJson(structure);

    ServerCallParams params;
    params.setParam(L"columns", columns);
    std::wstring sres = serverCall(path, L"createtable", &params);
    kl::JsonNode response;
    response.fromString(sres);

    return response["success"].getBoolean();
}

tango::IStreamPtr ClientDatabase::openStream(const std::wstring& path)
{
    ServerCallParams params;
    std::wstring sres = serverCall(path, L"openstream", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    return static_cast<tango::IStream*>(new ClientStream(this, response["handle"]));
}

bool ClientDatabase::createStream(const std::wstring& path, const std::wstring& mime_type)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    params.setParam(L"mime_type", mime_type);
    std::wstring sres = serverCall(L"", L"createstream", &params);
    kl::JsonNode response;
    response.fromString(sres);

    return response["success"].getBoolean();
}


tango::IIteratorPtr ClientDatabase::createIterator(const std::wstring& path,
                                                   const std::wstring& columns,
                                                   const std::wstring& wherec,
                                                   const std::wstring& order,
                                                   tango::IJob* job)
{
    std::wstring request_url;
    if (path.find(L"://") != path.npos)
        request_url = path;

    ServerCallParams params;
    params.setParam(L"columns", columns);
    params.setParam(L"where", wherec);
    params.setParam(L"order", order);
    params.setParam(L"limit", L"5");

    std::wstring sres = serverCall(path, L"", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
    {
        return xcm::null;
    }

    // initialize the iterator
    ClientIterator* iter = new ClientIterator(this);
    if (!iter->init(response["handle"], request_url))
    {
        delete iter;
        return xcm::null;
    }
    
    return static_cast<tango::IIterator*>(iter);
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


    std::wstring sres = serverCall(L"", L"query", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
    {
        return false;
    }



    // initialize the iterator
    ClientIterator* iter = new ClientIterator(this);
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

