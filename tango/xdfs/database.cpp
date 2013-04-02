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
#include "tango.h"
#include "database.h"
#include "xbaseset.h"
#include "delimitedtextset.h"
#include "fixedlengthtextset.h"
#include "xbase.h"
#include "delimitedtext.h"
#include "rawtext.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommon/sqlcommon.h"
#include "../xdcommon/extfileinfo.h"
#include "../xdcommon/filestream.h"
#include "../xdcommon/connectionstr.h"
#include <kl/url.h>


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






// -- FsDatabase class implementation --

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

    m_attr->setStringAttribute(tango::dbattrTempDirectory, xf_get_temp_path());
    m_attr->setStringAttribute(tango::dbattrDefinitionDirectory, xf_get_temp_path());

    
    m_db_mgr = tango::getDatabaseMgr();
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
    std::wstring result = m_attr->getStringAttribute(tango::dbattrTempDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}

std::wstring FsDatabase::getDefinitionDirectory()
{
    std::wstring result = m_attr->getStringAttribute(tango::dbattrDefinitionDirectory);
    if (result.empty())
    {
        result = xf_get_temp_path();
    }
    
    return result;
}

static std::wstring getSetMapKey(const std::wstring& path, tango::ISet* set)
{
    std::wstring result = L"[other] ";
    
    if (!set)
        return L"";
        
    tango::IDelimitedTextSetPtr dl = set;
    tango::IFixedLengthDefinitionPtr fl = set;
    if (dl.isOk())
        result = L"[delim] ";
    if (fl.isOk())
        result = L"[fixed] ";
    
    result += path;
    return result;
}

static std::wstring getSetMapKey(const std::wstring& path, int format)
{
    std::wstring result = L"[other] ";

    if (format == tango::formatDelimitedText)
        result = L"[delim] ";
     else if (format == tango::formatFixedLengthText)
        result = L"[fixed] ";
    
    result += path;
    return result;
}

void FsDatabase::registerSet(std::wstring path, tango::ISet* set)
{
    XCM_AUTO_LOCK(m_obj_mutex);
    
    std::wstring key = getSetMapKey(path, set);
    m_set_map[key] = set;
}

void FsDatabase::unregisterSet(tango::ISet* set)
{
    XCM_AUTO_LOCK(m_obj_mutex);

    std::map<std::wstring, tango::ISet*, kl::cmp_nocase>::iterator it;
    for (it = m_set_map.begin(); it != m_set_map.end(); ++it)
    {
        if (it->second == set)
        {
            m_set_map.erase(it);
            return;
        }
    }
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
        info->format = tango::formatNative;
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
        info->format = tango::formatFixedLengthText;
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
         
        info->format = tango::formatDelimitedText;
        return true;
    }
    
    // default format is fixed-length
    info->format = tango::formatFixedLengthText;
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


bool FsDatabase::getSetFormat(const std::wstring& path,
                              FsSetFormatInfo* info,
                              int info_mask)
{
    // get a physical pathname from the internal path name
    std::wstring phys_path = makeFullPath(path);
    if (!xf_get_file_exist(phys_path))
    {
        info->format = tango::formatNative;
        return false;
    }
       
    // figure out the config file name
    tango::IAttributesPtr attr = getAttributes();
    std::wstring definition_path = 
        attr->getStringAttribute(tango::dbattrDefinitionDirectory);
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
            info->format = tango::formatDelimitedText;
            
            if (delims.length() > 0)
                info->delimiters = delims;
                 else
                info->delimiters = L",";
            
            return true;
        }
         else if (format_str == L"text/fixed" ||
                  format_str == L"fixed_length")
        {
            info->format = tango::formatFixedLengthText;
            return true;
        }
         else if (format_str == L"text/plain")
        {
            info->format = tango::formatText;
            return true;
        }
    }

    // chop off and format the file extenstion
    std::wstring ext = kl::afterLast(path, L'.');
    if (ext.length() == path.length())
        ext = L"";

    // if a format is not specified in the ExtFileInfo,
    // use the file extension to determine the format
    if (0 == wcscasecmp(ext.c_str(), L"dbf"))
    {
        info->format = tango::formatXbase;
        return true;
    }
     else if (0 == wcscasecmp(ext.c_str(), L"csv") || 0 == wcscasecmp(ext.c_str(), L"icsv"))
    {
        info->format = tango::formatDelimitedText;
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
        
        info->format = tango::formatDelimitedText;
        return res;
    }
     else if (0 == wcscasecmp(ext.c_str(), L"tsv"))
    {
        info->format = tango::formatDelimitedText;
        info->delimiters = L"\t";
        return true;
    }
     else if (isTextFileExtension(ext))
    {
        info->format = tango::formatText;
        return true;
    }
    
    // read some of the file to see if we can determine the format
    return determineSetFormatInfo(phys_path, info);
}

