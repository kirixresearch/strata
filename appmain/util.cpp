/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2004-04-03
 *
 */


#include "appmain.h"
#include "util.h"
#include <wx/tokenzr.h>
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <xcm/xcm.h>
#include <kl/url.h>
#include <kl/utf8.h>
#include <kl/regex.h>
#include <kl/thread.h>
#include <kl/math.h>

#include "curlutil.h"
#include <curl/curl.h>

#ifdef WIN32
#include "wx/fontutil.h"
#endif

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif


// needed by getUserDocumentFolder on win32 systems
#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#endif

wxString doubleQuote(const wxString& src, wxChar quote)
{
    wxString ret;
    ret.Alloc(src.Length() + 10);
    const wxChar* ch;

    ch = src.c_str();
    while (*ch)
    {
        if (*ch != quote)
            ret += *ch;
              else
            {
                ret += quote;
                ret += quote;
            }

        ++ch;
    }

    return ret;
}

wxString makeProper(const wxString& input)
{
    if (input.Length() == 0)
        return input;
    
    wxString output = input;
    const wxChar* ch = output.c_str();
    int idx = 0;

    while (*ch && !wxIsalpha(*ch))
    {
        ch++;
        idx++;
    }

    if (!*ch)
    {
        return output;
    }
    
    // make sure that all characters are 7-bit
    // before trying to uppercase the first letter,
    // which can destroy some character data in
    // certain unicode cases
    
    ch = output.c_str();
    while (*ch)
    {
        ch++;
        if (*ch > 127)
            return output;
    }

    output.MakeLower();
    output.SetChar(idx, wxToupper(output.GetChar(idx)));

    return output;
}

wxString makeProperIfNecessary(const wxString& input)
{
    if (input.Length() == 0)
        return input;
        
    const wxChar* ch = input.c_str();
    
    while (*ch)
    {
        if (*ch > 127)
        {
            // unicode strings should not be made 'proper'
            return input;
        }
            
        if (wxIsalpha(*ch) && *ch != towupper(*ch))
        {
            // input is mixed-case, return original
            return input;
        }
        
        ++ch;
    }
    
    // if the string is all upper case, lower-case it
    // and capitalize the first letter
    return makeProper(input);
}

wxString filenameToUrl(const wxString& _filename)
{
    return kl::filenameToUrl(towstr(_filename));
}

wxString urlToFilename(const wxString& _url)
{
    return kl::urlToFilename(towstr(_url));
}

void trimUnwantedUrlChars(wxString& str)
{
    // trim windows filename invalid characters
    str.Replace(wxT("*"), wxT(" "));
    str.Replace(wxT("?"), wxT(" "));
    str.Replace(wxT("\\"), wxT(" "));
    str.Replace(wxT("/"), wxT(" "));
    str.Replace(wxT(":"), wxT(" "));
    str.Replace(wxT("\""), wxT(" "));
    str.Replace(wxT("<"), wxT(" "));
    str.Replace(wxT(">"), wxT(" "));
    str.Replace(wxT("|"), wxT(" "));

    // trim other unwanted cruft
    str.Replace(wxT("\r"), wxT(" "));
    str.Replace(wxT("\n"), wxT(" "));
    str.Replace(wxT(","), wxT(" "));
    str.Replace(wxT("."), wxT(" "));
    str.Replace(wxT("("), wxT(" "));
    str.Replace(wxT(")"), wxT(" "));
    str.Replace(wxT("'"), wxT(" "));
    str.Replace(wxT(":"), wxT(" "));
    str.Replace(wxT("-"), wxT(" "));
    str.Replace(wxT("+"), wxT(" "));
    str.Replace(wxT("/"), wxT(" "));
    str.Replace(wxT("*"), wxT(" "));
    str.Replace(wxT("%"), wxT(" "));
    str.Replace(wxT("["), wxT(" "));
    str.Replace(wxT("]"), wxT(" "));
    str.Replace(wxT("&"), wxT(" "));
    str.Replace(wxT("@"), wxT(" "));
    
    // get rid of any double (or more) spaces
    str.Replace(wxT("  "), wxT(" "));
    str.Replace(wxT("  "), wxT(" "));
    str.Replace(wxT("  "), wxT(" "));
    str.Replace(wxT("  "), wxT(" "));
    str.Replace(wxT("  "), wxT(" "));
    str.Replace(wxT("  "), wxT(" "));
    str.Replace(wxT("  "), wxT(" "));
    str.Replace(wxT("  "), wxT(" "));
    str.Replace(wxT("  "), wxT(" "));
    
    str.Trim(false);
    str.Trim(true);
}

wxString jsEscapeString(const wxString& input, wxChar quote)
{
    wxChar repstr1[2], repstr2[3];
    repstr1[0] = quote;
    repstr1[1] = 0;
    repstr2[0] = '\\';
    repstr2[1] = quote;
    repstr2[2] = 0;
    
    wxString result = input;
    result.Replace(wxT("\\"), wxT("\\\\"));
    result.Replace(repstr1, repstr2);
    
    return result;
}

wxString urlEscape(const wxString& input)
{
    wxString result;
    result.Alloc(input.Length() + 10);
    
    const wxChar* ch = input.c_str();
    unsigned int c;
    
    wxString u = wxT(" ");

    while ((c = *ch))
    {
        if (c >= 128)
        {
            // we need to utf-8 encode this character per RFC-3986
            u.SetChar(0, *ch);
            const wxCharBuffer utf8b = u.utf8_str();
            const char* utf8 = (const char*)utf8b;
            while (*utf8)
            {
                result += wxString::Format(wxT("%%%02X"), (unsigned char)*utf8);
                ++utf8;
            }
            
            ch++;
            continue;
        }
        
        if (c <= 0x1f ||
            c == '%' || c == ' ' || c == '&' || c == '=' ||
            c == '+' || c == '$' || c == '#' || c == '{' ||
            c == '}' || c == '\\' ||c == '|' || c == '^' ||
            c == '~' || c == '[' || c == ']' || c == '`' ||
            c == '<' || c == '>')
        {
            result += wxString::Format(wxT("%%%02X"), c);
        }
         else
        {
            result += *ch;
        }
        
        ++ch;
    }

    return result;
}



wxString removeChar(const wxString& s, wxChar c)
{
    wxString result;

    const wxChar* ch = s.c_str();
    while (*ch)
    {
        if (*ch != c)
            result += *ch;
        ch++;
    }
    return result;
}

wxChar* zl_strchr(wxChar* str, wxChar ch)
{
    int paren_level = 0;
    wxChar quote_char = 0;

    while (*str)
    {
        if (*str == quote_char)
        {
            quote_char = 0;
            str++;
            continue;
        }

        if (*str == wxT('\'') && !quote_char)
        {
            quote_char = wxT('\'');
        }
        
        if (*str == wxT('"') && !quote_char)
        {
            quote_char = wxT('\"');
        }

        if (!quote_char)
        {
            if (*str == wxT('('))
                paren_level++;

            if (*str == wxT(')'))
                paren_level--;

            if (paren_level == 0 && *str == ch)
                return str;
        }

        str++;
    }

    return NULL;
}

wxString makeUniqueString()
{
    unsigned int part1 = ((unsigned int)time(NULL)) & 0xffffffff;
    unsigned int part2 = rand();
    static unsigned int part3 = rand();
    
    part3++;
    
    wxString result;
    result.Printf(wxT("u%08x%04x%04x"), part1, (part2 & 0xffff), (part3 & 0xffff));
    return result;
}

void int2buf(unsigned char* buf, unsigned int i)
{
    *(buf)   = (i & 0x000000ff);
    *(buf+1) = (i & 0x0000ff00) >> 8;
    *(buf+2) = (i & 0x00ff0000) >> 16;
    *(buf+3) = (i & 0xff000000) >> 24;
}

int buf2int(const unsigned char* buf)
{
    return buf[0] + (buf[1]*256) + (buf[2]*65536) + (buf[3] * 16777216);
}


std::vector<wxString> stringToVector(const wxString& string)
{
    // note: converts a string containing a comma-delimited 
    // list in a vector of individual strings

    std::vector<wxString> result;

    // parse the group string and fill out the group fields
    // for the section
    wxStringTokenizer tokenizer(string, wxT(","));
    while (tokenizer.HasMoreTokens())
    {
        // get the property name
        wxString field_token = tokenizer.GetNextToken();
        field_token.Trim(true);
        field_token.Trim(false);
        result.push_back(field_token);
    }
    
    return result;
}

