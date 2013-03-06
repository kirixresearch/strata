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
#include <kl/utf8.h>


PgsqlRowInserter::PgsqlRowInserter(PgsqlDatabase* db, const std::wstring& table)
{
    m_database = db;
    m_database->ref();
    
    tango::IAttributesPtr attr = m_database->getAttributes();
    m_quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    m_quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    m_conn = NULL;

    m_buf_rows = 0;
    m_table = table;
    kl::replaceStr(m_table, L"\"", L"");

    m_utf8data = NULL;
    m_utf8data_len = 0;
}

PgsqlRowInserter::~PgsqlRowInserter()
{
    if (m_utf8data)
        free(m_utf8data);

    if (m_conn)
        m_database->closeConnection(m_conn);

    m_database->unref();
}

tango::objhandle_t PgsqlRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<PgsqlInsertFieldData>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp(it->m_name.c_str(), column_name.c_str()))
            return &(*it);
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

    f->m_value = kl::towstring(value);
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

    f->m_value = value;

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

    wchar_t buf[64];
    swprintf(buf, 63, L"%.*f", f->m_scale, value);
    f->m_value = buf;

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

    wchar_t buf[64];
    swprintf(buf, 63, L"%d", value);
    f->m_value = buf;

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


    f->m_value = value ? 'T' : 'F';

    return true;
}

bool PgsqlRowInserter::putDateTime(tango::objhandle_t column_handle,
                                   tango::datetime_t datetime)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    if (datetime == 0)
    {
        return putNull(column_handle);
    }

    tango::DateTime dt(datetime);
    wchar_t buf[64];

    if (f->m_tango_type == tango::typeDate)
    {
        swprintf(buf, 63, L"%04d-%02d-%02d", dt.getYear(), dt.getMonth(),  dt.getDay());
        f->m_value = buf;
    }
     else if (f->m_tango_type == tango::typeDateTime) //datetime
    {
        swprintf(buf, 63, L"%04d-%02d-%02d %02d:%02d:%02d", dt.getYear(), dt.getMonth(), dt.getDay(), dt.getHour(), dt.getMinute(), dt.getSecond());
        f->m_value = buf;
    }
     else
    {
        f->m_value = L"";
    }

    return true;
}

bool PgsqlRowInserter::putNull(tango::objhandle_t column_handle)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_value = L"\\N";

    return false;
}

bool PgsqlRowInserter::startInsert(const std::wstring& col_list)
{
    tango::IStructurePtr s = m_database->describeTable(m_table);
    if (s.isNull())
        return false;

    std::vector<std::wstring> columns;
    std::vector<std::wstring>::iterator it;
    std::wstring field_list;

    kl::parseDelimitedList(col_list, columns, L',');


    if (0 == wcscmp(col_list.c_str(), L"*") || col_list.length() == 0)
    {
        columns.clear();

        int i, col_count = s->getColumnCount();

        for (i = 0; i < col_count; ++i)
            columns.push_back(s->getColumnName(i));
    }

    m_fields.clear();

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        tango::IColumnInfoPtr col_info = s->getColumnInfo(*it);

        if (col_info.isNull())
            return false;

        PgsqlInsertFieldData d;
        d.m_name = col_info->getName();
        d.m_tango_type = col_info->getType();
        d.m_width = col_info->getWidth();
        d.m_scale = col_info->getScale();
        d.m_value = L"";

        m_fields.push_back(d);
    }


    m_conn = m_database->createConnection();
    if (!m_conn)
        return false;


    std::wstring sql;
    sql = L"COPY \"" + m_table + L"\" FROM stdin";

    PGresult* res = PQexec(m_conn, kl::toUtf8(sql));
    PQclear(res);
    m_buf_rows = 0;

    return true;
}

bool PgsqlRowInserter::insertRow()
{
    if (!m_conn)
        return false;

    if (m_wdata.length() > 0)
        m_wdata += L"\n";

    std::vector<PgsqlInsertFieldData>::iterator it;
    std::vector<PgsqlInsertFieldData>::iterator begin_it = m_fields.begin();
    std::vector<PgsqlInsertFieldData>::iterator end_it = m_fields.end();

    for (it = begin_it; it != end_it; ++it)
    {
        if (it != begin_it)
            m_wdata += L"\t";
        m_wdata += it->m_value;
    }

    m_buf_rows++;

    if (m_buf_rows == 1000)
    {
        flush();
    }


    return true;
}

void PgsqlRowInserter::finishInsert()
{
    if (!m_conn)
        return;

    flush();

    PQputCopyEnd(m_conn, NULL);

    if (m_utf8data)
    {
        free(m_utf8data);
        m_utf8data = NULL;
        m_utf8data_len = 0;
    }

    if (m_conn)
    {
        m_database->closeConnection(m_conn);
        m_conn = NULL;
    }
}

bool PgsqlRowInserter::flush()
{
    if (!m_conn)
        return false;

    if (!m_buf_rows)
        return true;

    int required_utf8data_len = m_wdata.length()*6;
    if (required_utf8data_len > m_utf8data_len)
    {
        m_utf8data = (char*)realloc(m_utf8data, required_utf8data_len);
        m_utf8data_len = required_utf8data_len;
    }

    size_t output_buf_size = 0;
    kl::utf8_wtoutf8(m_utf8data, m_utf8data_len, m_wdata.c_str(), m_wdata.length(), &output_buf_size);
    m_utf8data[output_buf_size] = 0;

    PQputCopyData(m_conn, m_utf8data, (int)output_buf_size);

    m_buf_rows = 0;

    return true;
}
