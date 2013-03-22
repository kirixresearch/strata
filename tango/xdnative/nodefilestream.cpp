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
#include "database.h"
#include "ofs.h"
#include "kl/json.h"
#include "kl/utf8.h"


static bool convertNodeToJson(INodeValuePtr ofsnode, kl::JsonNode& jsonnode)
{
    if (ofsnode.isNull())
        return false;

    // set the object name and value by default
    std::wstring name = ofsnode->getName();
    std::wstring value = ofsnode->getString();
    jsonnode[name] = value;

    // if we don't have any children, we're done
    int ofsnode_child_count = ofsnode->getChildCount();
    if (ofsnode_child_count == 0)
        return true;

    // get the children and convert them
    kl::JsonNode jsonnode_child = jsonnode[name];
    for (int idx = 0; idx < ofsnode_child_count; ++idx)
    {
        INodeValuePtr ofsnode_child = ofsnode->getChildByIdx(idx);
        if (!convertNodeToJson(ofsnode_child, jsonnode_child))
            return false;
    }

    return true;
}


NodeFileStream::NodeFileStream(tango::IDatabase* db)
{
    m_db = db;
    m_utf8data = NULL;
    m_stream_length = 0;
    m_stream_offset = 0;
}

NodeFileStream::~NodeFileStream()
{
    delete[] m_utf8data;
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
    OfsFile* ofsfile = OfsFile::openFile(m_db, filename);
    if (!ofsfile)
        return false;

    // get the ofs node and convert it to a json string
    INodeValuePtr ofsnode = ofsfile->getRootNode();
    if (ofsnode.isNull())
        return false;

    kl::JsonNode jsonnode;
    if (!convertNodeToJson(ofsnode, jsonnode))
        return false;

    std::wstring jsonstr = jsonnode.toString();

    // prepare to move the value in the json string to the buffer;
    // if the buffer has already been allocated, release the previously 
    // allocated memory
    if (m_utf8data)
        delete[] m_utf8data;

    // set the buffer
    unsigned int bufsize = jsonstr.length()*6;
    m_utf8data = new char[bufsize];  // 6 times string length
    kl::utf8_wtoutf8(m_utf8data, bufsize, jsonstr.c_str(), jsonstr.length());

    // reset the read offset
    m_stream_length = strlen(m_utf8data);
    m_stream_offset = 0;

    return true;
}

bool NodeFileStream::read(void* buf,
                          unsigned long read_size,
                          unsigned long* read_count)
{
    // nothing more to return
    if (m_stream_offset >= m_stream_length)
        return false;

    // set the read size so it doesn't exceed the bounds
    *read_count = read_size;
    if (m_stream_length - m_stream_offset < *read_count)
        *read_count = m_stream_length - m_stream_offset;

    memcpy(buf, m_utf8data + m_stream_offset, *read_count);
    m_stream_offset += *read_count;

    return true;
}
                  
bool NodeFileStream::write(const void* buf,
                           unsigned long write_size,
                           unsigned long* written_count)
{
    // note: created for backward compatability, so notion
    // of writing to a node file from a stream

    return false;
}

