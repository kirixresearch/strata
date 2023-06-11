/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#include <xd/xd.h>
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/dbfuncs.h"
#include "database.h"
#include "iterator.h"
#include "inserter.h"
#include <set>
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/md5.h>

#define XLNT_STATIC
#include <xlnt/xlnt.hpp>


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



// ExcelDatabase class implementation

ExcelDatabase::ExcelDatabase()
{
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

    m_wb = new xlnt::workbook;

    m_last_job = 0;
}

ExcelDatabase::~ExcelDatabase()
{
    close();

    delete m_wb;
}


std::wstring ExcelDatabase::getPath()
{
    return m_path;
}


std::wstring ExcelDatabase::getTempFileDirectory()
{
    std::wstring result = m_attr->getStringAttribute(xd::dbattrTempDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}

std::wstring ExcelDatabase::getDefinitionDirectory()
{
    std::wstring result = m_attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}




bool ExcelDatabase::open(const std::wstring& path)
{
    m_error.clearError();
    
    m_path = path;

    delete m_wb;
    m_wb = new xlnt::workbook;

    try
    {
        m_wb->load(path);
    }
    catch (xlnt::exception& e)
    {
        m_error.setError(xd::errorGeneral, kl::towstring(e.what()));
        return false;
    }
    
    //m_kpg = new PkgFile;
    //if (!m_kpg->open(path))
    //{
    //    delete m_kpg;
    //    m_kpg = NULL;
    //    return false;
    //}

    return true;
}


void ExcelDatabase::close()
{
    m_path = L"";
}




void ExcelDatabase::setDatabaseName(const std::wstring& name)
{
    m_db_name = name;
}

std::wstring ExcelDatabase::getDatabaseName()
{
    return m_db_name;
}

std::wstring ExcelDatabase::getActiveUid()
{
    return L"";
}

xd::IAttributesPtr ExcelDatabase::getAttributes()
{
    return m_attr;
}

std::wstring ExcelDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int ExcelDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void ExcelDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code, error_string);
}


bool ExcelDatabase::cleanup()
{
    return true;
}

xd::IJobPtr ExcelDatabase::createJob()
{
    KL_AUTO_LOCK(m_jobs_mutex);

    JobInfo* job = new JobInfo;
    job->setJobId(++m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<xd::IJob*>(job);
}

xd::IDatabasePtr ExcelDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool ExcelDatabase::setMountPoint(const std::wstring& path,
                                  const std::wstring& connection_str,
                                  const std::wstring& remote_path)
{
    return false;
}
                              
bool ExcelDatabase::getMountPoint(const std::wstring& path,
                                  std::wstring& connection_str,
                                  std::wstring& remote_path)
{
    return false;
}

bool ExcelDatabase::createFolder(const std::wstring& path)
{
    return false;
}

bool ExcelDatabase::renameFile(const std::wstring& _path,
                             const std::wstring& new_name)
{
    std::wstring path = _path;
    if (path.empty())
        return false;
    if (path[0] == '/')
        path.erase(0,1);

    std::wstring new_path;
    if (path.find(L'/') == path.npos)
        new_path = new_name;
         else
        new_path = kl::beforeLast(path, '/') + L"/" + new_name;

    return false;
}

bool ExcelDatabase::moveFile(const std::wstring& path,
                           const std::wstring& new_location)
{
    return false;
}

bool ExcelDatabase::copyFile(const std::wstring& src_path,
                           const std::wstring& dest_path)
{
    return false;
}

bool ExcelDatabase::copyData(const xd::CopyParams* info, xd::IJob* job)
{
    xd::IIteratorPtr iter;
    xd::Structure structure;

    if (info->iter_input.isOk())
    {
        iter = info->iter_input;
        structure = iter->getStructure();
    }
     else
    {
        xd::QueryParams qp;
        qp.from = info->input;
        qp.where = info->where;
        qp.order = info->order;

        iter = query(qp);
        if (iter.isNull())
            return false;

        structure = iter->getStructure();
        if (structure.isNull())
            return false;
    }


    
    if (!info->append)
    {
        deleteFile(info->output);

        xd::FormatDefinition fd;
        fd.columns = structure;

        if (!createTable(info->output, fd))
            return false;
    }


    xdcmnInsert(static_cast<xd::IDatabase*>(this), iter, info->output, info->copy_columns, info->where, info->limit, job);

    return true;
}

bool ExcelDatabase::deleteFile(const std::wstring& _path)
{
    std::wstring path = _path;
    if (path.empty())
        return false;
    if (path[0] == '/')
        path.erase(0,1);

    return false;
}

bool ExcelDatabase::getFileExist(const std::wstring& _path)
{
    std::wstring path = _path;
    if (path.empty())
        return false;
    if (path[0] == '/')
        path.erase(0,1);

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
        if (kl::iequals(info->getName(), path))
            return true;
    }

    return false;
}

