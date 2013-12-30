/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <ctime>
#include <xd/xd.h>
#include "xdfs.h"
#include "database.h"
#include "xbaseset.h"
#include "ttbset.h"
#include "delimitedtextset.h"
#include "fixedlengthtextset.h"
#include "xbase.h"
#include "delimitedtext.h"
#include "rawtext.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "../xdcommon/extfileinfo.h"
#include "../xdcommon/filestream.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/formatdefinition.h"
#include <kl/url.h>
#include <kl/hex.h>
#include <kl/json.h>
#include <kl/md5.h>


#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#endif


const wchar_t* xdfs_keywords =
                L"ADD,ALL,ALTER,AND,ANY,AS,ASC,BEGIN,BETWEEN,BOOL,BOOLEAN,"
                L"BOTH,BREAK,BY,CASE,CHAR,CHARACTER,CHECK,CLOSE,COLLATE,"
                L"COLUMN,COMMIT,CONNECT,CONTINUE,CREATE,CURRENT,CURSOR,"
                L"DATE,DATETIME,DECIMAL,DECLARE,DEFAULT,DELETE,DESC,"
                L"DESCRIBE,DISTINCT,DO,DOUBLE,DROP,ELSE,ELSEIF,END,EXISTS,"
                L"FALSE,FETCH,FLOAT,FOR,FOREIGN,FROM,FULL,FUNCTION,GOTO,"
                L"GRANT,GROUP,HAVING,IF,IN,INDEX,INNER,INSERT,INT,INTEGER,"
                L"INTERSECT,INTO,IS,JOIN,KEY,LEFT,LEVEL,LIKE,LONG,MATCH,NEW,"
                L"NOT,NULL,NUMERIC,OF,ON,OPEN,OPTION,OR,ORDER,OUTER,PRIMARY,"
                L"PRIVATE,PRECISION,PRIVILEGES,PROCEDURE,PROTECTED,PUBLIC,"
                L"READ,RESTRICT,RETURN,REVOKE,RIGHT,ROWS,SELECT,SESSION,SET,"
                L"SIZE,SHORT,SIGNED,SMALLINT,SOME,SWITCH,TABLE,THEN,THIS,TO,"
                L"TRUE,UNION,UNIQUE,UNSIGNED,UPDATE,USER,USING,VALUES,VARCHAR,"
                L"VARYING,VIEW,VOID,WHEN,WHENEVER,WHERE,WHILE,WITH";

const wchar_t* xdfs_keywords2 = L"";

const wchar_t* xdfs_invalid_column_chars =
                             L"~!@#$%^&*()+{}|:\"<>?`-=[]\\;',./";






// FsDatabase class implementation

FsDatabase::FsDatabase()
{
    m_last_job = 0;
    
    std::wstring kws;
    kws += xdfs_keywords;
    kws += L",";
    kws += xdfs_keywords2;

    // note: official filename restrictions on windows systems: \/:*?<>|
    // additional restrictions for quoting and items that would cause
    // parsing-related issues (e.g. [],"'=)
    
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

    m_attr->setStringAttribute(xd::dbattrTempDirectory, xf_get_temp_path());
    m_attr->setStringAttribute(xd::dbattrDefinitionDirectory, xf_get_temp_path());

    
    m_db_mgr = xd::getDatabaseMgr();
}

FsDatabase::~FsDatabase()
{
    std::vector<JobInfo*>::iterator it;
    for (it = m_jobs.begin(); it != m_jobs.end(); ++it)
    {
        (*it)->unref();
    }
}


bool FsDatabase::open(const std::wstring& path)
{
    if (path.length() > 0)
    {
        if (!xf_get_directory_exist(path))
            return false;
    }

    m_base_path = path;
    return true;
}

