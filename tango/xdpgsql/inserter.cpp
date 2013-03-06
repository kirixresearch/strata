/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2013-03-01
 *
 */




#include "libpq-fe.h"
#include "tango.h"
#include "database.h"
#include "inserter.h"
#include <kl/portable.h>
#include <kl/string.h>


PgsqlRowInserter::PgsqlRowInserter(PgsqlDatabase* db, const std::wstring& table)
{
    m_database = db;
    m_database->ref();

    m_inserting = false;
    m_table = table;
}

PgsqlRowInserter::~PgsqlRowInserter()
{
    std::vector<PgsqlInsertFieldData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete *it;

    m_database->unref();
}

tango::objhandle_t PgsqlRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<PgsqlInsertFieldData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->m_name.c_str(), column_name.c_str()))
            return (tango::objhandle_t)(*it);
    }

    return 0;
}

tango::IColumnInfoPtr PgsqlRowInserter::getInfo(tango::objhandle_t column_handle)
{
    return xcm::null;
}

bool PgsqlRowInserter::putRawPtr(tango::objhandle_t column_handle,
                                const unsigned char* value,
                                int length)
{
    return false;
}

bool PgsqlRowInserter::putString(tango::objhandle_t column_handle,
                                const std::string& value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    if (f->m_tango_type == tango::typeWideCharacter)
    {
        return putWideString(column_handle, kl::towstring(value));
    }

    int copy_len = value.length();
    if (copy_len > f->m_width)
        copy_len = f->m_width;

    f->m_indicator = copy_len;
    memcpy(f->m_str_val, value.c_str(), copy_len * sizeof(char));
    f->m_str_val[copy_len] = 0;

    return true;
}

bool PgsqlRowInserter::putWideString(tango::objhandle_t column_handle,
                                    const std::wstring& value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    if (f->m_tango_type == tango::typeCharacter)
    {
        return putString(column_handle, kl::tostring(value));
    }

    int copy_len = value.length();
    if (copy_len > f->m_width)
        copy_len = f->m_width;

    f->m_indicator = copy_len * sizeof(wchar_t);
    memcpy(f->m_wstr_val, value.c_str(), copy_len * sizeof(wchar_t));
    f->m_wstr_val[copy_len] = 0;

    return true;
}

bool PgsqlRowInserter::putDouble(tango::objhandle_t column_handle,
                                double value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_indicator = 0;
    f->m_dbl_val = value;
    f->m_int_val = (int)value;

    return true;
}

bool PgsqlRowInserter::putInteger(tango::objhandle_t column_handle,
                                 int value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }


    f->m_indicator = 0;
    f->m_int_val = value;
    f->m_dbl_val = value;

    return true;
}

bool PgsqlRowInserter::putBoolean(tango::objhandle_t column_handle,
                                 bool value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_indicator = 0;
    f->m_bool_val = value;
    f->m_int_val = value ? 1 : 0;
    f->m_dbl_val = value ? 1.0 : 0.0;

    return true;
}

bool PgsqlRowInserter::putDateTime(tango::objhandle_t column_handle,
                                  tango::datetime_t datetime)
{
/*
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_indicator = 0;

    if (datetime == 0)
    {
        // null date
        f->m_indicator = 1;
        return true;
    }


    if (true)  // date
    {
        tango::DateTime dt(datetime);

        f->m_date_val.year = dt.getYear();
        f->m_date_val.month = dt.getMonth();
        f->m_date_val.day = dt.getDay();

        // SQL Server cannot handle dates less than 1753
        if (f->m_date_val.year < 1753)
        {
            f->m_indicator = 1;
        }
    }
     else if (true) //datetime
    {
        tango::DateTime dt(datetime);

        f->m_datetime_val.year = dt.getYear();
        f->m_datetime_val.month = dt.getMonth();
        f->m_datetime_val.day = dt.getDay();
        f->m_datetime_val.hour = dt.getHour();
        f->m_datetime_val.minute = dt.getMinute();
        f->m_datetime_val.second = dt.getSecond();
        f->m_datetime_val.fraction = 0;
        if (f->m_tango_type == tango::typeDateTime)
        {
            f->m_datetime_val.fraction = dt.getMillisecond()*1000000;
        }

        // SQL Server cannot handle dates less than 1753
        if (f->m_datetime_val.year < 1753)
        {
            f->m_indicator = 1;
        }
    }


    */
    return true;
}

bool PgsqlRowInserter::putNull(tango::objhandle_t column_handle)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_indicator = 1;

    return false;
}

bool PgsqlRowInserter::startInsert(const std::wstring& col_list)
{
    return true;
}

bool PgsqlRowInserter::insertRow()
{
    if (!m_inserting)
        return false;

    return true;
}

void PgsqlRowInserter::finishInsert()
{
}

bool PgsqlRowInserter::flush()
{
    return true;
}