std::vector< std::pair<std::wstring, bool> > sortExprToVector(const std::wstring& sort_expr)
{
    // note: this function takes an input sort expression and parses out the
    // fieldnames that make up the sort expression into a vector of pairs,
    // where the first part of the pair is the name of the field and the 
    // second part of the field is a boolean value that indicates the sort
    // order: false = sort ascending and true = sort descending

    // used throughout
    std::vector< std::pair<std::wstring, bool> > sort_fields;
    std::vector<std::wstring> expr_parts;

    // get the fields from the sort expression
    kl::parseDelimitedList(sort_expr, expr_parts, ',', true);

    std::vector<std::wstring>::iterator it, it_end;
    it_end = expr_parts.end();

    for (it = expr_parts.begin(); it != it_end; ++it)
    {
        // get the string
        std::wstring field = *it;
        kl::trim(field);

        std::wstring upper_field = field;
        kl::makeUpper(upper_field);
        
        // get the field name, which is separated from the ASC/DESC
        // sort order qualifiers by a space
        bool sort_desc = false;
 
        // see if the ascending flag is included
        int offset = upper_field.find(L" ASC");
        if (offset != upper_field.npos)
        {
            // if the ascending flag is included,
            // strip it off and set the sort flag
            field = field.substr(0, offset);
            sort_desc = false;
        }
        else
        {
            // if the ascending flag isn't included,
            // check for the descending flag
            offset = upper_field.find(L" DESC");
            if (offset != upper_field.npos)
            {
                // if the descending flag is included,
                // strip it off and set the sort flag
                field = field.substr(0, offset);
                sort_desc = true;
            }
        }

        // trim any remaining spaces
        kl::trim(field);

        // set the fieldname and sort order, and
        // save the pair
        std::pair<std::wstring, bool> pair;
        pair.first = field;
        pair.second = sort_desc;
    
        sort_fields.push_back(pair);
    }
    
    // return the pairs of fields and their 
    // sort orders
    return sort_fields;
}

// helper function
static bool isValid(const wxString& str,
                    const wxString& invalid_starting_chars,
                    const wxString& invalid_chars,
                    int* err_idx)
{
    if (str.IsEmpty())
    {
        if (err_idx)
        {
            *err_idx = 0;
        }
        return false;
    }

    // check to see if the first character is valid
    const wxChar* p = str.c_str();

    if (invalid_starting_chars.Find(*p) != -1)
    {
        if (err_idx)
        {
            *err_idx = 0;
        }
        return false;
    }

    ++p;
    int idx = 1;
    while (*p)
    {
        if (invalid_chars.Find(*p) != -1)
        {
            if (err_idx)
            {
                *err_idx = idx;
            }
            return false;
        }

        ++p;
        ++idx;
    }

    return true;
}

bool isKeyword(const wxString& str,
               xd::IDatabasePtr db)
{
    if (db.isNull())
        db = g_app->getDatabase();

    xd::IAttributesPtr attr = db->getAttributes();
    if (!attr)
    {
        // couldn't get attributes from database,
        // assume the worst
        return true;
    }

    std::vector<wxString> vec;

    wxStringTokenizer t(attr->getStringAttribute(xd::dbattrKeywords), ",");
    while (t.HasMoreTokens())
    {
        wxString s = t.GetNextToken();
        s.Trim();
        s.Trim(FALSE);
        vec.push_back(s);
    }

    std::vector<wxString>::iterator it;

    for (it = vec.begin(); it != vec.end(); ++it)
    {
        if (str.CmpNoCase(*it) == 0)
            return true;
    }

    return false;
}

bool isValidFieldName(const wxString& str,
                      xd::IDatabasePtr db,
                      int* err_idx)
{
    // if the string is empty, it's invalid
    if (str.Length() == 0)
        return false;

    if (db.isNull())
        db = g_app->getDatabase();

    xd::IAttributesPtr attr = db->getAttributes();
    if (!attr)
        return false;

    if (isKeyword(str, db))
    {
        if (err_idx)
        {
            *err_idx = 0;
        }

        return false;
    }

    wxString invalid_starting = attr->getStringAttribute(xd::dbattrColumnInvalidStartingChars);
    wxString invalid = attr->getStringAttribute(xd::dbattrColumnInvalidChars);

    // test the content
    if (!isValid(str, invalid_starting, invalid, err_idx))
    {
        return false;
    }

    return true;
}

bool isValidObjectName(const wxString& str,
                       xd::IDatabasePtr db,
                       int* err_idx)
{
    // if the string is empty, it's invalid
    if (str.Length() == 0)
        return false;

    if (db.isNull())
        db = g_app->getDatabase();

    xd::IAttributesPtr attr = db->getAttributes();
    if (!attr)
        return false;

    if (isKeyword(str, db))
        return false;

    wxString invalid_starting = attr->getStringAttribute(xd::dbattrTableInvalidStartingChars);
    wxString invalid = attr->getStringAttribute(xd::dbattrTableInvalidChars);

    // test the content
    if (!isValid(str, invalid_starting, invalid, err_idx))
        return false;


    // test the length
    if (str.Length() > (size_t)attr->getIntAttribute(xd::dbattrTableMaxNameLength))
    {
        if (err_idx)
        {
            *err_idx = str.Length()-1;
        }
        return false;
    }

    return true;
}

bool isValidObjectPath(const wxString& str,
                       xd::IDatabasePtr db,
                       int* err_idx)
{
    // if the string is empty, it's invalid
    if (str.Length() == 0)
        return false;

    if (db.isNull())
        db = g_app->getDatabase();

    wxStringTokenizer tkz(str, wxT("/"));
    wxString stub = wxT("/");
    while (tkz.HasMoreTokens())
    {
        wxString token = tkz.GetNextToken();

        if (token.IsEmpty())
            continue;

        if (!isValidObjectName(token, db, err_idx))
        {
            return false;
        }

        if (!tkz.HasMoreTokens())
            break;
    }

    return true;
}

bool isValidTable(
               const std::wstring& str,
               xd::IDatabasePtr db)
{
    // if the string is empty, it's invalid
    if (str.length() == 0)
        return false;

    if (db.isNull())
        db = g_app->getDatabase();

    if (db.isNull())
        return false;

    xd::IFileInfoPtr finfo = db->getFileInfo(str);
    if (finfo.isOk() && finfo->getType() == xd::filetypeTable)
        return true;
         else
        return false;
}


wxString makeValidFieldName(const wxString& str,
                            xd::IDatabasePtr db)
{
    if (db.isNull())
        db = g_app->getDatabase();

    wxString work_str = str;
    unsigned int i = 0;

    while (1)
    {
        int err_idx = 0;
        if (isValidFieldName(work_str, db, &err_idx))
        {
            return work_str;
        }
    
        if (err_idx == 0)
        {
            if (work_str.Length() > 0 && ::iswalpha(work_str[0]))
            {
                // probably a keyword
                work_str = work_str + wxT("_");
            }
             else
            {
                // probably a number at the beginning
                work_str = wxT("_") + work_str;
            }
        }
         else
        {
            work_str.SetChar(err_idx, wxT('_'));
        }

        // avoid infinite loop
        if (++i > str.Length()*2)
            return work_str;
    }

    return work_str;
}

wxString makeValidObjectName(const wxString& str,
                             xd::IDatabasePtr db)
{
    if (db.isNull())
        db = g_app->getDatabase();

    wxString work_str = str;

    unsigned int i = 0;

    while (1)
    {
        int err_idx = 0;
        if (isValidObjectName(work_str, db, &err_idx))
        {
            return work_str;
        }
    
        if (err_idx == 0)
        {
            if (work_str.Length() > 0 && ::iswalpha(work_str[0]))
            {
                // probably a keyword
                work_str = work_str + wxT("_");
            }
             else
            {
                // probably a number at the beginning
                work_str = wxT("_") + work_str;
            }
        }
         else
        {
            work_str.SetChar(err_idx, wxT('_'));
        }

        // avoid infinite loop
        if (++i > str.Length()*2)
            return work_str;
    }

    return work_str;
}

wxString getDbColumnTypeText(int xd_type)
{
    switch (xd_type)
    {
        default:
        case xd::typeUndefined:      return _("Undefined");
        case xd::typeInvalid:        return _("Invalid");
        case xd::typeCharacter:      return _("Character");
        case xd::typeWideCharacter:  return _("Wide Character");
        case xd::typeBinary:         return _("Binary");
        case xd::typeNumeric:        return _("Numeric");
        case xd::typeDouble:         return _("Double");
        case xd::typeInteger:        return _("Integer");
        case xd::typeDate:           return _("Date");
        case xd::typeDateTime:       return _("DateTime");
        case xd::typeBoolean:        return _("Boolean");
    }

    return wxT("");
}

