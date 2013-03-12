/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-03-12
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "nodefilestream.h"
#include "ofs.h"


NodeFileStream::NodeFileStream(tango::IDatabase* db)
{
    m_db = db;
    m_utf8data = NULL;
}

NodeFileStream::~NodeFileStream()
{
}

bool NodeFileStream::create(const std::wstring& filename)
{
    // note: created for backward compatability, so notion
    // of creating a node file from a stream

    return false;
}

bool NodeFileStream::open(const std::wstring& filename)
{
    // try to open the file
    // OfsFile* ofsfile = OfsFile::openFile(m_db, filename);

    // 1. read in the entire nodefile into JsonNode objects using the OfsFile* and OfsValue* classes
    // 2. convert the JsonNode object to a std::wstring
    // 3. allocate a const char* m_utf8data member to 6 times the length of the json string
    // 4. convert the std::wstring containing the json data to utf8 in the member
    // 5. further stream operations will work on the m_utf8data

    return false;
}

bool NodeFileStream::read(void* buf,
                          unsigned long read_size,
                          unsigned long* read_count)
{
    return false;
}
                  
bool NodeFileStream::write(const void* buf,
                           unsigned long write_size,
                           unsigned long* written_count)
{
    // note: created for backward compatability, so notion
    // of writing to a node file from a stream

    return false;
}

