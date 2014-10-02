/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-11-09
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <ctime>
#include <kl/klib.h>
#include <kl/crypt.h>
#include <kl/url.h>
#include <kl/system.h>
#include <kl/hex.h>
#include <xd/xd.h>
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/filestream.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/cmndynamicset.h"
#include "../xdcommon/groupquery.h"
#include "nodefilestream.h"
#include "database.h"
#include "baseset.h"
#include "nativetbl.h"
#include "tableset.h"
#include "util.h"
#include "ofs.h"
#include "dbmgr.h"


const wchar_t* xdnative_keywords =
                L"ADD,ALL,ALTER,AND,ANY,AS,ASC,AVG,BEGIN,BETWEEN,BOOL,BOOLEAN,"
                L"BOTH,BREAK,BY,CASE,CHAR,CHARACTER,CHECK,CLOSE,COLLATE,"
                L"COLUMN,COMMIT,CONNECT,CONTINUE,CREATE,CURRENT,CURSOR,"
                L"DATE,DATETIME,DECIMAL,DECLARE,DEFAULT,DELETE,DESC,"
                L"DESCRIBE,DISTINCT,DO,DOUBLE,DROP,ELSE,ELSEIF,END,EXISTS,"
                L"FALSE,FETCH,FLOAT,FOR,FOREIGN,FROM,FULL,FUNCTION,GOTO,"
                L"GRANT,GROUP,HAVING,IF,IN,INDEX,INNER,INSERT,INT,INTEGER,"
                L"INTERSECT,INTO,IS,JOIN,KEY,LEFT,LEVEL,LIKE,LONG,MATCH,MIN,MAX,NEW,"
                L"NOT,NULL,NUMERIC,OF,ON,OPEN,OPTION,OR,ORDER,OUTER,PRIMARY,"
                L"PRIVATE,PRECISION,PRIVILEGES,PROCEDURE,PROTECTED,PUBLIC,"
                L"READ,RESTRICT,RETURN,REVOKE,RIGHT,ROWS,SELECT,SESSION,SET,"
                L"SIZE,SHORT,SIGNED,SMALLINT,SOME,SUM,SWITCH,TABLE,THEN,THIS,TO,"
                L"TRUE,UNION,UNIQUE,UNSIGNED,UPDATE,USER,USING,VALUES,VARCHAR,"
                L"VARYING,VIEW,VOID,WHEN,WHENEVER,WHERE,WHILE,WITH";


// secondary keywords (may be used in the future)

const wchar_t* xdnative_keywords2 = L"";



// Database class implementation

XdnativeDatabase::XdnativeDatabase()
{
    m_last_job = 0;
    m_base_dir = L"";

    std::wstring kws;
    kws += xdnative_keywords;
    kws += L",";
    kws += xdnative_keywords2;

    m_attr = static_cast<xd::IAttributes*>(new DatabaseAttributes);
    m_attr->setIntAttribute(xd::dbattrColumnMaxNameLength, 80);
    m_attr->setIntAttribute(xd::dbattrTableMaxNameLength, 80);
    m_attr->setStringAttribute(xd::dbattrKeywords, kws);
    m_attr->setStringAttribute(xd::dbattrColumnInvalidChars,
                               L"*|:\"<>?[]\\;'=,/\x00\x09\x0A\x0B\x0C\x0D\xFF");
    m_attr->setStringAttribute(xd::dbattrColumnInvalidStartingChars,
                               L"*|:\"<>?[]\\;'=,/\x00\x09\x0A\x0B\x0C\x0D\xFF");
    m_attr->setStringAttribute(xd::dbattrTableInvalidChars,
                               L"*|:\"<>?[]\\;'=,/\x00\x09\x0A\x0B\x0C\x0D\xFF");
    m_attr->setStringAttribute(xd::dbattrTableInvalidStartingChars,
                               L"*|:\"<>?[]\\;'=,/\x00\x09\x0A\x0B\x0C\x0D\xFF");
    m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, L"[");
    m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, L"]");
    m_attr->setStringAttribute(xd::dbattrIdentifierCharsNeedingQuote, L"`~# $!@%^&(){}-+.");

    m_db_mgr = static_cast<xd::IDatabaseMgr*>(new DatabaseMgr);
}

XdnativeDatabase::~XdnativeDatabase()
{
    std::vector<JobInfo*>::iterator it;

    m_jobs_mutex.lock();
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
        (*it)->unref();
    m_jobs_mutex.unlock();

    deleteTempData();
}


bool XdnativeDatabase::setBaseDirectory(const std::wstring& base_dir)
{    
    m_base_dir = base_dir;

    // make sure m_base_dir has a trailing slash

    if (m_base_dir.length() == 0)
    {
        m_base_dir += PATH_SEPARATOR_CHAR;
    }
     else
    {
        if (m_base_dir[m_base_dir.length()-1] != PATH_SEPARATOR_CHAR)
            m_base_dir += PATH_SEPARATOR_CHAR;
    }

    // set ofs root from m_base_dir

    m_ofs_root = m_base_dir;
    m_ofs_root += L"ofs";

    m_ordinal_counter_path = ofsToPhysFilename(L"/.system/ordinal_counter", false);

    // update these database attributes
    m_attr->setStringAttribute(xd::dbattrTempDirectory, getTempPath());
    m_attr->setStringAttribute(xd::dbattrDefinitionDirectory, getDefinitionPath());

    return true;
}

std::wstring XdnativeDatabase::getActiveUid()
{
    return m_uid;
}


xd::IAttributesPtr XdnativeDatabase::getAttributes()
{
    return m_attr;
}




void XdnativeDatabase::addFileToTrash(const std::wstring& filename)
{
    INodeValuePtr root;

    if (!getFileExist(L"/.system/trash"))
    {
        root = createNodeFile(L"/.system/trash");
    }
     else
    {
        root = openNodeFile(L"/.system/trash");
    }

    INodeValuePtr item = root->createChild(getUniqueString());
    item->setString(filename);
}

void XdnativeDatabase::emptyTrash()
{
    if (!getFileExist(L"/.system/trash"))
    {
        return;
    }

    INodeValuePtr root;
    root = openNodeFile(L"/.system/trash");

    size_t i, child_count = root->getChildCount();

    std::vector<std::wstring> nodes_to_remove;

    for (i = 0; i < child_count; ++i)
    {
        INodeValuePtr item = root->getChildByIdx(i);
        if (item.isNull())
            continue;

        std::wstring filename = item->getString();


        if (filename.find(L'\\') == -1 &&
            filename.find(L'/') == -1)
        {
            // filename is not absolute
            filename = makePathName(m_base_dir, L"data", filename);
        }



        if (!xf_get_file_exist(filename))
        {
            // file does not exist, so get rid
            // of this entry from the trash
            nodes_to_remove.push_back(item->getName());
            continue;
        }

        if (xf_remove(filename))
        {
            // removal successful
            nodes_to_remove.push_back(item->getName());
        }
    }

    std::vector<std::wstring>::iterator it;
    for (it = nodes_to_remove.begin();
         it != nodes_to_remove.end();
         ++it)
    {
        root->deleteChild(*it);
    }
}


std::wstring XdnativeDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int XdnativeDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void XdnativeDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code, error_string);
}


std::wstring XdnativeDatabase::getTempFilename()
{
    return makePathName(m_base_dir, L"temp", getUniqueString());
}

std::wstring XdnativeDatabase::getUniqueFilename()
{
    return makePathName(m_base_dir, L"data", getUniqueString());
}

std::wstring XdnativeDatabase::getTempOfsPath()
{
    std::wstring s = L"/.temp/";
    s += getUniqueString();
    return s;
}

std::wstring XdnativeDatabase::getTempPath()
{
    return makePathName(m_base_dir, L"temp", L"");
}

std::wstring XdnativeDatabase::getDefinitionPath()
{
    return makePathName(m_base_dir, L"def", L"");
}

std::wstring XdnativeDatabase::getBasePath()
{
    return m_base_dir;
}

std::wstring XdnativeDatabase::getOfsPath()
{
    return m_ofs_root;
}

#ifndef WIN32

static bool correctCase(const std::wstring& dir, std::wstring& fn)
{
    std::string s_dir = kl::tostring(dir);
    std::string s_fn = kl::tostring(fn);
    
    xf_dirhandle_t h = xf_opendir(dir);
    xf_direntry_t info;
    while (xf_readdir(h, &info))
    {
        if (0 == wcscasecmp(info.m_name.c_str(), fn.c_str()))
        {
            fn = info.m_name;
            xf_closedir(h);
            return true;
        }
    }
    xf_closedir(h);
    
    return false;
}

static bool correctFilenameCase(std::wstring& fn)
{
    std::vector<std::wstring> parts;
    std::vector<std::wstring>::iterator it;
    bool found = true;
    
    kl::parseDelimitedList(fn, parts, L'/', false);
    
    for (it = parts.begin(); it != parts.end(); ++it)
    {
        if (it->empty())
            continue;

         
        std::wstring dir;
        std::vector<std::wstring>::iterator bit;
        for (bit = parts.begin(); bit != it; ++bit)
        {
            dir += *bit;
            dir += L"/";
        }
        
        if (!correctCase(dir, *it))
        {
            found = false;
            break;
        }
    }
    
    // build result
    
    std::wstring result;
    for (it = parts.begin(); it != parts.end(); ++it)
    {
        result += *it;
        if (it+1 != parts.end())
        {
            result += L"/";
        }
    }
    
    fn = result;
    
    return found;
}

#endif


