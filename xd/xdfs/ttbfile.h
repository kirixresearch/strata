/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#ifndef __XDFS_TTBTBL_H
#define __XDFS_TTBTBL_H

#include <kl/file.h>

const int ttb_header_len = 1024;
const int ttb_column_descriptor_len = 256;

// in general these constants should be the same as
// xd::max_numeric_width and xd::max_numeric_scale

const int ttb_max_numeric_width = 20;
const int ttb_max_numeric_scale = 12;


char convertType_xd2ttb(int xd_type);
int convertType_ttb2xd(int native_type);



class BitmapFile;
class BitmapFileScroller;



class TtbTable
{
    friend class TtbRowDeleter;
    friend class TtbRowInserter;
    
public:
    static bool create(const std::wstring& filename, xd::IStructure* structure);

public:
    
    TtbTable();
    virtual ~TtbTable();

    bool open(const std::wstring& filename);
    bool isOpen() const { return m_file ? true : false; }
    bool reopen(bool exclusive);
    void close();

    std::wstring getFilename();
    std::wstring getMapFilename();

    int getRowWidth();
    xd::rowpos_t getRowCount(xd::rowpos_t* deleted_row_count = NULL);
    xd::IStructurePtr getStructure();

    xd::rowpos_t findNextRowPos(xd::rowpos_t offset, int delta);
    bool getRow(xd::rowpos_t row, unsigned char* buf);
    int getRows(unsigned char* buf,
                xd::rowpos_t* rowpos_arr,
                int skip,
                xd::rowpos_t start_row,
                int row_count,
                bool direction,
                bool include_deleted);
    int appendRows(unsigned char* buf, int row_count);

    bool isRowDeleted(xd::rowpos_t row);
    bool writeRow(xd::rowpos_t row, unsigned char* buf);
    bool writeColumnInfo(int col,
                         const std::wstring& col_name,
                         int type,
                         int width,
                         int scale);

    void recalcPhysRowCount();
    bool restoreDeleted();

    unsigned long long getStructureModifyTime();
    bool setStructureModified();

private:

    xd::rowpos_t _findNextRowPos(BitmapFileScroller* _bfs,
                                    xd::rowpos_t offset,
                                    int delta);

private:

    xcm::mutex m_object_mutex;

    xd::IStructurePtr m_structure;
    xf_file_t m_file;
    BitmapFile* m_map_file;

    std::wstring m_filename;
    std::wstring m_map_filename;

    xd::rowpos_t m_phys_row_count;     // number of rows in this table
    unsigned int m_row_width;          // row width
    unsigned int m_data_offset;        // offset where the data rows begin
    long long m_modified_time;         // time that an update has occurred
};




class TtbRowDeleter
{

public:

    TtbRowDeleter(TtbTable* table);
    virtual ~TtbRowDeleter();

    void startDelete();
    bool deleteRow(const xd::rowid_t& rowid);
    void finishDelete();
    void cancelDelete() { }

public:

    TtbTable* m_table;
    BitmapFileScroller* m_map_scroller;
    bool m_started;
    int m_deleted_count;
};











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