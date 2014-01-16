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
    std::wstring::const_iterator it, it_end = contents.cend();
    for (it = contents.cbegin(); it != it_end; ++it)
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
    std::wstring::const_iterator it, it_end = contents.cend();
    for (it = contents.cbegin(); it != it_end; ++it)
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




