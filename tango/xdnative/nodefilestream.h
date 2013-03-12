/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2013-03-12
 *
 */


#ifndef __XDNATIVE_NODEFILESTREAM_H
#define __XDNATIVE_NODEFILESTREAM_H


#include <kl/file.h>


class NodeFileStream : public tango::IStream
{
    XCM_CLASS_NAME("tango.NodeFileStream")
    XCM_BEGIN_INTERFACE_MAP(NodeFileStream)
        XCM_INTERFACE_ENTRY(tango::IStream)
    XCM_END_INTERFACE_MAP()

public:

    NodeFileStream(tango::IDatabase* db);
    ~NodeFileStream();

    bool create(const std::wstring& filename);
    bool open(const std::wstring& filename);

    bool read(void* buf,
              unsigned long read_size,
              unsigned long* read_count);

    bool write(const void* buf,
               unsigned long write_size,
               unsigned long* written_count);

private:

    tango::IDatabase* m_db;
    char* m_utf8data;
    unsigned long m_stream_length;
    unsigned long m_stream_offset;
};


#endif

