/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "libpq-fe.h"
#include "tango.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/indexinfo.h"
#include "database.h"
#include "iterator.h"
#include "set.h"
#include <set>
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>


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



// utility function to create a valid SQL field string

std::wstring pgsqlCreateFieldString(const std::wstring& name,
                                    int type,
                                    int width,
                                    int scale,
                                    bool allow_nulls,
                                    int db_type)
{
    wchar_t buf[255];

    if (width < 1)
        width = 1;

    switch (type)
    {
        case tango::typeWideCharacter:
        case tango::typeCharacter:
        {
            swprintf(buf, 255, L"%ls varchar (%d)%ls",
                                name.c_str(),
                                width,
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        /*
        case tango::typeWideCharacter:
        {
            swprintf(buf, 255, L"%ls nvarchar (%d)%ls",
                                name.c_str(),
                                width,
                                allow_nulls ? L" NULL" : L"");
            return buf;
        }
        */

        case tango::typeNumeric:
        {
            if (width > 28)
            {
                width = 28;
            }

            swprintf(buf, 255, L"%ls numeric (%d,%d)%ls",
                                name.c_str(),
                                width,
                                scale,
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case tango::typeInteger:
        {
            swprintf(buf, 255, L"%ls integer%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case tango::typeDouble:
        {
            swprintf(buf, 255, L"%ls double precision%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case tango::typeBoolean:
        {
            swprintf(buf, 255, L"%ls boolean%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case tango::typeDate:
        {
            swprintf(buf, 255, L"%ls date%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");

            return buf;
        }

        case tango::typeDateTime:
        {
            swprintf(buf, 255, L"%ls timestamp%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");

            return buf;
        }
    }

    return L"";
}


int pgsqlToTangoType(int pg_type)
{
    switch (pg_type)
    {
        case 16:  // boolean
            return tango::typeBoolean;

        case 17:  // bytea
        case 20:  // int8
        case 21:  // int2
        case 23:  // int4
        case 26:  // oid
            return tango::typeInteger;

        case 1700: // numeric
        case 790:  // money
            return tango::typeNumeric;

        case 700:  // float4
        case 701:  // float8
            return tango::typeDouble;

        case 1114: // timestamp
        case 1184: // timestamptz
            return tango::typeDateTime;

        case 1083: // time
        case 1266: // timetz
        case 1082: // date
            return tango::typeDate;

        default:
        case 18:   // char
        case 25:   // text
        case 1043: // varchar
            return tango::typeCharacter;

    }

    return tango::typeInvalid;
}

tango::IColumnInfoPtr pgsqlCreateColInfo(const std::wstring& col_name,
                                         int col_pg_type,
                                         int col_width,
                                         int col_scale,
                                         const std::wstring& col_expr,
                                         int datetime_sub)
{
    int col_tango_type = pgsqlToTangoType(col_pg_type);

    tango::IColumnInfoPtr col;
    col = static_cast<tango::IColumnInfo*>(new ColumnInfo);

    col->setName(col_name);
    col->setType(col_tango_type);
    col->setWidth(col_width);
    col->setScale(col_scale);
    col->setExpression(col_expr);
    col->setColumnOrdinal(0);

    if (col_expr.length() > 0)
        col->setCalculated(true);
        
    return col;
}



static std::wstring getTablenameFromPath(const std::wstring& path)
{
    std::wstring res;

    if (path.substr(0,1) == L"/")
        res = path.substr(1);
         else
        res = path;

    kl::replaceStr(res, L"\"", L"");

    return res;
}

// PgsqlFileInfo class implementation


class PgsqlFileInfo : public xdcommon::FileInfo
{
public:

    PgsqlFileInfo(PgsqlDatabase* db) : xdcommon::FileInfo()
    {
        m_db = db;
        m_db->ref();
    }
    
    ~PgsqlFileInfo()
    {
        m_db->unref();
    }
    
    const std::wstring& getPrimaryKey()
    {
        primary_key = m_db->getPrimaryKey(name);
        return primary_key;
    }
    
private:

    PgsqlDatabase* m_db;
};




// PgsqlDatabase class implementation

PgsqlDatabase::PgsqlDatabase()
{
    m_db_name = L"";
    m_conn_str = L"";

    m_using_dsn = false;
    m_db_type = -1;
    m_port = 0;
    m_server = L"";
    m_database = L"";
    m_username = L"";
    m_password = L"";
    
    m_attr = static_cast<tango::IAttributes*>(new DatabaseAttributes);

    m_attr->setIntAttribute(tango::dbattrColumnMaxNameLength, 63);
    m_attr->setIntAttribute(tango::dbattrTableMaxNameLength, 63);
    m_attr->setStringAttribute(tango::dbattrKeywords, sql92_keywords);    
    m_attr->setStringAttribute(tango::dbattrColumnInvalidChars, 
                               L"\\./\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrTableInvalidChars, 
                               L"\\./:*?<>|\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrColumnInvalidStartingChars,
                               L"\\./\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrTableInvalidStartingChars,
                               L"\\./:*?<>|\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteOpenChar, L"\"");
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteCloseChar, L"\"");
    m_attr->setStringAttribute(tango::dbattrIdentifierCharsNeedingQuote, L"`\"~# $!@%^&(){}-+.");   
}

PgsqlDatabase::~PgsqlDatabase()
{
    close();
}



std::wstring PgsqlDatabase::getServer()
{
    return m_server;
}

std::wstring PgsqlDatabase::getPath()
{
    return m_path;
}


std::wstring PgsqlDatabase::getTempFileDirectory()
{
    std::wstring result = m_attr->getStringAttribute(tango::dbattrTempDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}

std::wstring PgsqlDatabase::getDefinitionDirectory()
{
    std::wstring result = m_attr->getStringAttribute(tango::dbattrDefinitionDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}


PGconn* PgsqlDatabase::createConnection()
{
    std::wstring connstr;
    connstr += L"hostaddr='" + m_server + L"'";

    int port = m_port; if (port == 0) port = 5432;
    connstr += L" port='" + kl::itowstring(port) + L"'";

    connstr += L" dbname='" + m_database + L"'";
    connstr += L" user='" + m_username + L"'";
    connstr += L" password='" + m_password + L"'";

    PGconn* conn = PQconnectdb(kl::toUtf8(connstr));
    if (!conn)
        return NULL;

    if (PQstatus(conn) != CONNECTION_OK)
    {
        PQfinish(conn);
        return NULL;
    }


    return conn;
}

void PgsqlDatabase::closeConnection(PGconn* conn)
{
    PQfinish(conn);
}



bool PgsqlDatabase::open(const std::wstring& server,
                         int port,
                         const std::wstring& database,
                         const std::wstring& username,
                         const std::wstring& password)
{

    m_error.clearError();
    

    m_server = server;
    m_port = port;
    m_database = database;
    m_username = username;
    m_password = password;

    PGconn* conn = createConnection();
    if (!conn)
    {
        return false;
    }


    closeConnection(conn);


    return true;
}


void PgsqlDatabase::close()
{
    m_db_name = L"";
    m_conn_str = L"";

    m_db_type = -1;
    m_port = 0;
    m_server = L"";
    m_database = L"";
    m_username = L"";
    m_password = L"";
}




void PgsqlDatabase::setDatabaseName(const std::wstring& name)
{
    m_db_name = name;
}

std::wstring PgsqlDatabase::getDatabaseName()
{
    return m_db_name;
}

int PgsqlDatabase::getDatabaseType()
{
    return m_db_type;
}

std::wstring PgsqlDatabase::getActiveUid()
{
    return L"";
}

tango::IAttributesPtr PgsqlDatabase::getAttributes()
{
    return m_attr;
}

double PgsqlDatabase::getFreeSpace()
{
    return 0.0;
}

double PgsqlDatabase::getUsedSpace()
{
    return 0.0;
}


std::wstring PgsqlDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int PgsqlDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void PgsqlDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code, error_string);
}


bool PgsqlDatabase::cleanup()
{
    return true;
}


bool PgsqlDatabase::storeObject(xcm::IObject* obj,
                               const std::wstring& ofs_path)
{
    return false;
}

tango::IJobPtr PgsqlDatabase::createJob()
{
    return xcm::null;
}

tango::IJobPtr PgsqlDatabase::getJob(tango::jobid_t job_id)
{
    return xcm::null;
}

tango::IDatabasePtr PgsqlDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool PgsqlDatabase::setMountPoint(const std::wstring& path,
                                 const std::wstring& connection_str,
                                 const std::wstring& remote_path)
{
    return false;
}
                              
bool PgsqlDatabase::getMountPoint(const std::wstring& path,
                                 std::wstring& connection_str,
                                 std::wstring& remote_path)
{
    return false;
}

bool PgsqlDatabase::createFolder(const std::wstring& path)
{
    return false;
}

tango::INodeValuePtr PgsqlDatabase::createNodeFile(const std::wstring& path)
{
    return xcm::null;
}

tango::INodeValuePtr PgsqlDatabase::openNodeFile(const std::wstring& path)
{
    return xcm::null;
}

bool PgsqlDatabase::renameFile(const std::wstring& path,
                              const std::wstring& new_name)
{
    std::wstring command;

    std::wstring quote_openchar = m_attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = m_attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);


    command = L"ALTER TABLE ";
    command += quote_openchar;
    command += getTablenameFromPath(path);
    command += quote_closechar;
    command += L" RENAME TO ";
    command += quote_openchar;
    command += getTablenameFromPath(new_name);
    command += quote_closechar;

    if (command.length() > 0)
    {
        xcm::IObjectPtr result_obj;
        return execute(command, 0, result_obj, NULL);
    }
    
    return false;
}

bool PgsqlDatabase::moveFile(const std::wstring& path,
                            const std::wstring& new_location)
{
    return false;
}

bool PgsqlDatabase::copyFile(const std::wstring& src_path,
                            const std::wstring& dest_path)
{
    return false;
}

bool PgsqlDatabase::deleteFile(const std::wstring& path)
{
    std::wstring quote_openchar = m_attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = m_attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);    
    
    std::wstring command;
    command.reserve(1024);
    command = L"DROP TABLE ";
    command += quote_openchar;
    command += getTablenameFromPath(path);
    command += quote_closechar;

    xcm::IObjectPtr result_obj;
    execute(command, 0, result_obj, NULL);

    return true;
}

bool PgsqlDatabase::getFileExist(const std::wstring& _path)
{
    std::wstring path = kl::afterFirst(_path, L'/');

    // there may be a faster way to do this.  In order to
    // determine if the file exists, we are going to get
    // a list of all tables and look for 'path'

    tango::IFileInfoEnumPtr files = getFolderInfo(L"");
    if (!files)
        return false;

    int count = files->size();
    int i;

    for (i = 0 ; i < count; ++i)
    {
        tango::IFileInfoPtr info = files->getItem(i);
        if (wcscasecmp(info->getName().c_str(), path.c_str()) == 0)
        {
            return true;
        }
    }

    return false;
}

tango::IFileInfoPtr PgsqlDatabase::getFileInfo(const std::wstring& path)
{
    std::wstring folder;
    std::wstring name;
    
    if (path.empty() || path == L"/")
    {
        PgsqlFileInfo* f = new PgsqlFileInfo(this);
        f->name = L"/";
        f->type = tango::filetypeFolder;
        f->format = tango::formatNative;
        return static_cast<tango::IFileInfo*>(f);
    }
     else
    {
        folder = kl::beforeLast(path, L'/');
        name = kl::afterLast(path, L'/');
    }
    
    tango::IFileInfoEnumPtr files = getFolderInfo(folder);
    int i, count;
    
    count = files->size();
    for (i = 0; i < count; ++i)
    {
        tango::IFileInfoPtr finfo = files->getItem(i);
        if (0 == wcscasecmp(finfo->getName().c_str(), name.c_str()))
        {
            return finfo;
        }
    }
    
    return xcm::null;
}

tango::IFileInfoEnumPtr PgsqlDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;

    PGconn* conn = createConnection();
    if (!conn)
        return retval;

    PGresult* res = PQexec(conn, "select tablename as name from pg_tables where schemaname <> 'pg_catalog' and schemaname <> 'information_schema' UNION "
                                 "select viewname as name from pg_views where schemaname <> 'pg_catalog' and schemaname <> 'information_schema'");

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
        return retval;

    int i, rows = PQntuples(res);
    for (i = 0; i < rows; ++i)
    {
        PgsqlFileInfo* f = new PgsqlFileInfo(this);
        f->name = kl::towstring(PQgetvalue(res, i, 0));
        f->type = tango::filetypeSet;
        f->format = tango::formatNative;

        retval->append(f);
    }
    
    PQclear(res);

    closeConnection(conn);

    return retval;
}

std::wstring PgsqlDatabase::getPrimaryKey(const std::wstring path)
{
    std::wstring table = getTablenameFromPath(path);
    std::wstring pk;

    PGconn* conn = createConnection();
    if (!conn)
        return pk;

    std::wstring query = L"SELECT pg_attribute.attname, "
                         L"       format_type(pg_attribute.atttypid, pg_attribute.atttypmod) "
                         L"FROM   pg_index, pg_class, pg_attribute "
                         L"WHERE  pg_class.oid = '%tbl%'::regclass AND "
                         L"       indrelid = pg_class.oid AND "
                         L"       pg_attribute.attrelid = pg_class.oid AND "
                         L"       pg_attribute.attnum = any(pg_index.indkey) AND "
                         L"       indisprimary";

    kl::replaceStr(query, L"%tbl%", table);

    PGresult* res = PQexec(conn, kl::toUtf8(query));

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        PQclear(res);
        closeConnection(conn);
        return xcm::null;
    }

    int i, rows = PQntuples(res);
    for (i = 0; i < rows; ++i)
    {
        if (pk.length() > 0)
            pk += L",";
        pk += kl::towstring(PQgetvalue(res, i, 0));
    }


    closeConnection(conn);

    return pk;
}

tango::IStructurePtr PgsqlDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<tango::IStructure*>(s);
}

tango::ISetPtr PgsqlDatabase::createTable(const std::wstring& path,
                                          tango::IStructurePtr struct_config,
                                          tango::FormatInfo* format_info)
{
    std::wstring quote_openchar = m_attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = m_attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    std::wstring command;
    command.reserve(1024);

    command = L"CREATE TABLE ";
    command += quote_openchar;
    command += getTablenameFromPath(path);
    command += quote_closechar;
    command += L" (";

    std::wstring field;
    field.reserve(255);

    std::wstring name;
    int type;
    int width;
    int scale;
    
    int col_count = struct_config->getColumnCount();
    int i;
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr col_info;
        col_info = struct_config->getColumnInfoByIdx(i);

        // quote the fieldname
        name = L"";
        name += quote_openchar;
        name += col_info->getName();
        name += quote_closechar;

        type = col_info->getType();
        width = col_info->getWidth();
        scale = col_info->getScale();

        field = pgsqlCreateFieldString(name,
                                  type,
                                  width,
                                  scale,
                                  true,
                                  m_db_type);

        command += field;

        if (i+1 != col_count)
        {
            command += L", ";
        }
    }
    command += L")";

    xcm::IObjectPtr result_obj;
    if (!execute(command, 0, result_obj, NULL))
    {
        // failed
        return xcm::null;
    }

    return openSet(path);
}

tango::IStreamPtr PgsqlDatabase::openStream(const std::wstring& ofs_path)
{
    return xcm::null;
}

tango::IStreamPtr PgsqlDatabase::createStream(const std::wstring& ofs_path, const std::wstring& mime_type)
{
    return xcm::null;
}

tango::ISetPtr PgsqlDatabase::openSetEx(const std::wstring& ofs_path,
                                       int format)
{
    return openSet(ofs_path);
}

tango::ISetPtr PgsqlDatabase::openSet(const std::wstring& path)
{
    std::wstring tablename1 = getTablenameFromPath(path);

    if (tablename1.empty())
        return xcm::null;
        
    if (m_db_type == tango::dbtypeDb2 ||
        m_db_type == tango::dbtypeOracle)
    {
        kl::makeUpper(tablename1);
    }

    // create set and initialize variables
    PgsqlSet* set = new PgsqlSet(this);
    set->m_conn_str = m_conn_str;
    set->m_tablename = tablename1;

    // initialize Odbc connection for this set
    if (!set->init())
    {
        return xcm::null;
    }

    return static_cast<tango::ISet*>(set);
}


tango::IIteratorPtr PgsqlDatabase::createIterator(const std::wstring& path,
                                                  const std::wstring& columns,
                                                  const std::wstring& sort,
                                                  tango::IJob* job)
{
    tango::ISetPtr set = openSet(path);
    if (set.isNull())
        return xcm::null;
    return set->createIterator(columns, sort, job);
}


tango::IRelationEnumPtr PgsqlDatabase::getRelationEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

tango::IRelationPtr PgsqlDatabase::getRelation(const std::wstring& relation_id)
{
    return xcm::null;
}

tango::IRelationPtr PgsqlDatabase::createRelation(const std::wstring& tag,
                                                 const std::wstring& left_set_path,
                                                 const std::wstring& right_set_path,
                                                 const std::wstring& left_expr,
                                                 const std::wstring& right_expr)
{
    return xcm::null;
}

bool PgsqlDatabase::deleteRelation(const std::wstring& relation_id)
{
    return false;
}




tango::IIndexInfoPtr PgsqlDatabase::createIndex(const std::wstring& path,
                                               const std::wstring& name,
                                               const std::wstring& expr,
                                               tango::IJob* job)
{
    return xcm::null;
}


bool PgsqlDatabase::renameIndex(const std::wstring& path,
                               const std::wstring& name,
                               const std::wstring& new_name)
{
    return false;
}


bool PgsqlDatabase::deleteIndex(const std::wstring& path,
                               const std::wstring& name)
{
    return false;
}


tango::IIndexInfoEnumPtr PgsqlDatabase::getIndexEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IIndexInfoEnumPtr>* vec;
    vec = new xcm::IVectorImpl<tango::IIndexInfoEnumPtr>;


    std::wstring table = getTablenameFromPath(path);

    PGconn* conn = createConnection();
    if (!conn)
        return vec;

    std::wstring query = L"SELECT t.relname as table_name, "
                         L"       i.relname as index_name, "
                         L"       a.attname as column_name "
                         L"FROM   pg_class t, pg_class i, pg_index ix, pg_attribute a "
                         L"WHERE "
                         L"     t.oid = ix.indrelid "
                         L"     and i.oid = ix.indexrelid "
                         L"     and a.attrelid = t.oid "
                         L"     and a.attnum = ANY(ix.indkey) "
                         L"     and t.relkind = 'r' "
                         L"     and t.relname = '%tbl%' "
                         L"ORDER BY "
                         L"     t.relname, "
                         L"     i.relname ";

    kl::replaceStr(query, L"%tbl%", table);

    PGresult* res = PQexec(conn, kl::toUtf8(query));

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        PQclear(res);
        closeConnection(conn);
        return vec;
    }

    std::map<std::wstring, std::wstring> indexes;

    int i, rows = PQntuples(res);
    for (i = 0; i < rows; ++i)
    {
        std::wstring index_name = kl::towstring(PQgetvalue(res, i, 1));
        std::wstring index_column = kl::towstring(PQgetvalue(res, i, 2));

        if (indexes[index_name].length() > 0)
            indexes[index_name] += L",";
        indexes[index_name] += index_column;
    }


    closeConnection(conn);



    std::map<std::wstring, std::wstring>::iterator it;
    for (it = indexes.begin(); it != indexes.end(); ++it)
    {
        IndexInfo* ii = new IndexInfo;
        ii->setTag();
        ii->setExpression(it->expr);

        indexes->append(static_cast<tango::IIndexInfo*>(ii));
    }


    return vec;
}



tango::IStructurePtr PgsqlDatabase::describeTable(const std::wstring& path)
{
    std::wstring tbl = getTablenameFromPath(path);

    std::wstring query;
    query += L"select attname,atttypid,atttypmod from pg_attribute where ";
    query += L"attrelid = (select oid from pg_class where relname='"+tbl+L"') and ";
    query += L"attnum >= 1 order by attnum";

    PGconn* conn = createConnection();
    if (!conn)
        return xcm::null;

    PGresult* res = PQexec(conn, kl::toUtf8(query));

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        PQclear(res);
        return xcm::null;
    }

    // create new tango::IStructure
    Structure* s = new Structure;

    std::wstring colname;
    int pg_type;
    int tango_type;
    int type_mod;
    int col_width;
    int col_scale;

    int i, rows = PQntuples(res);
    for (i = 0; i < rows; ++i)
    {
        colname = kl::towstring(PQgetvalue(res, i, 0));
        pg_type = atoi(PQgetvalue(res, i, 1));
        type_mod = atoi(PQgetvalue(res, i, 2));
        tango_type = pgsqlToTangoType(pg_type);
        
        if (tango_type == tango::typeNumeric || tango_type == tango::typeDouble)
        {
            type_mod -= 4;
            col_width = (type_mod >> 16);
            col_scale = (type_mod & 0xffff);
        }
         else if (tango_type == tango::typeCharacter || tango_type == tango::typeWideCharacter)
        {
            col_width = type_mod - 4;
            col_scale = 0;
        }
         else if (tango_type == tango::typeDateTime)
        {
            col_width = 8;
            col_scale = 0;
        }
         else if (tango_type == tango::typeInteger || tango_type == tango::typeDate)
        {
            col_width = 4;
            col_scale = 0;
        }
         else
        {
            col_width = 8;
            col_scale = 0;
        }

        tango::IColumnInfoPtr colinfo = pgsqlCreateColInfo(colname,
                                                           pg_type, 
                                                           col_width,
                                                           col_scale,
                                                           L"",
                                                           0);
        s->addColumn(colinfo);
    }
    
    PQclear(res);

    closeConnection(conn);

    return static_cast<tango::IStructure*>(s);
}



