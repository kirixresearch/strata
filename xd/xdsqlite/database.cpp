/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <queue>

#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/file.h>
#include <kl/regex.h>
#include <kl/md5.h>


#include <xd/xd.h>
#include "database.h"
#include "stream.h"
#include "inserter.h"
#include "iterator.h"
#include "util.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/jobinfo.h"


const wchar_t* sql92_keywords =
                L"ABSOLUTE,ACTION,ADA,ADD,ALL,ALLOCATE,ALTER,AND,"
                L"ANY,ARE,AS,ASC,ASSERTION,AT,AUTHORIZATION,AVG,"
                L"BEGIN,BETWEEN,BIT,BIT_LENGTH,BOTH,BY,CASCADE,"
                L"CASCADED,CASE,CAST,CATALOG,CHAR,CHAR_LENGTH,"
                L"CHARACTER,CHARACTER_LENGTH,CHECK,CLOSE,COALESCE,"
                L"COLLATE,COLLATION,COLUMN,COMMIT,CONNECT,"
                L"CONNECTION,CONSTRAINT,CONSTRAINTS,CONTINUE,"
                L"CONVERT,CORRESPONDING,COUNT,CREATE,CROSS,CURRENT,"
                L"CURRENT_DATE,CURRENT_TIME,CURRENT_TIMESTAMP,"
                L"CURRENT_USER,CURSOR,DATE,DAY,DEALLOCATE,DEC,"
                L"DECIMAL,DECLARE,DEFAULT,DEFERRABLE,DEFERRED,"
                L"DELETE,DESC,DESCRIBE,DESCRIPTOR,DIAGNOSTICS,"
                L"DISCONNECT,DISTINCT,DOMAIN,DOUBLE,DROP,ELSE,END,"
                L"END-EXEC,ESCAPE,EXCEPT,EXCEPTIONEXEC,EXECUTE,"
                L"EXISTS,EXTERNAL,EXTRACT,FALSE,FETCH,FIRST,FLOAT,"
                L"FOR,FOREIGN,FORTRAN,FOUND,FROM,FULL,GET,GLOBAL,"
                L"GO,GOTO,GRANT,GROUP,HAVING,HOUR,IDENTITY,"
                L"IMMEDIATE,IN,INCLUDE,INDEX,INDICATOR,INITIALLY,"
                L"INNER,INPUT,INSENSITIVE,INSERT,INT,INTEGER,"
                L"INTERSECT,INTERVAL,INTO,IS,ISOLATION,JOIN,KEY,"
                L"LANGUAGE,LAST,LEADING,LEFT,LEVEL,LIKE,LOCAL,"
                L"LOWER,MATCH,MAX,MIN,MINUTE,MODULE,MONTH,NAMES,"
                L"NATIONAL,NATURAL,NCHAR,NEXT,NO,NONE,NOT,NULL,"
                L"NULLIF,NUMERIC,OCTET_LENGTH,OF,ON,ONLY,OPEN,"
                L"OPTION,OR,ORDER,OUTER,OUTPUT,OVERLAPS,PAD,"
                L"PARTIAL,PASCAL,POSITION,PRECISION,PREPARE,"
                L"PRESERVE,PRIMARY,PRIOR,PRIVILEGES,PROCEDURE,"
                L"PUBLIC,READ,REAL,REFERENCES,RELATIVE,RESTRICT,"
                L"REVOKE,RIGHT,ROLLBACK,ROWS,SCHEMA,SCROLL,"
                L"SECOND,SECTION,SELECT,SESSION,SESSION_USER,SET,"
                L"SIZE,SMALLINT,SOME,SPACE,SQL,SQLCA,SQLCODE,"
                L"SQLERROR,SQLSTATE,SQLWARNING,SUBSTRING,SUM,"
                L"SYSTEM_USER,TABLE,TEMPORARY,THEN,TIME,TIMESTAMP,"
                L"TIMEZONE_HOUR,TIMEZONE_MINUTE,TO,TRAILING,"
                L"TRANSACTION,TRANSLATE,TRANSLATION,TRIM,TRUE,"
                L"UNION,UNIQUE,UNKNOWN,UPDATE,UPPER,USAGE,USER,"
                L"USING,VALUE,VALUES,VARCHAR,VARYING,VIEW,WHEN,"
                L"WHENEVER,WHERE,WITH,WORK,WRITE,YEAR,ZONE";


