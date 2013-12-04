/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-09-03
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <kl/portable.h>
#include <kl/string.h>
#include "database.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/dbfuncs.h"
#include "iterator.h"
#include "inserter.h"
#include <set>

#ifdef WIN32
#include <xcm/xcmwin32.h>
#endif

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


void testSqlStmt(HSTMT stmt)
{
    SQLTCHAR state[255];
    SQLTCHAR message[2048];

    SQLINTEGER native_error_ptr;
    SQLSMALLINT text_len_ptr;

    SQLRETURN r = SQLGetDiagRec(SQL_HANDLE_STMT,
                                stmt,
                                1,
                                state,
                                &native_error_ptr,
                                message,
                                2048,
                                &text_len_ptr);
}

void testSqlConn(HDBC hdbc)
{
    SQLTCHAR state[255];
    SQLTCHAR message[2048];
    SQLINTEGER native_error_ptr;
    SQLSMALLINT text_len_ptr;

    SQLRETURN r = SQLGetDiagRec(SQL_HANDLE_DBC,
                                hdbc,
                                1,
                                state,
                                &native_error_ptr,
                                message,
                                2048,
                                &text_len_ptr);
}


// utility function to create a valid SQL field string

std::wstring createOdbcFieldString(const std::wstring& name,
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
        case xd::typeCharacter:
        {
            bool compression = false;

            if (db_type == xd::dbtypeAccess)
            {
                compression = true;

                if (width >= 255)
                    width = 255;
            }
            
            if (db_type == xd::dbtypeExcel)
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

        case xd::typeWideCharacter:
        {
            bool character_set = false;
            bool compression = false;
            
            if (db_type == xd::dbtypeSqlServer)
            {
                swprintf(buf, 255, L"%ls nvarchar (%d)%ls",
                                    name.c_str(),
                                    width,
                                    allow_nulls ? L" NULL" : L"");
                return buf;
            }
             else if (db_type == xd::dbtypeMySql)
            {
                character_set = true;
            }
             else if (db_type == xd::dbtypeAccess)
            {
                compression = true;
            }
             else if (db_type == xd::dbtypeExcel)
            {
                if (width >= 255)
                    width = 255;
            }

            swprintf(buf, 255, L"%ls varchar (%d)%ls%ls",
                                name.c_str(),
                                width,
                                character_set ? L" CHARACTER SET UTF8" : L"",
                                compression ? L" WITH COMPRESSION" : L"",
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case xd::typeNumeric:
        {
            if (width > 28)
            {
                width = 28;
            }

            if (db_type == xd::dbtypeMySql)
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

        case xd::typeInteger:
        {
            swprintf(buf, 255, L"%ls integer%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case xd::typeDouble:
        {
            swprintf(buf, 255, L"%ls float%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case xd::typeBoolean:
        {
            swprintf(buf, 255, L"%ls bit%ls",
                                name.c_str(),
                                allow_nulls ? L"" : L" NOT NULL");
            return buf;
        }

        case xd::typeDate:
        {
            if (db_type == xd::dbtypeMySql ||
                db_type == xd::dbtypeDb2 ||
				db_type == xd::dbtypePostgres)
            {
                swprintf(buf, 255, L"%ls date%ls",
                                    name.c_str(),
                                    allow_nulls ? L"" : L" NOT NULL");
            }
             else if (db_type == xd::dbtypeFirebird)
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

        case xd::typeDateTime:
        {
            if (db_type == xd::dbtypeDb2 ||
                db_type == xd::dbtypeFirebird ||
				db_type == xd::dbtypePostgres)
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


SQLSMALLINT xd2sqlType(int xd_type)
{
    switch (xd_type)
    {
        case xd::typeCharacter:
            return SQL_VARCHAR;
        case xd::typeWideCharacter:
            return SQL_WVARCHAR;
        case xd::typeNumeric:
        case xd::typeDouble:
            return SQL_DOUBLE;
        case xd::typeInteger:
            return SQL_INTEGER;
        case xd::typeDate:
            return SQL_TYPE_DATE;
        case xd::typeDateTime:
            return SQL_TYPE_TIMESTAMP;
        case xd::typeBoolean:
            return SQL_BIT;
    }

    return 0;
}

SQLSMALLINT xd2sqlCType(int xd_type)
{
    switch (xd_type)
    {
        case xd::typeCharacter:
            return SQL_C_CHAR;
        case xd::typeWideCharacter:
            return SQL_C_WCHAR;
        case xd::typeNumeric:
        case xd::typeDouble:
            return SQL_C_DOUBLE;
        case xd::typeInteger:
            return SQL_C_SLONG;
        case xd::typeDate:
            return SQL_C_DATE;
        case xd::typeDateTime:
            return SQL_C_TIMESTAMP;
        case xd::typeBoolean:
            return SQL_C_BIT;
    }

    return 0;
}

int sql2xdScale(SQLSMALLINT sql_type, int scale)
{
    // in sql2xdType, we handle some integer types with the DOUBLE
    // or NUMERIC data type; in these cases, the ODBC driver may return
    // a non-zero precision because of the way the data is stored in the
    // native database (SQLite sometimes stores numbers as REAL), and
    // as a result, we're setting large decimal precisions for values
    // that were originally integer but are now presented as DOUBLE or
    // NUMERIC; however, in these cases, the precision should be zero

    switch (sql_type)
    {
        case SQL_INTEGER:
        case SQL_BIGINT:
            return 0;
    }

    return scale;
}

int sql2xdType(SQLSMALLINT sql_type)
{
    switch (sql_type)
    {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
        case SQL_GUID:
            return xd::typeCharacter;
        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:
            return xd::typeWideCharacter;
        case SQL_NUMERIC:
        case SQL_DECIMAL:
            return xd::typeNumeric;
        case SQL_BIGINT:
        case SQL_FLOAT:
        case SQL_DOUBLE:
        case SQL_REAL:
            return xd::typeDouble;
        case SQL_SMALLINT:
        case SQL_TINYINT:
            return xd::typeInteger;
        case SQL_INTEGER:
            return xd::typeNumeric;
        case SQL_TYPE_DATE:
        case SQL_DATE:
            return xd::typeDate;
        case SQL_TYPE_TIME:
        case SQL_TYPE_TIMESTAMP:
        case SQL_TIME:
        case SQL_TIMESTAMP:
            return xd::typeDateTime;
        case SQL_BIT:
            return xd::typeBoolean;
        default:
            return xd::typeInvalid;
    }

    return xd::typeInvalid;
}


// this function consolidates all the rules for creating a
// xd column info structure from odbc field information

xd::IColumnInfoPtr createColInfo(int db_type,
                                    const std::wstring& col_name,
                                    int col_odbc_type,
                                    int col_width,
                                    int col_scale,
                                    const std::wstring& col_expr,
                                    int datetime_sub)
{
    int col_xd_type = sql2xdType(col_odbc_type);
    col_scale = sql2xdScale(col_odbc_type, col_scale);

    if (col_xd_type == xd::typeInvalid)
    {
        return xcm::null;
    }

    if (col_xd_type == xd::typeCharacter ||
        col_xd_type == xd::typeWideCharacter)
    {
        // mysql allows a zero-length character field,
        // which is not allowed presently in xd
        
        if (col_width == 0)
            col_width = 1;
    }

    if (col_xd_type == xd::typeDate &&
        datetime_sub != -1)
    {
        if (datetime_sub == SQL_CODE_TIMESTAMP)
        {
            col_xd_type = xd::typeDateTime;
        }
    }

    if (col_odbc_type == SQL_LONGVARCHAR ||
        col_odbc_type == SQL_WLONGVARCHAR)
    {
        // we don't know the width of this field, so set it to 512
        col_width = 512;
    }

    if (col_odbc_type == SQL_GUID ||
        col_odbc_type == SQL_BINARY || 
        col_odbc_type == SQL_VARBINARY ||
        col_odbc_type == SQL_LONGVARBINARY)
    {
        // limit the maximum column width for a binary field
        if (col_width <= 0 || col_width > 8192)
            col_width = 8192;
    }

    if (db_type == xd::dbtypeExcel &&
        col_scale == 0 &&
        (col_xd_type == xd::typeDouble ||
         col_xd_type == xd::typeNumeric))
    {
        // excel odbc drivers always return 0 for column scale, so we
        // will set it to a more acceptable value
        col_scale = 2;
    }

    if (db_type == xd::dbtypeAccess &&
        col_xd_type == xd::typeCharacter)
    {
        // access always uses Wide Characters
        col_xd_type = xd::typeWideCharacter;
    }

    // handle column width
    if (col_xd_type == xd::typeNumeric)
    {
        if (col_width > 18 || col_width < 1)
            col_width = 18;
    }

    if (col_xd_type == xd::typeDateTime ||
        col_xd_type == xd::typeDouble)
    {
        col_width = 8;
    }

    if (col_xd_type == xd::typeDate ||
        col_xd_type == xd::typeInteger)
    {
        col_width = 4;
    }

    if (col_xd_type == xd::typeBoolean)
    {
        col_width = 1;
    }

    // handle column scale
    if (col_xd_type != xd::typeNumeric &&
        col_xd_type != xd::typeDouble)
    {
        col_scale = 0;
    }
     else
    {
        if (col_scale > 12)
        {
            col_scale = 12;
        }
    }

    // for numeric types, make sure the width is at least 2 greater than the scale
    if (col_xd_type == xd::typeNumeric)
    {
        if (col_width < col_scale + 2)
            col_width = col_scale + 2;
    }

    xd::IColumnInfoPtr col;
    col = static_cast<xd::IColumnInfo*>(new ColumnInfo);

    col->setName(col_name);
    col->setType(col_xd_type);
    col->setWidth(col_width);
    col->setScale(col_scale);
    col->setExpression(col_expr);
    col->setColumnOrdinal(0);

    if (col_expr.length() > 0)
        col->setCalculated(true);
        
    return col;
}

void odbcFixAccessStructure(HDBC conn, const std::wstring& tablename, Structure* s);


// OdbcFileInfo class implementation


class OdbcFileInfo : public xdcommon::FileInfo
{
public:

    OdbcFileInfo(OdbcDatabase* db) : xdcommon::FileInfo()
    {
        m_db = db;
        m_db->ref();
    }
    
    ~OdbcFileInfo()
    {
        m_db->unref();
    }
    
    const std::wstring& getPrimaryKey()
    {
        primary_key = m_db->getPrimaryKey(name);
        return primary_key;
    }
    
private:

    OdbcDatabase* m_db;
};





void getOdbcDriverNames(std::vector<std::wstring>& drivers)
{
    drivers.clear();
    
    HENV env;
    SQLAllocEnv(&env);

    SQLRETURN retcode;
    SQLTCHAR driver_desc[255];
    SQLTCHAR driver_attr[255];
    SQLSMALLINT desc_len, attr_len;

    driver_desc[0] = 0;
    driver_attr[0] = 0;

    retcode = SQLDrivers(env, SQL_FETCH_FIRST, driver_desc, 255, &desc_len, driver_attr, 255, &attr_len);
    while (retcode == SQL_SUCCESS)
    {
        drivers.push_back(sql2wstring(driver_desc));
        retcode = SQLDrivers(env, SQL_FETCH_NEXT, driver_desc, 255, &desc_len, driver_attr, 255, &attr_len);
    }

    SQLFreeEnv(env);
}


static int odbcStateToXdError(SQLTCHAR* _s)
{
#ifdef _UNICODE
    #define IS_STATE(state) (0 == wcscmp((const wchar_t*)_s, L##state))
#else
    #define IS_STATE(state) (0 == strcmp((const char*)_s, #state))
#endif

    if (*_s == '0' && *(_s+1) == '1')
    {
        // just a warning
        return xd::errorNone;
    }
        
    if (IS_STATE("00000")) return xd::errorNone;
    if (IS_STATE("42000")) return xd::errorSyntax;
    if (IS_STATE("42S02")) return xd::errorObjectNotFound;
    if (IS_STATE("42S22")) return xd::errorColumnNotFound;
    if (IS_STATE("42S12")) return xd::errorIndexNotFound;
    if (IS_STATE("08S01")) return xd::errorTransmission;
    if (IS_STATE("HY000")) return xd::errorGeneral;
    if (IS_STATE("22012")) return xd::errorDivisionByZero;
    if (IS_STATE("HY001")) return xd::errorMemory;
    if (IS_STATE("HY013")) return xd::errorMemory;
    if (IS_STATE("HY014")) return xd::errorMemory;
    if (IS_STATE("HY008")) return xd::errorCancelled;
    if (IS_STATE("HYT00")) return xd::errorTimeoutExceeded;
    
    return xd::errorGeneral;
}


// OdbcDatabase class implementation

OdbcDatabase::OdbcDatabase()
{
    m_last_job = 0;

    m_env = 0;

    m_db_name = L"";
    m_conn_str = L"";

    m_using_dsn = false;
    m_db_type = -1;
    m_port = 0;
    m_server = L"";
    m_database = L"";
    m_username = L"";
    m_password = L"";
    
    m_attr = static_cast<xd::IAttributes*>(new DatabaseAttributes);
}

OdbcDatabase::~OdbcDatabase()
{
    close();
}



void OdbcDatabase::errorSqlStmt(HSTMT stmt)
{
    SQLTCHAR state[32];
    SQLTCHAR message[2048];

    SQLINTEGER native_error_ptr;
    SQLSMALLINT text_len_ptr;

    SQLRETURN r = SQLGetDiagRec(SQL_HANDLE_STMT,
                                stmt,
                                1,
                                state,
                                &native_error_ptr,
                                message,
                                2048,
                                &text_len_ptr);
    
    m_error.setError(odbcStateToXdError(state), kl::towstring((TCHAR*)message));
}

void OdbcDatabase::errorSqlConn(HDBC hdbc)
{
    SQLTCHAR state[32];
    SQLTCHAR message[2048];
    SQLINTEGER native_error_ptr;
    SQLSMALLINT text_len_ptr;

    if (hdbc)
    {
        SQLRETURN r = SQLGetDiagRec(SQL_HANDLE_DBC,
                                    hdbc,
                                    1,
                                    state,
                                    &native_error_ptr,
                                    message,
                                    2048,
                                    &text_len_ptr);
        m_error.setError(odbcStateToXdError(state), kl::towstring((TCHAR*)message));
    }
     else
    {
        m_error.setError(xd::errorGeneral, L"");
    }
                          
}


SQLRETURN OdbcDatabase::connect(HDBC conn)
{
    SQLTCHAR out_str[2048];
    short out_length;

    SQLRETURN retval;
    
    retval = SQLDriverConnect(conn,
                              NULL,
                              sqlt(m_conn_str),
                              m_conn_str.length(),
                              (SQLTCHAR*)out_str,
                              2048,
                              &out_length,
                              SQL_DRIVER_NOPROMPT);
                              
#ifdef _DEBUG
    if (retval != SQL_SUCCESS &&
        retval != SQL_SUCCESS_WITH_INFO)
    {
        errorSqlConn(conn);
    }
#endif

    return retval;
}

bool OdbcDatabase::getUsingDsn()
{
    return m_using_dsn;
}

std::wstring OdbcDatabase::getServer()
{
    return m_server;
}

std::wstring OdbcDatabase::getPath()
{
    return m_path;
}


std::wstring OdbcDatabase::getTempFileDirectory()
{
    std::wstring result = m_attr->getStringAttribute(xd::dbattrTempDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}

std::wstring OdbcDatabase::getDefinitionDirectory()
{
    std::wstring result = m_attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}

void OdbcDatabase::setAttributes(HDBC connection)
{
    std::wstring kws = sql92_keywords;

    if (m_db_type == xd::dbtypeAccess)
    {
        kws += L",";
        kws += access_keywords;

        m_attr->setIntAttribute(xd::dbattrColumnMaxNameLength, 64);
        m_attr->setIntAttribute(xd::dbattrTableMaxNameLength, 64);
        m_attr->setStringAttribute(xd::dbattrColumnInvalidChars, L"./\":!#&-`*[]");
        m_attr->setStringAttribute(xd::dbattrTableInvalidChars, L"./\":!#&-`*[]");
        m_attr->setStringAttribute(xd::dbattrColumnInvalidStartingChars, L" ");
        m_attr->setStringAttribute(xd::dbattrTableInvalidStartingChars, L" ");
        m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, L"[");
        m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, L"]");
        m_attr->setStringAttribute(xd::dbattrIdentifierCharsNeedingQuote, L"");        
    }
     else if (m_db_type == xd::dbtypeExcel)
    {
        kws += L",";
        kws += excel_keywords;

        m_attr->setIntAttribute(xd::dbattrColumnMaxNameLength, 31);
        m_attr->setIntAttribute(xd::dbattrTableMaxNameLength, 31);
        m_attr->setStringAttribute(xd::dbattrColumnInvalidChars, L":\\/?*[]");
        m_attr->setStringAttribute(xd::dbattrTableInvalidChars, L":\\/?*[]");
        m_attr->setStringAttribute(xd::dbattrColumnInvalidStartingChars, L" ");
        m_attr->setStringAttribute(xd::dbattrTableInvalidStartingChars, L" ");
        m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, L"[");
        m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, L"]");
        m_attr->setStringAttribute(xd::dbattrIdentifierCharsNeedingQuote, L"");        
    }
     else if (m_db_type == xd::dbtypeMySql)
    {
        // illegal characters in a table name include \/. and characters illegal
        // in filenames, the superset of which includes: \/:*?<>|    
    
        kws += L",";
        kws += mysql_keywords;

        m_attr->setIntAttribute(xd::dbattrColumnMaxNameLength, 64);
        m_attr->setIntAttribute(xd::dbattrTableMaxNameLength, 64);
        m_attr->setStringAttribute(xd::dbattrColumnInvalidChars, 
                                   L"\\./\x00\xFF");
        m_attr->setStringAttribute(xd::dbattrTableInvalidChars, 
                                   L"\\./:*?<>|\x00\xFF");
        m_attr->setStringAttribute(xd::dbattrColumnInvalidStartingChars,
                                   L"\\./\x00\xFF");
        m_attr->setStringAttribute(xd::dbattrTableInvalidStartingChars,
                                   L"\\./:*?<>|\x00\xFF");                               
        m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, L"`");
        m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, L"`");
        m_attr->setStringAttribute(xd::dbattrIdentifierCharsNeedingQuote, L"`~# $!@%^&(){}-+.");
    }
     else if (m_db_type == xd::dbtypeSqlServer)
    {
        kws += L",";
        kws += sqlserver_keywords;
        kws += L",";
        kws += sqlserver_keywords2;

        m_attr->setIntAttribute(xd::dbattrColumnMaxNameLength, 128);
        m_attr->setIntAttribute(xd::dbattrTableMaxNameLength, 128);
        m_attr->setStringAttribute(xd::dbattrColumnInvalidChars, L"");
        m_attr->setStringAttribute(xd::dbattrTableInvalidChars, L"");
        m_attr->setStringAttribute(xd::dbattrColumnInvalidStartingChars, L" ");
        m_attr->setStringAttribute(xd::dbattrTableInvalidStartingChars, L" ");
        m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, L"[");
        m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, L"]");
        m_attr->setStringAttribute(xd::dbattrIdentifierCharsNeedingQuote, L"");        
    }
	 else if (m_db_type == xd::dbtypePostgres)
    {
        kws += L",";
        kws += sqlserver_keywords;
        kws += L",";
        kws += sqlserver_keywords2;

        m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, L"\"");
        m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, L"\"");
    }
     else
    {
        if (connection)
        {
            // we don't know the attributes; get them from the connection info
            SQLRETURN retval;
            SQLTCHAR dbms_quote[16];
            std::wstring dbms_quote_str;
            short out_length = 0;
        
            retval = SQLGetInfo(connection, SQL_IDENTIFIER_QUOTE_CHAR, dbms_quote, 16, &out_length);;
            dbms_quote_str = sql2wstring(dbms_quote);

            m_attr->setStringAttribute(xd::dbattrColumnInvalidChars, 
                                       L"\\./\x00\xFF");
            m_attr->setStringAttribute(xd::dbattrTableInvalidChars,
                                       L"\\./:*?<>|\x00\xFF");
            m_attr->setStringAttribute(xd::dbattrColumnInvalidStartingChars,
                                       L"\\./\x00\xFF");
            m_attr->setStringAttribute(xd::dbattrTableInvalidStartingChars,
                                       L"\\./:*?<>|\x00\xFF");

            m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, dbms_quote_str);
            m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, dbms_quote_str);
            m_attr->setStringAttribute(xd::dbattrIdentifierCharsNeedingQuote, L"`~# $!@%^&(){}-+.");
        }
         else
        {
            // set default quote identifiers to a quote, so that if we don't 
            // explicitly know the quote identifier (e.g. some ODBC drivers), 
            // SQL statements using quote identifiers will still work if the 
            // driver supports SQL92 (quote is the quoting character in SQL92)
            m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, L"\"");
            m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, L"\"");
        }
    }

    m_attr->setStringAttribute(xd::dbattrKeywords, kws);
    m_attr->setIntAttribute(xd::dbattrTableMaxNameLength, 32);
}

