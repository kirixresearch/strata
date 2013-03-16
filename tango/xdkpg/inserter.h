/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2013-03-01
 *
 */


#ifndef __XDPGSQL_INSERTER_H
#define __XDPGSQL_INSERTER_H


const int insert_row_count = 500;

struct KpgInsertFieldData
{
    std::wstring m_name;
    int m_tango_type;
    int m_width;
    int m_scale;
    int m_idx;

    std::wstring m_value;

    KpgInsertFieldData()
    {
        m_name = L"";
        m_tango_type = tango::typeCharacter;
        m_width = 0;
        m_scale = 0;
        m_idx = 0;

        m_value = L"";
    }
};


class KpgRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdkpg.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(KpgRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    KpgRowInserter(KpgDatabase* db, const std::wstring& table);
    ~KpgRowInserter();

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

    KpgDatabase* m_database;

    bool m_inserting;
    std::wstring m_table;

    int m_buf_rows;
    int m_insert_rows;
    std::wstring m_wdata;
    char* m_utf8data;
    int m_utf8data_len;
    
    std::wstring m_quote_openchar;
    std::wstring m_quote_closechar;

    std::vector<KpgInsertFieldData> m_fields;
};




#endif