#define XDSQLITE_PATH_SEPARATOR L"___"
#define XDSQLITE_PATH_SEPARATOR_LEN 3
#define XDSQLITE_PATH_SEPARATOR_LIKE L"\\_\\_\\_"

std::wstring xdGetTablenameFromPath(const std::wstring& path, bool quote /* = false */)
{
    const wchar_t* p = path.c_str();
    if (*p == L'/')
    {
        ++p;
    }

    if (NULL == std::wcspbrk(p, L" /\""))
    {
        return p;
    }
    else
    {
        std::wstring result;

        std::vector<std::wstring> parts;
        kl::parseDelimitedList(path, parts, L'/');
        for (auto& part : parts)
        {
            if (!result.empty())
            {
                result += XDSQLITE_PATH_SEPARATOR;
            }

            result += part;
        }

        if (result.find('"') != result.npos)
        {
            kl::replaceStr(result, L"\"", L"\"\"");
        }

        if (quote)
        {
            return L"\"" + result + L"\"";
        }
        else
        {
            return result;
        }
    }
}


#define MAX_POOL_SIZE 10

class SlConnectionPool
{

public:

    SlConnectionPool()
    {
        
    }

    void setPath(const std::wstring& path)
    {
        m_path = kl::tostring(path);
    }

    sqlite3* getConnection()
    {
        m_mutex.lock();
        if (!m_queue.empty())
        {
            sqlite3* ret = m_queue.front();
            m_queue.pop();
            m_mutex.unlock();
            return ret;
        }
        else
        {
            m_mutex.unlock();
            sqlite3* db = NULL;
            if (m_path.empty() || SQLITE_OK != sqlite3_open(m_path.c_str(), &db))
                return NULL;
            return db;
        }
    }

    void freeConnection(sqlite3* db)
    {
        m_mutex.lock();
        if (m_queue.size() >= MAX_POOL_SIZE)
        {
            m_mutex.unlock();
            sqlite3_close(db);
        }
        else
        {
            m_queue.push(db);
            m_mutex.unlock();
        }
    }

    void closeAll()
    {
        sqlite3* db;

        while (true)
        {
            m_mutex.lock();
            if (m_queue.empty())
            {
                m_mutex.unlock();
                break;
            }
            db = m_queue.front();
            m_queue.pop();
            m_mutex.unlock();

            sqlite3_close(db);
        }
    }

private:

    std::string m_path;
    std::queue<sqlite3*> m_queue;
    kl::mutex m_mutex;
};




SlDatabase::SlDatabase()
{
    m_connection_pool = new SlConnectionPool;
    m_sqlite = NULL;
    m_last_job = 0;
}

SlDatabase::~SlDatabase()
{
    close();

    std::vector<JobInfo*>::iterator it;
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
    {
        (*it)->unref();
    }

    delete m_connection_pool;
    m_connection_pool = NULL;
}

bool SlDatabase::createDatabase(const std::wstring& path)
{
    m_sqlite = NULL;
    m_path = L"";

    if (xf_get_file_exist(path))
    {
        // database already exists; overwrite it
        if (!xf_remove(path))
        {
            return false;
        }
    }


    std::string ascpath = kl::tostring(path);
    sqlite3* db = NULL;

    if (SQLITE_OK != sqlite3_open(ascpath.c_str(), &db))
    {
        // database could not be opened
        return false;
    }

    m_sqlite = db;
    m_path = path;

    m_connection_pool->setPath(m_path);

    return true;
}