bool OdbcDatabase::open(int type,
                        const std::wstring& server,
                        int port,
                        const std::wstring& database,
                        const std::wstring& username,
                        const std::wstring& password,
                        const std::wstring& path)
{

    m_error.clearError();
    
    HDBC conn = NULL;

    // turn connection pooling on
    SQLSetEnvAttr(NULL,
                  SQL_ATTR_CONNECTION_POOLING,
                  (SQLPOINTER)SQL_CP_ONE_PER_DRIVER,
                  SQL_IS_INTEGER);

    // allocate odbc environment
    SQLAllocEnv(&m_env);

    // set ODBC version to 3.0
    SQLSetEnvAttr(m_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    SQLSetEnvAttr(m_env,
                  SQL_ATTR_CP_MATCH,
                  (SQLPOINTER)SQL_CP_RELAXED_MATCH,
                  SQL_IS_INTEGER);

    wchar_t db_label_buf[1024];
    wchar_t conn_buf[4096];

    SQLTCHAR db_name[1024];
    SQLTCHAR dbms_version[1024];
    SQLTCHAR dbms_name[1024];

    std::wstring db_name_str;
    std::wstring dbms_version_str;
    std::wstring dbms_name_str;

    SQLRETURN retval;

    m_using_dsn = false;

    // attempt a connection, based on requested driver type
    switch (type)
    {
        case xd::dbtypeSqlServer:
        {
            m_db_type = xd::dbtypeSqlServer;

            swprintf(db_label_buf, 1024, L"Microsoft SQL Server (%ls)", server.c_str());
            swprintf(conn_buf, 4096,
                     L"Driver={SQL Server};Server=%ls;Database=%ls;Uid=%ls;Pwd=%ls;ExtendedAnsiSQL=1",
                     server.c_str(),
                     database.c_str(),
                     username.c_str(),
                     password.c_str());
                     
            if ((username.length() == 0) || (0 == wcscasecmp(username.c_str(), L"[trusted]")))
            {
                // if no username is specified, use a trusted connection (windows authentication)
                swprintf(conn_buf, 4096,
                         L"Driver={SQL Server};Server=%ls;Database=%ls;Trusted_Connection=Yes;ExtendedAnsiSQL=1",
                         server.c_str(),
                         database.c_str(),
                         username.c_str(),
                         password.c_str());
            }
            
            m_conn_str = conn_buf;

            // attempt a connection
            conn = createConnection(&retval);
            
            break;
        }

        case xd::dbtypeAccess:
        {
            m_db_type = xd::dbtypeAccess;

            std::vector<std::wstring> drivers;
            getOdbcDriverNames(drivers);
            const wchar_t* driver = L"Microsoft Access Driver (*.mdb)";
            std::vector<std::wstring>::iterator driver_it;
            for (driver_it = drivers.begin(); driver_it != drivers.end(); ++driver_it)
            {
                std::wstring d = *driver_it;
                kl::makeUpper(d);
                if (d.find(L"ACCESS DRIVER") != d.npos)
                    driver = driver_it->c_str();
            }

            swprintf(db_label_buf, 1024, L"Microsoft Access (%ls)", path.c_str());
            swprintf(conn_buf, 4096,
                     L"Driver={%ls};Dbq=%ls;ExtendedAnsiSQL=1;Uid=admin;Pwd=",
                     driver, path.c_str());
            m_conn_str = conn_buf;

            // attempt a connection
            conn = createConnection(&retval);
            
            break;
        }

        case xd::dbtypeExcel:
        {
            m_db_type = xd::dbtypeExcel;

            std::vector<std::wstring> drivers;
            getOdbcDriverNames(drivers);
            const wchar_t* driver = L"Microsoft Excel Driver (*.xls)";
            std::vector<std::wstring>::iterator driver_it;
            for (driver_it = drivers.begin(); driver_it != drivers.end(); ++driver_it)
            {
                std::wstring d = *driver_it;
                kl::makeUpper(d);
                if (d.find(L"EXCEL DRIVER") != d.npos)
                    driver = driver_it->c_str();
            }

            swprintf(db_label_buf, 1024, L"Microsoft Excel (%ls)", path.c_str());
            swprintf(conn_buf, 4096,
                     L"Driver={%ls};DriverId=790;ExtendedAnsiSQL=1;Dbq=%ls;DefaultDir=c:\\;READONLY=FALSE",
                     driver, path.c_str());
            m_conn_str = conn_buf;
            
            // attempt a connection
            conn = createConnection(&retval);

            break;
        }

        case xd::dbtypeMySql:
        {
            m_db_type = xd::dbtypeMySql;

            std::vector<std::wstring> drivers;
            getOdbcDriverNames(drivers);
            const wchar_t* driver = NULL;
            std::vector<std::wstring>::iterator driver_it;
            for (driver_it = drivers.begin(); driver_it != drivers.end(); ++driver_it)
            {
                std::wstring d = *driver_it;
                kl::makeUpper(d);
                if (d.find(L"MYSQL") != d.npos)
                    driver = driver_it->c_str();
            }

            if (!driver)
            {
                m_error.setError(xd::errorNoDriver, L"The required MySQL ODBC driver was not found");
                return false;
            }
            
            swprintf(db_label_buf, 1024, L"MySQL (%ls)", server.c_str());
            swprintf(conn_buf, 4096,
                     L"Driver={%ls};Server=%ls;Port=%d;Option=3145731;Stmt=;Database=%ls;Uid=%ls;Pwd=%ls",
                     driver, server.c_str(), port, database.c_str(), username.c_str(), password.c_str());
            m_conn_str = conn_buf;

            // attempt a connection
            conn = createConnection(&retval);

            break;
        }

        case xd::dbtypeDb2:
        {
            m_db_type = xd::dbtypeDb2;

            swprintf(db_label_buf, 1024, L"DB2 (%ls)", server.c_str());
            swprintf(conn_buf, 4096,
                     L"DRIVER={IBM DB2 ODBC DRIVER};HOSTNAME=%ls;SERVICENAME=%d;DATABASE=%ls;UID=%ls;PWD=%ls;PROTOCOL=TCPIP",
                     server.c_str(),
                     port,
                     database.c_str(),
                     username.c_str(),
                     password.c_str());
            m_conn_str = conn_buf;
            
            // attempt a connection
            conn = createConnection(&retval);
        }

        default:
        case xd::dbtypeOdbc: // (dsn)
        {
            m_using_dsn = true;
            m_db_type = xd::dbtypeOdbc;
            
            m_conn_str = L"DSN=";
            
            // DSN should be specified in the database variable;
            // if not there, then in the host/server variable
            if (database.length() > 0)
            {
                m_conn_str += database;
                swprintf(db_label_buf, 1024, L"ODBC DSN %ls", database.c_str());
            }
             else
            {
                m_conn_str += server;
                swprintf(db_label_buf, 1024, L"ODBC DSN %ls", server.c_str());
            }
            
            if (username.length() > 0)
            {
                m_conn_str += L";UID=";
                m_conn_str += username;
            }
            
            if (password.length() > 0)
            {
                m_conn_str += L";PWD=";
                m_conn_str += password;
            }
            
            // attempt a connection
            conn = createConnection(&retval);
        }
    }

    if (retval == SQL_SUCCESS_WITH_INFO)
    {
        errorSqlConn(conn);
    }


    if (retval == SQL_NO_DATA ||
        retval == SQL_ERROR ||
        retval == SQL_INVALID_HANDLE)
    {
        errorSqlConn(conn);

        // failed
        closeConnection(conn);
        return false;
    }

    short out_length = 0;
    retval = SQLGetInfo(conn, SQL_DATABASE_NAME, db_name, 1024, &out_length);
    retval = SQLGetInfo(conn, SQL_DBMS_VER, dbms_version, 1024, &out_length);
    retval = SQLGetInfo(conn, SQL_DBMS_NAME, dbms_name, 1024, &out_length);

    db_name_str = sql2wstring(db_name);
    dbms_version_str = sql2wstring(dbms_version);
    dbms_name_str = sql2wstring(dbms_name);

    if (!wcscasecmp(dbms_name_str.c_str(), L"Microsoft SQL Server"))
    {
        m_db_type = xd::dbtypeSqlServer;
        swprintf(db_label_buf, 1024, L"SQL Server (%ls)", server.c_str());
    }
     else if (!wcscasecmp(dbms_name_str.c_str(), L"Oracle") ||
              !wcscasecmp(dbms_name_str.c_str(), L"Oracle8"))
    {
        m_db_type = xd::dbtypeOracle;
        swprintf(db_label_buf, 1024, L"Oracle (%ls)", server.c_str());
    }
     else if (!wcscasecmp(dbms_name_str.c_str(), L"MySQL"))
    {
        if (m_db_type == xd::dbtypeOdbc)
        {
            // add some parameters to the connection string
            // (which will be used to open new connections by
            // the iterator and set classes).  This option
            // makes large tables work better
            if (m_conn_str.length() > 0 && m_conn_str[m_conn_str.length()-1] != ';')
                m_conn_str += L";";
            m_conn_str += L"Option=3145731";
        }
        
        m_db_type = xd::dbtypeMySql;
        
        swprintf(db_label_buf, 1024, L"MySQL (%ls)", server.c_str());
    }
     else if (!wcscasecmp(dbms_name_str.c_str(), L"Firebird"))
    {
        m_db_type = xd::dbtypeFirebird;
        swprintf(db_label_buf, 1024, L"Firebird (%ls)", server.c_str());
    }
     else if (!wcscasecmp(dbms_name_str.c_str(), L"PostgreSQL"))
    {
        m_db_type = xd::dbtypePostgres;
        swprintf(db_label_buf, 1024, L"PostgreSQL (%ls)", server.c_str());
    }
     else if (!wcscasecmp(dbms_name_str.c_str(), L"Excel"))
    {
        m_db_type = xd::dbtypeExcel;
        swprintf(db_label_buf, 1024, L"Microsoft Excel (%ls)", path.c_str());
    }

    
    if (m_db_type == xd::dbtypeSqlServer)
    {
        // for the main connection (used by renames, other admin calls)
        // set a lock timeout of 10 seconds
        
        // NB: If update queries are executed using OdbcDatabases's connection
        // and locked records are encountered, the update query will simply skip
        // them and continue the transaction.  execute() should be modified to
        // execute some/all SQL requests using a different connection.  SELECT queries
        // are already run using a separate connection.
        
        xcm::IObjectPtr result;
        execute(L"SET LOCK_TIMEOUT 10000", 0, result, NULL);
    }
    

    // set the database attributes
    
    m_attr->setStringAttribute(xd::dbattrDatabaseName, db_label_buf);
    setAttributes(conn);

    m_port = port;
    m_server = server;
    m_username = username;
    m_password = password;
    m_path = path;


    closeConnection(conn);

    return true;
}


void OdbcDatabase::close()
{
    SQLFreeEnv(m_env);
    m_env = 0;

    m_db_name = L"";
    m_conn_str = L"";

    m_db_type = -1;
    m_port = 0;
    m_server = L"";
    m_database = L"";
    m_username = L"";
    m_password = L"";
}


HDBC OdbcDatabase::createConnection(SQLRETURN* retval)
{
    HDBC conn = NULL;
    SQLRETURN r;

    // allocate connection
    if (!SQL_SUCCEEDED(SQLAllocConnect(m_env, &conn)))
        return NULL;
    if (!conn)
        return NULL;

    r = connect(conn);
    if (retval) *retval = r;

    if (SQL_SUCCEEDED(r))
    {
        return conn;
    }
     else
    {
        SQLDisconnect(conn);
        SQLFreeConnect(conn);
        return NULL;
    }
}


void OdbcDatabase::closeConnection(HDBC conn)
{
    SQLDisconnect(conn);
    SQLFreeConnect(conn);
}


int OdbcDatabase::getDatabaseType()
{
    return m_db_type;
}

std::wstring OdbcDatabase::getActiveUid()
{
    return L"";
}

xd::IAttributesPtr OdbcDatabase::getAttributes()
{
    return m_attr;
}



std::wstring OdbcDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int OdbcDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void OdbcDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code, error_string);
}


