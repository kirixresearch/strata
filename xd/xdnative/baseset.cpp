/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-04-13
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <xd/xd.h>
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/exindex.h"
#include "../xdcommon/idxutil.h"
#include "xdnative_private.h"
#include "database.h"
#include "baseset.h"
#include "util.h"
#include <ctime>
#include <algorithm>


BaseSet::BaseSet(XdnativeDatabase* database)
{
    m_database = database;
    m_database->ref();
    
    m_set_flags = 0;
    m_calcrefresh_time = 0;
    m_rel_init = false;
}

BaseSet::~BaseSet()
{
    m_database->unref();
}

INodeValuePtr BaseSet::openSetDefinition(bool create_if_not_exist)
{
    std::wstring path;
    path.reserve(80);
    path = L"/.system/objects/";
    path += getSetId();

    INodeValuePtr file = m_database->openNodeFile(path);
    if (file.isOk())
    {
        return file;
    }

    if (create_if_not_exist)
    {
        return m_database->createNodeFile(path);
    }

    return xcm::null;
}


unsigned int BaseSet::getSetFlags()
{
    KL_AUTO_LOCK(m_setattributes_mutex);

    return m_set_flags;
}

void BaseSet::setSetFlags(unsigned int new_val)
{
    KL_AUTO_LOCK(m_setattributes_mutex);

    m_set_flags = new_val;
}

void BaseSet::setSetId(const std::wstring& new_val)
{
    KL_AUTO_LOCK(m_setattributes_mutex);
    
    m_set_id = new_val;
}

std::wstring BaseSet::getSetId()
{
    KL_AUTO_LOCK(m_setattributes_mutex);

    if (m_set_id.length() == 0)
        m_set_id = getUniqueString();

    return m_set_id;
}

xd::IRowInserterPtr BaseSet::getRowInserter()
{
    // default does nothing
    return xcm::null;
}



// calculated Field routines

bool BaseSet::createCalcField(xd::IColumnInfoPtr colinfo)
{
    KL_AUTO_LOCK(m_structure_mutex);

    INodeValuePtr file = openSetDefinition(true);
    if (file.isNull())
        return false;

    // load calculated fields
    INodeValuePtr folder_node = file->getChild(L"calc_fields", true);
    if (!folder_node)
        return false;

    std::wstring name = colinfo->getName();
    kl::makeUpper(name);

    if (folder_node->getChildExist(name))
    {
        return false;
    }

    INodeValuePtr item_node = folder_node->createChild(name);

    INodeValuePtr name_node = item_node->createChild(L"name");
    name_node->setString(colinfo->getName());

    INodeValuePtr type_node = item_node->createChild(L"type");
    type_node->setInteger(colinfo->getType());

    INodeValuePtr width_node = item_node->createChild(L"width");
    width_node->setInteger(colinfo->getWidth());

    INodeValuePtr scale_node = item_node->createChild(L"scale");
    scale_node->setInteger(colinfo->getScale());

    INodeValuePtr expr_node = item_node->createChild(L"expression");
    expr_node->setString(colinfo->getExpression());

    xd::IColumnInfoPtr newcol = colinfo->clone();
    newcol->setCalculated(true);

    m_calc_fields.push_back(newcol);

    return true;
}


