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


#include "tango.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/indexinfo.h"
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/util.h"
#include "database.h"
#include "iterator.h"
#include "inserter.h"
#include "pkgfile.h"
#include <set>
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/xml.h>


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




void xdkpgStructureToXml(tango::IStructurePtr s, kl::xmlnode& node)
{
    node.setNodeName(L"structure");

    tango::IColumnInfoPtr colinfo;
    int i, col_count;

    col_count = s->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        colinfo = s->getColumnInfoByIdx(i);

        kl::xmlnode& col = node.addChild();
        col.setNodeName(L"column");

        col.addChild(L"name", colinfo->getName());
        col.addChild(L"type", colinfo->getType());
        col.addChild(L"width", colinfo->getWidth());
        col.addChild(L"scale", colinfo->getScale());
        col.addChild(L"calculated", colinfo->getCalculated() ? 1 : 0);
        col.addChild(L"expression", colinfo->getExpression());
    }
}



tango::IStructurePtr xdkpgXmlToStructure(kl::xmlnode& node)
{
    tango::IStructurePtr s = static_cast<tango::IStructure*>(new Structure);

    int child_count = node.getChildCount();
    int i;

    for (i = 0; i < child_count; ++i)
    {
        kl::xmlnode& col = node.getChild(i);
        if (col.getNodeName() != L"column")
        {
            continue;
        }

        kl::xmlnode& colname = col.getChild(L"name");
        kl::xmlnode& coltype = col.getChild(L"type");
        kl::xmlnode& colwidth = col.getChild(L"width");
        kl::xmlnode& colscale = col.getChild(L"scale");
        kl::xmlnode& colcalculated = col.getChild(L"calculated");
        kl::xmlnode& colexpression = col.getChild(L"expression");

        if (colname.isEmpty() ||
            coltype.isEmpty() ||
            colwidth.isEmpty() ||
            colscale.isEmpty() ||
            colcalculated.isEmpty() ||
            colexpression.isEmpty())
        {
            return xcm::null;
        }

        tango::IColumnInfoPtr colinfo = s->createColumn();
        colinfo->setName(colname.getNodeValue());
        colinfo->setType(kl::wtoi(coltype.getNodeValue()));
        colinfo->setWidth(kl::wtoi(colwidth.getNodeValue()));
        colinfo->setScale(kl::wtoi(colscale.getNodeValue()));
        colinfo->setCalculated(kl::wtoi(colcalculated.getNodeValue()) != 0 ? true : false);
        colinfo->setExpression(colexpression.getNodeValue());
    }

    return s;
}







// KpgDatabase class implementation

KpgDatabase::KpgDatabase()
{
    m_attr = static_cast<tango::IAttributes*>(new DatabaseAttributes);

    m_attr->setIntAttribute(tango::dbattrColumnMaxNameLength, 63);
    m_attr->setIntAttribute(tango::dbattrTableMaxNameLength, 63);
    m_attr->setStringAttribute(tango::dbattrKeywords, sql92_keywords);    
    m_attr->setStringAttribute(tango::dbattrColumnInvalidChars, 
                               L"\\./\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrTableInvalidChars, 
                               L"\\./:*?<>|\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrColumnInvalidStartingChars,
                               L"\\./\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrTableInvalidStartingChars,
                               L"\\./:*?<>|\x00\xFF");
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteOpenChar, L"\"");
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteCloseChar, L"\"");
    m_attr->setStringAttribute(tango::dbattrIdentifierCharsNeedingQuote, L"`\"~# $!@%^&(){}-+.");   
}

KpgDatabase::~KpgDatabase()
{
    close();
}


std::wstring KpgDatabase::getPath()
{
    return m_path;
}