bool OdbcDatabase::cleanup()
{
    return true;
}

xd::IJobPtr OdbcDatabase::createJob()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<xd::IJob*>(job);
}

xd::IDatabasePtr OdbcDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool OdbcDatabase::setMountPoint(const std::wstring& path,
                                 const std::wstring& connection_str,
                                 const std::wstring& remote_path)
{
    return false;
}
                              
bool OdbcDatabase::getMountPoint(const std::wstring& path,
                                 std::wstring& connection_str,
                                 std::wstring& remote_path)
{
    return false;
}

bool OdbcDatabase::createFolder(const std::wstring& path)
{
    return false;
}

bool OdbcDatabase::renameFile(const std::wstring& path,
                              const std::wstring& new_name)
{
    std::wstring command;

    std::wstring quote_openchar = m_attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = m_attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

    if (m_db_type == xd::dbtypeSqlServer)
    {
        command = L"sp_rename ";
        command += quote_openchar;       
        command += getTablenameFromOfsPath(path);
        command += quote_closechar;       
        command += L",";
        command += quote_openchar;
        command += getTablenameFromOfsPath(new_name);
        command += quote_closechar;
    }
     else
    {
        command = L"RENAME TABLE ";
        command += quote_openchar;
        command += getTablenameFromOfsPath(path);
        command += quote_closechar;
        command += L" TO ";
        command += quote_openchar;
        command += getTablenameFromOfsPath(new_name);
        command += quote_closechar;
    }
    
    if (command.length() > 0)
    {
        xcm::IObjectPtr result_obj;
        return execute(command, 0, result_obj, NULL);
    }
    
    return false;
}

