/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2013-03-01
 *
 */




#include <xd/xd.h>
#include "database.h"
#include "inserter.h"
#include <kl/file.h>
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/math.h>
#include <kl/xml.h>
#include "../xdcommon/util.h"

const int PACKAGE_BLOCK_SIZE = 1000000;

ExcelRowInserter::ExcelRowInserter(ExcelDatabase* db, const std::wstring& table, const xd::Structure& structure)
{
    m_database = db;
    m_database->ref();
    
    m_structure = structure;
    m_data = NULL;

    xd::IAttributesPtr attr = m_database->getAttributes();

    m_buf_rows = 0;
    m_rows_per_buf = 0;
    m_table = table;
    kl::replaceStr(m_table, L"\"", L"");
}

ExcelRowInserter::~ExcelRowInserter()
{
    delete[] m_data;

    m_database->unref();
}

xd::objhandle_t ExcelRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<ExcelInsertFieldData>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals(it->m_name, column_name))
            return &(*it);
    }

    return 0;
}

bool ExcelRowInserter::putRawPtr(xd::objhandle_t column_handle,
                               const unsigned char* value,
                               int length)
{
    return false;
}

bool ExcelRowInserter::putString(xd::objhandle_t column_handle,
                                 const std::string& value)
{
    ExcelInsertFieldData* f = (ExcelInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_str_val = kl::towstring(value);

    return true;
}

bool ExcelRowInserter::putWideString(xd::objhandle_t column_handle,
                                   const std::wstring& value)
{
    ExcelInsertFieldData* f = (ExcelInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_str_val = value;

    return true;
}

bool ExcelRowInserter::putDouble(xd::objhandle_t column_handle,
                               double value)
{
    ExcelInsertFieldData* f = (ExcelInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_dbl_val = value;

    return true;
}

bool ExcelRowInserter::putInteger(xd::objhandle_t column_handle,
                                int value)
{
    ExcelInsertFieldData* f = (ExcelInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_int_val = value;

    return true;
}

bool ExcelRowInserter::putBoolean(xd::objhandle_t column_handle,
                                bool value)
{
    ExcelInsertFieldData* f = (ExcelInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_bool_val = value;

    return true;
}

bool ExcelRowInserter::putDateTime(xd::objhandle_t column_handle,
                                 xd::datetime_t value)
{
    ExcelInsertFieldData* f = (ExcelInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_datetime_val = value;

    return true;
}

bool ExcelRowInserter::putNull(xd::objhandle_t column_handle)
{
    ExcelInsertFieldData* f = (ExcelInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    return false;
}

bool ExcelRowInserter::startInsert(const std::wstring& col_list)
{
    return true;
}


// reipped from xdnative/util.cpp
static void dbl2decstr(char* dest, double d, int width, int scale)
{
    double intpart;

    // check for negative
    if (d < 0.0)
    {
        *dest = '-';
        dest++;
        width--;
        d = fabs(d);
    }

    // rounding
    d += (0.5/kl::pow10(scale));

    // put everything to the right of the decimal
    d /= kl::pow10(width-scale);

    while (width)
    {
        d *= 10;
        d = modf(d, &intpart);
        if (intpart > 9.1)
            intpart = 0.0;

        *dest = int(intpart) + '0';
        dest++;
        width--;
    }
}

bool ExcelRowInserter::insertRow()
{
    return true;
}

void ExcelRowInserter::finishInsert()
{
    if (!m_writer)
        return;
}

bool ExcelRowInserter::flush()
{
    return true;
}
