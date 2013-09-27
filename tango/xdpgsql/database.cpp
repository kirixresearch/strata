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
#include "libpq/libpq-fs.h"
#include <xd/xd.h>
#include "../xdcommon/dbattr.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/indexinfo.h"
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/util.h"
#include "database.h"
#include "iterator.h"
#include "inserter.h"
#include "stream.h"
#include <set>
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/md5.h>



#define FOLDER_SEPARATOR L"__"


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
        case xd::typeWideCharacter:
        case xd::typeCharacter:
        {
            swprintf(buf, 255, L"%ls varchar (%d)%ls",
                                name.c_str(),
                                width,
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        /*
        case xd::typeWideCharacter:
        {
            swprintf(buf, 255, L"%ls nvarchar (%d)%ls",
                                name.c_str(),
                                width,
                                allow_nulls ? L" NULL" : L"");
            return buf;
        }
        */

        case xd::typeNumeric:
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

        case xd::typeInteger:
        {
            swprintf(buf, 255, L"%ls integer%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case xd::typeDouble:
        {
            swprintf(buf, 255, L"%ls double precision%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case xd::typeBoolean:
        {
            swprintf(buf, 255, L"%ls boolean%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case xd::typeDate:
        {
            swprintf(buf, 255, L"%ls date%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");

            return buf;
        }

        case xd::typeDateTime:
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
            return xd::typeBoolean;

        case 17:  // bytea
        case 20:  // int8
        case 21:  // int2
        case 23:  // int4
        case 26:  // oid
            return xd::typeInteger;

        case 1700: // numeric
        case 790:  // money
            return xd::typeNumeric;

        case 700:  // float4
        case 701:  // float8
            return xd::typeDouble;

        case 1114: // timestamp
        case 1184: // timestamptz
            return xd::typeDateTime;

        case 1083: // time
        case 1266: // timetz
        case 1082: // date
            return xd::typeDate;

        default:
        case 18:   // char
        case 25:   // text
        case 1043: // varchar
            return xd::typeCharacter;

    }

    return xd::typeInvalid;
}

xd::IColumnInfoPtr pgsqlCreateColInfo(const std::wstring& col_name,
                                         int col_pg_type,
                                         int col_width,
                                         int col_scale,
                                         const std::wstring& col_expr,
                                         int datetime_sub)
{
    int col_tango_type = pgsqlToTangoType(col_pg_type);

    xd::IColumnInfoPtr col;
    col = static_cast<xd::IColumnInfo*>(new ColumnInfo);

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



std::wstring pgsqlGetTablenameFromPath(const std::wstring& path)
{
    std::wstring res;

    if (path.substr(0,1) == L"/")
        res = path.substr(1);
         else
        res = path;

    kl::trim(res);

    if (res.empty())
        return L"";

    if (res[res.length()-1] == '/')
        res = res.substr(0, res.length()-1);

    kl::replaceStr(res, L"/", FOLDER_SEPARATOR);
    kl::replaceStr(res, L"\"", L"");
    kl::makeLower(res);

    return res;
}

std::wstring pgsqlQuoteIdentifier(const std::wstring& str)
{
    std::wstring res;

    res = str;
    kl::replaceStr(res, L"\"", L"");
    kl::replaceStr(res, L"/*", L"");
    kl::replaceStr(res, L"-", L"");
    kl::makeLower(res);

    return L"\"" + res + L"\"";
}

std::wstring pgsqlQuoteIdentifierIfNecessary(const std::wstring& str)
{
    std::wstring res;

    res = str;
    kl::replaceStr(res, L"\"", L"");
    kl::replaceStr(res, L"/*", L"");
    kl::replaceStr(res, L"-", L"");
    kl::makeLower(res);

    if (res.find(' ') == str.npos)
        return res;


    return L"\"" + res + L"\"";
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
    
    const std::wstring& getObjectId()
    {
        if (!object_id.empty())
            return object_id;
        std::wstring hashsrc;
        hashsrc += m_db->getServer() + L";";
        hashsrc += m_db->getActiveUid() + L";";
        hashsrc += name;
        kl::makeLower(hashsrc);
        object_id = kl::md5str(hashsrc);
        return object_id;
    }

private:

    PgsqlDatabase* m_db;
    
};




xcm_interface IPgsqlJobInfo : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdpgsql.IPgsqlJobInfo")

public:

    virtual void setConnection(PGconn* conn) = 0;
};

XCM_DECLARE_SMARTPTR(IPgsqlJobInfo)


class PgsqlJobInfo : public JobInfo, public IPgsqlJobInfo
{
public:

    XCM_CLASS_NAME("xdpgsql.PgsqlJobInfo")
    XCM_BEGIN_INTERFACE_MAP(PgsqlJobInfo)
        XCM_INTERFACE_ENTRY(xd::IJob)
        XCM_INTERFACE_ENTRY(IJobInternal)
        XCM_INTERFACE_ENTRY(IPgsqlJobInfo)
    XCM_END_INTERFACE_MAP()

    PgsqlJobInfo() : JobInfo()
    {
        m_conn = NULL;
    }

    void setConnection(PGconn* conn)
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        m_conn = conn;
    }

    bool cancel()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        if (!m_conn)
            return JobInfo::cancel();
        
        if (!getCanCancel())
            return false;

        char err[256];
        PGcancel* c = PQgetCancel(m_conn);
        PQcancel(c, err, 256);
        PQfreeCancel(c);

        return JobInfo::cancel();
    }

private:

    PGconn* m_conn;
};



// PgsqlDatabase class implementation

PgsqlDatabase::PgsqlDatabase()
{
    m_port = 0;
    m_server = L"";
    m_database = L"";
    m_username = L"";
    m_password = L"";
    
    m_attr = static_cast<xd::IAttributes*>(new DatabaseAttributes);

    m_attr->setIntAttribute(xd::dbattrColumnMaxNameLength, 63);
    m_attr->setIntAttribute(xd::dbattrTableMaxNameLength, 63);
    m_attr->setStringAttribute(xd::dbattrKeywords, sql92_keywords);    
    m_attr->setStringAttribute(xd::dbattrColumnInvalidChars, 
                               L"\\./\x00\xFF");
    m_attr->setStringAttribute(xd::dbattrTableInvalidChars, 
                               L"\\./:*?<>|\x00\xFF");
    m_attr->setStringAttribute(xd::dbattrColumnInvalidStartingChars,
                               L"\\./\x00\xFF");
    m_attr->setStringAttribute(xd::dbattrTableInvalidStartingChars,
                               L"\\./:*?<>|\x00\xFF");
    m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, L"\"");
    m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, L"\"");
    m_attr->setStringAttribute(xd::dbattrIdentifierCharsNeedingQuote, L"`\"~# $!@%^&(){}-+.");   
}

PgsqlDatabase::~PgsqlDatabase()
{
    close();

    m_jobs_mutex.lock();
    std::vector<PgsqlJobInfo*>::iterator it;
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
        (*it)->unref();
    m_jobs_mutex.unlock();
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
    std::wstring result = m_attr->getStringAttribute(xd::dbattrTempDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}

std::wstring PgsqlDatabase::getDefinitionDirectory()
{
    std::wstring result = m_attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}


PGconn* PgsqlDatabase::createConnection()
{
    m_error.clearError();

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
        m_error.setError(xd::errorGeneral, kl::towstring(PQerrorMessage(conn)));

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
    m_port = 0;
    m_server = L"";
    m_database = L"";
    m_username = L"";
    m_password = L"";
}


int PgsqlDatabase::getDatabaseType()
{
    return xd::dbtypePostgres;
}

std::wstring PgsqlDatabase::getActiveUid()
{
    return m_username;
}

xd::IAttributesPtr PgsqlDatabase::getAttributes()
{
    return m_attr;
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

xd::IJobPtr PgsqlDatabase::createJob()
{
    XCM_AUTO_LOCK(m_jobs_mutex);

    PgsqlJobInfo* job = new PgsqlJobInfo;
    job->setJobId(++m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<xd::IJob*>(job);
}


xd::IDatabasePtr PgsqlDatabase::getMountDatabase(const std::wstring& path)
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
    deleteFile(path);


    PGconn* conn = createConnection();
    PGresult* res;
    if (!conn)
        return false;


    PQexec(conn, "BEGIN");

    std::wstring sql, tbl;

    tbl = pgsqlGetTablenameFromPath(path);
    sql = L"CREATE TABLE %tbl% (xdpgsql_folder VARCHAR(80))";
    kl::replaceStr(sql, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(tbl));

    res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        closeConnection(conn);
        return false;
    }




    sql = L"COMMENT ON TABLE %tbl% IS 'folder;'";
    kl::replaceStr(sql, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(tbl));

    res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        closeConnection(conn);
        return false;
    }

    PQexec(conn, "COMMIT");

    closeConnection(conn);
    return true;
}

bool PgsqlDatabase::renameFile(const std::wstring& path,
                               const std::wstring& new_name)
{
    if (!getFileExist(path))
        return false;

    

    std::wstring folder;
    int slash_pos = path.find_last_of(L'/');
    if (slash_pos == path.npos)
        folder = L"";
         else
        folder = path.substr(0, slash_pos);




    PGconn* conn = createConnection();
    PGresult* res;
    if (!conn)
        return false;


    PQexec(conn, "BEGIN");

    std::wstring sql, tbl, newname;
    tbl = pgsqlGetTablenameFromPath(path);
    newname = pgsqlGetTablenameFromPath(folder + L"/" + new_name);


    sql = L"LOCK TABLE " + pgsqlQuoteIdentifierIfNecessary(tbl) + L" IN ACCESS EXCLUSIVE MODE NOWAIT";
    res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        PQexec(conn, "ROLLBACK");
        closeConnection(conn);
        return false;
    }



    sql = L"ALTER TABLE %tbl% RENAME TO %newname%";
    kl::replaceStr(sql, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(tbl));
    kl::replaceStr(sql, L"%newname%", pgsqlQuoteIdentifierIfNecessary(newname));

    res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        closeConnection(conn);
        return false;
    }

    PQexec(conn, "COMMIT");

    closeConnection(conn);
    return true;
}