bool OdbcDatabase::moveFile(const std::wstring& path,
                            const std::wstring& new_location)
{
    return false;
}

bool OdbcDatabase::copyFile(const std::wstring& src_path,
                            const std::wstring& dest_path)
{
    return false;
}

bool OdbcDatabase::copyData(const xd::CopyParams* info, xd::IJob* job)
{
    xd::IStructurePtr structure;


    if (info->iter_input.isOk())
    {
        if (!info->append)
        {
            xd::IStructurePtr structure = info->iter_input->getStructure();
            if (structure.isNull())
                return false;

            deleteFile(info->output);

            if (!createTable(info->output, structure, NULL))
                return false;
        }

        // iterator copy - use xdcmnInsert
        xdcmnInsert(static_cast<xd::IDatabase*>(this), info->iter_input, info->output, info->where, info->limit, job);
        return true;
    }
     else
    {
        bool success = true;

        SQLRETURN r;
        HDBC conn = createConnection(&r);
        if (!conn)
            return false;

        std::wstring intbl = getTablenameFromOfsPath(info->input);
        std::wstring outtbl = getTablenameFromOfsPath(info->output);
        std::wstring sql = L"create table %outtbl% as select * from %intbl%";
        kl::replaceStr(sql, L"%intbl%", intbl);
        kl::replaceStr(sql, L"%outtbl%", outtbl);

        if (info->where.length() > 0)
            sql += (L" where " + info->where);
    
        if (info->order.length() > 0)
            sql += (L" order by " + info->order);


        HSTMT stmt;
        SQLAllocStmt(conn, &stmt);

        SQLRETURN retval;
        retval = SQLExecDirect(stmt, sqlt(sql), SQL_NTS);

        if (retval != SQL_SUCCESS)
        {
            success = false;
            errorSqlStmt(stmt);
        }

        if (stmt)
        {
            SQLCloseCursor(stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        }

        closeConnection(conn);
        return success;
    }

}

bool OdbcDatabase::deleteFile(const std::wstring& path)
{
    if (m_db_type == xd::dbtypeExcel)
    {
        std::wstring command;
        command.reserve(1024);
        command = L"DROP TABLE \"";
        command += getTablenameFromOfsPath(path);
        command += L"$\"";

        xcm::IObjectPtr result_obj;
        execute(command, 0, result_obj, NULL);
    }
     else
    {
        std::wstring quote_openchar = m_attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
        std::wstring quote_closechar = m_attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);    
    
        std::wstring command;
        command.reserve(1024);
        command = L"DROP TABLE ";
        command += quote_openchar;
        command += getTablenameFromOfsPath(path);
        command += quote_closechar;

        xcm::IObjectPtr result_obj;
        execute(command, 0, result_obj, NULL);
    }

    return true;
}

