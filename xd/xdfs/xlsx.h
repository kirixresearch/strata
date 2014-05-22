/*!
 *
 * Copyright (c) 2014, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2014-05-20
 *
 */


#ifndef __XDFS_XLSX_H
#define __XDFS_XLSX_H


#include <kl/klib.h>
#include "zipfile.h"


// -- XlsxDateTime struct declaration --

struct XlsxDateTime
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millisecond;
    
    XlsxDateTime()
    {
        year = 0;
        month = 0;
        day = 0;
        hour = 0;
        minute = 0;
        second = 0;
        millisecond = 0;
    }

    XlsxDateTime(int _year,
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

    XlsxDateTime(const XlsxDateTime& c)
    {
        year = c.year;
        month = c.month;
        day = c.day;
        hour = c.hour;
        minute = c.minute;
        second = c.second;
        millisecond = c.millisecond;
    }

    XlsxDateTime& operator=(const XlsxDateTime& c)
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


// XlsxField struct declaration

struct XlsxField
{
    std::string name;
    char type;
    int width;
    int scale;
    int offset;
    int ordinal;     // zero-based field index

    XlsxField()
    {
        name = "";
        type = 0;
        width = 0;
        scale = 0;
        offset = 0;
        ordinal = 0;
    }

    XlsxField(const std::string& _name,
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

    XlsxField(const XlsxField& c)
    {
        name = c.name;
        type = c.type;
        width = c.width;
        scale = c.scale;
        offset = c.offset;
        ordinal = c.ordinal;
    }

    XlsxField& operator=(const XlsxField& c)
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


// XlsxFile class declaration

class XlsxFile
{
public:

    XlsxFile();
    ~XlsxFile();

    bool openFile(const std::wstring& filename);
    bool createFile(const std::wstring& filename,
                    const std::vector<XlsxField>& fields);
    bool isOpen();
    void closeFile();

    const std::wstring& getFilename();
    
    size_t getRowCount();
    
    std::vector<XlsxField>& getFields();
    const XlsxField& getFieldInfo(const std::string& name);
    size_t getFieldIdx(const std::string& name);
    size_t getFieldCount();
    
    const std::string& getString(size_t col_idx);
    const XlsxDateTime& getDateTime(size_t col_idx);
    double getDouble(size_t col_idx);
    int getInteger(size_t col_idx);
    bool getBoolean(size_t col_idx);
    bool isNull(size_t col_idx);

    bool putRowBuffer(const unsigned char* value);
    bool putRaw(size_t col_idx, const unsigned char* value, size_t length);
    bool putString(size_t col_idx, const std::string& value);
    bool putDateTime(size_t col_idx, const XlsxDateTime& value);
    bool putDouble(size_t col_idx, double value);
    bool putInteger(size_t col_idx, int value);
    bool putBoolean(size_t col_idx, bool value);
    bool putNull(size_t col_idx);

    bool startInsert();     
    bool finishInsert();    
    bool insertRow();       
    bool writeRow();        

    void goRow(size_t row);

private:

    bool flush();
                            

private:

    ZipFile m_file;
    std::wstring m_filename;

    size_t m_currow_num;            // current row number (starting at 1)
    size_t m_row_count;             // number of rows in this row set
    bool m_inserting;               // flag showing startInsert() was called
    
    std::string m_str_result;       // return value for getString() function
    XlsxDateTime m_date_result;     // return value for getDate() function

    std::vector<XlsxField> m_fields;
};




#endif


