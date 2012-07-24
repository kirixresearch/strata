/*!
 *
 * Copyright (c) 2010-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2010-11-19
 *
 */


#ifndef __XDCOMMON_CMNFILTERSET_H
#define __XDCOMMON_CMNFILTERSET_H


class CommonFilterSetIndexEntry
{
public:
    std::wstring filename;
    std::wstring expr;
    IIndex* index;

    KeyLayout* key_expr;
    int key_length;
};




class CommonFilterSet : public tango::ISet,
                        public tango::IDynamicSet,
                        public tango::ISetRowUpdate
{
friend class CommonFilterSetRowDeleter;

    XCM_CLASS_NAME("xdnative.CommonFilterSet")
    XCM_BEGIN_INTERFACE_MAP(CommonFilterSet)
        XCM_INTERFACE_ENTRY(tango::ISet)
        XCM_INTERFACE_ENTRY(tango::IDynamicSet)
        XCM_INTERFACE_ENTRY(tango::ISetRowUpdate)
    XCM_END_INTERFACE_MAP()

public:

    CommonFilterSet();
    virtual ~CommonFilterSet();

    bool create(tango::IDatabasePtr database,
                tango::ISetPtr base_set);

    void setObjectPath(const std::wstring& new_path);
    std::wstring getObjectPath();
    bool isTemporary();
    bool storeObject(const std::wstring& ofs_path);

    // -- ICommonFilterSet --
    tango::ISetPtr getBaseSet();
    bool insertRow(const tango::rowid_t& rowid);
    bool deleteRow(const tango::rowid_t& rowid);

    // -- ISet --
    std::wstring getSetId();

    unsigned int getSetFlags();
    tango::IStructurePtr getStructure();
    bool modifyStructure(tango::IStructure* struct_config, 
                         tango::IJob* job);

    tango::IRelationPtr getRelation(const std::wstring& rel_tag);
    int getRelationCount();
    tango::IRelationEnumPtr getRelationEnum();
    bool deleteRelation(const std::wstring& tag);
    bool deleteAllRelations();
    tango::IRelationPtr createRelation(const std::wstring& tag,
                                       const std::wstring& right_set_path,
                                       const std::wstring& left_expr,
                                       const std::wstring& right_expr);

    tango::IRowInserterPtr getRowInserter();
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
    
    // -- index functions --
    tango::IIndexInfoEnumPtr getIndexEnum();
    tango::IIndexInfoPtr createIndex(const std::wstring& name,
                                     const std::wstring& expr,
                                     tango::IJob* job);

    tango::IIndexInfoPtr lookupIndex(const std::wstring& expr,
                                     bool exact_column_order);

    bool deleteIndex(const std::wstring& name);
    bool renameIndex(const std::wstring& name,
                     const std::wstring& new_name);
                     
    // -- ISetEvents --
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
    std::vector<CommonFilterSetIndexEntry> m_indexes;
    tango::ISetPtr m_base_set;
    tango::IIteratorPtr m_base_set_iter;
    tango::ISetRowUpdatePtr m_base_set_update;
    tango::rowpos_t m_row_count;
    std::wstring m_set_id;

    IIndex* m_index;
    bool m_temporary;
};




class CommonFilterSetRowDeleter : public tango::IRowDeleter
{
    XCM_CLASS_NAME("xdnative.CommonFilterSetRowDeleter")
    XCM_BEGIN_INTERFACE_MAP(CommonFilterSetRowDeleter)
        XCM_INTERFACE_ENTRY(tango::IRowDeleter)
    XCM_END_INTERFACE_MAP()

public:

    CommonFilterSetRowDeleter(CommonFilterSet* set);
    ~CommonFilterSetRowDeleter();

    void startDelete();
    bool deleteRow(const tango::rowid_t& rowid);
    void finishDelete();
    void cancelDelete();
    
private:

    CommonFilterSet* m_set;
    IIndex* m_index;
    std::vector<tango::rowid_t> m_rows_to_delete;
};






#endif