bool BaseSet::modifyCalcField(const std::wstring& _name,
                              xd::IColumnInfoPtr colinfo)
{
    KL_AUTO_LOCK(m_structure_mutex);

    INodeValuePtr file = openSetDefinition(true);
    if (file.isNull())
        return false;

    // load calculated fields
    INodeValuePtr folder_node = file->getChild(L"calc_fields", true);
    if (!folder_node)
        return false;

    std::wstring name = _name;
    kl::makeUpper(name);

    INodeValuePtr item_node = folder_node->getChild(name, false);
    if (item_node.isNull())
    {
        return false;
    }
    
    if (!colinfo->getCalculated())
    {
        // this is a make permanent operation
        return false;
    }

    if (colinfo->getName().length() > 0)
    {
        std::wstring new_name = colinfo->getName();
        kl::makeUpper(new_name);

        if (!folder_node->renameChild(name, new_name))
        {
            return false;
        }

        INodeValuePtr name_node = item_node->getChild(L"name", true);
        name_node->setString(colinfo->getName());
    }

    if (colinfo->getType() != -1)
    {
        INodeValuePtr type_node = item_node->getChild(L"type", true);
        type_node->setInteger(colinfo->getType());
    }

    if (colinfo->getWidth() != -1)
    {
        INodeValuePtr width_node = item_node->getChild(L"width", true);
        width_node->setInteger(colinfo->getWidth());
    }

    if (colinfo->getScale() != -1)
    {
        INodeValuePtr scale_node = item_node->getChild(L"scale", true);
        scale_node->setInteger(colinfo->getScale());
    }

    if (colinfo->getExpression().length() > 0)
    {
        INodeValuePtr expr_node = item_node->getChild(L"expression", true);
        expr_node->setString(colinfo->getExpression());
    }

    m_calcrefresh_time = 0;

    return true;
}


bool BaseSet::deleteCalcField(const std::wstring& _name)
{
    KL_AUTO_LOCK(m_structure_mutex);

    INodeValuePtr file = openSetDefinition(true);
    if (file.isNull())
        return false;

    // load calculated fields
    INodeValuePtr folder_node = file->getChild(L"calc_fields", true);
    if (!folder_node)
        return false;

    std::wstring name = _name;
    kl::makeUpper(name);

    if (!folder_node->getChildExist(name))
    {
        return false;
    }

    folder_node->deleteChild(name);

    m_calcrefresh_time = 0;

    return true;
}


void BaseSet::appendCalcFields(xd::IStructure* structure)
{
    // do this before the m_structure_mutex of BaseSet is locked;
    // this will avoid interlocking the mutexes of NativeTable
    // and BaseSet.  This problem won't exist anymore once we
    // make getStructureModifyTime in NativeTable take no mutex.
    unsigned long long t = getStructureModifyTime();
    
    KL_AUTO_LOCK(m_structure_mutex);

    if (t == 0 || t != m_calcrefresh_time)
    {
        m_calcrefresh_time = t;

        INodeValuePtr file = openSetDefinition(false);
        if (file.isNull())
            return;

        // load calculated fields
        INodeValuePtr folder_node = file->getChild(L"calc_fields", true);
        if (!folder_node)
            return;

        INodeValuePtr item_node, node;
        ColumnInfo* colinfo;

        int child_count = folder_node->getChildCount();
        m_calc_fields.clear();

        for (int i = 0; i < child_count; i++)
        {
            item_node = folder_node->getChildByIdx(i);
            colinfo = new ColumnInfo;

            node = item_node->getChild(L"name", false);
            if (node.isOk())
            {
                colinfo->setName(node->getString());
            }
             else
            {
                colinfo->setName(item_node->getName());
            }

            node = item_node->getChild(L"type", false);
            if (!node)
                continue;
            colinfo->setType(node->getInteger());

            node = item_node->getChild(L"width", false);
            if (!node)
                continue;
            colinfo->setWidth(node->getInteger());

            node = item_node->getChild(L"scale", false);
            if (!node)
                continue;
            colinfo->setScale(node->getInteger());

            node = item_node->getChild(L"expression", false);
            if (!node)
                continue;
            colinfo->setExpression(node->getString());

            colinfo->setCalculated(true);

            m_calc_fields.push_back(static_cast<xd::IColumnInfo*>(colinfo));
        }
    }

    IStructureInternalPtr intstruct = structure;

    std::vector<xd::IColumnInfoPtr>::iterator it;
    for (it = m_calc_fields.begin();
         it != m_calc_fields.end();
         ++it)
    {
        intstruct->addColumn((*it)->clone());
    }

}