std::wstring FsDatabase::getTempFileDirectory()
{
    std::wstring result = m_attr->getStringAttribute(xd::dbattrTempDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}

std::wstring FsDatabase::getDefinitionDirectory()
{
    std::wstring result = m_attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}


static int find_max(int a, int b, int c = 0, int d = 0, int e = 0, int f = 0,
                                  int g = 0, int h = 0, int i = 0, int j = 0)
{
    return std::max(a, std::max(b,
                       std::max(c,
                       std::max(d,
                       std::max(e,
                       std::max(f,
                       std::max(g,
                       std::max(h,
                       std::max(i, j)))))))));
}

static bool determineSetFormatInfo(const std::wstring& path,
                                   FsSetFormatInfo* info)
{
    BufferedFile f;
        
    if (!f.openFile(path, xfRead, xfShareReadWrite))
    {
        info->format = xd::formatDefault;
        return false;
    }
    
    std::vector<std::wstring> lines;
    
    
    size_t i;
    wchar_t ch;
    bool eof = false;
    std::wstring line;
    line.reserve(4096);

    for (i = 0; i < 4096; ++i)
    {
        ch = f.getChar(i, &eof);
        if (eof)
            break;
        
        if (ch == 0x0d)
            continue;
        
        if (ch == 0x0a)
        {
            lines.push_back(line);
            line = L"";
            continue;
        }
        
        line += ch;
    }
    
    
    // if we have one last string in the buffer, add it to the vector of lines
    if (line.length() > 0)
        lines.push_back(line);
    
    f.closeFile();
    
    
    // there's nothing in the file, bail out
    if (lines.size() == 0)
    {
        info->format = xd::formatFixedLengthText;
        return true;
    }
    
    size_t comma_count = 0,
           tab_count = 0,
           semicolon_count = 0,
           colon_count = 0,
           pipe_count = 0,
           tilde_count = 0;
    size_t comma_line_count = 0,
           tab_line_count = 0,
           semicolon_line_count = 0,
           colon_line_count = 0,
           pipe_line_count = 0,
           tilde_line_count = 0;
    
    std::vector<std::wstring>::iterator it;
    const wchar_t* p;
    for (it = lines.begin(); it != lines.end(); ++it)
    {
        p = (wchar_t*)it->c_str();
        while (*p)
        {
            if (*p == ',')
                comma_count++;
            if (*p == '\t')
                tab_count++;
            if (*p == ';')
                semicolon_count++;
            if (*p == ':')
                colon_count++;
            if (*p == '|')
                pipe_count++;
            if (*p == '~')
                tilde_count++;
            
            p++;
        }
        
        // if there's a comma or tab every twenty character, on average,
        // count this line as a comma or tab delimited line (we add one
        // since normally there is no delimiter at the end of the line)
        size_t line_length = it->length();
        if (comma_count > 0 && line_length/(comma_count+1) <= 25)
            comma_line_count++;
        if (tab_count > 0 && line_length/(tab_count+1) <= 25)
            tab_line_count++;
        if (semicolon_count > 0 && line_length/(semicolon_count+1) <= 25)
            semicolon_line_count++;
        if (colon_count > 0 && line_length/(colon_count+1) <= 25)
            colon_line_count++;
        if (pipe_count > 0 && line_length/(pipe_count+1) <= 25)
            pipe_line_count++;
        if (tilde_count > 0 && line_length/(tilde_count+1) <= 25)
            tilde_line_count++;
        
        comma_count = 0;
        tab_count = 0;
        semicolon_count = 0;
        colon_count = 0;
        pipe_count = 0;
        tilde_count = 0;
    }
    
    int max = find_max(comma_line_count,
                       tab_line_count,
                       semicolon_line_count,
                       colon_line_count,
                       pipe_line_count,
                       tilde_line_count);
    
    // at least half of the lines were "sensed" as delimited
    if (max > 0 && lines.size() > 1 && max >= (int)(lines.size()/2))
    {
        if (max == comma_line_count)
            info->delimiters = L",";
         else if (max == tab_line_count)
            info->delimiters = L"\t";
         else if (max == semicolon_line_count)
            info->delimiters = L";";
         else if (max == colon_line_count)
            info->delimiters = L":";
         else if (max == pipe_line_count)
            info->delimiters = L"|";
         else if (max == tilde_line_count)
            info->delimiters = L"~";
         
        info->format = xd::formatDelimitedText;
        return true;
    }
    
    // default format is fixed-length
    info->format = xd::formatFixedLengthText;
    return true;
}


static bool isTextFileExtension(const std::wstring& _ext)
{
    std::wstring ext = _ext;
    kl::makeUpper(ext);
    std::string aext = kl::tostring(ext);
    
    static const char* text_types[] = { "ASP", "C", "CC", "CPP", "CS", "CXX",
                                        "H", "HPP", "JAVA", "JS", "JSP", "PL", "PHP", "RC",
                                        (const char*)0 };
    
    for (size_t i = 0; text_types[i] != NULL; ++i)
        if (aext == text_types[i])
            return true;
    
    return false;
}


bool FsDatabase::getFileFormat(const std::wstring& phys_path,
                               FsSetFormatInfo* info,
                               int info_mask)
{
    if (!xf_get_file_exist(phys_path))
    {
        info->format = xd::formatDefault;
        return false;
    }

    // figure out the config file name
    xd::IAttributesPtr attr = getAttributes();
    std::wstring definition_path = 
        attr->getStringAttribute(xd::dbattrDefinitionDirectory);
    std::wstring configfile_path =
        ExtFileInfo::getConfigFilenameFromPath(definition_path, phys_path);

    ExtFileInfo fileinfo;
    if (fileinfo.load(configfile_path))
    {
        std::wstring format_str, delims;
        format_str = fileinfo.getGroup(L"file_info").getChildContents(L"type");
        delims = fileinfo.getGroup(L"file_info").getChild(L"settings").getChildContents(L"field_delimiters");
        
        // see if a text definition file defined the format
        if (format_str == L"text/delimited" ||
            format_str == L"text/csv" ||
            format_str == L"text_delimited")
        {
            info->format = xd::formatDelimitedText;
            
            if (delims.length() > 0)
                info->delimiters = delims;
                 else
                info->delimiters = L",";
            
            return true;
        }
         else if (format_str == L"text/fixed" ||
                  format_str == L"fixed_length")
        {
            info->format = xd::formatFixedLengthText;
            return true;
        }
         else if (format_str == L"text/plain")
        {
            info->format = xd::formatText;
            return true;
        }
    }

    // chop off and format the file extenstion
    std::wstring ext = kl::afterLast(phys_path, L'.');
    if (ext.length() == phys_path.length())
        ext = L"";

    // if a format is not specified in the ExtFileInfo,
    // use the file extension to determine the format
    if (0 == wcscasecmp(ext.c_str(), L"ttb"))
    {
        info->format = xd::formatTTB;
        return true;
    }
     else if (0 == wcscasecmp(ext.c_str(), L"dbf"))
    {
        info->format = xd::formatXbase;
        return true;
    }
     else if (0 == wcscasecmp(ext.c_str(), L"icsv"))
    {
        info->format = xd::formatTypedDelimitedText;
        return true;
    }
     else if (0 == wcscasecmp(ext.c_str(), L"csv"))
    {
        info->format = xd::formatDelimitedText;
        info->delimiters = L",";
        
        
        if (info_mask == FsSetFormatInfo::maskFormat)
        {
            // caller only wanted to know the format
            // so we can return immediately
            return true;
        }
        

        // the delimiter is almost certainly a comma, but
        // sometimes is something else, such as a semicolon
        bool res = determineSetFormatInfo(phys_path, info);
        
        // because the file extension is csv, don't let determineSetFormatInfop
        // guess anything different (happens sometimes with one-column csv's,
        // because there are no delimiters)
        
        info->format = xd::formatDelimitedText;
        return res;
    }
     else if (0 == wcscasecmp(ext.c_str(), L"tsv"))
    {
        info->format = xd::formatDelimitedText;
        info->delimiters = L"\t";
        return true;
    }
     else if (isTextFileExtension(ext))
    {
        info->format = xd::formatText;
        return true;
    }
    
    // read some of the file to see if we can determine the format
    return determineSetFormatInfo(phys_path, info);
}

void FsDatabase::close()
{

}

int FsDatabase::getDatabaseType()
{
    return xd::dbtypeFilesystem;
}

std::wstring FsDatabase::getActiveUid()
{
    return L"";
}

xd::IAttributesPtr FsDatabase::getAttributes()
{
    return m_attr;
}

std::wstring FsDatabase::getErrorString()
{
    return m_error.getErrorString();
}

int FsDatabase::getErrorCode()
{
    return m_error.getErrorCode();
}

void FsDatabase::setError(int error_code, const std::wstring& error_string)
{
    m_error.setError(error_code, error_string);
}


bool FsDatabase::cleanup()
{
    return true;
}

xd::IJobPtr FsDatabase::createJob()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<xd::IJob*>(job);
}



static std::wstring convertSlashes(const std::wstring& path)
{
    std::wstring retval = path;
    wchar_t* p = (wchar_t*)retval.c_str();

    while (*p)
    {
        if (*p == L'/' ||
            *p == L'\\')
        {
            *p = PATH_SEPARATOR_CHAR;
        }

        ++p;
    }

    return retval;
}


