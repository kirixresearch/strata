/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2013-03-01
 *
 */


#ifndef __XDPGSQL_SET_H
#define __XDPGSQL_SET_H


#if _MSC_VER < 1300
#define SQLLEN SQLINTEGER
#define SQLULEN SQLUINTEGER
#endif


#include "../xdcommon/cmnbaseset.h"

xcm_interface IPgsqlSet : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdpgsql.IPgsqlSet")

public:

    virtual void setWhereCondition(const std::wstring& condition) = 0;
};
XCM_DECLARE_SMARTPTR(IPgsqlSet)


class PgsqlSet : public CommonBaseSet,
                public IPgsqlSet
{

friend class PgsqlDatabase;
friend class PgsqlIterator;
friend class PgsqlRowInserter;

    XCM_CLASS_NAME("xdpgsql.Set")
    XCM_BEGIN_INTERFACE_MAP(PgsqlSet)
        XCM_INTERFACE_ENTRY(IPgsqlSet)
        XCM_INTERFACE_ENTRY(CommonBaseSet)
    XCM_END_INTERFACE_MAP()

public:

    PgsqlSet(PgsqlDatabase* database);
    ~PgsqlSet();

    bool init();

    void setObjectPath(const std::wstring& path);
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

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    tango::rowpos_t getRowCount();

    void setWhereCondition(const std::wstring& condition);

private:

    PgsqlDatabase* m_database;
    std::wstring m_tablename;
    bool m_filter_query;
        
    tango::rowpos_t m_cached_row_count;
    
    int m_db_type;
    std::wstring m_path;
    std::wstring m_conn_str;
    std::wstring m_set_id;
    std::wstring m_where_condition;

    tango::IStructurePtr m_structure;
};




const int insert_row_count = 500;

struct PgsqlInsertFieldData
{
    std::wstring m_name;
    int m_tango_type;
    int m_width;
    int m_scale;
    int m_idx;

    // data
    char* m_str_val;
    wchar_t* m_wstr_val;
    int m_int_val;
    double m_dbl_val;
    unsigned char m_bool_val;

    int m_indicator;

    PgsqlInsertFieldData()
    {
        m_name = L"";
        m_tango_type = tango::typeCharacter;
        m_width = 0;
        m_scale = 0;
        m_idx = 0;

        m_indicator = 0;
    }
};


class PgsqlRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdpgsql.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(PgsqlRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    PgsqlRowInserter(PgsqlSet* set);
    ~PgsqlRowInserter();

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

    PgsqlSet* m_set;
    bool m_inserting;

    std::vector<PgsqlInsertFieldData*> m_fields;
};




#endif



