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
#include <xd/xd.h>
#include "database.h"
#include "inserter.h"
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>


PgsqlRowInserter::PgsqlRowInserter(PgsqlDatabase* db, const std::wstring& table)
{
    m_database = db;
    m_database->ref();
    
    xd::IAttributesPtr attr = m_database->getAttributes();
    m_quote_openchar = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    m_quote_closechar = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

    m_conn = NULL;

    m_buf_rows = 0;
    m_insert_rows = 0;
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

xd::objhandle_t PgsqlRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<PgsqlInsertFieldData>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals(it->m_name, column_name))
            return &(*it);
    }

    return 0;
}

bool PgsqlRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                 const unsigned char* value,
                                 int length)
{
    return false;
}

bool PgsqlRowInserter::putString(xd::objhandle_t column_handle,
                                 const std::string& value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
        return false;

    if ((int)value.length() > f->m_width)
        f->m_value = kl::towstring(value.substr(0,f->m_width));
         else
        f->m_value = kl::towstring(value);

    if (f->m_value.find_first_of(L"\n\t\\") != std::wstring::npos)
    {
        kl::replaceStr(f->m_value, L"\\", L"\\\\");
        kl::replaceStr(f->m_value, L"\n", L"\\n");
        kl::replaceStr(f->m_value, L"\t", L"\\t");
    }

    return true;
}

bool PgsqlRowInserter::putWideString(xd::objhandle_t column_handle,
                                     const std::wstring& value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
        return false;

    if ((int)value.length() > f->m_width)
        f->m_value = value.substr(0,f->m_width);
         else
        f->m_value = value;

    if (f->m_value.find_first_of(L"\n\t\\") != std::wstring::npos)
    {
        kl::replaceStr(f->m_value, L"\\", L"\\\\");
        kl::replaceStr(f->m_value, L"\n", L"\\n");
        kl::replaceStr(f->m_value, L"\t", L"\\t");
    }

    return true;
}

bool PgsqlRowInserter::putDouble(xd::objhandle_t column_handle,
                                 double value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
        return false;

    wchar_t buf[64];
    swprintf(buf, 63, L"%.*f", f->m_scale, value);

    // make this work even in european locales, where the decimal is often a comma
    for (int i = 0; i < sizeof(buf); ++i)
    {
        if (!buf[i])
            break;
        if (buf[i] == ',')
            buf[i] = '.';
    }
    
    f->m_value = buf;

    return true;
}

bool PgsqlRowInserter::putInteger(xd::objhandle_t column_handle,
                                  int value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
        return false;

    wchar_t buf[64];
    swprintf(buf, 63, L"%d", value);
    f->m_value = buf;

    return true;
}

bool PgsqlRowInserter::putBoolean(xd::objhandle_t column_handle,
                                  bool value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
        return false;

    f->m_value = value ? 'T' : 'F';

    return true;
}

bool PgsqlRowInserter::putDateTime(xd::objhandle_t column_handle,
                                   xd::datetime_t datetime)
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

    xd::DateTime dt(datetime);
    wchar_t buf[64];

    if (f->m_xd_type == xd::typeDate)
    {
        swprintf(buf, 63, L"%04d-%02d-%02d", dt.getYear(), dt.getMonth(),  dt.getDay());
        f->m_value = buf;
    }
     else if (f->m_xd_type == xd::typeDateTime) //datetime
    {
        swprintf(buf, 63, L"%04d-%02d-%02d %02d:%02d:%02d", dt.getYear(), dt.getMonth(), dt.getDay(), dt.getHour(), dt.getMinute(), dt.getSecond());
        f->m_value = buf;
    }
     else
    {
        f->m_value = L"\\N";
    }

    return true;
}

bool PgsqlRowInserter::putNull(xd::objhandle_t column_handle)
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
    xd::Structure s = m_database->describeTable(m_table);
    if (s.isNull())
        return false;

    std::vector<std::wstring> columns;
    std::vector<std::wstring>::iterator it;
    std::wstring field_list;

    kl::parseDelimitedList(col_list, columns, L',');


    if (0 == wcscmp(col_list.c_str(), L"*") || col_list.length() == 0)
    {
        columns.clear();

        size_t i, col_count = s.getColumnCount();

        for (i = 0; i < col_count; ++i)
        {
            const std::wstring& colname = s.getColumnName(i);
            if (colname != L"xdrowid")
                columns.push_back(s.getColumnName(i));
        }
    }

    m_fields.clear();

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        const xd::ColumnInfo& col_info = s.getColumnInfo(*it);
        if (col_info.isNull())
            return false;

        PgsqlInsertFieldData d;
        d.m_name = col_info.name;
        d.m_xd_type = col_info.type;
        d.m_width = col_info.width;
        d.m_scale = col_info.scale;
        d.m_value = L"\\N";

        m_fields.push_back(d);
    }


    m_conn = m_database->createConnection();
    if (!m_conn)
        return false;

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        if (it != columns.begin())
            field_list += L",";
        field_list +=  pgsqlQuoteIdentifierIfNecessary(*it);
    }


    std::wstring sql;
    sql = L"COPY %tbl% (%cols%) FROM stdin";
    kl::replaceStr(sql, L"%tbl%", pgsqlQuoteIdentifierIfNecessary(m_table));
    kl::replaceStr(sql, L"%cols%", field_list);

    PGresult* res = PQexec(m_conn, kl::toUtf8(sql));
    PQclear(res);
    m_buf_rows = 0;

    return true;
}

bool PgsqlRowInserter::insertRow()
{
    if (!m_conn)
        return false;

    if (m_insert_rows++ > 0)
        m_wdata += L"\n";

    std::vector<PgsqlInsertFieldData>::iterator it;
    std::vector<PgsqlInsertFieldData>::iterator begin_it = m_fields.begin();
    std::vector<PgsqlInsertFieldData>::iterator end_it = m_fields.end();

    for (it = begin_it; it != end_it; ++it)
    {
        if (it != begin_it)
            m_wdata += L"\t";
        m_wdata += it->m_value;

        // reset value for the next row
        it->m_value = L"\\N";
    }

    m_buf_rows++;

    if (m_buf_rows == 5000)
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

    PGresult* res = PQgetResult(m_conn);


    if (m_utf8data)
    {
        free(m_utf8data);
        m_utf8data = NULL;
        m_utf8data_len = 0;
    }


    // analyze table for row count estimates
    std::wstring command = L"ANALYZE " + pgsqlQuoteIdentifierIfNecessary(m_table);
    res = PQexec(m_conn, kl::toUtf8(command));
    PQclear(res);



    // close connection
    m_database->closeConnection(m_conn);
    m_conn = NULL;
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

    int res = PQputCopyData(m_conn, m_utf8data, (int)output_buf_size);

    m_buf_rows = 0;
    m_wdata = L"";

    return true;
}