IJobInternalPtr XdnativeDatabase::createJobEntry()
{
    KL_AUTO_LOCK(m_jobs_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();

    m_jobs.push_back(job);

    return static_cast<IJobInternal*>(job);
}

xd::IJobPtr XdnativeDatabase::createJob()
{
    KL_AUTO_LOCK(m_jobs_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<xd::IJob*>(job);
}


std::wstring XdnativeDatabase::ofsToPhysFilename(const std::wstring& ofs_path, bool folder)
{
    std::wstring result;
    result = makePathName(m_ofs_root, ofs_path, L"", folder ? L"" : L"xml");
    
#ifdef WIN32
    return result;
#else
    // non-windows filesystems are case-sensitive, so correct the filename
    correctFilenameCase(result);
    return result;
#endif
}


bool XdnativeDatabase::createDatabase(const std::wstring& db_name,
                                      const std::wstring& base_dir)
{
    // create the directory
    if (!xf_get_directory_exist(base_dir))
    {
        bool res = xf_mkdir(base_dir);
        if (!res)
        {
            return false;
        }
    }

    // create the temporary directory
    if (!xf_mkdir(makePathName(base_dir, L"temp")))
        return false;

    // create the data store directory
    if (!xf_mkdir(makePathName(base_dir, L"data")))
        return false;
    
    // create the definitions directory
    if (!xf_mkdir(makePathName(base_dir, L"def")))
        return false;

    // create the object filesystem
    if (!xf_mkdir(makePathName(base_dir, L"ofs")))
        return false;

    setBaseDirectory(base_dir);


    // create system folder structure
    INodeValuePtr node;
    bool res;
    
    res = createFolder(L"/.appdata");
    if (!res)
        return false;

    res = createFolder(L"/.temp");
    if (!res)
        return false;

    res = createFolder(L"/.system");
    if (!res)
        return false;

    node = createNodeFile(L"/.system/ordinals");
    if (!node)
        return false;

    node = createNodeFile(L"/.system/database_name");
    if (!node)
        return false;
    node->setString(db_name);

    m_attr->setStringAttribute(xd::dbattrDatabaseName, db_name);

    node = createNodeFile(L"/.system/database_version");
    if (!node)
        return false;
    node->setInteger(2);

    m_uid = L"admin";

    // create an ordinal file
    xd::tableord_t ord = allocOrdinal();

    // create a mount point for external files
    setMountPoint(L"/.fs", L"Xdprovider=xdfs;Database=;User ID=;Password=;", L"/");

    return true;
}


bool XdnativeDatabase::openDatabase(const std::wstring& location,
                                    const std::wstring& uid,
                                    const std::wstring& password)
{
    // check for the existence of the db's base directory
    if (!xf_get_directory_exist(location))
        return false;

    // check for the existence of the db's registry
    if (!xf_get_directory_exist(makePathName(location, L"ofs")))
        return false;

    // check for the existence of the db's def directory;
    // create it if it's missing - in older versions it did
    // not used to exist
    if (!xf_get_directory_exist(makePathName(location, L"def")))
        xf_mkdir(makePathName(location, L"def"));
    
    setBaseDirectory(location);

    if (!getFileExist(L"/.system/users"))
    {
        m_uid = L"admin";
        createFolder(L"/.system/users");
        createUser(L"admin", L"");
    }

    if (!checkPassword(uid, password))
    {
        // bad password
        return false;
    }

    m_uid = uid;


    // read database name
    INodeValuePtr dbname = openNodeFile(L"/.system/database_name");
    if (!dbname)
        return false;

    m_attr->setStringAttribute(xd::dbattrDatabaseName, dbname->getString());

    // ensure that an ordinal counter exists
    INodeValuePtr last_ord = openNodeFile(L"/.system/ordinal_counter");
    if (!last_ord)
        return false;

    // ensure that a database version exists
    if (!getFileExist(L"/.system/database_version"))
    {
        INodeValuePtr db_ver = createNodeFile(L"/.system/database_version");
        if (db_ver)
        {
            db_ver->setInteger(2);
        }
    }
    
    // ensure that a xdfs mount exists;
    // note older versions of databases don't have this, so we check to make
    // sure it gets added to databases if it doesn't already exist
    if (!getFileExist(L"/.fs"))
    {
        setMountPoint(L"/.fs", L"Xdprovider=xdfs;Database=;", L"/");
    }

    return true;
}


void XdnativeDatabase::close()
{
    emptyTrash();
    deleteTempData();
}



inline std::wstring getUserPasswordEncryptionKey()
{
    return L"jgk5]4X4";
}


bool XdnativeDatabase::createUser(const std::wstring& uid,
                                  const std::wstring& password)
{
    // must be admin to create a user
    if (m_uid != L"admin")
        return false;

    std::wstring path = L"/.system/users";
    
    if (!getFileExist(path))
        createFolder(path);
    
    path += L"/";
    path += uid;

    // if user already exists, return failure
    if (getFileExist(path))
        return false;

    INodeValuePtr node = createNodeFile(path);
    if (node.isNull())
        return false;
        
    INodeValuePtr uid_node = node->getChild(L"uid", true);
    uid_node->setString(uid);
    
    INodeValuePtr password_node = node->getChild(L"password", true);
    password_node->setString(kl::encryptString(password, getUserPasswordEncryptionKey()));
    
    return true;
}

bool XdnativeDatabase::deleteUser(const std::wstring& uid)
{
    // must be admin to create a user
    if (m_uid != L"admin")
        return false;
    
    std::wstring path = L"/.system/users/";
    path += uid;
    
    return deleteFile(path);
}


bool XdnativeDatabase::checkPassword(
                          const std::wstring& uid,
                          const std::wstring& password)
{
    std::wstring path = L"/.system/users/";
    path += uid;
    
    INodeValuePtr node = openNodeFile(path);
    if (node.isNull())
        return false;
        
    INodeValuePtr uid_node = node->getChild(L"uid", false);
    if (!uid_node)
        return false;
    
    INodeValuePtr password_node = node->getChild(L"password", false);
    if (!password_node)
        return false;

    std::wstring s = kl::decryptString(password_node->getString(), getUserPasswordEncryptionKey());
    if (s == password)
        return true;
        
    return false;
}

bool XdnativeDatabase::getUserExist(const std::wstring& uid)
{
    std::wstring path = L"/.system/users/";
    path += uid;
    
    return getFileExist(path);
}

xd::tableord_t XdnativeDatabase::getMaximumOrdinal()
{
    kl::xmlnode ofs_file;

    if (!ofs_file.load(m_ordinal_counter_path, 0, kl::xmlnode::filemodeExclusiveWait))
        return 0;

    kl::xmlnode& ofs_node = ofs_file.getChild(L"ofs_node");
    if (ofs_file.isEmpty())
        return 0;

    kl::xmlnode& value = ofs_node.getChild(L"value");
    if (value.isEmpty())
        return 0;

    return kl::wtoi(value.getNodeValue());
}

xd::tableord_t XdnativeDatabase::allocOrdinal()
{
    if (!xf_get_file_exist(m_ordinal_counter_path))
    {
        kl::xmlnode ofs_file;
        ofs_file.setNodeName(L"ofs_file");
        ofs_file.appendProperty(L"version", L"1.0");
        ofs_file.addChild(L"file_type", L"generic");

        kl::xmlnode& ofs_node = ofs_file.addChild(L"ofs_node");
        ofs_node.appendProperty(L"name", L"root");
        ofs_node.addChild(L"value", L"1");

        if (!ofs_file.save(m_ordinal_counter_path, 0, kl::xmlnode::filemodeExclusiveWait))
            return 0;

        return 1;
    }
    

    // obtain an exclusive lock on the counter file
    xf_file_t f = (xf_file_t)0;
    
    for (int i = 0; i < 300; ++i)
    {
        f = xf_open(m_ordinal_counter_path, xfOpen, xfReadWrite, xfShareNone);
        if (f)
            break;
        kl::millisleep(100);
    }

   
    if (!f)
    {
        // could not open ordinal file
        return 0;
    }



    // get file length
    xf_seek(f, 0, xfSeekEnd);
    int read_length = (int)xf_get_file_pos(f);

    std::wstring xml;
    bool ucs2 = false;

    int buf_size = read_length + 64;
    unsigned char* buf = new unsigned char[buf_size];

    xf_seek(f, 0, xfSeekSet);

    int len = xf_read(f, buf, 1, read_length);
    buf[len] = 0;
    buf[len+1] = 0;

    if (buf[0] == 0xff && buf[1] == 0xfe && (buf[2] != 0x00 || buf[3] != 0x00))
    {
        kl::ucsle2wstring(xml, buf+2, len-2);
        ucs2 = true;
    }
     else
    {
        xml = kl::towstring((char*)buf);
    }

    kl::xmlnode ofs_file;
    if (!ofs_file.parse(xml))
    {
        xf_close(f);
        delete[] buf;
        return 0;
    }

    kl::xmlnode& ofs_node = ofs_file.getChild(L"ofs_node");
    if (ofs_file.isEmpty())
    {
        xf_close(f);
        delete[] buf;
        return 0;
    }

    kl::xmlnode& value = ofs_node.getChild(L"value");
    if (value.isEmpty())
    {
        xf_close(f);
        delete[] buf;
        return 0;
    }

    std::wstring v = value.getNodeValue();
    int ordinal = kl::wtoi(v);
    value.setNodeValue(ordinal + 1);

    xml = ofs_file.getXML();


    xf_seek(f, 0, xfSeekSet);

    if (ucs2)
    {
        buf[0] = 0xff;
        buf[1] = 0xfe;
        kl::wstring2ucsle(buf+2, xml, xml.length());
        xf_write(f, buf, 1, 2 + (xml.length()*2));
    }
     else
    {
        memset(buf, ' ', buf_size);
        std::string sxml = kl::tostring(xml);
        strcpy((char*)buf, sxml.c_str()); 
        xf_write(f, buf, 1, sxml.length());
    }

    xf_close(f);
    delete[] buf;

    return ordinal;
}

bool XdnativeDatabase::setOrdinalTable(xd::tableord_t ordinal,
                                       const std::wstring& _filename)
{
    std::wstring filename;
    std::wstring data_path = makePathName(m_base_dir, L"data");

    int path_len = data_path.length();

    if (path_len == 0 || data_path[path_len-1] != PATH_SEPARATOR_CHAR)
    {
        data_path += PATH_SEPARATOR_CHAR;
        ++path_len;
    }

    if (!wcsncasecmp(_filename.c_str(), data_path.c_str(), path_len))
    {
        filename = _filename.substr(path_len);
        
        if (filename.find(PATH_SEPARATOR_CHAR) != -1)
        {
            filename = _filename;
        }
    }
     else
    {
        filename = _filename;
    }


    // write out ordinal entry with table filename
    wchar_t tableord_path[64];
    swprintf(tableord_path, 64, L"/.system/ordinals/%u", ordinal);

    INodeValuePtr tableord;
    if (!getFileExist(tableord_path))
    {
        tableord = createNodeFile(tableord_path);
        if (!tableord)
            return false;
    }
     else
    {
        tableord = openNodeFile(tableord_path);
        if (!tableord)
            return false;
    }

    INodeValuePtr target_file = tableord->getChild(L"target_file", true);
    if (!target_file)
        return false;

    target_file->setString(filename);

    return true;
}

bool XdnativeDatabase::deleteOrdinal(xd::tableord_t ordinal)
{
    // write out ordinal entry with table filename
    wchar_t tableord_path[64];
    swprintf(tableord_path, 64, L"/.system/ordinals/%u", ordinal);
    
    if (!deleteFile(tableord_path))
        return false;

    return true;
}




void XdnativeDatabase::updateSetReference(const std::wstring& ofs_path)
{
    // open file
    INodeValuePtr set_file = openNodeFile(ofs_path);
    if (set_file.isNull())
        return;

    INodeValuePtr setid_node = set_file->getChild(L"set_id", false);
    if (!setid_node)
        return;
    
    std::wstring set_id = setid_node->getString();

    setid_node = xcm::null;
    set_file = xcm::null;

    std::wstring path;
    path.reserve(80);
    path = L"/.system/objects/";
    path += set_id;

    INodeValuePtr setid_file = openNodeFile(path);
    if (setid_file.isNull())
        return;

    INodeValuePtr ofspath_value = setid_file->getChild(L"ofs_path", true);
    ofspath_value->setString(ofs_path);

    // if the set is open, let it know that its ofs path changed
    IXdnativeSet* lookup_set = lookupSet(set_id);
    if (lookup_set)
    {
        lookup_set->onOfsPathChanged(ofs_path);

        // release ref set by lookupSet() above
        lookup_set->unref();
    }
}


void XdnativeDatabase::recursiveReferenceUpdate(const std::wstring& folder_path)
{
    xd::IFileInfoEnumPtr files = getFolderInfo(folder_path);
    xd::IFileInfoPtr info;

    int file_count = files->size();
    int i;

    for (i = 0; i < file_count; ++i)
    {
        info = files->getItem(i);
        int file_type = info->getType();

        std::wstring path = combineOfsPath(folder_path, info->getName());

        if (file_type == xd::filetypeTable)
        {
            updateSetReference(path);
        }
         else if (file_type == xd::filetypeFolder)
        {
            recursiveReferenceUpdate(path);
        }
    }
}



bool XdnativeDatabase::deleteStream(const std::wstring& ofs_path)
{
    xd::IFileInfoPtr file_info = getFileInfo(ofs_path);
    if (!file_info)
        return false;

    if (file_info->getType() != xd::filetypeStream)
        return false;
    file_info.clear();
        
    std::wstring stream_fname = getStreamFilename(ofs_path);
    
    if (xf_get_file_exist(stream_fname))
    {
        if (!xf_remove(stream_fname))
        {
            // can't delete, must be in use
            return false;
        }
    }
    
    // get object id
    INodeValuePtr stream_file = openNodeFile(ofs_path);
    if (!stream_file)
    {
        return false;
    }
    
    INodeValuePtr objectid_node = stream_file->getChild(L"object_id", false);
    if (!objectid_node)
        return false;
    
    std::wstring object_id = objectid_node->getString();
    stream_file.clear();
    objectid_node.clear();
    
    
    // delete file in objects folder
    if (object_id.length() > 0)
    {
        std::wstring path;
        path.reserve(80);
        path = L"/.system/objects/";
        path += object_id;
        deleteOfsFile(path);
    }

    // delete stream pointer file
    deleteOfsFile(ofs_path);
    
    return true;
}


bool XdnativeDatabase::deleteSet(const std::wstring& ofs_path)
{
    // make sure we are dealing with a file
    xd::IFileInfoPtr file_info = getFileInfo(ofs_path);
    if (!file_info)
        return false;

    if (file_info->getType() != xd::filetypeTable)
        return false;

    file_info.clear();

    // get set id
    INodeValuePtr set_file = openNodeFile(ofs_path);
    if (!set_file)
    {
        return false;
    }

    // load the set type
    INodeValuePtr setid_node = set_file->getChild(L"set_id", false);
    if (!setid_node)
        return false;

    std::wstring set_id = setid_node->getString();
    set_file.clear();
    setid_node.clear();

    // if the set is open by someone, don't allow the delete
    IXdnativeSet* lookup_set = lookupSet(set_id);
    if (lookup_set)
    {
        lookup_set->unref();
        return false;
    }

    // find the set id file

    std::wstring path;
    path.reserve(80);
    path = L"/.system/objects/";
    path += set_id;

    set_file = openNodeFile(path);
    if (!set_file)
    {
        std::wstring fname = ofsToPhysFilename(path, false);
        if (!xf_get_file_exist(fname))
        {
            // missing set id file-- the link to the data file is missing;
            // allow the delete to proceed
            return deleteOfsFile(ofs_path);
        }
        
        return false;
    }

    // get the set type
    INodeValuePtr settype_node = set_file->getChild(L"set_type", false);
    if (!settype_node)
    {
        return false;
    }

    std::wstring set_type = settype_node->getString();
    settype_node.clear();

    if (set_type == L"table")
    {
        // get the ordinal
        INodeValuePtr ordinal_node = set_file->getChild(L"ordinal", false);
        if (!ordinal_node)
        {
            set_file.clear();
            deleteFile(path);
            return false;
        }

        xd::tableord_t ordinal = ordinal_node->getInteger();


        // delete the table file and it's map file
        std::wstring filename = getTableFilename(ordinal);
        if (filename.length() > 0)
        {
            // table file
            xf_remove(filename);

            if (xf_get_file_exist(filename))
                return false;

            // map file
            filename = kl::beforeLast(filename, L'.');
            filename += L".map";

            xf_remove(filename);
        }



        // delete the set's indexes
        INodeValuePtr indexes_node = set_file->getChild(L"indexes", false);
        if (indexes_node)
        {
            int index_count = indexes_node->getChildCount();
            for (int i = 0; i < index_count; ++i)
            {
                INodeValuePtr index = indexes_node->getChildByIdx(i);

                INodeValuePtr idx_filename_node = index->getChild(L"filename", false);
                if (idx_filename_node.isNull())
                    continue;

                std::wstring full_idx_filename;
                full_idx_filename = makePathName(getBasePath(),
                                                 L"data",
                                                 idx_filename_node->getString());
                xf_remove(full_idx_filename);
            }
        }


        // delete the ordinal file
        deleteOrdinal(ordinal);

        // delete the set id file
        set_file.clear();
        deleteFile(path);
    }
     else if (set_type == L"dynamic")
    {
        // get the ordinal
        INodeValuePtr datafile_node = set_file->getChild(L"data_file", false);
        if (datafile_node)
        {
            xf_remove(datafile_node->getString());
        }

        set_file.clear();
        deleteFile(path);
    }
     else if (set_type == L"filter")
    {
        set_file.clear();
        deleteFile(path);
    }


    // finally, delete the set pointer file
    deleteOfsFile(ofs_path);


    // update the global relationship table. note
    // that this routine takes care of its own cleanup
    xd::IRelationEnumPtr rel_enum;
    rel_enum = getRelationEnum(L"");

    return true;
}


bool XdnativeDatabase::setMountPoint(const std::wstring& path,
                             const std::wstring& connection_str,
                             const std::wstring& remote_path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        bool file_exists = xf_get_file_exist(ofsToPhysFilename(path, false));

        if (!file_exists)
        {
            // move takes place in a mount
            xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
            if (db.isNull())
                return false;

            return db->setMountPoint(rpath, connection_str, remote_path);
        }
    }    


    std::wstring final_connection_string;
    
    std::wstring test = kl::beforeFirst(connection_str, '=');  // some connection strings have urls as parameters
    if (test.find(L"://") != -1)
        final_connection_string = xdcommon::urlToConnectionStr(connection_str);
         else
        final_connection_string = connection_str;

    final_connection_string = xdcommon::encryptConnectionStringPassword(final_connection_string);

    
    // try to create or open a file to store the mount point in
    OfsFile* file = OfsFile::createFile(this, path, xd::filetypeNode);
    if (!file)
        return false;

    INodeValuePtr root = file->getRootNode();
    INodeValuePtr cs = root->createChild(L"connection_str");
    cs->setString(final_connection_string);
    
    INodeValuePtr rp = root->createChild(L"remote_path");
    rp->setString(remote_path);
    
    file->unref();

    return true;
}

xd::IDatabasePtr XdnativeDatabase::getMountDatabase(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        return lookupOrOpenMountDb(cstr);
    }
  
    return xcm::null;
}

                    
bool XdnativeDatabase::getMountPoint(const std::wstring& path,
                             std::wstring& connection_str,
                             std::wstring& remote_path)
{
    INodeValuePtr root = openLocalNodeFile(path);
    if (root.isNull())
        return false;
        
    INodeValuePtr cs = root->getChild(L"connection_str", false);
    if (cs.isNull())
        return false;
        
    INodeValuePtr rp = root->getChild(L"remote_path", false);
    if (rp.isNull())
        return false;
        
    connection_str = xdcommon::decryptConnectionStringPassword(cs->getString());
    remote_path = rp->getString();

    return true;
}

