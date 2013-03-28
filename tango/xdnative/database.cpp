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
#include "tango.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/filestream.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/dbfuncs.h"
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

Database::Database()
{
    m_last_job = 0;
    m_base_dir = L"";

    std::wstring kws;
    kws += xdnative_keywords;
    kws += L",";
    kws += xdnative_keywords2;

    m_attr = static_cast<tango::IAttributes*>(new DatabaseAttributes);
    m_attr->setIntAttribute(tango::dbattrColumnMaxNameLength, 80);
    m_attr->setIntAttribute(tango::dbattrTableMaxNameLength, 80);
    m_attr->setStringAttribute(tango::dbattrKeywords, kws);
    m_attr->setStringAttribute(tango::dbattrColumnInvalidChars,
                               L"*|:\"<>?[]\\;'=,/\x00\x09\x0A\x0B\x0C\x0D\xFF");
    m_attr->setStringAttribute(tango::dbattrColumnInvalidStartingChars,
                               L"*|:\"<>?[]\\;'=,/\x00\x09\x0A\x0B\x0C\x0D\xFF");
    m_attr->setStringAttribute(tango::dbattrTableInvalidChars,
                               L"*|:\"<>?[]\\;'=,/\x00\x09\x0A\x0B\x0C\x0D\xFF");
    m_attr->setStringAttribute(tango::dbattrTableInvalidStartingChars,
                               L"*|:\"<>?[]\\;'=,/\x00\x09\x0A\x0B\x0C\x0D\xFF");
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteOpenChar, L"[");
    m_attr->setStringAttribute(tango::dbattrIdentifierQuoteCloseChar, L"]");
    m_attr->setStringAttribute(tango::dbattrIdentifierCharsNeedingQuote, L"`~# $!@%^&(){}-+.");

    m_db_mgr = static_cast<tango::IDatabaseMgr*>(new DatabaseMgr);
}

Database::~Database()
{
    std::vector<JobInfo*>::iterator it;

    m_jobs_mutex.lock();
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
        (*it)->unref();
    m_jobs_mutex.unlock();

    deleteTempData();
}


void Database::setDatabaseName(const std::wstring& db_name)
{
    m_dbname = db_name;

    INodeValuePtr node = openNodeFile(L"/.system/database_name");
    if (node)
    {
        node->setString(m_dbname);
    }
}

bool Database::setBaseDirectory(const std::wstring& base_dir)
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
    m_attr->setStringAttribute(tango::dbattrTempDirectory, getTempPath());
    m_attr->setStringAttribute(tango::dbattrDefinitionDirectory, getDefinitionPath());

    return true;
}

std::wstring Database::getBaseDirectory()
{
    return m_base_dir;
}

std::wstring Database::getActiveUid()
{
    return m_uid;
}


tango::IAttributesPtr Database::getAttributes()
{
    return m_attr;
}



double Database::getFreeSpace()
{
    return (double)xf_get_free_disk_space(m_base_dir);
}

double Database::getUsedSpace()
{
    double total_size = 2000000.0;

    std::wstring data_path = makePathName(m_base_dir, L"data");

    xf_dirhandle_t h = xf_opendir(data_path);
    xf_direntry_t info;
    while (xf_readdir(h, &info))
    {
        if (info.m_type == xfFileTypeNormal)
        {
            std::wstring path = makePathName(data_path, L"", info.m_name);
            total_size += xf_get_file_size(path);
        }
    }
    xf_closedir(h);

    return total_size;
}



void Database::addFileToTrash(const std::wstring& filename)
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

void Database::emptyTrash()
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


std::wstring Database::getErrorString()
{
    return m_error.getErrorString();
}

int Database::getErrorCode()
{
    return m_error.getErrorCode();
}

void Database::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code, error_string);
}


std::wstring Database::getTempFilename()
{
    return makePathName(m_base_dir, L"temp", getUniqueString());
}

std::wstring Database::getUniqueFilename()
{
    return makePathName(m_base_dir, L"data", getUniqueString());
}

std::wstring Database::getTempOfsPath()
{
    std::wstring s = L"/.temp/";
    s += getUniqueString();
    return s;
}

std::wstring Database::getTempPath()
{
    return makePathName(m_base_dir, L"temp", L"");
}

std::wstring Database::getDefinitionPath()
{
    return makePathName(m_base_dir, L"def", L"");
}

std::wstring Database::getBasePath()
{
    return m_base_dir;
}

std::wstring Database::getOfsPath()
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


