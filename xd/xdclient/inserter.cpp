/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-23
 *
 */



#include "xdclient.h"
#include "inserter.h"
#include "request.h"
#include "kl/url.h"
#include "kl/md5.h"
#include "../xdcommon/dbfuncs.h"


ClientRowInserter::ClientRowInserter(ClientDatabase* database, const std::wstring& path)
{
    m_database = database;
    m_database->ref();

    m_path = path;
    m_inserting = false;
    m_rows = L"";
    m_buffer_row_count = 0;
}

ClientRowInserter::~ClientRowInserter()
{
    m_database->unref();
}

xd::objhandle_t ClientRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<ClientInsertData>::iterator it;

    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        if (!wcscasecmp(it->m_col_name.c_str(), column_name.c_str()))
            return (xd::objhandle_t)&(*it);
    }

    return 0;
}

bool ClientRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                   const unsigned char* value,
                                   int length)
{
    return true;
}

static void escapedQuoteCopy(std::wstring& output, const std::string& input)
{
    size_t i, input_length = input.length();

    while (input_length > 0 && isspace((unsigned char)input[input_length-1]))
        input_length--;

    output.reserve((input_length*2)+5);

    output = L"\"";

    for (i = 0; i < input_length; ++i)
    {
        if (input[i] == '\\' || input[i] == '"')
            output += L'\\';

        output += input[i];
    }
    
    output += L'"';
}

bool ClientRowInserter::putString(xd::objhandle_t column_handle,
                                  const std::string& value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
        return false;

    escapedQuoteCopy(data->m_text, value);
    data->m_specified = true;
    return true;
}

bool ClientRowInserter::putWideString(xd::objhandle_t column_handle,
                                     const std::wstring& value)
{
    return putString(column_handle, kl::tostring(value));
}

bool ClientRowInserter::putDouble(xd::objhandle_t column_handle, double value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    wchar_t buf[64];
    swprintf(buf, 64, L"\"%.*f\"", data->m_xd_scale, value);
    buf[63] = 0;

    data->m_text = buf;
    data->m_specified = true;
    
    return true;
}

bool ClientRowInserter::putInteger(xd::objhandle_t column_handle, int value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;
    
    if (!data)
        return false;

    wchar_t buf[64];
    swprintf(buf, 64, L"\"%d\"", value);
    buf[63] = 0;

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool ClientRowInserter::putBoolean(xd::objhandle_t column_handle, bool value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    wchar_t buf[8];

    if (value)
    {
        swprintf(buf, 8, L"\"%d\"", 1);
        buf[7] = 0;
    }
    else
    {
        swprintf(buf, 8, L"\"%d\"", 0);
        buf[7] = 0;
    }

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool ClientRowInserter::putDateTime(xd::objhandle_t column_handle,
                                   xd::datetime_t datetime)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    xd::DateTime dt(datetime);

    if (dt.isNull())
    {
        data->m_text = L"null";
        data->m_specified = true;
    }
     else
    {
        wchar_t buf[64];
        swprintf(buf, 64, L"\"%04d/%02d/%02d %02d:%02d:%02d\"", 
                      dt.getYear(), dt.getMonth(), dt.getDay(), 
                      dt.getHour(), dt.getMinute(), dt.getSecond());

        data->m_text = buf;
        data->m_specified = true;
    }

    return true;
}


bool ClientRowInserter::putNull(xd::objhandle_t column_handle)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    data->m_text = L"null";
    data->m_specified = true;

    return true;
}




bool ClientRowInserter::startInsert(const std::wstring& col_list)
{
    if (m_inserting)
        return false;

    m_structure = m_database->describeTable(m_path);
    if (m_structure.isNull())
        return false;


    std::vector<std::wstring> columns;
    std::vector<std::wstring>::iterator it;
    std::wstring field_list;

    kl::parseDelimitedList(col_list, columns, L',');

    if (col_list == L"*")
    {
        columns.clear();

        size_t i, col_count = m_structure.getColumnCount();
        for (i = 0; i < col_count; ++i)
            columns.push_back(m_structure.getColumnName(i));
    }

    std::wstring scols;
    size_t c, cn = columns.size();
    for (c = 0; c < cn; ++c)
    {
        if (c > 0) scols += L",";
        scols += columns[c];
    }



    m_insert_data.clear();

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        const xd::ColumnInfo& col_info = m_structure.getColumnInfo(*it);
        if (col_info.isNull())
            return false;

        field_list += col_info.name;

        if (it+1 != columns.end())
            field_list += L",";

        ClientInsertData d;
        d.m_col_name = col_info.name;
        d.m_xd_type = col_info.type;
        d.m_xd_width = col_info.width;
        d.m_xd_scale = col_info.scale;
        d.m_text = L"null";
        d.m_specified = false;

        m_insert_data.push_back(d);
    }


    m_inserting = true;
    m_rows.reserve(16384);
    m_rows = L"[";
    m_buffer_row_count = 0;
    m_columns = scols;

    ServerCallParams params;
    params.setParam(L"columns", m_columns);
    params.setParam(L"handle", L"create");
    std::wstring sres = m_database->serverCall(m_path, L"insertrows", &params, true);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    m_handle = response["handle"];

    return true;
}

bool ClientRowInserter::insertRow()
{
    if (!m_inserting)
        return false;

    std::vector<ClientInsertData>::iterator it;
    std::vector<ClientInsertData>::iterator begin_it = m_insert_data.begin();
    std::vector<ClientInsertData>::iterator end_it = m_insert_data.end();

    if (m_buffer_row_count == 0)
        m_rows += L"[";
         else
        m_rows += L",[";


    for (it = begin_it; it != end_it; ++it)
    {
        if (it != begin_it)
            m_rows += L",";
        m_rows += it->m_text;
    }

    m_rows += L"]";
    m_buffer_row_count++;



    if (m_buffer_row_count == 1000)
    {
        if (!flush())
            return false;
    }

    // clear out values for the next row
    for (it = begin_it; it != end_it; ++it)
    {
        it->m_text = L"null";
        it->m_specified = false;
    }


    return true;
}

void ClientRowInserter::finishInsert()
{
    if (m_buffer_row_count > 0)
    {
        flush();
    }

    ServerCallParams params;
    params.setParam(L"handle", m_handle);
    std::wstring sres = m_database->serverCall(m_path, L"close", &params, true);
    kl::JsonNode response;
    response.fromString(sres);
}

bool ClientRowInserter::flush()
{
    m_rows += L"]";

    // debugging:
    // m_rows = xf_get_file_contents(L"c:\\users\\bwilliams\\rows.txt");
    // m_columns = xf_get_file_contents(L"c:\\users\\bwilliams\\columns.txt");

    ServerCallParams params;
    params.setParam(L"rows", m_rows);
    params.setParam(L"columns", m_columns);
    params.setParam(L"handle", m_handle);
    std::wstring sres = m_database->serverCall(m_path, L"insertrows", &params, true);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    m_rows = L"[";
    m_buffer_row_count = 0;

    return true;
}