bool XdnativeDatabase::createFolder(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->createFolder(rpath);
    }

    OfsFile* f = OfsFile::createFile(this, path, xd::filetypeFolder);

    if (!f)
    {
        return false;
    }

    f->unref();

    return true;
}


INodeValuePtr XdnativeDatabase::createNodeFile(const std::wstring& path)
{
    OfsFile* file = OfsFile::createFile(this, path, xd::filetypeNode);

    if (!file)
    {
        return xcm::null;
    }

    INodeValuePtr result = file->getRootNode();
    file->unref();
    return result;
}

INodeValuePtr XdnativeDatabase::openLocalNodeFile(const std::wstring& path)
{
    {
        KL_AUTO_LOCK(m_objregistry_mutex);

        // lookup in our list to see if the file is already open
        std::vector<OfsFile*>::iterator it;
        for (it = m_ofs_files.begin(); it != m_ofs_files.end(); ++it)
        {
            if (!wcscasecmp((*it)->getPath().c_str(), path.c_str()))
                return (*it)->getRootNode();
        }
    }

    OfsFile* file = OfsFile::openFile(this, path);

    if (!file)
    {
        return xcm::null;
    }

    INodeValuePtr result = file->getRootNode();
    file->unref();
    return result;
}

INodeValuePtr XdnativeDatabase::openNodeFile(const std::wstring& path)
{
    {
        KL_AUTO_LOCK(m_objregistry_mutex);

        // lookup in our list to see if the file is already open
        std::vector<OfsFile*>::iterator it;
        for (it = m_ofs_files.begin(); it != m_ofs_files.end(); ++it)
        {
            if (!wcscasecmp((*it)->getPath().c_str(), path.c_str()))
                return (*it)->getRootNode();
        }
    }


    OfsFile* file = OfsFile::openFile(this, path);

    if (!file)
    {
        return xcm::null;
    }

    INodeValuePtr result = file->getRootNode();
    file->unref();
    return result;
}


bool XdnativeDatabase::renameOfsFile(const std::wstring& _path,
                             const std::wstring& new_name)
{
    bool allow = true;
    
    std::wstring path = _path;
    if (path.find(L'/') == -1)
    {
        path = L"/" + path;
    }
    
    std::wstring dest_path;

    dest_path = kl::beforeLast(path, L'/');
    dest_path += L"/";
    dest_path += new_name;


    std::wstring s1 = ofsToPhysFilename(path, true);
    std::wstring s2 = ofsToPhysFilename(dest_path, true);
    if (xf_get_directory_exist(s1))
    {
        if (!xf_move(s1, s2))
            return false;
    }

    s1 = ofsToPhysFilename(path, false);
    s2 = ofsToPhysFilename(dest_path, false);
    return xf_move(s1, s2);
}


bool XdnativeDatabase::moveOfsFile(const std::wstring& ofs_path,
                           const std::wstring& new_path)
{
    if (getFileExist(new_path))
        return false;

    // make sure base destination path exists
    std::wstring base_dest = kl::beforeLast(new_path, L'/');
    if (base_dest.length() == 0)
    {
        base_dest = L"/";
    }

    if (base_dest != L"/")
    {
        // make sure destination folder exists
        std::wstring real_base = ofsToPhysFilename(base_dest, true);
        if (!xf_get_directory_exist(real_base))
        {
            createFolder(base_dest);
        }
    }


    // get filenames
    std::wstring dest_file = ofsToPhysFilename(new_path, false);
    std::wstring dest_path = ofsToPhysFilename(new_path, true);
    std::wstring src_file = ofsToPhysFilename(ofs_path, false);
    std::wstring src_path = ofsToPhysFilename(ofs_path, true);

    // attempt the move
    bool result;
    result = xf_move(src_file, dest_file);
    if (!result)
    {
        return false;
    }

    if (xf_get_directory_exist(src_path))
    {
        result = xf_move(src_path, dest_path);
        if (!result)
        {
            // try to move the src_file back
            xf_move(dest_file, src_file);
            return false;
        }
    }


    return true;
}






bool XdnativeDatabase::deleteOfsFile(const std::wstring& key_path)
{
    bool res = true;

    // remove key file
    xf_remove(ofsToPhysFilename(key_path, false));

    // remove sub keys
    res = xf_rmtree(ofsToPhysFilename(key_path, true));


    if (m_ofs_root.length() > 0)
    {
        std::wstring temps;

        wchar_t ch = m_ofs_root[m_ofs_root.length()-1];
        if (ch == L'\\' || ch == L'/')
        {
            temps = kl::beforeLast(m_ofs_root, ch);
        }
         else
        {
            temps = m_ofs_root;
        }

        if (!xf_get_file_exist(temps))
        {
            xf_mkdir(temps);
        }
    }

    return res;
}