bool PgsqlDatabase::moveFile(const std::wstring& path,
                             const std::wstring& new_location)
{
    if (!getFileExist(path))
        return false;

    if (getFileExist(new_location))
        return false;

    PGconn* conn = createConnection();
    PGresult* res;
    if (!conn)
        return false;


    PQexec(conn, "BEGIN");

    std::wstring sql, tbl, newname;
    tbl = pgsqlGetTablenameFromPath(path);
    newname = pgsqlGetTablenameFromPath(new_location);


    sql = L"LOCK TABLE " + pgsqlQuoteIdentifierIfNecessary(tbl) + L" IN ACCESS EXCLUSIVE MODE NOWAIT";
    res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        PQexec(conn, "ROLLBACK");
        closeConnection(conn);
        return false;
    }



    sql = L"ALTER TABLE %tbl% RENAME TO %newname%";
    kl::replaceStr(sql, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(tbl));
    kl::replaceStr(sql, L"%newname%", pgsqlQuoteIdentifierIfNecessary(newname));

    res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        closeConnection(conn);
        return false;
    }

    PQexec(conn, "COMMIT");

    closeConnection(conn);
    return true;
}

bool PgsqlDatabase::copyFile(const std::wstring& src_path,
                             const std::wstring& dest_path)
{
    return false;
}

