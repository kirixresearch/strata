/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-01-13
 *
 */


#include <xd/xd.h>
#include "fileinfo.h"


namespace xdcommon
{


FileInfo::FileInfo()
{
    name = L"";
    mime_type = L"";
    type = 0;
    format = 0;
    size = 0;
    row_count = 0;
    flags = 0;
    is_mount = false;
}

const std::wstring& FileInfo::getName()
{
    return name;
}

const std::wstring& FileInfo::getMimeType()
{
    return mime_type;
}

int FileInfo::getType()
{
    return type;
}

int FileInfo::getFormat()
{
    return format;
}

unsigned int FileInfo::getFlags()
{
    return flags;
}


long long FileInfo::getSize()
{
    return size;
}

xd::rowpos_t FileInfo::getRowCount()
{
    return row_count;
}

bool FileInfo::isMount()
{
    return is_mount;
}

const std::wstring& FileInfo::getPrimaryKey()
{
    return primary_key;
}

const std::wstring& FileInfo::getObjectId()
{
    return object_id;
}

}; // namespace xdcommon