static bool _copyFile(const std::wstring& src, const std::wstring& dest)
{
    xf_file_t src_file, dest_file;

    src_file = xf_open(src, xfOpen, xfRead, xfShareNone);
    if (!src_file)
    {
        return false;
    }

    xf_remove(dest);

    dest_file = xf_open(dest, xfCreate, xfReadWrite, xfShareNone);
    if (!dest_file)
    {
        xf_close(src_file);
        return false;
    }


    unsigned char* buf = new unsigned char[65536];

    int read_bytes, write_bytes;
    bool result = true;

    while (1)
    {
        read_bytes = xf_read(src_file, buf, 1, 65536);

        write_bytes = xf_write(dest_file, buf, 1, read_bytes);

        if (read_bytes != write_bytes)
        {
            result = false;
            break;
        }

        if (read_bytes != 65536)
        {
            break;
        }
    }


    delete[] buf;

    xf_close(src_file);
    xf_close(dest_file);

    return result;
}



static bool _copyTree(const std::wstring& path,
                      const std::wstring& dest_folder)
{
    std::vector<std::wstring> to_remove;
    std::wstring filename;

    if (path.find_last_not_of(PATH_SEPARATOR_CHAR) == -1)
        return false;

    filename = kl::afterLast(path, PATH_SEPARATOR_CHAR);


    if (!xf_mkdir(makePathName(dest_folder, L"", filename)))
    {
        return false;
    }


    xf_dirhandle_t h = xf_opendir(path);
    xf_direntry_t info;
    while (xf_readdir(h, &info))
    {
        if (info.m_type == xfFileTypeNormal)
        {
            if (!_copyFile(makePathName(path, L"", info.m_name),
                           makePathName(dest_folder, L"", info.m_name)))
            {
                return false;
            }
        }
        if (info.m_type == xfFileTypeDirectory && info.m_name[0] != '.')
        {
            if (!_copyTree(makePathName(path, info.m_name),
                           makePathName(dest_folder, L"", info.m_name)))
            {
                return false;
            }
        }
    }
    xf_closedir(h);

    return true;
}

bool XdnativeDatabase::copyData(const xd::CopyParams* info, xd::IJob* job)
{
    xd::IIteratorPtr iter;
    xd::IStructurePtr structure;

    if (info->iter_input.isOk())
    {
        iter = info->iter_input;
        structure = iter->getStructure();
    }
     else
    {
        xd::IFileInfoPtr finfo = getFileInfo(info->input);
        if (finfo->getType() == xd::filetypeStream)
            return copyFile(info->input, info->output);

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

        if (qp.where.empty())
        {

            if (finfo->getFlags() & xd::sfFastRowCount)
            {
                IJobInternalPtr ijob = job;
                if (ijob)
                    ijob->setMaxCount(finfo->getRowCount());
            }
        }
    }


    
    if (info->append)
    {
        IXdsqlTablePtr output = openTable(info->output);
        if (output.isNull())
            return false;
    }
     else
    {
        deleteFile(info->output);

        xd::FormatDefinition fd = info->output_format;
        fd.columns.clear();
        int i, colcount = structure->getColumnCount();
        for (i = 0; i < colcount; ++i)
            fd.createColumn(structure->getColumnInfoByIdx(i));

        if (!createTable(info->output, fd))
            return false;
    }


    std::wstring cstr, rpath;
    if (detectMountPoint(info->output, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        xdcmnInsert(db, iter, rpath, info->where, info->limit, job);
    }
     else
    {
        xdcmnInsert(static_cast<xd::IDatabase*>(this), iter, info->output, info->where, info->limit, job);
    }

    return true;
}

bool XdnativeDatabase::copyFile(const std::wstring& src_path,
                                const std::wstring& dest_path)
{
    if (src_path.empty() || dest_path.empty())
        return false;

    // find out if we are copying a stream
    xd::IFileInfoPtr info = getFileInfo(src_path);
    if (info.isOk() && info->getType() == xd::filetypeStream)
    {
        xd::IStreamPtr src_stream = openStream(src_path);
        if (src_stream.isNull())
            return false;
        
        if (!createStream(dest_path, info->getMimeType()))
            return false;

        xd::IStreamPtr dest_stream = openStream(dest_path);
        if (dest_stream.isNull())
            return false;

        unsigned char* buf = new unsigned char[16384];

        unsigned long read_bytes, write_bytes;
        bool result = true;

        while (1)
        {
            if (!src_stream->read(buf, 16384, &read_bytes))
                break;
            
            if (read_bytes == 0)    
                break;
                
            if (!dest_stream->write(buf, read_bytes, &write_bytes))
                break;
            
            if (read_bytes != write_bytes)
                break;
        }

        delete[] buf;
        
        return true;
    }
     else
    {
        std::wstring src_file_name = ofsToPhysFilename(src_path, false);
        std::wstring src_dir_name = ofsToPhysFilename(src_path, true);
        std::wstring dest_file_name = ofsToPhysFilename(dest_path, false);
        std::wstring dest_dir_name = ofsToPhysFilename(dest_path, true);

        if (!_copyFile(src_file_name, dest_file_name))
            return false;

        if (!_copyTree(src_dir_name, dest_dir_name))
            return false;
    }
    
    return true;
}


bool XdnativeDatabase::renameFile(const std::wstring& path,
                                  const std::wstring& new_name)
{
    if (path.empty() || new_name.empty())
        return false;


    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath) && !getLocalFileExist(path))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        return db->renameFile(rpath, new_name);
    }
    

    if (!renameOfsFile(path, new_name))
        return false;

    // create a fully-qualified path for the newly renamed object
    std::wstring new_path;

    int slash_pos = path.find_last_of(L'/');
    if (slash_pos != -1)
    {
        new_path = path.substr(0, slash_pos);
        new_path += L"/";
    }
     else
    {
        new_path = L"/";
    }

    new_path += new_name;



    xd::IFileInfoPtr file_info = getFileInfo(new_path);
    if (file_info.isNull())
        return false;

    if (!file_info->isMount())
    {
        int type = file_info->getType();

        if (type == xd::filetypeFolder)
        {
            // it is a folder, so all references underneath must be updated
            recursiveReferenceUpdate(new_path);
        }
         else if (type == xd::filetypeTable)
        {
            // the set reference must be updated
            updateSetReference(new_path);
        }
    }

    return true;
}

bool XdnativeDatabase::moveFile(const std::wstring& _src_path,
                                const std::wstring& _dest_path)
{
    std::wstring src_path = _src_path, dest_path = _dest_path;

    if (src_path.empty() || dest_path.empty() || src_path == dest_path)
        return false;

    if (src_path[0] != '/')
        src_path = L"/" + src_path;
    
    if (dest_path[0] != '/')
        dest_path = L"/" + dest_path;
        

    // if dest_path is an existing folder, the user wants
    // to move src_path while retaining its original name
    // into the existing dest_path folder
    xd::IFileInfoPtr dest_info = getFileInfo(dest_path);
    if (dest_info.isOk() && dest_info->getType() == xd::filetypeFolder)
    {
        std::wstring f = src_path;
        
        // strip any existing trailing slash
        if (f.length() > 0 && f[f.length()-1] == L'/')
            f.erase(f.length()-1, 1);
            
        f = kl::afterLast(f, L'/');
        
        // if necessary, append a slash to the dest path
        if (dest_path.length() == 0 || dest_path[dest_path.length()] != '/')
            dest_path += L"/";
            
        // append original source file
        dest_path += f;
    }


    bool source_item_is_mount = false;
    xd::IFileInfoPtr info = getFileInfo(src_path);
    if (info.isOk())
    {
        source_item_is_mount = info->isMount();
    }


    if (!source_item_is_mount)
    {
        std::wstring src_cstr, src_rpath;
        std::wstring dest_cstr, dest_rpath;
        
        if (detectMountPoint(src_path, &src_cstr, &src_rpath))
            src_path = src_rpath;
            
        if (detectMountPoint(dest_path, &dest_cstr, &dest_rpath))
            dest_path = dest_rpath;
        
        if (src_cstr != dest_cstr)
        {
            // moves between mount points are not allowed
            return false;
        }
        
        if (src_cstr.length() > 0)
        {
            // move takes place in a mount
            xd::IDatabasePtr db = lookupOrOpenMountDb(src_cstr);
            if (db.isNull())
                return false;

            return db->moveFile(src_path, dest_path);
        }
    }
    
    
    if (!moveOfsFile(src_path, dest_path))
        return false;

    xd::IFileInfoPtr file_info = getFileInfo(dest_path);
    if (file_info.isNull())
        return false;

    if (!file_info->isMount())
    {
        int type = file_info->getType();

        if (type == xd::filetypeFolder)
        {
            // it is a folder, so all references underneath
            // must be updated
            recursiveReferenceUpdate(dest_path);
        }
         else if (type == xd::filetypeTable)
        {
            // the set reference must be updated
            updateSetReference(dest_path);
        }
    }

    return true;
}

bool XdnativeDatabase::deleteFile(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // check if the mount is just a single
        // mount to another file
        INodeValuePtr f = openLocalNodeFile(path);
        if (f.isOk())
        {
            f.clear();
            return deleteOfsFile(path);
        }
        
  
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        return db->deleteFile(rpath);
    }
    
    
    
    xd::IFileInfoPtr file_info = getFileInfo(path);
    if (file_info.isNull())
    {
        // no file existed in the first place
        return false;
    }

    if (file_info->isMount())
    {
        return deleteOfsFile(path);
    }

    int type = file_info->getType();

    if (type == xd::filetypeTable)
    {
        return deleteSet(path);
    }
     else if (type == xd::filetypeStream)
    {
        return deleteStream(path);
    }
     else
    {
        return deleteOfsFile(path);
    }
}



bool XdnativeDatabase::getLocalFileExist(const std::wstring& path)
{
    if (path.empty())
        return false;

    return (xf_get_directory_exist(ofsToPhysFilename(path, true)) ||
            xf_get_file_exist(ofsToPhysFilename(path, false)));
}

bool XdnativeDatabase::getFileExist(const std::wstring& path)
{
    // root always exists
    if (path.empty() || path == L"/")
        return true;

    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // root always exists
        if (rpath.empty() || rpath == L"/")
            return true;

        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        return db->getFileExist(rpath);
    }

    return (xf_get_directory_exist(ofsToPhysFilename(path, true)) ||
            xf_get_file_exist(ofsToPhysFilename(path, false)));
}


long long XdnativeDatabase::getFileSize(const std::wstring& ofs_path)
{
    if (ofs_path.empty())
        return 0;

    // fix up set name problems
    std::wstring fixed_name;
    if (ofs_path[0] != L'/')
        fixed_name = L"/";
    fixed_name += ofs_path;
    kl::trimRight(fixed_name);

    // open up set file
    INodeValuePtr set_file = openNodeFile(fixed_name);
    if (!set_file)
        return 0;

    // load the set id
    INodeValuePtr setid_node = set_file->getChild(L"set_id", false);
    if (!setid_node)
        return 0;

    std::wstring set_id = setid_node->getString();
    if (set_id.length() == 0)
        return 0;

    // generate set filename
    std::wstring path;
    path = L"/.system/objects/";
    path += set_id;

    // get filename from registry

    set_file = openNodeFile(path);
    if (!set_file)
        return 0;

    // load ordinal
    INodeValuePtr ordinal_node = set_file->getChild(L"ordinal", false);
    if (!ordinal_node)
        return 0;

    std::wstring table_filename = getTableFilename(ordinal_node->getInteger());

    return xf_get_file_size(table_filename);
}