bool PgsqlDatabase::copyData(const xd::CopyParams* info, xd::IJob* job)
{
    if (info->iter_input.isOk())
    {
        xd::IIteratorPtr iter = info->iter_input;
        xd::IStructurePtr structure = iter->getStructure();
        if (structure.isNull())
            return false;
    
        if (info->append)
        {
            if (!getFileExist(info->output))
                return false;
        }
         else
        {
            deleteFile(info->output);
            if (!createTable(info->output, structure, NULL))
                return false;
        }


        xdcmnInsert(static_cast<xd::IDatabase*>(this), iter, info->output, info->where, info->limit, job);
        return true;
    }




    PGconn* conn = createConnection();
    if (!conn)
        return xcm::null;

    if (job)
    {
        IPgsqlJobInfoPtr pjob = job;
        if (pjob)
            pjob->setConnection(conn); 
    }



    std::wstring intbl = pgsqlGetTablenameFromPath(info->input);
    std::wstring outtbl = pgsqlGetTablenameFromPath(info->output);
    std::wstring sql;

    if (info->append)
    {
        xd::IStructurePtr instruct = describeTable(info->input);
        xd::IStructurePtr outstruct = describeTable(info->output);

        std::vector<std::wstring> common_fields;
        std::vector<std::wstring>::iterator it;

        int i, cnt = instruct->getColumnCount();
        for (i = 0; i < cnt; ++i)
        {
            std::wstring colname = instruct->getColumnName(i);
            if (outstruct->getColumnExist(colname))
                common_fields.push_back(colname);
        } 

        std::wstring collist;
        for (it = common_fields.begin(); it != common_fields.end(); ++it)
        {
            if (it != common_fields.begin())
                collist += L",";
            collist += *it;
        }

        sql = L"insert into %outtbl% (%collist%) SELECT %collist% from %intbl%";
        kl::replaceStr(sql, L"%intbl%", pgsqlQuoteIdentifierIfNecessary(intbl));
        kl::replaceStr(sql, L"%outtbl%", pgsqlQuoteIdentifierIfNecessary(outtbl));
        kl::replaceStr(sql, L"%collist%", collist);

        if (info->where.length() > 0)
            sql += (L" where " + info->where);
    
        if (info->order.length() > 0)
            sql += (L" order by " + info->order);
    }
     else
    {
        sql = L"create table %outtbl% as select * from %intbl%";
        kl::replaceStr(sql, L"%intbl%", intbl);
        kl::replaceStr(sql, L"%outtbl%", outtbl);

        if (info->where.length() > 0)
            sql += (L" where " + info->where);
    
        if (info->order.length() > 0)
            sql += (L" order by " + info->order);
    }

    PGresult* res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        if (res)
            PQclear(res);
        closeConnection(conn);
        return false;
    }

    closeConnection(conn);
    return true;
}

