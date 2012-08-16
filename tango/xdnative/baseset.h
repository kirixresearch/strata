/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-04-13
 *
 */


#ifndef __XDNATIVE_BASESET_H
#define __XDNATIVE_BASESET_H


#include "xdnative_private.h"


class BaseSet : public tango::ISet,
                public tango::ISetRowUpdate,
                public ISetInternal
{
    XCM_CLASS_NAME("xdnative.BaseSet")
    XCM_BEGIN_INTERFACE_MAP(BaseSet)
        XCM_INTERFACE_ENTRY(tango::ISet)
        XCM_INTERFACE_ENTRY(tango::ISetRowUpdate)
        XCM_INTERFACE_ENTRY(ISetInternal)
    XCM_END_INTERFACE_MAP()
 
public:

    BaseSet(tango::IDatabase* database);
    virtual ~BaseSet();

    // -- storing functions --
    void setObjectPath(const std::wstring& path) { }
    std::wstring getObjectPath() { return L""; }
    bool isTemporary() { return false; }
    
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

    // -- relationship functions --

    tango::IRelationEnumPtr getRelationEnum();
    tango::IRelationPtr createRelation(const std::wstring& tag,
                                       const std::wstring& right_set_path,
                                       const std::wstring& left_expr,
                                       const std::wstring& right_expr);

    tango::IRelationPtr getRelation(const std::wstring& tag);
    int getRelationCount();
    bool deleteRelation(const std::wstring& tag);
    bool deleteAllRelations();

    unsigned int getSetFlags();
    void setSetFlags(unsigned int new_val);
    void setSetId(const std::wstring& new_val);
    std::wstring getSetId();

    tango::IRowInserterPtr getRowInserter();
    tango::IRowDeleterPtr getRowDeleter();

    int insert(tango::IIteratorPtr source_iter,
               const std::wstring& where_condition,
               int max_rows,
               tango::IJob* job);

    tango::rowpos_t getRowCount();

    // -- ISetInternal --

    bool addEventHandler(ISetEvents* handler);
    bool removeEventHandler(ISetEvents* handler);

    // -- ISetUpdate --
    
    bool updateRow(tango::rowid_t rowid,
                   tango::ColumnUpdateInfo* info,
                   size_t info_size);

    virtual tango::tango_uint64_t getStructureModifyTime();

protected:

    tango::INodeValuePtr openSetDefinition(bool create_if_not_exist);

    bool modifyStructure(tango::IStructure* struct_config,
                         bool* done);

    bool createCalcField(tango::IColumnInfoPtr params);
    bool deleteCalcField(const std::wstring& name);
    bool modifyCalcField(const std::wstring& name,
                         tango::IColumnInfoPtr params);
    void appendCalcFields(tango::IStructure* structure);

    void onOfsPathChanged(const std::wstring& new_path);

    void checkRelInit();

    void fire_onSetDomainUpdated();
    void fire_onSetStructureUpdated();
    void fire_onSetRelationshipsUpdated();
    void fire_onSetRowUpdated(tango::rowid_t rowid);
    void fire_onSetRowDeleted(tango::rowid_t rowid);
    
protected:

    tango::IDatabasePtr m_database;
    IDatabaseInternalPtr m_dbi;
    
private:
    
    xcm::mutex m_object_mutex;
    std::wstring m_set_id;
    std::vector<tango::IRelationPtr> m_relations;
    std::vector<ISetEvents*> m_event_handlers;
    std::vector<tango::IColumnInfoPtr> m_calc_fields;
    tango::tango_uint64_t m_calcrefresh_time;
    unsigned int m_set_flags;
    bool m_rel_init;
};





#endif

