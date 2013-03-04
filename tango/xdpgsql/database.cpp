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


const wchar_t* access_keywords =
                L"ADD,ALL,ALPHANUMERIC,ALTER,AND,ANY,APPLICATION,AS,"
                L"ASC,ASSISTANT,AUTOINCREMENT,AVG,BETWEEN,BINARY,BIT,"
                L"BOOLEAN,BY,BYTE,CHAR,CHARACTER,COLUMN,COMPACTDATABASE,"
                L"CONSTRAINT,CONTAINER,COUNT,COUNTER,CREATE,"
                L"CREATEDATABASE,CREATEFIELD,CREATEGROUP,CREATEINDEX,"
                L"CREATEOBJECT,CREATEPROPERTY,CREATERELATION,"
                L"CREATETABLEDEF,CREATEUSER,CREATEWORKSPACE,CURRENCY,"
                L"CURRENTUSER,DATABASE,DATE,DATETIME,DELETE,DESC,"
                L"DESCRIPTION,DISALLOW,DISTINCT,DISTINCTROW,DOCUMENT,"
                L"DOUBLE,DROP,ECHO,ELSE,END,EQV,ERROR,EXISTS,EXIT,FALSE,"
                L"FIELD,FIELDS,FILLCACHE,FLOAT,FLOAT4,FLOAT8,FOREIGN,"
                L"FORM,FROM,FULL,FUNCTION,GENERAL,GETOBJECT,GETOPTION,"
                L"GOTOPAGE,GROUP,GUID,HAVING,IDLE,IEEEDOUBLE,IEEESINGLE,"
                L"IF,IGNORE,IMP,IN,INDEX,INDEXES,INNER,INSERT,"
                L"INSERTTEXT,INT,INTEGER,INTEGER1,INTEGER2,INTEGER4,"
                L"INTO,IS,JOIN,KEY,LASTMODIFIED,LEFT,LEVEL,LIKE,LOGICAL,"
                L"LOGICAL1,LONG,LONGBINARY,LONGTEXT,MACRO,MATCH,MAX,MIN,"
                L"MOD,MEMO,MODULE,MONEY,MOVE,NAME,NEWPASSWORD,NO,NOT,"
                L"NULL,NUMBER,NUMERIC,OBJECT,OLEOBJECT,OFF,ON,"
                L"OPENRECORDSET,OPTION,OR,ORDER,OUTER,OWNERACCESS,"
                L"PARAMETER,PARAMETERS,PARTIAL,PERCENT,PIVOT,PRIMARY,"
                L"PROCEDURE,PROPERTY,QUERIES,QUERY,QUIT,REAL,RECALC,"
                L"RECORDSET,REFERENCES,REFRESH,REFRESHLINK,"
                L"REGISTERDATABASE,RELATION,REPAINT,REPAIRDATABASE,"
                L"REPORT,REPORTS,REQUERY,RIGHT,SCREEN,SECTION,SELECT,"
                L"SET,SETFOCUS,SETOPTION,SHORT,SINGLE,SMALLINT,SOME,SQL,"
                L"STDEV,STDEVP,STRING,SUM,TABLE,TABLEDEF,TABLEDEFS,"
                L"TABLEID,TEXT,TIME,TIMESTAMP,TOP,TRANSFORM,TRUE,TYPE,"
                L"UNION,UNIQUE,UPDATE,VALUE,VALUES,VAR,VARP,VARBINARY,"
                L"VARCHAR,WHERE,WITH,WORKSPACE,XOR,YEAR,YES,YESNO";


const wchar_t* excel_keywords = L"";