bool PgsqlDatabase::deleteFile(const std::wstring& path)
{
    xd::IFileInfoPtr info = getFileInfo(path);
    if (info.isNull())
        return false;

    if (info->getType() == xd::filetypeStream)
    {
        PGconn* conn = createConnection();
        if (!conn)
            return xcm::null;

        std::wstring tbl = pgsqlGetTablenameFromPath(path);
        std::wstring sql = L"select blob_id from %tbl%";
        kl::replaceStr(sql, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(tbl));

        PGresult* res = PQexec(conn, kl::toUtf8(sql));
        if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            if (res)
                PQclear(res);
            closeConnection(conn);
            return xcm::null;
        }

        Oid oid = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);

        int r = lo_unlink(conn, oid);
        
        sql = L"drop table if exists %tbl%";
        kl::replaceStr(sql, L"%tbl%", tbl);

        PQexec(conn, kl::toUtf8(sql));

        closeConnection(conn);

        return true;
    }


    PGresult* res;
    PGconn* conn = createConnection();
    if (!conn)
        return false;

    res = PQexec(conn, "BEGIN");
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        closeConnection(conn);
        return false;
    }


    std::wstring command;
    std::wstring tbl = pgsqlGetTablenameFromPath(path);

    command = L"LOCK TABLE " + pgsqlQuoteIdentifierIfNecessary(tbl) + L" IN ACCESS EXCLUSIVE MODE NOWAIT";
    res = PQexec(conn, kl::toUtf8(command));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        PQexec(conn, "ROLLBACK");
        closeConnection(conn);
        return false;
    }

    command = L"DROP TABLE " + pgsqlQuoteIdentifierIfNecessary(tbl);
    res = PQexec(conn, kl::toUtf8(command));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        PQexec(conn, "ROLLBACK");
        closeConnection(conn);
        return false;
    }

    res = PQexec(conn, "COMMIT");
    closeConnection(conn);

    return true;
}

bool PgsqlDatabase::getFileExist(const std::wstring& path)
{
    std::wstring tbl = pgsqlGetTablenameFromPath(path);

    PGconn* conn = createConnection();
    if (!conn)
        return false;

    std::wstring command = L"select count(*) from pg_class where relname='%tbl%' and relkind='r'";
    kl::replaceStr(command, L"%tbl%", tbl);

    bool found = false;

    PGresult* res = PQexec(conn, kl::toUtf8(command));
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1)
    {
        if (0 == strcmp(PQgetvalue(res, 0, 0), "1"))
            found = true;
    }

    closeConnection(conn);
    return found;
}

