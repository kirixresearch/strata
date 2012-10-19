/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-21
 *
 */


#ifndef __XDCOMMON_LOCALROWCACHE_H
#define __XDCOMMON_LOCALROWCACHE_H


#include <map>
#include <kl/file.h>


class LocalRow
{
    friend class LocalRowCache;

public:

    LocalRow();
    ~LocalRow();
 
    unsigned char* getColumnData(int col_idx, unsigned int* size, bool* is_null);
    size_t getColumnCount() const;

private:

    void reserveBuffer(unsigned int size);

private:

    std::vector<unsigned char*> m_col_ptrs;
    unsigned char* m_buf;
    unsigned int m_buf_alloc_size;
};




class LocalRowUpdates
{
public:

    LocalRowUpdates() { }
    
    ~LocalRowUpdates()
    {
        std::map<int, unsigned char*>::iterator it;
        for (it = m_data.begin(); it != m_data.end(); ++it)
            delete[] it->second;
    }
    
public:
    std::map<int, unsigned char*> m_data;
};




class LocalRowCache
{
public:

    LocalRowCache();
    ~LocalRowCache();

    bool init();
    bool isOk() const;
    
    void goFirst();
    void goNext();
    void goPrev();
    void goRow(tango::rowpos_t row);
    bool eof();
    
    bool getRow(LocalRow& row);
    tango::rowpos_t getRowCount();


    // -- writing/appending functions --
    void createRow();
    void appendColumnData(unsigned char* data, unsigned int size);
    void appendNullColumn();
    void finishRow();
    void reserveAppendBuffer(unsigned int row_width);
    
    // -- updating functions --
    void updateValue(tango::rowpos_t row, int col, unsigned char* data, unsigned int size);
    void clearUpdateValues();
    unsigned char* getUpdateValue(int col);

private:

    void copyDataToBuffer(unsigned char* dest, const unsigned char* src, unsigned int size);
    
private:

    xf_file_t m_f;
    std::wstring m_filename;
    tango::rowpos_t m_row_count;
    tango::rowpos_t m_curpos;
    
    // -- reading variables --
    unsigned int m_read_reclen;
    tango::rowpos_t m_read_currow;
    bool m_eof;
    xf_off_t m_offset;

    // -- writing variables --
    unsigned char* m_append_data;
    unsigned char* m_append_colptr;
    unsigned int m_append_cursize;
    unsigned int m_append_allocsize;
    unsigned int m_append_rownum;
    
    std::map<tango::rowpos_t, LocalRowUpdates*> m_updates;
};


#endif
