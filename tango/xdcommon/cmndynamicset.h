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


class CommonDynamicSetIndexEntry
{
public:
    std::wstring filename;
    std::wstring expr;
    IIndex* index;

    KeyLayout* key_expr;
    int key_length;
};




class CommonDynamicSet : public tango::ISet,
                         public tango::ISetRowUpdate
{
friend class CommonDynamicSetRowDeleter;

    XCM_CLASS_NAME("xdnative.CommonDynamicSet")
    XCM_BEGIN_INTERFACE_MAP(CommonDynamicSet)
        XCM_INTERFACE_ENTRY(tango::ISet)
        XCM_INTERFACE_ENTRY(tango::ISetRowUpdate)
    XCM_END_INTERFACE_MAP()

public:

    CommonDynamicSet();
    virtual ~CommonDynamicSet();

    bool create(tango::IDatabasePtr database,
                tango::ISetPtr base_set);

    void setObjectPath(const std::wstring& new_path);
    std::wstring getObjectPath();
    bool isTemporary();


    tango::ISetPtr getBaseSet();
    void startBulkInsert();
    void finishBulkInsert();
    bool insertRow(const tango::rowid_t& rowid);
    bool deleteRow(const tango::rowid_t& rowid);
    
    // ISet
    std::wstring getSetId();

    unsigned int getSetFlags();
    tango::IStructurePtr getStructure();
    bool modifyStructure(tango::IStructure* struct_config, 
                         tango::IJob* job);

    tango::IRowDeleterPtr getRowDeleter();
    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);
    tango::IIteratorPtr getRow(tango::rowid_t rowid);
    tango::rowpos_t getRowCount();

    int insert(tango::IIteratorPtr source_iter,
               const std::wstring& where_condition,
               int max_rows,
               tango::IJob* job);

    bool updateRow(tango::rowid_t rowid,
                   tango::ColumnUpdateInfo* info,
                   size_t info_size);
                     
    // ISetEvents
    void onSetDomainUpdated();
    void onSetStructureUpdated();
    void onSetRelationshipsUpdated();
    void onSetRowUpdated(tango::rowid_t rowid);
    void onSetRowDeleted(tango::rowid_t rowid);

    void onOfsPathChanged(const std::wstring& new_path);

private:

    xcm::mutex m_object_mutex;
    tango::IDatabasePtr m_database;
    std::wstring m_ofspath;
    std::wstring m_filename;
    std::wstring m_temp_path;
    std::vector<CommonDynamicSetIndexEntry> m_indexes;
    tango::ISetPtr m_base_set;
    tango::IIteratorPtr m_base_set_iter;
    tango::ISetRowUpdatePtr m_base_set_update;
    tango::rowpos_t m_row_count;
    std::wstring m_set_id;

    IIndex* m_index;
    bool m_temporary;
};




class CommonDynamicSetRowDeleter : public tango::IRowDeleter
{
    XCM_CLASS_NAME("xdnative.CommonDynamicSetRowDeleter")
    XCM_BEGIN_INTERFACE_MAP(CommonDynamicSetRowDeleter)
        XCM_INTERFACE_ENTRY(tango::IRowDeleter)
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

