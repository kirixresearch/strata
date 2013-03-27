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
#include <kl/xml.h>


class KpgSet : public CommonBaseSet
{

friend class KpgDatabase;
friend class KpgIterator;
friend class KpgRowInserter;

    XCM_CLASS_NAME("xdkpg.Set")
    XCM_BEGIN_INTERFACE_MAP(KpgSet)
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

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    tango::rowpos_t getRowCount();

private:

    KpgDatabase* m_database;
    std::wstring m_tablename;
    std::wstring m_path;
    std::wstring m_set_id;
    bool m_creating;

    kl::xmlnode m_info;

    tango::IStructurePtr m_structure;
};


#endif
