/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2005-01-10
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <sqlite3.h>

#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/columninfo.h"
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


SlDatabase::SlDatabase()
{
    m_db = NULL;
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

bool SlDatabase::createDatabase(const std::wstring& path,
                                const std::wstring& db_name)
{
    m_db = NULL;
    m_path = L"";

    if (xf_get_file_exist(path))
    {
        // -- database already exists; overwrite it --
        if (!xf_remove(path))
        {
            return false;
        }
    }


    std::string ascpath = kl::tostring(path);
    sqlite3* db = NULL;

    if (SQLITE_OK != sqlite3_open(ascpath.c_str(), &db))
    {
        // -- database could not be opened --
        return false;
    }


    // -- create catalog table --

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

    // -- create columns table --

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


    // -- insert entry for root folder --

    const char* sql3 = "INSERT INTO catalog "
                          "(id, folder_id, name, type, obj_name) "
                          "VALUES (1, 0, '/', 'FOLDER', '');";

    if (SQLITE_OK != sqlite3_exec(db, sql3, NULL, NULL, NULL))
    {
        sqlite3_close(db);
        xf_remove(path);
        return false;
    }


    m_db = db;
    m_path = path;

    return true;
}


bool SlDatabase::openDatabase(int type,
                              const std::wstring& server,
                              int port,
                              const std::wstring& database,
                              const std::wstring& username,
                              const std::wstring& password,
                              const std::wstring& path)
{
    if (!xf_get_file_exist(path))
    {
        // -- database does not exist --
        return false;
    }


    std::string ascpath = kl::tostring(path);
    sqlite3* db = NULL;

    if (SQLITE_OK != sqlite3_open(ascpath.c_str(), &db))
    {
        // -- database could not be opened --
        return false;
    }

    m_db = db;
    m_path = path;

    return true;
}

// -- tango::IDatabase interface implementation --

void SlDatabase::close()
{
    if (m_db)
    {
        sqlite3_close(m_db);
    }
}

void SlDatabase::setDatabaseName(const std::wstring& name)
{
    m_db_name = name;
}

std::wstring SlDatabase::getDatabaseName()
{
    return m_db_name;
}

std::wstring SlDatabase::getActiveUid()
{
    return L"";
}

tango::IDatabaseAttributesPtr SlDatabase::getAttributes()
{
    DatabaseAttributes* attr = new DatabaseAttributes;

    std::wstring kws = sql92_keywords;

    attr->setIntAttribute(tango::dbattrColumnMaxNameLength, 64);
    attr->setIntAttribute(tango::dbattrTableMaxNameLength, 64);
    attr->setStringAttribute(tango::dbattrColumnInvalidChars, L"./\":!#&-`*[]");
    attr->setStringAttribute(tango::dbattrTableInvalidChars, L"./\":!#&-`*[]");
    attr->setStringAttribute(tango::dbattrColumnInvalidStartingChars, L" ");
    attr->setStringAttribute(tango::dbattrTableInvalidStartingChars, L" ");

    attr->setKeywords(kws);
    attr->setIntAttribute(tango::dbattrTableMaxNameLength, 32);
    return static_cast<tango::IDatabaseAttributes*>(attr);
}

double SlDatabase::getFreeSpace()
{
    return 0.0;
}

double SlDatabase::getUsedSpace()
{
    return 0.0;
}


std::wstring SlDatabase::getErrorString()
{
    return L"";
}

int SlDatabase::getErrorCode()
{
    return tango::errorNone;
}


bool SlDatabase::cleanup()
{
    return true;
}


bool SlDatabase::storeObject(xcm::IObject* _obj,
                             const std::wstring& ofs_path)
{
    // -- get the IStorable interface --
    tango::IStorablePtr obj = _obj;
    if (!obj)
    {
        return false;
    }

    // -- check path validity --

    if (ofs_path.length() == 0)
    {
        return false;
    }

    if (iswspace(ofs_path[0]))
    {
        return false;
    }


    // -- remove old object stored at 'ofs_path' --

    if (wcscasecmp(obj->getObjectPath().c_str(), ofs_path.c_str()) != 0)
    {
        if (getFileExist(ofs_path))
        {
            deleteFile(ofs_path);
        }
    }

    // -- store the object --

    return obj->storeObject(ofs_path);
}

tango::IJobPtr SlDatabase::createJob()
{
    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<IJobInternal*>(job);
}

tango::IJobPtr SlDatabase::getJob(tango::jobid_t job_id)
{
    std::vector<JobInfo*>::iterator it;
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
    {
        if ((*it)->getJobId() == job_id)
        {
            return static_cast<tango::IJob*>(*it);
        }
    }

    return xcm::null;
}

bool SlDatabase::createFolder(const std::wstring& path)
{
    int id;
    id = createCatalogEntry(path, tango::ofstypeFolder, L"");
    if (!id)
        return false;
    return true;
}

tango::IOfsValuePtr SlDatabase::createFile(const std::wstring& path)
{
    return xcm::null;
}

tango::IOfsValuePtr SlDatabase::openFile(const std::wstring& path)
{
    return xcm::null;
}

bool SlDatabase::renameFile(const std::wstring& path,
                            const std::wstring& new_name)
{
    int ordinal = getCatalogEntry(path, NULL, NULL, NULL);
    if (!ordinal)
        return false;

    return updateCatalogEntry(ordinal,
                              -1,
                              -1,
                              new_name.c_str(),
                              NULL);
}

bool SlDatabase::moveFile(const std::wstring& path,
                          const std::wstring& new_location)
{
    int ordinal = getCatalogEntry(path, NULL, NULL, NULL);
    if (!ordinal)
        return false;

    std::wstring new_name = kl::afterLast(new_location, L'/');
    std::wstring new_folder = kl::beforeLast(new_location, L'/');
    if (new_folder.empty())
        new_folder = L"/";


    int new_folder_id = getCatalogEntry(new_folder, NULL, NULL, NULL);
    if (!new_folder_id)
        return false;

    return updateCatalogEntry(ordinal,
                              new_folder_id,
                              -1,
                              new_name.c_str(),
                              NULL);
}

bool SlDatabase::copyFile(const std::wstring& src_path,
                          const std::wstring& dest_path)
{
    return false;
}

bool SlDatabase::deleteFile(const std::wstring& path)
{
    int ordinal, ofs_type;
    std::wstring obj_name;

    if (!getCatalogEntry(path, &ordinal, &ofs_type, &obj_name))
    {
        return false;
    }

    if (ofs_type == tango::ofstypeSet)
    {
        std::wstring command;
        command.reserve(1024);
        command = L"DROP TABLE ";
        command += obj_name;

        if (!execute(command, NULL))
        {
            return false;
        }
    }


    // -- delete catalog entry --

    if (!deleteCatalogEntry(ordinal))
    {
        return false;
    }


    return true;
}

bool SlDatabase::getFileExist(const std::wstring& _path)
{
    return (getCatalogEntry(_path, NULL, NULL, NULL) != 0) ? true : false;
}




int SlDatabase::getCatalogEntry(const std::wstring& _path,
                                int* _cat_id,
                                int* _ofstype,
                                std::wstring* _obj_name)
{
    std::wstring path;

    // -- make sure the path starts with a slash --
    if (_path.empty() || _path[0] != L'/')
        path = L"/";
    path += _path;

    // -- root folder always has an id of 1 --
    if (path == L"/")
        return 1;
    
    std::vector<std::wstring> parts;
    std::vector<std::wstring>::iterator it;
    parseDelimitedList(path, parts, L'/', true);

    // -- start at the root --
    std::string type;
    std::string obj_name;
    int cat_id = 1;
    int rc;

    for (it = parts.begin(); it != parts.end(); ++it)
    {
        if (it->empty())
            continue;

        wchar_t sql[1024];
        swprintf(sql,
               1024,
               L"SELECT id, type, obj_name FROM catalog WHERE folder_id=%d AND name='%s'",
               cat_id,
               it->c_str());

        std::string ascsql = kl::tostring(sql);

        int rows = 0;
        char** result;

        rc = sqlite3_get_table(m_db, ascsql.c_str(), &result, &rows, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            return 0;
        }       

        if (rows == 0)
        {
            // -- not found --
            sqlite3_free_table(result);    
            return 0;
        }

        cat_id = atoi(result[3]);
        type = result[4];
        obj_name = result[5];


        sqlite3_free_table(result);    
    }

    if (_cat_id)
    {
        *_cat_id = cat_id;
    }

    if (_ofstype)
    {
        if (type == "FOLDER")
            *_ofstype = tango::ofstypeFolder;
         else if (type == "SET")
            *_ofstype = tango::ofstypeSet;
         else
            *_ofstype = tango::ofstypeGeneric;
    }

    if (_obj_name)
    {
        *_obj_name = kl::towstring(obj_name);
    }

    return cat_id;
}


// -- begin a transaction --
//sqlite3_exec(m_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
// -- commit this transaction --
//sqlite3_exec(m_db, "COMMIT;", NULL, NULL, NULL);

int SlDatabase::createCatalogEntry(const std::wstring& _path,
                                   int ofstype,
                                   const std::wstring& obj_name)
{
    std::wstring path;

    // -- make sure the path starts with a slash --
    if (_path.empty() || _path[0] != L'/')
        path = L"/";
    path += _path;


    if (path == L"/")
    {
        // -- root already exists --
        return 0;
    }

    std::vector<std::wstring> parts;
    std::vector<std::wstring>::iterator it;
    parseDelimitedList(path, parts, L'/', true);


    // -- start at the root --
    int cat_id = 1;
    int rc;

    std::string ascsql;
    std::wstring type;

    for (it = parts.begin(); it != parts.end(); ++it)
    {
        if (it->empty())
            continue;

        wchar_t sql[1024];
        swprintf(sql,
               1024,
               L"SELECT id FROM catalog WHERE folder_id=%d AND name='%s'",
               cat_id,
               it->c_str());

        ascsql = kl::tostring(sql);


        int rows = 0;
        char** result;

        rc = sqlite3_get_table(m_db, ascsql.c_str(), &result, &rows, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            return 0;
        }       

        if (rows == 0)
        {
            type = L"FOLDER";
            
            if (it+1 == parts.end())
            {
                switch (ofstype)
                {
                    default:
                    case tango::ofstypeGeneric: type = L"GENERIC"; break;
                    case tango::ofstypeFolder:  type = L"FOLDER"; break;
                    case tango::ofstypeSet:     type = L"SET"; break;
                }
            }
             else
            {
                type = L"FOLDER";
            }


            swprintf(sql,
                   1024,
                   L"INSERT INTO catalog (id, folder_id, name, type, obj_name) VALUES (NULL, %d, '%ls', '%ls', '%ls')",
                   cat_id,
                   it->c_str(),
                   type.c_str(),
                   obj_name.c_str());

            ascsql = kl::tostring(sql);

            sqlite3_exec(m_db, ascsql.c_str(), NULL, NULL, NULL);

            cat_id = sqlite3_last_insert_rowid(m_db);
        }
         else
        {
            cat_id = atoi(result[1]);
        }

        sqlite3_free_table(result);    
    }


    return cat_id;
}


bool SlDatabase::updateCatalogEntry(int ordinal,
                                    int folder_id,
                                    int ofstype,
                                    const wchar_t* name,
                                    const wchar_t* obj_name)
{
    std::wstring sql, params;
    sql = L"UPDATE catalog SET ";
    
    if (folder_id != -1)
    {
        wchar_t temps[255];
        swprintf(temps, 255, L"folder_id=%d", folder_id);
        params += temps;
    }

    if (ofstype != -1)
    {
        std::wstring type;
        switch (ofstype)
        {
            default:
            case tango::ofstypeGeneric: type = L"GENERIC"; break;
            case tango::ofstypeFolder:  type = L"FOLDER"; break;
            case tango::ofstypeSet:     type = L"SET"; break;
        }

        if (!params.empty())
        {
            params += L",";
        }

        wchar_t temps[255];
        swprintf(temps, 255, L"type='%ls'", type.c_str());
        params += temps;
    }

    if (name)
    {
        if (!params.empty())
        {
            params += L",";
        }

        wchar_t temps[255];
        swprintf(temps, 255, L"name='%ls'", name);
        params += temps;
    }

    if (obj_name)
    {
        if (!params.empty())
        {
            params += L",";
        }

        wchar_t temps[255];
        swprintf(temps, 255, L"obj_name='%ls'", obj_name);
        params += temps;
    }

    sql += params;

    wchar_t temps[255];
    swprintf(temps, 255, L" WHERE id=%d;", ordinal);
    sql += temps;


    std::string ascsql = kl::tostring(sql);

    if (SQLITE_OK != sqlite3_exec(m_db, ascsql.c_str(), NULL, NULL, NULL))
        return false;

    return true;
}


bool SlDatabase::deleteCatalogEntry(int ordinal)
{
    wchar_t sql[1024];
    swprintf(sql,
           1024,
           L"DELETE FROM catalog WHERE id=%d;",
           ordinal);

    std::string ascsql = kl::tostring(sql);

    if (SQLITE_OK != sqlite3_exec(m_db, ascsql.c_str(), NULL, NULL, NULL))
        return false;

    return true;
}

std::wstring SlDatabase::getCatalogPath(int ordinal)
{
    std::wstring result;
    std::wstring name;
    int rc;

    while (ordinal != 1)
    {
        wchar_t sql[1024];
        swprintf(sql,
               1024,
               L"SELECT folder_id, name FROM catalog WHERE id=%d",
               ordinal);

        std::string ascsql = kl::tostring(sql);

        int rows = 0;
        char** tbl;

        rc = sqlite3_get_table(m_db, ascsql.c_str(), &tbl, &rows, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            return 0;
        }       

        if (rows == 0)
        {
            // -- not found --
            sqlite3_free_table(tbl);    
            return L"";
        }

        ordinal = atoi(tbl[2]);
        name = kl::towstring(tbl[3]);

        if (result.empty())
        {
            result = name;
        }
         else
        {
            std::wstring temps;
            temps = name;
            temps += L"/";
            temps += result;
            result = temps;
        }

        sqlite3_free_table(tbl);
    }

    std::wstring temps;
    temps = L"/";
    temps += result;

    return temps;
}

tango::IFileInfoPtr SlDatabase::getFileInfo(const std::wstring& path)
{
    return xcm::null;
}

tango::IFileInfoEnumPtr SlDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;

    int rc = 0;
    
    char** result;
    int rows = 0;
    int cat_id = 0;


    if (path == L"/" || path.empty())
    {
        // -- get root folder (which has an id of 1) --
        cat_id = 1;
    }
     else
    {
        cat_id = getCatalogEntry(path, NULL, NULL, NULL);
    }

    if (cat_id == 0)
    {
        // -- folder does not exist --
        return retval;
    }


    wchar_t sql[1024];
    
    
    swprintf(sql,
             1024,
             L"SELECT id, folder_id, name, type FROM catalog WHERE folder_id=%d",
             cat_id);

    std::string ascsql = kl::tostring(sql);

    rc = sqlite3_get_table(m_db, ascsql.c_str(), &result, &rows, NULL, NULL);

    if (rc != SQLITE_OK)
    {
        return retval;
    }


    int i;
    for (i = 1; i <= rows; ++i)
    {
        const char* s_id = result[(i*4)+0];
        const char* s_folder_id = result[(i*4)+1];
        const char* s_name = result[(i*4)+2];
        const char* s_type = result[(i*4)+3];

        std::wstring name = kl::towstring(s_name);

        FileInfo* f = new FileInfo;
        f->name = kl::towstring(s_name);

        if (0 == strcmp(s_type, "FOLDER"))
        {
            f->type = tango::ofstypeFolder;
        }
         else
        {
            f->type = tango::ofstypeSet;
        }
        
        retval->append(f);
    }

    sqlite3_free_table(result);

    return retval;
}

