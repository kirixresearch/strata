/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2012-10-05
 *
 */


#include <kl/file.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/system.h>


std::wstring xf_get_file_contents(const std::wstring& path, bool* success)
{
    xf_file_t f = xf_open(path, xfOpen, xfRead, xfShareReadWrite);
    if (!f)
    {
        if (success) *success = false;
        return L"";
    }

    std::wstring value;

    xf_off_t fsize = xf_get_file_size(path);
    unsigned char* buf = new unsigned char[fsize+1];
    if (!buf)
    {
        if (success) *success = false;
        return L"";
    }

    xf_off_t readbytes = xf_read(f, buf, 1, fsize);
    buf[readbytes] = 0;
        
    if (readbytes >= 2 && buf[0] == 0xff && buf[1] == 0xfe)
    {
        // little endian UCS-2
        kl::ucsle2wstring(value, buf+2, (readbytes-2)/2);
    }
     else if (readbytes >= 3 && buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
    {
        // utf-8
        wchar_t* tempbuf = new wchar_t[fsize+1];
        kl::utf8_utf8tow(tempbuf, fsize+1, (char*)buf+3, readbytes);
        value = tempbuf;
        delete[] tempbuf;
    }
     else
    {
        value = kl::towstring((char*)buf);
    }
        
    delete[] buf;
    xf_close(f);

    if (success) *success = true;
    return value;
}

bool xf_put_file_contents(const std::wstring& path, const std::wstring& contents)
{
    bool is_unicode = false;
    std::wstring::const_iterator it, it_end = contents.end();
    for (it = contents.begin(); it != it_end; ++it)
    {
        if (*it > (wchar_t)127)
        {
            is_unicode = true;
            break;
        }
    }




    xf_file_t f = xf_open(path, xfCreate, xfReadWrite, xfShareNone);
    if (!f)
        return false;

    if (is_unicode)
    {
        static const unsigned char bom[3] = { 0xef, 0xbb, 0xbf };
        if (xf_write(f, bom, 1, 3) != 3)
        {
            xf_close(f);
            return false;
        }

        kl::toUtf8 conv(contents);
        const char* buf = conv;
        size_t len = strlen(buf);

        if (xf_write(f, buf, 1, len) != len)
        {
            xf_close(f);
            return false;
        }
    }
     else
    {
        std::string asc = kl::tostring(contents);

        size_t len = asc.length();
        if (xf_write(f, (const char*)asc.c_str(), 1, len) != len)
        {
            xf_close(f);
            return false;
        }
    }

    xf_close(f);
    return true;
}

std::wstring xf_get_file_directory(const std::wstring& filename)
{
    if (filename.empty())
    {
        return filename;
    }
     else
    {
        std::wstring res = filename;
        size_t len = res.length();
        if (res[len-1] == xf_path_separator_wchar)
            res = res.substr(0, len-1);
        return kl::beforeLast(res, xf_path_separator_wchar);
    }
}


std::wstring xf_concat_path(const std::wstring& path, const std::wstring& append)
{
    if (path.empty())
        return append;
    
    std::wstring result = path;
    wchar_t last = result[result.length()-1];
    if (last != '/' && last != '\\')
        result += xf_path_separator_wchar;

    return (result + append);
}



static std::wstring getExtensionFromPath(const std::wstring& path)
{
    if (path.find('.') == path.npos)
        return L"";

    return kl::afterLast(path, '.');
}

std::wstring xf_get_mimetype_from_extension(const std::wstring& filename)
{
    std::wstring ext = getExtensionFromPath(filename);
    if (ext.length() == 0)
        ext = filename;
    kl::makeLower(ext);
    
         if (ext == L"bmp")                        return L"image/x-ms-bmp";
    else if (ext == L"css")                        return L"text/css";
    else if (ext == L"csv")                        return L"text/csv";
    else if (ext == L"dbf")                        return L"application/dbf";
    else if (ext == L"icsv")                       return L"application/vnd.interchange-csv";
    else if (ext == L"tsv")                        return L"text/tab-separated-values";
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
    else if (ext == L"xlsx")                       return L"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    else if (ext == L"doc")                        return L"application/msword";
    else if (ext == L"docx")                       return L"application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    else if (ext == L"ppt")                        return L"application/vnd.ms-powerpoint";
    else if (ext == L"pptx")                       return L"application/vnd.openxmlformats-officedocument.presentationml.presentation";
    else if (ext == L"xml")                        return L"application/xml";
    else if (ext == L"zip")                        return L"application/zip";
    else                                           return L"application/octet-stream";
}



std::wstring xf_get_extension_from_mimetype(const std::wstring& mime_type)
{
         if (mime_type == L"image/x-ms-bmp")                                                               return L"bmp";
    else if (mime_type == L"text/css")                                                                     return L"css";
    else if (mime_type == L"text/csv")                                                                     return L"csv";
    else if (mime_type == L"application/vnd.interchange-csv")                                              return L"icsv";
    else if (mime_type == L"application/dbf")                                                              return L"dbf";
    else if (mime_type == L"application/dbase")                                                            return L"dbf";
    else if (mime_type == L"application/x-dbf")                                                            return L"dbf";
    else if (mime_type == L"application/x-dbase")                                                          return L"dbf";
    else if (mime_type == L"text/tab-separated-values")                                                    return L"tsv";
    else if (mime_type == L"image/gif")                                                                    return L"gif";
    else if (mime_type == L"application/x-gzip")                                                           return L"gz";
    else if (mime_type == L"text/html")                                                                    return L"html";
    else if (mime_type == L"application/hta")                                                              return L"hta";
    else if (mime_type == L"image/jpeg")                                                                   return L"jpg";
    else if (mime_type == L"application/javascript")                                                       return L"js";
    else if (mime_type == L"application/json")                                                             return L"json";
    else if (mime_type == L"application/pdf")                                                              return L"pdf";
    else if (mime_type == L"image/png")                                                                    return L"png";
    else if (mime_type == L"image/svg+xml")                                                                return L"svg";
    else if (mime_type == L"image/tiff")                                                                   return L"tiff";
    else if (mime_type == L"text/plain")                                                                   return L"txt";
    else if (mime_type == L"application/vnd.ms-excel")                                                     return L"xls";
    else if (mime_type == L"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet")            return L"xlsx";
    else if (mime_type == L"application/msword")                                                           return L"doc";
    else if (mime_type == L"application/vnd.openxmlformats-officedocument.wordprocessingml.document")      return L"docx";
    else if (mime_type == L"application/vnd.ms-powerpoint")                                                return L"ppt";
    else if (mime_type == L"application/vnd.openxmlformats-officedocument.presentationml.presentation")    return L"pptx";
    else if (mime_type == L"application/xml")                                                              return L"xml";
    else if (mime_type == L"application/zip")                                                              return L"zip";
    else                                                                                                   return L"";
}


namespace kl
{


exclusive_file::exclusive_file(const std::wstring& path, int timeout)
{
    m_existed = xf_get_file_exist(path);

    m_f = xf_open(path, xfOpenCreateIfNotExist, xfReadWrite, xfShareReadWrite);
    if (m_f)
    {
        if (!xf_trylock(m_f, 0, 0, timeout))
        {
            xf_close(m_f);
            m_f = NULL;
        }
    }
}

exclusive_file::~exclusive_file()
{
    xf_unlock(m_f, 0, 0);
    xf_close(m_f);
}


std::wstring exclusive_file::getContents(bool* success)
{
    std::wstring value;

    xf_seek(m_f, 0, xfSeekEnd);
    xf_off_t fsize = xf_get_file_pos(m_f);
    xf_seek(m_f, 0, xfSeekSet);

    unsigned char* buf = new unsigned char[fsize+1];
    if (!buf)
    {
        if (success) *success = false;
        return L"";
    }

    xf_off_t readbytes = xf_read(m_f, buf, 1, fsize);
    buf[readbytes] = 0;
        
    if (readbytes >= 2 && buf[0] == 0xff && buf[1] == 0xfe)
    {
        // little endian UCS-2
        kl::ucsle2wstring(value, buf+2, (readbytes-2)/2);
    }
     else if (readbytes >= 3 && buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf)
    {
        // utf-8
        wchar_t* tempbuf = new wchar_t[fsize+1];
        kl::utf8_utf8tow(tempbuf, fsize+1, (char*)buf+3, readbytes);
        value = tempbuf;
        delete[] tempbuf;
    }
     else
    {
        value = kl::towstring((char*)buf);
    }

    delete[] buf;
    return value;
}

bool exclusive_file::putContents(const std::wstring& contents)
{
    if (!m_f)
        return false;

    xf_truncate(m_f);

    bool is_unicode = false;
    std::wstring::const_iterator it, it_end = contents.end();
    for (it = contents.begin(); it != it_end; ++it)
    {
        if (*it > (wchar_t)127)
        {
            is_unicode = true;
            break;
        }
    }

    if (is_unicode)
    {
        static const unsigned char bom[3] = { 0xef, 0xbb, 0xbf };
        if (xf_write(m_f, bom, 1, 3) != 3)
            return false;

        kl::toUtf8 conv(contents);
        const char* buf = conv;
        size_t len = strlen(buf);

        return (xf_write(m_f, buf, 1, len) == len ? true : false);
    }
     else
    {
        std::string asc = kl::tostring(contents);
        size_t len = asc.length();
        return (xf_write(m_f, (const char*)asc.c_str(), 1, len) == len ? true : false);
    }

}



};




