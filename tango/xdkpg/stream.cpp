/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-05-12
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <kl/file.h>
#include <kl/xml.h>
#include "tango.h"
#include "database.h"
#include "stream.h"
#include "pkgfile.h"

KpgStream::KpgStream(KpgDatabase* database)
{
    m_reader = NULL;
    m_writer = NULL;

    m_database = database;
    m_database->ref();
}

KpgStream::~KpgStream()
{
    delete m_reader;

    if (m_writer)
    {
        m_writer->finishWrite();
        delete m_writer;
    }

    m_database->unref();
}

bool KpgStream::init(const std::wstring& path, const std::wstring& mime_type)
{
    m_path = path;
    m_mime_type = mime_type;

    return true;
}


bool KpgStream::read(void* buf,
                     unsigned long read_size,
                     unsigned long* read_count)
{
    if (m_writer)
    {
        // we are writing, not reading to the stream;
        // in xdkpg, only one is possible at a time
        return false;
    }

    if (!m_reader)
    {
        m_reader = m_database->m_kpg->readStream(m_path);
        if (!m_reader)
            return false;

        // skip past the info block
        int info_block_size = 0;
        m_reader->loadNextBlock(&info_block_size);
    }

    size_t size = m_membuf.getDataSize();
    if (read_size > size)
    {
        // caller wants more data than we presently have;
        // try to get more
        int block_size = 0;
        const void* data = m_reader->loadNextBlock(&block_size);
        if (data && block_size > 0)
        {
            m_membuf.append((unsigned char*)data, (size_t)block_size);
        }
    }


    if (read_size > m_membuf.getDataSize())
        read_size = m_membuf.getDataSize();

    if (read_size == 0)
        return false;

    memcpy(buf, m_membuf.getData(), read_size);
    *read_count = read_size;
    m_membuf.popData(read_size);

    return true;
}
                  
bool KpgStream::write(const void* buf,
                      unsigned long write_size,
                      unsigned long* written_count)
{
    if (m_reader)
    {
        // we are writing, not reading to the stream;
        // in xdkpg, only one is possible at a time
        return false;
    }

    if (!m_writer)
    {
        m_writer = m_database->m_kpg->createStream(m_path);
        if (!m_writer)
            return false;

        // write out the info block
        kl::xmlnode stream_info;

        stream_info.setNodeName(L"pkg_stream_info");
        stream_info.appendProperty(L"type", L"stream");
        stream_info.addChild(L"version", 1);

        kl::xmlnode& mime_type_node = stream_info.addChild();
        mime_type_node.setNodeName(L"mime_type");
        mime_type_node.setNodeValue(m_mime_type);


        std::wstring info_block_str = stream_info.getXML(kl::xmlnode::formattingSpaces |
                                                         kl::xmlnode::formattingCrLf);

        m_writer->startWrite();

        // create a little-endian UCS-2 version of the info block xml
        int buf_len = (info_block_str.length()+1)*2;
        unsigned char* info_block = new unsigned char[buf_len];
        kl::wstring2ucsle(info_block, info_block_str, buf_len/2);
        m_writer->writeBlock(info_block, buf_len, true);
        delete[] info_block;
    }

    return m_writer->writeBlock(buf, (int)write_size, true);
}
