/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-22
 *
 */


#include "tango.h"
#include "database.h"
#include "request.h"
#include "set.h"
#include "iterator.h"
#include "stream.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/sqlcommon.h"
#include "../xdcommon/util.h"
#include "../../kscript/kscript.h"
#include "../../kscript/json.h"


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
    m_host = host;
    m_port = kl::itowstring(port);
    m_database = database;
    m_uid = uid;
    m_password = password;


    ServerCallParams params;
    params.setParam(L"username", L"admin");
    params.setParam(L"password", L"");
    std::wstring sres = serverCall(L"/api/login", &params);
    JsonNode response;
    response.fromString(sres);

    if (response["success"].getBoolean())
    {
        m_session_id = response["session_id"];
        if (m_session_id.length() == 0)
            return false;

        ServerCallParams params;
        std::wstring sres = serverCall(L"/api/selectdb", &params);
        JsonNode response;
        response.fromString(sres);
        if (!response["success"].getBoolean())
            return false;


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
    path.append(L"http://");
    path.append(m_host);
    if (m_port.length() > 0)
    {
        path.append(L":");
        path.append(m_port); 
    }
      
    return path;
}

std::wstring ClientDatabase::serverCall(const std::wstring& call_path,
                                        const ServerCallParams* params,
                                        bool use_multipart)
{
    std::vector<std::pair<std::wstring, std::wstring> >::const_iterator it;

    g_httprequest.resetPostParameters();
    if (use_multipart)
        g_httprequest.useMultipartPost();
    g_httprequest.setLocation(getRequestPath() + call_path);

    if (params)
    {
        for (it = params->m_params.begin(); it != params->m_params.end(); ++it)
            g_httprequest.setPostValue(it->first, it->second);
    }

    if (m_session_id.length() > 0)
        g_httprequest.setPostValue(L"sid", m_session_id);
    
    g_httprequest.send();

    return g_httprequest.getResponseString();
}

tango::IStructurePtr ClientDatabase::jsonToStructure(JsonNode& node)
{
    Structure* s = new Structure;

    JsonNode columns = node["columns"];
    size_t i = 0, cnt = columns.getCount();

    for (i = 0; i < cnt; ++i)
    {
        JsonNode column = columns[i];
        std::wstring type = column["type"];
        int ntype;

             if (type == L"undefined")     ntype = tango::typeUndefined;
        else if (type == L"invalid")       ntype = tango::typeInvalid;
        else if (type == L"character")     ntype = tango::typeCharacter;
        else if (type == L"widecharacter") ntype = tango::typeWideCharacter;
        else if (type == L"numeric")       ntype = tango::typeNumeric;
        else if (type == L"double")        ntype = tango::typeDouble;
        else if (type == L"integer")       ntype = tango::typeInteger;
        else if (type == L"date")          ntype = tango::typeDate;
        else if (type == L"datetime")      ntype = tango::typeDateTime;
        else if (type == L"boolean")       ntype = tango::typeBoolean;
        else if (type == L"binary")        ntype = tango::typeBinary;
        else ntype = tango::typeUndefined;

        tango::IColumnInfoPtr col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
        col->setName(column["name"]);
        col->setType(ntype);
        col->setWidth(column["width"].getInteger());
        col->setScale(column["scale"].getInteger());
        col->setColumnOrdinal(i);
        col->setExpression(column["expression"]);
        col->setCalculated(column["expression"].getString().length() > 0 ? true : false);

        s->addColumn(col);
    }

    return static_cast<tango::IStructure*>(s);
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
    return tango::dbtypeHttp;
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
    return false;
}

tango::INodeValuePtr ClientDatabase::createNodeFile(const std::wstring& path)
{
    return xcm::null;
}

tango::INodeValuePtr ClientDatabase::openNodeFile(const std::wstring& path)
{
    return xcm::null;
}

bool ClientDatabase::renameFile(const std::wstring& path, const std::wstring& new_name)
{
    return false;
}

bool ClientDatabase::moveFile(const std::wstring& path, const std::wstring& new_folder)
{
    return false;
}

bool ClientDatabase::copyFile(const std::wstring& src_path, const std::wstring& dest_path)
{
    return false;
}

bool ClientDatabase::deleteFile(const std::wstring& path)
{
    return false;
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
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    JsonNode file_info = response["file_info"];

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
    JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return xcm::null;

    size_t i, count;
    JsonNode items = response["items"];
    count = items.getCount();

    xcm::IVectorImpl<tango::IFileInfoPtr>* retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;

    for (i = 0; i < count; ++i)
    {
        JsonNode item = items[i];
    
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

        retval->append(f);
    }

    return retval;
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
    return xcm::null;
}

tango::ISetPtr ClientDatabase::createSet(const std::wstring& path,
                                         tango::IStructurePtr struct_config,
                                         tango::FormatInfo* format_info)
{
    return xcm::null;
}

tango::IStreamPtr ClientDatabase::openStream(const std::wstring& path)
{
    ServerCallParams params;
    params.setParam(L"path", path);
    std::wstring sres = serverCall(L"/api/openstream", &params);
    JsonNode response;
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
    JsonNode response;
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

tango::IRelationEnumPtr ClientDatabase::getRelationEnum()
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
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
    JsonNode response;
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

tango::ISetPtr ClientDatabase::runGroupQuery(tango::ISetPtr set,
                                           const std::wstring& group,
                                           const std::wstring& output,
                                           const std::wstring& where,
                                           const std::wstring& having,
                                           tango::IJob* job)
{
    return xcm::null;
}