bool OdbcDatabase::getFileExist(const std::wstring& _path)
{
    std::wstring path = kl::afterFirst(_path, L'/');

    // there may be a faster way to do this.  In order to
    // determine if the file exists, we are going to get
    // a list of all tables and look for 'path'

    xd::IFileInfoEnumPtr files = getFolderInfo(L"");
    if (!files)
        return false;

    int count = files->size();
    int i;

    for (i = 0 ; i < count; ++i)
    {
        xd::IFileInfoPtr info = files->getItem(i);
        if (wcscasecmp(info->getName().c_str(), path.c_str()) == 0)
        {
            return true;
        }
    }

    return false;
}

xd::IFileInfoPtr OdbcDatabase::getFileInfo(const std::wstring& path)
{
    std::wstring folder;
    std::wstring name;
    
    if (path.empty() || path == L"/")
    {
        OdbcFileInfo* f = new OdbcFileInfo(this);
        f->name = L"/";
        f->type = xd::filetypeFolder;
        f->format = xd::formatDefault;
        return static_cast<xd::IFileInfo*>(f);
    }
     else
    {
        folder = kl::beforeLast(path, L'/');
        name = kl::afterLast(path, L'/');
    }
    
    xd::IFileInfoEnumPtr files = getFolderInfo(folder);
    int i, count;
    
    count = files->size();
    for (i = 0; i < count; ++i)
    {
        xd::IFileInfoPtr finfo = files->getItem(i);
        if (0 == wcscasecmp(finfo->getName().c_str(), name.c_str()))
        {
            return finfo;
        }
    }
    
    return xcm::null;
}