bool PgsqlDatabase::execute(const std::wstring& command,
                            unsigned int flags,
                            xcm::IObjectPtr& result,
                            tango::IJob* job)
{
    m_error.clearError();
    result.clear();
    
    wchar_t* p = (wchar_t*)command.c_str();
    while (::iswspace(*p)) ++p;

    std::wstring first_word;
    while (*p && !iswspace(*p))
    {
        first_word += *p;
        ++p;
    }

    if (0 == wcscasecmp(first_word.c_str(), L"SELECT"))
    {
        // create an iterator based on our select statement
        PgsqlIterator* iter = new PgsqlIterator(this);

        if (!iter->init(command))
        {
            delete iter;
            return false;
        }

        result = static_cast<xcm::IObject*>(static_cast<tango::IIterator*>(iter));
        return true;
    }
     else
    {
        PGconn* conn = createConnection();
        if (!conn)
            return false;

        PGresult* res = PQexec(conn, kl::toUtf8(command));
        if (!res)
            return false;

        int result_status = PQresultStatus(res);

        PQclear(res);
        closeConnection(conn);
        
        return (result_status == PGRES_COMMAND_OK) ? true : false;
    }
}

tango::ISetPtr PgsqlDatabase::runGroupQuery(tango::ISetPtr set,
                                           const std::wstring& group,
                                           const std::wstring& output,
                                           const std::wstring& where,
                                           const std::wstring& having,
                                           tango::IJob* job)
{
    return xcm::null;
}

