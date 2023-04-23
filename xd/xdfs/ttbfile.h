/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#ifndef H_XDFS_TTBTBL_H
#define H_XDFS_TTBTBL_H

#include <kl/file.h>
#include <kl/memory.h>


const int ttb_header_len = 1024;
const int ttb_column_descriptor_len = 256;

// in general these constants should be the same as
// xd::max_numeric_width and xd::max_numeric_scale

const int ttb_max_numeric_width = 20;
const int ttb_max_numeric_scale = 12;


char convertType_xd2ttb(int xd_type);
int convertType_ttb2xd(int native_type);




class TtbField
{
public:

    std::wstring name;
    char ttb_type;
    int width;
    int scale;
    int offset;
    int ordinal;
    bool nulls_allowed;
};

class TtbTable
{
    friend class TtbRowInserter;
    friend class TtbRow;

public:

    enum
    {
        typeCharacter =     'C',
        typeWideCharacter = 'W',
        typeNumeric =       'N',
        typeDouble =        'B',
        typeDate =          'D',
        typeDateTime =      'T',
        typeBoolean =       'L',
        typeInteger =       'I'
    };

public:
    static bool create(const std::wstring& filename, const std::vector<xd::ColumnInfo>& structure);

public:
    
    TtbTable();
    virtual ~TtbTable();

    bool open(const std::wstring& filename);
    bool isOpen() const { return m_file ? true : false; }
    bool reopen(bool exclusive = false);
    void close();

    bool getGuid(unsigned char* guid /* 16 bytes */);
    bool setGuid(const unsigned char* guid /* 16 bytes */);

    std::wstring getFilename();

    int getRowWidth();
    xd::rowpos_t getRowCount(xd::rowpos_t* deleted_row_count = NULL);
    xd::Structure getStructure();

    xd::rowpos_t findNextRowPos(xd::rowpos_t offset, int delta);
    bool getRow(xd::rowpos_t row, unsigned char* buf);
    int getRows(unsigned char* buf,
                xd::rowpos_t* rowpos_arr,
                int skip,
                xd::rowpos_t start_row,
                int row_count,
                bool include_deleted);
    int appendRows(unsigned char* buf, int row_count);

    bool deleteRow(xd::rowid_t rowid);

    bool writeRow(xd::rowpos_t row, unsigned char* buf);
    bool writeColumnInfo(int col,
                         const std::wstring& col_name,
                         int type,
                         int width,
                         int scale);

    xd::rowpos_t recalcPhysRowCount();
    bool restoreDeleted();

    unsigned long long getStructureModifyTime();
    bool setStructureModified();

private:

    static void generateGuid(unsigned char* guid);
    void updateHeaderWithGuid();

private:

    kl::mutex m_object_mutex;

    std::vector<TtbField> m_fields;

    xf_file_t m_file;
    std::wstring m_filename;

    xd::rowpos_t m_phys_row_count;     // number of rows in this table
    unsigned int m_row_width;          // row width
    unsigned int m_data_offset;        // offset where the data rows begin
    long long m_modified_time;         // time that an update has occurred

    unsigned char m_guid[16];

    kl::mutex m_workbuf_mutex;
    kl::membuf m_workbuf;
};





class TtbRow
{
public:

    TtbRow();

    void setTable(TtbTable* table) { m_table = table; }
    void setRowPtr(unsigned char* rowptr) { m_rowptr = rowptr; }
    void clearRow();

    bool putRawPtr(int column_ordinal,
                   const unsigned char* value,
                   int length);

    bool putString(int column_ordinal,
                   const std::string& value);

    bool putWideString(int column_ordinal,
                       const std::wstring& value);

    bool putDouble(int column_ordinal,
                   double value);

    bool putInteger(int column_ordinal,
                    int value);

    bool putBoolean(int column_ordinal,
                    bool value);

    bool putDateTime(int column_ordinal,
                     xd::datetime_t value);

    bool putNull(int column_ordinal);

private:

    TtbTable* m_table;
    unsigned char* m_rowptr;
};



#endif