const wchar_t* mysql_keywords =
                L"ADD,ALL,ALTER,ANALYZE,AND,AS,ASC,ASENSITIVE,"
                L"AUTO_INCREMENT,BDB,BEFORE,BERKELEYDB,BETWEEN,"
                L"BIGINT,BINARY,BLOB,BOTH,BY,CALL,CASCADE,CASE,"
                L"CHANGE,CHAR,CHARACTER,CHECK,COLLATE,COLUMN,"
                L"COLUMNS,CONDITION,CONNECTION,CONSTRAINT,"
                L"CONTINUE,CREATE,CROSS,CURRENT_DATE,"
                L"CURRENT_TIME,CURRENT_TIMESTAMP,CURSOR,DATABASE,"
                L"DATABASES,DAY_HOUR,DAY_MICROSECOND,DAY_MINUTE,"
                L"DAY_SECOND,DEC,DECIMAL,DECLARE,DEFAULT,DELAYED,"
                L"DELETE,DESC,DESCRIBE,DETERMINISTIC,DISTINCT,"
                L"DISTINCTROW,DIV,DOUBLE,DROP,ELSE,ELSEIF,ENCLOSED,"
                L"ESCAPED,EXISTS,EXIT,EXPLAIN,FALSE,FETCH,FIELDS,"
                L"FLOAT,FOR,FORCE,FOREIGN,FOUND,FRAC_SECOND,FROM,"
                L"FULLTEXT,GRANT,GROUP,HAVING,HIGH_PRIORITY,"
                L"HOUR_MICROSECOND,HOUR_MINUTE,HOUR_SECOND,IF,"
                L"IGNORE,IN,INDEX,INFILE,INNER,INNODB,INOUT,"
                L"INSENSITIVE,INSERT,INT,INTEGER,INTERVAL,INTO,"
                L"IO_THREAD,IS,ITERATE,JOIN,KEY,KEYS,KILL,LEADING,"
                L"LEAVE,LEFT,LIKE,LIMIT,LINES,LOAD,LOCALTIME,"
                L"LOCALTIMESTAMP,LOCK,LONG,LONGBLOB,LONGTEXT,LOOP,"
                L"LOW_PRIORITY,MASTER_SERVER_ID,MATCH,MEDIUMBLOB,"
                L"MEDIUMINT,MEDIUMTEXT,MIDDLEINT,"
                L"MINUTE_MICROSECOND,MINUTE_SECOND,MOD,NATURAL,NOT,"
                L"NO_WRITE_TO_BINLOG,NULL,NUMERIC,ON,OPTIMIZE,"
                L"OPTION,OPTIONALLY,OR,ORDER,OUT,OUTER,OUTFILE,"
                L"PRECISION,PRIMARY,PRIVILEGES,PROCEDURE,PURGE,"
                L"READ,REAL,REFERENCES,REGEXP,RENAME,REPEAT,"
                L"REPLACE,REQUIRE,RESTRICT,RETURN,REVOKE,RIGHT,"
                L"RLIKE,SECOND_MICROSECOND,SELECT,SENSITIVE,"
                L"SEPARATOR,SET,SHOW,SMALLINT,SOME,SONAME,SPATIAL,"
                L"SPECIFIC,SQL,SQLEXCEPTION,SQLSTATE,SQLWARNING,"
                L"SQL_BIG_RESULT,SQL_CALC_FOUND_ROWS,"
                L"SQL_SMALL_RESULT,SQL_TSI_DAY,SQL_TSI_FRAC_SECOND,"
                L"SQL_TSI_HOUR,SQL_TSI_MINUTE,SQL_TSI_MONTH,"
                L"SQL_TSI_QUARTER,SQL_TSI_SECOND,SQL_TSI_WEEK,"
                L"SQL_TSI_YEAR,SSL,STARTING,STRAIGHT_JOIN,STRIPED,"
                L"TABLE,TABLES,TERMINATED,THEN,TIMESTAMPADD,"
                L"TIMESTAMPDIFF,TINYBLOB,TINYINT,TINYTEXT,TO,"
                L"TRAILING,TRUE,UNDO,UNION,UNIQUE,UNLOCK,UNSIGNED,"
                L"UPDATE,USAGE,USE,USER_RESOURCES,USING,UTC_DATE,"
                L"UTC_TIME,UTC_TIMESTAMP,VALUES,VARBINARY,VARCHAR,"
                L"VARCHARACTER,VARYING,WHEN,WHERE,WHILE,WITH,WRITE,"
                L"XOR,YEAR_MONTH,ZEROFILL"
                L"WITHOUT,WORK,WRITE,YEAR,ZONE";


