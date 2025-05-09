/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-13
 *
 */


#ifndef H_XDPGSQL_STREAM_H
#define H_XDPGSQL_STREAM_H


class PgsqlDatabase;
class PgsqlStream : public xd::IStream
{
    XCM_CLASS_NAME("xdpgsql.PgsqlStream")
    XCM_BEGIN_INTERFACE_MAP(PgsqlStream)
        XCM_INTERFACE_ENTRY(xd::IStream)
    XCM_END_INTERFACE_MAP()

public:

    PgsqlStream(PgsqlDatabase* database);
    virtual ~PgsqlStream();

    bool init(Oid oid, const std::wstring& mime_type = L"application/octet-stream", PGconn* conn = NULL);

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

    PgsqlDatabase* m_database;
    PGconn* m_conn;
    std::wstring m_mime_type;
    int m_fd;
};



#endif

