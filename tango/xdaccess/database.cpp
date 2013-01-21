/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2005-01-10
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "set.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/fileinfo.h"
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

int access2tangoType(int access_type)
{
    switch (access_type)
    {
        case MDB_BOOL:      return tango::typeBoolean;
        case MDB_BYTE:      return tango::typeInteger;
        case MDB_INT:       return tango::typeInteger;
        case MDB_LONGINT:   return tango::typeInteger;
        case MDB_MONEY:     return tango::typeNumeric;
        case MDB_FLOAT:     return tango::typeNumeric;
        case MDB_DOUBLE:    return tango::typeNumeric;
        case MDB_SDATETIME: return tango::typeDateTime;
        case MDB_TEXT:      return tango::typeWideCharacter;
        case MDB_OLE:       return tango::typeInvalid;
        case MDB_MEMO:      return tango::typeWideCharacter;
        case MDB_REPID:     return tango::typeInvalid;
        case MDB_NUMERIC:   return tango::typeNumeric;
        default:
            return tango::typeInvalid;
    };
}


MdbTableDef* findTableInCatalog(MdbHandle* mdb, gchar* table_name)
{
    MdbCatalogEntry* entry;

    mdb_read_catalog(mdb, MDB_TABLE);

    for (unsigned int i=0; i < mdb->num_catalog; i++)
    {
        entry = (MdbCatalogEntry*)g_ptr_array_index(mdb->catalog, i);

        if (!strcasecmp(entry->object_name, table_name))
            return mdb_read_table(entry);
    }

    return NULL;
}


AccessDatabase::AccessDatabase()
{
    m_mdb = NULL;
    mdb_init();
    mdb_set_date_fmt("%Y.%m.%d.%H.%M.%S.");
}

AccessDatabase::~AccessDatabase()
{
    close();
    mdb_exit();
}

bool AccessDatabase::open(int type,
                          const std::wstring& server,
                          int port,
                          const std::wstring& database,
                          const std::wstring& username,
                          const std::wstring& password,
                          const std::wstring& path)
{
    m_path = path;
    std::string ascpath = kl::tostring(path);

    m_mdb = mdb_open((char*)ascpath.c_str(), MDB_NOFLAGS);
    if (m_mdb == NULL)
        return false;
    
    return true;
}

// -- tango::IDatabase interface implementation --

void AccessDatabase::close()
{
    if (m_mdb)
    {
        mdb_close(m_mdb);
    }
}

void AccessDatabase::setDatabaseName(const std::wstring& name)
{
    m_db_name = name;
}

std::wstring AccessDatabase::getDatabaseName()
{
    return m_db_name;
}

int AccessDatabase::getDatabaseType()
{
    return tango::dbtypeAccess;
}

std::wstring AccessDatabase::getActiveUid()
{
    return L"";
}

tango::IAttributesPtr AccessDatabase::getAttributes()
{
    DatabaseAttributes* attr = new DatabaseAttributes;

    std::wstring kws = sql92_keywords;
    kws += L",";
    kws += access_keywords;

    attr->setIntAttribute(tango::dbattrColumnMaxNameLength, 64);
    attr->setIntAttribute(tango::dbattrTableMaxNameLength, 64);
    attr->setStringAttribute(tango::dbattrColumnInvalidChars, L"./\":!#&-`*[]");
    attr->setStringAttribute(tango::dbattrTableInvalidChars, L"./\":!#&-`*[]");
    attr->setStringAttribute(tango::dbattrColumnInvalidStartingChars, L" ");
    attr->setStringAttribute(tango::dbattrTableInvalidStartingChars, L" ");
    attr->setStringAttribute(tango::dbattrKeywords, kws);

    attr->setIntAttribute(tango::dbattrTableMaxNameLength, 32);
    return static_cast<tango::IAttributes*>(attr);
}

double AccessDatabase::getFreeSpace()
{
    return 0.0;
}

double AccessDatabase::getUsedSpace()
{
    return 0.0;
}


std::wstring AccessDatabase::getErrorString()
{
    return L"";
}

int AccessDatabase::getErrorCode()
{
    return tango::errorNone;
}

void AccessDatabase::setError(int error_code, const std::wstring& error_string)
{
}


bool AccessDatabase::cleanup()
{
    return true;
}


bool AccessDatabase::storeObject(xcm::IObject* obj,
                                 const std::wstring& ofs_path)
{
    return false;
}

tango::IJobPtr AccessDatabase::createJob()
{
    return xcm::null;
}

tango::IJobPtr AccessDatabase::getJob(tango::jobid_t job_id)
{
    return xcm::null;
}
tango::IDatabasePtr AccessDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool AccessDatabase::setMountPoint(const std::wstring& path,
                                 const std::wstring& connection_str,
                                 const std::wstring& remote_path)
{
    return false;
}

