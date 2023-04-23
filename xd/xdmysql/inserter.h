/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-05-22
 *
 */


#ifndef H_XDMYSQL_SET_H
#define H_XDMYSQL_SET_H

class MysqlDatabase;

class MySqlInsertData
{
public:
    std::wstring m_col_name;
    int m_xd_type;
    int m_xd_width;
    int m_xd_scale;

    std::wstring m_text;
    bool m_specified;
};


class MysqlRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdmysql.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(MysqlRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    MysqlRowInserter(MysqlDatabase* db, const std::wstring& table);
    ~MysqlRowInserter();

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

    void flushRow();

    MysqlDatabase* m_database;
    MYSQL* m_mysql;

    bool m_inserting;
    std::wstring m_table;

    std::string m_asc_insert_stmt;
    std::wstring m_insert_stmt;
    std::wstring m_insert_stub;
    std::wstring m_row;

    std::wstring m_quote_openchar;
    std::wstring m_quote_closechar;

    int m_rows_in_buf;

    std::vector<MySqlInsertData> m_insert_data;
};


#endif