bool getRemotePathIfExists(wxString& path)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    xd::IFileInfoPtr info = db->getFileInfo(towstr(path));
    if (info.isNull())
        return false;
    
    if (!info->isMount())
        return false;
    
    std::wstring cstr, rpath;
    db->getMountPoint(towstr(path), cstr, rpath);
    
    // convert the path to the remote path of the connection
    if (cstr.length() == 0)
    {
        path = rpath;
        return true;
    }
    
    return false;
}

bool isSamePath(const std::wstring& path1, const std::wstring& path2)
{
    std::wstring s1 = path1, s2 = path2;
    kl::makeLower(s1);
    kl::makeLower(s2);
    if (s1.length() > 0 && s1[0] == '/')
        s1.erase(0,1);
    if (s2.length() > 0 && s2[0] == '/')
        s2.erase(0,1);

    return (s1 == s2) ? true : false;
}


wxString getObjectPathFromMountPath(const wxString& database_path)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return database_path;
    
    wxString path = database_path;
    wxString conn_str;
    wxString to_append;

    if (path.Length() <= 1)
        return wxEmptyString;
        
    std::wstring cstr, rpath;
    if (db->getMountPoint(towstr(path), cstr, rpath))
        return getObjectPathFromMountPath(rpath);
    
    return path;
}

wxString getPhysPathFromMountPath(const wxString& database_path)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return wxEmptyString;
    
    wxString path = database_path;
    wxString conn_str;
    wxString to_append;
    
    while (1)
    {
        if (path.Length() <= 1)
            return wxEmptyString;
            
        std::wstring cstr, rpath;
        if (db->getMountPoint(towstr(path), cstr, rpath))
        {
            // if the connection string is empty, we're referring to another
            // mount path -- the physical path may be a few layers deep
            if (cstr.empty())
            {
                std::wstring res = towstr(getPhysPathFromMountPath(rpath));
                res += to_append;
                return res;
            }
            
            conn_str = cstr;
            break;
        }
    
        int old_len = path.Length();
        to_append.Prepend(path.AfterLast(wxT('/')));
        to_append.Prepend(PATH_SEPARATOR_CHAR);
        path = path.BeforeLast(wxT('/'));
        if (path.Length() == old_len)
            return wxEmptyString;
    }
    
    // find 'Database=' portion of the connection str
    wxString temps = conn_str;
    temps.MakeUpper();
    if (temps.Find("XDPROVIDER=XDFS") == -1)
        return wxEmptyString;
    int loc = temps.Find(wxT("DATABASE="));
    if (loc == wxNOT_FOUND)
        return wxEmptyString;
    loc += 9;
    temps = conn_str.Mid(loc);
    
    wxString res = temps.BeforeFirst(wxT(';'));
    if (res.Length() > 0 && res.Last() == PATH_SEPARATOR_CHAR)
        res.RemoveLast();
    res += to_append;
    return res;
}

std::wstring getMountRoot(xd::IDatabasePtr db, const std::wstring _path)
{
    std::wstring path = _path;
    size_t old_len = 0;

    while (1)
    {
        if (path.length() <= 1)
            return L"";
            
        std::wstring cstr, rpath;
        if (db->getMountPoint(path, cstr, rpath))
            return path;

        old_len = path.length();
        path = kl::beforeLast(path, '/');
        if (path.length() == old_len)
            return L"";
    }
}

bool getMountPointHelper(xd::IDatabasePtr& db, const std::wstring& _path, std::wstring& cstr, std::wstring& rpath)
{
    std::vector<std::wstring> parts;

    std::wstring path = _path;
    kl::trim(path);
    
    if (path.length() == 0 || path[0] != L'/')
        path = L"/" + path;
    if (path.length() > 0 && path[path.length()-1] == L'/')
        path = path.substr(0, path.length()-1);
    
    while (path.length() > 0)
    {
        std::wstring wcstr, wrpath;
        if (db->getMountPoint(path, wcstr, wrpath))
        {
            rpath = L"";
            cstr = wcstr;
            
            std::vector<std::wstring>::iterator it;
            for (it = parts.begin(); it != parts.end(); ++it)
            {
                rpath += L"/";
                rpath += *it;
            }
            
            if (wrpath.length() > 0 && wrpath != L"/")
                rpath = wrpath + rpath;
            return true;
        }


        if (kl::stringFrequency(path, '/') <= 1)
            return false;
        parts.push_back(kl::afterLast(path, '/'));
        path = kl::beforeLast(path, '/');
    }
    
    return false;
}

bool isTemporaryTable(const std::wstring& path)
{
    if (path.find(L"xtmp_") != path.npos ||
        path.find(L".temp") != path.npos)
    {
        return true;
    }

    return false;
}

// gets the filename from the path
wxString getFilenameFromPath(const wxString& path, bool include_extension)
{
    // note: return the filename portion of a full path;
    // if include_extension is true, returns the extension
    // as part of the filename, otherwise returns the filename
    // without the extension

    // path can be either in the database or in
    // the filesystem, so we have to check for two
    // types of path separators
    wxString filename;
    
    // get the portion of the path after a filesystem
    // path separator
    filename = path.AfterLast(PATH_SEPARATOR_CHAR);
    
    // get the portion of the path after a database
    // path separator
    filename = filename.AfterLast(wxT('/'));

    // if we're including the extension, we're done
    if (include_extension)
        return filename;

    // strip the extension; if the extension isn't included,
    // return the whole filename
    wxString filename_base = filename.BeforeLast(wxT('.'));
    if (filename_base.Length() == 0)
        return filename;

    // base filename, without extension
    return filename_base;
}

wxString getDirectoryFromPath(const wxString& path)
{
    // note: return the directory portion of a full
    // path; returns empty if no directory portion is
    // specified in the path

    // path can be either in the database or in the 
    // filesystem, so we have to check for two types of 
    // path separators
    wxString directory;

    // get the portion of the path before a filesystem
    // path separator
    directory = path.BeforeLast(PATH_SEPARATOR_CHAR);

    // get the portion of the path before a database
    // path separator
    directory = directory.BeforeLast(wxT('/'));

    // return the diretory; if no directory is specified, 
    // returns empty
    return directory;
}

// returns filename extension
std::wstring getExtensionFromPath(const std::wstring& path)
{
    if (path.find('.') == path.npos)
        return L"";

    return kl::afterLast(path, '.');
}

std::wstring getMimeTypeFromExtension(const std::wstring& path)
{
    std::wstring ext = getExtensionFromPath(path);
    if (ext.length() == 0)
        ext = path;
    kl::makeLower(ext);
    
         if (ext == L"bmp")                        return L"image/x-ms-bmp";
    else if (ext == L"css")                        return L"text/css";
    else if (ext == L"icsv")                       return L"application/vnd.interchange-csv";
    else if (ext == L"gif")                        return L"image/gif";
    else if (ext == L"gz")                         return L"application/x-gzip";
    else if (ext == L"htm" || ext == L"html")      return L"text/html";
    else if (ext == L"hta")                        return L"application/hta";
    else if (ext == L"jpg" || ext == L"jpeg")      return L"image/jpeg";
    else if (ext == L"js")                         return L"application/javascript";
    else if (ext == L"json")                       return L"application/json";
    else if (ext == L"pdf")                        return L"application/pdf";
    else if (ext == L"png")                        return L"image/png";
    else if (ext == L"svg")                        return L"image/svg+xml";
    else if (ext == L"tif" || ext == L"tiff")      return L"image/tiff";
    else if (ext == L"txt")                        return L"text/plain";
    else if (ext == L"xls")                        return L"application/vnd.ms-excel";
    else if (ext == L"xml")                        return L"application/xml";
    else if (ext == L"zip")                        return L"application/zip";
    else                                           return L"application/octet-stream";
}

