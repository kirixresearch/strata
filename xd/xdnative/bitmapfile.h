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

    bool getState(unsigned long long offset);
    bool findPrev(unsigned long long* offset, bool state);
    bool findNext(unsigned long long* offset, bool state);

    void startModify();
    void endModify();
    void setState(unsigned long long offset, bool state);

private:

    BitmapFile* m_bmp_file;
    xf_file_t m_file;
    unsigned char* m_buf;
    unsigned long long m_data_offset;
    unsigned long long m_buf_offset;

    bool m_locked;
    bool m_dirty;
    bool m_buf_valid;

    long long m_modify_set_bit_count;

    bool _flush();
    bool _goBlock(unsigned long long block_number,
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

    unsigned long long getSetBitCount();

    BitmapFileScroller* createScroller();

private:

    xf_file_t m_file;
    unsigned long long m_data_offset;
};



#endif
