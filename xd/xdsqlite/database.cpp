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
#pragma warning(disable : 4786)
#define _CRT_SECURE_NO_WARNINGS 1
#endif


#include "sqlite3.h"


#include <kl/portable.h>
#include <kl/utf8.h>
#include <kl/file.h>
#include <kl/regex.h>
#include <kl/md5.h>


#include <xd/xd.h>
#include "database.h"
#include "inserter.h"
#include "iterator.h"
#include "util.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/util.h"


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




std::wstring sqliteGetTablenameFromPath(const std::wstring& path)
{
    const wchar_t* p = path.c_str();
    while (iswspace(*p))
        p++;
    if (*p == '/')
        p++;
    
    std::wstring result;
    result.reserve(path.length());
    while (*p)
    {
        if (*p == '/')
            result += L"$$$";
             else
            result += *p;
        p++;
    }
    
    return result;
}



SlDatabase::SlDatabase()
{
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


    // create catalog table

    const char* sql1 = "CREATE TABLE catalog ("
                           "id INTEGER PRIMARY KEY, "
                           "folder_id INTEGER, "
                           "name TEXT, "
                           "type TEXT, "
                           "obj_name TEXT);";

    if (SQLITE_OK != sqlite3_exec(db, sql1, NULL, NULL, NULL))
    {
        sqlite3_close(db);
        xf_remove(path);
        return false;
    }

    // create columns table

    const char* sql2 = "CREATE TABLE columns ("
                           "obj_id INTEGER, "
                           "name TEXT, "
                           "type INTEGER, "
                           "width INTEGER, "
                           "scale INTEGER, "
                           "expr TEXT);";

    if (SQLITE_OK != sqlite3_exec(db, sql2, NULL, NULL, NULL))
    {
        sqlite3_close(db);
        xf_remove(path);
        return false;
    }


    // insert entry for root folder

    const char* sql3 = "INSERT INTO catalog "
                          "(id, folder_id, name, type, obj_name) "
                          "VALUES (1, 0, '/', 'FOLDER', '');";

    if (SQLITE_OK != sqlite3_exec(db, sql3, NULL, NULL, NULL))
    {
        sqlite3_close(db);
        xf_remove(path);
        return false;
    }


    m_sqlite = db;
    m_path = path;

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

    return true;
}


void SlDatabase::close()
{
    if (m_sqlite)
    {
        sqlite3_close(m_sqlite);
    }
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
    std::wstring objname = sqliteGetTablenameFromPath(path);
    
    std::wstring sql = L"CREATE TABLE ";
    sql += objname;
    sql += L" (xdsqlite_folder text)";
    
    xcm::IObjectPtr resobj;
    return execute(sql, 0, resobj, NULL);
}

bool SlDatabase::createStream(const std::wstring& path,
                              const std::wstring& mime_type)
{
    return false;
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
    std::wstring src_objname = sqliteGetTablenameFromPath(path);
    std::wstring dest_objname = sqliteGetTablenameFromPath(dest_path);

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
    std::wstring objname = sqliteGetTablenameFromPath(path);

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
    xd::IFileInfoEnumPtr files = getFolderInfo(L"");
    if (!files)
        return false;

    size_t i, count = files->size();

    for (i = 0 ; i < count; ++i)
    {
        xd::IFileInfoPtr info = files->getItem(i);
        
        if (wcscasecmp(info->getName().c_str(), path.c_str()) == 0)
            return true;
    }

    return false;
}


xd::IFileInfoPtr SlDatabase::getFileInfo(const std::wstring& path)
{
    std::wstring objname = kl::afterFirst(path, L'/');

    int rc = 0;
    
    char** result;
    int rows = 0;
    int cat_id = 0;

    wchar_t query[512];
    swprintf(query, 512, L"SELECT tbl_name, sql FROM sqlite_master WHERE name='%ls'", objname.c_str());
    
    rc = sqlite3_get_table(m_sqlite, kl::toUtf8(query), &result, &rows, NULL, NULL);
    if (rc != SQLITE_OK || rows < 1)
        return xcm::null;

    std::wstring type = kl::fromUtf8(result[3]);
    
    sqlite3_free_table(result);
    
    
    xdcommon::FileInfo* f = new xdcommon::FileInfo;
    f->name = objname;
    f->type = xd::filetypeTable;

    std::wstring str = m_path + L":" + path;
    kl::makeLower(str);
    f->object_id = kl::md5str(str);
    
    if (type.find(L"xdsqlite_folder") != -1)
        f->type = xd::filetypeFolder;

    return static_cast<xd::IFileInfo*>(f);
}

xd::IFileInfoEnumPtr SlDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;

    int rc = 0;
    
    char** result;
    int rows = 0;
    int cat_id = 0;

    
    std::wstring sql = L"SELECT tbl_name, sql FROM sqlite_master";
    
    if (path == L"" || path == L"/")
    {
        sql += L" WHERE name not like '%$$$%'";
    }
     else
    {
        sql += L" WHERE name like '";
        sql += sqliteGetTablenameFromPath(path);
        sql += L"$$$%'";
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
        size_t pos = name.find_last_of(L"$$$");
        if (pos != -1)
            name = name.substr(pos+1);

        xdcommon::FileInfo* f = new xdcommon::FileInfo;
        f->name = name;
        f->type = xd::filetypeTable;
        
        std::wstring sql = kl::fromUtf8(s_sql);
        if (sql.find(L"xdsqlite_folder") != -1)
            f->type = xd::filetypeFolder;
            
        retval->append(f);
    }

    sqlite3_free_table(result);

    return retval;
}

xd::IStructurePtr SlDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<xd::IStructure*>(s);
}


bool SlDatabase::createTable(const std::wstring& path, const xd::FormatDefinition& format_definition)
{
    // generate table name, SQL CREATE statment, and execute

    std::wstring sql;
    sql = L"CREATE TABLE ";
    sql += sqliteGetTablenameFromPath(path);
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
    return xcm::null;
}


xd::IIteratorPtr SlDatabase::query(const xd::QueryParams& qp)
{
    std::wstring columns = qp.columns;
    if (columns.length() == 0)
        columns = L"*";

    std::wstring sql = L"SELECT %columns% FROM %table%";
    kl::replaceStr(sql, L"%columns%", columns);
    kl::replaceStr(sql, L"%table%", sqliteGetTablenameFromPath(qp.from));

    if (qp.where.length() > 0)
        sql += L" WHERE " + qp.where;

    if (qp.order.length() > 0)
        sql += L" ORDER BY " + qp.order;


    SlIterator* iter = new SlIterator(this);
    iter->m_tablename = qp.from;
    iter->ref();

    if (!iter->init(sql))
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

xd::Structure SlDatabase::describeTable(const std::wstring& path)
{
    return xd::Structure();
}

xd::IStructurePtr SlDatabase::describeTableI(const std::wstring& _path)
{
    std::wstring path = sqliteGetTablenameFromPath(_path);

    int rc = 0;
    
    wchar_t buf[512];
    swprintf(buf, 512, L"SELECT sql FROM sqlite_master WHERE name='%ls'", path.c_str());
    
    
    char** result;
    int rows = 0;
    int cat_id = 0;

    rc = sqlite3_get_table(m_sqlite, kl::toUtf8(buf), &result, &rows, NULL, NULL);
    if (rc != SQLITE_OK || rows < 1)
    {
        // return failure
        return xcm::null;
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
