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

    bool create(xd::IDatabasePtr database,
                const std::wstring& base_path);

    bool create(xd::IDatabasePtr database,
                IXdsqlTablePtr base_table);

    void setObjectPath(const std::wstring& new_path);
    std::wstring getObjectPath();

    void startBulkInsert();
    void finishBulkInsert();
    bool insertRow(const xd::rowid_t& rowid);
    bool deleteRow(const xd::rowid_t& rowid);
    
    std::wstring getSetId();

    xd::Structure getStructure();
    xd::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       xd::IJob* job);

    xd::IIteratorPtr getRow(xd::rowid_t rowid);
    xd::rowpos_t getRowCount();

    IXdsqlRowDeleterPtr getRowDeleter();
    bool restoreDeleted() { return false; }

    int insert(xd::IIteratorPtr source_iter,
               const std::wstring& where_condition,
               int max_rows,
               xd::IJob* job);

    bool updateRow(xd::rowid_t rowid,
                   xd::ColumnUpdateInfo* info,
                   size_t info_size);

    bool modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job) { return false; }
    
private:

    kl::mutex m_object_mutex;
    xd::IDatabasePtr m_database;

    kl::mutex m_iters_mutex;
    std::vector<CommonDynamicIterator*> m_iters;

    std::wstring m_path;
    std::wstring m_filename;
    std::wstring m_temp_path;
    std::vector<CommonDynamicSetIndexEntry> m_indexes;
    IXdsqlTablePtr m_base_table;
    xd::IIteratorPtr m_base_iter;
    xd::rowpos_t m_row_count;
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
    bool deleteRow(const xd::rowid_t& rowid);
    void finishDelete();
    void cancelDelete();
    
private:

    CommonDynamicSet* m_set;
    IIndex* m_index;
    std::vector<xd::rowid_t> m_rows_to_delete;
};






#endif

