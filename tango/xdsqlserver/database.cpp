/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-03-17
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <ctpublic.h>
#include <tds.h>
#include <tdsconvert.h>
#include <tdsstring.h>


#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "iterator.h"
#include "set.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"


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


// utility function to create a valid SQL Server field string

std::wstring createSqlServerFieldString(const std::wstring& name,
                                        int type,
                                        int width,
                                        int scale,
                                        bool null)
{
    wchar_t buf[255];

    if (width < 1)
    {
        width = 1;
    }

    switch (type)
    {
        case tango::typeCharacter:
        case tango::typeWideCharacter:
        {
            swprintf(buf, 255, L"%ls varchar (%d)%ls", name.c_str(), width, null ? L" NULL" : L"");
            return buf;
        }

        case tango::typeDouble:
            width = 18;
            // fall through

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

        // export doubles as decimal fields to preserve the
        // decimal places.  (our native double fields round
        // to scale, making them unlike other dbms doubles)

        //case tango::typeDouble:
        //{
        //    swprintf(buf, 255, L"%ls float%ls", name.c_str(), null ? L" NULL" : L"");
        //    return buf;
        //}

        case tango::typeBoolean:
        {
            swprintf(buf, 255, L"%ls bit%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }

        case tango::typeDate:
        case tango::typeDateTime:
        {
            swprintf(buf, 255, L"%ls datetime%ls", name.c_str(), null ? L" NULL" : L"");
            return buf;
        }
    }

    return L"";
}




SqlServerDatabase::SqlServerDatabase()
{
    m_login = NULL;
    m_context = NULL;
    m_connect_info = NULL;

    m_db_name = L"";
    m_server = L"";
    m_port = 0;
    m_database = L"";
    m_username = L"";
    m_password = L"";
    
    std::wstring kws;
    kws += sqlserver_keywords;
    kws += L",";
    kws += sqlserver_keywords2;

    m_attr = new DatabaseAttributes;
    m_attr->setStringAttribute(tango::dbattrKeywords, kws);
    m_attr->setStringAttribute(tango::dbattrColumnInvalidStartingChars, L" ");
    m_attr->setStringAttribute(tango::dbattrTableInvalidStartingChars, L" ");
    m_attr->setIntAttribute(tango::dbattrTableMaxNameLength, 80);
    m_attr->setIntAttribute(tango::dbattrColumnMaxNameLength, 80);
}

SqlServerDatabase::~SqlServerDatabase()
{
    close();
}

bool SqlServerDatabase::open(const std::wstring& server,
                             int port,
                             const std::wstring& database,
                             const std::wstring& username,
                             const std::wstring& password)
{
    try
    {
        // start the windows socket
#ifdef WIN32
        WSADATA wsadata;
        WSAStartup(MAKEWORD(2,0), &wsadata);
#endif

        // allocate context
        m_context = tds_alloc_context(NULL);

        // set the date format
        m_context->locale->date_fmt = strdup("%m/%d/%Y");

        // allocate socket
        TDSSOCKET* tds;
        tds = tds_alloc_socket(m_context, 512);
        tds_set_parent(tds, NULL);

        std::string asc_server = kl::tostring(server);
        std::string asc_database = kl::tostring(database);
        std::string asc_username = kl::tostring(username);
        std::string asc_password = kl::tostring(password);

        // allocate login info
        m_login = tds_alloc_login();
        tds_set_user(m_login, asc_username.c_str());
        tds_set_passwd(m_login, asc_password.c_str());
        tds_set_app(m_login, "login");
        tds_set_host(m_login, "mypc");
        tds_set_library(m_login, "TDS-Library");
        tds_set_client_charset(m_login, "iso_1");
        tds_set_language(m_login, "us_english");
        tds_set_server(m_login, asc_server.c_str());
        tds_set_packet(m_login, 512);
        tds_set_version(m_login, 7, 0);

        // try to connect
        m_connect_info = tds_read_config_info(NULL, m_login, m_context->locale);

        // set the ip address and database name in the connection info
        char server_addr[80];

        tds_lookup_host(asc_server.c_str(),
                        server_addr);

        m_connect_info->ip_addr = (DSTR)strdup(server_addr);
        m_connect_info->database = (DSTR)strdup(asc_database.c_str());
        m_connect_info->port = port;

        // attempt to connect to the sql server
        if (!m_connect_info || tds_connect(tds, m_connect_info) == TDS_FAIL)
        {
            return false;
        }

        tds_free_socket(tds);

        m_server = server;
        m_port = port;
        m_database = database;
        m_username = username;
        m_password = password;

        wchar_t buf[1024];
        swprintf(buf, 1024, L"SQL Server (%ls)", server.c_str());
        m_attr->setStringAttribute(tango::dbattrDatabaseName, buf);

        return true;
    }
    catch(...)
    {
        return false;
    }
}

// tango::IDatabase interface implementation

void SqlServerDatabase::close()
{
    // clean up
    tds_free_connection(m_connect_info);
    tds_free_login(m_login);
    tds_free_context(m_context);

    m_login = NULL;
    m_context = NULL;
    m_connect_info = NULL;

    m_db_name = L"";
    m_server = L"";
    m_port = 0;
    m_database = L"";
    m_username = L"";
    m_password = L"";
}


int SqlServerDatabase::getDatabaseType()
{
    return tango::dbtypeSqlServer;
}

std::wstring SqlServerDatabase::getActiveUid()
{
    return L"";
}

tango::IAttributesPtr SqlServerDatabase::getAttributes()
{
    return static_cast<tango::IAttributes*>(m_attr);
}


std::wstring SqlServerDatabase::getErrorString()
{
    return L"";
}

int SqlServerDatabase::getErrorCode()
{
    return tango::errorNone;
}


void SqlServerDatabase::setError(int error_code, const std::wstring& error_string)
{
}


bool SqlServerDatabase::cleanup()
{
    return true;
}

tango::IJobPtr SqlServerDatabase::createJob()
{
    return xcm::null;
}

tango::IDatabasePtr SqlServerDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool SqlServerDatabase::setMountPoint(const std::wstring& path,
                                      const std::wstring& connection_str,
                                      const std::wstring& remote_path)
{
    return false;
}
                              
bool SqlServerDatabase::getMountPoint(const std::wstring& path,
                                      std::wstring& connection_str,
                                      std::wstring& remote_path)
{
    return false;
}

bool SqlServerDatabase::createFolder(const std::wstring& path)
{
    return false;
}

bool SqlServerDatabase::renameFile(const std::wstring& path,
                                   const std::wstring& new_name)
{
    std::wstring command;
    command.reserve(1024);
    command = L"RENAME TABLE [";
    command += getTablenameFromOfsPath(path);
    command += L"] TO [";
    command += getTablenameFromOfsPath(new_name);
    command += L"]";

    xcm::IObjectPtr result;
    execute(command, 0, result, NULL);

    return true;
}

bool SqlServerDatabase::moveFile(const std::wstring& path,
                                 const std::wstring& new_location)
{
    return false;
}

bool SqlServerDatabase::copyFile(const std::wstring& src_path,
                                 const std::wstring& dest_path)
{
    return false;
}

bool SqlServerDatabase::copyData(const tango::CopyInfo* info, tango::IJob* job)
{
    return false;
}

bool SqlServerDatabase::deleteFile(const std::wstring& path)
{
    std::wstring command;
    command.reserve(1024);
    command = L"DROP TABLE [";
    command += getTablenameFromOfsPath(path);
    command += L"]";

    xcm::IObjectPtr result;
    execute(command, 0, result, NULL);

    return true;
}

bool SqlServerDatabase::getFileExist(const std::wstring& _path)
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
        {
            return true;
        }
    }

    return false;
}