xd::rowpos_t XdnativeDatabase::getRowCount(const std::wstring& path)
{
    if (path.empty())
        return 0;

    // fix up set name problems
    std::wstring fixed_name;
    if (path[0] != L'/')
        fixed_name = L"/";
    fixed_name += path;
    kl::trimRight(fixed_name);

    // open up set file
    INodeValuePtr set_file = openNodeFile(fixed_name);
    if (!set_file)
        return 0;

    // load the set id
    INodeValuePtr setid_node = set_file->getChild(L"set_id", false);
    if (!setid_node)
        return 0;

    std::wstring set_id = setid_node->getString();
    if (set_id.length() == 0)
        return 0;

    IXdnativeSetPtr set = openSetById(set_id);
    if (set.isNull())
        return 0;

    return set->getRowCount();
}


std::wstring XdnativeDatabase::getFileMimeType(const std::wstring& path)
{
    if (path.empty())
        return L"";
    
    INodeValuePtr file = openNodeFile(path);
    if (!file)
        return L"";

    INodeValuePtr mime_type = file->getChild(L"mime_type", false);
    if (!mime_type)
        return L"";
    
    return mime_type->getString();
}

class XdnativeFileInfo : public xd::IFileInfo
{
    XCM_CLASS_NAME("xdnative.XdnativeFileInfo")
    XCM_BEGIN_INTERFACE_MAP(XdnativeFileInfo)
        XCM_INTERFACE_ENTRY(xd::IFileInfo)
    XCM_END_INTERFACE_MAP()

public:

    XdnativeFileInfo(XdnativeDatabase* _db)
    {
        db = _db;
        db->ref();

        fetched_size = false;
        fetched_mime_type = false;
    }

    virtual ~XdnativeFileInfo()
    {
        db->unref();
    }

    const std::wstring& getName()
    {
        return name;
    }

    int getType()
    {
        return type;
    }
    
    int getFormat()
    {
        return format;
    }

    long long getSize()
    {
        if (fetched_size)
            return size;
        
        size = db->getFileSize(path);
        fetched_size = true;
        return size;
    }

    xd::rowpos_t getRowCount()
    {
        if (type == xd::filetypeTable)
            return db->getRowCount(path);
             else
            return 0;
    }
    
    const std::wstring& getMimeType()
    {
        if (fetched_mime_type)
        {
            return mime_type;
        }
        
        mime_type = db->getFileMimeType(path);
        fetched_mime_type = true;
        return mime_type;
    }

    unsigned int getFlags()
    {
        if (type == xd::filetypeTable)
            return xd::sfFastRowCount;
             else
            return 0;
    }
    
    bool isMount()
    {
        return is_mount;
    }

    bool getMountInfo(std::wstring& _cstr, std::wstring& _rpath)
    {
        if (!is_mount)
        {
            _cstr = L"";
            _rpath = L"";
            return false;
        }

        _cstr = cstr;
        _rpath = rpath;
        return true;
    }

    const std::wstring& getPrimaryKey()
    {
        return primary_key;
    }
    
    const std::wstring& getObjectId()
    {
        if (object_id.length() > 0)
            return object_id;

        object_id = db->getSetIdFromPath(path);
        return object_id;
    }

    const std::wstring& getUrl()
    {
        return url;
    }

public:

    XdnativeDatabase* db;

    std::wstring name;
    std::wstring mime_type;
    std::wstring path;
    std::wstring primary_key;
    std::wstring object_id;
    std::wstring url;
    std::wstring cstr;
    std::wstring rpath;

    long long size;
    int type;
    int format;
    bool is_mount;
    
    bool fetched_mime_type;
    bool fetched_size;
};


bool XdnativeDatabase::setFileType(const std::wstring& path, int type)
{
    KL_AUTO_LOCK(m_objregistry_mutex);

    OfsFile* file = NULL;

    // lookup in our list to see if the file is already open
    std::vector<OfsFile*>::iterator it;
    for (it = m_ofs_files.begin(); it != m_ofs_files.end(); ++it)
    {
        if (!wcscasecmp((*it)->getPath().c_str(), path.c_str()))
        {
            file = *it;
            file->ref();
            break;
        }
    }

    if (file == NULL)
    {
        file = OfsFile::openFile(this, path);

        if (!file)
        {
            return false;
        }
    }

    file->setType(type);
    file->unref();

    return true;
}


bool XdnativeDatabase::getFileType(const std::wstring& path, int* type, bool* is_mount)
{
    if (path.empty() || path == L"/")
    {
        // caller wants file type information for root folder
        if (type)
            *type = xd::filetypeFolder;
        if (is_mount)
            *is_mount = false;
        return true;
    }

    {
        KL_AUTO_LOCK(m_objregistry_mutex);

        OfsFile* file = NULL;

        // lookup in our list to see if the file is already open
        std::vector<OfsFile*>::iterator it;
        for (it = m_ofs_files.begin(); it != m_ofs_files.end(); ++it)
        {
            if (!wcscasecmp((*it)->getPath().c_str(), path.c_str()))
            {
                file = *it;
            
                if (type)
                    *type = (*it)->getType();
                if (is_mount)
                {
                    *is_mount = false;
                    INodeValuePtr root = file->getRootNode();
                    if (root.isOk())
                    {
                        INodeValuePtr cs = root->getChild(L"connection_str", false);
                        if (cs.isOk())
                            *is_mount = true;
                    }
                }
            
                break;
            }
        }
    }


    return OfsFile::getFileType(this, path, type, is_mount);
}


xd::IFileInfoPtr XdnativeDatabase::getFileInfo(const std::wstring& path)
{
    if (path.substr(0, 11) == L"/.temp/.ptr")
    {
        xdcommon::FileInfo* f = new xdcommon::FileInfo;
        f->name = kl::afterLast(path, '/');
        f->type = xd::filetypeTable;
        f->format = xd::formatDefault;
        f->is_mount = false;
        return static_cast<xd::IFileInfo*>(f);
    }


    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // if it's the root, it's automatically a folder
        if (rpath.empty() || rpath == L"/")
        {
            xdcommon::FileInfo* f = new xdcommon::FileInfo;
            f->name = kl::afterLast(path, '/');
            f->type = xd::filetypeFolder;
            f->format = xd::formatDefault;
            f->is_mount = true;
            f->cstr = cstr;
            f->rpath = rpath;
            return static_cast<xd::IFileInfo*>(f);
        }
         else
        {
            std::wstring file_primary_key;
            int file_type = xd::filetypeTable;
            int file_format = xd::formatDefault;
            int is_mount = -1;
            std::wstring file_mime_type;
            std::wstring file_object_id;
            std::wstring file_url;

            if (getLocalFileExist(path))
                is_mount = 1;
                
            xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
            if (db.isOk())
            {
                if (!checkCircularMount(path, db, rpath))
                {
                    xd::IFileInfoPtr file_info = db->getFileInfo(rpath);
                    if (file_info.isOk())
                    {
                        file_type = file_info->getType();
                        file_format = file_info->getFormat();
                        file_primary_key = file_info->getPrimaryKey();
                        file_mime_type = file_info->getMimeType();
                        file_object_id = file_info->getObjectId();
                        file_url = file_info->getUrl();
                        if (is_mount == -1)
                            is_mount = file_info->isMount() ? 1 : 0;
                    }
                }
            }
                       
            xdcommon::FileInfo* f = new xdcommon::FileInfo;
            f->name = kl::afterLast(path, L'/');
            f->type = file_type;
            f->format = file_format;
            f->is_mount = (is_mount == 1 ? true : false);
            f->primary_key = file_primary_key;
            f->mime_type = file_mime_type;
            f->object_id = file_object_id;
            f->url = file_url;
            
            if (f->is_mount)
            {
                f->cstr = cstr;
                f->rpath = rpath;
            }

            return static_cast<xd::IFileInfo*>(f);
        }
    }


    if (!getFileExist(path))
        return xcm::null;


    XdnativeFileInfo* f = new XdnativeFileInfo(this);
    f->format = xd::formatDefault;
    f->type = xd::filetypeNode;
    f->is_mount = false;
    f->path = path;
    
    int slash_pos = path.find_last_of(L'/');
    if (slash_pos == -1)
        f->name = path;
         else
        f->name = path.substr(slash_pos+1);

    getFileType(path, &(f->type), &(f->is_mount));

    return static_cast<xd::IFileInfo*>(f);
}


xd::IDatabasePtr XdnativeDatabase::lookupOrOpenMountDb(const std::wstring& cstr)
{
    if (cstr.empty())
    {
        return static_cast<xd::IDatabase*>(this);
    }


    xd::IDatabasePtr db;
    
    m_mountdbs_mutex.lock();
    std::map<std::wstring, xd::IDatabasePtr>::iterator it;
    it = m_mounted_dbs.find(cstr);
    if (it != m_mounted_dbs.end())
        db = it->second;
    m_mountdbs_mutex.unlock();

    
    
    if (db.isNull())
    {
        db = m_db_mgr->open(cstr);
        
        if (db)
        {
            // tell the mount database to store its temporary files
            // in our temporary directory
            xd::IAttributesPtr attr = db->getAttributes();
            
            if (attr)
            {
                attr->setStringAttribute(xd::dbattrTempDirectory, getTempPath());
                attr->setStringAttribute(xd::dbattrDefinitionDirectory, getDefinitionPath());
            }
        }      
        
        m_mountdbs_mutex.lock();
        m_mounted_dbs[cstr] = db;
        m_mountdbs_mutex.unlock();
    }
    
    return db;
}



bool XdnativeDatabase::detectMountPoint(const std::wstring& path,
                                        std::wstring* connection_str,
                                        std::wstring* remote_path,
                                        std::wstring* mount_root)
{
    // /.system folder never contains mounts
    if (0 == path.compare(0, 8, L"/.system"))
        return false;
    if (0 == path.compare(0, 9, L"/.appdata"))
        return false;

    std::wstring cstr, rpath;

    if (0 == path.compare(0, 7, L"file://"))
    {
        cstr = L"Xdprovider=xdfs;Database=;User ID=;Password=;";
        rpath = path.substr(7);
        if (rpath.empty())
            return false;
        if (rpath.length() >= 3 && rpath[0] == '/' && rpath[2] == ':') // ex: file:///c:/abc.txt
            rpath.erase(0, 1);

        if (connection_str)
            *connection_str = cstr;
        if (remote_path)
            *remote_path = rpath;

        return true;
    }

    if (0 == path.compare(0, 7, L"http://") || 0 == path.compare(0, 8, L"https://") || 0 == path.compare(0, 9, L"sdserv://") || 0 == path.compare(0, 10, L"sdservs://"))
    {
        if (connection_str)
            *connection_str = L"Xdprovider=xdclient;Database=;User ID=;Password=;";
        if (remote_path)
            *remote_path = path;
        return true;
    }

    std::vector<std::wstring> parts;
    std::vector<std::wstring>::iterator it, it2;
    bool found = true;
    

    kl::parseDelimitedList(path, parts, L'/', false);
    
    std::wstring fpath = L"/";
    
    // delete any empty parts
    int i = 0;
    for (i = 0; i < (int)parts.size(); ++i)
    {
        if (parts[i].empty())
        {
            parts.erase(parts.begin() + i);
            --i;
        }
    }
    
    for (it = parts.begin(); it != parts.end(); ++it)
    {
        if (fpath.empty() || fpath[fpath.length()-1] != L'/')
            fpath += L"/";
        fpath += *it;

        bool is_mount = false;
        OfsFile::getFileType(this, fpath, NULL, &is_mount);
        if (!is_mount)
            continue;
            

        INodeValuePtr val = openLocalNodeFile(fpath);
        if (val.isNull())
            break;
            
        INodeValuePtr cs = val->getChild(L"connection_str", false);
        if (cs.isNull())
            continue;
            
        INodeValuePtr rp = val->getChild(L"remote_path", false);
        if (rp.isNull())
            continue;
        
        if (cs.isOk() && rp.isOk())
        {
            cstr = xdcommon::decryptConnectionStringPassword(cs->getString());
            rpath = rp->getString();

            for (it2 = it+1; it2 < parts.end(); ++it2)
            {
                if (rpath.empty() || rpath[rpath.length()-1] != '/')
                {
                    rpath += L'/';
                }
                
                rpath += *it2;
                if (it2+1 < parts.end())
                    rpath += L'/';
            }
            
            if (connection_str)
                *connection_str = cstr;
            if (remote_path)
                *remote_path = rpath;
            if (mount_root)
                *mount_root = fpath;

            return true;
        }
    }
    
    return false;
}

