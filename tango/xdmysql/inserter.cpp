/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-05-22
 *
 */


#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#include <xcm/xcm.h>
#include <kl/string.h>
#include <kl/portable.h>
#include <kl/md5.h>

#include "mysql.h"
#include "tango.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/util.h"
#include "../xdcommon/columninfo.h"
#include "../xdcommon/extfileinfo.h"
#include "database.h"
#include "inserter.h"



MysqlRowInserter::MysqlRowInserter(MysqlDatabase* db, const std::wstring& table)
{
    m_mysql = NULL;
    
    m_database = db;
    m_database->ref();

    m_inserting = false;
    m_table = table;

    m_asc_insert_stmt.reserve(65535);
    m_insert_stmt.reserve(65535);
    
    tango::IAttributesPtr attr = m_database->getAttributes();
    m_quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    m_quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);
}


MysqlRowInserter::~MysqlRowInserter()
{
    if (m_mysql)
    {
        mysql_close(m_mysql);
        m_mysql = NULL;
    }

    m_database->unref();
}

tango::objhandle_t MysqlRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<MySqlInsertData>::iterator it;

    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        if (!wcscasecmp(it->m_col_name.c_str(), column_name.c_str()))
            return (tango::objhandle_t)&(*it);
    }

    return 0;
}

tango::IColumnInfoPtr MysqlRowInserter::getInfo(tango::objhandle_t column_handle)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    if (!data)
    {
        return xcm::null;
    }

    tango::IStructurePtr structure = m_database->describeTable(m_table);
    return structure->getColumnInfo(data->m_col_name);
}


bool MysqlRowInserter::putRawPtr(tango::objhandle_t column_handle,
                                 const unsigned char* value,
                                 int length)
{
/*
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    switch (data->m_tango_type)
    {
        case tango::typeCharacter:
            doubleQuoteCopy(data->m_text, (const char*)value, length);
            return true;
        // FIXME: other tango types should be filled out here
        default:
            return false;
    }
*/

    return true;
}



static void doubleQuoteCopy(std::wstring& output, const std::string& input)
{
    int i;
    int input_length = input.length();

    while (input_length > 0 && isspace((unsigned char)input[input_length-1]))
    {
        input_length--;
    }

    output.reserve((input_length*2)+5);

    output = L"'";

    for (i = 0; i < input_length; ++i)
    {
        output += input[i];
        
        if (input[i] == '\\')
        {
            output += L'\\';
        }
         else if (input[i] == '\'')
        {
            output += L'\'';
        }
    }
    
    output += L'\'';
}

bool MysqlRowInserter::putString(tango::objhandle_t column_handle,
                                 const std::string& value)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    doubleQuoteCopy(data->m_text, value);
    data->m_specified = true;
    return true;
}

bool MysqlRowInserter::putWideString(tango::objhandle_t column_handle,
                                     const std::wstring& value)
{
    return putString(column_handle, kl::tostring(value));
}

bool MysqlRowInserter::putDouble(tango::objhandle_t column_handle, double value)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    wchar_t buf[64];
    swprintf(buf, 64, L"'%.*f'", data->m_tango_scale, value);

    data->m_text = buf;
    data->m_specified = true;
    
    return true;
}

bool MysqlRowInserter::putInteger(tango::objhandle_t column_handle, int value)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;
    
    if (!data)
        return false;

    wchar_t buf[64];
    swprintf(buf, 64, L"'%d'", value);

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool MysqlRowInserter::putBoolean(tango::objhandle_t column_handle, bool value)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    wchar_t buf[8];

    if (value)
        swprintf(buf, 8, L"'%d'", 1);
          else
        swprintf(buf, 8, L"'%d'", 0);

    data->m_text = buf;
    data->m_specified = true;

    return true;
}

bool MysqlRowInserter::putDateTime(tango::objhandle_t column_handle,
                                   tango::datetime_t datetime)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    tango::DateTime dt(datetime);

    if (dt.isNull())
    {
        data->m_text = L"NULL";
        data->m_specified = true;
    }
     else
    {
        wchar_t buf[64];
        swprintf(buf, 64, L"'%d/%d/%d %02d:%02d:%02d'", 
                      dt.getYear(), dt.getMonth(), dt.getDay(), 
                      dt.getHour(), dt.getMinute(), dt.getSecond());

        data->m_text = buf;
        data->m_specified = true;
    }

    return true;
}


