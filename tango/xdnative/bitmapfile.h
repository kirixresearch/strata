/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-15
 *
 */


#ifndef __XDNATIVE_BITMAPFILE_H
#define __XDNATIVE_BITMAPFILE_H


#include <kl/file.h>


class BitmapFile;


class BitmapFileScroller
{
    friend class BitmapFile;

public:

    BitmapFileScroller();
    BitmapFileScroller(const BitmapFileScroller& c);
    ~BitmapFileScroller();

    bool getState(tango::tango_uint64_t offset);
    bool findPrev(tango::tango_uint64_t* offset, bool state);
    bool findNext(tango::tango_uint64_t* offset, bool state);

    void startModify();
    void endModify();
    void setState(tango::tango_uint64_t offset, bool state);

private:

    BitmapFile* m_bmp_file;
    xf_file_t m_file;
    unsigned char* m_buf;
    tango::tango_uint64_t m_data_offset;
    tango::tango_uint64_t m_buf_offset;

    bool m_locked;
    bool m_dirty;
    bool m_buf_valid;

    tango::tango_int64_t m_modify_set_bit_count;

    bool _flush();
    bool _goBlock(tango::tango_uint64_t block_number,
                  bool lock,
                  bool pad);
};




class BitmapFile
{
public:

    BitmapFile();
    ~BitmapFile();

    bool open(const std::wstring& filename);
    bool close();
    bool isOpen();

    tango::tango_uint64_t getSetBitCount();

    BitmapFileScroller* createScroller();

private:

    xf_file_t m_file;
    tango::tango_uint64_t m_data_offset;
};



#endif