void BaseSet::onRelationshipsUpdated()
{
    fire_onSetRelationshipsUpdated();
}



bool BaseSet::baseSetModifyStructure(xd::IStructurePtr struct_config,
                                     bool* done_flag)
{
    KL_AUTO_LOCK(m_structure_mutex);

    *done_flag = false;

    // keep the file open
    INodeValuePtr file = openSetDefinition(true);
    if (file.isNull())
        return false;

    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    int processed_action_count = 0;

    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionDelete)
            continue;

        if (deleteCalcField(it->m_colname))
            processed_action_count++;
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionModify)
            continue;

        if (modifyCalcField(it->m_colname, it->m_params))
            processed_action_count++;
    }

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        if (it->m_params->getExpression().length() > 0)
        {
            if (createCalcField(it->m_params))
                processed_action_count++;
        }
    }

    // handle insert
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionInsert)
            continue;

        if (it->m_params->getExpression().length() > 0)
        {
            if (createCalcField(it->m_params))
                processed_action_count++;
        }
    }

    if (processed_action_count == actions.size())
    {
        // we have handled all actions, so we're done
        *done_flag = true;
    }

    return true;
}




xd::rowpos_t BaseSet::getRowCount()
{
    // we don't know how many rows are in the set
    return 0;
}



bool BaseSet::addEventHandler(IXdnativeSetEvents* handler)
{
    KL_AUTO_LOCK(m_event_mutex);
    m_event_handlers.push_back(handler);
    return true;
}

bool BaseSet::removeEventHandler(IXdnativeSetEvents* handler)
{
    KL_AUTO_LOCK(m_event_mutex);
    std::vector<IXdnativeSetEvents*>::iterator it;
    it = std::find(m_event_handlers.begin(),
                   m_event_handlers.end(),
                   handler);
    if (it == m_event_handlers.end())
        return false;
    m_event_handlers.erase(it);
    return true;
}

bool BaseSet::updateRow(xd::rowid_t rowid,
                        xd::ColumnUpdateInfo* info,
                        size_t info_size)
{
    return false;
}                           


unsigned long long BaseSet::getStructureModifyTime()
{
    return 0;
}


void BaseSet::fire_onSetDomainUpdated()
{
    KL_AUTO_LOCK(m_event_mutex);

    std::vector<IXdnativeSetEvents*>::iterator it;
    for (it = m_event_handlers.begin();
         it != m_event_handlers.end();
         ++it)
    {
        (*it)->onSetDomainUpdated();
    }
}

void BaseSet::fire_onSetStructureUpdated()
{
    KL_AUTO_LOCK(m_event_mutex);

    std::vector<IXdnativeSetEvents*>::iterator it;
    for (it = m_event_handlers.begin();
         it != m_event_handlers.end();
         ++it)
    {
        (*it)->onSetStructureUpdated();
    }
}

void BaseSet::fire_onSetRelationshipsUpdated()
{
    KL_AUTO_LOCK(m_event_mutex);

    std::vector<IXdnativeSetEvents*>::iterator it;
    for (it = m_event_handlers.begin();
         it != m_event_handlers.end();
         ++it)
    {
        (*it)->onSetRelationshipsUpdated();
    }
}

void BaseSet::fire_onSetRowUpdated(xd::rowid_t rowid)
{
    KL_AUTO_LOCK(m_event_mutex);

    std::vector<IXdnativeSetEvents*>::iterator it;
    for (it = m_event_handlers.begin();
         it != m_event_handlers.end();
         ++it)
    {
        (*it)->onSetRowUpdated(rowid);
    }
}

void BaseSet::fire_onSetRowDeleted(xd::rowid_t rowid)
{
    KL_AUTO_LOCK(m_event_mutex);

    std::vector<IXdnativeSetEvents*>::iterator it;
    for (it = m_event_handlers.begin();
         it != m_event_handlers.end();
         ++it)
    {
        (*it)->onSetRowDeleted(rowid);
    }
}





