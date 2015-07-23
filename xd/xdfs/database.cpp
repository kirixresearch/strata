/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#include <ctime>
#include "xdfs.h"
#include "database.h"
#include "xbaseset.h"
#include "ttbset.h"
#include "delimitedtextset.h"
#include "fixedlengthtextset.h"
#include "xlsxset.h"
#include "delimitedtext.h"
#include "rawtext.h"
#include "../../kscript/kscript.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/dbattr.h"
#include "../xdcommon/fileinfo.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "../xdcommon/extfileinfo.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/filestream.h"
#include "../xdcommon/connectionstr.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/formatdefinition.h"
#include "../xdcommon/cmndynamicset.h"
#include "../xdcommon/groupquery.h"
#include "../xdcommon/util.h"
#include "../xdcommon/idxutil.h"
#include "../xdcommon/relationinfo.h"
#include "ttbfile.h"
#include <kl/url.h>
#include <kl/hex.h>
#include <kl/json.h>
#include <kl/md5.h>
#include <kl/string.h>
#include <kl/file.h>


#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#endif


const wchar_t* xdfs_keywords =
                L"ADD,ALL,ALTER,AND,ANY,AS,ASC,BEGIN,BETWEEN,BOOL,BOOLEAN,"
                L"BOTH,BREAK,BY,CASE,CHAR,CHARACTER,CHECK,CLOSE,COLLATE,"
                L"COLUMN,COMMIT,CONNECT,CONTINUE,COUNT,CREATE,CURRENT,CURRENT_DATE,CURSOR,"
                L"DATE,DATETIME,DAY,DECIMAL,DECLARE,DEFAULT,DELETE,DESC,"
                L"DESCRIBE,DISTINCT,DO,DOUBLE,DROP,ELSE,ELSEIF,END,EXISTS,"
                L"FALSE,FETCH,FLOAT,FOR,FOREIGN,FROM,FULL,FUNCTION,GOTO,"
                L"GRANT,GROUP,HAVING,IF,IN,INDEX,INNER,INSERT,INT,INTEGER,"
                L"INTERSECT,INTO,IS,JOIN,KEY,LEFT,LEVEL,LIKE,LONG,MATCH,MONTH,NEW,"
                L"NOT,NULL,NUMERIC,OF,ON,OPEN,OPTION,OR,ORDER,OUTER,PRIMARY,"
                L"PRIVATE,PRECISION,PRIVILEGES,PROCEDURE,PROTECTED,PUBLIC,"
                L"READ,RESTRICT,RETURN,REVOKE,RIGHT,ROWS,SELECT,SESSION,SET,"
                L"SIZE,SHORT,SIGNED,SMALLINT,SOME,SUM,SWITCH,TABLE,THEN,THIS,TO,"
                L"TRUE,TYPE,UNION,UNIQUE,UNSIGNED,UPDATE,USER,USING,VALUES,VARCHAR,"
                L"VARYING,VIEW,VOID,WHEN,WHENEVER,WHERE,WHILE,WITH,YEAR,ZONE";

const wchar_t* xdfs_keywords2 = L"";

//const wchar_t* xdfs_invalid_column_chars =
//                           L"~!@#$%^&*()+{}|:\"<>?`-=[]\\;',./";
const wchar_t* xdfs_invalid_column_chars =
                             L"*|!@:\"<>()[]{}?\\:;'`-=,./\x00\x09\x0A\x0B\x0C\x0D\xFF\t\n";






// FsDatabase class implementation

FsDatabase::FsDatabase()
{
    m_last_job = 0;
    m_relations_filetime = 0;

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
                               xdfs_invalid_column_chars);
    m_attr->setStringAttribute(xd::dbattrColumnInvalidStartingChars,
                               xdfs_invalid_column_chars);
    m_attr->setStringAttribute(xd::dbattrTableInvalidChars,
                               xdfs_invalid_column_chars);
    m_attr->setStringAttribute(xd::dbattrTableInvalidStartingChars,
                               xdfs_invalid_column_chars);
    m_attr->setStringAttribute(xd::dbattrIdentifierQuoteOpenChar, L"[");
    m_attr->setStringAttribute(xd::dbattrIdentifierQuoteCloseChar, L"]");
    m_attr->setStringAttribute(xd::dbattrIdentifierCharsNeedingQuote, L"`~# $!@%^&(){}-+.'\"");    

    std::wstring temp_path = xf_get_temp_path();
    m_attr->setStringAttribute(xd::dbattrTempDirectory, temp_path);
    m_attr->setStringAttribute(xd::dbattrDefinitionDirectory, temp_path);

    m_db_mgr = xd::getDatabaseMgr();
}

FsDatabase::~FsDatabase()
{
    m_relations_mutex.lock();
    std::vector<RelationInfo*>::iterator rit;
    for (rit = m_relations.begin(); rit != m_relations.end(); ++rit)
        (*rit)->unref();
    m_relations.clear();
    m_relations_mutex.unlock();

    std::vector<JobInfo*>::iterator jit;
    for (jit = m_jobs.begin(); jit != m_jobs.end(); ++jit)
        (*jit)->unref();
}


bool FsDatabase::open(const std::wstring& path)
{
    if (path.length() > 0)
    {
        if (!xf_get_directory_exist(path))
            return false;

        std::wstring control_dir = path;
        
        #ifdef WIN32
        kl::replaceStr(control_dir, L"/", L"\\");
        #endif

        if (control_dir[control_dir.length()-1] != xf_path_separator_wchar)
            control_dir += xf_path_separator_wchar;
        control_dir += L"xdfs";

        if (control_dir == L"/xdfs")
        {
            control_dir = xf_get_temp_path();
            control_dir = L"/xdfs";
        }


        // make control directory
        if (!xf_get_directory_exist(control_dir))
            xf_mkdir(control_dir);

        // make temp directory
        std::wstring temp_dir = (control_dir + xf_path_separator_wchar) + L"temp";
        if (!xf_get_directory_exist(temp_dir))
            xf_mkdir(temp_dir);

        // make indexes directory
        std::wstring indexes_dir = (control_dir + xf_path_separator_wchar) + L"indexes";
        if (!xf_get_directory_exist(indexes_dir))
            xf_mkdir(indexes_dir);

        // make appdata directory
        std::wstring appdata_dir = (control_dir + xf_path_separator_wchar) + L"appdata";
        if (!xf_get_directory_exist(appdata_dir))
            xf_mkdir(appdata_dir);

        // make system directory
        std::wstring system_dir = (control_dir + xf_path_separator_wchar) + L"system";
        if (!xf_get_directory_exist(system_dir))
            xf_mkdir(system_dir);

        m_attr->setStringAttribute(xd::dbattrTempDirectory, temp_dir);
        m_attr->setStringAttribute(xd::dbattrDefinitionDirectory, temp_dir);

        m_ctrl_path = control_dir;
        m_base_path = path;
    }
     else
    {
        m_base_path = path;
        m_ctrl_path = xf_get_temp_path();
    }


    getAttributes()->setStringAttribute(xd::dbattrFilesystemPath, m_base_path);

    return true;
}