std::wstring FsDatabase::makeFullPath(const std::wstring& _path)
{
    // first, if the specified path is a url,
    // turn it into a real path
    
    std::wstring file_path = _path;
    if (kl::isFileUrl(_path))
    {
        file_path = kl::urlToFilename(_path);
    }
    
    
    
    std::wstring path = convertSlashes(file_path);
    
    if (path.length() >= 2)
    {
        // return final path if it is a DOS-style absolute path
        if (path[1] == L':')
            return path;
    }
    
    
    std::wstring base_path = m_base_path;
    
#ifdef WIN32
    if (base_path.empty())
    {
        // if the base path is empty, and we are running on windows,
        // then xdfs has a mode which puts drive letters and share
        // names in a virtual root, like
        //
        // /|c:/testdata/ap_hist.dbf = c:\testdata\ap_hist.dbf
        // /|cardiff/sys1/dump/ap_hist.csv = \\cardiff\sys1\dump\ap_hist.csv
        
        std::vector<std::wstring> path_parts;
        kl::parseDelimitedList(path, path_parts, PATH_SEPARATOR_CHAR);
        
        // if the path started with a /, the first element
        // will be empty; remove it
        while (path_parts.size() > 0 && path_parts[0].empty())
            path_parts.erase(path_parts.begin());
            
        if (path_parts.empty())
            return L"";
            
        if (path_parts[0].length() >= 1 && path_parts[0].length() > 0 && path_parts[0][0] == L'|')
        {
            path_parts[0].erase(0,1);
            
            std::wstring result;
            
            if (path_parts[0].find(L":") != -1)
            {
                result += path_parts[0];
            }
             else
            {
                result = L"\\\\";
                
                result += path_parts[0];
            }
            
            // get rid of the drive letter part that we've already processed
            path_parts.erase(path_parts.begin());
            
            
            std::vector<std::wstring>::iterator it;
            for (it = path_parts.begin(); it != path_parts.end(); ++it)
            {
                if (it->empty())
                    continue;
                    
                result += PATH_SEPARATOR_CHAR;
                result += *it;
            }
            
            return result;
        }
    }
#endif

   

    // start with the base path of the database,
    // because our path is relative to the base path
    std::wstring result = base_path;
    
    // add a trailing slash/backslash
    if (result.length() > 0)
    {
        if (result[result.length()-1] != PATH_SEPARATOR_CHAR)
            result += PATH_SEPARATOR_CHAR;
    }
     else
    {
        result += PATH_SEPARATOR_CHAR;
    }

    // append the rest of the path, without doubling any slashes/backslahes
    if (path.length() > 0 && path[0] == PATH_SEPARATOR_CHAR)
    {
        result += path.substr(1);
    }
     else
    {
        result += path;
    }
    
    return result;
}


bool FsDatabase::detectMountPoint(const std::wstring& path,
                                  std::wstring* connection_str,
                                  std::wstring* remote_path,
                                  std::wstring* mount_root)
{
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
    
    std::wstring phys_path;
    
    for (it = parts.begin(); it != parts.end(); ++it)
    {
        if (fpath.empty() || fpath[fpath.length()-1] != L'/')
            fpath += L"/";
        fpath += *it;


        phys_path = makeFullPath(fpath);
        phys_path += L".xdfs0";
        
        if (xf_get_file_exist(phys_path))
        {
            xd::FormatDefinition def;
            if (!loadDefinitionFromFile(phys_path, &def))
                return false;

            if (def.data_connection_string.length() == 0)
                return false;

            if (connection_str) *connection_str = def.data_connection_string;
            if (remote_path)    *remote_path = def.data_path;
            if (mount_root)     *mount_root = fpath;

            for (it2 = it+1; it2 < parts.end(); ++it2)
            {
                if (remote_path)
                {
                    if (remote_path->empty() || (*remote_path)[remote_path->length()-1] != '/')
                    {
                        *remote_path += L'/';
                    }
                
                    (*remote_path) += *it2;
                    if (it2+1 < parts.end())
                        (*remote_path) += L'/';
                }
            }

            return true;
        }
    }
    
    return false;
}



std::wstring FsDatabase::getObjectIdFromPath(const std::wstring& path)
{
    std::wstring object_id;
    
    object_id = L"xdfs:";
    object_id += xf_get_network_path(path);
    
#ifdef WIN32
    // win32's filenames are case-insensitive, so
    // when generating the set id, make the whole filename
    // lowercase to avoid multiple id's for the same file
    kl::makeLower(object_id);
#endif
    
    return kl::md5str(object_id);
}




bool FsDatabase::checkCircularMount(const std::wstring& path,
                                    xd::IDatabasePtr remote_db, 
                                    const std::wstring remote_path)
{
    // TODO: implement
    return false;
}

                            
xd::IDatabasePtr FsDatabase::lookupOrOpenMountDb(const std::wstring& cstr)
{
    if (cstr.empty())
    {
        return static_cast<xd::IDatabase*>(this);
    }


    xd::IDatabasePtr db;
    
    std::map<std::wstring, xd::IDatabasePtr>::iterator it;
    it = m_mounted_dbs.find(cstr);
    if (it != m_mounted_dbs.end())
    {
        db = it->second;
    }

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
                attr->setStringAttribute(xd::dbattrTempDirectory, m_attr->getStringAttribute(xd::dbattrTempDirectory));
                attr->setStringAttribute(xd::dbattrDefinitionDirectory, m_attr->getStringAttribute(xd::dbattrDefinitionDirectory));
            }
        }      
        
        m_mounted_dbs[cstr] = db;
    }
    
    return db;
}



xd::IDatabasePtr FsDatabase::getMountDatabase(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        return lookupOrOpenMountDb(cstr);
    }
  
    return xcm::null;
}
     
bool FsDatabase::setMountPoint(const std::wstring& path,
                               const std::wstring& connection_str,
                               const std::wstring& remote_path)
{
    // convert path to real file name
    std::wstring phys_path = makeFullPath(path);
    phys_path += L".xdfs0";
    
    // process connection string
    std::wstring final_connection_string = connection_str;
    if (final_connection_string.find(L"://") != -1)
        final_connection_string = xdcommon::urlToConnectionStr(connection_str);
    final_connection_string = xdcommon::encryptConnectionStringPassword(final_connection_string);


    xd::FormatDefinition fd;
    fd.data_connection_string = connection_str;
    fd.data_path = remote_path;
    fd.object_type = xd::filetypeFolder;
    fd.object_id = kl::getUniqueString();

    // prefer just to store an empty path designating root folder instead of a slash
    if (fd.data_path == L"/")
        fd.data_path = L"";


    return saveDefinitionToFile(phys_path, &fd);


    /*
    kl::xmlnode root;
    
    root.setNodeName(L"xdmnt");

    kl::xmlnode& cstr = root.addChild(L"connection_str");
    cstr.setNodeValue(final_connection_string);
    
    kl::xmlnode& rpath = root.addChild(L"remote_path");
    rpath.setNodeValue(remote_path);

    
    return root.save(path);
    */
}
                              
