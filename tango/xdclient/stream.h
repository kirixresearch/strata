/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2012-08-02
 *
 */

#ifndef __XDCLIENT_STREAM_H
#define __XDCLIENT_STREAM_H

class ClientDatabase;

class ClientStream : public tango::IStream
{
    XCM_CLASS_NAME("xdclient.ClientStream")
    XCM_BEGIN_INTERFACE_MAP(ClientStream)
        XCM_INTERFACE_ENTRY(tango::IStream)
    XCM_END_INTERFACE_MAP()

public:

    ClientStream(ClientDatabase* database, const std::wstring& handle);
    ~ClientStream();

    bool read(void* pv,
              unsigned long read_size,
              unsigned long* read_count);
                      
    bool write(const void* pv,
               unsigned long write_size,
               unsigned long* written_count);

private:

    ClientDatabase* m_database;
    std::wstring m_handle;
};



#endif