void FsDatabase::close()
{

}

void FsDatabase::setDatabaseName(const std::wstring& name)
{

}

std::wstring FsDatabase::getDatabaseName()
{
    return L"";
}

int FsDatabase::getDatabaseType()
{
    return tango::dbtypeFilesystem;
}

std::wstring FsDatabase::getActiveUid()
{
    return L"";
}

tango::IAttributesPtr FsDatabase::getAttributes()
{
    return m_attr;
}

double FsDatabase::getFreeSpace()
{
    return 0.0;
}

double FsDatabase::getUsedSpace()
{
    return 0.0;
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

tango::IJobPtr FsDatabase::createJob()
{
    XCM_AUTO_LOCK(m_obj_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<tango::IJob*>(job);
}

tango::IJobPtr FsDatabase::getJob(tango::jobid_t job_id)
{
    XCM_AUTO_LOCK(m_obj_mutex);

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
                                  std::wstring& connection_str,
                                  std::wstring& remote_path)
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
        phys_path += L".xdmnt";
        
        if (xf_get_file_exist(phys_path))
        {
            kl::xmlnode root;
            if (!root.load(phys_path))
                return false;
            
            kl::xmlnode& cs = root.getChild(L"connection_str");
            if (cs.isEmpty())
                return false;
                
            kl::xmlnode& rp = root.getChild(L"remote_path");
            if (rp.isEmpty())
                return false;
            

            connection_str = xdcommon::decryptConnectionStringPassword(cs.getNodeValue());
            remote_path = rp.getNodeValue();

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




bool FsDatabase::checkCircularMount(const std::wstring& path,
                                    tango::IDatabasePtr remote_db, 
                                    const std::wstring remote_path)
{
    // TODO: implement
    return false;
}
                            
tango::IDatabasePtr FsDatabase::lookupOrOpenMountDb(const std::wstring& cstr)
{
    if (cstr.empty())
    {
        return static_cast<tango::IDatabase*>(this);
    }


    tango::IDatabasePtr db;
    
    std::map<std::wstring, tango::IDatabasePtr>::iterator it;
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
            tango::IAttributesPtr attr = db->getAttributes();
            
            if (attr)
            {
                attr->setStringAttribute(tango::dbattrTempDirectory, m_attr->getStringAttribute(tango::dbattrTempDirectory));
                attr->setStringAttribute(tango::dbattrDefinitionDirectory, m_attr->getStringAttribute(tango::dbattrDefinitionDirectory));
            }
        }      
        
        m_mounted_dbs[cstr] = db;
    }
    
    return db;
}



tango::IDatabasePtr FsDatabase::getMountDatabase(const std::wstring& path)
{
    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        return lookupOrOpenMountDb(cstr);
    }
  
    return xcm::null;
}
     