xd::IFileInfoPtr PgsqlDatabase::getFileInfo(const std::wstring& path)
{
    std::wstring tbl = pgsqlGetTablenameFromPath(path);
    std::wstring folder = L"";
    std::wstring name = L"";


    if (kl::stringFrequency(path, '/') >= 1)
    {
        folder = kl::beforeLast(path, '/');
        name = kl::afterLast(path, '/');
    }
     else
    {
        folder = L"";
        name = path;
    }

    PGconn* conn = createConnection();
    if (!conn)
        return xcm::null;

    std::wstring command = L"select t.tablename as name, coalesce(d.description,'') as type from pg_tables t "
                           L"inner join pg_class as c on c.relname=t.tablename "
                           L"left outer join pg_description as d on c.oid=d.objoid "
                           L"where t.schemaname <> 'pg_catalog' and t.schemaname <> 'information_schema' and c.relname='%tbl%'";
                          
    kl::replaceStr(command, L"%tbl%", tbl);


    PGresult* res = PQexec(conn, kl::toUtf8(command));
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
    {
        PQclear(res);
        closeConnection(conn);
        return xcm::null;
    }

    std::wstring type = kl::towstring(PQgetvalue(res, 0, 1));

    PgsqlFileInfo* f = new PgsqlFileInfo(this);
    f->name = name;
    f->type = xd::filetypeTable;
    f->format = xd::formatNative;

    if (type.substr(0, 7) == L"stream;")
    {
        f->type = xd::filetypeStream;

        type = kl::afterFirst(type, L';');
        type = kl::beforeFirst(type, L';');
        kl::trim(type);

        f->mime_type = type;

        kl::trim(f->mime_type);
    }
     else if (type.substr(0, 7) == L"folder;")
    {
        f->type = xd::filetypeFolder;
        f->mime_type = L"";
    }



    PQclear(res);
    closeConnection(conn);
    return static_cast<xd::IFileInfo*>(f);
}

xd::IFileInfoEnumPtr PgsqlDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;

    std::string prefix = "";
    if (path.length() > 0 && path != L"/")
    {
        std::wstring temps = path;
        kl::trim(temps);
        if (temps.empty())
            return retval;
        if (temps[0] == '/') temps = temps.substr(1);
        kl::replaceStr(temps, L"/", FOLDER_SEPARATOR);
        prefix = kl::tostring(temps + FOLDER_SEPARATOR);
    }


    PGconn* conn = createConnection();
    if (!conn)
        return retval;


    std::string command = "select t.tablename as name, coalesce(d.description,'') as type from pg_tables t "
                          "inner join pg_class as c on c.relname=t.tablename "
                          "left outer join pg_description as d on c.oid=d.objoid "
                          "where t.schemaname <> 'pg_catalog' and t.schemaname <> 'information_schema' ";
                          
    if (prefix.length() > 0)
    {
        std::wstring wprefix = kl::towstring(prefix);
        kl::replaceStr(wprefix, L"_", L"\\_");
        std::string like = kl::tostring(wprefix) + "%";
        command += (" and t.tablename like '" + like + "' ");
    }

    command +=            " UNION "
                          "select viewname  as name, 'view' as type from pg_views  where schemaname <> 'pg_catalog' and schemaname <> 'information_schema'";

    PGresult* res = PQexec(conn, command.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        closeConnection(conn);
        return retval;
    }

    std::wstring name, type;

    int i, rows = PQntuples(res);
    for (i = 0; i < rows; ++i)
    {
        name = kl::towstring(PQgetvalue(res, i, 0));
        type = kl::towstring(PQgetvalue(res, i, 1));

        if (prefix.length() > 0)
            name.erase(0, prefix.length());
        
        if (name.find(FOLDER_SEPARATOR) != name.npos)
            continue; // this item is contained in a 'sub-folder'

        PgsqlFileInfo* f = new PgsqlFileInfo(this);
        f->name = name;
        f->type = xd::filetypeTable;
        f->format = xd::formatNative;

        if (type.substr(0, 7) == L"stream;")
        {
            f->type = xd::filetypeStream;

            type = kl::afterFirst(type, L';');
            type = kl::beforeFirst(type, L';');
            kl::trim(type);

            f->mime_type = type;

            kl::trim(f->mime_type);
        }
         else if (type.substr(0, 7) == L"folder;")
        {
            f->type = xd::filetypeFolder;
            f->mime_type = L"";
        }

        retval->append(f);
    }
    
    PQclear(res);

    closeConnection(conn);

    return retval;
}

std::wstring PgsqlDatabase::getPrimaryKey(const std::wstring path)
{
    std::wstring tbl = pgsqlGetTablenameFromPath(path);
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

    kl::replaceStr(query, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(tbl));

    PGresult* res = PQexec(conn, kl::toUtf8(query));

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        PQclear(res);
        closeConnection(conn);
        return pk;
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

xd::IStructurePtr PgsqlDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<xd::IStructure*>(s);
}

