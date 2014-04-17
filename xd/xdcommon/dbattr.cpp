/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2004-05-05
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <xd/xd.h>
#include "dbattr.h"


DatabaseAttributes::DatabaseAttributes()
{
}

DatabaseAttributes::~DatabaseAttributes()
{
}

bool DatabaseAttributes::getBoolAttribute(int attr_id)
{
    KL_AUTO_LOCK(m_obj_mutex);
    return m_bool_attrs[attr_id];
}

void DatabaseAttributes::setBoolAttribute(int attr_id, bool value)
{
    m_obj_mutex.lock();
    m_bool_attrs[attr_id] = value;
    m_obj_mutex.unlock();

    sigAttributeUpdated(attr_id);
}



int DatabaseAttributes::getIntAttribute(int attr_id)
{
    KL_AUTO_LOCK(m_obj_mutex);
    return m_int_attrs[attr_id];
}

void DatabaseAttributes::setIntAttribute(int attr_id, int value)
{
    m_obj_mutex.lock();
    m_int_attrs[attr_id] = value;
    m_obj_mutex.unlock();

    sigAttributeUpdated(attr_id);
}



std::wstring DatabaseAttributes::getStringAttribute(int attr_id)
{
    KL_AUTO_LOCK(m_obj_mutex);
    return m_str_attrs[attr_id];
}

void DatabaseAttributes::setStringAttribute(int attr_id,
                                            const std::wstring& value)
{
    m_obj_mutex.lock();
    m_str_attrs[attr_id] = value;
    m_obj_mutex.unlock();

    sigAttributeUpdated(attr_id);
}