std::wstring KpgDatabase::getTempFileDirectory()
{
    std::wstring result = m_attr->getStringAttribute(tango::dbattrTempDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}

std::wstring KpgDatabase::getDefinitionDirectory()
{
    std::wstring result = m_attr->getStringAttribute(tango::dbattrDefinitionDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}




bool KpgDatabase::open(const std::wstring& path)
{
    m_error.clearError();
    
    m_path = path;
    m_kpg = new PkgFile;
    if (!m_kpg->open(path))
    {
        delete m_kpg;
        m_kpg = NULL;
        return false;
    }

    return true;
}


void KpgDatabase::close()
{
    m_path = L"";
    delete m_kpg;
    m_kpg = NULL;
}




void KpgDatabase::setDatabaseName(const std::wstring& name)
{
    m_db_name = name;
}

std::wstring KpgDatabase::getDatabaseName()
{
    return m_db_name;
}

std::wstring KpgDatabase::getActiveUid()
{
    return L"";
}

tango::IAttributesPtr KpgDatabase::getAttributes()
{
    return m_attr;
}

std::wstring KpgDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int KpgDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void KpgDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code, error_string);
}


bool KpgDatabase::cleanup()
{
    return true;
}

tango::IJobPtr KpgDatabase::createJob()
{
    XCM_AUTO_LOCK(m_jobs_mutex);

    JobInfo* job = new JobInfo;
    job->setJobId(++m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<tango::IJob*>(job);
}

tango::IDatabasePtr KpgDatabase::getMountDatabase(const std::wstring& path)
{
    return xcm::null;
}

bool KpgDatabase::setMountPoint(const std::wstring& path,
                                  const std::wstring& connection_str,
                                  const std::wstring& remote_path)
{
    return false;
}
                              
bool KpgDatabase::getMountPoint(const std::wstring& path,
                                  std::wstring& connection_str,
                                  std::wstring& remote_path)
{
    return false;
}

bool KpgDatabase::createFolder(const std::wstring& path)
{
    return false;
}

bool KpgDatabase::renameFile(const std::wstring& path,
                               const std::wstring& new_name)
{
    return false;
}

bool KpgDatabase::moveFile(const std::wstring& path,
                             const std::wstring& new_location)
{
    return false;
}

bool KpgDatabase::copyFile(const std::wstring& src_path,
                             const std::wstring& dest_path)
{
    return false;
}

bool KpgDatabase::copyData(const tango::CopyParams* info, tango::IJob* job)
{
    tango::IIteratorPtr iter;
    tango::IStructurePtr structure;

    if (info->iter_input.isOk())
    {
        iter = info->iter_input;
        structure = iter->getStructure();
    }
     else
    {
        tango::QueryParams qp;
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
        if (!createTable(info->output, structure, NULL))
            return false;
    }


    xdcmnInsert(static_cast<tango::IDatabase*>(this), iter, info->output, info->where, info->limit, job);

    return true;

}

bool KpgDatabase::deleteFile(const std::wstring& path)
{
    return true;
}

bool KpgDatabase::getFileExist(const std::wstring& _path)
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
        if (kl::iequals(info->getName(), path))
            return true;
    }

    return false;
}

tango::IFileInfoPtr KpgDatabase::getFileInfo(const std::wstring& path)
{
    std::wstring folder;
    std::wstring name;
    
    if (path.empty() || path == L"/")
    {
        xdcommon::FileInfo* f = new xdcommon::FileInfo();
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
        if (kl::iequals(finfo->getName(), name))
        {
            return finfo;
        }
    }
    
    return xcm::null;
}

tango::IFileInfoEnumPtr KpgDatabase::getFolderInfo(const std::wstring& path)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    xcm::IVectorImpl<tango::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;

    PkgStreamEnum* kpgenum = m_kpg->getStreamEnum();
    if (kpgenum)
    {
        int i, cnt = kpgenum->getStreamCount();
        for (i = 0; i < cnt; ++i)
        {
            PkgDirEntry entry;
            std::wstring name = kpgenum->getStreamName(i);
            if (kpgenum->getStreamInfo(name, entry))
            {
                if (!entry.deleted)
                {
                    xdcommon::FileInfo* f = new xdcommon::FileInfo;
                    f->name = entry.stream_name;
                    f->type = tango::filetypeTable;

                    retval->append(static_cast<tango::IFileInfo*>(f));
                }
            }
        }
    }

    return retval;
}

std::wstring KpgDatabase::getPrimaryKey(const std::wstring path)
{
    return L"";
}

tango::IStructurePtr KpgDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<tango::IStructure*>(s);
}