std::wstring determineMimeType(const std::wstring& path)
{
    xf_file_t f = xf_open(path, xfOpen, xfRead, xfShareRead);
    if (!f)
        return wxT("");

    unsigned char* buf = new unsigned char[520];
    int buf_len = xf_read(f, buf, 1, 512);
    if (buf_len < 16)
        return getMimeTypeFromExtension(path);
    buf[buf_len] = 0;

    
    std::wstring result_text;
    

    if (buf_len >= 2 && buf[0] == 0xff && buf[1] == 0xfe)
    {
        kl::ucsle2wstring(result_text, buf+2, (buf_len-2)/2);
    }
     else if (buf_len >= 3 && buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
    {
        // utf-8
        wchar_t* tempbuf = new wchar_t[buf_len+1];
        kl::utf8_utf8tow(tempbuf, buf_len+1, (char*)buf+3, buf_len-3);
        result_text = tempbuf;
        delete[] tempbuf;
    }
     else
    {
        result_text = towstr((const char*)buf);
    }


    static const klregex::wregex regex(L"\"(?<res>application[\\\\]?/vnd[.]kx[.].*?)\"");

    klregex::wmatch matchres;
    if (!regex.search(result_text, matchres))
        return getMimeTypeFromExtension(path);

    const klregex::wsubmatch& res_match = matchres[L"res"];
    
    // remove any backslash characters
    std::wstring res = res_match.str();
    kl::replaceStr(res, L"\\", L"");
    
    return res;
}

std::wstring addExtensionIfExternalFsDatabase(const std::wstring& _path, const std::wstring& ext)
{
    std::wstring path = _path;
    std::wstring extension;
    xd::IDatabasePtr mount_db = g_app->getDatabase()->getMountDatabase(path);
    if (!mount_db.isNull() && mount_db->getDatabaseType() == xd::dbtypeFilesystem)
    {
        extension = getExtensionFromPath(path);
        if (extension.length() == 0)
            path += ext;
    }
    
    return path;
}

bool readStreamTextFile(xd::IDatabasePtr db,
                        const std::wstring& path,
                        std::wstring& result_text)
{
    if (db.isNull())
        return false;
        
    xd::IStreamPtr stream = db->openStream(path);
    if (!stream)
        return false;
    
    result_text = L"";
    
    // load stream data
    wxMemoryBuffer buf;
    
    char* tempbuf = new char[1025];
    unsigned long read = 0;
    while (1)
    {
        if (!stream->read(tempbuf, 1024, &read))
            break;
        
        buf.AppendData(tempbuf, read);
        
        if (read != 1024)
            break;
    }
    delete[] tempbuf;
    
    
    unsigned char* ptr = (unsigned char*)buf.GetData();
    size_t buf_len = buf.GetDataLen();
    if (buf_len >= 2 && ptr[0] == 0xff && ptr[1] == 0xfe)
    {
        kl::ucsle2wstring(result_text, ptr+2, (buf_len-2)/2);
    }
     else if (buf_len >= 3 && ptr[0] == 0xef && ptr[1] == 0xbb && ptr[2] == 0xbf)
    {
        // utf-8
        wchar_t* tempbuf = new wchar_t[buf_len+1];
        kl::utf8_utf8tow(tempbuf, buf_len+1, (char*)ptr+3, buf_len-3);
        result_text = tempbuf;
        delete[] tempbuf;
    }
     else
    {
        buf.AppendByte(0);
        result_text = towstr((char*)buf.GetData());
    }

    return true;
}

bool writeStreamTextFile(xd::IDatabasePtr db,
                         const std::wstring& path,
                         const std::wstring& val,
                         const std::wstring& mime_type)
{
    if (db.isNull())
        return false;
        
    bool uses_unicode = kl::is_unicode_string(val);
    

    size_t buf_len = (val.size() * 4) + 3;
    unsigned char* buf = new unsigned char[buf_len];
    
    if (uses_unicode)
    {
        // convert to utf8
        kl::utf8_wtoutf8((char*)buf+3, buf_len-3, val.c_str(), val.length(), &buf_len);
        
        if (buf_len > 0 && (buf+3)[buf_len-1] == 0)
            buf_len--;
            
        // add space for the byte order mark
        buf_len += 3;
        
        // add byte order mark
        buf[0] = 0xef;
        buf[1] = 0xbb;
        buf[2] = 0xbf;
    }
    /* else ( if we want to save in ucs2 le )
    {
        // little endian byte order mark
        buf[0] = 0xff;
        buf[1] = 0xfe;
        kl::wstring2ucsle(buf+2, val, val.length());
        buf_len = (val.length() * 2) + 2;
    }*/
     else
    {
        // just save as 7-bit ascii because we don't use
        // any characters > char code 127
        std::string s = kl::tostring(val);
        buf_len = val.length();
        memcpy(buf, s.c_str(), buf_len);
    }



    if (!db->createStream(path, mime_type))
    {
        delete[] buf;
        return false;
    }


    xd::IStreamPtr stream = db->openStream(path);
    if (stream.isNull())
    {
        delete[] buf;
        return false;
    }

    stream->write(buf, buf_len, NULL);

    delete[] buf;
    return true;
}

// helper function for createFolderStructure()
static void deleteDirs(std::vector<wxString> dirs)
{
    std::vector<wxString>::reverse_iterator it;
    for (it = dirs.rbegin(); it != dirs.rend(); ++it)
    {
        std::wstring dir = towstr(*it);
        xf_rmdir(dir);
    }
}

bool tryCreateFolderStructure(const wxString& folder_path)
{
    return createFolderStructure(folder_path, true);
}

bool createFolderStructure(const wxString& folder_path,
                           bool delete_on_success)
{
    wxString dirpath = folder_path.BeforeFirst(PATH_SEPARATOR_CHAR);
    wxString remainder = folder_path.AfterFirst(PATH_SEPARATOR_CHAR);

    std::vector<wxString> dirs_created; // in case we have to remove the dirs

    while (1)
    {
        if (dirpath.Length() > 0 && !xf_get_directory_exist(towstr(dirpath)))
        {
            // CHECK: can we create the folder?
            if (!xf_mkdir(towstr(dirpath)))
            {
                // we have encountered an error, delete all
                // of the directories that we created
                deleteDirs(dirs_created);
                return false;
            }

            dirs_created.push_back(dirpath);
        }

        // there are no more path separators, we're done
        if (remainder.IsEmpty())
            break;
        
        dirpath += PATH_SEPARATOR_CHAR;
        dirpath += remainder.BeforeFirst(PATH_SEPARATOR_CHAR);
        remainder = remainder.AfterFirst(PATH_SEPARATOR_CHAR);
    }
    
    // if this flag is set, we were just trying to create the folders
    // and didn't actually want to create them
    if (delete_on_success)
        deleteDirs(dirs_created);
    
    return true;
}

double getProjectSize(const wxString& project_path)
{
    double total_size = 2000000.0;

    wxString temp = project_path;
    if (temp.Last() == PATH_SEPARATOR_CHAR)
        temp.RemoveLast();
    temp += PATH_SEPARATOR_CHAR;
    temp += wxT("data");

    std::wstring base_path = towstr(temp);
    xf_dirhandle_t h = xf_opendir(base_path);
    xf_direntry_t info;
    
    std::wstring path_sep = PATH_SEPARATOR_STR;
    
    while (xf_readdir(h, &info))
    {
        if (info.m_type == xfFileTypeNormal)
        {
            std::wstring path = base_path;
            path += path_sep;
            path += info.m_name;
            total_size += xf_get_file_size(path);
        }
    }
    xf_closedir(h);

    return total_size;
}

wxString getDefaultProjectsPath()
{
    // choose a path in My Documents (or the home directory on linux)
    wxString project_path = wxStandardPaths::Get().GetDocumentsDir();
    if (project_path.Last() != PATH_SEPARATOR_CHAR)
        project_path += PATH_SEPARATOR_CHAR;
    project_path += APPLICATION_NAME;
    project_path += wxT(" ");
    project_path += wxT("Projects");
    return project_path;
}

xd::IIndexInfoPtr lookupIndex(xd::IIndexInfoEnumPtr idx_enum, const std::wstring& expr, bool exact_column_order)
{
    if (idx_enum.isNull())
        return xcm::null;

    std::vector<std::wstring> expr_cols;
    size_t i, idx_count = idx_enum->size();;
    xd::IIndexInfoPtr result;

    kl::parseDelimitedList(expr, expr_cols, L',', true);
    
    for (i = 0; i < idx_count; ++i)
    {
        std::vector<std::wstring> idx_cols;
        xd::IIndexInfoPtr idx = idx_enum->getItem(i);

        kl::parseDelimitedList(idx->getExpression(), idx_cols, L',', true);

        if (idx_cols.size() != expr_cols.size())
            continue;

        if (exact_column_order)
        {
            int col_count = idx_cols.size();
            int j;
            bool match = true;

            for (j = 0; j < col_count; ++j)
            {
                if (0 != wcscasecmp(idx_cols[j].c_str(),
                                    expr_cols[j].c_str()))
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                return idx;
            }
        }
         else
        {
            int col_count = idx_cols.size();
            int j, k;
            bool match = true;

            for (j = 0; j < col_count; ++j)
            {
                // -- try to find it in the idx columns --
                
                bool found = false;

                for (k = 0; k < col_count; ++k)
                {
                    if (0 == wcscasecmp(idx_cols[j].c_str(),
                                        expr_cols[k].c_str()))
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    match = false;
                }

            }

            if (match)
            {
                return idx;
            }

        }
    }

    return result;
}

kl::JsonNode createSortFilterJobParams(const std::wstring& path,
                                       const std::wstring& filter,
                                       const std::wstring& order)
{
    kl::JsonNode params;
    params["input"].setString(path);
    params["where"].setString(filter);
    params["order"].setArray();

    if (order.length() > 0)
    {
        std::vector< std::pair<std::wstring, bool> > sort_fields;
        sort_fields = sortExprToVector(order);

        std::vector< std::pair<std::wstring, bool> >::iterator it, it_end;
        it_end = sort_fields.end();

        for (it = sort_fields.begin(); it != it_end; ++it)
        {
            std::wstring order_field_str;

            std::wstring field = xd::dequoteIdentifier(g_app->getDatabase(), it->first);

            kl::JsonNode order_field_node = params["order"].appendElement();
            order_field_node["expression"] = it->first;
            if (it->second)
                order_field_node["direction"] = L"DESC";
        }
    }

    return params;
}

void appInvalidObjectMessageBox(const wxString& name,
                                wxWindow* parent)
{
    wxString message;

    if (name.IsEmpty())
    {
        message = _("An object with an invalid name was specified.  Please rename the item(s) to continue.  Database object names, such as table names, must not be a reserved keyword, and must not contain any control characters.  For a listing of reserved keywords and invalid characters, please consult the user documentation.");
    }
     else
    {
        message = wxString::Format(_("The object name '%s' is invalid.  Please rename the item to continue.  Database object names, such as table names, must not be a reserved keyword, and must not contain any control characters.  For a listing of reserved keywords and invalid characters, please consult the user documentation."),
                                   name.c_str());
    }

    deferredAppMessageBox(message,
                       _("Invalid Object Name"),
                       wxOK | wxICON_EXCLAMATION | wxCENTER,
                       parent);
}

void appInvalidFieldMessageBox(const wxString& name,
                               wxWindow* parent)
{
    wxString message;

    if (name.IsEmpty())
    {
        message = _("A field with an invalid name was specified.  Please rename the field to continue.  A field name must not be a reserved keyword, and must not contain any control characters.  For a listing of reserved keywords and invalid characters, please consult the user documentation.");
    }
     else
    {
        message = wxString::Format(_("The field name '%s' is invalid.  Please rename the field to continue.  A field name must not be a reserved keyword, and must not contain any control characters.  For a listing of reserved keywords and invalid characters, please consult the user documentation."),
                                   name.c_str());
    }

    deferredAppMessageBox(message,
                       _("Invalid Field Name"),
                       wxOK | wxICON_EXCLAMATION | wxCENTER,
                       parent);
}

#ifdef __WXGTK__

int appMessageBox(const wxString& message,
                  const wxString& caption,
                  int style,
                  wxWindow *parent,
                  int x, int y)
{
    long decorated_style = style;

    if ( ( style & ( wxICON_EXCLAMATION |
                     wxICON_HAND |
                     wxICON_INFORMATION |
                     wxICON_QUESTION ) ) == 0 )
    {
        decorated_style |= ( style & wxYES ) ? wxICON_QUESTION : wxICON_INFORMATION ;
    }


    if (0 != gtk_check_version(2,4,0))
    {
        // we have an old version of GTK, we must
        // use a different parent window

        int i;
        int cnt = wxTopLevelWindows.GetCount();

        wxWindow* main_wnd = wxTheApp->GetTopWindow();

        if (parent == NULL)
            parent = main_wnd;

        for (i = 0; i < cnt; ++i)
        {
            wxWindow* wnd = (wxWindow*)wxTopLevelWindows.Item(i)->GetData();
            if (wnd != main_wnd)
                parent = wnd;
        }
    }


    wxMessageDialog dialog(parent, message, caption, decorated_style);


    if (0 == gtk_check_version(2,4,0))
    {
        gtk_window_set_keep_above(GTK_WINDOW(dialog.m_widget), true);
    }

    
    int ans = dialog.ShowModal();
    switch ( ans )
    {
        case wxID_OK:
            return wxOK;
        case wxID_YES:
            return wxYES;
        case wxID_NO:
            return wxNO;
        case wxID_CANCEL:
            return wxCANCEL;
    }

    return wxCANCEL;
}

#else

int appMessageBox(const wxString& message,
                  const wxString& caption,
                  int style,
                  wxWindow *parent,
                  int x, int y)
{
    if (parent == NULL)
    {
        parent = wxTheApp->GetTopWindow();
    }

    wxWindowDisabler wd((wxWindow*)NULL);

    return wxMessageBox(message, caption, style, parent, x, y);
}

#endif

// helper class for deferredAppMessageBox()
class MsgBoxData : public wxEvtHandler
{
public:

    bool ProcessEvent(wxEvent& event)
    {
        appMessageBox(message, caption, style, parent, x, y);
        //delete this;
        return true;
    }
    
public:

    wxString message;
    wxString caption;
    int style;
    wxWindow* parent;
    int x;
    int y;
};

void deferredAppMessageBox(const wxString& message,
                           const wxString& caption,
                           int style,
                           wxWindow *parent,
                           int x, int y)
{
    MsgBoxData* data = new MsgBoxData;
    data->message = message;
    data->caption = caption;
    data->style = style;
    data->parent = parent;
    data->x = x;
    data->y = y;
    
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, 9000);
    ::wxPostEvent(data, evt);
}

