/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#ifndef __XDSQLITE_SET_H
#define __XDSQLITE_SET_H


class SlRowInserterData
{
public:

    std::wstring name;
    int idx;
    int type;
    int length;
    char* str_data;
    size_t buf_len;
    tango::IColumnInfoPtr colinfo;
};


class SlRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdsl.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(SlRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()

public:

    SlRowInserter(SlDatabase* database, const std::wstring& table);
    ~SlRowInserter();

    tango::objhandle_t getHandle(const std::wstring& column_name);
    tango::IColumnInfoPtr getInfo(tango::objhandle_t column_handle);

    bool putRawPtr(tango::objhandle_t column_handle, const unsigned char* value, int length);
    bool putString(tango::objhandle_t column_handle, const std::string& value);
    bool putWideString(tango::objhandle_t column_handle, const std::wstring& value);
    bool putDouble(tango::objhandle_t column_handle, double value);
    bool putInteger(tango::objhandle_t column_handle, int value);
    bool putBoolean(tango::objhandle_t column_handle, bool value);
    bool putDateTime(tango::objhandle_t column_handle, tango::datetime_t datetime);
    bool putNull(tango::objhandle_t column_handle);

    bool startInsert(const std::wstring& col_list);
    bool insertRow();
    void finishInsert();

    bool flush();

private:

    SlDatabase* m_database;
    sqlite3_stmt* m_stmt;
    bool m_inserting;
    std::wstring m_table;

    std::vector<SlRowInserterData> m_fields;
};




#endif