const wchar_t* sqlserver_keywords =
                L"ADD,ALL,ALTER,AND,ANY,AS,ASC,AUTHORIZATION,BACKUP,"
                L"BEGIN,BETWEEN,BREAK,BROWSE,BULK,BY,CASCADE,CASE,"
                L"CHECK,CHECKPOINT,CLOSE,CLUSTERED,COALESCE,COLLATE,"
                L"COLUMN,COMMIT,COMPUTE,CONSTRAINT,CONTAINS,"
                L"CONTAINSTABLE,CONTINUE,CONVERT,CREATE,CROSS,"
                L"CURRENT,CURRENT_DATE,CURRENT_TIME,"
                L"CURRENT_TIMESTAMP,CURRENT_USER,CURSOR,DATABASE,"
                L"DBCC,DEALLOCATE,DECLARE,DEFAULT,DELETE,DENY,DESC,"
                L"DISK,DISTINCT,DISTRIBUTED,DOUBLE,DROP,DUMMY,DUMP,"
                L"ELSE,END,ERRLVL,ESCAPE,EXCEPT,EXEC,EXECUTE,EXISTS,"
                L"EXIT,FETCH,FILE,FILLFACTOR,FOR,FOREIGN,FREETEXT,"
                L"FREETEXTTABLE,FROM,FULL,FUNCTION,GOTO,GRANT,GROUP,"
                L"HAVING,HOLDLOCK,IDENTITY,IDENTITY_INSERT,"
                L"IDENTITYCOL,IF,IN,INDEX,INNER,INSERT,INTERSECT,"
                L"INTO,IS,JOIN,KEY,KILL,LEFT,LIKE,LINENO,LOAD,"
                L"NATIONAL,NOCHECK,NONCLUSTERED,NOT,NULL,NULLIF,OF,"
                L"OFF,OFFSETS,ON,OPEN,OPENDATASOURCE,OPENQUERY,"
                L"OPENROWSET,OPENXML,OPTION,OR,ORDER,OUTER,OVER,"
                L"PERCENT,PLAN,PRECISION,PRIMARY,PRINT,PROC,"
                L"PROCEDURE,PUBLIC,RAISERROR,READ,READTEXT,"
                L"RECONFIGURE,REFERENCES,REPLICATION,RESTORE,"
                L"RESTRICT,RETURN,REVOKE,RIGHT,ROLLBACK,ROWCOUNT,"
                L"ROWGUIDCOL,RULE,SAVE,SCHEMA,SELECT,SESSION_USER,"
                L"SET,SETUSER,SHUTDOWN,SOME,STATISTICS,SYSTEM_USER,"
                L"TABLE,TEXTSIZE,THEN,TO,TOP,TRAN,TRANSACTION,"
                L"TRIGGER,TRUNCATE,TSEQUAL,UNION,UNIQUE,UPDATE,"
                L"UPDATETEXT,USE,USER,VALUES,VARYING,VIEW,WAITFOR,"
                L"WHEN,WHERE,WHILE,WITH,WRITETEXT";


// the following keywords are listed as
// "possible future" keywords by sql server