bool SlDatabase::openDatabase(const std::wstring& path,
                              const std::wstring& username,
                              const std::wstring& password)
{
    if (!xf_get_file_exist(path))
    {
        // database does not exist
        return false;
    }


    std::string ascpath = kl::tostring(path);
    sqlite3* db = NULL;

    if (SQLITE_OK != sqlite3_open(ascpath.c_str(), &db))
    {
        // database could not be opened
        return false;
    }

    m_sqlite = db;
    m_path = path;

    m_connection_pool->setPath(m_path);

    return true;
}


void SlDatabase::close()
{
    if (m_sqlite)
    {
        sqlite3_close(m_sqlite);
    }

    m_connection_pool->closeAll();
    m_connection_pool->setPath(L"");
}



int SlDatabase::getDatabaseType()
{
    return xd::dbtypeSqlite;
}

std::wstring SlDatabase::getActiveUid()
{
    return L"";
}

xd::IAttributesPtr SlDatabase::getAttributes()
{
    DatabaseAttributes* attr = new DatabaseAttributes;

    std::wstring kws = sql92_keywords;

    attr->setIntAttribute(xd::dbattrColumnMaxNameLength, 64);
    attr->setIntAttribute(xd::dbattrTableMaxNameLength, 64);
    attr->setStringAttribute(xd::dbattrColumnInvalidChars, L"./\":!#&-`*[]");
    attr->setStringAttribute(xd::dbattrTableInvalidChars, L"./\":!#&-`*[]");
    attr->setStringAttribute(xd::dbattrColumnInvalidStartingChars, L" ");
    attr->setStringAttribute(xd::dbattrTableInvalidStartingChars, L" ");

    attr->setStringAttribute(xd::dbattrKeywords, kws);
    attr->setIntAttribute(xd::dbattrTableMaxNameLength, 32);
    
    return static_cast<xd::IAttributes*>(attr);
}

std::wstring SlDatabase::getErrorString()
{
    return L"";
}

int SlDatabase::getErrorCode()
{
    return xd::errorNone;
}

void SlDatabase::setError(int error_code, const std::wstring& error_string)
{
}


bool SlDatabase::cleanup()
{
    return true;
}

sqlite3* SlDatabase::getPoolConnection()
{
    return m_connection_pool->getConnection();
}

void SlDatabase::freePoolConnection(sqlite3* db)
{
    m_connection_pool->freeConnection(db);
}



xd::IJobPtr SlDatabase::createJob()
{
    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<IJobInternal*>(job);
}


bool SlDatabase::createFolder(const std::wstring& path)
{
    std::wstring objname = xdGetTablenameFromPath(path, true);
    
    std::wstring sql = L"CREATE TABLE ";
    sql += objname;
    sql += L" (xdt_folder text)";
    
    xcm::IObjectPtr resobj;
    return execute(sql, 0, resobj, NULL);
}

bool SlDatabase::createStream(const std::wstring& path,
                              const std::wstring& mime_type)
{
    std::string sql;
    std::string objname = (const char*)kl::toUtf8(xdGetTablenameFromPath(path, true));
    std::string info;
    sqlite3_stmt* stmt;

    info = kl::stdsprintf("block_size=%d,mime_type=%s", SlStream::DEFAULT_BLOCK_SIZE, (const char*)kl::toUtf8(mime_type));

    sql = "DROP TABLE IF EXISTS " + objname;
    sqlite3_exec(m_sqlite, sql.c_str(), NULL, NULL, NULL);

    sql = "CREATE TABLE " + objname;
    sql += " (xdt_stream text, block_id integer primary key autoincrement, data blob)";

    if (SQLITE_OK != sqlite3_exec(m_sqlite, sql.c_str(), NULL, NULL, NULL))
    {
        return false;
    }

    // insert header row
    sql = "INSERT INTO " + objname + " (xdt_stream, block_id, data) VALUES (?, 0, ZEROBLOB(0))";
    if (sqlite3_prepare_v2(m_sqlite, sql.c_str(), -1, &stmt, NULL))
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, info.c_str(), (int)info.size(), NULL);
    sqlite3_step(stmt);

    // insert first block with zero-length blob
    sql = "INSERT INTO " + objname + " (xdt_stream, block_id, data) VALUES ('', 1, ZEROBLOB(0))";
    if (sqlite3_prepare_v2(m_sqlite, sql.c_str(), -1, &stmt, NULL))
    {
        return false;
    }

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return true;
}