IDocumentSitePtr lookupOtherDocumentSite(
                           IDocumentSitePtr site,
                           const std::string& doc_class_name)
{
    if (site.isNull())
        return xcm::null;
    
    IFramePtr frame = g_app->getMainFrame();
    if (frame.isNull())
        return xcm::null;
    
    IDocumentSitePtr active_hit;
    IDocumentSitePtr inactive_hit;
        
    IDocumentSiteEnumPtr sites;
    sites = frame->getDocumentSitesByContainer(site->getContainerWindow());
    size_t i, count = sites->size();
    for (i = 0; i < count; ++i)
    {
        IDocumentSitePtr site = sites->getItem(i);
        if (site.isNull())
            continue;
            
        IDocumentPtr doc = site->getDocument();
        if (doc.isNull())
            continue;
            
        xcm::class_info* doc_class_info = xcm::get_class_info(doc);
        if (!doc_class_info)
            continue;
            
        if (doc_class_name == doc_class_info->get_name())
        {
            if (site->isInPlaceActive())
            {
                active_hit = site;
            }
             else
            {
                if (inactive_hit.isNull())
                    inactive_hit = site;
            }
        }
    }
    
    
    // prefer to return the 'other' document
    // i.e. if there are two documents of the
    // same one, return the one that the user
    // currently isn't on
    if (inactive_hit.isOk())
        return inactive_hit;
    
    if (active_hit.isOk())
        return active_hit;
    
    return xcm::null;
}
                                  
IDocumentPtr lookupOtherDocument(
                           IDocumentSitePtr site,
                           const std::string& doc_class_name)
{
    IDocumentSitePtr s = lookupOtherDocumentSite(site, doc_class_name);
    if (s)
    {
        return s->getDocument();
    }
    
    return xcm::null;
}

void switchToOtherDocument(IDocumentSitePtr site,
                           const std::string& doc_class_name)
{
    IDocumentSitePtr s = lookupOtherDocumentSite(site, doc_class_name);
    if (s)
    {
        g_app->getMainFrame()->activateInPlace(s);
    }
}

jobs::IJobPtr appCreateJob(const std::wstring& job_id)
{
    jobs::IJobPtr job = jobs::createJob(job_id);

    if (job.isNull())
        return xcm::null;

    job->setDatabase(g_app->getDatabase());
    job->getJobInfo()->setProgressStringFormat(
                                    towstr(_("$c records processed")),
                                    towstr(_("$c of $m records processed ($p1%)")));

    return job;
}

