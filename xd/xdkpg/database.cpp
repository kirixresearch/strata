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
#include "../xdcommon/util.h"
#include "database.h"
#include "iterator.h"
#include "inserter.h"
#include "pkgfile.h"
#include "stream.h"
#include <set>
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/xml.h>
#include <kl/md5.h>


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




void xdkpgStructureToXml(const xd::Structure& s, kl::xmlnode& node)
{
    node.setNodeName(L"structure");

    std::vector<xd::ColumnInfo>::const_iterator it;
    for (it = s.columns.begin(); it != s.columns.end(); ++it)
    {
        const xd::ColumnInfo& colinfo = *it;

        kl::xmlnode& col = node.addChild();
        col.setNodeName(L"column");

        col.addChild(L"name", colinfo.name);
        col.addChild(L"type", colinfo.type);
        col.addChild(L"width", colinfo.width);
        col.addChild(L"scale", colinfo.scale);
        col.addChild(L"calculated", colinfo.calculated ? 1 : 0);
        col.addChild(L"expression", colinfo.expression);
    }
}



xd::Structure xdkpgXmlToStructure(kl::xmlnode& node)
{
    xd::Structure s;

    int i, child_count = node.getChildCount();

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
            return xd::Structure();
        }

        
        xd::ColumnInfo colinfo;
        colinfo.name = colname.getNodeValue();
        colinfo.type = kl::wtoi(coltype.getNodeValue());
        colinfo.width = kl::wtoi(colwidth.getNodeValue());
        colinfo.scale = kl::wtoi(colscale.getNodeValue());
        colinfo.calculated = kl::wtoi(colcalculated.getNodeValue()) != 0 ? true : false;
        colinfo.expression = colexpression.getNodeValue();
        
        s.createColumn(colinfo);
    }

    return s;
}




// KpgDatabase class implementation

KpgDatabase::KpgDatabase()
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
    std::wstring result = m_attr->getStringAttribute(xd::dbattrTempDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}

