/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-04-16
 *
 */


#ifdef WIN32
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#endif


#include "mysql.h"


#include <kl/xcm.h>
#include <kl/string.h>
#include <kl/portable.h>
#include <kl/regex.h>
#include <kl/md5.h>
#include <xd/xd.h>
#include "../xdcommon/structure.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/util.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "database.h"
#include "iterator.h"
#include "inserter.h"



#define FOLDER_SEPARATOR L"__"



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


// -- utility functions--

int mysql2xdType(int mysql_type)
{
    switch (mysql_type)
    {
        case FIELD_TYPE_TINY:
        case FIELD_TYPE_SHORT:
        case FIELD_TYPE_LONG:
        case FIELD_TYPE_INT24:
        case FIELD_TYPE_YEAR:
            return xd::typeInteger;
        case FIELD_TYPE_LONGLONG:
        case FIELD_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            return xd::typeNumeric;
        case FIELD_TYPE_FLOAT:
        case FIELD_TYPE_DOUBLE:
            return xd::typeDouble;
        case FIELD_TYPE_DATE:
            return xd::typeDate;
        case FIELD_TYPE_TIME:
        case FIELD_TYPE_TIMESTAMP:
        case FIELD_TYPE_DATETIME:
            return xd::typeDateTime;
        case FIELD_TYPE_STRING:
        case FIELD_TYPE_VAR_STRING:
        case FIELD_TYPE_BLOB:
        case FIELD_TYPE_SET:
        case FIELD_TYPE_ENUM:
            return xd::typeCharacter;
        case FIELD_TYPE_NULL:
        default:
            return xd::typeInvalid;
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


std::wstring createMySqlFieldString(const std::wstring& name, int type, int width, int scale, bool null)
{
    wchar_t buf[255];

    if (width < 1)
    {
        width = 1;
    }

    switch (type)
    {
        case xd::typeCharacter:
        {
            swprintf(buf, 255, L"%ls varchar (%d)%ls", name.c_str(), width, null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeWideCharacter:
        {
            swprintf(buf, 255, L"%ls varchar (%d)%ls", name.c_str(), width, null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeNumeric:
        {
            if (width > 28)
            {
                width = 28;
            }

            swprintf(buf, 255, L"%ls decimal (%d,%d)%ls", name.c_str(), width, scale, null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeInteger:
        {
            swprintf(buf, 255, L"%ls int%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeDouble:
        {
            swprintf(buf, 255, L"%ls float%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeDate:
        {
            swprintf(buf, 255, L"%ls date%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }

        case xd::typeDateTime:
        {
            swprintf(buf, 255, L"%ls datetime%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }
        
        case xd::typeBoolean:
        {
            swprintf(buf, 255, L"%ls tinyint%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }        
    }

    return L"";
}


std::wstring mysqlGetTablenameFromPath(const std::wstring& path)
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

std::wstring mysqlQuoteIdentifier(const std::wstring& str)
{
    std::wstring res;

    res = str;
    kl::replaceStr(res, L"\"", L"");
    kl::replaceStr(res, L"/*", L"");
    kl::replaceStr(res, L"-", L"");
    kl::makeLower(res);

    return L"`" + res + L"`";
}

std::wstring mysqlQuoteIdentifierIfNecessary(const std::wstring& str)
{
    std::wstring res;

    res = str;
    kl::replaceStr(res, L"\"", L"");
    kl::replaceStr(res, L"/*", L"");
    kl::replaceStr(res, L"-", L"");
    kl::makeLower(res);

    if (res.find(' ') == str.npos)
        return res;

    return L"`" + res + L"`";
}




// -- MySqlFileInfo class implementation --


class MySqlFileInfo : public xdcommon::FileInfo
{
public:

    MySqlFileInfo(MysqlDatabase* db) : xdcommon::FileInfo()
    {
        m_db = db;
        m_db->ref();
    }
    
    ~MySqlFileInfo()
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

    MysqlDatabase* m_db;
};



MysqlDatabase::MysqlDatabase()
{
    m_data = NULL;
    m_res = NULL;

    m_db_name = L"";
    m_server = L"";
    m_port = 0;
    m_database = L"";
    m_username = L"";
    m_password = L"";

    // illegal characters in a table name include \/. and characters illegal
    // in filenames, the superset of which includes: \/:*?<>|
    
    m_attr = static_cast<xd::IAttributes*>(new DatabaseAttributes);
    m_attr->setIntAttribute(xd::dbattrColumnMaxNameLength, 64);
    m_attr->setIntAttribute(xd::dbattrTableMaxNameLength, 64);
    m_attr->setStringAttribute(xd::dbattrKeywords, kws);    
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

MysqlDatabase::~MysqlDatabase()
{
    close();
}

bool MysqlDatabase::open(const std::wstring& server,
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
    
    m_data = open();
    if (!m_data)
        return false;
    
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

MYSQL* MysqlDatabase::open()
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
        MYSQL* mysql;

        mysql = mysql_init((MYSQL*)0);
        if (!mysql)
        {
            return NULL;
        }
        
        if (mysql_real_connect(mysql,
                               asc_server.c_str(),
                               asc_username.c_str(),
                               asc_password.c_str(),
                               asc_database.c_str(),
                               m_port,
                               NULL,
                               0))
        {
            if (mysql_select_db(mysql, asc_database.c_str()) < 0)
            {
                m_error.setError(xd::errorGeneral, kl::towstring(mysql_error(mysql)));
                mysql_close(mysql);
                return NULL;
            }
        }
         else
        {
            m_error.setError(xd::errorGeneral, kl::towstring(mysql_error(mysql)));
            mysql_close(mysql);
            return NULL;
        }
        
        return mysql;
    }
    catch(...)
    {
    }
    
    return NULL;
}

MYSQL* MysqlDatabase::getMySqlPtr()
{
    return m_data;
}

std::wstring MysqlDatabase::getServer()
{
    return m_server;
}

std::wstring MysqlDatabase::getDatabase()
{
    return m_database;
}


void MysqlDatabase::close()
{
    // clean up

    if (m_data)
    {
        mysql_close(m_data);
    }

    m_data = NULL;
    m_res = NULL;

    m_db_name = L"";
    m_server = L"";
    m_port = 0;
    m_database = L"";
    m_username = L"";
    m_password = L"";
}

void MysqlDatabase::setDatabaseName(const std::wstring& name)
{
    m_db_name = name;
}

std::wstring MysqlDatabase::getDatabaseName()
{
    return m_db_name;
}

int MysqlDatabase::getDatabaseType()
{
    return xd::dbtypeMySql;
}

std::wstring MysqlDatabase::getActiveUid()
{
    return L"";
}

xd::IAttributesPtr MysqlDatabase::getAttributes()
{
    return static_cast<xd::IAttributes*>(m_attr);
}

double MysqlDatabase::getFreeSpace()
{
    return 0.0;
}

double MysqlDatabase::getUsedSpace()
{
    return 0.0;
}

bool MysqlDatabase::cleanup()
{
    return true;
}

bool MysqlDatabase::storeObject(xcm::IObject* obj, const std::wstring& ofs_path)
{
    return false;
}

xd::IJobPtr MysqlDatabase::createJob()
{
    return xcm::null;
}

xd::IJobPtr MysqlDatabase::getJob(xd::jobid_t job_id)
{
    return xcm::null;
}

bool MysqlDatabase::createFolder(const std::wstring& path)
{
    return false;
}

bool MysqlDatabase::renameFile(const std::wstring& path, const std::wstring& new_name)
{
    std::wstring command;
    command.reserve(1024);
    command = L"RENAME TABLE ";
    command += m_attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    command += getTablenameFromOfsPath(path);
    command += m_attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);
    command += L" TO ";
    command += m_attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    command += new_name;
    command += m_attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

    xcm::IObjectPtr result_obj;
    return execute(command, 0, result_obj, NULL);
}

bool MysqlDatabase::moveFile(const std::wstring& path, const std::wstring& new_location)
{
    return false;
}

bool MysqlDatabase::copyFile(const std::wstring& src_path, const std::wstring& dest_path)
{
    return false;
}

bool MysqlDatabase::copyData(const xd::CopyParams* info, xd::IJob* job)
{
    return false;
}

bool MysqlDatabase::deleteFile(const std::wstring& path)
{
    std::wstring command;
    command.reserve(1024);
    command = L"DROP TABLE ";
    command += m_attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    command += getTablenameFromOfsPath(path);
    command += m_attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

    xcm::IObjectPtr result_obj;
    return execute(command, 0, result_obj, NULL);
}

bool MysqlDatabase::getFileExist(const std::wstring& _path)
{
    std::wstring path = kl::afterFirst(_path, L'/');

    // there may be a faster way to do this.  In order to
    // determine if the file exists, we are going to get
    // a list of all tables and look for 'path'

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

xd::IFileInfoPtr MysqlDatabase::getFileInfo(const std::wstring& path)
{
    std::wstring folder;
    std::wstring name;
    
    if (path.empty() || path == L"/")
    {
        MySqlFileInfo* f = new MySqlFileInfo(this);
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

xd::IFileInfoEnumPtr MysqlDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IFileInfoPtr>* retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;
    
    MYSQL* db = open();
    if (!db)
    {
        // couldn't open db
        return retval;
    }
    
    int res  = mysql_query(db, "SHOW TABLES");

    if (res != 0)
    {
        mysql_close(db);
        return retval;
    }
    
    m_res = mysql_use_result(db);

    MYSQL_ROW row_info;
    while ((row_info = mysql_fetch_row(m_res)))
    {
        std::wstring wtable_name = kl::towstring(*row_info);

        MySqlFileInfo* f = new MySqlFileInfo(this);
        kl::trim(wtable_name);
        f->name = wtable_name;
        f->type = xd::filetypeTable;
        f->format = xd::formatDefault;

        retval->append(f);
    }

    mysql_free_result(m_res);
    mysql_close(db);
    
    return retval;
}

std::wstring MysqlDatabase::getPrimaryKey(const std::wstring _path)
{
    std::wstring path = kl::afterFirst(_path, L'/');
    std::wstring result;

    // get the quote characters
    xd::IAttributesPtr attr = getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

    // create select statement
    std::wstring tablename = L"";
    tablename += quote_openchar;
    tablename += path;
    tablename += quote_closechar;

    wchar_t query[1024];
    swprintf(query, 1024, L"SELECT * FROM %ls WHERE 1=0", tablename.c_str());


    MYSQL* db = open();
    if (!db)
        return L"";
    
    std::string asc_query = kl::tostring(query);
    int error = mysql_query(db, asc_query.c_str());
    
    if (!error)
    {
        MYSQL_RES* res = mysql_use_result(db);

        MYSQL_FIELD* colinfo;
        while((colinfo = mysql_fetch_field(res)))
        {
            if (colinfo->flags & PRI_KEY_FLAG)
            {
                if (result.length() > 0)
                    result += L",";

                //result += quote_openchar;
                result += kl::towstring(colinfo->name);
                //result += quote_closechar;
            }
        }

        mysql_free_result(res);
    }

    mysql_close(db);
    
    return result;
}

xd::IDatabasePtr MysqlDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool MysqlDatabase::setMountPoint(const std::wstring& path,
                                  const std::wstring& connection_str,
                                  const std::wstring& remote_path)
{
    return false;
}
                 
bool MysqlDatabase::getMountPoint(const std::wstring& path,
                                  std::wstring& connection_str,
                                  std::wstring& remote_path)
{
    return false;
}

xd::IStructurePtr MysqlDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<xd::IStructure*>(s);
}

bool MysqlDatabase::createTable(const std::wstring& path, const xd::FormatDefinition& format_definition)
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
    command += m_attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    command += getTablenameFromOfsPath(path);
    command += m_attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);    
    command += L" ( ";

    std::wstring field;
    std::wstring name;

    std::vector<xd::ColumnInfo>::const_iterator it;
    for (it = format_definition.columns.cbegin(); it != format_definition.columns.cend(); ++it)
    {
        const xd::ColumnInfo& colinfo = *it;

        name = m_attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
        name += colinfo.name;
        name += m_attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

        field = createMySqlFieldString(name, colinfo.type, colinfo.width, colinfo.scale, true);
        command += field;

        if (it+1 != format_definition.columns.cend())
            command += L", ";
    }
    command += L" )";

    xcm::IObjectPtr result_obj;
    return execute(command, 0, result_obj, NULL);
}

xd::IStreamPtr MysqlDatabase::openStream(const std::wstring& path)
{
    return xcm::null;
}

bool MysqlDatabase::createStream(const std::wstring& path, const std::wstring& mime_type)
{
    return false;
}


xd::IIteratorPtr MysqlDatabase::query(const xd::QueryParams& qp)
{
    std::wstring tbl = mysqlGetTablenameFromPath(qp.from);

    std::wstring query;

    query = L"SELECT * FROM ";
    query += mysqlQuoteIdentifierIfNecessary(tbl);

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
    MysqlIterator* iter = new MysqlIterator(this);

    if (!iter->init(query))
    {
        delete iter;
        return xcm::null;
    }

    if (qp.where.length() == 0)
    {
        // we know the exact table name; this can be used to retrieve
        // better metrics, like row count; pass this table name on to
        // the iterator object
        iter->setMySqlTable(mysqlGetTablenameFromPath(qp.from));

        iter->setTable(qp.from);
    }

    return static_cast<xd::IIterator*>(iter);

}


xd::IndexInfo MysqlDatabase::createIndex(const std::wstring& path,
                                         const std::wstring& name,
                                         const std::wstring& expr,
                                         xd::IJob* job)
{
    return xd::IndexInfo();
}


bool MysqlDatabase::renameIndex(const std::wstring& path,
                                const std::wstring& name,
                                const std::wstring& new_name)
{
    return false;
}


bool MysqlDatabase::deleteIndex(const std::wstring& path,
                                const std::wstring& name)
{
    return false;
}

xd::IndexInfoEnum MysqlDatabase::getIndexEnum(const std::wstring& path)
{
    return xd::IndexInfoEnum();
}


xd::IRowInserterPtr MysqlDatabase::bulkInsert(const std::wstring& path)
{
    std::wstring tbl = mysqlGetTablenameFromPath(path);
    MysqlRowInserter* inserter = new MysqlRowInserter(this, tbl);
    return static_cast<xd::IRowInserter*>(inserter);
}

xd::Structure MysqlDatabase::describeTable(const std::wstring& path)
{
    // create select statement
    std::wstring tablename = L"";
    tablename += L"`";
    tablename += mysqlGetTablenameFromPath(path);
    tablename += L"`";

    wchar_t query[1024];
    swprintf(query, 1024, L"SELECT * FROM %ls WHERE 1=0", tablename.c_str());


    MYSQL* db = open();
    if (!db)
        return xd::Structure();
    
    std::string asc_query = kl::tostring(query);
    int error = mysql_query(db, asc_query.c_str());
    

    xd::Structure s;

    if (!error)
    {
        MYSQL_RES* res = mysql_use_result(db);

        int i = 0;
        MYSQL_FIELD* colinfo;
        while ((colinfo = mysql_fetch_field(res)))
        {
            int type = mysql2xdType(colinfo->type);

            std::wstring wcol_name = kl::towstring(colinfo->name);

            xd::ColumnInfo col;
            col.name = wcol_name;
            col.type = type;
            col.width = colinfo->length;
            col.scale = type == xd::typeDouble ? 4 : colinfo->decimals;
            col.column_ordinal = i;
            
            // limit blob/text fields to 4096 characters (for now);
            // this seems to be sensible behavior because copies of
            // the table will not clog of the database space-wise
            if (colinfo->type == FIELD_TYPE_BLOB && colinfo->length > 4096)
                col.width = 4096;
  
            s.createColumn(col);
            
            i++;
        }

        mysql_free_result(res);
    }

    mysql_close(db);
    
    return s;
}


bool MysqlDatabase::modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job)
{
    return false;
}


/*

xd::ISetPtr MysqlDatabase::openSet(const std::wstring& path)
{
    // get a list of tables
    xd::IFileInfoEnumPtr tables = getFolderInfo(L"/");

    std::wstring tablename1 = getTablenameFromOfsPath(path);
    std::wstring tablename2;
    bool found = false;

    size_t i, table_count = tables->size();
    for (i = 0; i < table_count; ++i)
    {
        xd::IFileInfoPtr info = tables->getItem(i);
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
    
    MySqlSet* set = new MySqlSet;
    set->m_database = this;
    set->m_tablename = tablename1;
    
    if (!set->init())
    {
        delete set;
        return xcm::null;
    }
    
    return static_cast<xd::ISet*>(set);
}
*/
                   

bool MysqlDatabase::execute(const std::wstring& command,
                            unsigned int flags,
                            xcm::IObjectPtr& result,
                            xd::IJob* job)
{
    m_error.clearError();
    result.clear();
    
    // first find out if this statement has any select's in it
    static klregex::wregex select_regex(L"(?i)\\bSELECT\\s");
    static klregex::wregex show_regex(L"(?i)\\bSHOW\\s");
    if (select_regex.search(command) || show_regex.search(command))
    {
        // open a new connection for the command
        MYSQL* data = open();
        if (!data)
        {
            m_error.setError(xd::errorGeneral);
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
                
            int error = mysql_query(data, asc_command.c_str());
            if (error)
            {
                m_error.setError(xd::errorGeneral, kl::towstring(mysql_error(data)));
                mysql_close(data);
                return xcm::null;
            }
            
            if (mysql_field_count(data))
            {
                // result is a query.  We should only create a MySQLIterator
                // if this is the last statement.
                
                if (it+1 == commands.end())
                {
                    MysqlIterator* iter = new MysqlIterator(this);
                    iter->m_database = this;
                    iter->m_data = data;
                    if (iter->init(*it))
                    {
                        result = static_cast<xcm::IObject*>(static_cast<xd::IIterator*>(iter));
                        return true;
                    }
                     else
                    {
                        // this is the last command, the iter->m_data handle will
                        // be closed by the mysql_close() statement at the end
                        
                        iter->m_data = NULL;
                        
                        if (!m_error.isError() && mysql_errno(iter->m_data))
                        {
                            m_error.setError(xd::errorGeneral);
                        }

                        delete iter;
                    }                
                }
                 else
                {
                    MYSQL_RES* res = mysql_store_result(data);
                    if (res)
                    {
                        mysql_free_result(res);
                    }
                }
            }
        }

        mysql_close(data);

        return false;
    }
     else
    {
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
                
            int error = mysql_query(m_data, asc_command.c_str());

            if (error)
            {
                m_error.setError(xd::errorGeneral, kl::towstring(mysql_error(m_data)));
                return false;
            }
            
            MYSQL_RES* res = mysql_store_result(m_data);
            if (res)
            {
                // a query type command, that must be read and freed
                // to maintain a good state with the server
                mysql_free_result(res);
            }
        }

        return true;
    }
}

bool MysqlDatabase::groupQuery(xd::GroupQueryParams* info, xd::IJob* job)
{
    return false;
}


std::wstring MysqlDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int MysqlDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void MysqlDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code, error_string);
}