bool isValidFileVersion(kl::JsonNode node, const std::wstring& type, int version)
{
    if (!node.isOk())
        return false;

    kl::JsonNode metadata_node = node["metadata"];
    if (!metadata_node.isOk())
        return false;

    kl::JsonNode type_node = metadata_node["type"];
    if (!type_node.isOk())
        return false;

    kl::JsonNode version_node = metadata_node["version"];
    if (!version_node.isOk())
        return false;

    if (type_node.getString() != type)
        return false;

    if (version_node.getInteger() != version)
        return false;

    return true;
}

wxString getWebFile(const wxString& urlstring)
{
    std::string fetch_url = (const char*)urlstring.mbc_str();
    std::string result_string;
    
    CURL* curl = curlCreateHandle();
    CurlAutoDestroy ad(curl);
    
    CURLcode curl_result;
    curl_result = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
    curl_result = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_result = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, TRUE);
    curl_result = curl_easy_setopt(curl, CURLOPT_AUTOREFERER, TRUE);
    curl_result = curl_easy_setopt(curl, CURLOPT_MAXREDIRS, -1);
    curl_result = curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_result = curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    curl_result = curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
    curl_result = curl_easy_setopt(curl, CURLOPT_URL, fetch_url.c_str());
    
    // get http proxy info from the registry
    if (g_app->getAppPreferences()->getLong(wxT("internet.proxy.type"), prefProxyDirect) != prefProxyDirect)
    {
        std::string proxy = tostr(getAppPrefsString(wxT("internet.proxy.http")));
        long proxy_port = g_app->getAppPreferences()->getLong(wxT("internet.proxy.http_port"), 8080);

        // set the curl proxy info
        curl_result = curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
        curl_result = curl_easy_setopt(curl, CURLOPT_PROXYPORT, proxy_port);
    }

    
    //if (m_basic_auth.length() > 0)
    //    curl_result = curl_easy_setopt(curl, CURLOPT_USERPWD, m_basic_auth.c_str());
    
    // set the result functions
    curl_result = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&result_string);
    if (curl_result != CURLE_OK)
        return wxEmptyString;
    
    curl_result = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (void*)curl_string_writer);
    if (curl_result != CURLE_OK)
        return wxEmptyString;
    
    // set the GET option  
    curl_result = curl_easy_setopt(curl, CURLOPT_HTTPGET, TRUE);
    if (curl_result != CURLE_OK)
        return wxEmptyString;
    
    // get the full body
    curl_result = curl_easy_setopt(curl, CURLOPT_NOBODY, FALSE);
    if (curl_result != CURLE_OK)
        return wxEmptyString;
    
    // retrieve the data from the URL
    curl_result = curl_easy_perform(curl);
    
    return wxString::FromUTF8(result_string.c_str());
}

bool doOutputPathCheck(const wxString& output_path, wxWindow* parent)
{
    if (!isValidObjectPath(output_path))
    {
        appInvalidObjectMessageBox(wxEmptyString, parent);
        return false;
    }

    wxStringTokenizer tkz(output_path, wxT("/"));
    wxString stub = wxT("/");
    while (tkz.HasMoreTokens())
    {
        wxString token = tkz.GetNextToken();

        if (token.IsEmpty())
            continue;

        if (!tkz.HasMoreTokens())
            break;

        stub += token;

        xd::IFileInfoPtr file_info;
        file_info = g_app->getDatabase()->getFileInfo(towstr(stub));
        if (file_info.isNull())
            break;

        if (file_info->getType() != xd::filetypeFolder)
        {
            appMessageBox(_("The specified output path is invalid because it does not specify a valid folder."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER,
                               parent);
            return false;
        }
    }

    // check the validity of the output filename
    wxString fn = output_path;
    fn = fn.AfterLast(wxT('/'));
    if (!isValidObjectName(fn))
    {
        appMessageBox(_("The specified output path is invalid because it specifies an invalid filename."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER,
                           parent);
        return false;
    }

    // see if the file is there already
    if (!g_app->getDatabase()->getFileExist(towstr(output_path)))
        return true;

    xd::IFileInfoPtr file_info;
    file_info = g_app->getDatabase()->getFileInfo(towstr(output_path));
    if (file_info.isNull())
        return true;

    int file_type = file_info->getType();

    if (file_type == xd::filetypeFolder)
    {
        appMessageBox(_("The specified output path is invalid because it specifies a folder that already exists."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER,
                           parent);
        return false;
    }
     else if (file_type == xd::filetypeTable)
    {
        int result;
        result = appMessageBox(_("The specified output file name already exists.  Would you like to overwrite it?"),
                                    APPLICATION_NAME,
                                    wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION | wxCENTER,
                                    parent);

        if (result != wxYES)
            return false;
    }

    return true;
}

kcl::BannerControl* createModuleBanner(wxWindow* parent, const wxString& title)
{
    // font for drawing the banner
    wxFont font = wxFont(10, wxSWISS, wxNORMAL, wxFONTWEIGHT_BOLD, false);
    
    // colors for drawing the banneruu
    wxColour base_color = kcl::getBaseColor();
    wxColour border_color = kcl::getBorderColor();
    wxColor caption_color = kcl::getCaptionColor();

    // create the banner
    kcl::BannerControl* banner = new kcl::BannerControl(parent,
                                                        title,
                                                        wxPoint(0,0),
                                                        wxSize(200, 40));
    banner->setStartColor(kcl::stepColor(base_color, 170));
    banner->setEndColor(kcl::stepColor(base_color, 90));
    banner->setGradientDirection(kcl::BannerControl::gradientVertical);
    banner->setBorderColor(border_color);
    banner->setBorder(kcl::BannerControl::borderBottom);
    banner->setTextColor(caption_color);
    banner->setTextPadding(15);
    banner->setFont(font);
    return banner;
}

// deferred focus helper
class FocusAgent : public wxEvtHandler
{

public:
    
    wxWindow* m_focus_target;

    bool ProcessEvent(wxEvent& event)
    {
        m_focus_target->SetFocus();

        if (!wxPendingDelete.Member(this))
            wxPendingDelete.Append(this);

        return true;
    }
};

void setFocusDeferred(wxWindow* focus)
{
    FocusAgent* f = new FocusAgent;
    f->m_focus_target = focus;

    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, 10000);
    ::wxPostEvent(f, event);
}

bool windowOrChildHasFocus(wxWindow* wnd)
{
    wxWindow* f = wxWindow::FindFocus();
    while (f)
    {
        if (wnd == f)
            return true;
        f = f->GetParent();
    }
    return false;
}

int getTaskBarHeight()
{
#ifdef WIN32
    APPBARDATA abd;
    memset(&abd, 0, sizeof(APPBARDATA));

    abd.cbSize = sizeof(APPBARDATA);
    if (SHAppBarMessage(ABM_GETSTATE, &abd) & ABS_AUTOHIDE)
        return 2;

    HWND hWndAppBar = abd.hWnd;

    memset(&abd, 0, sizeof(APPBARDATA));
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hWndAppBar;
    SHAppBarMessage(ABM_GETTASKBARPOS, &abd);

    return abd.rc.bottom - abd.rc.top;

#else
    // default task bar height
    return 50;
#endif
}

wxString getUserDocumentFolder()
{
#ifdef WIN32
    TCHAR path[512];
    LPITEMIDLIST pidl = NULL;
    if (FAILED(SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl)))
    {
        return _T("C:\\");
    }
    if (!pidl)
    {
        return _T("C:\\");
    }
    SHGetPathFromIDList(pidl, path);
    LPMALLOC pIMalloc;
    SHGetMalloc(&pIMalloc);
    pIMalloc->Free(pidl);
    pIMalloc->Release();
    if (_tcslen(path) == 0)
        return _T("C:\\");
    return path;
#else
    char path[512];
    char* home = getenv("HOME");
    if (home == NULL || strlen(home) == 0)
    {
        return _T("/");
    }
    strcpy(path, home);
    return path;
#endif

}

void limitFontSize(wxWindow* wnd, int size)
{
    wxFont font = wnd->GetFont();
    if (font.GetPointSize() > size)
    {
        font.SetPointSize(size);
        wnd->SetFont(font);
    }
}

void makeFontBold(wxWindow* window)
{
    wxFont font = window->GetFont();
    font.SetWeight(wxBOLD);
    window->SetFont(font);
}

void setTextWrap(wxStaticText* text, bool wrap)
{
#ifdef WIN32
#endif

#ifdef __WXGTK__
    text->GetBestSize();
    gtk_label_set_line_wrap(GTK_LABEL(text->m_widget), wrap);
#endif
}

void resizeStaticText(wxStaticText* text, int width)
{
    text->SetWindowStyleFlag(text->GetWindowStyleFlag() | wxST_NO_AUTORESIZE);
    setTextWrap(text, true);
    
    wxWindow* parent = text->GetParent();

    int intended_width;

    if (width == -1)
    {
        wxSize s = parent->GetClientSize();
        intended_width = s.GetWidth() - 10;
    }
     else
    {
        intended_width = width;
    }

    // by default
    wxString label = text->GetLabel();
    wxCoord text_width, text_height;
    
    wxClientDC dc(text->GetParent());
    dc.SetFont(text->GetFont());
    dc.GetTextExtent(label, &text_width, &text_height);

    // add 5% to the text width
    text_width = (text_width*100)/95;
    
    if (intended_width < 50)
        intended_width = 50;
        
    int row_count = (text_width/intended_width)+1;
    
    const wxChar* c = label.c_str();
    while (*c)
    {
        if (*c == L'\n')
            row_count++;
        ++c;
    }

    text->SetSize(intended_width, row_count*text_height);
    text->SetSizeHints(intended_width, row_count*text_height);
}

wxSize getMaxTextSize(wxStaticText* st0,
                      wxStaticText* st1,
                      wxStaticText* st2,
                      wxStaticText* st3,
                      wxStaticText* st4,
                      wxStaticText* st5,
                      wxStaticText* st6)
{
    wxStaticText* ctrls[7];

    ctrls[0] = st0;
    ctrls[1] = st1;
    ctrls[2] = st2;
    ctrls[3] = st3;
    ctrls[4] = st4;
    ctrls[5] = st5;
    ctrls[6] = st6;

    int sx, sy;
    int x, y;
    x = 0;
    y = 0;

    for (int i = 0; i < 7; i++)
    {
        if (!ctrls[i])
        {
            continue;
        }

        ctrls[i]->GetBestSize(&sx, &sy);
        if (sx > x)
            x = sx;
        if (sy > y)
            y = sy;
    }

    return wxSize(x, y);
}

// helper for suppressConsoleLogging()
#ifdef __WXGTK__
void null_log_function(const gchar* log_domain,
                       GLogLevelFlags log_level,
                       const gchar* message,
                       gpointer user_data)
{
}
#endif

void suppressConsoleLogging()
{
    // no logging on wx
    wxLog::EnableLogging(false);

#ifdef __WXGTK__
    // no logging on gtk -- but only disable it
    // if gtk is being used at all
    if (gdk_screen_get_default())
    {
        g_log_set_handler("Gdk",
                      (GLogLevelFlags)(
                      G_LOG_LEVEL_WARNING |
                      G_LOG_LEVEL_ERROR | 
                      G_LOG_LEVEL_CRITICAL |
                      G_LOG_FLAG_FATAL |
                      G_LOG_FLAG_RECURSION),
                      null_log_function,
                      NULL);
    }
#endif
}

wxString getProxyServer()
{
    //wxT("10.1.1.1:3128");
#ifdef __WXMSW__

    wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"));

    if(!pRegKey->Exists())
        return wxEmptyString;

    long enable = 0;

    if (!pRegKey->QueryValue(wxT("ProxyEnable"), &enable))
        return wxEmptyString;

    if (!enable)
        return wxEmptyString;

    wxString proxy;
    if (!pRegKey->QueryValue(wxT("ProxyServer"), proxy))
        return wxEmptyString;

    return proxy;

#else

    wxString val;
    if (!::wxGetEnv(wxT("HTTP_PROXY"), &val))
        return wxEmptyString;
    return val.AfterLast(wxT('/'));

#endif

}



struct LocalePtrContainer
{
    LocalePtrContainer()
    {
        m_ptr = NULL;
    }

    ~LocalePtrContainer()
    {
        delete m_ptr;
    }

    Locale* getLocalePtr()
    {
        if (m_ptr)
            return m_ptr;
        m_ptr = new Locale;
        return m_ptr;
    }

    Locale* m_ptr;
};

LocalePtrContainer g_locale_ptr_container;

Locale::Locale()
{
    lconv* l = localeconv();
    decimal_point = (unsigned char)*l->decimal_point;
    thousands_separator = (unsigned char)*l->thousands_sep;
    paper_type = wxPAPER_LETTER;

    // get date format
#ifdef WIN32
    TCHAR s[32];
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDATE, s, 32);
    if (*s == wxT('1'))
        date_order = DateOrderDDMMYY;
            else
        date_order = DateOrderMMDDYY;

    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDATE, s, 32);
    date_separator = s[0];