/*
bool XdnativeDatabase::checkCircularMount(const std::wstring& path,
                                  xd::IDatabasePtr remote_db, 
                                  const std::wstring remote_path)
{
    if (static_cast<xd::IDatabase*>(this) != remote_db.p)
        return false;
        
    return (0 == wcscasecmp(remote_path.c_str(), path.c_str())) ? true : false;
}
*/



bool XdnativeDatabase::checkCircularMountInternal(std::set<std::wstring, kl::cmp_nocase>& bad_paths,
                                          xd::IDatabasePtr remote_db, 
                                          const std::wstring remote_path)
{
    // if remote is a different db, it's not circular
    if (static_cast<xd::IDatabase*>(this) != remote_db.p)
        return false;
        
    if (bad_paths.find(remote_path) != bad_paths.end())
        return true;
       
    std::wstring cstr, rpath;
    if (!detectMountPoint(remote_path, &cstr, &rpath))
    {
        // remote is not a reference itself, so not circular
        return false;
    }
    
    if (cstr.length() > 0)
    {
        // there's a connection string, which means that
        // the remote database is different than |this|,
        // so we'll return false for circular mount
        return false;
    }
    
    xd::IDatabasePtr rdb = lookupOrOpenMountDb(cstr);
    bad_paths.insert(remote_path);
    
    return checkCircularMountInternal(bad_paths, rdb, rpath);
}




bool XdnativeDatabase::checkCircularMount(const std::wstring& path,
                                  xd::IDatabasePtr remote_db, 
                                  const std::wstring remote_path)
{
    std::set<std::wstring, kl::cmp_nocase> bad_paths;
    bad_paths.insert(path);
    
    return checkCircularMountInternal(bad_paths, remote_db, remote_path);
}



xd::IFileInfoEnumPtr XdnativeDatabase::getFolderInfo(const std::wstring& _mask)
{ 
    xcm::IVectorImpl<xd::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;
    
    // if the parameter is empty, assume they want the root folder
    std::wstring mask = _mask;
    if (mask.length() == 0)
        mask = L"/";


    // detect if the specified folder is a mount point
    
    std::wstring cstr, rpath;
    
    if (detectMountPoint(mask, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return retval;
            
        delete retval;
        return db->getFolderInfo(rpath);
    }




    // find the physical directory
    std::wstring dir = ofsToPhysFilename(mask, true);


    // find the base ofs directory (to be prepended to
    // a filename in order to create a full path)
    std::wstring base_ofs_path;
    if (mask[0] != L'/')
    {
        base_ofs_path = L"/";
    }
    base_ofs_path += mask;
    if (base_ofs_path[base_ofs_path.length()-1] != L'/')
    {
        base_ofs_path += L"/";
    }


    // get directory entries, if this key is a folder

    xf_dirhandle_t h = xf_opendir(dir);
    xf_direntry_t info;

    while (xf_readdir(h, &info))
    {
        if (info.m_type == xfFileTypeNormal)
        {
            std::wstring file_name;

            int dot_pos = info.m_name.find_last_of(L'.');

            if (dot_pos <= 0)
                file_name = info.m_name;
                 else
                file_name = info.m_name.substr(0, dot_pos);


            //  get file type from getFileInfo()
            std::wstring ofs_path = base_ofs_path;
            ofs_path += file_name;

            xd::IFileInfoPtr file_info = getFileInfo(ofs_path);
            if (file_info.isOk())
            {
                retval->append(file_info);
            }
        }

    }

    xf_closedir(h);

    return retval;
}



void XdnativeDatabase::registerNodeFile(OfsFile* file)
{
    KL_AUTO_LOCK(m_objregistry_mutex);

    m_ofs_files.push_back(file);
}

void XdnativeDatabase::unregisterNodeFile(OfsFile* file)
{
    KL_AUTO_LOCK(m_objregistry_mutex);

    std::vector<OfsFile*>::iterator it;
    for (it = m_ofs_files.begin(); it != m_ofs_files.end(); ++it)
    {
        if (*it == file)
        {
            m_ofs_files.erase(it);
            return;
        }
    }
}

void XdnativeDatabase::registerSet(IXdnativeSet* set)
{
    KL_AUTO_LOCK(m_objregistry_mutex);

    m_sets.push_back(set);
}

void XdnativeDatabase::unregisterSet(IXdnativeSet* set)
{
    KL_AUTO_LOCK(m_objregistry_mutex);

    std::vector<IXdnativeSet*>::iterator it;
    for (it = m_sets.begin(); it != m_sets.end(); ++it)
    {
        if (*it == set)
        {
            m_sets.erase(it);
            return;
        }   
    }
}

void XdnativeDatabase::registerTable(ITable* table)
{
    KL_AUTO_LOCK(m_objregistry_mutex);

    m_tables.push_back(table);
}

void XdnativeDatabase::unregisterTable(ITable* table)
{
    KL_AUTO_LOCK(m_objregistry_mutex);

    std::vector<ITable*>::iterator it;
    for (it = m_tables.begin(); it != m_tables.end(); ++it)
    {
        if (*it == table)
        {
            m_tables.erase(it);
            return;
        }
    }
}


bool XdnativeDatabase::deleteTempData()
{
    if (m_base_dir.length() == 0)
        return false;

    // get directory entries, if this key is a folder
    xf_dirhandle_t h = xf_opendir(makePathName(m_base_dir, L"temp"));
    xf_direntry_t info;
    while (xf_readdir(h, &info))
    {
        if (info.m_type == xfFileTypeNormal)
        {
            xf_remove(makePathName(m_base_dir, L"temp", info.m_name));
        }
    }

    xf_closedir(h);

    return true;
}






void XdnativeDatabase::getFolderUsedOrdinals(const std::wstring& folder_path,
                                             std::set<int>& used_ordinals)
{
    // skip mounts
    xd::IFileInfoPtr fileinfo = getFileInfo(folder_path);
    if (fileinfo.isNull() || fileinfo->isMount())
        return;
    

    xd::IFileInfoEnumPtr files = getFolderInfo(folder_path);
    xd::IFileInfoPtr info;

    int file_count = files->size();
    int i;

    for (i = 0; i < file_count; ++i)
    {
        info = files->getItem(i);
        int file_type = info->getType();
        
        // skip mounts
        if (info->isMount())
            continue;

        std::wstring path = combineOfsPath(folder_path, info->getName());


        if (file_type == xd::filetypeTable)
        {
            INodeValuePtr set_file = openNodeFile(path);
            if (set_file)
            {
                INodeValuePtr setid_node;
                setid_node = set_file->getChild(L"set_id", false);

                if (!setid_node)
                    continue;
    
                std::wstring set_id = setid_node->getString();

                path = L"/.system/objects/";
                path += set_id;

                INodeValuePtr file;
                file = openNodeFile(path);

                if (file)
                {
                    INodeValuePtr setid_node;
                    setid_node = file->getChild(L"ordinal", false);
                    if (!setid_node)
                        continue;

                    used_ordinals.insert(setid_node->getInteger());
                }
            }
        }

        getFolderUsedOrdinals(path, used_ordinals);
    }
}



bool XdnativeDatabase::cleanup()
{
    std::set<int> used_ordinals;
    int max_ordinal = (int)getMaximumOrdinal();

    getFolderUsedOrdinals(L"/", used_ordinals);

    int i;
    std::wstring filename;

    for (i = 0; i < max_ordinal; ++i)
    {
        if (used_ordinals.find(i) == used_ordinals.end())
        {
            bool ok = true;

            filename = getTableFilename(i);
            if (filename.length() > 0)
            {
                ok = (xf_remove(filename) == 0 ? true : false);
            }

            if (ok)
            {
                deleteOrdinal(i);
            }
        }
    }

    deleteTempData();
    emptyTrash();

    return true;
}


xd::IStreamPtr XdnativeDatabase::openStream(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->openStream(rpath);
    }


    // see if the file is a node file; if it is, open the
    // stream from the node
    xd::IFileInfoPtr info = getFileInfo(path);
    if (info.isOk() && info->getType() == xd::filetypeNode)
    {
        NodeFileStream* stream = new NodeFileStream(this);
        if (!stream->open(path))
        {
            delete stream;
            return xcm::null;
        }

        return static_cast<xd::IStream*>(stream);
    }


    // not a node file; open as a regular stream
    std::wstring stream_filename = getStreamFilename(path);
    if (stream_filename.length() == 0)
        return xcm::null;

    FileStream* stream = new FileStream;
    if (!stream->open(stream_filename))
    {
        delete stream;
        return xcm::null;
    }

    return static_cast<xd::IStream*>(stream);
}

bool XdnativeDatabase::createStream(const std::wstring& path, const std::wstring& mime_type)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        return db->createStream(rpath, mime_type);
    }



    std::wstring stream_fname = getStreamFilename(path);
    if (stream_fname.length() > 0)
    {
        // change mime_type in stream info file
        OfsFile* file = OfsFile::openFile(this, path);
        if (file)
        {
            INodeValuePtr root = file->getRootNode();
            INodeValuePtr mime_type_node = root->getChild(L"mime_type", true);
            mime_type_node->setString(mime_type);
            file->unref();
        }
        
        if (xf_remove(stream_fname))
        {
            // just overwrite old file
            FileStream* file_stream = new FileStream;
            if (!file_stream->create(stream_fname))
            {
                delete file_stream;
                return xcm::null;
            }

            delete file_stream;
            return true;
        }
         else
        {
            // mark the stream file which is in use for later deletion;
            // create a new pointer file point to a new stream location
            addFileToTrash(stream_fname);
        }
    }
    

    // normal xdnative stream
    if (getFileExist(path))
    {
        deleteFile(path);
    }
    
    
    // create or open an ofs node file
    OfsFile* file = OfsFile::createFile(this, path, xd::filetypeStream);
    if (!file)
    {
        return false;
    }

    INodeValuePtr root = file->getRootNode();
    
    // create a new object id
    std::wstring object_id = getUniqueString();
    
    // set object id in ofs node file
    INodeValuePtr object_id_node = root->getChild(L"object_id", true);
    object_id_node->setString(object_id);
    
    INodeValuePtr mime_type_node = root->getChild(L"mime_type", true);
    mime_type_node->setString(mime_type);
    
    object_id_node.clear();
    mime_type_node.clear();
    file->unref();
    
    // create an object entry
    std::wstring object_path;
    object_path.reserve(80);
    object_path = L"/.system/objects/";
    object_path += object_id;

    root = createNodeFile(object_path);
    if (!root)
    {
        return xcm::null;
    }
    
    
    // create a filename for the data file
    std::wstring data_filename = getUniqueString();
    data_filename += L".dat";
    
    
    // add entry pointing to data file
    INodeValuePtr data_file_node = root->getChild(L"data_file", true);
    if (!data_file_node)
    {
        return xcm::null;
    }
    
    data_file_node->setString(data_filename);


    // create data file stream
    std::wstring full_path = makePathName(m_base_dir, L"data", data_filename);
    FileStream* file_stream = new FileStream;
    if (!file_stream->create(full_path))
    {
        delete file_stream;
        return false;
    }

    delete file_stream;
    return true;
}