xd::IFileInfoPtr ExcelDatabase::getFileInfo(const std::wstring& path)
{
    std::wstring folder;
    std::wstring name;
    
    if (path.empty() || path == L"/")
    {
        xdcommon::FileInfo* f = new xdcommon::FileInfo();
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

    size_t i, count = files->size();
    for (i = 0; i < count; ++i)
    {
        xd::IFileInfoPtr finfo = files->getItem(i);
        if (kl::iequals(finfo->getName(), name))
        {
            return finfo;
        }
    }
    
    return xcm::null;
}





// ExcelFileInfo class implementation


class ExcelFileInfo : public xdcommon::FileInfo
{
public:

    ExcelFileInfo(ExcelDatabase* db) : xdcommon::FileInfo()
    {
        m_db = db;
        m_db->ref();
    }

    ~ExcelFileInfo()
    {
        m_db->unref();
    }

    xd::rowpos_t getRowCount()
    {
        try
        {
            xlnt::worksheet ws = m_db->m_wb->sheet_by_title((const char*)kl::toUtf8(this->getName()));
            return ws.highest_row_or_props();
        }
        catch (...)
        {
            return 0;
        }
    }

private:

    ExcelDatabase* m_db;
};





xd::IFileInfoEnumPtr ExcelDatabase::getFolderInfo(const std::wstring& path)
{
    KL_AUTO_LOCK(m_obj_mutex);

    std::wstring lower_path = m_path;
    kl::makeLower(lower_path);

    xcm::IVectorImpl<xd::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;

    size_t i, sheet_count = m_wb->sheet_count();

    for (i = 0; i < sheet_count; ++i)
    {
        xlnt::worksheet sheet = (*m_wb)[i];
        std::string title = sheet.title();

        ExcelFileInfo* f = new ExcelFileInfo(this);
        f->name = kl::fromUtf8(title.c_str());
        f->type = xd::filetypeTable;
        f->object_id = kl::md5str(L"xdexcel:" + lower_path + L":" + f->name);
        f->flags = xd::sfFastRowCount;

        retval->append(static_cast<xd::IFileInfo*>(f));
    }

    return retval;
}

std::wstring ExcelDatabase::getPrimaryKey(const std::wstring& path)
{
    return L"";
}

bool ExcelDatabase::createTable(const std::wstring& _path,
                                const xd::FormatDefinition& format_definition)
{
    KL_AUTO_LOCK(m_obj_mutex);

    if (getFileExist(_path))
    {
        // object already exists
        return false;
    }

    std::wstring path = _path;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    if (path.length() == 0)
        return false;

    m_create_tables[path] = format_definition;

    return true;
}

xd::IStreamPtr ExcelDatabase::openStream(const std::wstring& _path)
{
    std::wstring mime_type;

    std::wstring path = _path;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    if (!getFileExist(path))
    {
        std::map<std::wstring, std::wstring /*mime type*/, kl::cmp_nocase>::iterator it;
        it = m_create_streams.find(path);
        if (it == m_create_streams.end())
            return xcm::null;
        mime_type = it->second;
    }


    //KpgStream* pstream = new KpgStream(this);
    //if (!pstream->init(path, mime_type))
    //{
    //    delete pstream;
    //   return xcm::null;
    //}
    //
    //return static_cast<xd::IStream*>(pstream);
    return xcm::null;

}

bool ExcelDatabase::createStream(const std::wstring& _path, const std::wstring& mime_type)
{
    KL_AUTO_LOCK(m_obj_mutex);

    if (getFileExist(_path))
    {
        // object already exists
        return false;
    }

    std::wstring path = _path;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    if (path.length() == 0)
        return false;

    m_create_streams[path] = mime_type;

    return true;
}


bool ExcelDatabase::getStreamInfoBlock(const std::wstring& _path, std::wstring& output)
{
    return false;
}


xd::IIteratorPtr ExcelDatabase::query(const xd::QueryParams& qp)
{
    KL_AUTO_LOCK(m_obj_mutex);

    std::wstring path = qp.from;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    // create an iterator based on our select statement
    ExcelIterator* iter = new ExcelIterator(this);

    if (!iter->init(path))
    {
        delete iter;
        return xcm::null;
    }

    return static_cast<xd::IIterator*>(iter);
}



xd::IndexInfo ExcelDatabase::createIndex(const std::wstring& path,
                                       const std::wstring& name,
                                       const std::wstring& expr,
                                       xd::IJob* job)
{
    return xd::IndexInfo();
}


bool ExcelDatabase::renameIndex(const std::wstring& path,
                              const std::wstring& name,
                              const std::wstring& new_name)
{
    return false;
}


bool ExcelDatabase::deleteIndex(const std::wstring& path,
                              const std::wstring& name)
{
    return false;
}


xd::IndexInfoEnum ExcelDatabase::getIndexEnum(const std::wstring& path)
{
    return xd::IndexInfoEnum();
}


xd::IRowInserterPtr ExcelDatabase::bulkInsert(const std::wstring& _path)
{
    KL_AUTO_LOCK(m_obj_mutex);

    std::wstring path = _path;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    ExcelRowInserter* inserter;

    std::map<std::wstring, xd::FormatDefinition, kl::cmp_nocase>::iterator it;
    it = m_create_tables.find(path);
    if (it != m_create_tables.end())
    {
        inserter = new ExcelRowInserter(this, path, it->second.columns);
    }
     else
    {
        xd::Structure structure = describeTable(_path);
        if (structure.isNull())
            return xcm::null;

        inserter = new ExcelRowInserter(this, path, structure);
    }

    return static_cast<xd::IRowInserter*>(inserter);
}


xd::Structure ExcelDatabase::describeTable(const std::wstring& _path)
{
    KL_AUTO_LOCK(m_obj_mutex);

    std::wstring path = _path;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    std::map<std::wstring, xd::FormatDefinition, kl::cmp_nocase>::iterator it;
    it = m_create_tables.find(path);
    if (it != m_create_tables.end())
    {
        xd::Structure s;
        for (int i = 0, colcount = (int)it->second.columns.size(); i < colcount; ++i)
            s.createColumn(it->second.columns[i]);
        return s;
    }

    return xd::Structure();
}


bool ExcelDatabase::modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job)
{
    return false;
}

bool ExcelDatabase::execute(const std::wstring& command,
                          unsigned int flags,
                          xcm::IObjectPtr& result,
                          xd::IJob* job)
{
    return false;
}

bool ExcelDatabase::groupQuery(xd::GroupQueryParams* info, xd::IJob* job)
{
    return false;
}