tango::IStructurePtr SlDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<tango::IStructure*>(s);
}


tango::IStructurePtr SlDatabase::getStructureFromOrdinal(int ordinal)
{
    wchar_t sql[1024];
    swprintf(sql,
           1024,
           L"SELECT name, type, width, scale, expr FROM columns WHERE obj_id=%d",
           ordinal);

    std::string ascsql = kl::tostring(sql);

    int rc;
    int rows = 0;
    char** result;

    rc = sqlite3_get_table(m_db, ascsql.c_str(), &result, &rows, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        return xcm::null;
    }       

    if (rows == 0)
    {
        // -- not found --
        sqlite3_free_table(result);    
        return xcm::null;
    }

    Structure* s = new Structure;

    int i;
    for (i = 1; i <= rows; ++i)
    {
        std::wstring name;
        int type;
        int width;
        int scale;
        std::wstring expr;

        name = kl::towstring(result[(i*5)+0]);
        type = atoi(result[(i*5)+1]);
        width = atoi(result[(i*5)+2]);
        scale = atoi(result[(i*5)+3]);
        expr = kl::towstring(result[(i*5)+4]);


        ColumnInfo* colinfo = new ColumnInfo;
        colinfo->setName(name);
        colinfo->setType(type);
        colinfo->setWidth(width);
        colinfo->setScale(scale);
        colinfo->setExpression(expr);

        if (expr.length() > 0)
            colinfo->setCalculated(true);
        
        s->addColumn(static_cast<tango::IColumnInfo*>(colinfo));
    }

    sqlite3_free_table(result);

    return static_cast<tango::IStructure*>(s);
}



