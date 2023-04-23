/*!
 *
 * Copyright (c) 2007-2022, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2023-02-19
 *
 */


#ifndef H_XDSQLITE_STREAM_H
#define H_XDSQLITE_STREAM_H


class SlDatabase;
class SlStream : public xd::IStream
{
    XCM_CLASS_NAME("xdsqlite.SlStream")
    XCM_BEGIN_INTERFACE_MAP(SlStream)
        XCM_INTERFACE_ENTRY(xd::IStream)
    XCM_END_INTERFACE_MAP()

public:

    static const int DEFAULT_BLOCK_SIZE = 4096;

public:

    SlStream(SlDatabase* database);
    ~SlStream();

    bool init(const std::wstring& stream_object_name,
              const std::wstring& mime_type = L"application/octet-stream",
              int block_size = SlStream::DEFAULT_BLOCK_SIZE);

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

    bool readBlock(long long block, void* buf, unsigned long* read_count);
    bool writeBlock(long long block, void* buf, unsigned long size);

private:

    SlDatabase* m_database;
    std::wstring m_mime_type;
    std::string m_stream;
    long long m_offset;
    int m_block_size;
};



#endif