std::wstring FsDatabase::getTempFileDirectory()
{
    return m_attr->getStringAttribute(xd::dbattrTempDirectory);
}

std::wstring FsDatabase::getDefinitionDirectory()
{
    return m_attr->getStringAttribute(xd::dbattrDefinitionDirectory);
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

static bool determineSetFormatInfo(xd::IStream* stream, xd::FormatDefinition* info)
{
    // first look for magic numbers
    unsigned char buf[64];
    memset(buf, 0, sizeof(buf));


    if (stream->seek(0))
    {
        if (stream->read(buf, 64, NULL))
        {
            // XLSX file
            unsigned char xlsx[8] = { 0x50,0x4B,0x03,0x04,0x14,0x00,0x06,0x00 };
            if (0 == memcmp(buf, xlsx, 8))
            {
                info->format = xd::formatXLSX;
                return true;
            }

            // ttb files
            if (buf[0] == 0x99 && buf[1] == 0x22 && buf[2] == 0xaa && buf[3] == 0xdd)
            {
                info->format = xd::formatTTB;
                return true;
            }

            // xbase files -- first check for one of the possible file signatures
            if (buf[0] == 0x03 || buf[0] == 0x30 || buf[0] == 0x31 || buf[0] == 0x32 || buf[0] == 0x43 || buf[0] == 0x63 || buf[0] == 0x83 || buf[0] == 0x8B)
            {
                if (buf[2] <= 12 && buf[3] <= 31) // check the MMDD of the YYMMDD signature for valid values
                {
                    // look at the first field entry and make sure it conforms to the dbf format
                    if (strchr("BCDFGILMNTY@+0", buf[43]))
                    {
                        // check to make sure field decimals byte is < 20
                        if (buf[49] <= 20)
                        {
                            info->format = xd::formatXbase;
                            return true;
                        }
                    }
                }
            }
        }
    }


    if (false)
    {
        info->format = xd::formatFixedLengthText;
        info->encoding = xd::encodingEBCDIC;
        return true;
    }


    BufferedTextFile f;
    if (!f.open(stream))
    {
        info->format = xd::formatDefault;
        return false;
    }
    
    std::vector<std::wstring> lines;
    
    size_t i;
    wchar_t ch;
    std::wstring line;

    for (i = 0; i < 4096; ++i)
    {
        if (f.eof())
            break;

        ch = f.getChar();
        f.skip(1);

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
    
    f.close();
    
    // there's nothing in the file, return that it is a fixed-length file
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
            info->delimiter = L",";
         else if (max == tab_line_count)
            info->delimiter = L"\t";
         else if (max == semicolon_line_count)
            info->delimiter = L";";
         else if (max == colon_line_count)
            info->delimiter = L":";
         else if (max == pipe_line_count)
            info->delimiter = L"|";
         else if (max == tilde_line_count)
            info->delimiter = L"~";
         
        info->format = xd::formatDelimitedText;
        return true;
    }
    
    // default format is fixed-length
    info->format = xd::formatFixedLengthText;
    return true;
}


static bool isTextFileExtension(const std::wstring& _ext)
{
    std::string ext = kl::tostring(_ext);
    kl::makeUpper(ext);
    
    static const char* text_types[] = { "ASP", "C", "CC", "CPP", "CS", "CXX",
                                        "H", "HPP", "JAVA", "JS", "JSP", "PL", "PHP", "PHTML", "RC", "SQL",
                                        (const char*)0 };
    
    for (size_t i = 0; text_types[i] != NULL; ++i)
        if (ext == text_types[i])
            return true;
    
    return false;
}


bool FsDatabase::getFileFormat(const std::wstring& path,
                               xd::IStream* stream,
                               xd::FormatDefinition* info,
                               bool discover_delimiters)
{
    std::wstring ext;

    if (stream == NULL && path.substr(0, 12) == L"streamptr://")
    {
        unsigned long l = (unsigned long)kl::hexToUint64(path.substr(12));
        stream = (xd::IStream*)l;
        ext = xf_get_extension_from_mimetype(stream->getMimeType());
    }
     else
    {
        // find the file extenstion
        size_t ext_pos = path.find_last_of('.');
        if (ext_pos != path.npos)
        {
            ext = path.substr(ext_pos+1);
            kl::makeLower(ext);
        }
    }


    // certain extensions indicate a file format unambiguously
    if (ext == L"ttb")
    {
        info->format = xd::formatTTB;
        return true;
    }
     else if (ext == L"dbf")
    {
        info->format = xd::formatXbase;
        return true;
    }
     else if (ext == L"icsv")
    {
        info->format = xd::formatTypedDelimitedText;
        return true;
    }
     else if (ext == L"tsv")
    {
        info->format = xd::formatDelimitedText;
        info->delimiter = L"\t";
        return true;
    }
     else if (ext == L"xlsx")
    {
        info->format = xd::formatXLSX;
        return true;
    }
     else if (ext == L"ebc")
    {
        info->format = xd::formatFixedLengthText;
        return true;
    }
     else if (isTextFileExtension(ext))
    {
        info->format = xd::formatText;
        return true;
    }
     else if (ext == L"csv")
    {
        info->format = xd::formatDelimitedText;
        info->delimiter = L",";
        
        if (stream && discover_delimiters)
        {
            FileStream* f = NULL;
            if (!stream)
            {
                f = new FileStream;
                stream = f;
            }

            // the delimiter is almost certainly a comma, but
            // sometimes is something else, such as a semicolon
            bool res = determineSetFormatInfo(stream, info);
        
            // because the file extension is csv, don't let determineSetFormatInfo
            // guess anything different (happens sometimes with one-column csv's,
            // because there are no delimiters)
        
            info->format = xd::formatDelimitedText;

            delete f;
            return res;
        }

        return true;
    }
     else
    {
        xd::IStream* s = stream;
        if (s)
        {
            s->ref();
        }
         else
        {
            FileStream* f = new FileStream;
            f->ref();
            if (!f->open(path))
            {
                f->unref();
                info->format = xd::formatFixedLengthText;
                return true;
            }
            s = f;
        }

        determineSetFormatInfo(s, info);
        s->unref();
        return true;
    }
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
    return L"admin";
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
    KL_AUTO_LOCK(m_obj_mutex);

    m_last_job++;

    JobInfo* job = new JobInfo;
    job->setJobId(m_last_job);
    job->ref();
    m_jobs.push_back(job);

    return static_cast<xd::IJob*>(job);
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

static bool _mkdirTree(const std::wstring& _path)
{
    std::vector<std::wstring> to_add;

    std::wstring path = _path;
    to_add.push_back(path);

    while (true)
    {
        path = xf_get_file_directory(path);

        if (path.length() == 0)
            break;

        if (xf_get_directory_exist(path))
            break;

        to_add.push_back(path);
    }

    std::vector<std::wstring>::reverse_iterator rit;
    for (rit = to_add.rbegin(); rit != to_add.rend(); ++rit)
    {
        if (!xf_mkdir(*rit))
            break;
    }

    return true;
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

    if (kl::isUrl(file_path))
    {
        // we don't support other urls
        return _path;
    }
    
    
    // convert /.appdata to /xdfs/appdata
    kl::replaceStr(file_path, L".appdata", L"xdfs/appdata");

        
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
    if (kl::isFileUrl(path))
        return false;

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
        phys_path += L".xddef";
        
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


    std::wstring ext = kl::afterLast(path, '.');
    if (kl::iequals(ext, L"TTB"))
    {
        unsigned char guid[16];
        bool guid_ok = false;

        {
            // fetch TTB object id from the header
            TtbTable ttb;
            if (ttb.open(path))
            {
                guid_ok = ttb.getGuid(guid);
                ttb.close();
            }
        }

        if (guid_ok)
        {
            object_id = kl::stdswprintf(L"%08x%08x%08x%08x",
                                         buf2int(guid), buf2int(guid+4), buf2int(guid+8), buf2int(guid+12));
            return object_id;
        }

    }



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
    phys_path += L".xddef";
    
    // make sure containing directory exists
    std::wstring dir = xf_get_file_directory(phys_path);
    if (!xf_get_directory_exist(dir))
        _mkdirTree(dir);

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


    return saveDefinitionToFile(phys_path, fd);
}
                              
bool FsDatabase::getMountPoint(const std::wstring& path,
                               std::wstring& connection_str,
                               std::wstring& remote_path)
{
    // convert path to real file name
    std::wstring phys_path = makeFullPath(path);
    phys_path += L".xddef";

    xd::FormatDefinition fd;

    if (!loadDefinitionFromFile(phys_path, &fd))
        return false;

    connection_str = fd.data_connection_string;
    remote_path = fd.data_path;

    return true;
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

    std::wstring phys_path = makeFullPath(path);

    // if there is a trailing slash, strip it off
    if (phys_path.length() > 0 && phys_path[phys_path.length()-1] == PATH_SEPARATOR_CHAR)
        phys_path.erase(phys_path.length()-1, 1);
    
    std::wstring stub_path = kl::beforeLast(phys_path, PATH_SEPARATOR_CHAR);
    
    std::wstring new_path = stub_path;
    new_path += PATH_SEPARATOR_CHAR;
    new_path += new_name;


    if (xf_get_file_exist(phys_path + L".xddef"))
    {
        // mount file itself being renamed
        if (!xf_move(phys_path + L".xddef", new_path + L".xddef"))
            return false;
        return true;
    }

    std::wstring cstr, rpath;
    if (detectMountPoint(path, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        return db->renameFile(rpath, new_name);
    }
    



    
    

    

    if (xf_get_file_exist(phys_path + L".ttb"))
    {
        if (!xf_move(phys_path + L".ttb", new_path + L".ttb"))
            return false;
        xf_move(phys_path + L".map", new_path + L".map");
    }
     else
    {
        if (!xf_move(phys_path, new_path))
            return false;
    }



        
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
    
    if (xf_get_file_exist(src_phys_path + L".ttb"))
    {
        if (xf_get_file_exist(dest_phys_path + L".ttb"))
        {
            // destination already exists
            return false;
        }

        xf_remove(dest_phys_path + L".map");
        xf_move(src_phys_path + L".map", dest_phys_path + L".map");
        return xf_move(src_phys_path + L".ttb", dest_phys_path + L".ttb");
    }
     else if (xf_get_file_exist(src_phys_path + L".xddef"))
    {
        return xf_move(src_phys_path + L".xddef", dest_phys_path + L".xddef");
    }
     else
    {
        return xf_move(src_phys_path, dest_phys_path);
    }
}


bool FsDatabase::copyFile(const std::wstring& src_path,
                          const std::wstring& dest_path)
{
    return true;
}

bool FsDatabase::copyData(const xd::CopyParams* info, xd::IJob* job)
{
    xd::IIteratorPtr iter;
    xd::Structure structure;

    if (info->iter_input.isOk())
    {
        iter = info->iter_input;
        structure = iter->getStructure();
        if (structure.isNull())
            return false;
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
        IXdsqlTablePtr output = openTable(info->output, info->output_format);
        if (output.isNull())
            return false;
    }
     else
    {
        deleteFile(info->output);

        xd::FormatDefinition fd = info->output_format;
        fd.columns = structure.columns;
        if (!createTable(info->output, fd))
            return false;
    }


    std::wstring cstr, rpath;
    if (detectMountPoint(info->output, &cstr, &rpath))
    {
        xd::IDatabasePtr db = lookupOrOpenMountDb(cstr);
        if (db.isNull())
            return false;

        xdcmnInsert(db, iter, rpath, info->copy_columns, info->where, info->limit, job);
    }
     else
    {
        xdcmnInsert(static_cast<xd::IDatabase*>(this), iter, info->output, info->copy_columns, info->where, info->limit, job);
    }

    return true;
}


bool FsDatabase::deleteFile(const std::wstring& _path)
{
    std::wstring path = makeFullPath(_path);
    
    if (path.empty() || path == L"/")
        return false;

    if (xf_get_file_exist(path + L".xddef"))
    {
        xf_remove(path + L".xddef");
        return true;
    }
    
    if (xf_get_file_exist(path + L".ttb"))
    {
        xf_remove(path + L".ttb");
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


    if (xf_get_file_exist(phys_path + L".ttb"))
        return true;

    if (xf_get_file_exist(phys_path + L".xddef"))
        return true;

    if (xf_get_file_exist(phys_path))
        return true;

    return false;
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
        if (format != -1)
            return format;
            
        // get the format and cache the result
        xd::FormatDefinition info;
        db->getFileFormat(phys_path, NULL, &info);
        format = info.format;
        
        return format;
    }

    unsigned int getFlags()
    {
        if (format == xd::formatTTB)
            return xd::sfFastRowCount;

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
        if (getFormat() == xd::formatTTB)
        {
            TtbTable ttb;
            if (ttb.open(phys_path))
            {
                xd::rowpos_t deleted_row_count = 0;
                xd::rowpos_t res = ttb.getRowCount(&deleted_row_count);
                ttb.close();
                return (res - deleted_row_count);
            }

            return 0;
        }

        return row_count;
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
    std::wstring cstr;
    std::wstring rpath;
    FsDatabase* db;
    xd::rowpos_t row_count;
};

xd::IFileInfoPtr FsDatabase::getFileInfo(const std::wstring& path)
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
            f->is_mount = false;
            if (is_mount == 1)
            {
                f->is_mount = true;
                f->cstr = cstr;
                f->rpath = rpath;
            }
            f->primary_key = file_primary_key;
            f->object_id = object_id;
            
            return static_cast<xd::IFileInfo*>(f);
        }
    }



    std::wstring phys_path = makeFullPath(path);

    if (xf_get_file_exist(phys_path + L".xddef"))
    {
        xd::FormatDefinition def;
        if (!loadDefinitionFromFile(phys_path + L".xddef", &def))
            return xcm::null;

        xdcommon::FileInfo* f = new xdcommon::FileInfo;
        f->name = kl::afterLast(phys_path, PATH_SEPARATOR_CHAR);
        kl::trim(f->name);
        f->type = def.object_type;
        f->format = def.format;
        f->object_id = def.object_id;
        f->is_mount = true;
        
        return static_cast<xd::IFileInfo*>(f);
    }

    if (xf_get_file_exist(phys_path + L".ttb"))
    {
        XdfsFileInfo* f = new XdfsFileInfo(this);
        f->name = kl::afterLast(phys_path, PATH_SEPARATOR_CHAR);
        kl::trim(f->name);
        f->type = xd::filetypeTable;
        f->format = xd::formatTTB;
        f->path = path;
        f->phys_path = phys_path + L".ttb";

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
            if (kl::iequals(info.m_name, L"xdfs"))
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

            if (ext == L"tmp" || ext == L"xdi")
            {
                // don't show these extensions
                continue;
            }

            if (ext == L"xddef")
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
                    f->cstr = fd.data_connection_string;
                    f->rpath = fd.data_path;
                    retval->append(f);
                }

                continue;
            }
             else if (ext == L"ttb")
            {
                XdfsFileInfo* f = new XdfsFileInfo(this);
                f->name = info.m_name.substr(0, info.m_name.length() - 4);
                f->type = xd::filetypeTable;
                f->format = xd::formatTTB;
                retval->append(f);
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

IXdsqlTablePtr FsDatabase::openTable(const std::wstring& path, const xd::FormatDefinition& format_definition, xd::IJob* job)
{
    const xd::FormatDefinition* fi = &format_definition;
    xd::FormatDefinition deffi;

    // check for table ptr sets
    if (path.substr(0, 12) == L"/.temp/.ptr/")
    {
        std::wstring ptr_string = kl::afterLast(path, L'/');
        unsigned long l = (unsigned long)kl::hexToUint64(ptr_string);
        IXdsqlTablePtr sptr = (IXdsqlTable*)l;
        return sptr;
    }


    // if the file doesn't exist, bail out
    std::wstring phys_path;
    

    if (path.substr(0, 12) == L"streamptr://")
    {
        phys_path = path;
    }
     else
    {
        phys_path = makeFullPath(path);

        if (xf_get_file_exist(phys_path + L".xddef"))
        {
            if (!loadDefinition(path, &deffi))
                return xcm::null;
            if (deffi.format != xd::formatDefault)
                fi = &deffi;
            phys_path = makeFullPath(deffi.data_path);
        }

        if (xf_get_file_exist(phys_path + L".ttb"))
        {
            phys_path += L".ttb";
        }

        if (!xf_get_file_exist(phys_path))
            return xcm::null;
    }




        
    int format = fi->format;

    // if the native format was passed, have the database do it's best to
    // determine the format from the text definition or the file extension
    if (format == xd::formatDefault)
    {
        xd::FormatDefinition info;
        getFileFormat(phys_path, NULL, &info, false);
        format = info.format;
    }


    // open the set in the appropriate format
    if (format == xd::formatXbase) // dbf
    {
        XbaseSet* rawtable = new XbaseSet(this);
        rawtable->setObjectPath(path);
        rawtable->ref();
        if (!rawtable->init(phys_path))
        {
            rawtable->unref();
            return xcm::null;
        }

        IXdsqlTablePtr table = static_cast<IXdfsSet*>(rawtable);
        rawtable->unref();
        return table;
    }
     else if (format == xd::formatTTB) // ttb
    {
        TtbSet* rawtable = new TtbSet(this);
        rawtable->setObjectPath(path);
        rawtable->ref();
        if (!rawtable->init(phys_path))
        {
            rawtable->unref();
            return xcm::null;
        }

        IXdsqlTablePtr table = static_cast<IXdfsSet*>(rawtable);
        rawtable->unref();
        return table;
    }
     else if (format == xd::formatTypedDelimitedText) // icsv
    {
        xd::FormatDefinition default_format;
        default_format.format = xd::formatDefault;

        DelimitedTextSet* rawtable = new DelimitedTextSet(this);
        rawtable->ref();
        rawtable->setObjectPath(path);
        if (!rawtable->init(phys_path, default_format))
        {
            rawtable->unref();
            return xcm::null;
        }
    
        IXdsqlTablePtr table = static_cast<IXdfsSet*>(rawtable);
        rawtable->unref();
        return table;
    }
     else if (format == xd::formatDelimitedText)      // csv or tsv
    {
        DelimitedTextSet* rawtable = new DelimitedTextSet(this);
        rawtable->ref();
        rawtable->setObjectPath(path);
        if (!rawtable->init(phys_path, *fi))
        {
            rawtable->unref();
            return xcm::null;
        }
    
        IXdsqlTablePtr table = static_cast<IXdfsSet*>(rawtable);
        rawtable->unref();
        return table;
    }       
     else if (format == xd::formatFixedLengthText) // fixed length
    {
        FixedLengthTextSet* rawtable = new FixedLengthTextSet(this);
        rawtable->ref();
        rawtable->setObjectPath(path);
        if (!rawtable->init(phys_path, *fi))
        {
            rawtable->unref();
            return xcm::null;
        }

        IXdsqlTablePtr table = static_cast<IXdfsSet*>(rawtable);
        rawtable->unref();
        return table;
    }
     else if (format == xd::formatXLSX) // XLSX spreadsheet
    {
        XlsxSet* rawtable = new XlsxSet(this);
        rawtable->ref();
        rawtable->setObjectPath(path);
        if (!rawtable->init(phys_path))
        {
            rawtable->unref();
            return xcm::null;
        }

        IXdsqlTablePtr table = static_cast<IXdfsSet*>(rawtable);
        rawtable->unref();
        return table;
    }
     else
    {
        // unknown format
        return xcm::null;
    }
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


    if (qp.where.length() > 0)
    {
        // create an iterator for the table we are interested in
        IXdsqlTablePtr table = openTable(qp.from, qp.format);
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


    IXdsqlTablePtr tbl = openTable(qp.from, qp.format);
    if (tbl.isNull())
        return xcm::null;

    return tbl->createIterator(qp.columns, qp.order, qp.job);
}


bool FsDatabase::detectStreamFormat(const std::wstring& path, xd::FormatDefinition* format_definition, const xd::FormatDefinition* defaults, xd::IJob* job)
{
    std::wstring phys_path = makeFullPath(path);

    if (!kl::isUrl(path) && xf_get_file_exist(phys_path + L".xddef"))
        return loadDefinitionFromFile(phys_path + L".xddef", format_definition);

    if (path.substr(0, 12) == L"streamptr://" || xf_get_file_exist(phys_path))
    {
        xd::FormatDefinition fd;
        if (defaults)
            fd = *defaults;

        IXdfsSetPtr set = openTable(path, fd, job);
        if (set.isNull())
            return false;

        set->getFormatDefinition(format_definition);
        return true;
    }
     else
    {
        return false;
    }

    return false;
}


bool FsDatabase::loadDefinition(const std::wstring& path, xd::FormatDefinition* def)
{
    std::wstring phys_path = makeFullPath(path);

    if (xf_get_file_exist(phys_path + L".xddef"))
        return loadDefinitionFromFile(phys_path + L".xddef", def);
         else;
        return false;
}


bool FsDatabase::saveDefinition(const std::wstring& path, const xd::FormatDefinition& def)
{
    std::wstring phys_path = makeFullPath(path) + L".xddef";
    
    // make sure containing directory exists
    std::wstring dir = xf_get_file_directory(phys_path);
    if (!xf_get_directory_exist(dir))
        _mkdirTree(dir);

    if (def.object_id.empty())
    {
        // add an object id
        xd::FormatDefinition fd = def;
        fd.object_id = kl::getUniqueString();
        return saveDefinitionToFile(phys_path, fd);
    }
     else
    {
        return saveDefinitionToFile(phys_path, def);
    }
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


bool FsDatabase::createTable(const std::wstring& path, const xd::FormatDefinition& format_definition)
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


    int format = format_definition.format;
    
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
    
    if (format == xd::formatTTB)
    {
        if (!kl::icontains(phys_path, L".ttb"))
            phys_path += L".ttb";

        return TtbTable::create(phys_path, format_definition.columns.columns);
    }
     else if (format == xd::formatXbase)
    {
        // create an xbase file with no rows in it
        
        std::vector<XbaseField> fields;
        
        // create vector of XbaseFields
        int i, col_count = (int)format_definition.columns.size();
        for (i = 0; i < col_count; ++i)
        {
            const xd::ColumnInfo& col_info = format_definition.columns[i];

            XbaseField f;
            f.name = kl::tostring(col_info.name);
            f.type = xd2xbaseType(col_info.type);
            f.width = col_info.width;
            f.scale = col_info.scale;
            f.ordinal = i;

            fields.push_back(f);
        }

        // create the xbase file
        XbaseFile file;
        if (!file.create(phys_path, fields))
            return false;
        file.close();
        
        return xf_get_file_exist(phys_path);
    }
     else if (format == xd::formatTypedDelimitedText)
    {
        DelimitedTextFile file;

        // create a text-delimited file
        std::vector<std::wstring> fields;
        bool unicode_data_found = false;
        
        // create vector of fields
        int i, col_count = (int)format_definition.columns.size();

        for (i = 0; i < col_count; ++i)
        {
            const xd::ColumnInfo& col_info = format_definition.columns[i];
            if (col_info.type == xd::typeWideCharacter)
                unicode_data_found = true;
            
            if (!col_info.calculated)
            {
                std::wstring fld = col_info.name;
                fld += L"(";
                    
                switch (col_info.type)
                {
                    default:
                    case xd::typeCharacter:
                    case xd::typeWideCharacter:
                    {
                        wchar_t info[255];
                        swprintf(info, 255, L"C %d", col_info.width);
                        fld += info;
                        break;
                    }
                    case xd::typeNumeric:
                    case xd::typeDouble:
                    case xd::typeInteger:
                    {
                        wchar_t info[255];
                        swprintf(info, 255, L"N %d %d", col_info.width, col_info.scale);
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

        int xd_encoding = format_definition.encoding;

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
        
        if (!format_definition.header_row)
        {
            // no field names in the first row
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
            if (format_definition.line_delimiter.length() > 0)
                file.setLineDelimiters(format_definition.line_delimiter);
            if (format_definition.delimiter.length() > 0)
                file.setDelimiters(format_definition.delimiter);
            if (format_definition.text_qualifier.length() > 0)
                file.setTextQualifiers(format_definition.text_qualifier);
        }
        
        
        // create vector of field names
        std::vector<std::wstring> fields;
        bool unicode_data_found = false;

        std::vector<xd::ColumnInfo>::const_iterator it;
        for (it = format_definition.columns.cbegin(); it != format_definition.columns.cend(); ++it)
        {
            if (it->type == xd::typeWideCharacter)
                unicode_data_found = true;
            
            if (!it->calculated)
                fields.push_back(it->name);
        }

        
        // determine the encoding we will use in the csv

        int xd_encoding = format_definition.encoding;

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
        

        if (!format_definition.header_row)
        {
            // no field names
            fields.clear();
        }

        file.createFile(phys_path, fields, csv_encoding);
        file.closeFile();
        
/*
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
            if (format_info && !format_info->header_row)
                tset->setFirstRowColumnNames(false);
        }

        tset->saveConfiguration();
        delete tset;

        return true;
*/
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
    
    // make sure containing directory exists
    std::wstring containing_directory = xf_get_file_directory(phys_path);
    if (!xf_get_directory_exist(containing_directory))
        _mkdirTree(containing_directory);

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

    IXdsqlTablePtr set = openTable(path, fi);
    if (set.isNull())
        return xcm::null;
        
    IXdfsSetPtr xdfs_set = set;
    if (xdfs_set.isNull())
        return xcm::null;
    
    return xdfs_set->getRowInserter();
}

xd::Structure FsDatabase::describeTable(const std::wstring& path)
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


    xd::FormatDefinition fi;
    fi.format = xd::formatDefault;

    IXdsqlTablePtr tbl = openTable(path, fi);
    if (tbl.isNull())
        return xd::Structure();

    return tbl->getStructure();
}

bool FsDatabase::modifyStructure(const std::wstring& path, const xd::StructureModify& mod_params, xd::IJob* job)
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

    IXdsqlTablePtr tbl = openTable(path);
    if (tbl.isNull())
        return false;

    return tbl->modifyStructure(mod_params, job);
}


bool FsDatabase::execute(const std::wstring& command,
                         unsigned int flags,
                         xcm::IObjectPtr& result,
                         xd::IJob* job)
{
    m_error.clearError();
    result.clear();

    return doSQL(static_cast<xd::IDatabase*>(this), command, flags, result, m_error, job);
}

bool FsDatabase::groupQuery(xd::GroupQueryParams* info, xd::IJob* job)
{
    return xdcommon::runGroupQuery(static_cast<xd::IDatabase*>(this), info, job);
}



xd::IndexInfo FsDatabase::createIndex(const std::wstring& path,
                                      const std::wstring& name,
                                      const std::wstring& expr,
                                      xd::IJob* job)
{
    // get object id from path
    xd::IFileInfoPtr info = getFileInfo(path);
    if (info.isNull())
        return xd::IndexInfo();

    std::wstring object_id = info->getObjectId();
    std::wstring idx_filename = kl::getUniqueString() + L".idx1";

    std::wstring idx_path = m_ctrl_path;
    idx_path += xf_path_separator_wchar;
    idx_path += L"indexes";
    idx_path += xf_path_separator_wchar;
    idx_path += idx_filename;


    IIndex* idx = createExternalIndex(static_cast<xd::IDatabase*>(this),
                                      path,
                                      idx_path,
                                      getTempFileDirectory(),
                                      expr,
                                      true,
                                      false, // delete_on_close is false because this is a 'permanant index'
                                      job);

    if (!idx)
        return xd::IndexInfo();
    
    idx->unref();

    kl::exclusive_file file(m_ctrl_path + xf_path_separator_wchar + L"indexes" + xf_path_separator_wchar + object_id + L".info");
    if (!file.isOk())
    {
        xf_remove(idx_filename);
        return xd::IndexInfo();
    }

    std::wstring json = file.getContents();

    kl::JsonNode root;
    if (!json.empty() && !root.fromString(json))
    {
        xf_remove(idx_filename);
        return xd::IndexInfo();
    }

    kl::JsonNode indexes = root["indexes"];
    if (!indexes.isArray())
        indexes.setArray();

    // add index entry
    kl::JsonNode index_node = indexes.appendElement();
    index_node["name"] = name;
    index_node["expression"] = expr;
    index_node["filename"] = idx_filename;

    
    file.putContents(root.toString());


    // return info about the new index
    xd::IndexInfo ii;
    ii.name = name;
    ii.expression = expr;
    return ii;
}


bool FsDatabase::renameIndex(const std::wstring& path,
                             const std::wstring& name,
                             const std::wstring& new_name)
{
    // get object id from path
    xd::IFileInfoPtr info = getFileInfo(path);
    if (info.isNull())
        return xcm::null;

    std::wstring object_id = info->getObjectId();

    // update index registry
    kl::exclusive_file file(m_ctrl_path + xf_path_separator_wchar + L"indexes" + xf_path_separator_wchar + object_id + L".info");
    if (!file.isOk())
        return false;

    std::wstring json = file.getContents();

    kl::JsonNode root;
    if (json.empty() || !root.fromString(json))
        return false;

    kl::JsonNode indexes = root["indexes"];
    std::vector<kl::JsonNode> index_nodes = indexes.getChildren();
    std::vector<kl::JsonNode>::iterator it, it_end = index_nodes.end();
    for (it = index_nodes.begin(); it != it_end; ++it)
    {
        if (kl::iequals((*it)["name"], name))
        {
            (*it)["name"] = new_name;
            break;
        }
    }

    file.putContents(root.toString());

    return true;
}


bool FsDatabase::deleteIndex(const std::wstring& path, const std::wstring& name)
{
    // get object id from path
    xd::IFileInfoPtr info = getFileInfo(path);
    if (info.isNull())
        return xcm::null;

    std::wstring object_id = info->getObjectId();

    // update index registry

    kl::exclusive_file file(m_ctrl_path + xf_path_separator_wchar + L"indexes" + xf_path_separator_wchar + object_id + L".info");
    if (!file.isOk())
        return false;

    std::wstring json = file.getContents();

    kl::JsonNode root;
    if (json.empty() || !root.fromString(json))
        return false;

    kl::JsonNode indexes = root["indexes"];
    std::vector<kl::JsonNode> index_nodes = indexes.getChildren();
    std::vector<kl::JsonNode>::iterator it, it_end = index_nodes.end();
    size_t counter = 0;
    for (it = index_nodes.begin(); it != it_end; ++it)
    {
        if (kl::iequals((*it)["name"], name))
        {
            std::wstring idx_path = m_ctrl_path + xf_path_separator_wchar + L"indexes" + xf_path_separator_wchar + (*it)["filename"].getString();
            if (!xf_remove(idx_path))
                return false;

            indexes.deleteChild(counter);
            file.putContents(root.toString());
            return true;
        }
        ++counter;
    }

    return false;
}


xd::IndexInfoEnum FsDatabase::getIndexEnum(const std::wstring& path)
{
    xd::IndexInfoEnum vec;

    // get object id from path
    xd::IFileInfoPtr info = getFileInfo(path);
    if (info.isNull())
        return vec;

    std::wstring object_id = info->getObjectId();

    std::wstring idx_info_filename = m_ctrl_path + xf_path_separator_wchar + L"indexes" + xf_path_separator_wchar + object_id + L".info";

    if (!xf_get_file_exist(idx_info_filename))
        return vec;

    kl::exclusive_file file(idx_info_filename);
    if (!file.isOk())
        return vec;

    std::wstring json = file.getContents();

    kl::JsonNode root;
    if (json.empty() || !root.fromString(json))
        return vec;

    kl::JsonNode indexes = root["indexes"];
    std::vector<kl::JsonNode> index_nodes = indexes.getChildren();

    std::wstring name, expr, filename;

    xd::IndexInfo ii;
    std::vector<kl::JsonNode>::iterator it, it_end = index_nodes.end();
    for (it = index_nodes.begin(); it != it_end; ++it)
    {
        ii.name = (*it)["name"];
        ii.expression = (*it)["expression"];
        vec.push_back(ii);
    }

    return vec;
}





xd::IRelationPtr FsDatabase::createRelation(const std::wstring& tag,
                                            const std::wstring& left_table,
                                            const std::wstring& right_table,
                                            const std::wstring& left_expr,
                                            const std::wstring& right_expr)
{
    kl::exclusive_file file(m_ctrl_path + xf_path_separator_wchar + L"system" + xf_path_separator_wchar + L"rel_table.info");
    if (!file.isOk())
        return xcm::null;

    std::wstring relation_id = kl::getUniqueString();
    std::wstring json = file.getContents();

    kl::JsonNode root;
    if (json.length() > 0 && !root.fromString(json))
        return xcm::null;


    kl::JsonNode relations = root["relations"];
    if (!relations.isArray())
        relations.setArray();

    kl::JsonNode relation_node = relations.appendElement();
    relation_node["id"] = relation_id;
    relation_node["tag"] = tag;
    relation_node["left_table"] = left_table;
    relation_node["right_table"] = right_table;
    relation_node["left_expression"] = left_expr;
    relation_node["right_expression"] = right_expr;

    file.putContents(root.toString());

    // make sure file is re-read on next call to getRelationEnum(), etc
    m_relations_mutex.lock();
    m_relations_filetime = 0;
    m_relations_mutex.unlock();

    return xcm::null;
}

bool FsDatabase::deleteRelation(const std::wstring& relation_id)
{
    // update index registry

    kl::exclusive_file file(m_ctrl_path + xf_path_separator_wchar + L"system" + xf_path_separator_wchar + L"rel_table.info");
    if (!file.isOk())
        return false;

    std::wstring json = file.getContents();

    kl::JsonNode root;
    if (json.length() > 0 && !root.fromString(json))
        return false;


    kl::JsonNode relations = root["relations"];
    std::vector<kl::JsonNode> index_nodes = relations.getChildren();
    std::vector<kl::JsonNode>::iterator it, it_end = index_nodes.end();
    size_t counter = 0;
    for (it = index_nodes.begin(); it != it_end; ++it)
    {
        if ((*it)["id"].getString() == relation_id)
        {
            relations.deleteChild(counter);
            file.putContents(root.toString());
            return true;
        }
        ++counter;
    }


    // make sure file is re-read on next call to getRelationEnum(), etc
    m_relations_mutex.lock();
    m_relations_filetime = 0;
    m_relations_mutex.unlock();

    return false;
}

xd::IRelationPtr FsDatabase::getRelation(const std::wstring& relation_id)
{
    refreshRelationInfo();

    m_relations_mutex.lock();
    std::vector<RelationInfo*>::iterator it, it_end = m_relations.end();
    for (it = m_relations.begin(); it != it_end; ++it)
    {
        if ((*it)->getRelationId() == relation_id)
        {
            xd::IRelationPtr ret = static_cast<xd::IRelation*>(*it);
            m_relations_mutex.unlock();
            return ret;
        }
    }
    m_relations_mutex.unlock();

    return xcm::null;
}

xd::IRelationEnumPtr FsDatabase::getRelationEnum(const std::wstring& path)
{
    refreshRelationInfo();

    xcm::IVectorImpl<xd::IRelationPtr>* vec;
    vec = new xcm::IVectorImpl<xd::IRelationPtr>;

    m_relations_mutex.lock();
    std::vector<RelationInfo*>::iterator it, it_end = m_relations.end();
    for (it = m_relations.begin(); it != it_end; ++it)
    {
        if (path.length() > 0)
        {
            if (!isSamePath((*it)->getLeftTable(), path))
                continue;
        }

        vec->append(static_cast<xd::IRelation*>(*it));
    }
    m_relations_mutex.unlock();

    return vec;
}



void FsDatabase::refreshRelationInfo()
{
    std::wstring rel_table_file = m_ctrl_path + xf_path_separator_wchar + L"system" + xf_path_separator_wchar + L"rel_table.info";

    if (!xf_get_file_exist(rel_table_file))
    {
        // relationship table file doesn't exist, clear out vector
        m_relations_mutex.lock();
        std::vector<RelationInfo*>::iterator rit;
        for (rit = m_relations.begin(); rit != m_relations.end(); ++rit)
            (*rit)->unref();
        m_relations.clear();
        m_relations_mutex.unlock();
    }


    xf_filetime_t ft = xf_get_file_modify_time(rel_table_file);
    {
        KL_AUTO_LOCK(m_relations_mutex);
        if (ft <= m_relations_filetime)
            return; // no update necessary
        m_relations_filetime = ft;
    }


    std::vector<RelationInfo*> vec;

    {
        kl::exclusive_file file(rel_table_file);
        if (!file.isOk())
            return;

        std::wstring json = file.getContents();

        kl::JsonNode root;
        if (json.length() > 0 && !root.fromString(json))
            return;

        kl::JsonNode relations = root["relations"];
        std::vector<kl::JsonNode> index_nodes = relations.getChildren();
        std::vector<kl::JsonNode>::iterator it, it_end = index_nodes.end();
        RelationInfo* r;
        for (it = index_nodes.begin(); it != it_end; ++it)
        {
            r = new RelationInfo;
            r->ref();

            r->setRelationId((*it)["id"]);
            r->setTag((*it)["tag"]);
            r->setLeftTable((*it)["left_table"]);
            r->setRightTable((*it)["right_table"]);
            r->setLeftExpression((*it)["left_expression"]);
            r->setRightExpression((*it)["right_expression"]);

            vec.push_back(r);
        }
    }

    // set m_relations vector to the vector we just read in
    m_relations_mutex.lock();
    std::vector<RelationInfo*>::iterator rit;
    for (rit = m_relations.begin(); rit != m_relations.end(); ++rit)
        (*rit)->unref();
    m_relations = vec;
    m_relations_mutex.unlock();
}




static bool group_parse_hook(kscript::ExprParseHookInfo& hook_info)
{
    xd::Structure* structure = (xd::Structure*)hook_info.hook_param;
    
    if (hook_info.element_type == kscript::ExprParseHookInfo::typeIdentifier)
    {
        if (hook_info.expr_text.length() > 1 &&
            hook_info.expr_text[0] == '[' &&
            hook_info.expr_text[hook_info.expr_text.length()-1] == ']')
        {
            // remove brackets from identifier; i.e.:
            //     [field] => field
            //     table.[field] => table.field
            //     [table].field => table.field
            //     [table].[field] => table.field
            
            // get the two parts separated by the period, if there is one
            std::wstring part1, part2;

            const wchar_t* pstr = hook_info.expr_text.c_str();
            const wchar_t* pperiod = zl_strchr((wchar_t*)pstr, '.', L"[", L"]");
            int period_pos = pperiod ? (pperiod-pstr) : -1;

            if (period_pos == -1)
            {
                part1 = hook_info.expr_text;
                dequote(part1, '[', ']');
                hook_info.expr_text = part1;
            }
            else
            {
                part1 = hook_info.expr_text.substr(0, period_pos);
                part2 = hook_info.expr_text.substr(period_pos + 1);
                dequote(part1, '[', ']');
                dequote(part2, '[', ']');
                hook_info.expr_text = part1 + L"." + part2;
            }
        }


        const xd::ColumnInfo& col = structure->getColumnInfo(hook_info.expr_text);
        if (col.isNull())
            return false;
        
        kscript::Value* v = new kscript::Value;
        v->setGetVal(xd2kscriptType(col.type), NULL, NULL);
        hook_info.res_element = v;
        
        return true;
    }
     else if (hook_info.element_type == kscript::ExprParseHookInfo::typeFunction)
    {
        hook_info.res_element = NULL;
        
        std::wstring str = hook_info.expr_text;
        kl::trim(str);

        int len = str.length();
        if (len == 0)
            return true;

        if (str[len-1] != L')')
            return true;

        str.resize(len-1);
        len--;

        if (len == 0)
            return true;

        std::wstring func_name = kl::beforeFirst(str, L'(');
        std::wstring param = kl::afterFirst(str, L'(');

        kl::trim(func_name);
        kl::makeUpper(func_name);

        if (func_name == L"FIRST" ||
            func_name == L"LAST" ||
            func_name == L"MIN" ||
            func_name == L"MAX")
        {
            xd::ColumnInfo colinfo = structure->getColumnInfo(param);
            if (colinfo.isNull())
            {
                // try the parameter dequoted
                std::wstring dequoted_param = param;
                dequote(dequoted_param, '[', ']');
                colinfo = structure->getColumnInfo(dequoted_param);
            }

            if (colinfo.isNull())
            {
                kscript::Value* v = new kscript::Value;
                if (param.find(L'.') != -1)
                {
                    // let child fields be valid,
                    // even if we can't determine their type
                    // assume character
                    v->setString(L"");
                    hook_info.res_element = v;
                }
                
                return true;
            }

            kscript::Value* v = new kscript::Value;
            
            switch (colinfo.type)
            {
                case xd::typeCharacter:
                case xd::typeWideCharacter:
                    v->setString(L"");
                    break;
                case xd::typeNumeric:
                case xd::typeDouble:
                    v->setDouble(0.0);
                    break;
                case xd::typeInteger:
                    v->setInteger(0);
                    break;
                case xd::typeBoolean:
                    v->setBoolean(true);
                    break;
                case xd::typeDateTime:
                case xd::typeDate:
                    v->setDateTime(0,0);
                    break;
                case xd::typeBinary:
                    v->setType(kscript::Value::typeBinary);
                    break;
                default:
                    return true;
            }

            hook_info.res_element = v;

            return true;
        }
         else if (func_name == L"SUM" ||
                  func_name == L"AVG" ||
                  func_name == L"STDDEV" ||
                  func_name == L"VARIANCE")
        {
            xd::ColumnInfo colinfo = structure->getColumnInfo(param);

            if (colinfo.isNull())
            {
                // try the parameter dequoted
                std::wstring dequoted_param = param;
                dequote(dequoted_param, '[', ']');
                colinfo = structure->getColumnInfo(dequoted_param);
            }

            if (colinfo.isNull())
            {
                if (param.find(L'.') != -1)
                {
                    // let child fields be valid
                    kscript::Value* v = new kscript::Value;
                    v->setDouble(0.0);
                    hook_info.res_element = v;
                }
                
                return true;
            }

            if (colinfo.type != xd::typeNumeric &&
                colinfo.type != xd::typeInteger &&
                colinfo.type != xd::typeDouble)
            {
                return true;
            }

            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"MAXDISTANCE")
        {
            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"MERGE")
        {
            xd::ColumnInfo colinfo  = structure->getColumnInfo(param);

            if (colinfo.isNull())
            {
                // try the parameter dequoted
                std::wstring dequoted_param = param;
                dequote(dequoted_param, '[', ']');
                colinfo = structure->getColumnInfo(dequoted_param);
            }

            if (colinfo.isNull())
            {
                if (param.find(L'.') != -1)
                {
                    // let child fields be valid
                    kscript::Value* v = new kscript::Value;
                    v->setString(L"");
                    hook_info.res_element = v;
                }
                
                return true;
            }
            
            if (colinfo.type != xd::typeCharacter &&
                colinfo.type != xd::typeWideCharacter)
            {
                return true;
            }

            kscript::Value* v = new kscript::Value;
            v->setString(L"");
            hook_info.res_element = v;
            return true;
        }
         else if (func_name == L"COUNT" ||
                  func_name == L"GROUPID")
        {
            kscript::Value* v = new kscript::Value;
            v->setDouble(0.0);
            hook_info.res_element = v;
            return true;
        }
    }

    // not handled
    return false;
}





xd::ColumnInfo FsDatabase::validateExpression(const std::wstring& expression, const xd::Structure& structure, const std::wstring& path_context)
{
    // if the expression is a column name, simply look it up via getColumnInfo
    std::wstring dequoted_expression = expression;
    dequote(dequoted_expression, '[', ']');
    const xd::ColumnInfo& colinfo = structure.getColumnInfo(dequoted_expression);
    if (colinfo.isOk())
        return colinfo;


    kscript::ExprParser* parser = createExprParser();
    
    // we added this here so that the expression
    // parser in structuredoc would recognize this as a valid function;
    // these are from xdnative/baseiterator.cpp
    parser->addFunction(L"recno", false, NULL, false, L"f()", this);
    parser->addFunction(L"rownum", false, NULL, false, L"f()", this);
    parser->addFunction(L"recid", false, NULL, false, L"s()", this);
    parser->addFunction(L"reccrc", false, NULL, false, L"x()", this);
    
    
    // create field bindings and add them to the expression parser

    parser->setParseHook(kscript::ExprParseHookInfo::typeFunction |
                         kscript::ExprParseHookInfo::typeIdentifier,
                         group_parse_hook,
                         (void*)&structure);

    if (!parser->parse(expression))
    {
        delete parser;
        return xd::ColumnInfo();
    }

    int type = kscript2xdType(parser->getType());
    delete parser;
    if (type == xd::typeInvalid)
        return xd::ColumnInfo();

    xd::ColumnInfo ret;
    ret.name = L"EXPR001";
    ret.type = type;
    return ret;
}

