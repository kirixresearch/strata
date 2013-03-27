/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-04-16
 *
 */





#include <xcm/xcm.h>
#include <kl/string.h>
#include <kl/portable.h>
#include <kl/regex.h>
#include "tango.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/util.h"
#include "../xdcommon/sqlcommon.h"
#include "drizzle_client.h"
#include "database.h"
#include "set.h"
#include "iterator.h"


const wchar_t* kws =
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


// utility functions

int drizzle2tangoType(int mysql_type)
{
    switch (mysql_type)
    {
        case DRIZZLE_COLUMN_TYPE_TINY:
        case DRIZZLE_COLUMN_TYPE_SHORT:
        case DRIZZLE_COLUMN_TYPE_LONG:
        case DRIZZLE_COLUMN_TYPE_INT24:
        case DRIZZLE_COLUMN_TYPE_YEAR:
            return tango::typeInteger;
        case DRIZZLE_COLUMN_TYPE_LONGLONG:
        case DRIZZLE_COLUMN_TYPE_DECIMAL:
        case DRIZZLE_COLUMN_TYPE_NEWDECIMAL:
            return tango::typeNumeric;
        case DRIZZLE_COLUMN_TYPE_FLOAT:
        case DRIZZLE_COLUMN_TYPE_DOUBLE:
            return tango::typeDouble;
        case DRIZZLE_COLUMN_TYPE_DATE:
            return tango::typeDate;
        case DRIZZLE_COLUMN_TYPE_TIME:
        case DRIZZLE_COLUMN_TYPE_TIMESTAMP:
        case DRIZZLE_COLUMN_TYPE_DATETIME:
            return tango::typeDateTime;
        case DRIZZLE_COLUMN_TYPE_STRING:
        case DRIZZLE_COLUMN_TYPE_VAR_STRING:
        case DRIZZLE_COLUMN_TYPE_BLOB:
        case DRIZZLE_COLUMN_TYPE_LONG_BLOB:
        case DRIZZLE_COLUMN_TYPE_SET:
        case DRIZZLE_COLUMN_TYPE_ENUM:
            return tango::typeCharacter;
        case DRIZZLE_COLUMN_TYPE_NULL:
        default:
            return tango::typeInvalid;
    }

    // TODO: the following types have not been added yet
    /*
        FIELD_TYPE_NEWDATE
        FIELD_TYPE_TINY_BLOB
        FIELD_TYPE_MEDIUM_BLOB
        FIELD_TYPE_LONG_BLOB
        FIELD_TYPE_CHAR
        FIELD_TYPE_INTERVAL
        FIELD_TYPE_GEOMETRY
        FIELD_TYPE_BIT
    */
}


