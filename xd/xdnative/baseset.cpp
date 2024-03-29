/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-04-13
 *
 */


#include <xd/xd.h>
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

    m_temporary = false;
}

BaseSet::~BaseSet()
{
    m_database->unref();
}

void BaseSet::setTemporary(bool value)
{
    m_temporary = value;
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

bool BaseSet::createCalcField(const xd::ColumnInfo& colinfo)
{
    KL_AUTO_LOCK(m_structure_mutex);

    INodeValuePtr file = openSetDefinition(true);
    if (file.isNull())
        return false;

    // load calculated fields
    INodeValuePtr folder_node = file->getChild(L"calc_fields", true);
    if (!folder_node)
        return false;

    std::wstring name = colinfo.name;
    kl::makeUpper(name);

    if (folder_node->getChildExist(name))
    {
        return false;
    }

    INodeValuePtr item_node = folder_node->createChild(name);

    INodeValuePtr name_node = item_node->createChild(L"name");
    name_node->setString(colinfo.name);

    INodeValuePtr type_node = item_node->createChild(L"type");
    type_node->setInteger(colinfo.type);

    INodeValuePtr width_node = item_node->createChild(L"width");
    width_node->setInteger(colinfo.width);

    INodeValuePtr scale_node = item_node->createChild(L"scale");
    scale_node->setInteger(colinfo.scale);

    INodeValuePtr expr_node = item_node->createChild(L"expression");
    expr_node->setString(colinfo.expression);

    m_calc_fields.push_back(colinfo);

    return true;
}


bool BaseSet::modifyCalcField(const std::wstring& _name, const xd::ColumnInfo& colinfo)
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
    
    if ((colinfo.mask & xd::ColumnInfo::maskCalculated) && !colinfo.calculated)
    {
        // this is a make permanent operation
        return false;
    }

    if (colinfo.mask & xd::ColumnInfo::maskName)
    {
        std::wstring new_name = colinfo.name;
        kl::makeUpper(new_name);

        if (!folder_node->renameChild(name, new_name))
        {
            return false;
        }

        INodeValuePtr name_node = item_node->getChild(L"name", true);
        name_node->setString(colinfo.name);
    }

    if (colinfo.mask & xd::ColumnInfo::maskType)
    {
        INodeValuePtr type_node = item_node->getChild(L"type", true);
        type_node->setInteger(colinfo.type);
    }

    if (colinfo.mask & xd::ColumnInfo::maskWidth)
    {
        INodeValuePtr width_node = item_node->getChild(L"width", true);
        width_node->setInteger(colinfo.width);
    }

    if (colinfo.mask & xd::ColumnInfo::maskScale)
    {
        INodeValuePtr scale_node = item_node->getChild(L"scale", true);
        scale_node->setInteger(colinfo.scale);
    }

    if (colinfo.mask & xd::ColumnInfo::maskExpression)
    {
        INodeValuePtr expr_node = item_node->getChild(L"expression", true);
        expr_node->setString(colinfo.expression);
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


void BaseSet::appendCalcFields(xd::Structure& structure)
{
    // do this before the m_structure_mutex of BaseSet is locked;
    // this will avoid interlocking the mutexes of NativeTable
    // and BaseSet.  This problem won't exist anymore once we
    // make getStructureModifyTime in NativeTable take no mutex

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

        int child_count = folder_node->getChildCount();
        m_calc_fields.clear();

        for (int i = 0; i < child_count; i++)
        {
            item_node = folder_node->getChildByIdx(i);
            xd::ColumnInfo colinfo;

            node = item_node->getChild(L"name", false);
            if (node.isOk())
            {
                colinfo.name = node->getString();
            }
             else
            {
                colinfo.name = item_node->getName();
            }

            node = item_node->getChild(L"type", false);
            if (!node)
                continue;
            colinfo.type = node->getInteger();

            node = item_node->getChild(L"width", false);
            if (!node)
                continue;
            colinfo.width = node->getInteger();

            node = item_node->getChild(L"scale", false);
            if (!node)
                continue;
            colinfo.scale = node->getInteger();

            node = item_node->getChild(L"expression", false);
            if (!node)
                continue;
            colinfo.expression = node->getString();

            colinfo.calculated = true;

            m_calc_fields.push_back(colinfo);
        }
    }


    std::vector<xd::ColumnInfo>::iterator it;
    for (it = m_calc_fields.begin(); it != m_calc_fields.end(); ++it)
        structure.createColumn(*it);
}


void BaseSet::onRelationshipsUpdated()
{
    fire_onSetRelationshipsUpdated();
}



bool BaseSet::baseSetModifyStructure(const xd::StructureModify& mod_params, bool* done_flag)
{
    KL_AUTO_LOCK(m_structure_mutex);

    *done_flag = false;

    // keep the file open
    INodeValuePtr file = openSetDefinition(true);
    if (file.isNull())
        return false;

    std::vector<xd::StructureModify::Action>::const_iterator it;
    size_t processed_action_count = 0;

    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionDelete)
            continue;

        if (deleteCalcField(it->column))
            processed_action_count++;
    }

    // handle modify
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionModify)
            continue;

        if (modifyCalcField(it->column, it->params))
            processed_action_count++;
    }

    // handle create
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionCreate)
            continue;

        if (it->params.expression.length() > 0)
        {
            if (createCalcField(it->params))
                processed_action_count++;
        }
    }

    // handle insert
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionInsert)
            continue;

        if (it->params.expression.length() > 0)
        {
            if (createCalcField(it->params))
                processed_action_count++;
        }
    }

    if (processed_action_count == mod_params.actions.size())
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





