/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2013-03-01
 *
 */


#ifndef H_XDKPG_INSERTER_H
#define H_XDKPG_INSERTER_H


const int insert_row_count = 500;

struct KpgInsertFieldData
{
    std::wstring m_name;
    int m_xd_type;
    int m_width;
    int m_scale;
    int m_idx;
    int m_offset;

    std::wstring m_str_val;
    int m_int_val;
    double m_dbl_val;
    unsigned char m_bool_val;
    xd::datetime_t m_datetime_val;

    KpgInsertFieldData()
    {
        m_name = L"";
        m_xd_type = xd::typeCharacter;
        m_width = 0;
        m_scale = 0;
        m_idx = 0;
    }
};


class PkgStreamWriter;
class KpgRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdkpg.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(KpgRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    KpgRowInserter(KpgDatabase* db, const std::wstring& table, const xd::Structure& format_definition);
    virtual ~KpgRowInserter();

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

    KpgDatabase* m_database;
    PkgStreamWriter* m_writer;

    std::wstring m_table;

    int m_buf_rows;
    int m_row_width;
    int m_rows_per_buf;
    unsigned char* m_data;

    xd::Structure m_structure;
    std::vector<KpgInsertFieldData> m_fields;
};




#endif