bool FsDatabase::getMountPoint(const std::wstring& path,
                               std::wstring& connection_str,
                               std::wstring& remote_path)
{
    // convert path to real file name
    std::wstring phys_path = makeFullPath(path);
    phys_path += L".xdfs0";

    xd::FormatDefinition fd;

    if (!loadDefinitionFromFile(phys_path, &fd))
        return false;

    connection_str = fd.data_connection_string;
    remote_path = fd.data_path;

    return true;

    /*
    // convert path to real file name
    std::wstring path = makeFullPath(_path);
    path += L".xdmnt";

    kl::xmlnode root;
    if (!root.load(path))
        return false;
    
    if (root.getNodeName() != L"xdmnt")
        return false;
    
    kl::xmlnode& cs = root.getChild(L"connection_str");
    if (cs.isEmpty())
        return false;
        
    kl::xmlnode& rp = root.getChild(L"remote_path");
    if (rp.isEmpty())
        return false;
    
    connection_str = xdcommon::decryptConnectionStringPassword(cs.getNodeValue());
    remote_path = rp.getNodeValue();

    return true;
    */
}


bool FsDatabase::createFolder(const std::wstring& path)
{
    if (path.empty())
        return false;

    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->createFolder(rpath);
    }


    std::wstring phys_path = makeFullPath(path);
    
    return xf_mkdir(phys_path);
}

bool FsDatabase::renameFile(const std::wstring& path,
                            const std::wstring& new_name)
{
    if (path.empty() || new_name.empty())
        return false;

    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        return db->renameFile(rpath, new_name);
    }
    



    std::wstring phys_path = makeFullPath(path);
    
    // if there is a trailing slash, strip it off
    if (phys_path.length() > 0 && phys_path[phys_path.length()-1] == PATH_SEPARATOR_CHAR)
        phys_path.erase(phys_path.length()-1, 1);
    
    std::wstring stub_path = kl::beforeLast(phys_path, PATH_SEPARATOR_CHAR);
    
    std::wstring new_path = stub_path;
    new_path += PATH_SEPARATOR_CHAR;
    new_path += new_name;
    
    if (!xf_move(phys_path, new_path))
        return false;
        
    // perhaps there is a definition file that needs renaming too
    std::wstring oldextf = ExtFileInfo::getConfigFilenameFromPath(getDefinitionDirectory(), phys_path);
    if (xf_get_file_exist(oldextf))
    {
        std::wstring newextf = ExtFileInfo::getConfigFilenameFromPath(getDefinitionDirectory(), new_path);
        xf_move(oldextf, newextf);
    }
    
    return true;
}

bool FsDatabase::moveFile(const std::wstring& src_path,
                          const std::wstring& dest_path)
{
    std::wstring src_cstr, src_rpath;
    std::wstring dest_cstr, dest_rpath;

    if (detectMountPoint(src_path, &src_cstr, &src_rpath))
    {

        if (detectMountPoint(dest_path, &dest_cstr, &src_rpath))
        {
            if (src_cstr == dest_cstr)
            {
                xd::IDatabasePtr db = lookupOrOpenMountDb(src_cstr);
                if (db.isNull())
                    return false;

                return db->moveFile(src_rpath, dest_rpath);
            }
        }

        return false;  // no cross-mount moves
    }
    
    if (detectMountPoint(dest_path, &dest_cstr, &src_rpath))
        return false;  // no cross-mount moves


    std::wstring src_phys_path = makeFullPath(src_path);
    std::wstring dest_phys_path = makeFullPath(dest_path);
    
    return xf_move(src_phys_path, dest_phys_path);
}


bool FsDatabase::copyFile(const std::wstring& src_path,
                          const std::wstring& dest_path)
{
    return true;
}

