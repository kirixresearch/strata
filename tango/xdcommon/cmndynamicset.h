/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-11-11
 *
 */


#ifndef __XDCOMMON_CMNDYNAMICSET_H
#define __XDCOMMON_CMNDYNAMICSET_H

#include "../xdcommonsql/xdcommonsql.h"


class IIndex;
class KeyLayout;

class CommonDynamicSetIndexEntry
{
public:
    std::wstring filename;
    std::wstring expr;
    IIndex* index;

    KeyLayout* key_expr;
    int key_length;
};



class CommonDynamicIterator;
class CommonDynamicSet : public IXdsqlTable
{
friend class CommonDynamicIterator;
friend class CommonDynamicSetRowDeleter;

    XCM_CLASS_NAME("xdnative.CommonDynamicSet")
    XCM_BEGIN_INTERFACE_MAP(CommonDynamicSet)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
    XCM_END_INTERFACE_MAP()

public:

    CommonDynamicSet();
    virtual ~CommonDynamicSet();

    bool create(tango::IDatabasePtr database,
                const std::wstring& base_path);

    void setObjectPath(const std::wstring& new_path);
    std::wstring getObjectPath();

    void startBulkInsert();
    void finishBulkInsert();
    bool insertRow(const tango::rowid_t& rowid);
    bool deleteRow(const tango::rowid_t& rowid);
    
    std::wstring getSetId();

    tango::IStructurePtr getStructure();
    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    tango::IIteratorPtr getRow(tango::rowid_t rowid);
    tango::rowpos_t getRowCount();

    IXdsqlRowDeleterPtr getRowDeleter();
    bool restoreDeleted() { return false; }

    int insert(tango::IIteratorPtr source_iter,
               const std::wstring& where_condition,
               int max_rows,
               tango::IJob* job);

    bool updateRow(tango::rowid_t rowid,
                   tango::ColumnUpdateInfo* info,
                   size_t info_size);
    
private:

    xcm::mutex m_object_mutex;
    tango::IDatabasePtr m_database;

    xcm::mutex m_iters_mutex;
    std::vector<CommonDynamicIterator*> m_iters;

    std::wstring m_path;
    std::wstring m_filename;
    std::wstring m_temp_path;
    std::vector<CommonDynamicSetIndexEntry> m_indexes;
    std::wstring m_base_path;
    IXdsqlTablePtr m_base_table;
    tango::IIteratorPtr m_base_iter;
    tango::rowpos_t m_row_count;
    std::wstring m_set_id;

    IIndex* m_index;
    bool m_temporary;
};




class CommonDynamicSetRowDeleter : public IXdsqlRowDeleter
{
    XCM_CLASS_NAME("xdnative.CommonDynamicSetRowDeleter")
    XCM_BEGIN_INTERFACE_MAP(CommonDynamicSetRowDeleter)
        XCM_INTERFACE_ENTRY(IXdsqlRowDeleter)
    XCM_END_INTERFACE_MAP()

public:

    CommonDynamicSetRowDeleter(CommonDynamicSet* set);
    ~CommonDynamicSetRowDeleter();

    void startDelete();
    bool deleteRow(const tango::rowid_t& rowid);
    void finishDelete();
    void cancelDelete();
    
private:

    CommonDynamicSet* m_set;
    IIndex* m_index;
    std::vector<tango::rowid_t> m_rows_to_delete;
};






#endif

