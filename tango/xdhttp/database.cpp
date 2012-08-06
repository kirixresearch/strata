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
#include "httprequest.h"
#include "set.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/sqlcommon.h"
#include "../xdcommon/util.h"
#include "../../kscript/kscript.h"
#include "../../kscript/json.h"


const wchar_t* xdhttp_keywords =
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

const wchar_t* xdhttp_invalid_column_chars =
                             L"~!@#$%^&*()+{}|:\"<>?`-=[]\\;',./";
const wchar_t* xdhttp_invalid_column_starting_chars =
                             L"~!@#$%^&*()+{}|:\"<>?`-=[]\\;',./ 0123456789";
const wchar_t* xdhttp_invalid_object_chars =
                             L"~!@#$%^&*()+{}|:\"<>?`-=[]\\;',/";
const wchar_t* xdhttp_invalid_object_starting_chars =
                             L"~!@#$%^&*()+{}|:\"<>?`-=[]\\;',./ 0123456789";


HttpDatabase::HttpDatabase()
{
    m_last_job = 0;

    m_attr = static_cast<tango::IAttributes*>(new DatabaseAttributes);
    m_attr->setStringAttribute(tango::dbattrKeywords, xdhttp_keywords);
    m_attr->setIntAttribute(tango::dbattrColumnMaxNameLength, 80);
    m_attr->setIntAttribute(tango::dbattrTableMaxNameLength, 80);
    m_attr->setStringAttribute(tango::dbattrColumnInvalidChars,
                               xdhttp_invalid_column_chars);
    m_attr->setStringAttribute(tango::dbattrColumnInvalidStartingChars,
                               xdhttp_invalid_column_starting_chars);
    m_attr->setStringAttribute(tango::dbattrTableInvalidChars,
                               xdhttp_invalid_object_chars);
    m_attr->setStringAttribute(tango::dbattrTableInvalidStartingChars,
                               xdhttp_invalid_object_starting_chars);
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteOpenChar, L"[");
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteCloseChar, L"]");
}

HttpDatabase::~HttpDatabase()
{
    std::vector<JobInfo*>::iterator it;
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
    {
        (*it)->unref();
    }
}

bool HttpDatabase::open(const std::wstring& host, 
                        const std::wstring& database, 
                        const std::wstring& uid, 
                        const std::wstring& password)
{
    m_host = host;
    m_database = database;
    m_uid = uid;
    m_password = password;

    return true;
}

std::wstring HttpDatabase::getRequestPath()
{
    std::wstring path;
    path.append(L"http://");
    path.append(m_host);
    path.append(L":80");
    
    return path;
}

void HttpDatabase::close()
{
}

void HttpDatabase::setDatabaseName(const std::wstring& name)
{
}

std::wstring HttpDatabase::getDatabaseName()
{
    return m_database;
}

int HttpDatabase::getDatabaseType()
{
    return tango::dbtypeClient;
}

tango::IAttributesPtr HttpDatabase::getAttributes()
{
    return m_attr;
}

std::wstring HttpDatabase::getActiveUid()
{
    return m_uid;
}

std::wstring HttpDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int HttpDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void HttpDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code);
}

double HttpDatabase::getFreeSpace()
{
    return 0.0f;
}

double HttpDatabase::getUsedSpace()
{
    return 0.0f;
}

bool HttpDatabase::cleanup()
{
    return false;
}

bool HttpDatabase::storeObject(xcm::IObject* obj, const std::wstring& path)
{
    return false;
}

tango::IJobPtr HttpDatabase::createJob()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<tango::IJob*>(job);
}

tango::IJobPtr HttpDatabase::getJob(tango::jobid_t job_id)
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

bool HttpDatabase::createFolder(const std::wstring& path)
{
    return false;
}

tango::INodeValuePtr HttpDatabase::createNodeFile(const std::wstring& path)
{
    return xcm::null;
}

tango::INodeValuePtr HttpDatabase::openNodeFile(const std::wstring& path)
{
    return xcm::null;
}

bool HttpDatabase::renameFile(const std::wstring& path, const std::wstring& new_name)
{
    return false;
}

bool HttpDatabase::moveFile(const std::wstring& path, const std::wstring& new_folder)
{
    return false;
}