IXdnativeSet* XdnativeDatabase::lookupSet(const std::wstring& set_id)
{
    KL_AUTO_LOCK(m_objregistry_mutex);

    // search existing sets for a match
    std::vector<IXdnativeSet*>::iterator sit;
    for (sit = m_sets.begin(); sit != m_sets.end(); ++sit)
    {
        if (!wcscasecmp((*sit)->getSetId().c_str(), set_id.c_str()))
        {
            (*sit)->ref();
            return (*sit)->getRawXdnativeSetPtr();
        }
    }

    return NULL;
}

bool XdnativeDatabase::createTable(const std::wstring& path, const xd::FormatDefinition& format_definition)
{
    if (path.length() == 0)
        return false;

    if (getFileExist(path))
        return false;  // already exists

    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->createTable(rpath, format_definition);
    }


    TableSet* set = new TableSet(this);
    set->ref();

    if (!set->create(format_definition, path))
    {
        set->unref();
        return false;
    }

    set->setTemporary(false);
    set->unref();
    return true;
}

int XdnativeDatabase::getDatabaseType()
{
    return xd::dbtypeXdnative;
}

std::wstring XdnativeDatabase::getStreamFilename(const std::wstring& ofs_path)
{    
    INodeValuePtr file = openNodeFile(ofs_path);
    if (!file)
        return L"";

    INodeValuePtr object_id_node = file->getChild(L"object_id", false);
    if (!object_id_node)
        return L"";

    std::wstring object_id = object_id_node->getString();
    if (object_id.length() == 0)
        return L"";
        
    file.clear();
    
    std::wstring object_path;
    object_path.reserve(80);
    object_path = L"/.system/objects/";
    object_path += object_id;

    file = openNodeFile(object_path);
    if (!file)
        return L"";
        
    INodeValuePtr data_file_node = file->getChild(L"data_file", false);
    if (!data_file_node)
        return L"";

    std::wstring data_file = data_file_node->getString();
    if (data_file.length() == 0)
        return L"";

    return makePathName(m_base_dir, L"data", data_file);
}


std::wstring XdnativeDatabase::getTableFilename(xd::tableord_t table_ordinal)
{
    wchar_t ord_key_name[64];
    swprintf(ord_key_name, 64, L"/.system/ordinals/%d", table_ordinal);

    INodeValuePtr ordinal = openNodeFile(ord_key_name);
    if (!ordinal)
        return L"";

    INodeValuePtr target_file = ordinal->getChild(L"target_file", false);
    if (!target_file)
        return L"";

    std::wstring filename = target_file->getString();

    if (filename.find(PATH_SEPARATOR_CHAR) != -1)
        return filename;

    return makePathName(m_base_dir, L"data", filename);
}


ITablePtr XdnativeDatabase::openTableByOrdinal(xd::tableord_t table_ordinal)
{
    {
        KL_AUTO_LOCK(m_objregistry_mutex);

        // see if the table is already opened

        std::vector<ITable*>::iterator it;
        for (it = m_tables.begin(); it != m_tables.end(); ++it)
        {
            if ((*it)->getTableOrdinal() == table_ordinal)
            {
                return *it;
            }
        }
    }
    
    // table is not already opened, so open it

    std::wstring table_filename = getTableFilename(table_ordinal);

    if (table_filename.empty() == 0)
        return xcm::null;

    if (!xf_get_file_exist(table_filename))
        return xcm::null;

    // find file extension
    int ext_pos = table_filename.find_last_of(L'.');
    if (ext_pos == -1)
        return xcm::null;
    
    std::wstring ext = table_filename.substr(ext_pos+1);

    if (!wcscasecmp(ext.c_str(), L"ttb"))
    {
        NativeTable* table = new NativeTable(this);
        if (!table->open(table_filename, table_ordinal))
        {
            delete table;
            return xcm::null;
        }
        return static_cast<ITable*>(table);
    }

    return xcm::null;
}



IXdnativeSetPtr XdnativeDatabase::openSetById(const std::wstring& set_id)
{
    // check if the set is locked
    if (getSetLocked(set_id))
    {
        return xcm::null;
    }

    // check if the set is already open
    IXdnativeSet* lookup_set = lookupSet(set_id);
    if (lookup_set)
    {
        // result is already ref'ed by lookupSet();
        IXdnativeSetPtr ret(lookup_set, false);
        return ret;
    }


    std::wstring path;
    path.reserve(80);
    path = L"/.system/objects/";
    path += set_id;

    // get filename from registry

    INodeValuePtr set_file = openNodeFile(path);
    if (!set_file)
    {
        return xcm::null;
    }

    // load the set type
    INodeValuePtr settype_node = set_file->getChild(L"set_type", false);
    if (!settype_node)
        return xcm::null;

    std::wstring set_type = settype_node->getString();

    if (set_type == L"table")
    {
        TableSet* set = new TableSet(this);
        set->ref();
        if (!set->load(set_file))
        {
            set->unref();
            return xcm::null;
        }

        return IXdnativeSetPtr(static_cast<IXdnativeSet*>(set), false);
    }

    return xcm::null;
}



std::wstring XdnativeDatabase::getSetIdFromPath(const std::wstring& set_path)
{
    if (set_path.empty())
        return L"";

    // fix up set name problems
    std::wstring fixed_name;

    if (*(set_path.c_str()) != L'/')
        fixed_name += L"/";

    fixed_name += set_path;
    kl::trim(fixed_name);

    // open up set file

    INodeValuePtr set_file = openNodeFile(fixed_name);
    if (!set_file)
        return L"";

    // load the set id
    INodeValuePtr setid_node = set_file->getChild(L"set_id", false);
    if (!setid_node)
        return L"";

    return setid_node->getString();
}


std::wstring XdnativeDatabase::getSetPathFromId(const std::wstring& set_id)
{
    std::wstring path;
    path.reserve(80);
    path = L"/.system/objects/";
    path += set_id;

    INodeValuePtr setid_file = openNodeFile(path);
    if (setid_file.isNull())
        return L"";

    INodeValuePtr ofspath_value = setid_file->getChild(L"ofs_path", true);
    if (ofspath_value.isNull())
        return L"";

    return ofspath_value->getString();
}




IXdsqlTablePtr XdnativeDatabase::openTable(const std::wstring& path)
{
    // check for ptr sets
    if (path.substr(0, 12) == L"/.temp/.ptr/")
    {
        std::wstring ptr_string = kl::afterLast(path, L'/');
        unsigned long l = (unsigned long)kl::hexToUint64(ptr_string.c_str());
        IXdsqlTable* sptr = (IXdsqlTable*)l;
        return sptr;
    }

    std::wstring set_id = getSetIdFromPath(path);
    if (set_id.empty())
        return xcm::null;

    return openSetById(set_id);
}


xd::IIteratorPtr XdnativeDatabase::query(const xd::QueryParams& qp)
{
    std::wstring cstr, rpath, mount_root;
    if (detectMountPoint(qp.from, &cstr, &rpath, &mount_root))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        xd::QueryParams call_params = qp;
        call_params.from = rpath;
        
        xd::IIteratorPtr ret = db->query(call_params);
        if (ret.isOk())
        {
            std::wstring tbl = ret->getTable();
            if (tbl.length() > 0)
            {
                tbl = xd::appendPath(mount_root, tbl);
                ret->setTable(tbl);
            }

            return ret;
        }
    }

    if (qp.where.length() > 0)
    {
        // create an iterator for the table we are interested in
        IXdsqlTablePtr table = openTable(qp.from);
        if (table.isNull())
            return xcm::null;
        xd::IIteratorPtr iter = table->createIterator(L"", L"", NULL);
        if (iter.isNull())
            return xcm::null;

        // create a filtered record set
        CommonDynamicSet* dynset = new CommonDynamicSet;
        dynset->ref();
        if (!dynset->create(static_cast<xd::IDatabase*>(this), table))
        {
            // error.setError(xd::errorGeneral, L"Unable to process WHERE clause");
            dynset->unref();
            return xcm::null;
        }
        
        // insert all rows meeting expression
        int res = dynset->insert(iter, qp.where, 0, qp.job);
        if (res == -1)
        {
            // error.setError(xd::errorGeneral, L"Unable to process WHERE clause");
            dynset->unref();
            return xcm::null;
        }
        
        xd::IIteratorPtr result_iter = dynset->createIterator(qp.columns, qp.order, NULL);
        dynset->unref();
        return result_iter;
    }


    IXdsqlTablePtr table = openTable(qp.from);
    if (table.isNull())
        return xcm::null;
    return table->createIterator(qp.columns, qp.order, qp.job);
}

bool XdnativeDatabase::groupQuery(xd::GroupQueryParams* info, xd::IJob* job)
{
    return xdcommon::runGroupQuery(static_cast<xd::IDatabase*>(this), info, job);
}


bool XdnativeDatabase::lockSet(const std::wstring& set_id)
{
    KL_AUTO_LOCK(m_lockedsets_mutex);

    std::vector<std::wstring>::iterator it;
    it = std::find(m_locked_sets.begin(), m_locked_sets.end(), set_id);
    if (it != m_locked_sets.end())
        return false;

    m_locked_sets.push_back(set_id);
    return true;
}

bool XdnativeDatabase::unlockSet(const std::wstring& set_id)
{
    KL_AUTO_LOCK(m_lockedsets_mutex);

    std::vector<std::wstring>::iterator it;
    it = std::find(m_locked_sets.begin(), m_locked_sets.end(), set_id);
    if (it == m_locked_sets.end())
        return false;

    m_locked_sets.erase(it);
    return true;
}


bool XdnativeDatabase::getSetLocked(const std::wstring& set_id)
{
    KL_AUTO_LOCK(m_lockedsets_mutex);

    std::vector<std::wstring>::iterator it;
    it = std::find(m_locked_sets.begin(), m_locked_sets.end(), set_id);
    if (it == m_locked_sets.end())
        return false;

    return true;
}


