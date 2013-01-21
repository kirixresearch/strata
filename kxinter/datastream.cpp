/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#include "kxinter.h"
#include "datastream.h"


class FileDataStream : public IDataStream
{
    XCM_CLASS_NAME("kxinter.FileDataStream")
    XCM_BEGIN_INTERFACE_MAP(FileDataStream)
        XCM_INTERFACE_ENTRY(IDataStream)
    XCM_END_INTERFACE_MAP()

    xf_file_t m_file;
    wxString m_filename;
    xf_off_t m_stream_size;

public:

    FileDataStream()
    {
        m_file = 0;
        m_stream_size = 0;
    }

    virtual ~FileDataStream()
    {
        if (m_file)
        {
            close();
        }
    }

    bool open(const wxString& filename)
    {
        m_stream_size = xf_get_file_size(towstr(filename));
        m_file = xf_open(towstr(filename), xfOpen, xfRead, xfShareReadWrite);
        if (!m_file)
            return false;

        m_filename = filename;

        xf_seek(m_file, 0, xfSeekSet);

        return true;
    }

    void close()
    {
        if (m_file)
        {
            xf_close(m_file);
            m_file = 0;
        }
    }

    IDataStreamPtr clone()
    {
        FileDataStream* s = new FileDataStream;
        if (!s->open(m_filename))
        {
            delete s;
            return xcm::null;
        }

        return static_cast<IDataStream*>(s);
    }

    virtual int getData(unsigned char* buf, int read_len)
    {
        if (m_file == 0)
            return 0;

        return xf_read(m_file, buf, 1, read_len);
    }

    virtual ds_off_t getPosition()
    {
        if (m_file == 0)
            return 0;

        return xf_get_file_pos(m_file);
    }

    virtual bool setPosition(const ds_off_t& offset)
    {
        if (m_file == 0)
            return false;

        if (!xf_seek(m_file, offset, xfSeekSet))
            return false;

        return true;
    }

    ds_off_t getStreamSize()
    {
        return m_stream_size;
    }
};




IDataStreamPtr openFileStream(const wxString& filename)
{
    FileDataStream* f = new FileDataStream;
    if (!f->open(filename))
    {
        delete f;
        return xcm::null;
    }

    return static_cast<IDataStream*>(f);
}




