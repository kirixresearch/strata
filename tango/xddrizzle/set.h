/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-05-22
 *
 */


#ifndef __XDDRIZZLE_INSERTER_H
#define __XDDRIZZLE_INSERTER_H


#include <xcm/xcm.h>

class DrizzleInsertData
{
public:
    std::wstring m_col_name;
    int m_tango_type;
    int m_tango_width;
    int m_tango_scale;

    std::wstring m_text;
    bool m_specified;
};


class DrizzleDatabase;
class DrizzleRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xddrizzle.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(DrizzleRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    DrizzleRowInserter(DrizzleDatabase* database, const std::wstring& table);
    ~DrizzleRowInserter();

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

    void flushRow();

    DrizzleDatabase* m_database;
    drizzle_st* m_drizzle;
    std::wstring m_table;
    tango::IStructurePtr m_structure;
    
    std::string m_asc_insert_stmt;
    std::wstring m_insert_stmt;
    std::wstring m_insert_stub;

    std::wstring m_quote_openchar;
    std::wstring m_quote_closechar;

    bool m_inserting;

    std::vector<DrizzleInsertData> m_insert_data;
};


#endif
