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


#define XLNT_STATIC
#include <xlnt/xlnt.hpp>
#include <xlnt/worksheet/sheet_format_properties.hpp>

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

    m_ws = new xlnt::worksheet();
}

ExcelRowInserter::~ExcelRowInserter()
{
    delete[] m_data;

    m_database->unref();
}

xd::objhandle_t ExcelRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<ExcelInsertFieldData>::iterator it;
    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
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

bool ExcelRowInserter::putDouble(xd::objhandle_t column_handle, double value)
{
    ExcelInsertFieldData* f = (ExcelInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_dbl_val = value;

    return true;
}

bool ExcelRowInserter::putInteger(xd::objhandle_t column_handle, int value)
{
    ExcelInsertFieldData* f = (ExcelInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_int_val = value;

    return true;
}

bool ExcelRowInserter::putBoolean(xd::objhandle_t column_handle, bool value)
{
    ExcelInsertFieldData* f = (ExcelInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_bool_val = value;

    return true;
}

bool ExcelRowInserter::putDateTime(xd::objhandle_t column_handle, xd::datetime_t value)
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

    f->m_str_val = L"";
    f->m_bool_val = false;
    f->m_dbl_val = 0.0;
    f->m_int_val = 0;
    f->m_datetime_val = 0;

    return true;
}

bool ExcelRowInserter::startInsert(const std::wstring& col_list)
{
    try
    {
        *m_ws = m_database->m_wb->sheet_by_title(kl::toUtf8(m_table).m_s);
    }
    catch(xlnt::key_not_found e)
    {
        if (m_database->m_wb->sheet_count() == 1 && m_database->m_wb->active_sheet().next_row() == 1)
        {
            *m_ws = m_database->m_wb->active_sheet();
            m_ws->title(kl::toUtf8(m_table).m_s);
        }
         else
        {
            *m_ws = m_database->m_wb->create_sheet();
            m_ws->title(kl::toUtf8(m_table).m_s);
        }
    }

    
    // these next four lines fix a bug in xlnt when exporting to a sheet that
    // is not the active sheet. For some reason the baseColWidth and defaultRowHeight
    // attributes are not written in the sheetFormatPr tag unless the following are explicitly set
    xlnt::sheet_format_properties format_properties;
    format_properties.base_col_width = 10.0;
    format_properties.default_row_height = 16.0;
    (*m_ws).format_properties(format_properties);


    xlnt::row_t row = m_ws->next_row();

    xd::Structure structure = m_database->describeTable(m_table);

    // calculate the total physical row width
    std::vector<xd::ColumnInfo>::const_iterator it;
    int idx = 1;
    for (it = structure.columns.begin(); it != structure.columns.end(); ++it)
    {
        ExcelInsertFieldData d;
        d.m_name = it->name;
        d.m_xd_type = it->type;
        d.m_str_val = L"";
        d.m_idx = idx++;

        if (row == 1)
        {
            m_ws->cell((xlnt::column_t)d.m_idx, row).value(kl::toUtf8(d.m_name).m_s);
        }

        m_insert_data.push_back(d);
    }

    return true;
}



bool ExcelRowInserter::insertRow()
{
    xlnt::row_t row = m_ws->next_row();

    //std::vector<ExcelInsertFieldData>::iterator it;
    //for (it = m_

    for (auto& fld : m_insert_data)
    {
        switch (fld.m_xd_type)
        {
            case xd::typeCharacter:
            case xd::typeWideCharacter:
                m_ws->cell((xlnt::column_t)fld.m_idx, row).value(kl::toUtf8(fld.m_str_val).m_s);
                break;

            case xd::typeNumeric:
            case xd::typeDouble:
                m_ws->cell((xlnt::column_t)fld.m_idx, row).value(fld.m_dbl_val);
                break;

            case xd::typeInteger:
                m_ws->cell((xlnt::column_t)fld.m_idx, row).value(fld.m_int_val);
                break;

            case xd::typeBoolean:
                m_ws->cell((xlnt::column_t)fld.m_idx, row).value(fld.m_bool_val);
                break;

            case xd::typeDate:
            {
                xd::DateTime dt(fld.m_datetime_val);
                m_ws->cell((xlnt::column_t)fld.m_idx, row).value(xlnt::date(dt.getYear(),dt.getMonth(),dt.getDay()));
                break;
            }

            case xd::typeDateTime:
            {
                xd::DateTime dt(fld.m_datetime_val);
                m_ws->cell((xlnt::column_t)fld.m_idx, row).value(xlnt::datetime(dt.getYear(), dt.getMonth(), dt.getDay(), dt.getHour(), dt.getMinute(), dt.getSecond(), dt.getMillisecond()*1000));
                break;
            }
        }

        // reset for next round
        fld.m_str_val = L"";
    }

    return true;
}

void ExcelRowInserter::finishInsert()
{
    m_database->m_wb->save(m_database->m_path);
}

bool ExcelRowInserter::flush()
{
    return true;
}
