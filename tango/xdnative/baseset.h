/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
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

    // storing functions
    void setObjectPath(const std::wstring& path) { }
    std::wstring getObjectPath() { return L""; }
    bool isTemporary() { return false; }

    unsigned int getSetFlags();
    void setSetFlags(unsigned int new_val);
    void setSetId(const std::wstring& new_val);
    std::wstring getSetId();

    tango::IRowInserterPtr getRowInserter();
    tango::rowpos_t getRowCount();

    // ISetInternal

    bool addEventHandler(ISetEvents* handler);
    bool removeEventHandler(ISetEvents* handler);


    
    tango::IIndexInfoPtr createIndex(const std::wstring& name,
                                     const std::wstring& expr,
                                     tango::IJob* job)
    {
        return xcm::null;
    }


    bool renameIndex(const std::wstring& name,
                     const std::wstring& new_name)
    {
        return false;
    }


    bool deleteIndex(const std::wstring& name)
    {
        return false;
    }


    tango::IIndexInfoEnumPtr getIndexEnum()
    {
        xcm::IVectorImpl<tango::IIndexInfoEnumPtr>* vec;
        vec = new xcm::IVectorImpl<tango::IIndexInfoEnumPtr>;
        return vec;
    }


    // ISetUpdate
    
    bool updateRow(tango::rowid_t rowid,
                   tango::ColumnUpdateInfo* info,
                   size_t info_size);

    virtual unsigned long long getStructureModifyTime();

protected:

    INodeValuePtr openSetDefinition(bool create_if_not_exist);

    bool modifyStructure(tango::IStructure* struct_config,
                         bool* done);

    bool createCalcField(tango::IColumnInfoPtr params);
    bool deleteCalcField(const std::wstring& name);
    bool modifyCalcField(const std::wstring& name,
                         tango::IColumnInfoPtr params);
    void appendCalcFields(tango::IStructure* structure);

    void onOfsPathChanged(const std::wstring& new_path);
    void onRelationshipsUpdated();


    void fire_onSetDomainUpdated();
    void fire_onSetStructureUpdated();
    void fire_onSetRelationshipsUpdated();
    void fire_onSetRowUpdated(tango::rowid_t rowid);
    void fire_onSetRowDeleted(tango::rowid_t rowid);
    
protected:

    tango::IDatabasePtr m_database;
    IDatabaseInternalPtr m_dbi;
    
private:
    
    xcm::mutex m_event_mutex;         // for m_event_handlers
    xcm::mutex m_structure_mutex;     // for structure mods and m_calc_fields
    xcm::mutex m_setattributes_mutex; // for m_set_id and m_set_flags

    std::wstring m_set_id;
    unsigned int m_set_flags;

    std::vector<ISetEvents*> m_event_handlers;
    std::vector<tango::IColumnInfoPtr> m_calc_fields;
    unsigned long long m_calcrefresh_time;
    bool m_rel_init;
};


#endif