bool SlDatabase::renameFile(const std::wstring& path,
                            const std::wstring& new_name)
{
    std::wstring new_path = kl::beforeLast(path, L'/');
    new_path += L"/";
    new_path += new_name;
    return moveFile(path, new_path);
}

bool SlDatabase::moveFile(const std::wstring& path,
                          const std::wstring& dest_path)
{
    std::wstring src_objname = xdGetTablenameFromPath(path, true);
    std::wstring dest_objname = xdGetTablenameFromPath(dest_path, true);

    std::wstring command;
    command.reserve(1024);
    command = L"ALTER TABLE ";
    command += src_objname;
    command += L" RENAME TO ";
    command += dest_objname;

    xcm::IObjectPtr result;
    if (!execute(command, 0, result, NULL))
    {
        return false;
    }

    return true;
}

bool SlDatabase::copyFile(const std::wstring& src_path,
                          const std::wstring& dest_path)
{
    return false;
}

bool SlDatabase::copyData(const xd::CopyParams* info, xd::IJob* job)
{
    return false;
}

bool SlDatabase::deleteFile(const std::wstring& path)
{
    std::wstring objname = xdGetTablenameFromPath(path, true);

    std::wstring command;
    command.reserve(1024);
    command = L"DROP TABLE ";
    command += objname;

    xcm::IObjectPtr result;
    if (!execute(command, 0, result, NULL))
    {
        return false;
    }

    return true;
}

bool SlDatabase::getFileExist(const std::wstring& path)
{
    std::wstring query = L"SELECT *  from '";
    query += xdGetTablenameFromPath(path, false);
    query += L"' LIMIT 0";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_sqlite, kl::toUtf8(query), -1, &stmt, NULL))
    {
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}


xd::IFileInfoPtr SlDatabase::getFileInfo(const std::wstring& path)
{
    int rc = 0;
    
    char** result;
    int rows = 0;

    std::wstring query = L"SELECT tbl_name, sql FROM sqlite_master WHERE name='";
    query += xdGetTablenameFromPath(path, false);
    query += L"'";
    
    rc = sqlite3_get_table(m_sqlite, kl::toUtf8(query), &result, &rows, NULL, NULL);
    if (rc != SQLITE_OK || rows < 1)
        return xcm::null;

    std::wstring type = kl::fromUtf8(result[3]);
    
    sqlite3_free_table(result);
    
    
    xdcommon::FileInfo* f = new xdcommon::FileInfo;
    f->name = kl::afterLast(path, L'/');
    f->type = xd::filetypeTable;

    std::wstring str = m_path + L":" + path;
    kl::makeLower(str);
    f->object_id = kl::md5str(str);
    
    if (type.find(L"xdt_folder") != -1)
        f->type = xd::filetypeFolder;
    else if (type.find(L"xdt_stream") != -1)
    {
        f->type = xd::filetypeStream;
        f->mime_type = L"text/plain";
    }

    return static_cast<xd::IFileInfo*>(f);
}

