/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2007-01-31
 *
 */


#ifndef H_XDCOMMON_FILESTREAM_H
#define H_XDCOMMON_FILESTREAM_H


#include <kl/file.h>


class FileStream : public xd::IStream
{
    XCM_CLASS_NAME("xd.FileStream")
    XCM_BEGIN_INTERFACE_MAP(FileStream)
        XCM_INTERFACE_ENTRY(xd::IStream)
    XCM_END_INTERFACE_MAP()

public:

    FileStream();
    virtual ~FileStream();

    bool create(const std::wstring& filename);
    bool open(const std::wstring& filename);

    bool read(void* buf,
              unsigned long read_size,
              unsigned long* read_count);
                      
    bool write(const void* buf,
              unsigned long write_size,
              unsigned long* written_count);
        
    bool seek(long long seek_pos, int whence = xd::seekSet);

    long long getSize();
    std::wstring getMimeType();
        
private:

    xf_file_t m_file;
    std::wstring m_mime_type;
    bool m_read_only;
};


#endif
