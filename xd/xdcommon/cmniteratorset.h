/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-11-11
 *
 */


#ifndef H_XDCOMMON_CMNDYNAMICSET_H
#define H_XDCOMMON_CMNDYNAMICSET_H

#include "../xdcommonsql/xdcommonsql.h"



class CommonIteratorSet : public IXdsqlTable
{
    XCM_CLASS_NAME("xdnative.CommonIteratorSet")
    XCM_BEGIN_INTERFACE_MAP(CommonIteratorSet)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
    XCM_END_INTERFACE_MAP()

public:

    CommonIteratorSet();
    virtual ~CommonIteratorSet();

    bool create(xd::IDatabasePtr database);
    void setIterator(xd::IIteratorPtr iter) { m_iter = iter; }

    void setObjectPath(const std::wstring& new_path);
    std::wstring getObjectPath();

    xd::Structure getStructure();
    xd::IIteratorPtr createIterator(const std::wstring& columns,
                                    const std::wstring& expr,
                                    xd::IJob* job);

    bool modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job) { return false; }
    
    IXdsqlRowDeleterPtr getRowDeleter() { return xcm::null; }
    bool restoreDeleted() { return false; }
    bool updateRow(xd::rowid_t rowid, xd::ColumnUpdateInfo* info, size_t info_size) { return false; }

private:

    xd::IDatabasePtr m_database;
    kl::mutex m_object_mutex;
    std::wstring m_path;
    std::wstring m_temp_path;
    xd::IIteratorPtr m_iter;
};



#endif