xd::IFileInfoEnumPtr OdbcDatabase::getTreeFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;

    // db2 needs to represent it's tablespace as a tree

    if (path.empty() || path == L"/")
    {
        SQLCHAR schema_name[255];
        SQLLEN schema_name_length;

        SQLRETURN r;

        HDBC conn = createConnection(&r);
        if (!conn)
            return retval;
            
        HSTMT stmt = 0;
        r = SQLAllocStmt(conn, &stmt);
        if (r != SQL_SUCCESS)
        {
            closeConnection(conn);
            return retval;
        }
        
        r = SQLTables(stmt, NULL, 0, NULL, 0, NULL, 0, _t("TABLE"), SQL_NTS);
        if (r != SQL_SUCCESS)
        {
            closeConnection(conn);
            return retval;
        }
        
        // bind columns in result set to storage locations
        r = SQLBindCol(stmt, 2, SQL_C_CHAR, schema_name, 255, &schema_name_length);
        if (r != SQL_SUCCESS)
        {
            closeConnection(conn);
            return retval;
        }
        
        std::set<std::wstring> schemas;

        // populate the tables vector
        while ((r = SQLFetch(stmt)) == SQL_SUCCESS)
        {
            std::wstring name = kl::towstring((const char*)schema_name);
            kl::trim(name);
            kl::makeLower(name);
            schemas.insert(name);
        }

        std::set<std::wstring>::iterator it;
        for (it = schemas.begin(); it != schemas.end(); ++it)
        {
            OdbcFileInfo* f = new OdbcFileInfo(this);
            f->name = *it;
            f->type = xd::filetypeFolder;
            f->format = xd::formatDefault;

            retval->append(f);
        }

        if (stmt)
        {
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        }

        closeConnection(conn);
        
        return retval;
    }
     else
    {
        std::wstring schema = kl::afterFirst(path, L'/');
        schema = kl::beforeLast(schema, L'/');
        
        kl::makeUpper(schema);

        SQLCHAR table_name[255];
        SQLLEN table_name_length;

        SQLRETURN r;

        HDBC conn = createConnection(&r);
        if (!conn)
            return retval;

        HSTMT stmt = 0;
        r = SQLAllocStmt(conn, &stmt);
        if (r != SQL_SUCCESS)
        {
            closeConnection(conn);
            return retval;
        }

        r = SQLTables(stmt, NULL, 0, sqlt(schema), SQL_NTS, NULL, 0, _t("TABLE"), SQL_NTS);
        if (r != SQL_SUCCESS)
        {
            closeConnection(conn);
            return retval;
        }

        // bind columns in result set to storage locations
        r = SQLBindCol(stmt, 3, SQL_C_CHAR, table_name, 255, &table_name_length);
        if (r != SQL_SUCCESS)
        {
            closeConnection(conn);
            return retval;
        }

        // populate the tables vector
        while ((r = SQLFetch(stmt)) == SQL_SUCCESS)
        {
            OdbcFileInfo* f = new OdbcFileInfo(this);
            f->name = kl::towstring((char*)table_name);
            f->type = xd::filetypeTable;
            f->format = xd::formatDefault;

            kl::trim(f->name);
            kl::makeLower(f->name);

            retval->append(f);
        }

        if (stmt)
        {
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        }
        
        closeConnection(conn);

        return retval;
    }

    return retval;
}

xd::IFileInfoEnumPtr OdbcDatabase::getFolderInfo(const std::wstring& path)
{
    if (m_db_type == xd::dbtypeDb2)
    {
        return getTreeFolderInfo(path);
    }

    xcm::IVectorImpl<xd::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;

    //unsigned char table_qualifier[255];
    unsigned char table_owner[255];
    unsigned char table_name[255];
    unsigned char table_type[255];
    //unsigned char table_remarks[255];

    //SQLLEN table_qualifier_length;
    SQLLEN table_owner_length;
    SQLLEN table_name_length;
    SQLLEN table_type_length;
    //SQLLEN table_remarks_length;

    SQLRETURN r;

    HSTMT conn = createConnection(&r);
    if (!conn)
    {
        // no connection
        return retval;
    }
    
    HSTMT stmt = 0;
    r = SQLAllocStmt(conn, &stmt);

    std::wstring filter;
    
    if (m_db_type == xd::dbtypeExcel)
        filter = L"TABLE,SYSTEM TABLE";
         else
        filter = L"TABLE,VIEW";

    r = SQLTables(stmt, NULL, 0, NULL, 0, NULL, 0, sqlt(filter), SQL_NTS);

    // bind columns in result set to storage locations
    
    //r = SQLBindCol(stmt, 1, SQL_C_CHAR, table_qualifier, 255, &table_qualifier_length);
    r = SQLBindCol(stmt, 2, SQL_C_CHAR, table_owner, 255, &table_owner_length);
    r = SQLBindCol(stmt, 3, SQL_C_CHAR, table_name, 255, &table_name_length);
    r = SQLBindCol(stmt, 4, SQL_C_CHAR, table_type, 255, &table_type_length);
    //r = SQLBindCol(stmt, 5, SQL_C_CHAR, table_remarks, 255, &table_remarks_length);

    std::vector<std::wstring>::iterator it;
    std::vector<std::wstring> tables_added;
    
    // populate the tables vector
    while ((r = SQLFetch(stmt)) == SQL_SUCCESS)
    {
        std::wstring wtablename = kl::towstring((const char*)table_name);
        kl::trim(wtablename);

/*
        // useful for debugging
        wtablename += L",";
        wtablename += kl::towstring((const char*)table_owner);
        wtablename += L",";
        wtablename += kl::towstring((const char*)table_type);
*/

        // this comparison is specific to sql server
        if (m_db_type == xd::dbtypeSqlServer)
        {
            if (0 == strcasecmp((const char*)table_owner, "sys"))
                continue;
            if (0 == strcasecmp((const char*)table_owner, "information_schema"))
                continue;
            if (0 == strcasecmp((const char*)table_name, "dtproperties"))
                continue;
        }

        if (m_db_type == xd::dbtypeExcel)
        {
            if (wtablename.find(L'$') == -1)
                continue;
            
            // strip off quotes
            wtablename = kl::afterFirst(wtablename, L'\'');
            wtablename = kl::beforeLast(wtablename, L'\'');

            // strip off dollar sign (is this correct?)
            wtablename = kl::beforeLast(wtablename, L'$');
        }
        
        // check to make sure we don't already have a table with this name
        bool found = false;
        std::wstring check_name = wtablename;
        kl::makeUpper(check_name);
        for (it = tables_added.begin(); it != tables_added.end(); ++it)
        {
            if (check_name == (*it))
            {
                found = true;
                break;
            }
        }
        
        // don't add duplicate tablenames to the list
        if (found)
            continue;
        
        tables_added.push_back(check_name);
        
        OdbcFileInfo* f = new OdbcFileInfo(this);
        f->name = wtablename;
        f->type = xd::filetypeTable;
        f->format = xd::formatDefault;

        retval->append(f);
    }
    

    if (stmt)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }
    
    closeConnection(conn);

    return retval;
}

