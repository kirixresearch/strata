/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-23
 *
 */


#ifndef __XDCLIENT_INSERTER_H
#define __XDCLIENT_INSERTER_H


#include <xcm/xcm.h>
#include "../xdcommon/cmnbaseset.h"



class ClientInsertData
{
public:
    std::wstring m_col_name;
    int m_xd_type;
    int m_xd_width;
    int m_xd_scale;

    std::wstring m_text;
    bool m_specified;
};


class ClientRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdclient.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(ClientRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()

public:

    ClientRowInserter(ClientDatabase* database,  const std::wstring& path);
    ~ClientRowInserter();

    xd::objhandle_t getHandle(const std::wstring& column_name);
    xd::IColumnInfoPtr getInfo(xd::objhandle_t column_handle);

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

    ClientDatabase* m_database;
    xd::IStructurePtr m_structure;

    std::wstring m_path;
    std::wstring m_handle;
    std::wstring m_columns;
    bool m_inserting;
    std::wstring m_rows;
    int m_buffer_row_count;

    std::vector<ClientInsertData> m_insert_data;
};


#endif