tango::ISetPtr SlDatabase::createSet(const std::wstring& _path,
                                     tango::IStructurePtr struct_config,
                                     int format)
{
    std::wstring path;

    // -- ofs path --
    if (_path.empty())
    {
        path = L"/.temp/";
        path += getUniqueString();
    }
     else
    {
        path = _path;
    }

    // -- generate table name, SQL CREATE statment, and execute --

    std::wstring obj_name;
    obj_name = getUniqueString();

    std::wstring sql;
    sql = L"CREATE TABLE ";
    sql += obj_name;
    sql += L" (";
    
    int col_count, i;
    
    col_count = struct_config->getColumnCount();

    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo;
        colinfo = struct_config->getColumnInfoByIdx(i);
        
        std::wstring piece;
        piece += colinfo->getName();
        piece += L" ";

        std::wstring type;
        switch (colinfo->getType())
        {
            case tango::typeCharacter:
            case tango::typeWideCharacter:
                type = L"TEXT";
                break;
            case tango::typeInteger:
                type = L"INTEGER";
                break;
            case tango::typeDouble:
            case tango::typeNumeric:
                type = L"REAL";
                break;
            case tango::typeDate:
            case tango::typeDateTime:
            case tango::typeBoolean:
                type = L"TEXT";
                break;
            case tango::typeBinary:
                type = L"BLOB";
                break;
        }

        piece += type;
        if (i+1 < col_count)
            piece += L", ";

        sql += piece;
    }
    sql += L");";

    std::string ascsql = kl::tostring(sql);

    if (SQLITE_OK != sqlite3_exec(m_db, ascsql.c_str(), NULL, NULL, NULL))
    {
        return xcm::null;
    }


    // -- create entry in the catalog --
    int cat_id = createCatalogEntry(path, tango::ofstypeSet, obj_name);


    // -- add table schema to the 'columns' table --

    sql.reserve(10000);
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo;
        colinfo = struct_config->getColumnInfoByIdx(i);

        sql = L"INSERT INTO columns "
                 L"(obj_id, name, type, width, scale, expr) VALUES (";

        wchar_t piece[1024];
        swprintf(piece,
                 1024,
                 L"%d, '%ls', %d, %d, %d, '%ls'",
                 cat_id,
                 colinfo->getName().c_str(),
                 colinfo->getType(),
                 colinfo->getWidth(),
                 colinfo->getScale(),
                 colinfo->getExpression().c_str());

        sql += piece;
        sql += L");";

        ascsql = kl::tostring(sql);

        if (SQLITE_OK != sqlite3_exec(m_db, ascsql.c_str(), NULL, NULL, NULL))
        {
            return xcm::null;
        }
    }


    return openSet(path);
}

