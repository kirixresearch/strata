/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-01-13
 *
 */


#include "tango.h"
#include "dbentry.h"


DatabaseEntry::DatabaseEntry()
{
    m_name = L"";
    m_desc = L"";
}

void DatabaseEntry::setName(const std::wstring& name)
{
    m_name = name;
}

void DatabaseEntry::setDescription(const std::wstring& desc)
{
    m_desc = desc;
}

std::wstring DatabaseEntry::getName()
{
    return m_name;
}

std::wstring DatabaseEntry::getDescription()
{
    return m_desc;
}


