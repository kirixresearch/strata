/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-13
 *
 */


#ifndef H_XDKPG_STREAM_H
#define H_XDKPG_STREAM_H


#include <kl/memory.h>

class KpgDatabase;
class PkgStreamReader;
class PkgStreamWriter;

class KpgStream : public xd::IStream
{
    XCM_CLASS_NAME("xdpgsql.KpgStream")
    XCM_BEGIN_INTERFACE_MAP(KpgStream)
        XCM_INTERFACE_ENTRY(xd::IStream)
    XCM_END_INTERFACE_MAP()

public:

    KpgStream(KpgDatabase* database);
    virtual ~KpgStream();

    bool init(const std::wstring& path, const std::wstring& mime_type);

    bool read(void* buf,
              unsigned long read_size,
              unsigned long* read_count);
                      
    bool write(const void* buf,
               unsigned long write_size,
               unsigned long* written_count);

    bool seek(long long seek_pos, int whence);

    long long getSize();
    std::wstring getMimeType();
  
private:

    KpgDatabase* m_database;
    PkgStreamReader* m_reader;
    PkgStreamWriter* m_writer;

    std::wstring m_path;
    std::wstring m_mime_type;

    kl::membuf m_membuf;
};



#endif