xd::IFileInfoEnumPtr SlDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;

    int rc = 0;
    
    char** result;
    int rows = 0;
    
    std::wstring sql = L"SELECT tbl_name, sql FROM sqlite_master";
    
    if (path == L"" || path == L"/")
    {
        sql += L" WHERE name not like '%" XDSQLITE_PATH_SEPARATOR_LIKE "%' ESCAPE '\\'";
    }
     else
    {
        sql += L" WHERE name like '";
        sql += xdGetTablenameFromPath(path + XDSQLITE_PATH_SEPARATOR_LIKE, false);
        sql += L"%' ESCAPE '\\'";
    }

    rc = sqlite3_get_table(m_sqlite, kl::toUtf8(sql), &result, &rows, NULL, NULL);
    if (rc != SQLITE_OK)
        return retval;

    int i;
    for (i = 1; i <= rows; ++i)
    {
        const char* s_tbl_name = result[i*2];
        const char* s_sql = result[(i*2)+1];

        std::wstring name = kl::fromUtf8(s_tbl_name);
        size_t pos = name.rfind(XDSQLITE_PATH_SEPARATOR);
        if (pos != -1)
            name = name.substr(pos + XDSQLITE_PATH_SEPARATOR_LEN);

        xdcommon::FileInfo* f = new xdcommon::FileInfo;
        f->name = name;
        f->type = xd::filetypeTable;
        
        std::wstring sql = kl::fromUtf8(s_sql);
        if (sql.find(L"xdt_folder") != -1)
            f->type = xd::filetypeFolder;
        else if (sql.find(L"xdt_stream") != -1)
            f->type = xd::filetypeStream;

        retval->append(f);
    }

    sqlite3_free_table(result);

    return retval;
}

bool SlDatabase::createTable(const std::wstring& path, const xd::FormatDefinition& format_definition)
{
    // generate table name, SQL CREATE statment, and execute

    std::wstring sql;
    sql = L"CREATE TABLE ";
    sql += xdGetTablenameFromPath(path, true);
    sql += L" (";
    
    std::vector<xd::ColumnInfo>::const_iterator it;
    for (it = format_definition.columns.cbegin(); it != format_definition.columns.cend(); ++it)
    {
        const xd::ColumnInfo& colinfo = *it;
        
        std::wstring piece;
        piece += colinfo.name;
        piece += L" ";

        std::wstring type;
        switch (colinfo.type)
        {
            case xd::typeCharacter:
            case xd::typeWideCharacter:
                type = L"TEXT";
                break;
            case xd::typeInteger:
                type = L"INTEGER";
                break;
            case xd::typeDouble:
                type = L"REAL";
                break;
            case xd::typeNumeric:
                {
                    wchar_t buf[128];
                    swprintf(buf, 127, L"NUMERIC(%d,%d)", colinfo.width, colinfo.scale);
                    type = buf;
                }
                break;
            case xd::typeDate:
                type = L"DATE";
                break;
            case xd::typeDateTime:
                type = L"DATETIME";
                break;
            case xd::typeBoolean:
                type = L"BOOLEAN";
                break;
            case xd::typeBinary:
                type = L"BLOB";
                break;
        }

        piece += type;
        if (it+1 != format_definition.columns.cend())
            piece += L", ";

        sql += piece;
    }
    sql += L");";

    std::string ascsql = kl::tostring(sql);

    if (SQLITE_OK != sqlite3_exec(m_sqlite, ascsql.c_str(), NULL, NULL, NULL))
        return false;

    return true;
}


xd::IStreamPtr SlDatabase::openStream(const std::wstring& path)
{
    std::wstring object_name = xdGetTablenameFromPath(path, false);
    std::wstring escaped_object_name = xdGetTablenameFromPath(path, true);
    std::wstring sql = L"SELECT xdt_stream from " + escaped_object_name;
    sql += L" ORDER BY block_id LIMIT 1";

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(m_sqlite, kl::toUtf8(sql), -1, &stmt, NULL);
    if (!stmt)
    {
        // could not open stream
        return xcm::null;
    }

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        return xcm::null;
    }

    int block_size = SlStream::DEFAULT_BLOCK_SIZE;
    std::wstring mime_type = L"text/plain";

    const char* s_info = (const char*)sqlite3_column_text(stmt, 0);

    std::vector<std::wstring> vec;
    kl::parseDelimitedList(kl::towstring(s_info), vec, L',');
    for (auto& el : vec)
    {
        std::wstring key = kl::beforeFirst(el, '=');
        std::wstring val = kl::afterFirst(el, L'=');
        kl::trim(key);
        kl::trim(val);

        if (key == L"block_size")
        {
            block_size = kl::wtoi(val);
        }
        else if (key == L"mime_type")
        {
            mime_type = val;
        }
    }

    sqlite3_finalize(stmt);

    SlStream* stream = new SlStream(this);
    xd::IStreamPtr sp_stream = stream;
    stream->init(object_name, mime_type, block_size);

    return sp_stream;
}