const wchar_t* sqlserver_keywords2 =
                L"ABSOLUTE,ACTION,ADMIN,AFTER,AGGREGATE,ALIAS,"
                L"ALLOCATE,ARE,ARRAY,ASSERTION,AT,BEFORE,BINARY,"
                L"BIT,BLOB,BOOLEAN,BOTH,BREADTH,CALL,CASCADED,"
                L"CAST,CATALOG,CHAR,CHARACTER,CLASS,CLOB,COLLATION,"
                L"COMPLETION,CONNECT,CONNECTION,CONSTRAINTS,"
                L"CONSTRUCTOR,CORRESPONDING,CUBE,CURRENT_PATH,"
                L"CURRENT_ROLE,CYCLE,DATA,DATE,DAY,DEC,DECIMAL,"
                L"DEFERRABLE,DEFERRED,DEPTH,DEREF,DESCRIBE,"
                L"DESCRIPTOR,DESTROY,DESTRUCTOR,DETERMINISTIC,"
                L"DIAGNOSTICS,DICTIONARY,DISCONNECT,DOMAIN,DYNAMIC,"
                L"EACH,END-EXEC,EQUALS,EVERY,EXCEPTION,EXTERNAL,"
                L"FALSE,FIRST,FLOAT,FOUND,FREE,GENERAL,GET,GLOBAL,"
                L"GO,GROUPING,HOST,HOUR,IGNORE,IMMEDIATE,INDICATOR,"
                L"INITIALIZE,INITIALLY,INOUT,INPUT,INT,INTEGER,"
                L"INTERVAL,ISOLATION,ITERATE,LANGUAGE,LARGE,LAST,"
                L"LATERAL,LEADING,LESS,LEVEL,LIMIT,LOCAL,LOCALTIME,"
                L"LOCALTIMESTAMP,LOCATOR,MAP,MATCH,MINUTE,MODIFIES,"
                L"MODIFY,MODULE,MONTH,NAMES,NATURAL,NCHAR,NCLOB,"
                L"NEW,NEXT,NO,NONE,NUMERIC,OBJECT,OLD,ONLY,"
                L"OPERATION,ORDINALITY,OUT,OUTPUT,PAD,PARAMETER,"
                L"PARAMETERS,PARTIAL,PATH,POSTFIX,PREFIX,PREORDER,"
                L"PREPARE,PRESERVE,PRIOR,PRIVILEGES,READS,REAL,"
                L"RECURSIVE,REF,REFERENCING,RELATIVE,RESULT,"
                L"RETURNS,ROLE,ROLLUP,ROUTINE,ROW,ROWS,SAVEPOINT,"
                L"SCOPE,SCROLL,SEARCH,SECOND,SECTION,SEQUENCE,"
                L"SESSION,SETS,SIZE,SMALLINT,SPACE,SPECIFIC,"
                L"SPECIFICTYPE,SQL,SQLEXCEPTION,SQLSTATE,"
                L"SQLWARNING,START,STATE,STATEMENT,STATIC,"
                L"STRUCTURE,TEMPORARY,TERMINATE,THAN,TIME,"
                L"TIMESTAMP,TIMEZONE_HOUR,TIMEZONE_MINUTE,TRAILING,"
                L"TRANSLATION,TREAT,TRUE,UNDER,UNKNOWN,UNNEST,"
                L"USAGE,USING,VALUE,VARCHAR,VARIABLE,WHENEVER";


// utility function to create a valid SQL field string