class XdnativeRelationInfo : public xd::IRelation,
                             public IRelationInternal
{
    XCM_CLASS_NAME("xdnative.XdnativeRelationInfo")
    XCM_BEGIN_INTERFACE_MAP(XdnativeRelationInfo)
        XCM_INTERFACE_ENTRY(xd::IRelation)
        XCM_INTERFACE_ENTRY(IRelationInternal)
    XCM_END_INTERFACE_MAP()

public:

    XdnativeRelationInfo(XdnativeDatabase* db)
    {
        // XdnativeRelationInfo does not need to call ref() on the database object
        // since references themselves are held by the db
        m_db = db;
    }

    void setRelationId(const std::wstring& new_val)
    {
        m_relation_id = new_val;
    }

    const std::wstring& getRelationId()
    {
        return m_relation_id;
    }

    void setTag(const std::wstring& new_val)
    {
        m_tag = new_val;
    }

    const std::wstring& getTag()
    {
        return m_tag;
    }

    void setLeftSet(const std::wstring& new_value)
    {
        m_left_path = new_value;
        m_left_setid = L"";
    }
    
    std::wstring getLeftTable()
    {
        if (m_left_path.length() > 0)
            return m_left_path;
             else
            return m_db->getSetPathFromId(m_left_setid);
    }

    void setLeftSetId(const std::wstring& new_value)
    {
        m_left_path = L"";
        m_left_setid = new_value;
    }

    std::wstring getLeftSetId()
    {
        return m_left_setid;
    }

    void setLeftExpression(const std::wstring& new_value)
    {
        m_left_expression = new_value;
    }

    std::wstring getLeftExpression()
    {
        return m_left_expression;
    }


    
    void setRightSet(const std::wstring& new_value)
    {
        m_right_path = new_value;
        m_right_setid = L"";
    }

    void setRightSetId(const std::wstring& new_value)
    {
        m_right_path = L"";
        m_right_setid = new_value;
    }
    
    std::wstring getRightSetId()
    {
        return m_right_setid;
    }

    std::wstring getRightTable()
    {
        if (m_right_path.length() > 0)
            return m_right_path;
             else
            return m_db->getSetPathFromId(m_right_setid);
    }

    void setRightExpression(const std::wstring& new_value)
    {
        m_right_expression = new_value;
    }

    std::wstring getRightExpression()
    {
        return m_right_expression;
    }


private:

    XdnativeDatabase* m_db;
    std::wstring m_relation_id;
    std::wstring m_tag;

    std::wstring m_left_path;
    std::wstring m_left_setid;
    std::wstring m_left_expression;

    std::wstring m_right_path;
    std::wstring m_right_setid;
    std::wstring m_right_expression;
};


xd::IRelationEnumPtr XdnativeDatabase::getRelationEnum(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IRelationPtr>* vec;
    vec = new xcm::IVectorImpl<xd::IRelationPtr>;

    bool filter_active = path.length() > 0 ? true : false;
    std::wstring filter_set_id = getSetIdFromPath(path);
    std::wstring filter_set_path = path;
    

    INodeValuePtr file;

    if (!getFileExist(L"/.system/rel_table"))
        return vec;

    file = openNodeFile(L"/.system/rel_table");
    if (!file)
        return vec;

    size_t i, count = file->getChildCount();

    std::wstring tag, left_set_id, right_set_id, left_set_path, right_set_path, left_expr, right_expr;

    INodeValuePtr rel_node, left_setid_node, right_setid_node, left_setpath_node, right_setpath_node,
                        left_expr_node, right_expr_node, tag_node;


    std::vector<std::wstring> to_delete;

    for (i = 0; i < count; ++i)
    {
        INodeValuePtr rel_node = file->getChildByIdx(i);

        tag_node = rel_node->getChild(L"tag", false);
        if (!tag_node)
            continue;
        tag = tag_node->getString();

        left_setid_node = rel_node->getChild(L"left_set_id", false);
        if (!left_setid_node)
            continue;
        left_set_id = left_setid_node->getString();

        left_setpath_node = rel_node->getChild(L"left_set_path", false);
        if (left_setpath_node.isOk())
            left_set_path = left_setpath_node->getString();
             else
            left_set_path = L"";

        right_setid_node = rel_node->getChild(L"right_set_id", false);
        if (!right_setid_node)
            continue;
        right_set_id = right_setid_node->getString();

        right_setpath_node = rel_node->getChild(L"right_set_path", false);
        if (right_setpath_node.isOk())
            right_set_path = right_setpath_node->getString();
             else
            right_set_path = L"";

        left_expr_node = rel_node->getChild(L"left_expression", false);
        if (!left_expr_node)
            continue;
        left_expr = left_expr_node->getString();

        right_expr_node = rel_node->getChild(L"right_expression", false);
        if (!right_expr_node)
            continue;
        right_expr = right_expr_node->getString();

        if (filter_active)
        {
            if (filter_set_id.length() > 0 && filter_set_id != left_set_id)
                continue;

            if (filter_set_path.length() > 0 && left_set_path.length() > 0 && 0 != wcscasecmp(filter_set_path.c_str(), left_set_path.c_str()))
                continue;
        }

        // check to make sure both the left set and the right set still exist
        std::wstring temps;
        
        temps = getSetPathFromId(left_set_id);
        if (temps.length() == 0 && left_set_id.length() > 0)
        {
            to_delete.push_back(rel_node->getName());
            continue;
        }

        temps = getSetPathFromId(right_set_id);
        if (temps.length() == 0 && right_set_id.length() > 0)
        {
            to_delete.push_back(rel_node->getName());
            continue;
        }

        XdnativeRelationInfo* relation;
        relation = new XdnativeRelationInfo(this);
        relation->setRelationId(rel_node->getName());
        relation->setTag(tag);
        if (left_set_id.length() > 0)
            relation->setLeftSetId(left_set_id);
             else
            relation->setLeftSet(left_set_path);
        if (right_set_id.length() > 0)
            relation->setRightSetId(right_set_id);
             else
            relation->setRightSet(right_set_path);
        relation->setLeftExpression(left_expr);
        relation->setRightExpression(right_expr);

        xd::IRelationPtr r = static_cast<xd::IRelation*>(relation);
        vec->append(r);
    }


    // now delete entries which are invalid
    std::vector<std::wstring>::iterator it;
    for (it = to_delete.begin(); it != to_delete.end(); ++it)
        file->deleteChild(*it);

    return vec;
}

xd::IRelationPtr XdnativeDatabase::getRelation(const std::wstring& relation_id)
{
    xd::IRelationEnumPtr rel_enum = getRelationEnum(L"");
    size_t i, rel_count = rel_enum->size();
    for (i = 0; i < rel_count; ++i)
    {
        if (rel_enum->getItem(i)->getRelationId() == relation_id)
            return rel_enum->getItem(i);
    }

    return xcm::null;
}

xd::IRelationPtr XdnativeDatabase::createRelation(const std::wstring& tag,
                                                  const std::wstring& left_set_path,
                                                  const std::wstring& right_set_path,
                                                  const std::wstring& left_expr,
                                                  const std::wstring& right_expr)
{    
    INodeValuePtr root;

    if (!getFileExist(L"/.system/rel_table"))
        root = createNodeFile(L"/.system/rel_table");
         else
        root = openNodeFile(L"/.system/rel_table");

    if (!root)
        return xcm::null;

    std::wstring rel_id = getUniqueString();

    std::wstring left_set_id = getSetIdFromPath(left_set_path);
    std::wstring right_set_id = getSetIdFromPath(right_set_path);

    // create a new entry in our relationship table file
    INodeValuePtr rel_node, left_setid_node, right_setid_node,
                        left_expr_node, right_expr_node, tag_node;

    rel_node = root->getChild(rel_id, true);
    if (rel_node.isNull())
        return xcm::null;

    tag_node = rel_node->getChild(L"tag", true);
    tag_node->setString(tag);

    left_setid_node = rel_node->getChild(L"left_set_id", true);
    left_setid_node->setString(left_set_id);

    left_setid_node = rel_node->getChild(L"left_set_path", true);
    left_setid_node->setString(left_set_path);

    right_setid_node = rel_node->getChild(L"right_set_id", true);
    right_setid_node->setString(right_set_id);

    right_setid_node = rel_node->getChild(L"right_set_path", true);
    right_setid_node->setString(right_set_path);

    left_expr_node = rel_node->getChild(L"left_expression", true);
    left_expr_node->setString(left_expr);

    right_expr_node = rel_node->getChild(L"right_expression", true);
    right_expr_node->setString(right_expr);


    // create a new relationship object to return to the caller
    XdnativeRelationInfo* relation = new XdnativeRelationInfo(this);
    relation->setRelationId(rel_id);
    relation->setTag(tag);
    relation->setLeftExpression(left_expr);
    relation->setRightExpression(right_expr);

    if (left_set_id.length() > 0)
        relation->setLeftSetId(left_set_id);
         else
        relation->setLeftSet(left_set_path);

    if (right_set_id.length() > 0)
        relation->setRightSetId(right_set_id);
         else
        relation->setRightSetId(right_set_path);

    IXdsqlTablePtr left_table = openTable(left_set_path);
    if (left_table.isOk())
    {
        IXdnativeSetPtr set_int = left_table;
        if (set_int)
            set_int->onRelationshipsUpdated();
    }

    return static_cast<xd::IRelation*>(relation);
}


bool XdnativeDatabase::deleteRelation(const std::wstring& relation_id)
{
    xd::IRelationPtr rel = getRelation(relation_id);
    if (rel.isNull())
        return false;

    INodeValuePtr root;

    if (!getFileExist(L"/.system/rel_table"))
        return false;

    root = openNodeFile(L"/.system/rel_table");

    if (!root)
        return false;
    

    if (!root->deleteChild(relation_id))
        return false;

    
    std::wstring left_table_path = rel->getLeftTable();
    IXdsqlTablePtr left_table = openTable(left_table_path);
    if (left_table)
    {
        IXdnativeSetPtr set_int = left_table;
        if (set_int)
            set_int->onRelationshipsUpdated();
    }


    return true;
}




xd::IndexInfo XdnativeDatabase::createIndex(const std::wstring& path,
                                            const std::wstring& name,
                                            const std::wstring& expr,
                                            xd::IJob* job)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xd::IndexInfo();

        std::vector<std::wstring> cols;
        std::vector<std::wstring>::iterator it;
        kl::parseDelimitedList(expr, cols, L',', true);
        std::wstring newexpr;
        for (it = cols.begin(); it != cols.end(); ++it)
        {
            if (newexpr.length() > 0)
                newexpr += L",";
            dequote(*it, '[', ']');
            kl::replaceStr(*it, L",", L"");
            newexpr += *it;
        }

        return db->createIndex(rpath, name, newexpr, job);
    }

    IXdnativeSetPtr set_int = openTable(path);
    if (set_int.isNull())
        return xd::IndexInfo();

    return set_int->createIndex(name, expr, job);
}


bool XdnativeDatabase::renameIndex(const std::wstring& path,
                           const std::wstring& name,
                           const std::wstring& new_name)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->renameIndex(rpath, name, new_name);
    }

    IXdnativeSetPtr set_int = openTable(path);
    if (set_int.isNull())
        return false;

    return set_int->renameIndex(name, new_name);
}


bool XdnativeDatabase::deleteIndex(const std::wstring& path,
                                   const std::wstring& name)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->deleteIndex(rpath, name);
    }

    IXdnativeSetPtr set_int = openTable(path);
    if (set_int.isNull())
        return xcm::null;

    return set_int->deleteIndex(name);
}


xd::IndexInfoEnum XdnativeDatabase::getIndexEnum(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xd::IndexInfoEnum();

        return db->getIndexEnum(rpath);
    }

    IXdnativeSetPtr set_int = openTable(path);
    if (set_int.isNull())
    {
        // IXdnativeSet not supported -- return no indexes
        return xd::IndexInfoEnum();
    }
    
    return set_int->getIndexEnum();
}

xd::Structure XdnativeDatabase::describeTable(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xd::Structure();

        return db->describeTable(rpath);
    }

    IXdsqlTablePtr table = openTable(path);
    if (table.isNull())
        return xd::Structure();

    xd::IStructurePtr s = table->getStructure();
    return s.isOk() ? s->toStructure() : xd::Structure();
}


xd::IRowInserterPtr XdnativeDatabase::bulkInsert(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->bulkInsert(rpath);
    }


    IXdnativeSetPtr set = openTable(path);
    if (set.isNull())
        return xcm::null;

    return set->getRowInserter();
}



bool XdnativeDatabase::modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->modifyStructure(path, mod_params, job);
    }


    IXdnativeSetPtr set = openTable(path);
    if (set.isNull())
        return xcm::null;

    return set->modifyStructure(mod_params, job);
}
