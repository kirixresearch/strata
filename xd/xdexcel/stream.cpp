/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-05-12
 *
 */


#include <kl/file.h>
#include <kl/xml.h>
#include <xd/xd.h>
#include "database.h"
#include "stream.h"


ExcelStream::ExcelStream(ExcelDatabase* database)
{
    m_database = database;
    m_database->ref();
}

ExcelStream::~ExcelStream()
{
    m_database->unref();
}

bool ExcelStream::init(const std::wstring& path, const std::wstring& mime_type)
{
    m_path = path;
    m_mime_type = mime_type;

    return true;
}


bool ExcelStream::read(void* buf,
                     unsigned long read_size,
                     unsigned long* read_count)
{
    return false;
}
                  
bool ExcelStream::write(const void* buf,
                      unsigned long write_size,
                      unsigned long* written_count)
{
    return false;
}


bool ExcelStream::seek(long long seek_pos, int whence)
{
    // not implemented in ExcelStream
    return false;
}


long long ExcelStream::getSize()
{
    // not implemented in ExcelStream
    return 0;
}

std::wstring ExcelStream::getMimeType()
{
    // not implemented in ExcelStream
    return L"";
}