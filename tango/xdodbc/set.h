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

    OdbcSet(OdbcDatabase* database);
    ~OdbcSet();

    bool init();

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

    OdbcDatabase* m_database;
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




#endif