bool FsDatabase::setMountPoint(const std::wstring& _path,
                               const std::wstring& connection_str,
                               const std::wstring& remote_path)
{
    // convert path to real file name
    std::wstring path = makeFullPath(_path);
    path += L".xdmnt";
    
    // process connection string
    std::wstring final_connection_string = connection_str;
    if (final_connection_string.find(L"://") != -1)
        final_connection_string = xdcommon::urlToConnectionStr(connection_str);
    final_connection_string = xdcommon::encryptConnectionStringPassword(final_connection_string);


    kl::xmlnode root;
    
    root.setNodeName(L"xdmnt");

    kl::xmlnode& cstr = root.addChild(L"connection_str");
    cstr.setNodeValue(final_connection_string);
    
    kl::xmlnode& rpath = root.addChild(L"remote_path");
    rpath.setNodeValue(remote_path);
    
    return root.save(path);
}
                              
bool FsDatabase::getMountPoint(const std::wstring& _path,
                               std::wstring& connection_str,
                               std::wstring& remote_path)
{
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
}


bool FsDatabase::createFolder(const std::wstring& _path)
{
    std::wstring path = makeFullPath(_path);
    
    return xf_mkdir(path);
}

bool FsDatabase::renameFile(const std::wstring& path,
                            const std::wstring& new_name)
{
    std::wstring good_path = makeFullPath(path);
    
    // if there is a trailing slash, strip it off
    if (good_path.length() > 0 && good_path[good_path.length()-1] == PATH_SEPARATOR_CHAR)
        good_path.erase(good_path.length()-1, 1);
    
    std::wstring stub_path = kl::beforeLast(good_path, PATH_SEPARATOR_CHAR);
    
    std::wstring new_path = stub_path;
    new_path += PATH_SEPARATOR_CHAR;
    new_path += new_name;
    
    if (!xf_move(good_path, new_path))
        return false;
        
    // perhaps there is a definition file that needs renaming too
    std::wstring oldextf = ExtFileInfo::getConfigFilenameFromPath(getDefinitionDirectory(), good_path);
    if (xf_get_file_exist(oldextf))
    {
        std::wstring newextf = ExtFileInfo::getConfigFilenameFromPath(getDefinitionDirectory(), new_path);
        xf_move(oldextf, newextf);
    }
    
    return true;
}

bool FsDatabase::moveFile(const std::wstring& _path,
                          const std::wstring& _new_path)
{
    std::wstring path = makeFullPath(_path);
    std::wstring new_path = makeFullPath(_new_path);
    
    return xf_move(path, new_path);
}

bool FsDatabase::copyFile(const std::wstring& src_path,
                          const std::wstring& dest_path)
{
    return true;
}