std::wstring OdbcDatabase::getPrimaryKey(const std::wstring _path)
{
    HDBC conn = createConnection();
    if (!conn)
        return L"";

    std::wstring path = kl::afterFirst(_path, L'/');
    std::wstring result;
    
    SQLRETURN r;
    HSTMT stmt = 0;
    r = SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);

    if (!stmt)
    {
        closeConnection(conn);
        return L"";
    }
        
    r = SQLSetStmtAttr(stmt,
                       SQL_ATTR_CURSOR_TYPE,
                       (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                       SQL_IS_UINTEGER);
                       
    r = SQLPrimaryKeys(stmt, NULL, 0, NULL, 0, sqlt(path), SQL_NTS);
    
    wchar_t col_name[255];
    SQLLEN col_name_ind = SQL_NULL_DATA;



    if (!(r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO))
    {
        #ifdef _DEBUG
        testSqlStmt(stmt);
        #endif
        
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        closeConnection(conn);
        return L"";
    }
    

    // bind column name
    r = SQLBindCol(stmt,
                   4,
                   SQL_C_WCHAR,
                   col_name,
                   255*sizeof(wchar_t),
                   &col_name_ind);

    while (1)
    {
        col_name[0] = 0;

        r = SQLFetch(stmt);
        
        #ifdef _DEBUG
        testSqlStmt(stmt);
        #endif

        if (r == SQL_ERROR || r == SQL_NO_DATA)
        {
            // failed
            break;
        }

        if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
        {
            std::wstring wcol_name = col_name;

            if (result.length() > 0)
                result += L",";
            result += wcol_name;
        }
         else
        {
            // failed
            break;
        }
    }

    SQLCloseCursor(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    
    closeConnection(conn);

    return result;
}

xd::IStructurePtr OdbcDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<xd::IStructure*>(s);
}

bool OdbcDatabase::createTable(const std::wstring& path,
                               xd::IStructurePtr struct_config,
                               xd::FormatInfo* format_info)
{
    std::wstring quote_openchar = m_attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = m_attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

    std::wstring command;
    command.reserve(1024);

    command = L"CREATE TABLE ";
    command += quote_openchar;
    command += getTablenameFromOfsPath(path);
    command += quote_closechar;
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
        name = L"";
        name += quote_openchar;
        name += col_info->getName();
        name += quote_closechar;

        type = col_info->getType();
        width = col_info->getWidth();
        scale = col_info->getScale();

        field = createOdbcFieldString(name,
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
    return execute(command, 0, result_obj, NULL);
}

xd::IStreamPtr OdbcDatabase::openStream(const std::wstring& ofs_path)
{
    return xcm::null;
}

bool OdbcDatabase::createStream(const std::wstring& ofs_path, const std::wstring& mime_type)
{
    return false;
}


xd::IIteratorPtr OdbcDatabase::query(const xd::QueryParams& qp)
{
    std::wstring query;
    query.reserve(1024);

    std::wstring tablename = getTablenameFromOfsPath(qp.from);

    if (m_db_type == xd::dbtypeAccess)
    {
        xd::IStructurePtr s = describeTable(qp.from);
        if (s.isNull())
            return xcm::null;

        int i, cnt = s->getColumnCount();

        query = L"SELECT ";
        for (i = 0; i < cnt; ++i)
        {
            xd::IColumnInfoPtr colinfo;
            colinfo = s->getColumnInfoByIdx(i);

            if (colinfo->getCalculated())
                continue;
                
            if (i != 0)
            {
                query += L",";
            }
            
            if (colinfo->getType() == xd::typeNumeric ||
                colinfo->getType() == xd::typeDouble)
            {
                query += L"IIF(ISNUMERIC([";
                query += colinfo->getName();
                query += L"]),VAL(STR([";
                query += colinfo->getName();
                query += L"])),null) AS ";
                query += L"[";
                query += colinfo->getName();
                query += L"] ";
            }
             else
            {
                query += L"[";
                query += colinfo->getName();
                query += L"]";
            }
        }

        query += L" FROM ";
        query += L"[";
        query += tablename;
        query += L"]";
    }
     else if (m_db_type == xd::dbtypeExcel)
    {
        query = L"SELECT * FROM ";
        query += L"\"";
        query += tablename;
        query += L"$\"";
    }
     else if (m_db_type == xd::dbtypeOracle)
    {
        xd::IStructurePtr s = describeTable(qp.from);

        int i, cnt = s->getColumnCount();

        query = L"SELECT ";
        for (i = 0; i < cnt; ++i)
        {
            if (i != 0)
                query += L",";

            query += s->getColumnName(i);
        }

        query += L" FROM ";
        query += tablename;
    }
     else
    {
        xd::IAttributesPtr attr = this->getAttributes();
        std::wstring quote_openchar = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
        std::wstring quote_closechar = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);    
    
        query = L"SELECT * FROM ";
        query += quote_openchar;
        query += tablename;
        query += quote_closechar;
    }

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
    OdbcIterator* iter = new OdbcIterator(this);

    if (!iter->init(query))
        return xcm::null;

    return static_cast<xd::IIterator*>(iter);
}





xd::IIndexInfoPtr OdbcDatabase::createIndex(const std::wstring& path,
                                               const std::wstring& name,
                                               const std::wstring& expr,
                                               xd::IJob* job)
{
    return xcm::null;
}


bool OdbcDatabase::renameIndex(const std::wstring& path,
                               const std::wstring& name,
                               const std::wstring& new_name)
{
    return false;
}


bool OdbcDatabase::deleteIndex(const std::wstring& path,
                               const std::wstring& name)
{
    return false;
}


xd::IIndexInfoEnumPtr OdbcDatabase::getIndexEnum(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IIndexInfoPtr>* vec;
    vec = new xcm::IVectorImpl<xd::IIndexInfoPtr>;

    return vec;
}

xd::IRowInserterPtr OdbcDatabase::bulkInsert(const std::wstring& path)
{
    std::wstring tablename = getTablenameFromOfsPath(path);

    OdbcRowInserter* inserter = new OdbcRowInserter(this, tablename);
    return static_cast<xd::IRowInserter*>(inserter);
}