std::wstring createPgsqlFieldString(const std::wstring& name,
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
        case tango::typeCharacter:
        {
            bool compression = false;

            if (db_type == tango::dbtypeAccess)
            {
                compression = true;

                if (width >= 255)
                    width = 255;
            }
            
            if (db_type == tango::dbtypeExcel)
            {
                if (width >= 255)
                    width = 255;
            }

            swprintf(buf, 255, L"%ls varchar (%d)%ls%ls",
                                name.c_str(),
                                width,
                                compression ? L" WITH COMPRESSION" : L"",
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case tango::typeWideCharacter:
        {
            swprintf(buf, 255, L"%ls nvarchar (%d)%ls",
                                name.c_str(),
                                width,
                                allow_nulls ? L" NULL" : L"");
            return buf;
        }

        case tango::typeNumeric:
        {
            if (width > 28)
            {
                width = 28;
            }

            if (db_type == tango::dbtypeMySql)
            {
                // we need to add 2 to the width of decimal fields in
                // MySql for backward compatibility as versions below
                // 3.23 include the minus and period in the width

                width += 2;
            }

            swprintf(buf, 255, L"%ls decimal (%d,%d)%ls",
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
            swprintf(buf, 255, L"%ls float%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case tango::typeBoolean:
        {
            swprintf(buf, 255, L"%ls bit%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case tango::typeDate:
        {
            if (db_type == tango::dbtypeMySql ||
                db_type == tango::dbtypeDb2 ||
				db_type == tango::dbtypePostgres)
            {
                swprintf(buf, 255, L"%ls date%ls",
                                    name.c_str(),
                                    allow_nulls ? L"" : L" NOT NULL");
            }
             else if (db_type == tango::dbtypeFirebird)
            {
                swprintf(buf, 255, L"%ls timestamp%ls",
                        name.c_str(),
                        allow_nulls ? L"" : L" NOT NULL");
            }
             else
            {
                swprintf(buf, 255, L"%ls datetime%ls",
                                    name.c_str(),
                                    allow_nulls ? L"" : L" NOT NULL");
            }

            return buf;
        }

        case tango::typeDateTime:
        {
            if (db_type == tango::dbtypeDb2 ||
                db_type == tango::dbtypeFirebird ||
				db_type == tango::dbtypePostgres)
            {
                swprintf(buf, 255, L"%ls timestamp%ls",
                                    name.c_str(),
                                    allow_nulls ? L"" : L" NOT NULL");
            }
             else
            {
                swprintf(buf, 255, L"%ls datetime%ls",
                                    name.c_str(),
                                    allow_nulls ? L"" : L" NOT NULL");
            }

            return buf;
        }
    }

    return L"";
}

static std::wstring getTablenameFromPath(const std::wstring& path)
{
    return path;
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

    PGresult* res = PQexec(conn, "select tablename from pg_tables");

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
        return retval;

    std::vector<std::string> arr;

    int i, rows = PQntuples(res);
    for (i = 0; i < rows; ++i)
    {
        PgsqlFileInfo* f = new PgsqlFileInfo(this);
        f->name = kl::towstring(PQgetvalue(res, i, 0));
        f->type = tango::filetypeSet;
        f->format = tango::formatNative;

        retval->append(f);
    }
    

    closeConnection(conn);

    return retval;
}

std::wstring PgsqlDatabase::getPrimaryKey(const std::wstring _path)
{
    return L"";
}

tango::IStructurePtr PgsqlDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<tango::IStructure*>(s);
}

tango::ISetPtr PgsqlDatabase::createSet(const std::wstring& path,
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

        field = createPgsqlFieldString(name,
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

    return vec;
}



tango::IStructurePtr PgsqlDatabase::describeTable(const std::wstring& path)
{
    tango::ISetPtr set = openSet(path);
    if (set.isNull())
        return xcm::null;

    return set->getStructure();
}



bool PgsqlDatabase::execute(const std::wstring& command,
                            unsigned int flags,
                            xcm::IObjectPtr& result,
                            tango::IJob* job)
{
    return false;
/*
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

        // initialize Odbc connection for this set
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
        HDBC conn = createConnection();
        if (!conn)
        {
            // no connection could be made
            // (return more specific m_error code here)
            return false;
        }

        HSTMT stmt;
        SQLAllocStmt(conn, &stmt);

        SQLRETURN retval;
        retval = SQLExecDirect(stmt, sqlt(command), SQL_NTS);

        #ifdef _DEBUG
        std::string s = kl::tostring(command);
        #endif

        if (retval != SQL_SUCCESS)
        {
            errorSqlStmt(stmt);
        }

        if (stmt)
        {
            SQLCloseCursor(stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        }

        closeConnection(conn);
        
        return (SQL_SUCCEEDED(retval) || retval == SQL_NO_DATA_FOUND) ? true : false;
    }
    */
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

