/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-04-18
 *
 */


#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 1
#endif


#include <kl/string.h>
#include <kl/portable.h>
#include <kl/regex.h>
#include <oci.h>
#include <xd/xd.h>
#include "database.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/jobinfo.h"

const wchar_t* oracle_keywords =
                L"ACCESS,ADD,ALL,ALTER,AND,ANY,ARRAYLEN,AS,ASC,AUDIT,"
                L"BETWEEN,BY,CHAR,CHECK,CLUSTER,COLUMN,COMMENT,"
                L"COMPRESS,CONNECT,CREATE,CURRENT,DATE,DECIMAL,"
                L"DEFAULT,DELETE,DESC,DISTINCT,DROP,ELSE,EXCLUSIVE,"
                L"EXISTS,FILE,FLOAT,FOR,FROM,GRANT,GROUP,HAVING,"
                L"IDENTIFIED,IMMEDIATE,IN,INCREMENT,INDEX,INITIAL,"
                L"INSERT,INTEGER,INTERSECT,INTO,IS,LEVEL,LIKE,LOCK,"
                L"LONG,MAXEXTENTS,MINUS,MODE,MODIFY,NOAUDIT,"
                L"NOCOMPRESS,NOT,NOTFOUND,NOWAIT,NULL,NUMBER,OF,"
                L"OFFLINE,ON,ONLINE,OPTION,OR,ORDER,PCTFREE,PRIOR,"
                L"PRIVILEGES,PUBLIC,RAW,RENAME,RESOURCE,REVOKE,ROW,"
                L"ROWID,ROWLABEL,ROWNUM,ROWS,SELECT,SESSION,SET,"
                L"SHARE,SIZE,SMALLINT,SQLBUF,START,SUCCESSFUL,"
                L"SYNONYM,SYSDATE,TABLE,THEN,TO,TRIGGER,UID,UNION,"
                L"UNIQUE,UPDATE,USER,VALIDATE,VALUES,VARCHAR,"
                L"VARCHAR2,VIEW,WHENEVER,WHERE,WITH";


// the following keywords are listed as
// "secondary" keywords by oracle
const wchar_t* oracle_keywords2 =
                L"ADMIN,AFTER,ALLOCATE,ANALYZE,ARCHIVE,ARCHIVELOG,"
                L"AUTHORIZATION,AVG,BACKUP,BECOME,BEFORE,BEGIN,"
                L"BLOCK,BODY,CACHE,CANCEL,CASCADE,CHANGE,CHARACTER,"
                L"CHECKPOINT,CLOSE,COBOL,COMMIT,COMPILE,CONSTRAINT,"
                L"CONSTRAINTS,CONTENTS,CONTINUE,CONTROLFILE,COUNT,"
                L"CURSOR,CYCLE,DATABASE,DATAFILE,DBA,DEC,DECLARE,"
                L"DISABLE,DISMOUNT,DOUBLE,DUMP,EACH,ENABLE,END,"
                L"ESCAPE,EVENTS,EXCEPT,EXCEPTIONS,EXEC,EXECUTE,"
                L"EXPLAIN,EXTENT,EXTERNALLY,FETCH,FLUSH,FORCE,"
                L"FOREIGN,FORTRAN,FOUND,FREELIST,FREELISTS,"
                L"FUNCTION,GO,GOTO,GROUPS,INCLUDING,INDICATOR,"
                L"INITRANS,INSTANCE,INT,KEY,LANGUAGE,LAYER,LINK,"
                L"LISTS,LOGFILE,MANAGE,MANUAL,MAX,MAXDATAFILES,"
                L"MAXINSTANCES,MAXLOGFILES,MAXLOGHISTORY,"
                L"MAXLOGMEMBERS,MAXTRANS,MAXVALUE,MIN,MINEXTENTS,"
                L"MINVALUE,MODULE,MOUNT,NEW,NEXT,NOARCHIVELOG,"
                L"NOCACHE,NOCYCLE,NOMAXVALUE,NOMINVALUE,NONE,"
                L"NOORDER,NORESETLOGS,NORMAL,NOSORT,NUMERIC,OFF,"
                L"OLD,ONLY,OPEN,OPTIMAL,OWN,PACKAGE,PARALLEL,"
                L"PCTINCREASE,PCTUSED,PLAN,PLI,PRECISION,PRIMARY,"
                L"PRIVATE,PROCEDURE,PROFILE,QUOTA,READ,REAL,"
                L"RECOVER,REFERENCES,REFERENCING,RESETLOGS,"
                L"RESTRICTED,REUSE,ROLE,ROLES,ROLLBACK,SAVEPOINT,"
                L"SCHEMA,SCN,SECTION,SEGMENT,SEQUENCE,SHARED,"
                L"SNAPSHOT,SOME,SORT,SQL,SQLCODE,SQLERROR,SQLSTATE,"
                L"STATEMENT_ID,STATISTICS,STOP,STORAGE,SUM,SWITCH,"
                L"SYSTEM,TABLES,TABLESPACE,TEMPORARY,THREAD,TIME,"
                L"TRACING,TRANSACTION,TRIGGERS,TRUNCATE,UNDER,"
                L"UNLIMITED,UNTIL,USE,USING,WHEN,WORK,WRITE";


