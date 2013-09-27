/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-03-17
 *
 */


#ifndef __XDSQLSERVER_SET_H
#define __XDSQLSERVER_SET_H


#include "../xdcommon/cmnbaseset.h"


class SqlServerSet : public CommonBaseSet
{
friend class SqlServerDatabase;
friend class SqlServerRowInserter;

    XCM_CLASS_NAME("xdsqlserver.Set")
    XCM_BEGIN_INTERFACE_MAP(SqlServerSet)
        XCM_INTERFACE_ENTRY(CommonBaseSet)
    XCM_END_INTERFACE_MAP()

public:

    std::wstring getSetId();

    xd::IStructurePtr getStructure();

    xd::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& order,
                                       xd::IJob* job);

    xd::rowpos_t getRowCount();

private:

    xd::IDatabasePtr m_database;

    TDSSOCKET* m_tds;
    TDSCONNECTION* m_connect_info;
    TDSCONTEXT* m_context;

    std::wstring m_path;
    std::wstring m_tablename;

    xcm::mutex m_object_mutex;
};




class SqlServerInsertData
{
public:
    std::wstring m_col_name;

    int m_xd_type;
    int m_xd_width;
    int m_xd_scale;

    std::string m_text;
};


class SqlServerRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdsqlserver.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(SqlServerRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()

public:

    SqlServerRowInserter(SqlServerSet* set);
    ~SqlServerRowInserter();

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

    TDSSOCKET* m_tds;
    SqlServerSet* m_set;

    char m_insert_stmt[65535];
    std::string m_insert_stub;
    bool m_inserting;

    std::vector<SqlServerInsertData> m_insert_data;
};



#endif

