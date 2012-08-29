/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-22
 *
 */


#include "xdclient.h"
#include "request.h"
#include "stream.h"
#include "nodefile.h"



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

std::wstring ClientDatabase::dbtypeToString(int type)
{
    switch (type)
    {
        default:
        case tango::typeUndefined:     return L"undefined";    
        case tango::typeInvalid:       return L"invalid";      
        case tango::typeCharacter:     return L"character";    
        case tango::typeWideCharacter: return L"widecharacter";
        case tango::typeNumeric:       return L"numeric";      
        case tango::typeDouble:        return L"double";       
        case tango::typeInteger:       return L"integer";      
        case tango::typeDate:          return L"date";         
        case tango::typeDateTime:      return L"datetime";     
        case tango::typeBoolean:       return L"boolean";      
        case tango::typeBinary:        return L"binary";       
    }
}

int ClientDatabase::stringToDbtype(const std::wstring& type)
{
         if (type == L"undefined")     return tango::typeUndefined;
    else if (type == L"invalid")       return tango::typeInvalid;
    else if (type == L"character")     return tango::typeCharacter;
    else if (type == L"widecharacter") return tango::typeWideCharacter;
    else if (type == L"numeric")       return tango::typeNumeric;
    else if (type == L"double")        return tango::typeDouble;
    else if (type == L"integer")       return tango::typeInteger;
    else if (type == L"date")          return tango::typeDate;
    else if (type == L"datetime")      return tango::typeDateTime;
    else if (type == L"boolean")       return tango::typeBoolean;
    else if (type == L"binary")        return tango::typeBinary;
    else return tango::typeUndefined;
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
        col->setType(stringToDbtype(column["type"]));
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
    column["type"] = dbtypeToString(info->getType());
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

        switch (info->getType())
        {
            default:
            case tango::typeUndefined:     column["type"] = L"undefined";      break;
            case tango::typeInvalid:       column["type"] = L"invalid";        break;
            case tango::typeCharacter:     column["type"] = L"character";      break; 
            case tango::typeWideCharacter: column["type"] = L"widecharacter";  break;
            case tango::typeNumeric:       column["type"] = L"numeric";        break;
            case tango::typeDouble:        column["type"] = L"double";         break;
            case tango::typeInteger:       column["type"] = L"integer";        break;
            case tango::typeDate:          column["type"] = L"date";           break;
            case tango::typeDateTime:      column["type"] = L"datetime";       break;
            case tango::typeBoolean:       column["type"] = L"boolean";        break;
            case tango::typeBinary:        column["type"] = L"binary";         break;
        }

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

tango::INodeValuePtr ClientDatabase::createNodeFile(const std::wstring& path)
{
    NodeFile* file = NodeFile::createFile(this, path);
    if (!file)
        return xcm::null;
    file->ref();
    tango::INodeValuePtr value = file->getRootNode();
    file->unref();
    return value;
}

tango::INodeValuePtr ClientDatabase::openNodeFile(const std::wstring& path)
{
    NodeFile* file = NodeFile::openFile(this, path);
    if (!file)
        return xcm::null;
    file->ref();
    tango::INodeValuePtr value = file->getRootNode();
    file->unref();
    return value;
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

tango::ISetPtr ClientDatabase::createSet(const std::wstring& path,
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

    /*
    return doSQL(static_cast<tango::IDatabase*>(this),
                 command, flags, result, m_error, job);
    */
}

tango::ISetPtr ClientDatabase::runGroupQuery(tango::ISetPtr sp_set,
                                             const std::wstring& group,
                                             const std::wstring& output,
                                             const std::wstring& where,
                                             const std::wstring& having,
                                             tango::IJob* job)
{
    ClientSet* set = (ClientSet*)sp_set.p;

    ServerCallParams params;
    params.setParam(L"path", set->m_path);
    params.setParam(L"group", group);
    params.setParam(L"output", output);
    params.setParam(L"where", where);
    params.setParam(L"having", having);

    std::wstring sres = serverCall(L"/api/groupquery", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;


    
    ClientSet* result_set = new ClientSet(this);
    if (!result_set->init(response["path"]))
    {
        return xcm::null;
    }

    return static_cast<tango::ISet*>(result_set);
}