xd::IIteratorPtr SlDatabase::query(const xd::QueryParams& qp)
{
    SlIterator* iter = new SlIterator(this);
    iter->ref();

    if (!iter->init(qp))
    {
        iter->unref();
        return xcm::null;
    }

    xd::IIteratorPtr result = static_cast<xd::IIterator*>(iter);
    iter->unref();

    return result;
}


xd::IndexInfo SlDatabase::createIndex(const std::wstring& path,
                                      const std::wstring& name,
                                      const std::wstring& expr,
                                      xd::IJob* job)
{
    return xd::IndexInfo();
}


bool SlDatabase::renameIndex(const std::wstring& path,
                             const std::wstring& name,
                             const std::wstring& new_name)
{
    return false;
}


bool SlDatabase::deleteIndex(const std::wstring& path,
                             const std::wstring& name)
{
    return false;
}

xd::IRowInserterPtr SlDatabase::bulkInsert(const std::wstring& path)
{
    SlRowInserter* inserter = new SlRowInserter(this, path);
    return static_cast<xd::IRowInserter*>(inserter);
}

xd::Structure SlDatabase::describeTable(const std::wstring& _path)
{
    std::wstring path = xdGetTablenameFromPath(_path, false);
 
    wchar_t buf[512];
    swprintf(buf, 512, L"SELECT sql FROM sqlite_master WHERE name='%ls'", path.c_str());
    
    char** result;
    int rows = 0;
    int rc;

    rc = sqlite3_get_table(m_sqlite, kl::toUtf8(buf), &result, &rows, NULL, NULL);
    if (rc != SQLITE_OK || rows < 1)
    {
        // return failure
        return xd::Structure();
    }
    
    std::wstring create_stmt = kl::fromUtf8(result[1]);
    sqlite3_free_table(result);
    
    return parseCreateStatement(create_stmt);
}


bool SlDatabase::modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job)
{
    return false;
}


xd::IndexInfoEnum SlDatabase::getIndexEnum(const std::wstring& path)
{
    return xd::IndexInfoEnum();
}


xd::IDatabasePtr SlDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool SlDatabase::setMountPoint(const std::wstring& path,
                               const std::wstring& connection_str,
                               const std::wstring& remote_path)
{
    return false;
}
                              
bool SlDatabase::getMountPoint(const std::wstring& path,
                               std::wstring& connection_str,
                               std::wstring& remote_path)
{
    return false;
}

bool SlDatabase::execute(const std::wstring& command,
                         unsigned int flags,
                         xcm::IObjectPtr& result,
                         xd::IJob* job)
{
    m_error.clearError();
    result.clear();
    
    // first find out if this statement has any select's in it
    static klregex::wregex select_regex(L"SELECT\\s");
    if (select_regex.search(command))
    {
        SlIterator* iter = new SlIterator(this);
        iter->m_sqlite = m_sqlite;

        iter->ref();

        if (!iter->init(command))
        {
            result.clear();
            iter->unref();
            return false;
        }

        result = static_cast<xd::IIterator*>(iter);
        iter->unref();

        return true;
    }
     else
    {
        if (SQLITE_OK != sqlite3_exec(m_sqlite, kl::toUtf8(command), NULL, NULL, NULL))
        {
            return false;
        }
    }
    
    return true;
}

bool SlDatabase::groupQuery(xd::GroupQueryParams* info, xd::IJob* job)
{
    return false;
}
