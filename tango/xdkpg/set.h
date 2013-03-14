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

xcm_interface IKpgSet : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdpgsql.IKpgSet")

public:

    virtual void setWhereCondition(const std::wstring& condition) = 0;
};
XCM_DECLARE_SMARTPTR(IKpgSet)


class KpgSet : public CommonBaseSet,
                public IKpgSet
{

friend class KpgDatabase;
friend class KpgIterator;
friend class KpgRowInserter;

    XCM_CLASS_NAME("xdpgsql.Set")
    XCM_BEGIN_INTERFACE_MAP(KpgSet)
        XCM_INTERFACE_ENTRY(IKpgSet)
        XCM_INTERFACE_ENTRY(CommonBaseSet)
    XCM_END_INTERFACE_MAP()

public:

    KpgSet(KpgDatabase* database);
    ~KpgSet();

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

    KpgDatabase* m_database;
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