bool FsDatabase::copyData(const xd::CopyParams* info, xd::IJob* job)
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


    
    if (info->append)
    {
        xd::FormatDefinition fi;
        fi.format = xd::formatDefault;

        IXdfsSetPtr output = openSetEx(info->output, fi);
        if (output.isNull())
            return false;
    }
     else
    {
        deleteFile(info->output);
        if (!createTable(info->output, structure, NULL))
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

// from xdnative/database.cpp
static bool _deleteTree(const std::wstring& path)
{
    std::vector<std::wstring> to_remove;

    xf_dirhandle_t h = xf_opendir(path);
    xf_direntry_t info;
    while (xf_readdir(h, &info))
    {
        if (info.m_type == xfFileTypeNormal)
        {
            xf_remove(makePathName(path, L"", info.m_name));
        }
        if (info.m_type == xfFileTypeDirectory && info.m_name[0] != '.')
        {
            to_remove.push_back(makePathName(path, info.m_name));
        }
    }
    xf_closedir(h);

    std::vector<std::wstring>::iterator it;
    for (it = to_remove.begin(); it != to_remove.end(); ++it)
    {
        _deleteTree(*it);
    }

    xf_rmdir(path);

    return true;
}

bool FsDatabase::deleteFile(const std::wstring& _path)
{
    std::wstring path = makeFullPath(_path);
    
    if (path.empty() || path == L"/")
        return false;

    if (xf_get_file_exist(path + L".xdfs0"))
    {
        xf_remove(path + L".xdfs0");
        return true;
    }
    
    std::wstring cstr, rpath;
    if (detectMountPoint(_path, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        return db->deleteFile(rpath);
    }
    
    

    if (xf_get_directory_exist(path))
    {
        return _deleteTree(path);
    }
     else
    {
        if (xf_get_file_exist(path))
            return xf_remove(path);
             else
            return false;
    }
    
    return true;
}

bool FsDatabase::getFileExist(const std::wstring& path)
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

    std::wstring phys_path = makeFullPath(path);

    if (!xf_get_file_exist(phys_path))
        return false;

    return true;
}

bool FsDatabase::getLocalFileExist(const std::wstring& path)
{
    std::wstring good_path = makeFullPath(path);

    if (!xf_get_file_exist(good_path))
        return false;

    return true;
}

class XdfsFileInfo : public xd::IFileInfo
{
    XCM_CLASS_NAME("xdnative.FileInfo")
    XCM_BEGIN_INTERFACE_MAP(XdfsFileInfo)
        XCM_INTERFACE_ENTRY(xd::IFileInfo)
    XCM_END_INTERFACE_MAP()

public:

    XdfsFileInfo(FsDatabase* _db)
    {
        db = _db;
        db->ref();

        type = -1;
        format = -1;
        fetched_format = false;
        is_mount = false;
        row_count = 0;
    }

    virtual ~XdfsFileInfo()
    {
        db->unref();
    }

    const std::wstring& getName()
    {
        return name;
    }

    int getType()
    {
        // if type == -1, we will determine the file type from
        // the file format, which is determined by getFileFormat
        if (type == -1)
        {
            int format = getFormat();
            switch (format)
            {
                default:
                case xd::formatDefault:
                case xd::formatXbase:
                case xd::formatDelimitedText:
                case xd::formatFixedLengthText:
                    return xd::filetypeTable;
                case xd::formatText:
                    return xd::filetypeStream;
            }
        }
        
        return type;
    }
    
    int getFormat()
    {
        if (fetched_format)
            return format;
            
        // get the format and cache the result
        FsSetFormatInfo info;
        db->getFileFormat(phys_path, &info, FsSetFormatInfo::maskFormat);
        format = info.format;
        fetched_format = true;
        
        return format;
    }

    unsigned int getFlags()
    {
        return 0;
    }

    const std::wstring& getMimeType()
    {
        return mime_type;
    }
    
    long long getSize()
    {
        return xf_get_file_size(phys_path);
    }

    xd::rowpos_t getRowCount()
    {
        return row_count;
    }
    
    bool isMount()
    {
        return is_mount;
    }

    const std::wstring& getPrimaryKey()
    {
        return primary_key;
    }

    const std::wstring& getObjectId()
    {
        if (!object_id.empty())
            return object_id;
        object_id = db->getObjectIdFromPath(phys_path);
        return object_id;
    }
    
    const std::wstring& getUrl()
    {
        return url;
    }
    
public:

    std::wstring name;
    std::wstring mime_type;
    std::wstring path;
    std::wstring phys_path;
    std::wstring primary_key;
    std::wstring object_id;
    std::wstring url;
    int type;
    int format;
    bool is_mount;
    FsDatabase* db;
    bool fetched_format;
    xd::rowpos_t row_count;
};

xd::IFileInfoPtr FsDatabase::getFileInfo(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // if it's the root, it's automatically a folder
        if (rpath.empty() || rpath == L"/")
        {
            xdcommon::FileInfo* f = new xdcommon::FileInfo;
            f->name = kl::afterLast(path, L'/');
            f->type = xd::filetypeFolder;
            f->format = xd::formatDefault;
            f->is_mount = true;
            return static_cast<xd::IFileInfo*>(f);
        }
         else
        {
            std::wstring file_primary_key;
            std::wstring object_id;
            int file_type = xd::filetypeTable;
            int file_format = xd::formatDefault;
            int is_mount = -1;
            
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
                        object_id = file_info->getObjectId();
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
            f->object_id = object_id;
            
            return static_cast<xd::IFileInfo*>(f);
        }
    }



    std::wstring phys_path = makeFullPath(path);

    if (xf_get_file_exist(phys_path + L".xdfs0"))
    {
        xd::FormatDefinition def;
        if (!loadDefinitionFromFile(phys_path + L".xdfs0", &def))
            return xcm::null;

        xdcommon::FileInfo* f = new xdcommon::FileInfo;
        f->name = kl::afterLast(phys_path, PATH_SEPARATOR_CHAR);
        kl::trim(f->name);
        f->type = def.object_type;
        f->format = def.format;
        f->is_mount = true;
        
        return static_cast<xd::IFileInfo*>(f);
    }


    if (xf_get_directory_exist(phys_path))
    {
        xdcommon::FileInfo* f = new xdcommon::FileInfo;
        f->name = kl::afterLast(phys_path, PATH_SEPARATOR_CHAR);
        kl::trim(f->name);
        f->type = xd::filetypeFolder;
        f->format = xd::formatDefault;
        
        return static_cast<xd::IFileInfo*>(f);
    }
     else
    {
        if (xf_get_file_exist(phys_path))
        {
            XdfsFileInfo* f = new XdfsFileInfo(this);
            f->name = kl::afterLast(phys_path, PATH_SEPARATOR_CHAR);
            kl::trim(f->name);
            f->path = path;
            f->phys_path = phys_path;
            f->type = -1;  // auto-determine type
            // file info's format field is retrieved on demand
            
            if (f->name.find('.') != f->name.npos)
            {
                std::wstring ext = kl::afterLast(f->name, '.');
                if (kl::iequals(ext, "html") || kl::iequals(ext, "htm"))
                {
                    f->type = xd::filetypeStream;
                    f->mime_type = L"text/html";
                }
                 else if (kl::iequals(ext, "js"))
                {
                    f->type = xd::filetypeStream;
                    f->mime_type = L"application/javascript";
                }
            }

            return static_cast<xd::IFileInfo*>(f);
        }
    }
           
    return xcm::null;
}

xd::IFileInfoEnumPtr FsDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<xd::IFileInfoPtr>;


    // detect if the specified folder is a mount point
    
    std::wstring cstr, rpath;
    
    if (detectMountPoint(path, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return retval;
            
        delete retval;
        return db->getFolderInfo(rpath);
    }




#ifdef WIN32
    if ((path.empty() || path == L"/") && m_base_path.empty())
    {
        std::vector<std::wstring> entries;
        
        TCHAR buf[512];
        TCHAR* p;
        GetLogicalDriveStrings(512, buf);
        
        p = buf;
        while (*p)
        {
            TCHAR entry[2];
            entry[0] = *p;
            entry[1] = 0;
            entries.push_back(entry);
            
            p += wcslen(p)+1;
        }
        
        std::vector<std::wstring>::iterator it;
        for (it = entries.begin(); it != entries.end(); ++it)
        {
            xdcommon::FileInfo* f = new xdcommon::FileInfo;
            f->name = *it;
            f->type = xd::filetypeFolder;
            f->format = xd::formatDefault;
            retval->append(f);
        }
        
        return retval;
    }
#endif



    std::wstring phys_path = makeFullPath(path);
    xf_dirhandle_t handle = xf_opendir(phys_path);
    xf_direntry_t info;

    while (xf_readdir(handle, &info))
    {
        if (info.m_type == xfFileTypeDirectory)
        {
            if (!info.m_name.empty() && info.m_name[0] == L'.')
                continue;

            xdcommon::FileInfo* f = new xdcommon::FileInfo;
            f->name = info.m_name;
            kl::trim(f->name);
            f->type = xd::filetypeFolder;
            f->format = xd::formatDefault;
            retval->append(f);
        }
         else if (info.m_type == xfFileTypeNormal)
        {
            std::wstring ext = kl::afterLast(info.m_name, '.');
            kl::makeLower(ext);

            if (ext == L"xdfs0")
            {
                std::wstring full_path = phys_path;
                full_path += PATH_SEPARATOR_CHAR;
                full_path += info.m_name;

                xd::FormatDefinition fd;

                if (loadDefinitionFromFile(full_path, &fd))
                {
                    XdfsFileInfo* f = new XdfsFileInfo(this);
                    f->name = info.m_name.substr(0, info.m_name.length() - 6);
                    f->type = fd.object_type;
                    f->is_mount = true;
                    retval->append(f);
                }

                continue;
            }
            
            
            
            XdfsFileInfo* f = new XdfsFileInfo(this);

            f->name = info.m_name;
            kl::trim(f->name);
            
            f->path = path;
            if (f->path.length() == 0 || f->path[f->path.length()-1] != L'/')
                f->path += L'/';
            f->path += info.m_name;
            
            f->phys_path = makePathName(phys_path, L"", info.m_name, L"");
            
            // file info's type and format fields are retrieved on demand
            retval->append(f);
        }
    }

    xf_closedir(handle);

    return retval;
}