std::wstring createDrizzleFieldString(const std::wstring& name, int type, int width, int scale, bool null)
{
    wchar_t buf[255];

    if (width < 1)
    {
        width = 1;
    }

    switch (type)
    {
        case tango::typeCharacter:
        {
            swprintf(buf, 255, L"%ls varchar (%d)%ls", name.c_str(), width, null ? L" NULL" : L"");
            return buf;
        }

        case tango::typeWideCharacter:
        {
            swprintf(buf, 255, L"%ls varchar (%d)%ls", name.c_str(), width, null ? L" NULL" : L"");
            return buf;
        }

        case tango::typeNumeric:
        {
            if (width > 28)
            {
                width = 28;
            }

            swprintf(buf, 255, L"%ls decimal (%d,%d)%ls", name.c_str(), width, scale, null ? L" NULL" : L"");
            return buf;
        }

        case tango::typeInteger:
        {
            swprintf(buf, 255, L"%ls int%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }

        case tango::typeDouble:
        {
            swprintf(buf, 255, L"%ls float%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }

        case tango::typeDate:
        {
            swprintf(buf, 255, L"%ls date%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }

        case tango::typeDateTime:
        {
            swprintf(buf, 255, L"%ls datetime%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }
        
        case tango::typeBoolean:
        {
            swprintf(buf, 255, L"%ls tinyint%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }        
    }

    return L"";
}



// DrizzleFileInfo class implementation


class DrizzleFileInfo : public xdcommon::FileInfo
{
public:

    DrizzleFileInfo(DrizzleDatabase* db) : xdcommon::FileInfo()
    {
        m_db = db;
        m_db->ref();
    }
    
    ~DrizzleFileInfo()
    {
        m_db->unref();
    }
    
    const std::wstring& getPrimaryKey()
    {
        primary_key = m_db->getPrimaryKey(name);
        return primary_key;
    }

private:

    DrizzleDatabase* m_db;
};



DrizzleDatabase::DrizzleDatabase()
{
    m_db_name = L"";
    m_server = L"";
    m_port = 0;
    m_database = L"";
    m_username = L"";
    m_password = L"";
    
    m_attr = static_cast<tango::IAttributes*>(new DatabaseAttributes);
    m_attr->setIntAttribute(tango::dbattrColumnMaxNameLength, 64);
    m_attr->setIntAttribute(tango::dbattrTableMaxNameLength, 64);
    m_attr->setStringAttribute(tango::dbattrKeywords, kws);    
    m_attr->setStringAttribute(tango::dbattrColumnInvalidChars, 
                               L"\\./\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrTableInvalidChars, 
                               L"\\./:*?<>|\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrColumnInvalidStartingChars,
                               L"\\./\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrTableInvalidStartingChars,
                               L"\\./:*?<>|\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteOpenChar, L"`");
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteCloseChar, L"`");
    m_attr->setStringAttribute(tango::dbattrIdentifierCharsNeedingQuote, L"`~# $!@%^&(){}-+.");    
}

DrizzleDatabase::~DrizzleDatabase()
{
    close();
}


static int sanityCheck()
{
    const char *query = "select name from source_definition";
    drizzle_st drizzle;
    drizzle_con_st* con;
    drizzle_result_st result;
    drizzle_return_t ret;
    char **row;


    if (drizzle_create(&drizzle) == NULL)
    {
        printf("drizzle_create:NULL\n");
        return 1;
    }


    con = drizzle_con_create(&drizzle, NULL);

    drizzle_con_set_auth(con, "testuser", "testpasswd");
    drizzle_con_add_options(con, DRIZZLE_CON_MYSQL);
    drizzle_con_set_tcp(con, "127.0.0.1", 3306);
    drizzle_con_set_db(con, "db_test");

    ret = drizzle_con_connect(con);
    if (ret != DRIZZLE_RETURN_OK)
    {
        printf("drizzle_con_connect:%s\n", drizzle_con_error(con));
        return 1;
    }

    (void)drizzle_query_str(con, &result, query, &ret);
    if (ret != DRIZZLE_RETURN_OK)
    {
        printf("drizzle_query:%s\n", drizzle_con_error(con));
        return 1;
    }

    ret = drizzle_result_buffer(&result);
    if (ret != DRIZZLE_RETURN_OK)
    {
        printf("drizzle_result_buffer:%s\n", drizzle_con_error(con));
        return 1;
    }

    while ((row = (char **)drizzle_row_next(&result)) != NULL)
    {
        for (int x = 0; x < drizzle_result_column_count(&result); x++)
            printf("%s%s", x == 0 ? "" : ":", row[x] == NULL ? "NULL" : row[x]);
        printf("\n");
    }

    drizzle_result_free(&result);
    drizzle_con_free(con);
    drizzle_free(&drizzle);


  return 0;
}

bool DrizzleDatabase::open(const std::wstring& server,
                           int port,
                           const std::wstring& database,
                           const std::wstring& username,
                           const std::wstring& password)
{
   // sanityCheck();
    
    m_error.clearError();
    
    // initialize drizzle library
    if (NULL == drizzle_create(&m_drizzle))
        return false;
    
    // set default timeout to 15 seconds
    drizzle_set_timeout(&m_drizzle, 15000);
    
    m_server = server;
    m_port = port;
    m_database = database;
    m_username = username;
    m_password = password;
    
    // attempt to open the connection
    drizzle_con_st* con = open();
    if (!con)
        return false;
    drizzle_con_free(con);
    
    
    wchar_t buf[1024];
    swprintf(buf, 1024, L"MySQL (%ls)", server.c_str());
    setDatabaseName(buf);
    
    
    // clear query cache -- this is an experimental measure which fixes the
    // following problem:  1) Start app in debug mode, 2) Open a table,
    // 3) Kill the program, 4) Run the app again, 5) Open the table 6) scroll down
    // a few thousand records.  The program locks up
    
    xcm::IObjectPtr null_result;
    execute(L"RESET QUERY CACHE", 0, null_result, NULL);


    return true;
}

drizzle_con_st* DrizzleDatabase::open()
{
    m_error.clearError();
    
    std::string asc_server = kl::tostring(m_server);
    std::string asc_database = kl::tostring(m_database);
    std::string asc_username = kl::tostring(m_username);
    std::string asc_password = kl::tostring(m_password);

    // connecting with localhost sometimes makes
    // mysql connect through a local socket like
    // /tmp/mysql.sock -- this doesn't work on 
    // servers that are not properly configured
    // for this behavior.  To correct this, we'll
    // substitute localhost with 127.0.0.1

    std::string localhost_test = asc_server;
    kl::makeLower(localhost_test);
    kl::trim(localhost_test);
    if (localhost_test == "localhost")
        asc_server = "127.0.0.1";

    try
    {   
        drizzle_con_st* con;
        drizzle_return_t ret;

        con = drizzle_con_add_tcp(&m_drizzle, NULL,
                                 asc_server.c_str(),
                                 (m_port != 0 ? m_port : 3306),
                                 asc_username.c_str(),
                                 asc_password.c_str(),
                                 asc_database.c_str(),
                                 DRIZZLE_CON_MYSQL);
        
        if (!con)
        {
            m_error.setError(tango::errorGeneral, kl::towstring(drizzle_error(&m_drizzle)));
            return NULL;
        }

        ret = drizzle_con_connect(con);
        if (ret != DRIZZLE_RETURN_OK)
        {
            drizzle_con_free(con);
            m_error.setError(tango::errorGeneral, kl::towstring(drizzle_error(&m_drizzle)));
            return NULL;
        }

        return con;
    }
    catch(...)
    {
    }
    
    return NULL;
}


std::wstring DrizzleDatabase::getServer()
{
    return m_server;
}

std::wstring DrizzleDatabase::getDatabase()
{
    return m_database;
}

    
// tango::IDatabase interface implementation

void DrizzleDatabase::close()
{
    m_db_name = L"";
    m_server = L"";
    m_port = 0;
    m_database = L"";
    m_username = L"";
    m_password = L"";
}

void DrizzleDatabase::setDatabaseName(const std::wstring& name)
{
    m_db_name = name;
}

std::wstring DrizzleDatabase::getDatabaseName()
{
    return m_db_name;
}

int DrizzleDatabase::getDatabaseType()
{
    return tango::dbtypeMySql;
}

std::wstring DrizzleDatabase::getActiveUid()
{
    return L"";
}

tango::IAttributesPtr DrizzleDatabase::getAttributes()
{
    return static_cast<tango::IAttributes*>(m_attr);
}

double DrizzleDatabase::getFreeSpace()
{
    return 0.0;
}

double DrizzleDatabase::getUsedSpace()
{
    return 0.0;
}

bool DrizzleDatabase::cleanup()
{
    return true;
}

bool DrizzleDatabase::storeObject(xcm::IObject* obj, const std::wstring& ofs_path)
{
    return false;
}

tango::IJobPtr DrizzleDatabase::createJob()
{
    return xcm::null;
}

tango::IJobPtr DrizzleDatabase::getJob(tango::jobid_t job_id)
{
    return xcm::null;
}

bool DrizzleDatabase::createFolder(const std::wstring& path)
{
    return false;
}



bool DrizzleDatabase::renameFile(const std::wstring& path, const std::wstring& new_name)
{
    std::wstring command;
    command.reserve(1024);
    command = L"RENAME TABLE ";
    command += m_attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    command += getTablenameFromOfsPath(path);
    command += m_attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);
    command += L" TO ";
    command += m_attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    command += new_name;
    command += m_attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    xcm::IObjectPtr result_obj;
    return execute(command, 0, result_obj, NULL);
}

bool DrizzleDatabase::moveFile(const std::wstring& path, const std::wstring& new_location)
{
    return false;
}

bool DrizzleDatabase::copyFile(const std::wstring& src_path, const std::wstring& dest_path)
{
    return false;
}

bool DrizzleDatabase::copyData(const tango::CopyInfo* info, tango::IJob* job)
{
    return false;
}

bool DrizzleDatabase::deleteFile(const std::wstring& path)
{
    std::wstring command;
    command.reserve(1024);
    command = L"DROP TABLE ";
    command += m_attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    command += getTablenameFromOfsPath(path);
    command += m_attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    xcm::IObjectPtr result_obj;
    return execute(command, 0, result_obj, NULL);
}

bool DrizzleDatabase::getFileExist(const std::wstring& _path)
{
    std::wstring path = kl::afterFirst(_path, L'/');

    // there may be a faster way to do this.  In order to
    // determine if the file exists, we are going to get
    // a list of all tables and look for 'path'

    tango::IFileInfoEnumPtr files = getFolderInfo(L"");
    if (!files)
        return false;

    size_t i, count = files->size();

    for (i = 0 ; i < count; ++i)
    {
        tango::IFileInfoPtr info = files->getItem(i);
        
        if (wcscasecmp(info->getName().c_str(), path.c_str()) == 0)
            return true;
    }

    return false;
}

tango::IFileInfoPtr DrizzleDatabase::getFileInfo(const std::wstring& path)
{
    std::wstring folder;
    std::wstring name;
    
    if (path.empty() || path == L"/")
    {
        DrizzleFileInfo* f = new DrizzleFileInfo(this);
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
    size_t i, count = files->size();
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

tango::IFileInfoEnumPtr DrizzleDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IFileInfoPtr>* retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;
    
    drizzle_con_st* con = open();
    if (!con)
    {
        // couldn't open db
        return retval;
    }
    
    
    drizzle_return_t ret;
    drizzle_result_st* result;
    char** row;
    
    result = drizzle_query_str(con, NULL, "SHOW TABLES", &ret);
    if (ret != DRIZZLE_RETURN_OK || result == NULL)
    {
        m_error.setError(tango::errorGeneral, kl::towstring(drizzle_error(&m_drizzle)));
        drizzle_con_free(con);
        return xcm::null;
    }
    
    ret = drizzle_result_buffer(result);
    
    while ((row = (char**)drizzle_row_next(result)) != NULL)
    {
        std::wstring wtable_name = kl::towstring(row[0]);

        DrizzleFileInfo* f = new DrizzleFileInfo(this);
        kl::trim(wtable_name);
        f->name = wtable_name;
        f->type = tango::filetypeSet;
        f->format = tango::formatNative;

        retval->append(f);
    }


    drizzle_result_free(result);
    drizzle_con_free(con);
    
    return retval;
}

std::wstring DrizzleDatabase::getPrimaryKey(const std::wstring _path)
{
    std::wstring path = kl::afterFirst(_path, L'/');
    std::wstring prikey;

    // get the quote characters
    tango::IAttributesPtr attr = getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    // create select statement
    std::wstring tablename = L"";
    tablename += quote_openchar;
    tablename += path;
    tablename += quote_closechar;

    wchar_t query[1024];
    swprintf(query, 1024, L"SELECT * FROM %ls WHERE 1=0", tablename.c_str());


    drizzle_con_st* con = open();
    if (!con)
        return L"";
    
    std::string asc_query = kl::tostring(query);
    
    
    drizzle_return_t ret;
    drizzle_result_st* result;
    char** row;
    
    result = drizzle_query_str(con, NULL, "SHOW TABLES", &ret);
    if (ret != DRIZZLE_RETURN_OK || result == NULL)
    {
        m_error.setError(tango::errorGeneral, kl::towstring(drizzle_error(&m_drizzle)));
        drizzle_con_free(con);
        return L"";
    }


    while ((row = (char**)drizzle_row_next(result)) != NULL)
    {
/*
            if (colinfo->flags & PRI_KEY_FLAG)
            {
                if (prikey.length() > 0)
                    prikey += L",";

                //prikey += quote_openchar;
                prikey += kl::towstring(colinfo->name);
                //prikey += quote_closechar;
            }
*/
    }


    drizzle_result_free(result);
    drizzle_con_free(con);

    return prikey;
}

tango::IDatabasePtr DrizzleDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool DrizzleDatabase::setMountPoint(const std::wstring& path,
                                    const std::wstring& connection_str,
                                    const std::wstring& remote_path)
{
    return false;
}
                 
bool DrizzleDatabase::getMountPoint(const std::wstring& path,
                                    std::wstring& connection_str,
                                    std::wstring& remote_path)
{
    return false;
}

tango::IStructurePtr DrizzleDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<tango::IStructure*>(s);
}

tango::ISetPtr DrizzleDatabase::createTable(const std::wstring& path,
                                          tango::IStructurePtr struct_config,
                                          tango::FormatInfo* format_info)
{
/*
    std::wstring command;
    command.reserve(2048);

    command = L"";
    command += L"CREATE TABLE `test124` (";
    command += L"field1 tinyint(10), ";
    command += L"field2 smallint(10), ";
    command += L"field3 mediumint(10), ";
    command += L"field4 int(10), ";
    command += L"field5 integer(10), ";
    command += L"field6 bigint(10), ";
    command += L"field7 real(10,4), ";
    command += L"field8 double(10,4), ";
    command += L"field9 float(10,4), ";
    command += L"field10 decimal(10,4), ";
    command += L"field11 numeric(10,4), ";
    command += L"field12 char(10), ";
    command += L"field13 varchar(10), ";
    command += L"field14 date, ";
    command += L"field15 time, ";
    command += L"field16 timestamp, ";
    command += L"field17 datetime, ";
    command += L"field18 tinyblob, ";
    command += L"field19 blob, ";
    command += L"field20 mediumblob, ";
    command += L"field21 longblob, ";
    command += L"field22 tinytext, ";
    command += L"field23 text, ";
    command += L"field24 mediumtext, ";
    command += L"field25 longtext";
    command += L")";

    execute(command, 0, NULL);

    return openSet(path);
*/
    std::wstring command;
    command.reserve(1024);

    command = L"CREATE TABLE ";
    command += m_attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    command += getTablenameFromOfsPath(path);
    command += m_attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);    
    command += L" ( ";

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
        tango::IColumnInfoPtr col_info = struct_config->getColumnInfoByIdx(i);

        name = L"";
        name += m_attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
        name += col_info->getName();
        name += m_attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

        type = col_info->getType();
        width = col_info->getWidth();
        scale = col_info->getScale();

        field = createDrizzleFieldString(name, type, width, scale, true);
        command += field;

        if (i+1 != col_count)
        {
            command += L", ";
        }
    }
    command += L" )";

    xcm::IObjectPtr result_obj;
    execute(command, 0, result_obj, NULL);

    return openSet(path);
}

tango::IStreamPtr DrizzleDatabase::openStream(const std::wstring& path)
{
    return xcm::null;
}

tango::IStreamPtr DrizzleDatabase::createStream(const std::wstring& path, const std::wstring& mime_type)
{
    return xcm::null;
}

tango::ISetPtr DrizzleDatabase::openSet(const std::wstring& path)
{
    // get a list of tables
    tango::IFileInfoEnumPtr tables = getFolderInfo(L"/");

    std::wstring tablename1 = getTablenameFromOfsPath(path);
    std::wstring tablename2;
    bool found = false;

    size_t i, table_count = tables->size();
    for (i = 0; i < table_count; ++i)
    {
        tango::IFileInfoPtr info = tables->getItem(i);
        tablename2 = info->getName();
        
        if (0 == wcscasecmp(tablename1.c_str(), tablename2.c_str()))
        {
            found = true;
            break;
        }
    }

    // check if we couldn't find the table in the database
    if (!found)
        return xcm::null;

    // open the set
    
    DrizzleSet* set = new DrizzleSet;
    set->m_database = this;
    set->m_drizzle = &m_drizzle;
    set->m_tablename = tablename1;
    
    if (!set->init())
    {
        delete set;
        return xcm::null;
    }
    
    return static_cast<tango::ISet*>(set);
}

tango::ISetPtr DrizzleDatabase::openSetEx(const std::wstring& ofs_path,
                                          int format)
{
    return openSet(ofs_path);
}


tango::IIteratorPtr DrizzleDatabase::createIterator(const std::wstring& path,
                                                    const std::wstring& columns,
                                                    const std::wstring& sort,
                                                    tango::IJob* job)
{
    tango::ISetPtr set = openSet(path);
    if (set.isNull())
        return xcm::null;
    return set->createIterator(columns, sort, job);
}


tango::IIndexInfoPtr DrizzleDatabase::createIndex(const std::wstring& path,
                                                  const std::wstring& name,
                                                  const std::wstring& expr,
                                                  tango::IJob* job)
{
    return xcm::null;
}


bool DrizzleDatabase::renameIndex(const std::wstring& path,
                                  const std::wstring& name,
                                  const std::wstring& new_name)
{
    return false;
}


bool DrizzleDatabase::deleteIndex(const std::wstring& path,
                                  const std::wstring& name)
{
    return false;
}


tango::IIndexInfoEnumPtr DrizzleDatabase::getIndexEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IIndexInfoPtr>* vec;
    vec = new xcm::IVectorImpl<tango::IIndexInfoPtr>;

    return vec;
}

tango::IRowInserterPtr DrizzleDatabase::bulkInsert(const std::wstring& path)
{
    //DrizzleRowInserter* inserter = new DrizzleRowInserter(this);
    //return static_cast<tango::IRowInserter*>(inserter);

    return xcm::null;
}


tango::IStructurePtr DrizzleDatabase::describeTable(const std::wstring& path)
{
    tango::ISetPtr set = openSet(path);
    if (set.isNull())
        return xcm::null;

    return set->getStructure();
}


bool DrizzleDatabase::modifyStructure(const std::wstring& path, tango::IStructurePtr struct_config, tango::IJob* job)
{
    return false;
}


tango::IRelationEnumPtr DrizzleDatabase::getRelationEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

tango::IRelationPtr DrizzleDatabase::getRelation(const std::wstring& relation_id)
{
    return xcm::null;
}

tango::IRelationPtr DrizzleDatabase::createRelation(const std::wstring& tag,
                                                    const std::wstring& left_set_path,
                                                    const std::wstring& right_set_path,
                                                    const std::wstring& left_expr,
                                                    const std::wstring& right_expr)
{
    return xcm::null;
}

bool DrizzleDatabase::deleteRelation(const std::wstring& relation_id)
{
    return false;
}

bool DrizzleDatabase::execute(const std::wstring& command,
                              unsigned int flags,
                              xcm::IObjectPtr& result_object,
                              tango::IJob* job)
{
    m_error.clearError();
    result_object.clear();
    
    // first find out if this statement has any select's in it
    static klregex::wregex select_regex(L"(?i)\\bSELECT\\s");
    static klregex::wregex show_regex(L"(?i)\\bSHOW\\s");
    if (select_regex.search(command) || show_regex.search(command))
    {
        // open a new connection for the command
        drizzle_con_st* data = open();
        if (!data)
        {
            m_error.setError(tango::errorGeneral);
            return xcm::null;
        }


        // sometimes there are multiple commands passed to us
        // because MySQL doesn't support multiple commands until
        // version 5.0, we have to handle this ourselves
        std::vector<std::wstring> commands;
        splitSQL(command, commands);
        

        std::vector<std::wstring>::iterator it;
        
        // remove empty commands
        int i, count = commands.size();
        for (i = 0; i < count; ++i)
        {
            kl::trim(commands[i]);
            if (commands[i].length() == 0)
            {
                commands.erase(commands.begin() + i);
                count--;
                i--;
            }
        }
        
        for (it = commands.begin(); it != commands.end(); ++it)
        {
            std::string asc_command = kl::tostring(*it);

            drizzle_return_t ret;
            drizzle_result_st* result;
            
            result = drizzle_query_str(data, NULL, asc_command.c_str(), &ret);
            if (ret != DRIZZLE_RETURN_OK || result == NULL)
            {
                m_error.setError(tango::errorGeneral, kl::towstring(drizzle_error(&m_drizzle)));
                drizzle_con_free(data);
                return xcm::null;
            }
  

            if (drizzle_result_column_count(result) > 0)
            {
                // result is a query.  We should only create a MySQLIterator
                // if this is the last statement.
                
                if (it+1 == commands.end())
                {
                    DrizzleIterator* iter = new DrizzleIterator;
                    iter->m_database = this;
                    iter->m_result = result;
                    if (iter->init(*it))
                    {
                        result_object = static_cast<xcm::IObject*>(static_cast<tango::IIterator*>(iter));
                        return true;
                    }
                     else
                    {
                        // this is the last command, the iter->m_data handle will
                        // be closed by the mysql_close() statement at the end
                        
                        iter->m_result = NULL;
                        delete iter;
                        
                        char** row;
                        while ((row = (char**)drizzle_row_next(result)) != NULL) { }
                        drizzle_result_free(result);
                    }
                }
                 else
                {
                    char** row;
                    while ((row = (char**)drizzle_row_next(result)) != NULL) { }
                    drizzle_result_free(result);
                }
            }
        }

        drizzle_con_free(data);

        return false;
    }
     else
    {
    
        // open a new connection for the command
        drizzle_con_st* con = open();
        if (!con)
        {
            m_error.setError(tango::errorGeneral);
            return xcm::null;
        }

    
        // sometimes there are multiple commands passed to us
        // because MySQL doesn't support multiple commands until
        // version 5.0, we have to handle this ourselves
        std::vector<std::wstring> commands;
        splitSQL(command, commands);
        
        std::vector<std::wstring>::iterator it;
        for (it = commands.begin(); it != commands.end(); ++it)
        {
            kl::trim(*it);
            std::string asc_command = kl::tostring(*it);
            if (asc_command.length() == 0)
                continue;
                
            drizzle_return_t ret;
            drizzle_result_st* result;
            char** row;
            
            result = drizzle_query_str(con, NULL, asc_command.c_str(), &ret);
            if (ret != DRIZZLE_RETURN_OK || result == NULL)
            {
                m_error.setError(tango::errorGeneral, kl::towstring(drizzle_error(&m_drizzle)));
                drizzle_con_free(con);
                return false;
            }

            while ((row = (char**)drizzle_row_next(result)) != NULL) { }
            drizzle_result_free(result);
        }

        drizzle_con_free(con);

        return true;
    }
}

std::wstring DrizzleDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int DrizzleDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void DrizzleDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code, error_string);
}