bool MysqlRowInserter::putNull(tango::objhandle_t column_handle)
{
    MySqlInsertData* data = (MySqlInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    data->m_text = L"NULL";
    data->m_specified = true;

    return true;
}

bool MysqlRowInserter::startInsert(const std::wstring& col_list)
{
    m_mysql = m_database->open();
    if (m_mysql == NULL)
        return false;
    
    tango::IStructurePtr s = m_database->describeTable(m_table);

    std::vector<std::wstring> columns;
    std::vector<std::wstring>::iterator it;
    std::wstring field_list;

    kl::parseDelimitedList(col_list, columns, L',');


    if (!wcscmp(col_list.c_str(), L"*"))
    {
        columns.clear();

        int col_count = s->getColumnCount();
        int i;

        for (i = 0; i < col_count; ++i)
        {
            columns.push_back(s->getColumnName(i));
        }
    }

    m_insert_data.clear();

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        tango::IColumnInfoPtr col_info = s->getColumnInfo(*it);

        if (col_info.isNull())
        {
            return false;
        }

        field_list += m_quote_openchar;
        field_list += col_info->getName();
        field_list += m_quote_closechar;

        if (it+1 != columns.end())
        {
            field_list += L",";
        }

        MySqlInsertData d;
        d.m_col_name = col_info->getName();
        d.m_tango_type = col_info->getType();
        d.m_tango_width = col_info->getWidth();
        d.m_tango_scale = col_info->getScale();
        d.m_text = L"NULL";
        d.m_specified = false;

        m_insert_data.push_back(d);
    }

    m_insert_stub = L"INSERT INTO ";
    m_insert_stub += m_quote_openchar;
    m_insert_stub += m_table;
    m_insert_stub += m_quote_closechar;
    m_insert_stub += L" (";
    m_insert_stub += field_list;
    m_insert_stub += L") VALUES (";

    m_inserting = true;

    return true;
}

bool MysqlRowInserter::insertRow()
{
    if (!m_inserting)
    {
        return false;
    }

    // make the insert statement
    std::vector<MySqlInsertData>::iterator it;
    std::vector<MySqlInsertData>::iterator begin_it = m_insert_data.begin();
    std::vector<MySqlInsertData>::iterator end_it = m_insert_data.end();

    m_insert_stmt = L"";


    size_t fields_specified = 0;
    
    for (it = begin_it; it != end_it; ++it)
    {
        if (it->m_specified)
        {
            if (fields_specified > 0)
                m_insert_stmt += L',';

            m_insert_stmt += it->m_text;
            fields_specified++;
        }
    }

    m_insert_stmt += L')';


    if (fields_specified == m_insert_data.size())
    {
        m_insert_stmt.insert(0, m_insert_stub);
    }
     else
    {
        std::wstring insert_stub = L"INSERT INTO ";
        insert_stub += m_quote_openchar;
        insert_stub += m_table;
        insert_stub += m_quote_closechar;
        insert_stub += L" (";
        for (it = begin_it; it != end_it; ++it)
        {
            if (it->m_specified)
            {
                if (fields_specified > 0)
                    insert_stub += L',';
                insert_stub += m_quote_openchar;
                insert_stub += it->m_col_name;
                insert_stub += m_quote_closechar;
                fields_specified++;
            }
        }
        insert_stub += L") VALUES (";
        m_insert_stmt.insert(0, insert_stub);
    }



    // execute the insert statement
    m_asc_insert_stmt = kl::tostring(m_insert_stmt);
    int error = mysql_real_query(m_mysql,
                                 m_asc_insert_stmt.c_str(),
                                 m_asc_insert_stmt.length());
    if (error != 0)
    {
        const char* err = mysql_error(m_mysql);
        
        return false;
    }


    // clear out values for the next row
    for (it = begin_it; it != end_it; ++it)
    {
        it->m_text = L"NULL";
        it->m_specified = false;
    }

    return true;
}


bool MysqlRowInserter::flush()
{
    return true;
}

void MysqlRowInserter::finishInsert()
{
    m_inserting = false;
}