xd::IStreamPtr FsDatabase::openStream(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->openStream(rpath);
    }


    FileStream* stream = new FileStream;
    
    std::wstring phys_path = makeFullPath(path);
    
    if (!stream->open(phys_path))
    {
        delete stream;
        return xcm::null;
    }
    
    return static_cast<xd::IStream*>(stream);
}


static IXdfsSetPtr openFixedLengthTextSet(FsDatabase* db,
                                          const std::wstring& path)
{
    // we need to manually protect the ref count because
    // smart pointer operations happen in init()
    
    FixedLengthTextSet* set = new FixedLengthTextSet(db);
    set->ref();
    if (!set->init(path))
    {
        set->unref();
        return xcm::null;
    }
    
    IXdfsSetPtr retval = static_cast<IXdfsSet*>(set);
    set->unref();
    return retval;
}

static IXdfsSetPtr openDelimitedTextSet(FsDatabase* db,
                                        const std::wstring& path)
{
    // we need to manually protect the ref count because
    // smart pointer operations happen in init()
    

}


IXdfsSetPtr FsDatabase::openSetEx(const std::wstring& path, const xd::FormatDefinition& _fi)
{
    const xd::FormatDefinition* fi = &_fi;
    xd::FormatDefinition deffi;

    // check for ptr sets
    if (path.substr(0, 12) == L"/.temp/.ptr/")
    {
        std::wstring ptr_string = kl::afterLast(path, L'/');
        unsigned long l = (unsigned long)kl::hexToUint64(ptr_string.c_str());
        IXdsqlTablePtr sptr = (IXdsqlTable*)l;
        return sptr;
    }


    // if the file doesn't exist, bail out
    std::wstring phys_path = makeFullPath(path);

    if (xf_get_file_exist(phys_path + L".xdfs0"))
    {
        if (!loadDataView(path, &deffi))
            return xcm::null;
        if (deffi.format != xd::formatDefault)
            fi = &deffi;
        phys_path = deffi.data_path;
    }


    if (!xf_get_file_exist(phys_path))
        return xcm::null;
    
    std::wstring delimiters = L"";
    
    int format = fi->format;


    // if the native format was passed, have the database do it's best to
    // determine the format from the text definition or the file extension
    if (format == xd::formatDefault)
    {
        FsSetFormatInfo info;
        getFileFormat(phys_path, &info, FsSetFormatInfo::maskFormat | FsSetFormatInfo::maskDelimiters);
        format = info.format;
        delimiters = info.delimiters;
    }


    
    IXdfsSetPtr set;
    
    // open the set in the appropriate format
    if (format == xd::formatXbase) // dbf
    {
        XbaseSet* rawset = new XbaseSet(this);
        rawset->setObjectPath(path);
        rawset->ref();
        if (!rawset->init(phys_path))
        {
            rawset->unref();
            return xcm::null;
        }

        set = static_cast<IXdfsSet*>(rawset);
        rawset->unref();
    }
     else if (format == xd::formatTTB) // ttb
    {
        TtbSet* rawset = new TtbSet(this);
        rawset->setObjectPath(path);
        rawset->ref();
        if (!rawset->init(phys_path))
        {
            rawset->unref();
            return xcm::null;
        }

        set = static_cast<IXdfsSet*>(rawset);
        rawset->unref();
    }
     else if (format == xd::formatTypedDelimitedText) // icsv
    {
        DelimitedTextSet* rawset = new DelimitedTextSet(this);
        rawset->setObjectPath(path);
        rawset->ref();
        if (!rawset->init(phys_path))
        {
            rawset->unref();
            return xcm::null;
        }
    
        set = static_cast<IXdfsSet*>(rawset);
        rawset->unref();
    }
     else if (format == xd::formatDelimitedText)      // csv or tsv
    {
        DelimitedTextSet* rawset = new DelimitedTextSet(this);
        rawset->setObjectPath(path);
        rawset->ref();
        if (!rawset->init(phys_path))
        {
            rawset->unref();
            return xcm::null;
        }
    
        set = static_cast<IXdfsSet*>(rawset);
        rawset->unref();


        if (fi->format == xd::formatDefault)
        {
            xd::IDelimitedTextSetPtr tset = set;
            
            // default format specified
            if (delimiters != tset->getDelimiters())
                tset->setDelimiters(fi->delimiters, true);

            if (fi->determine_structure)
                tset->determineColumns(-1, NULL);
        }
         else if (fi->format == xd::formatDelimitedText)
        {
            xd::IDelimitedTextSetPtr tset = set;
            bool need_refresh = false;

            if (fi->delimiters != tset->getDelimiters())
            {
                tset->setDelimiters(fi->delimiters, false);
                need_refresh = true;
            }
            
            if (fi->text_qualifiers != tset->getTextQualifier())
            {
                tset->setTextQualifier(fi->text_qualifiers, false);
                need_refresh = true;
            }

            if (fi->line_delimiters != tset->getLineDelimiters())
            {
                tset->setLineDelimiters(fi->line_delimiters, false);
                need_refresh = true;
            }

            if (fi->first_row_column_names != tset->isFirstRowColumnNames())
            {
                tset->setDiscoverFirstRowColumnNames(false);
                tset->setFirstRowColumnNames(fi->first_row_column_names);

                need_refresh = true;
            }

            if (need_refresh || fi->determine_structure)
                tset->determineColumns(-1, NULL);
        }

    }       
     else if (format == xd::formatFixedLengthText) // fixed length
    {
        set = openFixedLengthTextSet(this, phys_path);
        if (set.isNull())
            return xcm::null;
    }

    return set;
}