xd::IStructurePtr OdbcDatabase::describeTable(const std::wstring& path)
{
    HDBC conn = createConnection();
    if (!conn)
    {
        // no connection could be made
        // TODO: return more specific m_error code here
        return xcm::null;
    }

    std::wstring schema;
    std::wstring tablename = getTablenameFromOfsPath(path);

    Structure* s = new Structure;

    wchar_t col_name[255];
    short int col_type;
    int col_width;
    short int col_scale;
    short int datetime_sub;

    SQLLEN col_name_ind = SQL_NULL_DATA;
    SQLLEN col_type_ind = SQL_NULL_DATA;
    SQLLEN col_width_ind = SQL_NULL_DATA;
    SQLLEN col_scale_ind = SQL_NULL_DATA;
    SQLLEN datetime_sub_ind = SQL_NULL_DATA;


    HSTMT stmt = 0;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);



    if (tablename.find(L'.') != -1)
    {
        schema = kl::beforeFirst(tablename, L'.');
        tablename = kl::afterFirst(tablename, L'.');
    }


    if (m_db_type == xd::dbtypeExcel)
    {
        // excel wants a $ at the end
        tablename += L"$";
    }


    SQLRETURN r;
    if (m_db_type == xd::dbtypeExcel ||
        m_db_type == xd::dbtypeAccess ||
        schema.empty())
    {
        // excel and access drivers cannot tolerate even an 
        // empty string for parameter 4 of SQLColumns();
        // it wants it to be NULL

        // even SQL Server, if a schema is not specified,
        // wants a NULL instead of an empty string.  This
        //  is taken care of by the 'schema.isEmpty()' check above

        r = SQLColumns(stmt,
                       NULL,
                       0,
                       NULL,
                       0,
                       sqlt(tablename),
                       SQL_NTS,
                       NULL,
                       0);
    }
     else
    {
        r = SQLColumns(stmt,
                       NULL,
                       0,
                       sqlt(schema),
                       SQL_NTS,
                       sqlt(tablename),
                       SQL_NTS,
                       NULL,
                       0);
    }



    #ifdef _DEBUG
    if (r != SQL_SUCCESS)
    {
        testSqlStmt(stmt);
    }
    #endif


    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
    {
        // bind columns in result set to buffers
        
        int c_ord = 5;
        if (m_db_type == xd::dbtypeOracle)
        {
            c_ord = 14;
        }

        // bind column name
        r = SQLBindCol(stmt,
                       4,
                       SQL_C_WCHAR,
                       col_name,
                       255*sizeof(wchar_t),
                       &col_name_ind);

        // bind column type
        r = SQLBindCol(stmt,
                       c_ord,
                       SQL_C_SSHORT,
                       &col_type,
                       sizeof(short int),
                       &col_type_ind);

        // bind column width
        r = SQLBindCol(stmt,
                       7,
                       SQL_C_SLONG,
                       &col_width,
                       sizeof(long),
                       &col_width_ind);

        // bind column scale
        r = SQLBindCol(stmt,
                       9,
                       SQL_C_SSHORT,
                       &col_scale,
                       sizeof(short int),
                       &col_scale_ind);

        if (m_db_type == xd::dbtypeOracle)
        {
            r = SQLBindCol(stmt,
                           15,
                           SQL_C_SSHORT,
                           &datetime_sub,
                           sizeof(short int),
                           &datetime_sub_ind);
        }

        int i = 0;

        // populate the structure
        while (1)
        {
            col_name[0] = 0;
            col_type = 0;
            col_width = 0;
            col_scale = 0;

            r = SQLFetch(stmt);
            
            #ifdef _DEBUG
            testSqlStmt(stmt);
            #endif

            if (r == SQL_ERROR || r == SQL_NO_DATA)
            {
                // failed
                break;
            }

            if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
            {
                std::wstring wcol_name;
                wcol_name = col_name;

                xd::IColumnInfoPtr col;
                
                col = createColInfo(m_db_type,
                                    wcol_name,
                                    col_type,
                                    col_width,
                                    col_scale,
                                    L"",
                                      (datetime_sub_ind == SQL_NULL_DATA) ?
                                         -1 : datetime_sub);

                if (col.isNull())
                    continue;

                col->setColumnOrdinal(i);
                i++;

                s->addColumn(col);
            }
             else
            {
                // failed
                break;
            }
        }
    }

    if (stmt)
    {
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }


    if (m_db_type == xd::dbtypeAccess)
    {
        odbcFixAccessStructure(conn, tablename, s);
    }

    closeConnection(conn);

    /*
    xd::IStructurePtr ret = m_structure->clone();
    appendCalcFields(ret);
    return ret;
    */

    return static_cast<xd::IStructure*>(s);
}

bool OdbcDatabase::modifyStructure(const std::wstring& path,
                                   xd::IStructurePtr struct_config,
                                   xd::IJob* job)
{
    return false;
}


bool OdbcDatabase::execute(const std::wstring& command,
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

    if (0 == wcscasecmp(first_word.c_str(), L"SELECT"))
    {
        // create an iterator based on our select statement
        OdbcIterator* iter = new OdbcIterator(this);

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
}

bool OdbcDatabase::groupQuery(xd::GroupQueryParams* info, xd::IJob* job)
{
    return false;
}








struct TempInfo
{
    xd::IColumnInfoPtr colinfo;
    std::wstring colname;
    char buf[255];
    SQLLEN indicator;
    int max_dec;
};


void odbcFixAccessStructure(HDBC conn, const std::wstring& tablename, Structure* s)
{
    // because Access can store arbitrary numeric scales (decimal digits
    // to the right of the decimal point), we must find out the correct
    // scale by performing a query to look at the data itself

    std::wstring query;
    xd::IColumnInfoPtr colinfo;
    int col_count = s->getColumnCount();
    int i;
    std::vector<TempInfo> cols;
    std::vector<TempInfo>::iterator it;

    query = L"SELECT TOP 100 ";
    
    bool first = true;
    for (i = 0; i < col_count; ++i)
    {
        colinfo = s->getColumnInfoByIdx(i);
        if (colinfo->getType() == xd::typeNumeric ||
            colinfo->getType() == xd::typeDouble)
        {
            if (!first)
                query += L",";
            first = false;
            query += L"STR([";
            query += colinfo->getName();
            query += L"])";
            TempInfo ti;
            ti.colinfo = colinfo;
            ti.max_dec = 0;
            ti.colname = colinfo->getName();
            cols.push_back(ti);
        }
    }

    query += L" FROM [";
    query += tablename;
    query += L"]";



    HSTMT stmt = NULL;
    SQLRETURN retval = 0;
    SQLAllocStmt(conn, &stmt);

    retval = SQLSetStmtAttr(stmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                            0);

    retval = SQLSetStmtAttr(stmt,
                            SQL_ATTR_CONCURRENCY,
                            (SQLPOINTER)SQL_CONCUR_READ_ONLY,
                            0);

    retval = SQLExecDirect(stmt,
                           sqlt(query),
                           SQL_NTS);

    if (retval == SQL_NO_DATA ||
        retval == SQL_ERROR ||
        retval == SQL_INVALID_HANDLE)
    {
        // failed (make no changes to the structure)
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }

    SQLSMALLINT result_col_count = 0;
    SQLNumResultCols(stmt, &result_col_count);

    if (result_col_count != cols.size())
    {
        // failed (make no changes to the structure)
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }

    // bind the columns to their return values
    i = 1;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        retval = SQLBindCol(stmt,
                            i++,
                            SQL_C_CHAR,
                            it->buf,
                            255,
                            &it->indicator);
        if (retval != SQL_SUCCESS)
        {
            // failed (make no changes to the structure)
            SQLCloseCursor(stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            return;
        }
    }


    while (SQLFetch(stmt) == SQL_SUCCESS)
    {
        for (it = cols.begin(); it != cols.end(); ++it)
        {
            if (it->indicator == SQL_NULL_DATA)
                continue;
            char *p = strchr(it->buf, '.');
            if (p)
            {
                int offset = p - it->buf;
                int len = strlen(it->buf);
                int decimal_places = len-(offset+1);
                if (decimal_places > it->max_dec)
                    it->max_dec = decimal_places;
            }
        }
    }

#ifdef _DEBUG
    testSqlStmt(stmt);
#endif

    SQLCloseCursor(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    for (it = cols.begin(); it != cols.end(); ++it)
    {
        it->colinfo->setScale(it->max_dec);
    }


    // now grab scale from SQLColAttribute()

    query = L"SELECT TOP 20 ";
    
    first = true;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        if (!first)
            query += L",";
        first = false;
        query += L"[";
        query += it->colname;
        query += L"]";
    }

    query += L" FROM [";
    query += tablename;
    query += L"]";



    stmt = NULL;
    retval = 0;
    SQLAllocStmt(conn, &stmt);

    retval = SQLSetStmtAttr(stmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                            0);

    retval = SQLSetStmtAttr(stmt,
                            SQL_ATTR_CONCURRENCY,
                            (SQLPOINTER)SQL_CONCUR_READ_ONLY,
                            0);

    retval = SQLExecDirect(stmt,
                           sqlt(query),
                           SQL_NTS);

    if (retval == SQL_NO_DATA ||
        retval == SQL_ERROR ||
        retval == SQL_INVALID_HANDLE)
    {
        // failed (make no changes to the structure)
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }

    result_col_count = 0;
    SQLNumResultCols(stmt, &result_col_count);

    if (result_col_count != cols.size())
    {
        // failed (make no changes to the structure)
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }

    // bind the columns to their return values
    i = 1;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        SQLLEN scale = 0;

        if (SQL_SUCCESS != SQLColAttribute(stmt,
                                           i++,
                                           SQL_DESC_SCALE,
                                           0,
                                           0,
                                           0,
                                           &scale))
        {
            // failed (make no changes to the structure)
            SQLCloseCursor(stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            return;
        }

        if (scale > it->max_dec)
            it->max_dec = scale;
    }

    SQLCloseCursor(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    for (it = cols.begin(); it != cols.end(); ++it)
    {
        it->colinfo->setScale(it->max_dec);
    }
}
