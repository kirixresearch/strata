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
    kl::toUtf8 conv(contents);
    const char* buf = conv;
    size_t len = strlen(buf);


    xf_file_t f = xf_open(path, xfCreate, xfReadWrite, xfShareNone);
    if (!f)
        return false;

    static const unsigned char bom[3] = { 0xef, 0xbb, 0xbf };
    if (xf_write(f, bom, 1, 3) != 3)
    {
        xf_close(f);
        return false;
    }

    if (xf_write(f, buf, 1, len) != len)
    {
        xf_close(f);
        return false;
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
        if (res[len] == xf_path_separator_wchar)
            res = res.substr(0, len-1);
        return kl::beforeLast(res, xf_path_separator_wchar);
    }
}