xd::IIteratorPtr FsDatabase::query(const xd::QueryParams& qp)
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

    IXdsqlTablePtr tbl = openSetEx(qp.from, qp.format);
    if (tbl.isNull())
        return xcm::null;

    return tbl->createIterator(qp.columns, qp.order, qp.job);
}


bool FsDatabase::loadDataView(const std::wstring& path, xd::FormatDefinition* info)
{
    std::wstring phys_path = makeFullPath(path) + L".xdfs0";

    return loadDefinitionFromFile(phys_path, info);
}

bool FsDatabase::saveDataView(const std::wstring& path, const xd::FormatDefinition* info)
{
    std::wstring phys_path = makeFullPath(path) + L".xdfs0";
    
    return saveDefinitionToFile(phys_path, info);
}


xd::IStructurePtr FsDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<xd::IStructure*>(s);
}

static int xdToDelimitedTextEncoding(int xd_encoding)
{
    switch (xd_encoding)
    {
        case xd::encodingASCII:
        case xd::encodingISO8859_1:
            return DelimitedTextFile::encodingISO88591;
            break;
                
        case xd::encodingUTF8:
            return DelimitedTextFile::encodingUTF8;
            break;
            
        case xd::encodingUCS2:
        case xd::encodingUTF16:
            return DelimitedTextFile::encodingUTF16LE;
            break;
                
        case xd::encodingUTF16BE:
            return DelimitedTextFile::encodingUTF16BE;
            break;
            
        default:
            return -1;
    }
}


bool FsDatabase::createTable(const std::wstring& path,
                             xd::IStructurePtr structure,
                             xd::FormatDefinition* format_info)
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

        return db->createTable(rpath, structure, format_info);
    }


    size_t i, col_count = structure->getColumnCount();

    int format = xd::formatDefault;
    if (format_info)
        format = format_info->format;
    
    std::wstring phys_path = makeFullPath(path);

    // traverse the path given and create
    // all directories that do not exist

    std::wstring base_path = kl::beforeFirst(phys_path, PATH_SEPARATOR_CHAR);
    std::wstring remainder = kl::afterFirst(phys_path, PATH_SEPARATOR_CHAR);
    std::wstring old_remainder;

    while (1)
    {
        if (base_path.length() > 0)
        {
            if (!xf_get_directory_exist(base_path))
            {
                if (!xf_mkdir(base_path))
                    return false;
            }
        }
        
        old_remainder = remainder;

        base_path += PATH_SEPARATOR_CHAR;
        base_path += kl::beforeFirst(remainder, PATH_SEPARATOR_CHAR);
        remainder = kl::afterFirst(remainder, PATH_SEPARATOR_CHAR);

        // if the remainder returns the same result twice,
        // there are no more path separators
        if (!wcscasecmp(remainder.c_str(), old_remainder.c_str()))
        {
            break;
        }
    }

    if (format == xd::formatDefault)
    {
        // look for an extension to yield some guidance as to which
        // file format to use by default -- if no extension, assume csv
        std::wstring ext;
        int ext_pos = phys_path.find_last_of(L'.');
        if (ext_pos >= 0)
            ext = phys_path.substr(ext_pos+1);
        kl::makeLower(ext);

        // default to a ttb
        format = xd::formatTTB;

        if (ext == L"icsv")
            format = xd::formatTypedDelimitedText;
        else if (ext == L"dbf")
            format = xd::formatXbase;
        else if (ext == L"ttb")
            format = xd::formatTTB;
    }
    
    if (format == xd::formatXbase)
    {
        // create an xbase file with no rows in it
        
        std::vector<XbaseField> fields;
        
        // create vector of XbaseFields
        for (i = 0; i < col_count; ++i)
        {
            xd::IColumnInfoPtr col_info;
            col_info = structure->getColumnInfoByIdx(i);

            XbaseField f;
            f.name = kl::tostring(col_info->getName());
            f.type = xd2xbaseType(col_info->getType());
            f.width = col_info->getWidth();
            f.scale = col_info->getScale();
            f.ordinal = i;

            fields.push_back(f);
        }

        // create the xbase file
        XbaseFile file;
        if (!file.createFile(phys_path, fields))
            return false;
        file.closeFile();
        
        return xf_get_file_exist(phys_path);
    }
     else if (format == xd::formatTypedDelimitedText)
    {
        DelimitedTextFile file;

        // create a text-delimited file
        std::vector<std::wstring> fields;
        bool unicode_data_found = false;
        
        // create vector of fields
        for (i = 0; i < col_count; ++i)
        {
            xd::IColumnInfoPtr col_info;
            col_info = structure->getColumnInfoByIdx(i);
            if (col_info->getType() == xd::typeWideCharacter)
                unicode_data_found = true;
            
            if (!col_info->getCalculated())
            {
                std::wstring fld = col_info->getName();
                fld += L"(";
                    
                switch (col_info->getType())
                {
                    default:
                    case xd::typeCharacter:
                    case xd::typeWideCharacter:
                    {
                        wchar_t info[255];
                        swprintf(info, 255, L"C %d", col_info->getWidth());
                        fld += info;
                        break;
                    }
                    case xd::typeNumeric:
                    case xd::typeDouble:
                    case xd::typeInteger:
                    {
                        wchar_t info[255];
                        swprintf(info, 255, L"N %d %d", col_info->getWidth(), col_info->getScale());
                        fld += info;
                        break;
                    }
                    case xd::typeDate:
                        fld += L"D";
                        break;
                    case xd::typeDateTime:
                        fld += L"T";
                        break;
                    case xd::typeBoolean:
                        fld += L"B";
                        break;
                }
                    
                fld += L")";
                fields.push_back(fld);
            }
        }
        
        
        // determine the encoding we will use in the icsv

        int xd_encoding = xd::encodingUndefined;
        if (format_info)
            xd_encoding = format_info->encoding;
        
        if (xd_encoding == xd::encodingUndefined)
        {
            if (unicode_data_found)
                xd_encoding = xd::encodingUTF8;
                 else
                xd_encoding = xd::encodingISO8859_1;
        }

        int csv_encoding = xdToDelimitedTextEncoding(xd_encoding);
        if (csv_encoding == -1)
            return false; // unknown encoding
        
        if (format_info && !format_info->first_row_column_names)
        {
            // no field names
            fields.clear();
        }

        file.createFile(phys_path, fields, csv_encoding);
        file.closeFile();

        return xf_get_file_exist(phys_path);
    }
     else if (format == xd::formatDelimitedText)
    {
        // create the text-delimited file
        DelimitedTextFile file;
    

        // look for an extension to yield some guidance as to which
        // file format to use by default -- if no extension, assume csv
        std::wstring ext;
        int ext_pos = phys_path.find_last_of(L'.');
        if (ext_pos >= 0)
            ext = phys_path.substr(ext_pos+1);
             else
            ext = L"csv";
        kl::makeLower(ext);
    
    
        if (ext == L"tsv")
        {
            file.setDelimiters(L"\t");
            file.setTextQualifiers(L"");
        }
         else
        {
            // use the csv defaults as specified in
            // the DelimitedTextFile constructor
            if (format_info && format_info->line_delimiters.length() > 0)
                file.setLineDelimiters(format_info->line_delimiters);
            if (format_info && format_info->delimiters.length() > 0)
                file.setDelimiters(format_info->delimiters);
            if (format_info && format_info->text_qualifiers.length() > 0)
                file.setTextQualifiers(format_info->text_qualifiers);
        }
        
        
        // create a text-delimited file
        std::vector<std::wstring> fields;
        bool unicode_data_found = false;
        
        // create vector of fields
        for (i = 0; i < col_count; ++i)
        {
            xd::IColumnInfoPtr col_info;
            col_info = structure->getColumnInfoByIdx(i);
            if (col_info->getType() == xd::typeWideCharacter)
                unicode_data_found = true;
            
            if (!col_info->getCalculated())
                fields.push_back(col_info->getName());
        }
        
        
        // determine the encoding we will use in the csv

        int xd_encoding = xd::encodingUndefined;
        if (format_info)
            xd_encoding = format_info->encoding;
        
        if (xd_encoding == xd::encodingUndefined)
        {
            if (unicode_data_found)
                xd_encoding = xd::encodingUTF8;
                 else
                xd_encoding = xd::encodingISO8859_1;
        }

        int csv_encoding = xdToDelimitedTextEncoding(xd_encoding);
        if (csv_encoding == -1)
            return false; // unknown encoding
        

        if (format_info && !format_info->first_row_column_names)
        {
            // no field names
            fields.clear();
        }

        file.createFile(phys_path, fields, csv_encoding);
        file.closeFile();
        

        // save structure to edf
        DelimitedTextSet* tset = new DelimitedTextSet(this);
        if (!tset->init(phys_path))
        {
            delete tset;
            return false;
        }

        tset->setCreateStructure(structure);

        if (ext == L"tsv")
        {
            tset->setDelimiters(L"\t", false);
            tset->setTextQualifier(L"", false);
        }
            else
        {
            // use the csv defaults as specified in
            // the DelimitedTextFile constructor
            if (format_info && format_info->line_delimiters.length() > 0)
                tset->setLineDelimiters(format_info->line_delimiters, false);
            if (format_info && format_info->delimiters.length() > 0)
                tset->setDelimiters(format_info->delimiters, false);
            if (format_info && format_info->text_qualifiers.length() > 0)
                tset->setTextQualifier(format_info->text_qualifiers, false);
            if (format_info && !format_info->first_row_column_names)
                tset->setFirstRowColumnNames(false);
        }

        tset->saveConfiguration();
        delete tset;

        return true;
    }
     else if (format == xd::formatFixedLengthText)
    {
        // create the fixed length file
        xf_file_t file = xf_open(phys_path, xfCreate, xfReadWrite, xfShareNone);
        xf_close(file);

        /*
        // TODO: implement
        FixedLengthTextSet* tset = static_cast<FixedLengthTextSet*>(set.p);
        tset->setCreateStructure(struct_config);
        tset->saveConfiguration();
        */

        return xf_get_file_exist(phys_path);
    }

    return false;
}


