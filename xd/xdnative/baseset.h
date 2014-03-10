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


#include "../xdcommon/xd_private.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "xdnative_private.h"
#include "database.h"

class XdnativeDatabase;
class BaseSet : public IXdnativeSet,
                public IXdsqlTable
{
    XCM_CLASS_NAME("xdnative.BaseSet")
    XCM_BEGIN_INTERFACE_MAP(BaseSet)
        XCM_INTERFACE_ENTRY(IXdnativeSet)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
    XCM_END_INTERFACE_MAP()
 
public:

    BaseSet(XdnativeDatabase* database);
    virtual ~BaseSet();

    void setTemporary(bool value);

    IXdnativeSet* getRawXdnativeSetPtr() { return this; }

    bool modifyStructure(xd::IStructurePtr struct_config, xd::IJob* job) { return false; }

    unsigned int getSetFlags();
    void setSetFlags(unsigned int new_val);
    void setSetId(const std::wstring& new_val);
    std::wstring getSetId();

    xd::IRowInserterPtr getRowInserter();
    xd::rowpos_t getRowCount();

    // IXdnativeSet

    bool addEventHandler(IXdnativeSetEvents* handler);
    bool removeEventHandler(IXdnativeSetEvents* handler);


    xd::IIndexInfoPtr createIndex(const std::wstring& name,
                                     const std::wstring& expr,
                                     xd::IJob* job)
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

    xd::IIndexInfoEnumPtr getIndexEnum()
    {
        xcm::IVectorImpl<xd::IIndexInfoEnumPtr>* vec;
        vec = new xcm::IVectorImpl<xd::IIndexInfoEnumPtr>;
        return vec;
    }


    bool updateRow(xd::rowid_t rowid,
                   xd::ColumnUpdateInfo* info,
                   size_t info_size);

    virtual unsigned long long getStructureModifyTime();

protected:

    INodeValuePtr openSetDefinition(bool create_if_not_exist);

    bool baseSetModifyStructure(xd::IStructurePtr struct_config,
                                bool* done);

    bool createCalcField(xd::IColumnInfoPtr params);
    bool deleteCalcField(const std::wstring& name);
    bool modifyCalcField(const std::wstring& name,
                         xd::IColumnInfoPtr params);
    void appendCalcFields(xd::IStructure* structure);

    void onOfsPathChanged(const std::wstring& new_path) { }
    void onRelationshipsUpdated();


    void fire_onSetDomainUpdated();
    void fire_onSetStructureUpdated();
    void fire_onSetRelationshipsUpdated();
    void fire_onSetRowUpdated(xd::rowid_t rowid);
    void fire_onSetRowDeleted(xd::rowid_t rowid);
    
protected:

    XdnativeDatabase* m_database;
    bool m_temporary;

private:
    
    kl::mutex m_event_mutex;         // for m_event_handlers
    kl::mutex m_structure_mutex;     // for structure mods and m_calc_fields
    kl::mutex m_setattributes_mutex; // for m_set_id and m_set_flags

    std::wstring m_set_id;
    unsigned int m_set_flags;

    std::vector<IXdnativeSetEvents*> m_event_handlers;
    std::vector<xd::IColumnInfoPtr> m_calc_fields;
    unsigned long long m_calcrefresh_time;
    bool m_rel_init;
};


#endif