bool HttpDatabase::copyFile(const std::wstring& src_path, const std::wstring& dest_path)
{
    return false;
}

bool HttpDatabase::deleteFile(const std::wstring& path)
{
    return false;
}

bool HttpDatabase::getFileExist(const std::wstring& path)
{
    return false;
}

bool HttpDatabase::getLocalFileExist(const std::wstring& path)
{
    return false;
}

tango::IFileInfoPtr HttpDatabase::getFileInfo(const std::wstring& path)
{
    return xcm::null;
}

tango::IFileInfoEnumPtr HttpDatabase::getFolderInfo(const std::wstring& path)
{
    std::wstring location = getRequestPath();
    location.append(path);

    g_httprequest.setLocation(kl::tostring(location));
    g_httprequest.send();
    std::wstring response = g_httprequest.getResponseString();

    kscript::JsonNode node;
    node.fromString(response);

    size_t i, count;
    count = node["total_count"].getInteger();
    kscript::JsonNode items = node["items"];


    xcm::IVectorImpl<tango::IFileInfoPtr>* retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;

    for (i = 0; i < count; ++i)
    {
        kscript::JsonNode item = items[i];
    
        xdcommon::FileInfo* f = new xdcommon::FileInfo;
        f->name = item["name"];
        
        std::wstring type = item["type"];
        if (type == L"folder")
            f->type = tango::filetypeFolder;
        if (type == L"node")
            f->type = tango::filetypeNode;
        if (type == L"set")
            f->type = tango::filetypeSet;
        if (type == L"stream")
            f->type = tango::filetypeStream;

        std::wstring format = item["format"];
        if (type == L"native")
            f->format = tango::formatNative;
        if (type == L"delimitedtext")
            f->format = tango::formatDelimitedText;
        if (type == L"fixedlengthtext")
            f->format = tango::formatFixedLengthText;
        if (type == L"text")
            f->format = tango::formatText;
        if (type == L"xbase")
            f->format = tango::formatXbase;            

        retval->append(f);
    }

    return retval;
}

tango::IDatabasePtr HttpDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool HttpDatabase::setMountPoint(const std::wstring& path,
                                 const std::wstring& connection_str,
                                 const std::wstring& remote_path)
{
    return false;
}
                              
bool HttpDatabase::getMountPoint(const std::wstring& path,
                                 std::wstring& connection_str,
                                 std::wstring& remote_path)
{
    return false;
}

tango::IStructurePtr HttpDatabase::createStructure()
{
    return xcm::null;
}

tango::ISetPtr HttpDatabase::createSet(const std::wstring& path, tango::IStructurePtr struct_config, tango::FormatInfo* format_info)
{
    return xcm::null;
}

tango::IStreamPtr HttpDatabase::openStream(const std::wstring& ofs_path)
{
    return xcm::null;
}

tango::IStreamPtr HttpDatabase::createStream(const std::wstring& ofs_path, const std::wstring& mime_type)
{
    return xcm::null;
}

tango::ISetPtr HttpDatabase::openSet(const std::wstring& ofs_path)
{
    HttpSet* set = new HttpSet();
    set->m_database = static_cast<tango::IDatabase*>(this);
    set->m_tablename = getTablenameFromOfsPath(ofs_path);

    if (!set->init())
    {
        delete set;
        return xcm::null;
    }

    return static_cast<tango::ISet*>(set);
}

tango::ISetPtr HttpDatabase::openSetEx(const std::wstring& ofs_path, int format)
{
    return xcm::null;
}

tango::IRelationEnumPtr HttpDatabase::getRelationEnum()
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

bool HttpDatabase::execute(const std::wstring& command,
                           unsigned int flags,
                           xcm::IObjectPtr& result,
                           tango::IJob* job)
{
    m_error.clearError();
    result.clear();

    return doSQL(static_cast<tango::IDatabase*>(this),
                 command, flags, result, m_error, job);
}

tango::ISetPtr HttpDatabase::runGroupQuery(tango::ISetPtr set,
                                           const std::wstring& group,
                                           const std::wstring& output,
                                           const std::wstring& where,
                                           const std::wstring& having,
                                           tango::IJob* job)
{
    return xcm::null;
}