bool KpgDatabase::createTable(const std::wstring& _path,
                              tango::IStructurePtr struct_config,
                              tango::FormatInfo* format_info)
{
    XCM_AUTO_LOCK(m_obj_mutex);

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

    m_create_tables[path] = struct_config;

    return true;
}

tango::IStreamPtr KpgDatabase::openStream(const std::wstring& path)
{
    return xcm::null;
}

bool KpgDatabase::createStream(const std::wstring& path, const std::wstring& mime_type)
{
    return false;
}


bool KpgDatabase::getStreamInfoBlock(const std::wstring& _path, std::wstring& output)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::wstring path = _path;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    PkgStreamReader* reader = m_kpg->readStream(path);
    if (!reader)
        return xcm::null;

    int len;
    unsigned char* data = (unsigned char*)reader->loadNextBlock(&len);
    if (!data || len == 0)
    {
        delete reader;
        return xcm::null;
    }

    if (m_kpg->getVersion() == 1)
    {
        output = kl::towstring((const char*)data);
    }
     else
    {
        kl::ucsle2wstring(output, data, len/2);
    }

    delete reader;

    return true;
}


tango::IIteratorPtr KpgDatabase::query(const tango::QueryParams& qp)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::wstring path = qp.from;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    // create an iterator based on our select statement
    KpgIterator* iter = new KpgIterator(this);

    if (!iter->init(path))
    {
        delete iter;
        return xcm::null;
    }

    return static_cast<tango::IIterator*>(iter);
}



tango::IIndexInfoPtr KpgDatabase::createIndex(const std::wstring& path,
                                              const std::wstring& name,
                                              const std::wstring& expr,
                                              tango::IJob* job)
{
    return xcm::null;
}


bool KpgDatabase::renameIndex(const std::wstring& path,
                              const std::wstring& name,
                              const std::wstring& new_name)
{
    return false;
}


bool KpgDatabase::deleteIndex(const std::wstring& path,
                              const std::wstring& name)
{
    return false;
}


tango::IIndexInfoEnumPtr KpgDatabase::getIndexEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IIndexInfoPtr>* vec;
    vec = new xcm::IVectorImpl<tango::IIndexInfoPtr>;

    return vec;
}



tango::IRowInserterPtr KpgDatabase::bulkInsert(const std::wstring& _path)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::wstring path = _path;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    tango::IStructurePtr structure;

    std::map<std::wstring, tango::IStructurePtr, kl::cmp_nocase>::iterator it;
    it = m_create_tables.find(path);
    if (it != m_create_tables.end())
    {
        structure = it->second;
    }
     else
    {
        structure = describeTable(_path);
    }

    if (structure.isNull())
        return xcm::null;

    KpgRowInserter* inserter = new KpgRowInserter(this, path, structure);
    return static_cast<tango::IRowInserter*>(inserter);
}


tango::IStructurePtr KpgDatabase::describeTable(const std::wstring& _path)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::wstring path = _path;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    tango::IStructurePtr structure;

    std::map<std::wstring, tango::IStructurePtr, kl::cmp_nocase>::iterator it;
    it = m_create_tables.find(path);
    if (it != m_create_tables.end())
    {
        return it->second;
    }



    std::wstring stream_info;
    if (!getStreamInfoBlock(path, stream_info))
        return xcm::null;

    // create set and initialize variables
    kl::xmlnode info;

    if (!info.parse(stream_info))
        return xcm::null;

    int node_idx = info.getChildIdx(L"structure");
    if (node_idx == -1)
        return xcm::null;

    kl::xmlnode& structure_node = info.getChild(node_idx);

    return xdkpgXmlToStructure(structure_node);
}

bool KpgDatabase::modifyStructure(const std::wstring& path, tango::IStructurePtr struct_config, tango::IJob* job)
{
    return false;
}

bool KpgDatabase::execute(const std::wstring& command,
                          unsigned int flags,
                          xcm::IObjectPtr& result,
                          tango::IJob* job)
{
    return false;
}

bool KpgDatabase::groupQuery(tango::GroupQueryParams* info, tango::IJob* job)
{
    return false;
}