#else
    date_separator = '/';
    date_order = DateOrderMMDDYY;
#endif

    // get default paper size
#ifdef WIN32
    GetLocaleInfo(LOCALE_USER_DEFAULT, 0x0000100A, s, 32);
    if (*s == wxT('1'))
        paper_type = wxPAPER_LETTER;
            else
        paper_type = wxPAPER_A4;
#endif
}

Locale::~Locale()
{
}

// static
const Locale* Locale::getSettings()
{
    return g_locale_ptr_container.getLocalePtr();
}


wxString Locale::formatDate(int year,
                            int month,
                            int day,
                            int hour,
                            int min,
                            int sec)
{
    static int date_order = -1;
    static wxChar date_separator = 0;

    if (date_order == -1)
    {
        date_order = Locale::getSettings()->date_order;
        date_separator = Locale::getSettings()->date_separator;
    }

    if (date_order == DateOrderMMDDYY)
    {
        if (hour == -1)
        {
            return wxString::Format(wxT("%02d%c%02d%c%04d"),
                                    month, date_separator,
                                    day,   date_separator,
                                    year);
        }
         else
        {
            return wxString::Format(wxT("%02d%c%02d%c%04d %02d:%02d:%02d"),
                                    month, date_separator,
                                    day,   date_separator,
                                    year,
                                    hour, min, sec);
        }
    }
     else if (date_order == DateOrderDDMMYY)
    {
        if (hour == -1)
        {
            return wxString::Format(wxT("%02d%c%02d%c%04d"),
                                    day,   date_separator,
                                    month, date_separator,
                                    year);
        }
         else
        {
            return wxString::Format(wxT("%02d%c%02d%c%04d %02d:%02d:%02d"),
                                    day,   date_separator,
                                    month, date_separator,
                                    year,
                                    hour, min, sec);
        }
    }

    return wxEmptyString;
}