tango::IFileInfoPtr SqlServerDatabase::getFileInfo(const std::wstring& path)
{
    return xcm::null;
}

tango::IFileInfoEnumPtr SqlServerDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IFileInfoPtr>* retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;

    TDS_INT table_res;
    TDS_INT row_res;
    TDS_INT compute_id;
    int res_type;
    std::wstring table_name;


    // allocate socket
    TDSSOCKET* tds;
    tds = tds_alloc_socket(m_context, 512);
    tds_set_parent(tds, NULL);


    // attempt to connect to the sql server
    if (!m_connect_info || tds_connect(tds, m_connect_info) == TDS_FAIL)
    {
        tds_free_connection(m_connect_info);
        fprintf(stderr, "There was a problem connecting to the server\n");
        return retval;
    }


    if (tds_submit_query(tds, "sp_tables") == TDS_FAIL)
    {
        return retval;
    }

    while ((table_res = tds_process_tokens(tds, &res_type, NULL, TDS_HANDLE_ALL)) == TDS_SUCCEED)
    {
        while ((row_res = tds_process_tokens(tds, &res_type, &compute_id, TDS_HANDLE_ALL)) == TDS_SUCCEED)
        {
            int i;
            for (i = 0; i < tds->res_info->num_cols; i++)
            {
                unsigned char* src;
                TDS_INT src_len;

                src = tds->res_info->current_row + tds->res_info->columns[i]->column_offset;
                src_len = tds->res_info->columns[i]->column_cur_size;

                std::string coldata;
                coldata.assign((char*)src, src_len);

                // get the table name

                if (!strcasecmp(tds->res_info->columns[i]->column_name, "TABLE_NAME"))
                {
                    table_name = kl::towstring(coldata);
                    kl::trim(table_name);
                }


                // only add the actual tables, not views, system tables, etc.

                if (!strcasecmp(tds->res_info->columns[i]->column_name, "TABLE_TYPE") &&
                    !strcasecmp(coldata.c_str(), "TABLE"))
                {
                    if (!wcscasecmp(table_name.c_str(), L"dtproperties"))
                    {
                        continue;
                    }

                    xdcommon::FileInfo* f = new xdcommon::FileInfo;
                    f->name = table_name;
                    f->type = tango::filetypeTable;
                    f->format = tango::formatNative;

                    retval->append(f);
                }
            }
        }
    }

    tds_free_socket(tds);
    return retval;
}

