/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-23
 *
 */


#ifndef __XDCLIENT_SET_H
#define __XDCLIENT_SET_H


#include <xcm/xcm.h>
#include "../xdcommon/cmnbaseset.h"


class ClientSet : public tango::ISet
{
friend class ClientDatabase;
friend class ClientIterator;
friend class ClientRowInserter;

    XCM_CLASS_NAME("xdclient.Set")
    XCM_BEGIN_INTERFACE_MAP(ClientSet)
        XCM_INTERFACE_ENTRY(tango::ISet)
    XCM_END_INTERFACE_MAP()

public:

    ClientSet();
    virtual ~ClientSet();
};


class ClientInsertData
{
public:
    std::wstring m_col_name;
    int m_tango_type;
    int m_tango_width;
    int m_tango_scale;

    std::wstring m_text;
    bool m_specified;
};


class ClientRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdclient.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(ClientRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()

public:

    ClientRowInserter(ClientDatabase* database,  const std::wstring& path);
    ~ClientRowInserter();

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

    ClientDatabase* m_database;
    tango::IStructurePtr m_structure;

    std::wstring m_path;
    std::wstring m_handle;
    bool m_inserting;
    std::wstring m_rows;
    int m_buffer_row_count;

    std::vector<ClientInsertData> m_insert_data;
};


#endif

