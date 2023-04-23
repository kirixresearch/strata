/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-19
 *
 */


#ifndef H_XDFS_XBASE_H
#define H_XDFS_XBASE_H


#include <xd/xd.h>


// -- XbaseDate struct declaration --

struct XbaseDate
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millisecond;
    
    XbaseDate()
    {
        year = 0;
        month = 0;
        day = 0;
        hour = 0;
        minute = 0;
        second = 0;
        millisecond = 0;
    }

    XbaseDate(int _year,
              int _month,
              int _day,
              int _hour = 0,
              int _minute = 0,
              int _second = 0,
              int _millisecond = 0)
    {
        year = _year;
        month = _month;
        day = _day;
        hour = _hour;
        minute = _minute;
        second = _second;
        millisecond = _millisecond;
    }

    XbaseDate(const XbaseDate& c)
    {
        year = c.year;
        month = c.month;
        day = c.day;
        hour = c.hour;
        minute = c.minute;
        second = c.second;
        millisecond = c.millisecond;
    }

    XbaseDate& operator=(const XbaseDate& c)
    {
        year = c.year;
        month = c.month;
        day = c.day;
        hour = c.hour;
        minute = c.minute;
        second = c.second;
        millisecond = c.millisecond;
        return *this;
    }

    bool isNull() const { return (year == 0 && month == 0 && day == 0 &&
                                  hour == 0 && minute == 0 && second == 0 &&
                                  millisecond == 0) ? true : false; }
};


// XbaseField struct declaration

struct XbaseField
{
    std::string name;
    char type;
    int width;
    int scale;
    int offset;
    int ordinal;     // zero-based field index

    XbaseField()
    {
        name = "";
        type = 0;
        width = 0;
        scale = 0;
        offset = 0;
        ordinal = 0;
    }

    XbaseField(const std::string& _name,
               char _type,
               int _width,
               int _scale,
               int _offset,
               int _ordinal)
    {
        name = _name;
        type = _type;
        width = _width;
        scale = _scale;
        offset = _offset;
        ordinal = _ordinal;
    }

    XbaseField(const XbaseField& c)
    {
        name = c.name;
        type = c.type;
        width = c.width;
        scale = c.scale;
        offset = c.offset;
        ordinal = c.ordinal;
    }

    XbaseField& operator=(const XbaseField& c)
    {
        name = c.name;
        type = c.type;
        width = c.width;
        scale = c.scale;
        offset = c.offset;
        ordinal = c.ordinal;
        return *this;
    }
    
    bool isOk() const { return name.empty(); }
};


// XbaseFile class declaration

class XbaseFile
{
public:

    XbaseFile();
    ~XbaseFile();

    bool open(const std::wstring& filename);
    bool open(xd::IStream* stream);
    bool create(const std::wstring& filename, const std::vector<XbaseField>& fields);
    bool isOpen();
    void close();

    size_t getHeaderLength();
    size_t getRowWidth();
    size_t getRowCount();
    
    std::vector<XbaseField>& getFields();
    const XbaseField& getFieldInfo(const std::string& name);
    size_t getFieldIdx(const std::string& name);
    size_t getFieldCount();
    
    unsigned char* getRowBuffer();
    unsigned char* getRaw(size_t col_idx);
    const std::string& getString(size_t col_idx);
    const XbaseDate& getDateTime(size_t col_idx);
    double getDouble(size_t col_idx);
    int getInteger(size_t col_idx);
    bool getBoolean(size_t col_idx);
    bool isNull(size_t col_idx);

    bool putRowBuffer(const unsigned char* value);
    bool putRaw(size_t col_idx, const unsigned char* value, size_t length);
    bool putString(size_t col_idx, const std::string& value);
    bool putDateTime(size_t col_idx, const XbaseDate& value);
    bool putDouble(size_t col_idx, double value);
    bool putInteger(size_t col_idx, int value);
    bool putBoolean(size_t col_idx, bool value);
    bool putNull(size_t col_idx);

    bool startInsert();     // -- these functions are used for bulk-inserting
    bool finishInsert();    //    rows at the end of a file (most likely for
    bool insertRow();       //    file creation).  startInsert() must be called
                            //    before insertRow() can be called. Also,
                            //    insertRow() should be called after all of the
                            //    puts (putString(), etc.) have been done --

    bool writeRow();        // -- this function is only used to
                            //    write to an existing record --

    void goRow(size_t row);
    void setReadAhead(bool read_ahead);

private:

    bool flush();           // -- this function ensures any rows left in our
                            //    buffer get written to the file --

private:

    xd::IStream* m_stream;

    unsigned char* m_buf;           // buffer for rows
    size_t m_buf_rows;              // number of rows in m_buf
    size_t m_buf_firstrow;          // number of the first row in m_buf
    size_t m_buf_maxrows;           // number of rows that fit in m_buf

    unsigned char* m_currow_ptr;    // current row pointer
    size_t m_currow_num;            // current row number (starting at 1)

    size_t m_header_len;            // header length
    size_t m_row_count;             // number of rows in this rowset
    size_t m_row_width;             // width of each row

    size_t m_read_ahead;            // number of rows to read ahead
    bool m_inserting;               // flag showing startInsert() was called
    
    std::string m_str_result;       // return value for getString() function
    XbaseDate m_date_result;        // return value for getDate() function

    std::vector<XbaseField> m_fields;
};




#endif


