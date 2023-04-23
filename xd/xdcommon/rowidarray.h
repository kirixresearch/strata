/*!
 *
 * Copyright (c) 2010-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2010-11-15
 *
 */


#ifndef H_XDCOMMON_ROWIDARRAY_H
#define H_XDCOMMON_ROWIDARRAY_H


// a class that provides a large sorted rowid array that can use the disk as storage

class ExVirtualKeyPool;

class RowIdArray
{

public:

    RowIdArray(const std::wstring& temp_dir);
    virtual ~RowIdArray();
    
    void append(xd::rowid_t rowid);
    
    void goFirst();
    void goNext();
    bool isEof();
    xd::rowid_t getItem();

private:

    ExVirtualKeyPool* m_pool;
};


#endif