bool AccessDatabase::getMountPoint(const std::wstring& path,
                                 std::wstring& connection_str,
                                 std::wstring& remote_path)
{
    return false;
}

bool AccessDatabase::createFolder(const std::wstring& path)
{
    return false;
}

tango::IStreamPtr AccessDatabase::createStream(const std::wstring& path, const std::wstring& mime_type)
{
    return xcm::null;
}

tango::INodeValuePtr AccessDatabase::createNodeFile(const std::wstring& path)
{
    return xcm::null;
}

tango::IStreamPtr AccessDatabase::openStream(const std::wstring& path)
{
    return xcm::null;
}

tango::INodeValuePtr AccessDatabase::openNodeFile(const std::wstring& path)
{
    return xcm::null;
}

bool AccessDatabase::renameFile(const std::wstring& path,
                                const std::wstring& new_name)
{
    return false;
}

bool AccessDatabase::moveFile(const std::wstring& path,
                              const std::wstring& new_location)
{
    return false;
}

bool AccessDatabase::copyFile(const std::wstring& src_path,
                              const std::wstring& dest_path)
{
    return false;
}

bool AccessDatabase::deleteFile(const std::wstring& path)
{
    std::wstring command;
    command.reserve(1024);
    command = L"DROP TABLE ";
    command += getTablenameFromOfsPath(path);

    xcm::IObjectPtr result;

    execute(command, 0, result, NULL);

    return true;
}

bool AccessDatabase::getFileExist(const std::wstring& _path)
{
    std::wstring path = kl::afterFirst(_path, L'/');

    // -- there may be a faster way to do this.  In order to
    //    determine if the file exists, we are going to get
    //    a list of all tables and look for 'path' --

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

tango::IFileInfoPtr AccessDatabase::getFileInfo(const std::wstring& path)
{
    return xcm::null;
}






static bool mdb_is_system_table(MdbCatalogEntry *entry)
{
    //return ((entry->object_type == MDB_TABLE) && (entry->flags & 0x80000002)) ? true : false;
    //return (entry->object_type != MDB_TABLE) ? true : false;
    if (entry->object_name && (0 == strncmp(entry->object_name, "MSys", 4)))
    	return true;
    return false;
}


tango::IFileInfoEnumPtr AccessDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;

    MdbCatalogEntry *entry;
    
    if (!mdb_read_catalog(m_mdb, MDB_ANY))
        return xcm::null;
            
    int i;
    for (i = 0; i < m_mdb->num_catalog; i++)
    {
        entry = (MdbCatalogEntry*)g_ptr_array_index(m_mdb->catalog, i);

        if (entry->object_type != MDB_TABLE)
            continue;

        if (mdb_is_system_table(entry))
            continue;

        xdcommon::FileInfo* f = new xdcommon::FileInfo;
        f->name = kl::towstring(entry->object_name);
        f->type = tango::filetypeSet;
        
        retval->append(f);
    }
    
    return retval;
}

tango::IStructurePtr AccessDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<tango::IStructure*>(s);
}

tango::ISetPtr AccessDatabase::createSet(const std::wstring& path,
                                         tango::IStructurePtr struct_config,
                                         int format)
{
    return openSet(path);
}

tango::ISetPtr AccessDatabase::createDynamicSet(tango::ISetPtr base_set)
{
    return xcm::null;
}

tango::ISetPtr AccessDatabase::createFilterSet(tango::ISetPtr base_set,
                                               const std::wstring& condition)
{
    return xcm::null;
}



tango::ISetPtr AccessDatabase::openSet(const std::wstring& path)
{
    std::wstring tablename = kl::afterFirst(path, L'/');
    
    AccessSet* set = new AccessSet;
    
    set->m_mdb = m_mdb;
    set->m_path = m_path;
    set->m_tablename = tablename;
    set->m_database = static_cast<tango::IDatabase*>(this);
    
    if (!set->init())
    {
        delete set;
    }
    
    return static_cast<tango::ISet*>(set);
}

tango::ISetPtr AccessDatabase::openSetEx(const std::wstring& path,
                                         int format)
{
    return openSet(path);
}

tango::IRelationEnumPtr AccessDatabase::getRelationEnum()
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

tango::IIteratorPtr AccessDatabase::runQuery(const std::wstring& command,
                                             unsigned int flags,
                                             tango::IJob* job)
{
    return xcm::null;
}

bool AccessDatabase::execute(const std::wstring& command,
                             unsigned int flags,
                             xcm::IObjectPtr& result,
                             tango::IJob* job)
{
    return true;
}

tango::ISetPtr AccessDatabase::runGroupQuery(tango::ISetPtr set,
                                             const std::wstring& group,
                                             const std::wstring& output,
                                             const std::wstring& where,
                                             const std::wstring& having,
                                             tango::IJob* job)
{
    return xcm::null;
}




