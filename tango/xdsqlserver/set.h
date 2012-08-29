/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
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

    // -- tango::ISet interface --

    void setObjectPath(const std::wstring& obj_path);
    std::wstring getObjectPath();

    unsigned int getSetFlags();
    std::wstring getSetId();

    tango::IStructurePtr getStructure();
    bool modifyStructure(tango::IStructure* struct_config,
                         tango::IJob* job);

    tango::IRowInserterPtr getRowInserter();
    tango::IRowDeleterPtr getRowDeleter();

    int insert(tango::IIteratorPtr source_iter,
               const std::wstring& where_condition,
               int max_rows,
               tango::IJob* job);

    tango::IIndexInfoEnumPtr getIndexEnum();
    tango::IIndexInfoPtr createIndex(const std::wstring& tag,
                                     const std::wstring& expr,
                                     tango::IJob* job);
    tango::IIndexInfoPtr lookupIndex(const std::wstring& expr,
                                     bool exact_column_order);
    bool deleteIndex(const std::wstring& name);
    bool renameIndex(const std::wstring& name,
                     const std::wstring& new_name);

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    tango::rowpos_t getRowCount();

private:

    tango::IDatabasePtr m_database;

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

    int m_tango_type;
    int m_tango_width;
    int m_tango_scale;

    std::string m_text;
};


class SqlServerRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdsqlserver.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(SqlServerRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()

public:

    SqlServerRowInserter(SqlServerSet* set);
    ~SqlServerRowInserter();

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

    TDSSOCKET* m_tds;
    SqlServerSet* m_set;

    char m_insert_stmt[65535];
    std::string m_insert_stub;
    bool m_inserting;

    std::vector<SqlServerInsertData> m_insert_data;
};



#endif

