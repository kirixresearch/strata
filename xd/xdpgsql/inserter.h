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

struct PgsqlInsertFieldData
{
    std::wstring m_name;
    int m_xd_type;
    int m_width;
    int m_scale;
    int m_idx;

    std::wstring m_value;

    PgsqlInsertFieldData()
    {
        m_name = L"";
        m_xd_type = xd::typeCharacter;
        m_width = 0;
        m_scale = 0;
        m_idx = 0;

        m_value = L"";
    }
};


class PgsqlRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdpgsql.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(PgsqlRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    PgsqlRowInserter(PgsqlDatabase* db, const std::wstring& table);
    ~PgsqlRowInserter();

    xd::objhandle_t getHandle(const std::wstring& column_name);

    bool putRawPtr(xd::objhandle_t column_handle, const unsigned char* value, int length);
    bool putString(xd::objhandle_t column_handle, const std::string& value);
    bool putWideString(xd::objhandle_t column_handle, const std::wstring& value);
    bool putDouble(xd::objhandle_t column_handle, double value);
    bool putInteger(xd::objhandle_t column_handle, int value);
    bool putBoolean(xd::objhandle_t column_handle, bool value);
    bool putDateTime(xd::objhandle_t column_handle, xd::datetime_t datetime);
    bool putNull(xd::objhandle_t column_handle);

    bool startInsert(const std::wstring& col_list);
    bool insertRow();
    void finishInsert();

    bool flush();

private:

    PgsqlDatabase* m_database;
    PGconn* m_conn;

    bool m_inserting;
    std::wstring m_table;

    int m_buf_rows;
    int m_insert_rows;
    std::wstring m_wdata;
    char* m_utf8data;
    int m_utf8data_len;
    
    std::wstring m_quote_openchar;
    std::wstring m_quote_closechar;

    std::vector<PgsqlInsertFieldData> m_fields;
};




#endif