bool PgsqlDatabase::createTable(const std::wstring& path,
                                xd::IStructurePtr struct_config,
                                xd::FormatInfo* format_info)
{
    std::wstring tbl = pgsqlQuoteIdentifierIfNecessary(path);


    std::wstring quote_openchar = m_attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = m_attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

    std::wstring command;
    command.reserve(1024);

    command = L"CREATE TABLE ";
    command += pgsqlQuoteIdentifierIfNecessary(pgsqlGetTablenameFromPath(tbl));
    command += L" (";

    std::wstring field;
    field.reserve(255);

    std::wstring name;
    int type;
    int width;
    int scale;
    
    int i, col_count = struct_config->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        xd::IColumnInfoPtr col_info;
        col_info = struct_config->getColumnInfoByIdx(i);

        // quote the fieldname
        name = pgsqlQuoteIdentifierIfNecessary(col_info->getName());

        type = col_info->getType();
        width = col_info->getWidth();
        scale = col_info->getScale();

        field = pgsqlCreateFieldString(name,
                                  type,
                                  width,
                                  scale,
                                  true,
                                  xd::dbtypePostgres);

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
        // sql command failed
        return false;
    }

    return true;
}

xd::IStreamPtr PgsqlDatabase::openStream(const std::wstring& path)
{
    PGconn* conn = createConnection();
    if (!conn)
        return xcm::null;

    PQexec(conn, "BEGIN");

    std::wstring tbl = pgsqlGetTablenameFromPath(path);
    std::wstring sql = L"select blob_id from %tbl%";
    kl::replaceStr(sql, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(tbl));

    PGresult* res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        if (res)
            PQclear(res);
        closeConnection(conn);
        return xcm::null;
    }

    Oid oid = atoi(PQgetvalue(res, 0, 0));
    
    PQclear(res);


    PgsqlStream* pstream = new PgsqlStream(this);
    if (!pstream->init(oid, conn))
    {
        delete pstream;
        return xcm::null;
    }

    return static_cast<xd::IStream*>(pstream);
}

bool PgsqlDatabase::createStream(const std::wstring& path, const std::wstring& mime_type)
{
    deleteFile(path);


    PGconn* conn = createConnection();
    PGresult* res;
    if (!conn)
        return false;


    PQexec(conn, "BEGIN");

    std::wstring sql, tbl;

    tbl = pgsqlGetTablenameFromPath(path);
    sql = L"CREATE TABLE %tbl% (xdpgsql_stream VARCHAR(80), mime_type VARCHAR(80), blob_id oid)";
    kl::replaceStr(sql, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(tbl));

    res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        closeConnection(conn);
        return false;
    }



    Oid oid = lo_creat(conn, 0);
    if (oid < 0)
        return false;


    sql = L"INSERT INTO %tbl% (xdpgsql_stream, mime_type, blob_id) VALUES ('', '%mimetype%', %oid%)";
    kl::replaceStr(sql, L"%tbl%", tbl);
    kl::replaceStr(sql, L"%mimetype%", mime_type);
    kl::replaceStr(sql, L"%oid%", kl::stdswprintf(L"%u", (unsigned int)oid));

    res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        closeConnection(conn);
        return false;
    }


    sql = L"COMMENT ON TABLE %tbl% IS 'stream; %mimetype%'";
    kl::replaceStr(sql, L"%tbl%", tbl);
    kl::replaceStr(sql, L"%mimetype%", mime_type);

    res = PQexec(conn, kl::toUtf8(sql));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        closeConnection(conn);
        return false;
    }

    PQexec(conn, "COMMIT");

    closeConnection(conn);
    return true;
}

xd::IIteratorPtr PgsqlDatabase::query(const xd::QueryParams& qp)
{
    std::wstring tbl = pgsqlGetTablenameFromPath(qp.from);


    std::wstring query;


    query = L"SELECT * FROM ";
    query += pgsqlQuoteIdentifierIfNecessary(tbl);

    if (qp.where.length() > 0)
    {
        query += L" WHERE ";
        query += qp.where;
    }


    if (qp.order.length() > 0)
    {
        query += L" ORDER BY ";
        query += qp.order;
    }
    

    // create an iterator based on our select statement
    PgsqlIterator* iter = new PgsqlIterator(this);

    if (!iter->init(query))
    {
        delete iter;
        return xcm::null;
    }

    return static_cast<xd::IIterator*>(iter);
}



