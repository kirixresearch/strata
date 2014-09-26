/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-08-26
 *
 */


#include <string>
#include <kl/portable.h>
#include <xd/xd.h>
#include "columninfo.h"



ColumnInfo::ColumnInfo()
{
    m_name[0] = 0;
    m_expression = NULL;
    m_type = xd::typeInvalid;
    m_width = 0;
    m_scale = 0;
    m_calculated = false;
    m_offset = 0;
    m_col_ord = 0;
    m_table_ord = 0;
    m_encoding = xd::encodingUndefined;
    m_nulls_allowed = false;
}

ColumnInfo::~ColumnInfo()
{
    if (m_expression)
    {
        free(m_expression);
    }
}

void ColumnInfo::setName(const std::wstring& new_val)
{
    wcsncpy(m_name, new_val.c_str(), 80);
    m_name[80] = 0;
}

std::wstring ColumnInfo::getName()
{
    return m_name;
}

void ColumnInfo::setType(int new_val)
{
    m_type = new_val;
}

int ColumnInfo::getType()
{
    return m_type;
}

void ColumnInfo::setWidth(int new_val)
{
    m_width = new_val;
}

int ColumnInfo::getWidth()
{
    return m_width;
}

void ColumnInfo::setScale(int new_val)
{
    m_scale = new_val;
}

int ColumnInfo::getScale()
{
    return m_scale;
}

void ColumnInfo::setNullsAllowed(bool new_val)
{
    m_nulls_allowed = new_val;
}

bool ColumnInfo::getNullsAllowed()
{
    return m_nulls_allowed;
}

void ColumnInfo::setCalculated(bool new_val)
{
    m_calculated = new_val;
}

bool ColumnInfo::getCalculated()
{
    return m_calculated;
}

void ColumnInfo::setExpression(const std::wstring& new_val)
{
    if (m_expression)
    {
        free(m_expression);
    }

    m_expression = wcsdup(new_val.c_str());
}

std::wstring ColumnInfo::getExpression()
{
    if (!m_expression)
        return L"";

    return m_expression;
}

void ColumnInfo::setOffset(int new_val)
{
    m_offset = new_val;
}

int ColumnInfo::getOffset()
{
    return m_offset;
}

void ColumnInfo::setEncoding(int new_val)
{
    m_encoding = new_val;
}

int ColumnInfo::getEncoding()
{
    return m_encoding;
}

void ColumnInfo::setColumnOrdinal(int new_val)
{
    m_col_ord = new_val;
}

int ColumnInfo::getColumnOrdinal()
{
    return m_col_ord;
}

void ColumnInfo::setTableOrdinal(int new_val)
{
    m_table_ord = new_val;
}

int ColumnInfo::getTableOrdinal()
{
    return m_table_ord;
}


xd::IColumnInfoPtr ColumnInfo::clone()
{
    ColumnInfo* p = new ColumnInfo;

    wcscpy(p->m_name, m_name);
    p->m_type = m_type;
    p->m_width = m_width;
    p->m_scale = m_scale;
    p->m_offset = m_offset;
    p->m_col_ord = m_col_ord;
    p->m_table_ord = m_table_ord;
    p->m_encoding = m_encoding;
    p->m_calculated = m_calculated;
    p->m_nulls_allowed = m_nulls_allowed;

    if (m_expression)
    {
        p->m_expression = wcsdup(m_expression);
    }

    return static_cast<xd::IColumnInfo*>(p);
}

void ColumnInfo::copyTo(xd::IColumnInfoPtr dest)
{
    dest->setName(m_name);
    dest->setType(m_type);
    dest->setWidth(m_width);
    dest->setScale(m_scale);
    dest->setOffset(m_offset);
    dest->setCalculated(m_calculated);
    dest->setNullsAllowed(m_nulls_allowed);
    if (m_expression)
        dest->setExpression(m_expression);
         else
        dest->setExpression(L"");
}


void ColumnInfo::fromColumnInfo(const xd::ColumnInfo& col)
{
    wcscpy(m_name, col.name.c_str());
    m_type = col.type;
    m_width = col.width;
    m_scale = col.scale;
    m_offset = col.source_offset;
    m_col_ord = col.column_ordinal;
    m_table_ord = col.table_ordinal;
    m_encoding = col.source_encoding;
    m_calculated = col.calculated;
    m_nulls_allowed = col.nulls_allowed;
    if (m_expression) free(m_expression);
    m_expression = NULL;
    if (col.expression.length() > 0)
    {
        m_expression = wcsdup(col.expression.c_str());
    }
}

xd::ColumnInfo ColumnInfo::toColumnInfo()
{
    xd::ColumnInfo res;
    res.name = m_name;
    res.width = m_width;
    res.scale = m_scale;
    res.source_offset = m_offset;
    res.column_ordinal = m_col_ord;
    res.table_ordinal = m_table_ord;
    res.source_encoding = m_encoding;
    res.calculated = m_calculated;
    res.nulls_allowed = m_nulls_allowed;
    if (m_expression)
        res.expression = m_expression;
    return res;
}