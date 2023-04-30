/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#ifndef H_XDFS_FIXEDLENGTH_H
#define H_XDFS_FIXEDLENGTH_H

#include <kl/file.h>
#include <kl/memory.h>



class FixedLengthTable
{
    friend class FixedLengthRowInserter;
    friend class FixedLengthRow;

public:
    
    FixedLengthTable();
    virtual ~FixedLengthTable();

    bool open(const std::wstring& filename, const xd::FormatDefinition& def);
    bool isOpen() const { return m_file ? true : false; }
    bool reopen(bool exclusive = false);
    void close();

    bool getGuid(unsigned char* guid /* 16 bytes */);
    bool setGuid(const unsigned char* guid /* 16 bytes */);

    std::wstring getFilename();

    int getRowWidth();
    xd::rowpos_t getRowCount();
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

    bool writeRow(xd::rowpos_t row, unsigned char* buf);

private:

    kl::mutex m_object_mutex;

    xd::FormatDefinition m_def;

    xf_file_t m_file;
    std::wstring m_filename;

    kl::mutex m_workbuf_mutex;
    kl::membuf m_workbuf;
};





class FixedLengthRow
{
public:

    FixedLengthRow();

    void setTable(FixedLengthTable* table) { m_table = table; }
    void setRowPtr(unsigned char* rowptr) { m_rowptr = rowptr; }
    void clearRow();


    const unsigned char* getRawPtr(int column_ordinal);
    int getRawWidth(int column_ordinal);
    const std::string& getString(int column_ordinal);
    const std::wstring& getWideString(int column_ordinal);
    xd::datetime_t getDateTime(int column_ordinal);
    double getDouble(int column_ordinal);
    int getInteger(int column_ordinal);
    bool getBoolean(int column_ordinal);
    bool isNull(int column_ordinal);


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

    FixedLengthTable* m_table;
    unsigned char* m_rowptr;

    std::string m_str_result;
    std::wstring m_wstr_result;
};



#endif
