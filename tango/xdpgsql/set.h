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




#endif