bool FsDatabase::createStream(const std::wstring& path, const std::wstring& mime_type)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        return db->createStream(rpath, mime_type);
    }





    FileStream* stream = new FileStream;
    stream->ref();

    std::wstring phys_path = makeFullPath(path);
    
    if (!stream->create(phys_path))
    {
        stream->unref();
        return false;
    }
    
    
    // write out mime type

    if (mime_type.length() > 0)
    {
        std::wstring config_file = ExtFileInfo::getConfigFilenameFromPath(getDefinitionDirectory(), phys_path);

        ExtFileInfo fileinfo;
        fileinfo.load(config_file);
        fileinfo.deleteGroup(L"file_info");
        ExtFileEntry file_info_group = fileinfo.getGroup(L"file_info");
        file_info_group.addChild(L"type", mime_type);
        fileinfo.save(config_file);
    }

    stream->unref();
    return true;
}



xd::IRowInserterPtr FsDatabase::bulkInsert(const std::wstring& path)
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


    xd::FormatDefinition fi;
    fi.format = xd::formatDefault;

    IXdfsSetPtr set = openSetEx(path, fi);
    if (set.isNull())
        return xcm::null;
    
    return set->getRowInserter();
}

xd::IStructurePtr FsDatabase::describeTable(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->describeTable(rpath);
    }


    xd::FormatDefinition fi;
    fi.format = xd::formatDefault;

    IXdsqlTablePtr tbl = openSetEx(path, fi);
    if (tbl.isNull())
        return xcm::null;

    return tbl->getStructure();
}

bool FsDatabase::modifyStructure(const std::wstring& path,
                                 xd::IStructurePtr struct_config,
                                 xd::IJob* job)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        // action takes place in a mount
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return xcm::null;

        return db->modifyStructure(path, struct_config, job);
    }



    return false;
}


bool FsDatabase::execute(const std::wstring& command,
                         unsigned int flags,
                         xcm::IObjectPtr& result,
                         xd::IJob* job)
{
    m_error.clearError();
    result.clear();

    return doSQL(static_cast<xd::IDatabase*>(this),
                 command, flags, result, m_error, job);
}

bool FsDatabase::groupQuery(xd::GroupQueryParams* info, xd::IJob* job)
{
    return false;
}



xd::IIndexInfoPtr FsDatabase::createIndex(const std::wstring& path,
                                             const std::wstring& name,
                                             const std::wstring& expr,
                                             xd::IJob* job)
{
    return xcm::null;
}


bool FsDatabase::renameIndex(const std::wstring& path,
                             const std::wstring& name,
                             const std::wstring& new_name)
{
    return false;
}


bool FsDatabase::deleteIndex(const std::wstring& path,
                             const std::wstring& name)
{
    return false;
}


xd::IIndexInfoEnumPtr FsDatabase::getIndexEnum(const std::wstring& path)
{
    xcm::IVectorImpl<xd::IIndexInfoPtr>* vec;
    vec = new xcm::IVectorImpl<xd::IIndexInfoPtr>;

    return vec;
}