// -- utility functions --

std::wstring createOracleFieldString(const std::wstring& _name,
                                     int type,
                                     int width,
                                     int scale,
                                     bool null)
{
    std::wstring name = L"\"" + _name + L"\"";
    kl::makeUpper(name);
    wchar_t buf[255];

    if (width < 1)
    {
        width = 1;
    }

    switch (type)
    {
        case xd::typeCharacter:
        {
            if (width > 4000)
            {
                width = 4000;
            }

            swprintf(buf, 255, L"%ls varchar2(%d char)%ls",
                                    name.c_str(),
                                    width,
                                    null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeWideCharacter:
        {
            if (width > 2000)
            {
                width = 2000;
            }

            swprintf(buf, 255, L"%ls nvarchar2(%d)%ls",
                                    name.c_str(),
                                    width,
                                    null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeNumeric:
        {
            if (width > 21)
            {
                width = 21;
            }

            swprintf(buf, 255, L"%ls number(%d,%d)%ls",
                                    name.c_str(),
                                    width,
                                    scale,
                                    null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeInteger:
        {
            swprintf(buf, 255, L"%ls number(12)%ls",
                                    name.c_str(),
                                    null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeDouble:
        {
            swprintf(buf, 255, L"%ls number(21,%d)%ls",
                                    name.c_str(),
                                    scale,
                                    null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeBoolean:
        {
            swprintf(buf, 255, L"%ls char(1)%ls",
                                    name.c_str(),
                                    null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeDate:
        case xd::typeDateTime:
        {
            swprintf(buf, 255, L"%ls date%ls",
                                    name.c_str(),
                                    null ? L" NULL" : L"");
            return buf;
        }
    }

    return L"";
}


int oracle2xdType(int oracle_type, int oracle_charset)
{
    switch (oracle_type)
    {
        case OCI_TYPECODE_CHAR:              /* SQL CHAR(N)  OTS SQL_CHAR(N) */
        case OCI_TYPECODE_VARCHAR:           /* SQL VARCHAR(N)  OTS SQL_VARCHAR(N) */
        case OCI_TYPECODE_VARCHAR2:          /* SQL VARCHAR2(N)  OTS SQL_VARCHAR2(N) */
            return (oracle_charset == SQLCS_NCHAR) ? xd::typeWideCharacter : xd::typeCharacter;

        case OCI_TYPECODE_SMALLINT:          /* SQL SMALLINT  OTS SMALLINT */
        case OCI_TYPECODE_INTEGER:           /* SQL INTEGER  OTS INTEGER */
        case OCI_TYPECODE_SIGNED8:           /* SQL SIGNED INTEGER(8)  OTS SINT8 */
        case OCI_TYPECODE_SIGNED16:          /* SQL SIGNED INTEGER(16)  OTS SINT16 */
        case OCI_TYPECODE_SIGNED32:          /* SQL SIGNED INTEGER(32)  OTS SINT32 */
        case OCI_TYPECODE_UNSIGNED8:         /* SQL UNSIGNED INTEGER(8)  OTS UINT8 */
        case OCI_TYPECODE_UNSIGNED16:        /* SQL UNSIGNED INTEGER(16)  OTS UINT16 */
        case OCI_TYPECODE_UNSIGNED32:        /* SQL UNSIGNED INTEGER(32)  OTS UINT32 */
            return xd::typeInteger;

        case OCI_TYPECODE_NUMBER:            /* SQL NUMBER(P S)  OTS NUMBER(P S) */
        case OCI_TYPECODE_DECIMAL:           /* SQL DECIMAL(P S)  OTS DECIMAL(P S) */
            return xd::typeNumeric;

        case OCI_TYPECODE_REAL:              /* SQL REAL  OTS SQL_REAL */
        case OCI_TYPECODE_DOUBLE:            /* SQL DOUBLE PRECISION  OTS SQL_DOUBLE */
        case OCI_TYPECODE_FLOAT:             /* SQL FLOAT(P)  OTS FLOAT(P) */
            return xd::typeDouble;

        case OCI_TYPECODE_DATE:              /* SQL DATE  OTS DATE */
            return xd::typeDate;

        case OCI_TYPECODE_TIME:              /* SQL/OTS TIME */
        case OCI_TYPECODE_TIME_TZ:           /* SQL/OTS TIME_TZ */
        case OCI_TYPECODE_TIMESTAMP:         /* SQL/OTS TIMESTAMP */
        case OCI_TYPECODE_TIMESTAMP_TZ:      /* SQL/OTS TIMESTAMP_TZ */
        case OCI_TYPECODE_TIMESTAMP_LTZ:     /* TIMESTAMP_LTZ */
            return xd::typeDateTime;

    }

    return xd::typeInvalid;
}

int xd2oracleType(int xd_type)
{
    switch (xd_type)
    {
        default:
        case xd::typeCharacter:
        case xd::typeWideCharacter:
            return OCI_TYPECODE_VARCHAR2;

        case xd::typeBoolean:
        case xd::typeInteger:
            return OCI_TYPECODE_INTEGER;
        
        case xd::typeNumeric:
            return OCI_TYPECODE_NUMBER;
            
        case xd::typeDouble:
            return OCI_TYPECODE_DOUBLE;

        case xd::typeDate:
            return OCI_TYPECODE_DATE;
            
        case xd::typeDateTime:
            return OCI_TYPECODE_TIMESTAMP;
    }

    return xd::typeInvalid;
}


xd::ColumnInfo createColInfo(const std::wstring& col_name,
                             int oracle_type,
                             int oracle_charset,
                             int width,
                             int precision,
                             int scale,
                             const std::wstring& expr)
{
    int xd_type = oracle2xdType(oracle_type, oracle_charset);

    if (xd_type == xd::typeNumeric)
    {
        width = precision;

        if (width > xd::max_numeric_width)
        {
            width = xd::max_numeric_width;
        }
        
        if (width <= 0 || scale < 0)
        {
            // column scale is unknown, assume 2
            scale = 2;
            
            if (width <= 0)
                width = 18;
        }
    }
     else if (xd_type == xd::typeDateTime)
    {
        width = 8;
        scale = 0;
    }
     else if (xd_type == xd::typeDouble)
    {
        width = 8;
    }
     else if (xd_type == xd::typeDate)
    {
        width = 4;
        scale = 0;
    }
     else if (xd_type == xd::typeInteger)
    {
        width = 4;
        scale = 0;
    }
     else if (xd_type == xd::typeBoolean)
    {
        width = 1;
        scale = 0;
    }


    xd::ColumnInfo c;

    c.name = col_name;
    c.type = xd_type;
    c.width = width;
    c.scale = scale;

    if (expr.length() > 0)
    {
        c.expression = expr;
        c.calculated = true;
    }

    return c;
}





// -- utility function to test oracle function calls --


static sword checkerr(OCIError* err, sword status, char* msg, size_t msgbuf_size)
{
    text errbuf[512];
    sb4 errcode = 0;

    errbuf[0] = 0;

    switch (status)
    {
        case OCI_SUCCESS:
            break;
        case OCI_SUCCESS_WITH_INFO:
            break;
        case OCI_NEED_DATA:
            strcpy((char*)errbuf, "OCI_NEED_DATA");
            break;
        case OCI_NO_DATA:
            strcpy((char*)errbuf, "OCI_NODATA");
            break;
        case OCI_INVALID_HANDLE:
            strcpy((char*)errbuf, "OCI_INVALID_HANDLE");
            break;
        case OCI_STILL_EXECUTING:
            strcpy((char*)errbuf, "OCI_STILL_EXECUTE");
            break;
        case OCI_CONTINUE:
            strcpy((char*)errbuf, "OCI_CONTINUE");
            break;
        case OCI_ERROR:
            OCIErrorGet((dvoid*)err, (ub4)1, (text*)NULL, &errcode, errbuf, (ub4)sizeof(errbuf), OCI_HTYPE_ERROR);
            break;
        default:
            break;
    }

    if (msg && msgbuf_size > 0)
    {
        snprintf(msg, msgbuf_size, (const char*)errbuf);
    }

    return status;
}



static void convertToDotSyntax(std::wstring& str)
{
    if (str.length() > 0 && str[0] == '/')
    {
        static klregex::wregex r(L"/(.+?)/(.+?)");
        
        klregex::wmatch match;
        r.replace(str, L"$1.$2");
    }
     else
    {
        static klregex::wregex r1(L"\\s/(.+?)/(.+?)\\b");
        static klregex::wregex r2(L"\"/(.+?)/(.+?)\"");
        
        klregex::wmatch match;
        r1.replace(str, L" $1.$2");
        r2.replace(str, L"\"$1.$2\"");
    }
}





sword OracleDatabase::checkerr(OCIError* err, sword status)
{
    if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO)
    {
        char errbuf[512];
        ::checkerr(err, status, errbuf, 500);
        m_error.setError(xd::errorGeneral, kl::towstring(errbuf));
    }
    
    return status;
}









OracleDatabase::OracleDatabase()
{
    m_server = L"";
    m_port = 0;
    m_database = L"";
    m_username = L"";
    m_password = L"";

    m_auth = (OCISession*)0;
    m_env = (OCIEnv*)0;
    m_srv = (OCIServer*)0;
    m_svc = (OCISvcCtx*)0;
    m_err = (OCIError*)0;
    
    m_last_job = 0;

    std::wstring kws;
    kws += oracle_keywords;
    kws += L",";
    kws += oracle_keywords2;

    m_attr = static_cast<xd::IAttributes*>(new DatabaseAttributes);
    m_attr->setIntAttribute(xd::dbattrColumnMaxNameLength, 30);
    m_attr->setIntAttribute(xd::dbattrTableMaxNameLength, 30);
    m_attr->setStringAttribute(xd::dbattrKeywords, kws);
    m_attr->setStringAttribute(xd::dbattrColumnInvalidChars,
                               L"~`!@%^&*()-=+{}[]|\\:;\"'<,>.?/ \t");
    m_attr->setStringAttribute(xd::dbattrTableInvalidChars,
                               L"~`!@%^&*()-=+{}[]|\\:;\"'<,>.?/ \t");
    m_attr->setStringAttribute(xd::dbattrColumnInvalidStartingChars,
                               L"~`!@#$%^&*()-_=+{}[]|\\:;\"'<,>.?/0123456789 \t");
    m_attr->setStringAttribute(xd::dbattrTableInvalidStartingChars,
                               L"~`!@#$%^&*()-_=+{}[]|\\:;\"'<,>.?/0123456789 \t");
    m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, L"\"");
    m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, L"\"");
    m_attr->setStringAttribute(xd::dbattrIdentifierCharsNeedingQuote, L"");    
}

OracleDatabase::~OracleDatabase()
{
    close();
}




/*
typedef sword (*OCIEnvNlsCreateFunc)(OCIEnv **envhpp,
                          ub4 mode,
                          dvoid* ctxp,
                          dvoid* (*malocfp)(dvoid* ctxp, size_t size),
                          dvoid* (*ralocfp)(dvoid* ctxp, dvoid* memptr, size_t newsize),
                          void (*mfreefp)(dvoid* ctxp, dvoid* memptr),
                          size_t xtramemsz,
                          dvoid** usrmempp,
                          ub2 charset,
                          ub2 ncharset);

typedef ub2 (*OCINlsCharSetNameToIdFunc)(dvoid *hndl, 
                                         CONST oratext *name);

*/

bool OracleDatabase::open(const std::wstring& server,
                          int port,
                          const std::wstring& database,
                          const std::wstring& username,
                          const std::wstring& password)
{
    m_error.clearError();

    // if there is an existing connection, bail out.
    // The application shouldn't call OracleDatabase::open() more than once
    
    if (m_env)
        return false;



    // initialize oci
    
    if (OCI_SUCCESS != checkerr(m_err, OCIEnvCreate((OCIEnv**)&m_env,
                                                   OCI_THREADED | OCI_OBJECT,
                                                   (dvoid*)0,
                                                   (dvoid*(*)(dvoid*, size_t))0,
                                                   (dvoid*(*)(dvoid*, dvoid*, size_t))0,
                                                   (void(*)(dvoid *, dvoid *))0,
                                                   (size_t)0,
                                                   (void **)0)))
    {
        m_error.setError(xd::errorGeneral, L"The Oracle client environment could not be created.");

        return false;
    }

/*
    if (OCI_SUCCESS != checkerr(m_err, OCIEnvNlsCreate((OCIEnv**)&m_env,
                                                   OCI_THREADED | OCI_OBJECT,
                                                   (dvoid*)0,
                                                   (dvoid*(*)(dvoid*, size_t))0,
                                                   (dvoid*(*)(dvoid*, dvoid*, size_t))0,
                                                   (void(*)(dvoid *, dvoid *))0,
                                                   (size_t)0,
                                                   (void **)0)))
    {
        return false;
    }
*/

    // allocate handles
    if (OCI_SUCCESS != checkerr(m_err, OCIHandleAlloc((dvoid*)m_env,
                                                       (dvoid**)&m_err,
                                                       OCI_HTYPE_ERROR,
                                                       (size_t)0,
                                                       (dvoid**)0)))
    {
        return false;
    }

    if (OCI_SUCCESS != checkerr(m_err, OCIHandleAlloc((dvoid*)m_env,
                                                      (dvoid**)&m_srv,
                                                      OCI_HTYPE_SERVER,
                                                      (size_t)0,
                                                      (dvoid**)0)))
    {
        return false;
    }

    if (OCI_SUCCESS != checkerr(m_err, OCIHandleAlloc((dvoid*)m_env,
                                                      (dvoid**)&m_svc,
                                                      OCI_HTYPE_SVCCTX,
                                                      (size_t)0,
                                                      (dvoid**)0)))
    {
        return false;
    }

    if (OCI_SUCCESS != checkerr(m_err, OCIHandleAlloc((dvoid*)m_env,
                                                      (dvoid**)&m_auth,
                                                      OCI_HTYPE_SESSION,
                                                      (size_t)0,
                                                      (dvoid**)0)))
    {
        return false;
    }

    // create connection
    wchar_t connect_str[512];

    if (server.length() == 0 || database.length() == 0)
    {
        // use a service name, passed in the server or
        // database parameter of this function

        if (server.length() > 0)
        {
            wcscpy(connect_str, server.c_str());
        }
         else if (database.length() > 0)
        {
            wcscpy(connect_str, database.c_str());
        }
         else
        {
            return false;
        }
    }
     else
    {
        // build a connection string
        swprintf(connect_str, 512,
             L"(DESCRIPTION=(ADDRESS=(PROTOCOL=TCP)"
             L"(HOST=%ls)(PORT=%d))(CONNECT_DATA=(SERVICE_NAME=%ls)))",
                server.c_str(),
                port,
                database.c_str());
    }



    // connect to the server
    std::string cs = kl::tostring(connect_str);
    if (OCI_SUCCESS != checkerr(m_err, OCIServerAttach(m_srv,
                                                       m_err,
                                                       (text*)cs.c_str(),
                                                       (sb4)cs.length(),
                                                       0)))
    {
        return false;
    }


    // set attribute server context in the service context
    if (OCI_SUCCESS != checkerr(m_err, OCIAttrSet((dvoid*)m_svc,
                                                  (ub4)OCI_HTYPE_SVCCTX,
                                                  (dvoid*)m_srv,
                                                  (ub4)0,
                                                  (ub4)OCI_ATTR_SERVER,
                                                  (OCIError*)m_err)))
    {
        return false;
    }


    // set username
    std::string un = kl::tostring(username);
    if (OCI_SUCCESS != checkerr(m_err, OCIAttrSet((dvoid*)m_auth,
                                                  (ub4)OCI_HTYPE_SESSION,
                                                  (dvoid*)un.c_str(),
                                                  (ub4)un.length(),
                                                  (ub4)OCI_ATTR_USERNAME,
                                                  m_err)))
    {
        return false;
    }


    // set password
    std::string pw = kl::tostring(password);
    if (OCI_SUCCESS != checkerr(m_err, OCIAttrSet((dvoid*)m_auth,
                                                  (ub4)OCI_HTYPE_SESSION,
                                                  (dvoid*)pw.c_str(),
                                                  (ub4)pw.length(),
                                                  (ub4)OCI_ATTR_PASSWORD,
                                                  m_err)))
    {
        return false;
    }


    // attempt connection
    if (OCI_SUCCESS != checkerr(m_err, OCISessionBegin(m_svc,
                                                       m_err,
                                                       m_auth,
                                                       OCI_CRED_RDBMS,
                                                       (ub4)OCI_DEFAULT)))
    {
        return false;
    }


    // set service
    if (OCI_SUCCESS != checkerr(m_err, OCIAttrSet((dvoid*)m_svc,
                                                  (ub4)OCI_HTYPE_SVCCTX,
                                                  (dvoid*)m_auth,
                                                  (ub4)0,
                                                  (ub4)OCI_ATTR_SESSION,
                                                  m_err)))
    {
        return false;
    }

    m_server = server;
    m_port = port;
    m_database = database;
    m_username = username;
    m_password = password;

    wchar_t buf[1024];
    swprintf(buf, 1024, L"Oracle (%ls)", server.c_str());
    m_attr->setStringAttribute(xd::dbattrDatabaseName, buf);
    return true;
}


void OracleDatabase::close()
{
    if (!m_env)
    {
        // nothing to clean up
        return;
    }

    // clean up

    if (m_auth)
        OCIHandleFree((dvoid*)m_auth, (ub4)OCI_HTYPE_SESSION);

    if (m_svc)
        OCIHandleFree((dvoid*)m_svc, (ub4)OCI_HTYPE_SVCCTX);

    if (m_srv)
        OCIHandleFree((dvoid*)m_srv, (ub4)OCI_HTYPE_SERVER);

    if (m_err)
        OCIHandleFree((dvoid*)m_err, (ub4)OCI_HTYPE_ERROR);

    if (m_env)
        OCIHandleFree((dvoid*)m_env, (ub4)OCI_HTYPE_ENV);
}




// xd::IDatabase

int OracleDatabase::getDatabaseType()
{
    return xd::dbtypeOracle;
}

std::wstring OracleDatabase::getActiveUid()
{
    return L"";
}

xd::IAttributesPtr OracleDatabase::getAttributes()
{
    return static_cast<xd::IAttributes*>(m_attr);
}

std::wstring OracleDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int OracleDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void OracleDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code, error_string);
}



bool OracleDatabase::cleanup()
{
    return true;
}

xd::IJobPtr OracleDatabase::createJob()
{
    KL_AUTO_LOCK(m_obj_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<xd::IJob*>(job);
}

xd::IDatabasePtr OracleDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool OracleDatabase::setMountPoint(const std::wstring& path,
                                   const std::wstring& connection_str,
                                   const std::wstring& remote_path)
{
    return false;
}
                              
bool OracleDatabase::getMountPoint(const std::wstring& path,
                                   std::wstring& connection_str,
                                   std::wstring& remote_path)
{
    return false;
}

bool OracleDatabase::createFolder(const std::wstring& path)
{
    return false;
}

bool OracleDatabase::renameFile(const std::wstring& path,
                                const std::wstring& new_name)
{
    std::wstring command;
    command.reserve(1024);
    command = L"RENAME TABLE ";
    command += getTablenameFromOfsPath(path);
    command += L" TO ";
    command += getTablenameFromOfsPath(new_name);

    xcm::IObjectPtr result_obj;
    return execute(command, 0, result_obj, NULL);
}


bool OracleDatabase::moveFile(const std::wstring& path,
                              const std::wstring& new_location)
{
    return false;
}

bool OracleDatabase::copyFile(const std::wstring& src_path,
                              const std::wstring& dest_path)
{
    return false;
}

bool OracleDatabase::copyData(const xd::CopyParams* info, xd::IJob* job)
{
    if (info->iter_input.isOk())
    {
        if (!info->append)
        {
            xd::Structure structure = info->iter_input->getStructure();
            if (structure.isNull())
                return false;

            deleteFile(info->output);

            xd::FormatDefinition fd = info->output_format;
            fd.columns.clear();
            size_t i, col_count = structure.getColumnCount();
            for (i = 0; i < col_count; ++i)
                fd.createColumn(structure.getColumnInfoByIdx(i));

            if (!createTable(info->output, fd))
                return false;
        }

        // iterator copy - use xdcmnInsert

        xdcmnInsert(static_cast<xd::IDatabase*>(this), info->iter_input, info->output, info->copy_columns, info->where, info->limit, job);
        return true;
    }
    else
    {
        std::wstring intbl = getTablenameFromOfsPath(info->input);
        std::wstring outtbl = getTablenameFromOfsPath(info->output);
        std::wstring sql = L"create table %outtbl% as select * from %intbl%";
        kl::replaceStr(sql, L"%intbl%", intbl);
        kl::replaceStr(sql, L"%outtbl%", outtbl);

        if (info->where.length() > 0)
            sql += (L" where " + info->where);

        if (info->order.length() > 0)
            sql += (L" order by " + info->order);

        xcm::IObjectPtr result_obj;
        return execute(sql, 0, result_obj, NULL);
    }
}

bool OracleDatabase::deleteFile(const std::wstring& path)
{
    std::wstring command;
    command.reserve(1024);
    command = L"DROP TABLE ";
    command += getTablenameFromOfsPath(path);

    xcm::IObjectPtr result_obj;
    return execute(command, 0, result_obj, NULL);
}

bool OracleDatabase::getFileExist(const std::wstring& path)
{    
    std::wstring owner = m_username;
    std::wstring table = path;
    
    // oracle stores this information in uppercase
    kl::makeUpper(owner);
    kl::makeUpper(table);
    
    convertToDotSyntax(table);
    
    if (table.find(L".") != table.npos)
    {
        owner = kl::beforeFirst(table, '.');
        table = kl::afterFirst(table, '.');
    }



    std::wstring sql = L"SELECT TABLE_NAME FROM ALL_TABLES WHERE ";
    sql += L"OWNER = '";
    sql += owner;
    sql += L"' AND TABLE_NAME = '";
    sql += table;
    sql += L"'";
    
    
    xcm::IObjectPtr result_obj;
    if (!execute(sql, 0, result_obj, NULL))
        return false;
        
    xd::IIteratorPtr iter = result_obj;
    if (iter.isNull())
        return false;
    
    xd::objhandle_t h = iter->getHandle(L"TABLE_NAME");
    if (!h)
        return false;
    
    iter->goFirst();
    return iter->eof() ? false : true;
}



// -- OracleFileInfo class implementation --

class OracleFileInfo : public xdcommon::FileInfo
{
public:

    OracleFileInfo(OracleDatabase* db) : xdcommon::FileInfo()
    {
        m_db = db;
        m_db->ref();
    }
    
    ~OracleFileInfo()
    {
        m_db->unref();
    }
    
    const std::wstring& getPrimaryKey()
    {
        primary_key = m_db->getPrimaryKey(name);
        return primary_key;
    }

private:

    OracleDatabase* m_db;
};


xd::IFileInfoPtr OracleDatabase::getFileInfo(const std::wstring& _path)
{
    std::wstring path = _path;

    if (!getFileExist(path))
        return xcm::null;

    convertToDotSyntax(path);
    
    OracleFileInfo* f = new OracleFileInfo(this);
    f->name = kl::afterFirst(path, '.');
    f->type = xd::filetypeTable;
    f->format = xd::formatDefault;
    return static_cast<xd::IFileInfo*>(f);
}

xd::IFileInfoEnumPtr OracleDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;

    if (path.empty() || path == L"/")
    {
        // the "root" folder for xdoracle will contain a list of user names

        std::wstring query = L"SELECT DISTINCT OWNER FROM ALL_TABLES "
                             L"WHERE TABLESPACE_NAME <> 'SYSAUX'";

        OracleIterator* iter = new OracleIterator(this);
        iter->m_env = m_env;
        iter->m_svc = m_svc;
        iter->m_name = query;
        if (!iter->init(query))
        {
            delete iter;
            return retval;
        }
        
        std::string owner;
        std::string table_name;

        iter->goFirst();
        
        xd::objhandle_t handle = iter->getHandle(L"OWNER");
        
        while (!iter->eof())
        {
            owner = iter->getString(handle);

            OracleFileInfo* f = new OracleFileInfo(this);
            f->name = kl::towstring(owner);
            kl::trim(f->name);
            f->type = xd::filetypeFolder;
            f->format = xd::formatDefault;
            retval->append(f);

            iter->skip(1);
        }

        iter->releaseHandle(handle);
        delete iter;
    }
     else
    {
        std::wstring owner = kl::afterFirst(path, L'/');
        owner = kl::beforeLast(owner, L'/');
        
        std::wstring query = L"SELECT TABLE_NAME AS NAME, "
                             L"       'TABLE' AS TYPE "
                             L"       FROM ALL_TABLES WHERE OWNER='%s' "
                             L"UNION ALL "
                             L"SELECT VIEW_NAME AS NAME, "
                             L"       'VIEW' AS TYPE "
                             L"       FROM ALL_VIEWS WHERE OWNER='%s' ";
        kl::replaceStr(query, L"%s", owner);

        OracleIterator* iter = new OracleIterator(this);
        iter->m_env = m_env;
        iter->m_svc = m_svc;
        iter->m_name = query;
        if (!iter->init(query))
        {
            delete iter;
            return retval;
        }

        std::string table_name;

        iter->goFirst();
        
        xd::objhandle_t handle = iter->getHandle(L"NAME");
        
        while (!iter->eof())
        {
            table_name = iter->getString(handle);

            OracleFileInfo* f = new OracleFileInfo(this);
            f->name = kl::towstring(table_name);
            kl::trim(f->name);
            f->type = xd::filetypeTable;
            f->format = xd::formatDefault;
            retval->append(f);

            iter->skip(1);
        }

        iter->releaseHandle(handle);
        delete iter;
    }


    return retval;
}


std::wstring OracleDatabase::getPrimaryKey(const std::wstring& _table)
{
    std::wstring result;
    
    std::wstring owner = m_username;
    std::wstring table = _table;
    
    // oracle stores this information in uppercase
    kl::makeUpper(owner);
    kl::makeUpper(table);
    
    convertToDotSyntax(table);
    
    if (table.find(L".") != table.npos)
    {
        owner = kl::beforeFirst(table, '.');
        table = kl::afterFirst(table, '.');
    }
    
    std::wstring sql = L"select cons.table_name as table_name,"
                       L"cols.column_name as column_name "
                       L"from all_constraints cons, all_cons_columns cols "
                       L"where cons.constraint_name = cols.constraint_name AND "
                       L"cons.constraint_type = 'P' AND ";
                       
                       
    sql += L" cons.owner='";
    sql += owner;
    sql += L"' AND cons.table_name='";
    sql += table;
    sql += L"'";
    

    xcm::IObjectPtr result_obj;
    if (!execute(sql, 0, result_obj, NULL))
        return L"";
        
    xd::IIteratorPtr iter = result_obj;
    if (iter.isNull())
        return L"";
    
    xd::objhandle_t h = iter->getHandle(L"column_name");
    if (!h)
        return L"";
    
    iter->goFirst();
    while (!iter->eof())
    {
        if (result.length() > 0)
            result += L",";
        result += iter->getWideString(h);
        iter->skip(1);
    }
    
    return result;
}

bool OracleDatabase::createTable(const std::wstring& path, const xd::FormatDefinition& format_definition)
{
    std::wstring command;
    command.reserve(1024);

    command = L"CREATE TABLE ";
    command += getTablenameFromOfsPath(path);
    command += L" ( ";

    std::wstring field;

    std::vector<xd::ColumnInfo>::const_iterator it;
    for (it = format_definition.columns.cbegin(); it != format_definition.columns.cend(); ++it)
    {
        const xd::ColumnInfo& colinfo = *it;
        
        field = createOracleFieldString(colinfo.name, colinfo.type, colinfo.width, colinfo.scale, true);
        command += field;

        if (it+1 != format_definition.columns.cend())
            command += L", ";
    }
    command += L" )";

    xcm::IObjectPtr result_obj;
    return execute(command, 0, result_obj, NULL);
}

xd::IStreamPtr OracleDatabase::openStream(const std::wstring& ofs_path)
{
    return xcm::null;
}

bool OracleDatabase::createStream(const std::wstring& ofs_path, const std::wstring& mime_type)
{
    return false;
}

xd::IIteratorPtr OracleDatabase::query(const xd::QueryParams& qp)
{
    std::wstring columns = qp.columns;
    if (columns.length() == 0)
        columns = L"*";

    std::wstring sql = L"SELECT %columns% FROM %table%";
    kl::replaceStr(sql, L"%columns%", columns);
    kl::replaceStr(sql, L"%table%", qp.from);

    if (qp.order.length() > 0)
        sql += L" ORDER BY " + qp.order;

    xcm::IObjectPtr resobj;
    if (!execute(sql, 0, resobj, qp.job))
        return xcm::null;

    return resobj;
}



xd::IndexInfo OracleDatabase::createIndex(const std::wstring& path,
                                          const std::wstring& name,
                                          const std::wstring& expr,
                                          xd::IJob* job)
{
    return xd::IndexInfo();
}


bool OracleDatabase::renameIndex(const std::wstring& path,
                                 const std::wstring& name,
                                 const std::wstring& new_name)
{
    return false;
}


bool OracleDatabase::deleteIndex(const std::wstring& path,
                                 const std::wstring& name)
{
    return false;
}


xd::IndexInfoEnum OracleDatabase::getIndexEnum(const std::wstring& path)
{
    return xd::IndexInfoEnum();
}


xd::IRowInserterPtr OracleDatabase::bulkInsert(const std::wstring& path)
{
    //OracleRowInserter* inserter = new OracleRowInserter(this);
    //return static_cast<xd::IRowInserter*>(inserter);
    return xcm::null;
}

xd::Structure OracleDatabase::describeTable(const std::wstring& path)
{
    // TODO: implement
    return xd::Structure();
}


bool OracleDatabase::modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job)
{
    return false;
}


bool OracleDatabase::execute(const std::wstring& _command,
                             unsigned int flags,
                             xcm::IObjectPtr& result,
                             xd::IJob* job)
{
    m_error.clearError();
    result.clear();

    std::wstring command = _command;
    
    wchar_t* p = (wchar_t*)command.c_str();
    while (::iswspace(*p)) ++p;
    
    std::wstring first_word;
    while (*p && !iswspace(*p))
    {
        first_word += *p;
        ++p;
    }

    // convert /SCOTT/TABLE to SCOTT.TABLE
    convertToDotSyntax(command);


    if (0 == wcscasecmp(first_word.c_str(), L"SELECT"))
    {
        // create an iterator based on our select statement
        OracleIterator* iter = new OracleIterator(this);
        iter->m_env = m_env;
        iter->m_svc = m_svc;
        iter->m_name = command;
        
        if (!iter->init(command))
        {
            delete iter;
            return false;
        }

        result = static_cast<xcm::IObject*>(static_cast<xd::IIterator*>(iter));
        return true;
    }
     else
    {
        // allocate statement handle
        OCIStmt* stmt;
        sword res;

        checkerr(m_err, res = OCIHandleAlloc((dvoid*)m_env,
                                             (dvoid**)&stmt,
                                             OCI_HTYPE_STMT,
                                             (size_t)0,
                                             (dvoid**)0));


        // prepare sql statement
        std::string asc_command = kl::tostring(command);
        checkerr(m_err, res = OCIStmtPrepare(stmt,
                                             m_err,
                                             (text*)asc_command.c_str(),
                                             (ub4)asc_command.length(),
                                             (ub4)OCI_NTV_SYNTAX,
                                             (ub4)OCI_DEFAULT));


        if (res != OCI_SUCCESS && res != OCI_SUCCESS_WITH_INFO)
        {
            // free statement handle
            OCIHandleFree((dvoid*)stmt, (ub4)OCI_HTYPE_STMT);
            return false;
        }


        // execute sql statement
        checkerr(m_err, res = OCIStmtExecute(m_svc,
                                             stmt,
                                             m_err,
                                             (ub4)1,
                                             (ub4)0,
                                             (CONST OCISnapshot*)NULL,
                                             (OCISnapshot*)NULL,
                                             OCI_DEFAULT));


        if (res != OCI_SUCCESS && res != OCI_SUCCESS_WITH_INFO)
        {
            // free statement handle
            OCIHandleFree((dvoid*)stmt, (ub4)OCI_HTYPE_STMT);
            return false;
        }


        // free statement handle
        OCIHandleFree((dvoid*)stmt, (ub4)OCI_HTYPE_STMT);
        return true;
    }
}


bool OracleDatabase::groupQuery(xd::GroupQueryParams* info, xd::IJob* job)
{
    return false;
}


/*
    if (OCI_SUCCESS != checkerr(m_err, OCIEnvNlsCreate((OCIEnv**)&m_env,
                                                       OCI_DEFAULT,
                                                       (dvoid*)0,
                                                       (dvoid*(*)(dvoid*, size_t))0,
                                                       (dvoid*(*)(dvoid*, dvoid*, size_t))0,
                                                       (void(*)(dvoid *, dvoid *))0,
                                                       (size_t)0,
                                                       (void **)0,
                                                       (ub2)OCI_UTF16ID,
                                                       (ub2)OCI_UTF16ID)))
    {
        return false;
    }
*/

/*  
    OCIEnvNlsCreateFunc pOCIEnvNlsCreate = NULL;
    OCINlsCharSetNameToIdFunc pOCINlsCharSetNameToId = NULL;

#ifdef WIN32
    HMODULE h = LoadLibrary(L"OCI.DLL");
    pOCIEnvNlsCreate = (OCIEnvNlsCreateFunc)GetProcAddress(h, "OCIEnvNlsCreate");
    pOCINlsCharSetNameToId = (OCINlsCharSetNameToIdFunc)GetProcAddress(h, "OCINlsCharSetNameToId");
#else
#endif


    if (pOCIEnvNlsCreate && pOCINlsCharSetNameToId)
    {
        OCIEnv* tenv;
        ub2 charset_id = 0;

        OCIEnvInit(&tenv, OCI_DEFAULT, 0, NULL);
        //charset_id = pOCINlsCharSetNameToId(tenv, (text*)"UTF8");
        OCIHandleFree((dvoid*)tenv, (ub4)OCI_HTYPE_ENV);

        if (OCI_SUCCESS != checkerr(m_err, pOCIEnvNlsCreate((OCIEnv**)&m_env,
                                                           //OCI_DEFAULT,
                                                           OCI_OBJECT,
                                                           (dvoid*)0,
                                                           (dvoid*(*)(dvoid*, size_t))0,
                                                           (dvoid*(*)(dvoid*, dvoid*, size_t))0,
                                                           (void(*)(dvoid *, dvoid *))0,
                                                           (size_t)0,
                                                           (void **)0,
                                                           (ub2)charset_id,
                                                           (ub2)charset_id)))
        {
            return false;
        }
    }
     else
    {
        if (OCI_SUCCESS != checkerr(m_err, OCIEnvCreate((OCIEnv**)&m_env,
                                                       OCI_DEFAULT,
                                                       (dvoid*)0,
                                                       (dvoid*(*)(dvoid*, size_t))0,
                                                       (dvoid*(*)(dvoid*, dvoid*, size_t))0,
                                                       (void(*)(dvoid *, dvoid *))0,
                                                       (size_t)0,
                                                       (void **)0)))
        {
            return false;
        }

    }
*/