bool Locale::parseDateTime(const wxString& input,
                           int* year,
                           int* month,
                           int* day,
                           int* hour,
                           int* minute,
                           int* second)
{
    if (year)
        *year = -1;
    if (month)
        *month = -1;
    if (day)
        *day = -1;
    if (hour)
        *hour = -1;
    if (minute)
        *minute = -1;
    if (second)
        *second = -1;

    std::vector<int> parts;

    wxStringTokenizer tkz(input, wxT("/-.: "));
    while (tkz.HasMoreTokens())
    {
        wxString part = tkz.GetNextToken();
        if (part.IsEmpty())
            continue;
        parts.push_back(wxAtoi(part));
    }

    int part_count = parts.size();

    if (part_count < 3)
    {
        return false;
    }    

    int y = 0, m = 0, d = 0, hh = -1, mm = -1, ss = 0;


    // date part positions
    int year_pos = 2;
    int month_pos = 0;
    int day_pos = 1;

    if (Locale::getDateOrder() == Locale::DateOrderDDMMYY)
    {
        year_pos = 2;
        month_pos = 1;
        day_pos = 0;
    }


    // first assign date portion
    y = parts[year_pos];
    m = parts[month_pos];
    d = parts[day_pos];

    if (part_count == 4)
        return false;

    if (part_count >= 5)
    {
        // time is included
        hh = parts[3];
        mm = parts[4];

        if (part_count > 5)
        {
            ss = parts[5];
        }
    }


    // check validity
    if (m <= 0 || m > 12)
    {
        return false;
    }

    int month_days[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (d <= 0 || d > month_days[m-1])
    {
        return false;
    }

    if (y < 70)
    {
        y += 2000;
    }
     else if (y < 100)
    {
        y += 1900;
    }

    if (part_count >= 5)
    {
        if (hh == 24)
            hh = 0;

        if (hh < 0 || hh >= 24)
            return false;

        if (m < 0 || mm >= 60)
            return false;
        
        if (part_count > 5)
        {
            if (ss < 0 || ss >= 60)
                return false;
        }
    }

    if (year)
        *year = y;
    if (month)
        *month = m;
    if (day)
        *day = d;
    if (hour)
        *hour = hh;
    if (minute)
        *minute = mm;
    if (second)
        *second = ss;

    return true;
}


BEGIN_EVENT_TABLE(JobGaugeUpdateTimer, wxTimer)
    EVT_MENU(10000, JobGaugeUpdateTimer::onJobAddedInMainThread)
    EVT_MENU(10001, JobGaugeUpdateTimer::onJobStateChangedInMainThread)
END_EVENT_TABLE()

JobGaugeUpdateTimer::JobGaugeUpdateTimer(
                    IStatusBarPtr statusbar,
                    jobs::IJobQueuePtr job_queue,
                    wxGauge* gauge)
{
    m_statusbar = statusbar;
    m_gauge = gauge;
    
    m_job_queue = job_queue;
    m_job_queue->sigJobAdded().connect(this, &JobGaugeUpdateTimer::onJobAdded);
    
    m_job_text_item = m_statusbar->getItem(wxT("app_job_text"));
    m_job_separator_item = m_statusbar->getItem(wxT("app_job_text_separator"));
    m_job_gauge_item = m_statusbar->getItem(wxT("app_job_gauge"));
    m_job_failed_item = m_statusbar->getItem(wxT("app_job_failed"));
}

JobGaugeUpdateTimer::~JobGaugeUpdateTimer()
{
}

void JobGaugeUpdateTimer::hideJobFailedIcon()
{
    if (m_job_failed_item->isShown())
    {
        m_job_failed_item->show(false);
        m_statusbar->populate();
    }
}

void JobGaugeUpdateTimer::showJobFailedIcon()
{
    if (!m_job_failed_item->isShown())
    {
        m_job_failed_item->show(true);
        m_statusbar->populate();
    }
}

void JobGaugeUpdateTimer::UnInit()
{
    m_statusbar.p = NULL;
    m_job_queue.clear();
    m_gauge = NULL;
}

void JobGaugeUpdateTimer::Notify()
{
    wxASSERT_MSG(::wxIsMainThread(), wxT("Being called outside of main/gui thread!"));
    wxASSERT_MSG(m_statusbar.isOk(), wxT("Null statusbar!"));
    wxASSERT_MSG(m_job_queue.isOk(), wxT("Null job queue!"));
    wxASSERT_MSG(m_job_text_item.isOk(), wxT("Null status item!"));
    wxASSERT_MSG(m_job_gauge_item.isOk(), wxT("Null status item!"));
    wxASSERT_MSG(m_job_failed_item.isOk(), wxT("Null status item!"));
    wxASSERT_MSG(m_gauge != NULL, wxT("Null gauge control!"));
    
    if (!m_job_queue->getJobsActive())
    {
        // no running jobs, so hide the job gauge and text
        if (m_job_text_item->isShown() || m_job_gauge_item->isShown())
        {
            m_job_text_item->show(false);
            m_job_separator_item->show(false);
            m_job_gauge_item->show(false);
            m_job_text_item->setValue(wxEmptyString);
            m_statusbar->populate();
            
            // make sure the statusbar's refresh signal is fired here
            m_statusbar->refresh();
        }
        
        return;
    }



    // job progress statusbar text
    wxString job_text;

    // this is the average of all of the job progress percentages
    double tot_pct_done = 0.0;
    
    // this flag indicates whether the job gauge should pulse or not
    bool is_indeterminate = false;
    
    // determine the overall progress
    jobs::IJobInfoEnumPtr jobs = m_job_queue->getJobInfoEnum(jobs::jobStateRunning);
    size_t i, job_count = jobs->size();
    
    // this check is here because sometimes the running job count is
    // equal to zero before getJobsActive() is false -- when this happens,
    // don't go past this point; instead, let the above check get processed
    if (job_count == 0)
        return;
    
    if (job_count > 0)
    {
        // there's at least one job running, make sure
        // the job text and gauge are shown
        if (!m_job_text_item->isShown() || !m_job_gauge_item->isShown())
        {
            m_job_text_item->show(true);
            m_job_separator_item->show(true);
            m_job_gauge_item->show(true);
            m_statusbar->populate();
        }
    }
    
    for (i = 0; i < job_count; i++)
    {
        jobs::IJobInfoPtr job_info = jobs->getItem(i);
        
        // update the total percentage
        double pct = job_info->getPercentage();
        if ((job_info->getInfoMask() & jobs::jobMaskPercentage) && pct >= 0.0)
            tot_pct_done += pct;

        // determine if the job is indeterminate or not
        if (kl::dblcompare(job_info->getMaxCount(), 0.0) == 0)
        {
            is_indeterminate = true;
            break;
        }
         else
        {
            time_t start_time = job_info->getStartTime();
            time_t current_time = time(NULL);
            
            // the job has been running for more than 3 seconds and
            // is still stuck at 0%, so it's indeterminate right now
            if ((current_time - start_time) >= 3 && pct == 0.0)
            {
                is_indeterminate = true;
                break;
            }
        }
    }
    
    // make sure we average the total percetanges of all running jobs
    tot_pct_done /= job_count;
    
    // create the job statusbar text
    if (job_count == 1)
        job_text = jobs->getItem(0)->getTitle();
         else if (job_count > 1)
        job_text = wxString::Format(_("%d jobs running"), job_count);
    
    // update the gauge and update job statusbar text where appropriate
    if (is_indeterminate)
    {
        m_gauge->Pulse();
        
        // add a little space between job gauge and job text
        job_text.Prepend(wxT(" "));
    }
     else
    {
        m_gauge->SetValue((int)tot_pct_done);
        job_text.Prepend(wxString::Format(wxT("%.1f%%  "), tot_pct_done));
    }
    
    // if the statusbar text has changed, refresh the statusbar
    if (m_job_text_item->getValue().Cmp(job_text) != 0)
    {
        m_job_text_item->setValue(job_text);
        m_statusbar->refresh();
    }
}

void JobGaugeUpdateTimer::onJobAdded(jobs::IJobInfoPtr job_info)
{
    // post an event that a job has been added; we need to send
    // an event rather than rely on the job state because some
    // jobs process very quickly, and it's not possible to
    // catch the job running before the job state is set to
    // finished
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, 10000);
    evt.SetExtraLong((long)job_info.p);

    // when a job sets its state, it may not be in the main thread, so it is
    // important that we do this here since there are GUI operations involved
    // and we can't have GUI stuff happening outside of the main thread
    if (::wxIsMainThread())
        onJobAddedInMainThread(evt);
         else
        ::wxPostEvent(this, evt);

    // connect the job state changed function so we can track the
    // state of individual jobs        
    job_info->sigStateChanged().connect(this, &JobGaugeUpdateTimer::onJobStateChanged);
}

void JobGaugeUpdateTimer::onJobStateChanged(jobs::IJobInfoPtr job_info)
{
    // post an event that a particular job status has changed
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, 10001);
    evt.SetExtraLong((long)job_info.p);

    // when a job sets its state, it may not be in the main thread, so it is
    // important that we do this here since there are GUI operations involved
    // and we can't have GUI stuff happening outside of the main thread
    if (::wxIsMainThread())
        onJobStateChangedInMainThread(evt);
         else
        ::wxPostEvent(this, evt);
}

void JobGaugeUpdateTimer::onJobAddedInMainThread(wxCommandEvent& evt)
{
    wxASSERT_MSG(::wxIsMainThread(), wxT("Being called outside of main/gui thread!"));

    // if a new job is added, hide the job failed icon;
    // we want the user to know if any of the jobs running
    // in the queue fails; however, when a new job is added 
    // after a failure has occurred, we want the user to see 
    // if the new job fails so they can see if the problem 
    // has been fixed rather than seeing the failed state of 
    // a previous job

    hideJobFailedIcon();
}

void JobGaugeUpdateTimer::onJobStateChangedInMainThread(wxCommandEvent& evt)
{
    wxASSERT_MSG(::wxIsMainThread(), wxT("Being called outside of main/gui thread!"));

    jobs::IJobInfoPtr job_info = (jobs::IJobInfo*)evt.GetExtraLong();
    if (job_info->getState() == jobs::jobStateFailed)
    {
        // if any of the jobs in the queue fail,
        // show a job failed icon

        showJobFailedIcon();
    }
}

