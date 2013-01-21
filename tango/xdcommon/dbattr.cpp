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


#include "tango.h"
#include "dbattr.h"


DatabaseAttributes::DatabaseAttributes()
{
}

DatabaseAttributes::~DatabaseAttributes()
{
}

bool DatabaseAttributes::getBoolAttribute(int attr_id)
{
    return m_bool_attrs[attr_id];
}

void DatabaseAttributes::setBoolAttribute(int attr_id, bool value)
{
    m_bool_attrs[attr_id] = value;
}



int DatabaseAttributes::getIntAttribute(int attr_id)
{
    return m_int_attrs[attr_id];
}

void DatabaseAttributes::setIntAttribute(int attr_id, int value)
{
    m_int_attrs[attr_id] = value;
}



std::wstring DatabaseAttributes::getStringAttribute(int attr_id)
{
    return m_str_attrs[attr_id];
}

void DatabaseAttributes::setStringAttribute(int attr_id,
                                            const std::wstring& value)
{
    m_str_attrs[attr_id] = value;
}