bool FsDatabase::copyData(const tango::CopyInfo* info, tango::IJob* job)
{
    return false;
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
    
    
    
    std::wstring cstr, rpath;
    if (detectMountPoint(_path, cstr, rpath))
    {
        // check if they are deleting a mount file
        path += L".xdmnt";
        if (xf_get_file_exist(path))
            return xf_remove(path);
        
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
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

class XdfsFileInfo : public tango::IFileInfo
{
    XCM_CLASS_NAME("xdnative.FileInfo")
    XCM_BEGIN_INTERFACE_MAP(XdfsFileInfo)
        XCM_INTERFACE_ENTRY(tango::IFileInfo)
    XCM_END_INTERFACE_MAP()

public:

    XdfsFileInfo(IFsDatabasePtr _db)
    {
        db = _db;
        type = -1;
        format = -1;
        fetched_format = false;
        is_mount = false;
        row_count = 0;
    }

    const std::wstring& getName()
    {
        return name;
    }

    int getType()
    {
        // if type == -1, we will determine the file type from
        // the file format, which is determined by getSetFormat
        if (type == -1)
        {
            int format = getFormat();
            switch (format)
            {
                default:
                case tango::formatNative:
                case tango::formatXbase:
                case tango::formatDelimitedText:
                case tango::formatFixedLengthText:
                    return tango::filetypeSet;
                case tango::formatText:
                    return tango::filetypeStream;
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
        db->getSetFormat(path, &info, FsSetFormatInfo::maskFormat);
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

    tango::rowpos_t getRowCount()
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
        return object_id;
    }
    
public:

    std::wstring name;
    std::wstring mime_type;
    std::wstring path;
    std::wstring phys_path;
    std::wstring primary_key;
    std::wstring object_id;
    int type;
    int format;
    bool is_mount;
    IFsDatabasePtr db;
    bool fetched_format;
    tango::rowpos_t row_count;
};

tango::IFileInfoPtr FsDatabase::getFileInfo(const std::wstring& path)
{
    std::wstring phys_path = makeFullPath(path);



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
            
            return static_cast<tango::IFileInfo*>(f);
        }
    }





    if (xf_get_directory_exist(phys_path))
    {
        xdcommon::FileInfo* f = new xdcommon::FileInfo;
        f->name = kl::afterLast(phys_path, PATH_SEPARATOR_CHAR);
        kl::trim(f->name);
        f->type = tango::filetypeFolder;
        f->format = tango::formatNative;
        
        return static_cast<tango::IFileInfo*>(f);
    }
     else
    {
        if (xf_get_file_exist(phys_path))
        {
            XdfsFileInfo* f = new XdfsFileInfo(static_cast<IFsDatabase*>(this));
            f->name = kl::afterLast(phys_path, PATH_SEPARATOR_CHAR);
            kl::trim(f->name);
            f->path = path;
            f->phys_path = phys_path;
            f->type = -1;  // auto-determine type
            // file info's format field is retrieved on demand
            
            return static_cast<tango::IFileInfo*>(f);
        }
    }
           
    return xcm::null;
}

tango::IFileInfoEnumPtr FsDatabase::getFolderInfo(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IFileInfoPtr>* retval;
    retval = new xcm::IVectorImpl<tango::IFileInfoPtr>;


    // detect if the specified folder is a mount point
    
    std::wstring cstr, rpath;
    
    if (detectMountPoint(path, cstr, rpath))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
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
            f->type = tango::filetypeFolder;
            f->format = tango::formatNative;
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
            f->type = tango::filetypeFolder;
            f->format = tango::formatNative;
            retval->append(f);
        }
         else if (info.m_type == xfFileTypeNormal)
        {
            if (kl::afterLast(info.m_name, '.') == L"xdmnt")
            {
                info.m_name = info.m_name.substr(0, info.m_name.length() - 6);
                std::wstring mount_path = path;
                if (mount_path.length() == 0 || mount_path[mount_path.length()-1] != '/')
                    mount_path += L"/";
                mount_path += info.m_name;
                tango::IFileInfoPtr file_info = getFileInfo(mount_path);
                if (file_info)
                    retval->append(file_info);
                continue;
            }
            
            
            
            XdfsFileInfo* f = new XdfsFileInfo(static_cast<IFsDatabase*>(this));

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

tango::IStreamPtr FsDatabase::openStream(const std::wstring& path)
{
    FileStream* stream = new FileStream;
    
    std::wstring phys_path = makeFullPath(path);
    
    if (!stream->open(phys_path))
    {
        delete stream;
        return xcm::null;
    }
    
    return static_cast<tango::IStream*>(stream);
}


tango::ISetPtr FsDatabase::openSet(const std::wstring& path)
{
    return openSetEx(path, tango::formatNative);
}

static tango::ISetPtr openXbaseSet(tango::IDatabasePtr db,
                                   const std::wstring& path)
{
    // we need to manually protect the ref count because
    // smart pointer operations happen in init()
    
    XbaseSet* set = new XbaseSet;
    set->ref();
    if (!set->init(static_cast<tango::IDatabase*>(db), path))
    {
        set->unref();
        return xcm::null;
    }
    
    tango::ISetPtr retval = static_cast<tango::ISet*>(set);
    set->unref();
    return retval;
}

static tango::ISetPtr openFixedLengthTextSet(tango::IDatabasePtr db,
                                             const std::wstring& path)
{
    // we need to manually protect the ref count because
    // smart pointer operations happen in init()
    
    FixedLengthTextSet* set = new FixedLengthTextSet;
    set->ref();
    if (!set->init(static_cast<tango::IDatabase*>(db), path))
    {
        set->unref();
        return xcm::null;
    }
    
    tango::ISetPtr retval = static_cast<tango::ISet*>(set);
    set->unref();
    return retval;
}

static tango::ISetPtr openDelimitedTextSet(tango::IDatabasePtr db,
                                           const std::wstring& path)
{
    // we need to manually protect the ref count because
    // smart pointer operations happen in init()
    
    DelimitedTextSet* set = new DelimitedTextSet;
    set->ref();
    if (!set->init(static_cast<tango::IDatabase*>(db), path))
    {
        set->unref();
        return xcm::null;
    }
    
    tango::ISetPtr retval = static_cast<tango::ISet*>(set);
    set->unref();
    return retval;
}

tango::ISetPtr FsDatabase::openSetEx(const std::wstring& path, int format)
{
    // check for ptr sets
    if (path.substr(0, 12) == L"/.temp/.ptr/")
    {
        std::wstring ptr_string = kl::afterLast(path, L'/');
        unsigned long l = (unsigned long)hex2uint64(ptr_string.c_str());
        tango::ISet* sptr = (tango::ISet*)l;
        return sptr;
    }


    std::wstring cstr, rpath;
    if (detectMountPoint(path, cstr, rpath))
    {
        tango::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        
        if (db.isNull())
            return xcm::null;
        
        if (checkCircularMount(path, db, rpath))
            return xcm::null;
        
        tango::ISetPtr set = db->openSet(rpath);
        if (set.isNull())
            return xcm::null;
        
        set->setObjectPath(path);
        return set;
    }



    // if the file doesn't exist, bail out
    std::wstring phys_path = makeFullPath(path);
    if (!xf_get_file_exist(phys_path))
        return xcm::null;
    
    std::wstring delimiters = L"";
    
    // if the native format was passed, have the database do it's best to
    // determine the format from the text definition or the file extension
    if (format == tango::formatNative)
    {
        FsSetFormatInfo info;
        getSetFormat(path, &info, FsSetFormatInfo::maskFormat | FsSetFormatInfo::maskDelimiters);
        format = info.format;
        delimiters = info.delimiters;
    }

    // lookup the path in our set map and return the pointer
    // to the open set if we find an entry for this path
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        
        std::map<std::wstring, tango::ISet*, kl::cmp_nocase>::iterator it;
        it = m_set_map.find(getSetMapKey(phys_path, format));
        if (it != m_set_map.end())
            return static_cast<tango::ISet*>(it->second);
    }
    
    tango::ISetPtr set;
    
    // open the set in the appropriate format
    if (format == tango::formatXbase)
    {
        set = openXbaseSet(this, phys_path);
        if (set.isNull())
            return xcm::null;
    }
     else if (format == tango::formatDelimitedText)
    {
        set = openDelimitedTextSet(this, phys_path);
        if (set.isNull())
            return xcm::null;
        
        if (delimiters.length() > 0)
        {
            tango::IDelimitedTextSetPtr tset = set;
            
            if (delimiters != tset->getDelimiters())
                tset->setDelimiters(delimiters, true);
        }
    }       
     else if (format == tango::formatFixedLengthText)
    {
        set = openFixedLengthTextSet(this, phys_path);
        if (set.isNull())
            return xcm::null;
    }

    if (set.isOk())
        set->setObjectPath(path);

    return set;
}


tango::IIteratorPtr FsDatabase::createIterator(const std::wstring& path,
                                               const std::wstring& columns,
                                               const std::wstring& sort,
                                               tango::IJob* job)
{
    return xcm::null;
}


tango::IStructurePtr FsDatabase::createStructure()
{
    Structure* s = new Structure;
    return static_cast<tango::IStructure*>(s);
}

bool FsDatabase::createTable(const std::wstring& _path,
                             tango::IStructurePtr struct_config,
                             tango::FormatInfo* format_info)
{
    size_t i, col_count = struct_config->getColumnCount();

    int format = tango::formatNative;
    if (format_info)
        format = format_info->table_format;
    
    std::wstring path = makeFullPath(_path);

    // traverse the path given and create
    // all directories that do not exist

    std::wstring base_path = kl::beforeFirst(path, PATH_SEPARATOR_CHAR);
    std::wstring remainder = kl::afterFirst(path, PATH_SEPARATOR_CHAR);
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

    if (format == tango::formatNative)
    {
        // default to a csv
        format = tango::formatDelimitedText;
    }

    if (format == tango::formatXbase)
    {
        // create an xbase file with no rows in it
        
        std::vector<XbaseField> fields;
        
        // create vector of XbaseFields
        for (i = 0; i < col_count; ++i)
        {
            tango::IColumnInfoPtr col_info;
            col_info = struct_config->getColumnInfoByIdx(i);

            XbaseField f;
            f.name = kl::tostring(col_info->getName());
            f.type = tango2xbaseType(col_info->getType());
            f.width = col_info->getWidth();
            f.scale = col_info->getScale();
            f.ordinal = i;

            fields.push_back(f);
        }

        // create the xbase file
        XbaseFile file;
        if (!file.createFile(path, fields))
            return false;
        file.closeFile();
        
        return xf_get_file_exist(path);
    }

    if (format == tango::formatDelimitedText)
    {
        // look for an extension to yield some guidance as to which
        // file format to use by default -- if no extension, assume csv
        std::wstring ext;
        int ext_pos = path.find_last_of(L'.');
        if (ext_pos >= 0)
            ext = path.substr(ext_pos+1);
             else
            ext = L"csv";
        kl::makeLower(ext);
    
        
    

        // create the text-delimited file
        DelimitedTextFile file;
        
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
            tango::IColumnInfoPtr col_info;
            col_info = struct_config->getColumnInfoByIdx(i);
            if (col_info->getType() == tango::typeWideCharacter)
                unicode_data_found = true;
            
            if (!col_info->getCalculated())
            {
                if (ext == L"icsv")
                {
                    std::wstring fld = col_info->getName();
                    fld += L"(";
                    
                    switch (col_info->getType())
                    {
                        default:
                        case tango::typeCharacter:
                        case tango::typeWideCharacter:
                        {
                            wchar_t info[255];
                            swprintf(info, 255, L"C %d", col_info->getWidth());
                            fld += info;
                            break;
                        }
                        case tango::typeNumeric:
                        case tango::typeDouble:
                        case tango::typeInteger:
                        {
                            wchar_t info[255];
                            swprintf(info, 255, L"N %d %d", col_info->getWidth(), col_info->getScale());
                            fld += info;
                            break;
                        }
                        case tango::typeDate:
                            fld += L"D";
                            break;
                        case tango::typeDateTime:
                            fld += L"T";
                            break;
                        case tango::typeBoolean:
                            fld += L"B";
                            break;
                    }
                    
                    fld += L")";
                    fields.push_back(fld);
                }
                 else
                {
                    fields.push_back(col_info->getName());
                }
            }
        }
        
        
        int encoding;
        int tango_encoding = tango::encodingUndefined;
        if (format_info)
            tango_encoding = format_info->default_encoding;
        
        switch (tango_encoding)
        {
            case tango::encodingUndefined:
                if (unicode_data_found)
                    encoding = DelimitedTextFile::encodingUTF8;
                     else
                    encoding = DelimitedTextFile::encodingISO88591;
                break;
            
            case tango::encodingASCII:
            case tango::encodingISO8859_1:
                encoding = DelimitedTextFile::encodingISO88591;
                break;
                
            case tango::encodingUTF8:
                encoding = DelimitedTextFile::encodingUTF8;
                break;
            
            case tango::encodingUCS2:
            case tango::encodingUTF16:
                encoding = DelimitedTextFile::encodingUTF16LE;
                break;
                
            case tango::encodingUTF16BE:
                encoding = DelimitedTextFile::encodingUTF16BE;
                break;
            
            default:
                // invalid or non-supported encoding
                return false;
        }
        

        if (format_info && !format_info->first_row_column_names)
        {
            // no field names
            std::vector<std::wstring> f;
            file.createFile(path, f, encoding);
        }
         else
        {
            file.createFile(path, fields, encoding);
        }

        file.closeFile();
        

        if (ext != L"icsv")
        {
            tango::ISetPtr set = openSetEx(path, tango::formatDelimitedText);
            if (set.isNull())
                return false;
        
            DelimitedTextSet* tset = static_cast<DelimitedTextSet*>(set.p);
            tset->setCreateStructure(struct_config);

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

            set.clear();
        }

        return xf_get_file_exist(path);
    }

    if (format == tango::formatFixedLengthText)
    {
        // create the fixed length file
        xf_file_t file = xf_open(path, xfCreate, xfReadWrite, xfShareNone);
        xf_close(file);
        
        tango::ISetPtr set = openSetEx(path, tango::formatFixedLengthText);
        if (set.isNull())
            return false;
        
        FixedLengthTextSet* tset = static_cast<FixedLengthTextSet*>(set.p);
        tset->setCreateStructure(struct_config);
        tset->saveConfiguration();

        return xf_get_file_exist(path);
    }

    return false;
}


tango::IStreamPtr FsDatabase::createStream(const std::wstring& path, const std::wstring& mime_type)
{
    FileStream* stream = new FileStream;
    
    std::wstring phys_path = makeFullPath(path);
    
    if (!stream->create(phys_path))
    {
        delete stream;
        return xcm::null;
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

    return static_cast<tango::IStream*>(stream);
}



tango::IRelationEnumPtr FsDatabase::getRelationEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IRelationPtr>* relations;
    relations = new xcm::IVectorImpl<tango::IRelationPtr>;
    return relations;
}

tango::IRelationPtr FsDatabase::getRelation(const std::wstring& relation_id)
{
    return xcm::null;
}

tango::IRelationPtr FsDatabase::createRelation(const std::wstring& tag,
                                               const std::wstring& left_set_path,
                                               const std::wstring& right_set_path,
                                               const std::wstring& left_expr,
                                               const std::wstring& right_expr)
{
    return xcm::null;
}

bool FsDatabase::deleteRelation(const std::wstring& relation_id)
{
    return false;
}


tango::IRowInserterPtr FsDatabase::bulkInsert(const std::wstring& path)
{
    return xcm::null;
}


tango::IStructurePtr FsDatabase::describeTable(const std::wstring& path)
{
/*
    tango::ISetPtr set = openSet(path);
    if (set.isNull())
        return xcm::null;

    return set->getStructure();
*/
    // TODO: implement
    return xcm::null;
}

bool FsDatabase::modifyStructure(const std::wstring& path,
                                 tango::IStructurePtr struct_config,
                                 tango::IJob* job)
{
    return false;
}


bool FsDatabase::execute(const std::wstring& command,
                         unsigned int flags,
                         xcm::IObjectPtr& result,
                         tango::IJob* job)
{
    m_error.clearError();
    result.clear();

    return doSQL(static_cast<tango::IDatabase*>(this),
                 command, flags, result, m_error, job);
}

bool FsDatabase::groupQuery(tango::GroupQueryInfo* info, tango::IJob* job)
{
    return false;
}



tango::IIndexInfoPtr FsDatabase::createIndex(const std::wstring& path,
                                             const std::wstring& name,
                                             const std::wstring& expr,
                                             tango::IJob* job)
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


tango::IIndexInfoEnumPtr FsDatabase::getIndexEnum(const std::wstring& path)
{
    xcm::IVectorImpl<tango::IIndexInfoPtr>* vec;
    vec = new xcm::IVectorImpl<tango::IIndexInfoPtr>;

    return vec;
}