tango::IStructurePtr SqlServerDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<tango::IStructure*>(s);
}

bool SqlServerDatabase::createTable(const std::wstring& path,
                                    tango::IStructurePtr struct_config,
                                    tango::FormatInfo* format_info)
{
    std::wstring command;
    command.reserve(1024);

    const wchar_t* open_bracket_char = L"[";
    const wchar_t* close_bracket_char = L"]";

    command = L"CREATE TABLE ";
    command += open_bracket_char;
    command += getTablenameFromOfsPath(path);
    command += close_bracket_char;
    command += L" ( ";

    std::wstring field;
    field.reserve(255);

    std::wstring name;
    int type;
    int width;
    int scale;
    
    int i, col_count = struct_config->getColumnCount();

    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr col_info = struct_config->getColumnInfoByIdx(i);
        
        name = col_info->getName();
        type = col_info->getType();
        width = col_info->getWidth();
        scale = col_info->getScale();

        field = createSqlServerFieldString(name, type, width, scale, true);
        command += field;

        if (i+1 != col_count)
        {
            command += L", ";
        }
    }
    command += L" )";

    xcm::IObjectPtr result;
    return execute(command, 0, result, NULL);
}

tango::IStreamPtr SqlServerDatabase::openStream(const std::wstring& ofs_path)
{
    return xcm::null;
}

bool SqlServerDatabase::createStream(const std::wstring& ofs_path, const std::wstring& mime_type)
{
    return false;
}


tango::IIteratorPtr SqlServerDatabase::query(const tango::QueryParams& qp)
{
    // TODO: implement
    return xcm::null;
}




tango::IIndexInfoPtr SqlServerDatabase::createIndex(const std::wstring& path,
                                                    const std::wstring& name,
                                                    const std::wstring& expr,
                                                    tango::IJob* job)
{
    return xcm::null;
}


bool SqlServerDatabase::renameIndex(const std::wstring& path,
                                    const std::wstring& name,
                                    const std::wstring& new_name)
{
    return false;
}


bool SqlServerDatabase::deleteIndex(const std::wstring& path,
                                    const std::wstring& name)
{
    return false;
}


tango::IIndexInfoEnumPtr SqlServerDatabase::getIndexEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IIndexInfoPtr>* vec;
    vec = new xcm::IVectorImpl<tango::IIndexInfoPtr>;

    return vec;
}

tango::IRowInserterPtr SqlServerDatabase::bulkInsert(const std::wstring& path)
{
    //SqlServerRowInserter* inserter = new SqlServerRowInserter(this);
    //return static_cast<tango::IRowInserter*>(inserter);

    return xcm::null;
}

tango::IStructurePtr SqlServerDatabase::describeTable(const std::wstring& path)
{
    // TODO: implement
    return xcm::null;
}

bool SqlServerDatabase::modifyStructure(const std::wstring& path, tango::IStructurePtr struct_config, tango::IJob* job)
{
    return false;
}


bool SqlServerDatabase::execute(const std::wstring& command,
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

    if (!wcscasecmp(first_word.c_str(), L"SELECT"))
    {
        // create an iterator based on our select statement
        SqlServerIterator* iter = new SqlServerIterator;
        iter->m_connect_info = m_connect_info;
        iter->m_context = m_context;
        iter->m_database = this;
        iter->m_set = xcm::null;
        iter->m_name = command;

        if (!iter->init(command))
        {
            delete iter;
            return false;
        }
        
        result = static_cast<xcm::IObject*>(iter);
        return true;
    }
     else
    {
        // allocate socket
        TDSSOCKET* tds;
        tds = tds_alloc_socket(m_context, 512);
        tds_set_parent(tds, NULL);

        // attempt to connect to the sql server
        if (!m_connect_info || tds_connect(tds, m_connect_info) == TDS_FAIL)
        {
            // there was a problem connecting to the server
            tds_free_connection(m_connect_info);
            return false;
        }

        // submit the query to the sql server
        std::string asc_command = kl::tostring(command);
        if (tds_submit_query(tds, asc_command.c_str()) != TDS_SUCCEED)
        {
            tds_free_socket(tds);
            return false;
        }

        int result_type;
        while (tds_process_tokens(tds, &result_type, NULL, TDS_HANDLE_ALL) == TDS_SUCCEED)
        {
        }

        tds_free_socket(tds);
        return true;
    }
}