tango::ISetPtr SlDatabase::createDynamicSet(tango::ISetPtr base_set)
{
    return xcm::null;
}

tango::ISetPtr SlDatabase::createFilterSet(tango::ISetPtr base_set,
                                           const std::wstring& condition)
{
    return xcm::null;
}

tango::ISetPtr SlDatabase::createBookmarkSet(tango::rowid_t rowid)
{
    return xcm::null;
}

tango::IUnionPtr SlDatabase::createUnionSet()
{
    return xcm::null;
}

tango::ISetPtr SlDatabase::openSet(const std::wstring& path)
{
    std::wstring tablename = kl::afterFirst(path, L'/');
    

    std::wstring obj_name;
    int type;
    int id = getCatalogEntry(path, NULL, &type, &obj_name);

    if (id == 0)
        return xcm::null;
    if (type != tango::ofstypeSet)
        return xcm::null;

    SlSet* set = new SlSet;

    set->m_database = static_cast<tango::IDatabase*>(this);
    set->m_dbint = this;
    set->m_tablename = obj_name;
    set->m_ordinal = id;
    set->m_db = m_db;

    return static_cast<tango::ISet*>(set);
}

tango::IRelationEnumPtr SlDatabase::getRelationEnum()
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

tango::IIteratorPtr SlDatabase::runQuery(const std::wstring& command,
                                         tango::IJob* job)
{
    SlIterator* iter = new SlIterator;

    iter->m_db = m_db;
    iter->m_database = static_cast<tango::IDatabase*>(this);
    iter->m_set = xcm::null;

    if (!iter->init(command))
        return xcm::null;

    return static_cast<tango::IIterator*>(iter);
}

bool SlDatabase::execute(const std::wstring& command,
                         tango::IJob* job)
{
    std::string ascsql;

    ascsql = kl::tostring(command);

    if (SQLITE_OK != sqlite3_exec(m_db, ascsql.c_str(), NULL, NULL, NULL))
    {
        return false;
    }

    return true;
}

tango::ISetPtr SlDatabase::runGroupQuery(tango::ISetPtr set,
                                         const std::wstring& group,
                                         const std::wstring& output,
                                         const std::wstring& filter,
                                         tango::IJob* job)
{
    return xcm::null;
}