IJobInternalPtr Database::createJobEntry()
{
    XCM_AUTO_LOCK(m_jobs_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();

    m_jobs.push_back(job);

    return static_cast<IJobInternal*>(job);
}

tango::IJobPtr Database::createJob()
{
    XCM_AUTO_LOCK(m_jobs_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<tango::IJob*>(job);
}

tango::IJobPtr Database::getJob(tango::jobid_t job_id)
{
    XCM_AUTO_LOCK(m_jobs_mutex);

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



std::wstring Database::ofsToPhysFilename(const std::wstring& ofs_path,
                                         bool folder)
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

std::wstring Database::urlToOfsFilename(const std::wstring& url)
{
    // get a filename from a url
    std::wstring fname = kl::urlToFilename(url);
    
    // change all slashes to forward slashes
    size_t i, len = fname.length();
    for (i = 0; i < len; ++i)
    {
        if (fname[i] == L'\\')
            fname[i] = L'/';
    }
    
    // transform normal filename into a filename
    // in the /.fs mount directory
    
    if (fname.length() >= 2 && isalpha(fname[0]) && fname[1] == L':')
    {
        fname.insert(0, L"|");
    }
     else
    {
        if (fname.substr(0, 2) == L"//")
        {
            fname.erase(0,2);
            fname.insert(0, L"|");
        }
         else
        {
            // remove all leading slashes
            while (fname.length() > 0 && fname[0] == '/')
                fname.erase(0, 1);
        }
    }
    
    fname = L"/.fs/" + fname;
    
    return fname;
}

bool Database::createDatabase(const std::wstring& db_name,
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

    node = createNodeFile(L"/.system/database_version");
    if (!node)
        return false;
    node->setInteger(2);

    m_dbname = db_name;
    m_uid = L"admin";

    // create an ordinal file
    tango::tableord_t ord = allocOrdinal();

    // create a mount point for external files
    setMountPoint(L"/.fs", L"Xdprovider=xdfs;Database=;User ID=;Password=;", L"/");

    return true;
}


bool Database::openDatabase(const std::wstring& location,
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


    // in 2005.1 and before, these folders were not 'folder' type objects, but
    // rather generics.  These lines will convert them to folder objects, because
    // having the generic type caused problems with the save dialog
    createFolder(L"/.appdata");
    createFolder(L"/.temp");
    createFolder(L"/.system");

    // read database name
    INodeValuePtr dbname = openNodeFile(L"/.system/database_name");
    if (!dbname)
        return false;

    m_dbname = dbname->getString();

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


void Database::close()
{
    emptyTrash();
    deleteTempData();
}



inline std::wstring getUserPasswordEncryptionKey()
{
    return L"jgk5]4X4";
}


bool Database::createUser(const std::wstring& uid,
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

bool Database::deleteUser(const std::wstring& uid)
{
    // must be admin to create a user
    if (m_uid != L"admin")
        return false;
    
    std::wstring path = L"/.system/users/";
    path += uid;
    
    return deleteFile(path);
}


bool Database::checkPassword(
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

bool Database::getUserExist(const std::wstring& uid)
{
    std::wstring path = L"/.system/users/";
    path += uid;
    
    return getFileExist(path);
}

tango::tableord_t Database::getMaximumOrdinal()
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

tango::tableord_t Database::allocOrdinal()
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

bool Database::setOrdinalTable(tango::tableord_t ordinal,
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
    wchar_t tableord_path[255];
    swprintf(tableord_path, 255, L"/.system/ordinals/%u", ordinal);

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

bool Database::deleteOrdinal(tango::tableord_t ordinal)
{
    // write out ordinal entry with table filename
    wchar_t tableord_path[255];
    swprintf(tableord_path, 255, L"/.system/ordinals/%u", ordinal);
    
    if (!deleteFile(tableord_path))
        return false;

    return true;
}




void Database::updateSetReference(const std::wstring& ofs_path)
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
    tango::ISet* lookup_set = lookupSet(set_id);
    if (lookup_set)
    {
        ISetInternalPtr iset;
        iset = lookup_set;
        if (iset)
        {
            iset->onOfsPathChanged(ofs_path);
        }
        
        // release ref set by lookupSet() above
        lookup_set->unref();
    }
}


void Database::recursiveReferenceUpdate(const std::wstring& folder_path)
{
    tango::IFileInfoEnumPtr files = getFolderInfo(folder_path);
    tango::IFileInfoPtr info;

    int file_count = files->size();
    int i;

    for (i = 0; i < file_count; ++i)
    {
        info = files->getItem(i);
        int file_type = info->getType();

        std::wstring path = combineOfsPath(folder_path, info->getName());

        if (file_type == tango::filetypeSet)
        {
            updateSetReference(path);
        }
         else if (file_type == tango::filetypeFolder)
        {
            recursiveReferenceUpdate(path);
        }
    }
}

void Database::onOfsPostRenameFile(std::wstring ofs_path, std::wstring new_name)
{
    // create a fully-qualified path for the newly renamed object
    std::wstring new_path = kl::beforeLast(ofs_path, L'/');
    if (new_path == ofs_path)
        new_path = new_name;
         else
        new_path += new_name;

    // check to see if a folder has been renamed
    tango::IFileInfoPtr file_info = getFileInfo(new_path);
    if (file_info->getType() == tango::filetypeFolder)
    {
        recursiveReferenceUpdate(new_path);
        return;
    }


    // a regular object was renamed, update it if it was a set
    updateSetReference(new_path);
}


void Database::onOfsPostMoveFile(std::wstring ofs_path, std::wstring new_path)
{
    updateSetReference(new_path);
}




bool Database::deleteStream(const std::wstring& ofs_path)
{
    tango::IFileInfoPtr file_info = getFileInfo(ofs_path);
    if (!file_info)
        return false;

    if (file_info->getType() != tango::filetypeStream)
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


bool Database::deleteSet(const std::wstring& ofs_path)
{
    // make sure we are dealing with a file
    tango::IFileInfoPtr file_info = getFileInfo(ofs_path);
    if (!file_info)
        return false;

    if (file_info->getType() != tango::filetypeSet)
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
    tango::ISet* lookup_set = lookupSet(set_id);
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

        tango::tableord_t ordinal = ordinal_node->getInteger();


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
    tango::IRelationEnumPtr rel_enum;
    rel_enum = getRelationEnum(L"");

    return true;
}


bool Database::setMountPoint(const std::wstring& path,
                             const std::wstring& connection_str,
                             const std::wstring& remote_path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        bool file_exists = xf_get_file_exist(ofsToPhysFilename(path, false));

        if (!file_exists)
        {
            // move takes place in a mount
            tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
            if (db.isNull())
                return false;

            return db->setMountPoint(rpath, connection_str, remote_path);
        }
    }    


    std::wstring final_connection_string = connection_str;
    if (final_connection_string.find(L"://") != -1)
        final_connection_string = xdcommon::urlToConnectionStr(connection_str);
    final_connection_string = xdcommon::encryptConnectionStringPassword(final_connection_string);

    
    // try to create or open a file to store the mount point in
    OfsFile* file = OfsFile::createFile(this, path, tango::filetypeNode);
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

tango::IDatabasePtr Database::getMountDatabase(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        return lookupOrOpenMountDb(cstr);
    }
  
    return xcm::null;
}

                    
bool Database::getMountPoint(const std::wstring& path,
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

bool Database::createFolder(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        // action takes place in a mount
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->createFolder(rpath);
    }

    OfsFile* f = OfsFile::createFile(this, path, tango::filetypeFolder);

    if (!f)
    {
        return false;
    }

    f->unref();

    return true;
}


INodeValuePtr Database::createNodeFile(const std::wstring& path)
{
    OfsFile* file = OfsFile::createFile(this, path, tango::filetypeNode);

    if (!file)
    {
        return xcm::null;
    }

    INodeValuePtr result = file->getRootNode();
    file->unref();
    return result;
}

INodeValuePtr Database::openLocalNodeFile(const std::wstring& path)
{
    {
        XCM_AUTO_LOCK(m_objregistry_mutex);

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

INodeValuePtr Database::openNodeFile(const std::wstring& _path)
{
    if (_path.empty())
        return xcm::null;
        
    std::wstring path = _path;
    if (kl::isFileUrl(_path))
        path = urlToOfsFilename(_path);

    {
        XCM_AUTO_LOCK(m_objregistry_mutex);

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


bool Database::renameOfsFile(const std::wstring& _path,
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


bool Database::moveOfsFile(const std::wstring& ofs_path,
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






bool Database::deleteOfsFile(const std::wstring& key_path)
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

bool Database::copyData(const tango::CopyInfo* info, tango::IJob* job)
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
        tango::ISetPtr input = openSet(info->input);
        if (input.isNull())
            return false;

        iter = input->createIterator(L"", info->order, NULL);

        structure = input->getStructure();
        if (structure.isNull())
            return false;
    }


    tango::ISetPtr result_set;
    tango::ISetPtr output;
    
    if (info->append)
    {
        output = openSet(info->output);
    }
     else
    {
        deleteFile(info->output);
        output = createTable(info->output, structure, NULL);
    }

    if (output.isNull())
        return false;

    std::wstring cstr, rpath;
    if (detectMountPoint(info->output, cstr, rpath))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        xdcmnInsert(db, iter, rpath, info->where, info->max_rows, job);
    }
     else
    {
        xdcmnInsert(static_cast<tango::IDatabase*>(this), iter, info->output, info->where, info->max_rows, job);
    }

    return true;
}

bool Database::copyFile(const std::wstring& src_path,
                        const std::wstring& dest_path)
{
    if (src_path.empty() || dest_path.empty())
        return false;

    // find out if we are copying a stream
    tango::IFileInfoPtr info = getFileInfo(src_path);
    if (info.isOk() && info->getType() == tango::filetypeStream)
    {
        tango::IStreamPtr src_stream = openStream(src_path);
        tango::IStreamPtr dest_stream = createStream(dest_path, info->getMimeType());
        if (src_stream.isNull() || dest_stream.isNull())
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


bool Database::renameFile(const std::wstring& _path,
                          const std::wstring& new_name)
{
    if (_path.empty() || new_name.empty())
        return false;

    std::wstring path = _path;
    if (kl::isFileUrl(_path))
        path = urlToOfsFilename(_path);
        
    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath) && !getLocalFileExist(path))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
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



    tango::IFileInfoPtr file_info = getFileInfo(new_path);
    if (file_info.isNull())
        return false;

    if (!file_info->isMount())
    {
        int type = file_info->getType();

        if (type == tango::filetypeFolder)
        {
            // it is a folder, so all references underneath must be updated
            recursiveReferenceUpdate(new_path);
        }
         else if (type == tango::filetypeSet)
        {
            // the set reference must be updated
            updateSetReference(new_path);
        }
    }

    return true;
}

bool Database::moveFile(const std::wstring& _src_path,
                        const std::wstring& _dest_path)
{
    if (_src_path.empty() || _dest_path.empty() || _src_path == _dest_path)
        return false;
    
    std::wstring src_path = _src_path, dest_path = _dest_path;
    if (kl::isFileUrl(_src_path))
        src_path = urlToOfsFilename(_src_path);
    if (kl::isFileUrl(_dest_path))
        dest_path = urlToOfsFilename(_dest_path);

    if (src_path.empty() || dest_path.empty())
        return false;

    if (src_path[0] != '/')
        src_path = L"/" + src_path;
    
    if (dest_path[0] != '/')
        dest_path = L"/" + dest_path;
        

    // if dest_path is an existing folder, the user wants
    // to move src_path while retaining its original name
    // into the existing dest_path folder
    tango::IFileInfoPtr dest_info = getFileInfo(dest_path);
    if (dest_info.isOk() && dest_info->getType() == tango::filetypeFolder)
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
    tango::IFileInfoPtr info = getFileInfo(src_path);
    if (info.isOk())
    {
        source_item_is_mount = info->isMount();
    }


    if (!source_item_is_mount)
    {
        std::wstring src_cstr, src_rpath;
        std::wstring dest_cstr, dest_rpath;
        
        if (detectMountPoint(src_path, src_cstr, src_rpath))
            src_path = src_rpath;
            
        if (detectMountPoint(dest_path, dest_cstr, dest_rpath))
            dest_path = dest_rpath;
        
        if (src_cstr != dest_cstr)
        {
            // moves between mount points are not allowed
            return false;
        }
        
        if (src_cstr.length() > 0)
        {
            // move takes place in a mount
            tango::IDatabasePtr db = lookupOrOpenMountDb(src_cstr);
            if (db.isNull())
                return false;

            return db->moveFile(src_path, dest_path);
        }
    }
    
    
    if (!moveOfsFile(src_path, dest_path))
        return false;

    tango::IFileInfoPtr file_info = getFileInfo(dest_path);
    if (file_info.isNull())
        return false;

    if (!file_info->isMount())
    {
        int type = file_info->getType();

        if (type == tango::filetypeFolder)
        {
            // it is a folder, so all references underneath
            // must be updated
            recursiveReferenceUpdate(dest_path);
        }
         else if (type == tango::filetypeSet)
        {
            // the set reference must be updated
            updateSetReference(dest_path);
        }
    }

    return true;
}

bool Database::deleteFile(const std::wstring& _path)
{
    if (_path.empty())
        return false;
        
    std::wstring path = _path;
    if (kl::isFileUrl(_path))
        path = urlToOfsFilename(_path);

    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        // check if the mount is just a single
        // mount to another file
        INodeValuePtr f = openLocalNodeFile(path);
        if (f.isOk())
        {
            f.clear();
            return deleteOfsFile(path);
        }
        
  
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        return db->deleteFile(rpath);
    }
    
    
    
    tango::IFileInfoPtr file_info = getFileInfo(path);
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

    if (type == tango::filetypeSet)
    {
        return deleteSet(path);
    }
     else if (type == tango::filetypeStream)
    {
        return deleteStream(path);
    }
     else
    {
        return deleteOfsFile(path);
    }
}



bool Database::getLocalFileExist(const std::wstring& _path)
{
    std::wstring path = _path;
    if (kl::isFileUrl(_path))
        path = urlToOfsFilename(_path);
        
    if (path.empty())
        return false;

    return (xf_get_directory_exist(ofsToPhysFilename(path, true)) ||
            xf_get_file_exist(ofsToPhysFilename(path, false)));
}

bool Database::getFileExist(const std::wstring& _path)
{
    std::wstring path = _path;
    if (kl::isFileUrl(_path))
        path = urlToOfsFilename(_path);
        
    if (path.empty())
        return false;

    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        // root always exists
        if (rpath.empty() || rpath == L"/")
            return true;
            
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        return db->getFileExist(rpath);
    }

    return (xf_get_directory_exist(ofsToPhysFilename(path, true)) ||
            xf_get_file_exist(ofsToPhysFilename(path, false)));
}


long long Database::getFileSize(const std::wstring& ofs_path)
{
    if (ofs_path.empty())
        return 0;

    // fix up set name problems
    std::wstring fixed_name;

    if (*(ofs_path.c_str()) != L'/')
    {
        fixed_name = L"/";
    }

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
    path.reserve(80);
    path = L"/.system/objects/";
    path += set_id;

    // get filename from registry

    set_file = openNodeFile(path);
    if (!set_file)
    {
        return 0;
    }

    // load ordinal
    INodeValuePtr ordinal_node = set_file->getChild(L"ordinal", false);
    if (!ordinal_node)
        return 0;

    std::wstring table_filename = getTableFilename(ordinal_node->getInteger());

    return xf_get_file_size(table_filename);
}

std::wstring Database::getFileMimeType(const std::wstring& path)
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

class NativeFileInfo : public tango::IFileInfo
{
    XCM_CLASS_NAME("xdnative.FileInfo")
    XCM_BEGIN_INTERFACE_MAP(NativeFileInfo)
        XCM_INTERFACE_ENTRY(tango::IFileInfo)
    XCM_END_INTERFACE_MAP()

public:

    NativeFileInfo(tango::IDatabasePtr _db)
    {
        db = _db;
        fetched_size = false;
        fetched_mime_type = false;
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
    
    bool isMount()
    {
        return is_mount;
    }

    const std::wstring& getPrimaryKey()
    {
        return primary_key;
    }
    
public:

    std::wstring name;
    std::wstring mime_type;
    std::wstring path;
    std::wstring primary_key;
    long long size;
    int type;
    int format;
    bool is_mount;
    
    bool fetched_mime_type;
    bool fetched_size;
    IDatabaseInternalPtr db;
};


bool Database::setFileType(const std::wstring& path, int type)
{
    XCM_AUTO_LOCK(m_objregistry_mutex);

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


bool Database::getFileType(const std::wstring& path, int* type, bool* is_mount)
{
    if (path.empty() || path == L"/")
    {
        // caller wants file type information for root folder
        if (type)
            *type = tango::filetypeFolder;
        if (is_mount)
            *is_mount = false;
        return true;
    }

    {
        XCM_AUTO_LOCK(m_objregistry_mutex);

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


tango::IFileInfoPtr Database::getFileInfo(const std::wstring& _path)
{
    if (_path.empty())
        return xcm::null;
        
    std::wstring path = _path;
    if (kl::isFileUrl(_path))
        path = urlToOfsFilename(_path);

    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        // if it's the root, it's automatically a folder
        if (rpath.empty() || rpath == L"/")
        {
            xdcommon::FileInfo* f = new xdcommon::FileInfo;
            f->name = kl::afterLast(path, L'/');
            f->type = tango::filetypeFolder;
            f->format = tango::formatNative;
            f->is_mount = true;
            return static_cast<tango::IFileInfo*>(f);
        }
         else
        {
            std::wstring file_primary_key;
            int file_type = tango::filetypeSet;
            int file_format = tango::formatNative;
            int is_mount = -1;
            std::wstring file_mime_type;
            
            if (getLocalFileExist(path))
                is_mount = 1;
                
            tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
            if (db.isOk())
            {
                if (!checkCircularMount(path, db, rpath))
                {
                    tango::IFileInfoPtr file_info = db->getFileInfo(rpath);
                    if (file_info.isOk())
                    {
                        file_type = file_info->getType();
                        file_format = file_info->getFormat();
                        file_primary_key = file_info->getPrimaryKey();
                        file_mime_type = file_info->getMimeType();
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
            
            return static_cast<tango::IFileInfo*>(f);
        }
    }


    if (!getFileExist(path))
        return xcm::null;


    NativeFileInfo* f = new NativeFileInfo(static_cast<tango::IDatabase*>(this));
    f->type = tango::filetypeNode;
    f->is_mount = false;
    
    int slash_pos = path.find_last_of(L'/');
    if (slash_pos == -1)
    {
        f->name = path;
    }
     else
    {
        f->name = path.substr(slash_pos+1);
    }

    f->path = path;
    f->format = tango::formatNative;
    getFileType(path, &(f->type), &(f->is_mount));

    return static_cast<tango::IFileInfo*>(f);
}


tango::IDatabasePtr Database::lookupOrOpenMountDb(const std::wstring& cstr)
{
    if (cstr.empty())
    {
        return static_cast<tango::IDatabase*>(this);
    }


    tango::IDatabasePtr db;
    
    m_mountdbs_mutex.lock();
    std::map<std::wstring, tango::IDatabasePtr>::iterator it;
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
            tango::IAttributesPtr attr = db->getAttributes();
            
            if (attr)
            {
                attr->setStringAttribute(tango::dbattrTempDirectory, getTempPath());
                attr->setStringAttribute(tango::dbattrDefinitionDirectory, getDefinitionPath());
            }
        }      
        
        m_mountdbs_mutex.lock();
        m_mounted_dbs[cstr] = db;
        m_mountdbs_mutex.unlock();
    }
    
    return db;
}



bool Database::detectMountPoint(const std::wstring& path,
                                std::wstring& connection_str,
                                std::wstring& remote_path)
{
    std::vector<std::wstring> parts;
    std::vector<std::wstring>::iterator it, it2;
    bool found = true;
    
    
    // /.system folder never contains mounts
    if (0 == path.compare(0, 8, L"/.system"))
        return false;
    
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
            connection_str = xdcommon::decryptConnectionStringPassword(cs->getString());
            remote_path = rp->getString();

            for (it2 = it+1; it2 < parts.end(); ++it2)
            {
                if (remote_path.empty() || remote_path[remote_path.length()-1] != '/')
                {
                    remote_path += L'/';
                }
                
                remote_path += *it2;
                if (it2+1 < parts.end())
                    remote_path += L'/';
            }
            
            return true;
        }
    }
    
    return false;
}

/*
bool Database::checkCircularMount(const std::wstring& path,
                                  tango::IDatabasePtr remote_db, 
                                  const std::wstring remote_path)
{
    if (static_cast<tango::IDatabase*>(this) != remote_db.p)
        return false;
        
    return (0 == wcscasecmp(remote_path.c_str(), path.c_str())) ? true : false;
}
*/



bool Database::checkCircularMountInternal(std::set<std::wstring, kl::cmp_nocase>& bad_paths,
                                          tango::IDatabasePtr remote_db, 
                                          const std::wstring remote_path)
{
    // if remote is a different db, it's not circular
    if (static_cast<tango::IDatabase*>(this) != remote_db.p)
        return false;
        
    if (bad_paths.find(remote_path) != bad_paths.end())
        return true;
       
    std::wstring cstr, rpath;
    if (!detectMountPoint(remote_path, cstr, rpath))
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
    
    tango::IDatabasePtr rdb = lookupOrOpenMountDb(cstr);
    bad_paths.insert(remote_path);
    
    return checkCircularMountInternal(bad_paths, rdb, rpath);
}




bool Database::checkCircularMount(const std::wstring& path,
                                  tango::IDatabasePtr remote_db, 
                                  const std::wstring remote_path)
{
    std::set<std::wstring, kl::cmp_nocase> bad_paths;
    bad_paths.insert(path);
    
    return checkCircularMountInternal(bad_paths, remote_db, remote_path);
}



tango::IFileInfoEnumPtr Database::getFolderInfo(const std::wstring& _mask)
{ 
    xcm::IVectorImpl<tango::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;
    
    // if the parameter is empty, assume they want the root folder
    std::wstring mask = _mask;
    if (mask.length() == 0)
        mask = L"/";


    // detect if the specified folder is a mount point
    
    std::wstring cstr, rpath;
    
    if (detectMountPoint(mask, cstr, rpath))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
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

            tango::IFileInfoPtr file_info = getFileInfo(ofs_path);
            if (file_info.isOk())
            {
                retval->append(file_info);
            }
        }

    }

    xf_closedir(h);

    return retval;
}



void Database::registerNodeFile(OfsFile* file)
{
    XCM_AUTO_LOCK(m_objregistry_mutex);

    m_ofs_files.push_back(file);
}

void Database::unregisterNodeFile(OfsFile* file)
{
    XCM_AUTO_LOCK(m_objregistry_mutex);

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

void Database::registerSet(tango::ISet* set)
{
    XCM_AUTO_LOCK(m_objregistry_mutex);

    m_sets.push_back(set);
}

void Database::unregisterSet(tango::ISet* set)
{
    XCM_AUTO_LOCK(m_objregistry_mutex);

    std::vector<tango::ISet*>::iterator it;
    for (it = m_sets.begin(); it != m_sets.end(); ++it)
    {
        if (*it == set)
        {
            m_sets.erase(it);
            return;
        }   
    }
}

void Database::registerTable(ITable* table)
{
    XCM_AUTO_LOCK(m_objregistry_mutex);

    m_tables.push_back(table);
}

void Database::unregisterTable(ITable* table)
{
    XCM_AUTO_LOCK(m_objregistry_mutex);

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


bool Database::deleteTempData()
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






void Database::getFolderUsedOrdinals(const std::wstring& folder_path,
                                      std::set<int>& used_ordinals)
{
    // skip mounts
    tango::IFileInfoPtr fileinfo = getFileInfo(folder_path);
    if (fileinfo.isNull() || fileinfo->isMount())
        return;
    

    tango::IFileInfoEnumPtr files = getFolderInfo(folder_path);
    tango::IFileInfoPtr info;

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


        if (file_type == tango::filetypeSet)
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



bool Database::cleanup()
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


tango::IStreamPtr Database::openStream(const std::wstring& _path)
{
    std::wstring path = _path;
    if (kl::isFileUrl(_path))
        path = urlToOfsFilename(_path);

    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->openStream(rpath);
    }


    // see if the file is a node file; if it is, open the
    // stream from the node
    tango::IFileInfoPtr info = getFileInfo(_path);
    if (info.isOk() && info->getType() == tango::filetypeNode)
    {
        NodeFileStream* stream = new NodeFileStream(this);
        if (!stream->open(_path))
        {
            delete stream;
            return xcm::null;
        }

        return static_cast<tango::IStream*>(stream);
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

    return static_cast<tango::IStream*>(stream);
}

tango::IStreamPtr Database::createStream(const std::wstring& _path, const std::wstring& mime_type)
{
    std::wstring path = _path;
    if (kl::isFileUrl(_path))
        path = urlToOfsFilename(_path);

    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

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

            return static_cast<tango::IStream*>(file_stream);
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
    OfsFile* file = OfsFile::createFile(this, path, tango::filetypeStream);
    if (!file)
    {
        return xcm::null;
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
        return xcm::null;
    }

    return static_cast<tango::IStream*>(file_stream);
}



tango::ISet* Database::lookupSet(const std::wstring& set_id)
{
    XCM_AUTO_LOCK(m_objregistry_mutex);

    // search existing sets for a match
    std::vector<tango::ISet*>::iterator sit;
    for (sit = m_sets.begin(); sit != m_sets.end(); ++sit)
    {
        if (!wcscasecmp((*sit)->getSetId().c_str(), set_id.c_str()))
        {
            (*sit)->ref();
            return *sit;
        }
    }

    return NULL;
}


tango::IStructurePtr Database::createStructure()
{
    Structure* s = new Structure;
    return static_cast<tango::IStructure*>(s);
}

tango::ISetPtr Database::createTable(const std::wstring& _path,
                                     tango::IStructurePtr structure,
                                     tango::FormatInfo* format_info)
{
    std::wstring path = _path;
    
    if (path.length() == 0)
        return xcm::null;

    if (kl::isFileUrl(_path))
        path = urlToOfsFilename(_path);

    if (getFileExist(path))
        return xcm::null;  // already exists

    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        tango::ISetPtr set = db->createTable(rpath, structure, format_info);
        if (set.isOk())
        {
            set->setObjectPath(_path);
        }
            
        return set;
    }


    TableSet* set = new TableSet(this);
    set->ref();

    if (!set->create(structure, path))
        return xcm::null;

    return tango::ISetPtr(set, false);
}


std::wstring Database::getDatabaseName()
{
    return m_dbname;
}

int Database::getDatabaseType()
{
    return tango::dbtypeXdnative;
}

std::wstring Database::getStreamFilename(const std::wstring& ofs_path)
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


std::wstring Database::getTableFilename(tango::tableord_t table_ordinal)
{
    wchar_t ord_key_name[255];
    swprintf(ord_key_name, 255, L"/.system/ordinals/%d", table_ordinal);

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


ITablePtr Database::openTableByOrdinal(tango::tableord_t table_ordinal)
{
    {
        XCM_AUTO_LOCK(m_objregistry_mutex);

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



tango::ISetPtr Database::openSetById(const std::wstring& set_id)
{
    // check if the set is locked
    if (getSetLocked(set_id))
    {
        return xcm::null;
    }

    // check if the set is already open
    tango::ISet* lookup_set = lookupSet(set_id);
    if (lookup_set)
    {
        // result is already ref'ed by lookupSet();
        tango::ISetPtr ret(lookup_set, false);
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

        return tango::ISetPtr(static_cast<tango::ISet*>(set), false);
    }

    return xcm::null;
}



std::wstring Database::getSetIdFromPath(const std::wstring& set_path)
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


std::wstring Database::getSetPathFromId(const std::wstring& set_id)
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




tango::ISetPtr Database::openSet(const std::wstring& set_path)
{
    if (kl::isFileUrl(set_path))
    {
        return openSetEx(set_path, tango::formatNative);
    }

    std::wstring cstr, rpath;
    if (detectMountPoint(set_path, cstr, rpath))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);

        if (db.isNull())
            return xcm::null;

        if (checkCircularMount(set_path, db, rpath))
            return xcm::null;

        tango::ISetPtr set = db->openSet(rpath);
        if (set.isNull())
            return xcm::null;

        set->setObjectPath(set_path);
        return set;
    }


    // check for ptr sets
    if (set_path.substr(0, 12) == L"/.temp/.ptr/")
    {
        std::wstring ptr_string = kl::afterLast(set_path, L'/');
        unsigned long l = (unsigned long)hex2uint64(ptr_string.c_str());
        tango::ISet* sptr = (tango::ISet*)l;
        return sptr;
    }



    std::wstring set_id = getSetIdFromPath(set_path);
    if (set_id.empty())
        return xcm::null;

    return openSetById(set_id);
}

tango::ISetPtr Database::openSetEx(const std::wstring& _path,
                                   int format)
{
    std::wstring path;

    if (kl::isFileUrl(_path))
        path = urlToOfsFilename(_path);
         else
        path = _path;

    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        tango::ISetPtr set = db->openSetEx(rpath, format);
        if (set)
        {
            set->setObjectPath(path);
        }

        return set;
    }

    return openSet(path);
}


tango::IIteratorPtr Database::createIterator(const std::wstring& path,
                                             const std::wstring& columns,
                                             const std::wstring& sort,
                                             tango::IJob* job)
{
    tango::ISetPtr set = openSet(path);
    if (set.isNull())
        return xcm::null;
    return set->createIterator(columns, sort, job);
}


bool Database::lockSet(const std::wstring& set_id)
{
    XCM_AUTO_LOCK(m_lockedsets_mutex);

    std::vector<std::wstring>::iterator it;
    it = std::find(m_locked_sets.begin(), m_locked_sets.end(), set_id);
    if (it != m_locked_sets.end())
        return false;

    m_locked_sets.push_back(set_id);
    return true;
}

bool Database::unlockSet(const std::wstring& set_id)
{
    XCM_AUTO_LOCK(m_lockedsets_mutex);

    std::vector<std::wstring>::iterator it;
    it = std::find(m_locked_sets.begin(), m_locked_sets.end(), set_id);
    if (it == m_locked_sets.end())
        return false;

    m_locked_sets.erase(it);
    return true;
}


bool Database::getSetLocked(const std::wstring& set_id)
{
    XCM_AUTO_LOCK(m_lockedsets_mutex);

    std::vector<std::wstring>::iterator it;
    it = std::find(m_locked_sets.begin(), m_locked_sets.end(), set_id);
    if (it == m_locked_sets.end())
        return false;

    return true;
}


class RelationInfo : public tango::IRelation,
                     public IRelationInternal
{
    XCM_CLASS_NAME("xdnative.RelationInfo")
    XCM_BEGIN_INTERFACE_MAP(RelationInfo)
        XCM_INTERFACE_ENTRY(tango::IRelation)
        XCM_INTERFACE_ENTRY(IRelationInternal)
    XCM_END_INTERFACE_MAP()

public:

    RelationInfo(IDatabaseInternal* db)
    {
        // RelationInfo does not need to call ref() on the database object
        // since references themselves are held by the db
        m_dbi = db;
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
    
    std::wstring getLeftSet()
    {
        if (m_left_path.length() > 0)
            return m_left_path;
             else
            return m_dbi->getSetPathFromId(m_left_setid);
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

    std::wstring getRightSet()
    {
        if (m_right_path.length() > 0)
            return m_right_path;
             else
            return m_dbi->getSetPathFromId(m_right_setid);
    }

    tango::ISetPtr getRightSetPtr()
    {
        if (m_right_path.length() > 0)
        {
            tango::IDatabasePtr db = static_cast<xcm::IObject*>(m_dbi);
            return db->openSet(m_right_path);
        }
         else
        {
            return m_dbi->openSetById(m_right_setid);
        }
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

    IDatabaseInternal* m_dbi;
    std::wstring m_relation_id;
    std::wstring m_tag;

    std::wstring m_left_path;
    std::wstring m_left_setid;
    std::wstring m_left_expression;

    std::wstring m_right_path;
    std::wstring m_right_setid;
    std::wstring m_right_expression;
};


tango::IRelationEnumPtr Database::getRelationEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IRelationPtr>* vec;
    vec = new xcm::IVectorImpl<tango::IRelationPtr>;

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

        RelationInfo* relation;
        relation = new RelationInfo(static_cast<IDatabaseInternal*>(this));
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

        tango::IRelationPtr r = static_cast<tango::IRelation*>(relation);
        vec->append(r);
    }


    // now delete entries which are invalid
    std::vector<std::wstring>::iterator it;
    for (it = to_delete.begin(); it != to_delete.end(); ++it)
        file->deleteChild(*it);

    return vec;
}

tango::IRelationPtr Database::getRelation(const std::wstring& relation_id)
{
    tango::IRelationEnumPtr rel_enum = getRelationEnum(L"");
    size_t i, rel_count = rel_enum->size();
    for (i = 0; i < rel_count; ++i)
    {
        if (rel_enum->getItem(i)->getRelationId() == relation_id)
            return rel_enum->getItem(i);
    }

    return xcm::null;
}

tango::IRelationPtr Database::createRelation(const std::wstring& tag,
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
    RelationInfo* relation = new RelationInfo(static_cast<IDatabaseInternal*>(this));
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

    tango::ISetPtr left_set = openSet(left_set_path);
    if (left_set.isOk())
    {
        ISetInternalPtr set_int = left_set;
        if (set_int)
            set_int->onRelationshipsUpdated();
    }

    return static_cast<tango::IRelation*>(relation);
}


bool Database::deleteRelation(const std::wstring& relation_id)
{
    tango::IRelationPtr rel = getRelation(relation_id);
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

    
    std::wstring left_set_path = rel->getLeftSet();
    tango::ISetPtr left_set = openSet(left_set_path);
    if (left_set)
    {
        ISetInternalPtr set_int = left_set;
        if (set_int)
            set_int->onRelationshipsUpdated();
    }


    return true;
}




tango::IIndexInfoPtr Database::createIndex(const std::wstring& path,
                                           const std::wstring& name,
                                           const std::wstring& expr,
                                           tango::IJob* job)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        // action takes place in a mount
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

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

    tango::ISetPtr set = openSet(path);
    ISetInternalPtr set_int = set;
    if (set_int.isNull())
        return xcm::null;

    return set_int->createIndex(name, expr, job);
}


bool Database::renameIndex(const std::wstring& path,
                           const std::wstring& name,
                           const std::wstring& new_name)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        // action takes place in a mount
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->renameIndex(rpath, name, new_name);
    }

    tango::ISetPtr set = openSet(path);
    ISetInternalPtr set_int = set;
    if (set_int.isNull())
        return false;

    return set_int->renameIndex(name, new_name);
}


bool Database::deleteIndex(const std::wstring& path,
                           const std::wstring& name)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        // action takes place in a mount
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->deleteIndex(rpath, name);
    }

    tango::ISetPtr set = openSet(path);
    ISetInternalPtr set_int = set;
    if (set_int.isNull())
    {
        return xcm::null;
    }

    return set_int->deleteIndex(name);
}


tango::IIndexInfoEnumPtr Database::getIndexEnum(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        // action takes place in a mount
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->getIndexEnum(rpath);
    }

    tango::ISetPtr set = openSet(path);
    ISetInternalPtr set_int = set;
    if (set_int.isNull())
    {
        // ISetInternal not supported -- return no indexes
        xcm::IVectorImpl<tango::IIndexInfoPtr>* vec;
        vec = new xcm::IVectorImpl<tango::IIndexInfoPtr>;
        return vec;
    }
    
    return set_int->getIndexEnum();
}



tango::IStructurePtr Database::describeTable(const std::wstring& path)
{
    tango::ISetPtr set = openSet(path);
    if (set.isNull())
        return xcm::null;

    return set->getStructure();
}

tango::IRowInserterPtr Database::bulkInsert(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        // action takes place in a mount
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->bulkInsert(rpath);
    }


    ISetInternalPtr set = openSet(path);
    if (set.isNull())
        return xcm::null;

    return set->getRowInserter();
}



bool Database::modifyStructure(const std::wstring& path, tango::IStructurePtr struct_config, tango::IJob* job)
{
    ISetInternalPtr set = openSet(path);
    if (set.isNull())
        return xcm::null;

    return set->modifyStructure(struct_config, job);
}
