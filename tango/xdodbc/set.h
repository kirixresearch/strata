/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-09-03
 *
 */


#ifndef __XDODBC_SET_H
#define __XDODBC_SET_H


#if _MSC_VER < 1300
#define SQLLEN SQLINTEGER
#define SQLULEN SQLUINTEGER
#endif


#include "../xdcommon/cmnbaseset.h"

xcm_interface IOdbcSet : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdodbc.IOdbcSet")

public:

    virtual void setWhereCondition(const std::wstring& condition) = 0;
};
XCM_DECLARE_SMARTPTR(IOdbcSet)


class OdbcSet : public CommonBaseSet,
                public IOdbcSet
{

friend class OdbcDatabase;
friend class OdbcIterator;
friend class OdbcRowInserter;

    XCM_CLASS_NAME("xdodbc.Set")
    XCM_BEGIN_INTERFACE_MAP(OdbcSet)
        XCM_INTERFACE_ENTRY(IOdbcSet)
        XCM_INTERFACE_ENTRY(CommonBaseSet)
    XCM_END_INTERFACE_MAP()

public:

    OdbcSet();
    ~OdbcSet();
    bool init();

    // tango::ISet interface
    
    void setObjectPath(const std::wstring& path);
    std::wstring getObjectPath();

    unsigned int getSetFlags();
    std::wstring getSetId();
    tango::IStructurePtr getStructure();

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    tango::rowpos_t getRowCount();

    void setWhereCondition(const std::wstring& condition);

private:

    void fixAccessStructure(tango::IStructurePtr& s);

private:

    tango::IDatabasePtr m_database;
    std::wstring m_tablename;
    bool m_filter_query;
        
    tango::rowpos_t m_cached_row_count;
    
    int m_db_type;
    std::wstring m_path;
    std::wstring m_conn_str;
    std::wstring m_set_id;
    std::wstring m_where_condition;

    HENV m_env;
    HDBC m_conn;
    
    tango::IStructurePtr m_structure;
};




const int insert_row_count = 500;

struct OdbcInsertFieldData
{
    std::wstring m_name;
    int m_tango_type;
    SQLSMALLINT m_sql_c_type;
    SQLSMALLINT m_sql_type;
    int m_width;
    int m_scale;
    int m_idx;

    // data
    char* m_str_val;
    wchar_t* m_wstr_val;
    int m_int_val;
    double m_dbl_val;
    unsigned char m_bool_val;
    SQL_NUMERIC_STRUCT m_num_val;
    SQL_DATE_STRUCT m_date_val;
    SQL_TIMESTAMP_STRUCT m_datetime_val;

    SQLLEN m_indicator;

    OdbcInsertFieldData()
    {
        m_name = L"";
        m_tango_type = tango::typeCharacter;
        m_sql_c_type = 0;
        m_sql_type = 0;
        m_width = 0;
        m_scale = 0;
        m_idx = 0;

        m_indicator = 0;
    }
};


class OdbcRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdodbc.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(OdbcRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    OdbcRowInserter(OdbcSet* set);
    ~OdbcRowInserter();

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

    HSTMT m_stmt;
    HDBC m_conn;
    
    OdbcSet* m_set;
    bool m_inserting;

    std::vector<OdbcInsertFieldData*> m_fields;
};




#endif