xd::IIndexInfoPtr PgsqlDatabase::createIndex(const std::wstring& path,
                                                const std::wstring& name,
                                                const std::wstring& expr,
                                                xd::IJob* job)
{
    std::wstring tbl = pgsqlGetTablenameFromPath(path);

    PGconn* conn = createConnection();
    if (!conn)
        return xcm::null;

    std::wstring query = L"CREATE INDEX %idx% ON %tbl% (%expr%)";
    kl::replaceStr(query, L"%idx%", pgsqlQuoteIdentifierIfNecessary(name));
    kl::replaceStr(query, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(tbl));
    kl::replaceStr(query, L"%expr%", expr);

    PGresult* res = PQexec(conn, kl::toUtf8(query));
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    closeConnection(conn);

    if (!success)
        return xcm::null;

    IndexInfo* ii = new IndexInfo;
    ii->setTag(name);
    ii->setExpression(expr);

    return static_cast<xd::IIndexInfo*>(ii);
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
    std::wstring table = pgsqlGetTablenameFromPath(path);

    PGconn* conn = createConnection();
    if (!conn)
        return xcm::null;

    std::wstring query = L"DROP INDEX %idx%";
    kl::replaceStr(query, L"%idx%", pgsqlQuoteIdentifierIfNecessary(name));

    PGresult* res = PQexec(conn, kl::toUtf8(query));
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    closeConnection(conn);

    return success;
}


xd::IIndexInfoEnumPtr PgsqlDatabase::getIndexEnum(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IIndexInfoPtr>* vec;
    vec = new xcm::IVectorImpl<xd::IIndexInfoPtr>;


    std::wstring tbl = pgsqlGetTablenameFromPath(path);

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

    kl::replaceStr(query, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(tbl));

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
        ii->setTag(it->first);
        ii->setExpression(it->second);

        vec->append(static_cast<xd::IIndexInfo*>(ii));
    }

    return vec;
}


xd::IRowInserterPtr PgsqlDatabase::bulkInsert(const std::wstring& path)
{
    std::wstring tbl = pgsqlGetTablenameFromPath(path);

    PgsqlRowInserter* inserter = new PgsqlRowInserter(this, tbl);
    return static_cast<xd::IRowInserter*>(inserter);
}



xd::IStructurePtr PgsqlDatabase::describeTable(const std::wstring& path)
{
    std::wstring tbl = pgsqlGetTablenameFromPath(path);

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

    // create new xd::IStructure
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
        
        if (tango_type == xd::typeNumeric || tango_type == xd::typeDouble)
        {
            type_mod -= 4;
            col_width = (type_mod >> 16);
            col_scale = (type_mod & 0xffff);
        }
         else if (tango_type == xd::typeCharacter || tango_type == xd::typeWideCharacter)
        {
            col_width = type_mod - 4;
            col_scale = 0;
        }
         else if (tango_type == xd::typeDateTime)
        {
            col_width = 8;
            col_scale = 0;
        }
         else if (tango_type == xd::typeInteger || tango_type == xd::typeDate)
        {
            col_width = 4;
            col_scale = 0;
        }
         else
        {
            col_width = 8;
            col_scale = 0;
        }

        xd::IColumnInfoPtr colinfo = pgsqlCreateColInfo(colname,
                                                           pg_type, 
                                                           col_width,
                                                           col_scale,
                                                           L"",
                                                           0);
        s->addColumn(colinfo);
    }
    
    PQclear(res);

    closeConnection(conn);

    return static_cast<xd::IStructure*>(s);
}


bool PgsqlDatabase::modifyStructure(const std::wstring& path, xd::IStructurePtr struct_config, xd::IJob* job)
{
    return false;
}


bool PgsqlDatabase::execute(const std::wstring& command,
                            unsigned int flags,
                            xcm::IObjectPtr& result,
                            xd::IJob* job)
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

    if (kl::iequals(first_word, L"SELECT"))
    {
        // create an iterator based on our select statement
        PgsqlIterator* iter = new PgsqlIterator(this);

        if (flags & xd::sqlAlwaysCopy)
        {
            std::wstring tbl = L"xtmp_" + kl::getUniqueString();
            std::wstring command2 = L"CREATE TABLE " + tbl + L" AS " + command;

            xcm::IObjectPtr resobj;
            bool res = execute(command2, 0, resobj, job);
            if (res)
            {
                if (!iter->init(L"SELECT * FROM " + tbl))
                {
                    delete iter;
                    return false;
                }

                iter->setTable(tbl);
            }
             else
            {
                delete iter;
                return false;
            }
        }
         else
        {
            if (!iter->init(command))
            {
                delete iter;
                return false;
            }
        }

        result = static_cast<xcm::IObject*>(static_cast<xd::IIterator*>(iter));
        return true;
    }
     else
    {
        PGconn* conn = createConnection();
        if (!conn)
            return false;

        if (job)
        {
            IPgsqlJobInfoPtr pjob = job;
            if (pjob)
                pjob->setConnection(conn); 
        }


        PGresult* res = PQexec(conn, kl::toUtf8(command));
        if (!res)
            return false;

        int result_status = PQresultStatus(res);

        PQclear(res);
        closeConnection(conn);
        
        return (result_status == PGRES_COMMAND_OK) ? true : false;
    }
}



