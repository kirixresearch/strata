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

tango::objhandle_t ClientRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<ClientInsertData>::iterator it;

    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        if (!wcscasecmp(it->m_col_name.c_str(), column_name.c_str()))
            return (tango::objhandle_t)&(*it);
    }

    return 0;
}

tango::IColumnInfoPtr ClientRowInserter::getInfo(tango::objhandle_t column_handle)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
        return xcm::null;

    return m_structure->getColumnInfo(data->m_col_name);
}



bool ClientRowInserter::putRawPtr(tango::objhandle_t column_handle,
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

bool ClientRowInserter::putString(tango::objhandle_t column_handle,
                                  const std::string& value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
        return false;

    escapedQuoteCopy(data->m_text, value);
    data->m_specified = true;
    return true;
}

bool ClientRowInserter::putWideString(tango::objhandle_t column_handle,
                                     const std::wstring& value)
{
    return putString(column_handle, kl::tostring(value));
}

bool ClientRowInserter::putDouble(tango::objhandle_t column_handle, double value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    wchar_t buf[64];
    swprintf(buf, 64, L"\"%.*f\"", data->m_tango_scale, value);

    data->m_text = buf;
    data->m_specified = true;
    
    return true;
}

bool ClientRowInserter::putInteger(tango::objhandle_t column_handle, int value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;
    
    if (!data)
        return false;

    wchar_t buf[64];
    swprintf(buf, 64, L"\"%d\"", value);

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool ClientRowInserter::putBoolean(tango::objhandle_t column_handle, bool value)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    wchar_t buf[8];

    if (value)
        swprintf(buf, 8, L"\"%d\"", 1);
          else
        swprintf(buf, 8, L"\"%d\"", 0);

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool ClientRowInserter::putDateTime(tango::objhandle_t column_handle,
                                   tango::datetime_t datetime)
{
    ClientInsertData* data = (ClientInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    tango::DateTime dt(datetime);

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


bool ClientRowInserter::putNull(tango::objhandle_t column_handle)
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

        int i, col_count = m_structure->getColumnCount();
        for (i = 0; i < col_count; ++i)
            columns.push_back(m_structure->getColumnName(i));
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
        tango::IColumnInfoPtr col_info = m_structure->getColumnInfo(*it);

        if (col_info.isNull())
            return false;

        field_list += col_info->getName();

        if (it+1 != columns.end())
            field_list += L",";

        ClientInsertData d;
        d.m_col_name = col_info->getName();
        d.m_tango_type = col_info->getType();
        d.m_tango_width = col_info->getWidth();
        d.m_tango_scale = col_info->getScale();
        d.m_text = L"null";
        d.m_specified = false;

        m_insert_data.push_back(d);
    }


    m_inserting = true;
    m_rows.reserve(16384);
    m_rows = L"[";
    m_buffer_row_count = 0;
    m_columns = scols;

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
}

bool ClientRowInserter::flush()
{
    m_rows += L"]";

    ServerCallParams params;
    params.setParam(L"rows", m_rows);
    params.setParam(L"columns", m_columns);
    std::wstring sres = m_database->serverCall(m_path, L"insertrows", &params);
    kl::JsonNode response;
    response.fromString(sres);

    if (!response["success"].getBoolean())
        return false;

    m_rows = L"[";
    m_buffer_row_count = 0;

    return true;
}