std::wstring KpgDatabase::getDefinitionDirectory()
{
    std::wstring result = m_attr->getStringAttribute(xd::dbattrDefinitionDirectory);
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

xd::IAttributesPtr KpgDatabase::getAttributes()
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

xd::IJobPtr KpgDatabase::createJob()
{
    KL_AUTO_LOCK(m_jobs_mutex);

    JobInfo* job = new JobInfo;
    job->setJobId(++m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<xd::IJob*>(job);
}

xd::IDatabasePtr KpgDatabase::getMountDatabase(const std::wstring& path)
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

bool KpgDatabase::renameFile(const std::wstring& _path,
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

    m_kpg->renameStream(L".resource/" + path, L".resource/" + new_path);
    return m_kpg->renameStream(path, new_path);
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

bool KpgDatabase::copyData(const xd::CopyParams* info, xd::IJob* job)
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

bool KpgDatabase::deleteFile(const std::wstring& _path)
{
    std::wstring path = _path;
    if (path.empty())
        return false;
    if (path[0] == '/')
        path.erase(0,1);

    return m_kpg->deleteStream(path);
}

bool KpgDatabase::getFileExist(const std::wstring& _path)
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

xd::IFileInfoPtr KpgDatabase::getFileInfo(const std::wstring& path)
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
        //folder = kl::beforeLast(path, L'/');
        //name = kl::afterLast(path, L'/');
        folder = L"";
        name = path;
        while (name.substr(0,1) == L"/")
            name = name.substr(1);
    }
    
    xd::IFileInfoEnumPtr files = getFolderInfo(folder);
    int i, count;
    
    count = files->size();
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

xd::IFileInfoEnumPtr KpgDatabase::getFolderInfo(const std::wstring& path)
{
    KL_AUTO_LOCK(m_obj_mutex);

    std::wstring lower_path = m_path;
    kl::makeLower(lower_path);

    xcm::IVectorImpl<xd::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;

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
                    std::wstring stream_info;
                    if (!getStreamInfoBlock(name, stream_info))
                        continue;

                    std::wstring lower_stream = name;
                    kl::makeLower(lower_stream);

                    kl::xmlnode info;
                    if (!info.parse(stream_info))
                        continue;
                    std::wstring object_type = info.getProperty(L"type").value;
                    std::wstring mime_type = info.getChild(L"mime_type").getNodeValue();

                    xdcommon::FileInfo* f = new xdcommon::FileInfo;
                    f->name = entry.stream_name;
                    f->type = (object_type == L"stream" ? xd::filetypeStream : xd::filetypeTable);
                    if (mime_type.length() > 0)
                        f->mime_type = mime_type;
                    f->object_id = kl::md5str(L"xdkpg:" + lower_path + L":" + lower_stream);

                    retval->append(static_cast<xd::IFileInfo*>(f));
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

bool KpgDatabase::createTable(const std::wstring& _path,
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

xd::IStreamPtr KpgDatabase::openStream(const std::wstring& _path)
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

    KpgStream* pstream = new KpgStream(this);
    if (!pstream->init(path, mime_type))
    {
        delete pstream;
        return xcm::null;
    }

    return static_cast<xd::IStream*>(pstream);
}

bool KpgDatabase::createStream(const std::wstring& _path, const std::wstring& mime_type)
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


bool KpgDatabase::getStreamInfoBlock(const std::wstring& _path, std::wstring& output)
{
    KL_AUTO_LOCK(m_obj_mutex);

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


xd::IIteratorPtr KpgDatabase::query(const xd::QueryParams& qp)
{
    KL_AUTO_LOCK(m_obj_mutex);

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

    return static_cast<xd::IIterator*>(iter);
}



xd::IndexInfo KpgDatabase::createIndex(const std::wstring& path,
                                       const std::wstring& name,
                                       const std::wstring& expr,
                                       xd::IJob* job)
{
    return xd::IndexInfo();
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


xd::IndexInfoEnum KpgDatabase::getIndexEnum(const std::wstring& path)
{
    return xd::IndexInfoEnum();
}


xd::IRowInserterPtr KpgDatabase::bulkInsert(const std::wstring& _path)
{
    KL_AUTO_LOCK(m_obj_mutex);

    std::wstring path = _path;
    if (path.substr(0,1) == L"/")
        path.erase(0,1);

    KpgRowInserter* inserter;

    std::map<std::wstring, xd::FormatDefinition, kl::cmp_nocase>::iterator it;
    it = m_create_tables.find(path);
    if (it != m_create_tables.end())
    {
        inserter = new KpgRowInserter(this, path, it->second.columns);
    }
     else
    {
        xd::Structure structure = describeTable(_path);
        if (structure.isNull())
            return xcm::null;

        inserter = new KpgRowInserter(this, path, structure);
    }

    return static_cast<xd::IRowInserter*>(inserter);
}


xd::Structure KpgDatabase::describeTable(const std::wstring& _path)
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


    std::wstring stream_info;
    if (!getStreamInfoBlock(path, stream_info))
        return xd::Structure();

    // create set and initialize variables
    kl::xmlnode info;

    if (!info.parse(stream_info))
        return xd::Structure();

    int node_idx = info.getChildIdx(L"structure");
    if (node_idx == -1)
        return xd::Structure();

    kl::xmlnode& structure_node = info.getChild(node_idx);

    return xdkpgXmlToStructure(structure_node);
}


bool KpgDatabase::modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job)
{
    return false;
}

bool KpgDatabase::execute(const std::wstring& command,
                          unsigned int flags,
                          xcm::IObjectPtr& result,
                          xd::IJob* job)
{
    return false;
}

bool KpgDatabase::groupQuery(xd::GroupQueryParams* info, xd::IJob* job)
{
    return false;
}