bool PgsqlDatabase::groupQuery(xd::GroupQueryParams* info, xd::IJob* job)
{
    bool detail_rows = false;
    size_t grouped_column_count = 0;

    std::wstring sql = L"SELECT ";


    std::vector<std::wstring> columns;
    std::vector<std::wstring> group_parts;
    std::vector<std::wstring>::iterator it;

    kl::parseDelimitedList(info->columns, columns, ',', true);
    kl::parseDelimitedList(info->group, group_parts, ',', true);


    for (it = columns.begin(); it != columns.end(); ++it)
    {
        if (kl::iequals(*it, L"[detail]"))
        {
            columns.erase(it);
            detail_rows = true;
            break;
        }
    }



    size_t cnt = 0;
    for (it = columns.begin(); it != columns.end(); ++it)
    {
        std::wstring fld = kl::beforeFirst(*it, '=');
        std::wstring expr = kl::afterFirst(*it, '=');

        std::wstring func = kl::beforeFirst(expr, '(');
        kl::makeLower(func);
        if (func == L"first")
        {
            expr.erase(0,5);
            dequote(expr, '(', ')');
        }
         else if (func == L"groupid")
        {
            expr = L"row_number() over ()";
        }
         else if (func == L"count")
        {
            expr = L"count(*)";
        }


        if (cnt > 0)
            sql += L",";

        dequote(fld, '"', '"');

        sql += expr + L" AS " + fld;
        cnt++;
    }

    if (detail_rows)
    {
        // we need to make sure that all grouping fields make it into the inner grouping sql,
        // because these will be used to join the detail back to the aggregate output
        int counter = 0;
        for (it = group_parts.begin(); it != group_parts.end(); ++it)
        {
            sql += L",";
            sql += *it;
            sql += L" AS ";
            sql += kl::stdswprintf(L"xdgrpfld%02d", counter++);
        }
    }



    sql += L" FROM " + pgsqlQuoteIdentifierIfNecessary(pgsqlGetTablenameFromPath(info->input));


    if (info->where.length() > 0)
        sql += L" WHERE " + info->where;
    
    if (info->group.length() > 0)
        sql += L" GROUP BY " + info->group;
        
    if (info->having.length() > 0)
        sql += L" HAVING " + info->having;
    
    if (detail_rows)
    {
        std::wstring aggcols; // grouped output columns (aggregate results)
        std::wstring order;

        size_t i;
        for (i = 0; i < columns.size(); ++i)
        {
            std::wstring fld = kl::beforeFirst(columns[i], '=');
            std::wstring expr = kl::afterFirst(columns[i], '=');

            if (i > 0)
                aggcols += L",";
            aggcols += L"b." + fld;

            if (kl::iequals(expr.substr(0,8), L"groupid("))
            {
                // if a groupid() is included, make that the output sort order
                order = fld;
            }
        }


        std::wstring outer_sql = L"SELECT %aggcols%, a.* FROM %tbl% AS a, (%sql%) AS b WHERE ";
        kl::replaceStr(outer_sql, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(pgsqlGetTablenameFromPath(info->input)));
        kl::replaceStr(outer_sql, L"%sql%", sql);
        kl::replaceStr(outer_sql, L"%aggcols%", aggcols);

        int counter = 0;
        for (it = group_parts.begin(); it != group_parts.end(); ++it)
        {
            if (it != group_parts.begin())
                outer_sql += L" AND ";
            outer_sql +=  *it + L" = " + kl::stdswprintf(L"b.xdgrpfld%02d", counter++);
        }

        if (order.length() > 0)
            outer_sql += L" ORDER BY " + order;

        sql = outer_sql;
    }

    if (info->output.length() > 0)
        sql = L"CREATE TABLE " + pgsqlQuoteIdentifierIfNecessary(pgsqlGetTablenameFromPath(info->output)) + L" AS " + sql;


    PGconn* conn = createConnection();
    if (!conn)
        return false;

    PGresult* res = PQexec(conn, kl::toUtf8(sql));
    if (!res)
        return false;

    int result_status = PQresultStatus(res);

    PQclear(res);
    closeConnection(conn);
        
    return (result_status == PGRES_COMMAND_OK) ? true : false;
}
