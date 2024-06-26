/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-03-17
 *
 */


#include <tds.h>
#include <ctpublic.h>
#include <kl/klib.h>
#include <xd/xd.h>
#include "database.h"
#include "set.h"
#include "../xdcommon/structure.h"
#include "../xdcommon/util.h"
#include "../xdcommon/dbfuncs.h"


int tds2xdType(TDSCOLUMN* tds_col);

// utility function to convert xd dates to FreeTDS dates

/* NOTE: This function is reverse engineering the tds_datecrack
   function and currently only handles dates, not datetimes */

void xd2tdsdate(const xd::datetime_t& src, TDS_DATETIME* res)
{
    xd::datetime_t julian_days = (src >> 32);
    xd::datetime_t time_ms = (src & 0xffffffff);

    //  make julian days based off of 1-1-1900
    julian_days -= 2415021;

    res->dtdays = julian_days;
}

std::wstring SqlServerSet::getSetId()
{
    return L"";
}

xd::Structure SqlServerSet::getStructure()
{
    // create new xd::IStructure
    xd::Structure s;

    if (!m_connect_info)
    {
        return xd::Structure();
    }

    // allocate socket
    TDSSOCKET* tds;
    tds = tds_alloc_socket(m_context, 512);
    tds_set_parent(tds, NULL);

    // attempt to connect to the sql server
    if (!m_connect_info || tds_connect(tds, m_connect_info) == TDS_FAIL)
    {
        fprintf(stderr, "There was a problem connecting to the server\n");
        return xd::Structure();
    }


    TDS_INT res;
    TDS_INT res_type;
    TDSCOLUMN* colinfo;
    int xd_type;
    int sql_type;
    int col_count = 0;

    // create select statement
    char query[1024];
    sprintf(query, "SELECT * FROM %s WHERE 1=0", kl::tostring(m_tablename).c_str());

    // submit the query to the sql server
    res = tds_submit_query(tds, query);

    if (res != TDS_SUCCEED)
    {
        // tds_submit_query() failed
        return xd::Structure();
    }

    // add columns to the table's structure
    res = tds_process_tokens(tds, &res_type, NULL, TDS_HANDLE_ALL);
    col_count = tds->res_info->num_cols;

    int i = 0;
    for (i = 0; i < col_count; ++i)
    {
        colinfo = tds->res_info->columns[i];
        sql_type = colinfo->column_type;
        xd_type = tds2xdType(colinfo);

        if (xd_type == xd::typeInvalid)
        {
            // certain complex types are not supported
            continue;
        }


        xd::ColumnInfo col;
        col.name = kl::towstring(colinfo->column_name);
        col.type = xd_type;

        if (xd_type == xd::typeNumeric)
        {
            col.width = colinfo->column_prec;
        }
         else
        {
            col.width = colinfo->column_size;
        }

        if (sql_type == SYBMONEYN ||
            sql_type == SYBMONEY ||
            sql_type == SYBMONEY4)
        {
            col.width = 18;
            col.scale = 4;
        }

        if (sql_type == SYBTEXT ||
            sql_type == SYBNTEXT)
        {
            // we cannot determine the length of
            // this field, so set it to a default for now
            col.width = 512;
        }


        col.scale = colinfo->column_scale;
        col.column_ordinal = i;

        s.createColumn(col);
    }

    tds_free_socket(tds);

    return s;
}

/*
bool SqlServerSet::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    KL_AUTO_LOCK(m_object_mutex);

    unsigned int processed_action_count = 0;

    xd::Structure current_struct = getStructure();
    IStructureInternalPtr s = struct_config;
    std::vector<StructureAction>& actions = s->getStructureActions();
    std::vector<StructureAction>::iterator it;

    std::wstring command;
    command.reserve(1024);

    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionDelete)
        {
            command = L"ALTER TABLE ";
            command += m_tablename;
            command += L" DROP COLUMN ";
            command += it->m_colname;

            xcm::IObjectPtr result;
            m_database->execute(command, 0, result, NULL);
            command = L"";
        }
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionModify)
        {
        }
    }

    int i;
    int col_count = current_struct->getColumnCount();
    xd::IColumnInfoPtr colinfo;
    bool found = false;

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->action == xd::StructureModify::Action::actionCreate)
        {
            for (i = 0; i < col_count; ++i)
            {
                colinfo = current_struct->getColumnInfoByIdx(i);

                if (!wcscasecmp(colinfo->getName().c_str(), it->m_params->getName().c_str()))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::wstring field_string;
                
                
                field_string = createSqlServerFieldString(it->m_params->getName(),
                                                          it->m_params->getType(),
                                                          it->m_params->getWidth(),
                                                          it->m_params->getScale(),
                                                          false);
                command = L"ALTER TABLE ";
                command += m_tablename;
                command += L" ADD ";
                command += field_string;

                xcm::IObjectPtr result;
                m_database->execute(command, 0, result, NULL);
                command = L"";
            }

            found = false;
        }
    }

    return true;
}

*/


xd::IIteratorPtr SqlServerSet::createIterator(const std::wstring& columns,
                                                 const std::wstring& order,
                                                 xd::IJob* job)
{
    std::wstring query;
    query.reserve(1024);

    query = L"SELECT * FROM ";
    query += m_tablename;

    if (order.length() > 0)
    {
        query += L" ORDER BY ";
        query += order;
    }

    SqlServerIterator* iter = new SqlServerIterator;
    iter->m_connect_info = m_connect_info;
    iter->m_context = m_context;
    iter->m_database = m_database;
    iter->m_name = m_tablename;
    iter->init(query);

    return static_cast<xd::IIterator*>(iter);
}

xd::rowpos_t SqlServerSet::getRowCount()
{
    wchar_t query[1024];
    swprintf(query, 1024, L"SELECT rows FROM sysindexes WHERE id = OBJECT_ID('%ls') AND indid < 2", m_tablename.c_str());

    SqlServerIterator* iter = new SqlServerIterator;
    iter->m_connect_info = m_connect_info;
    iter->m_context = m_context;
    iter->m_database = m_database;
    iter->m_set = this;
    iter->m_name = L"row_count_query";

    if (!iter->init(query))
    {
        delete iter;
        return 0;
    }

    iter->goFirst();

    xd::rowpos_t row_count;
    row_count = iter->getInteger(iter->getHandle(L"rows"));
    delete iter;

    return row_count;
}






SqlServerRowInserter::SqlServerRowInserter(SqlServerSet* set)
{
    m_set = set;
    m_set->ref();

    m_inserting = false;

    // allocate socket
    m_tds = tds_alloc_socket(m_set->m_context, 512);
    tds_set_parent(m_tds, NULL);

    // attempt to connect to the sql server
    if (!m_set->m_connect_info || tds_connect(m_tds, m_set->m_connect_info) == TDS_FAIL)
    {
        fprintf(stderr, "There was a problem connecting to the server\n");
    }
}

SqlServerRowInserter::~SqlServerRowInserter()
{
    // clean up
    tds_free_socket(m_tds);

    m_set->unref();
}

xd::objhandle_t SqlServerRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<SqlServerInsertData>::iterator it;

    for (it = m_insert_data.begin(); it != m_insert_data.end(); ++it)
    {
        if (!wcscasecmp(it->m_col_name.c_str(), column_name.c_str()))
            return (xd::objhandle_t)&(*it);
    }

    return 0;
}


bool SqlServerRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                     const unsigned char* value,
                                     int length)
{
    return false;
}


// this is temporary until we figure out what is going on with Unicode and FreeTDS

void doubleQuoteCopy(std::string& output, const std::string& input)
{
    int i;
    int input_length = input.length();

    while (input_length > 0 && isspace((unsigned char)input[input_length-1]))
    {
        input_length--;
    }

    output.reserve((input_length*2)+5);

    output = "'";

    for (i = 0; i < input_length; ++i)
    {
        if (input[i] == 0)
            break;

        output += input[i];

        if (input[i] == '\'')
        {
            output += '\'';
        }
    }
    
    output += '\'';
}

bool SqlServerRowInserter::putString(xd::objhandle_t column_handle,
                                     const std::string& value)
{
    SqlServerInsertData* data = (SqlServerInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    doubleQuoteCopy(data->m_text, value);

    return true;
}

bool SqlServerRowInserter::putWideString(xd::objhandle_t column_handle,
                                         const std::wstring& value)
{
    SqlServerInsertData* data = (SqlServerInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    doubleQuoteCopy(data->m_text, kl::tostring(value));

    return true;
}

bool SqlServerRowInserter::putDouble(xd::objhandle_t column_handle,
                                     double value)
{
    SqlServerInsertData* data = (SqlServerInsertData*)column_handle;

    char buf[64];
    sprintf(buf, "%.*f", data->m_xd_scale, value);

    // convert a euro decimal character to a decimal point
    char* p = buf;
    while (*p)
    {
        if (*p == ',')
            *p = '.';
        ++p;
    }

    data->m_text = buf;
    return true;
}

bool SqlServerRowInserter::putInteger(xd::objhandle_t column_handle,
                                      int value)
{
    SqlServerInsertData* data = (SqlServerInsertData*)column_handle;
    
    if (!data)
        return false;

    char buf[64];
    sprintf(buf, "%d", value);

    data->m_text = buf;
    return true;
}

bool SqlServerRowInserter::putBoolean(xd::objhandle_t column_handle,
                                      bool value)
{
    SqlServerInsertData* data = (SqlServerInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    if (value)
    {
        data->m_text = "1";
    }
     else
    {
        data->m_text = "0";
    }

    return true;
}

bool SqlServerRowInserter::putDateTime(xd::objhandle_t column_handle,
                                       xd::datetime_t datetime)
{
    SqlServerInsertData* data = (SqlServerInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    if (datetime == 0)
    {
        data->m_text = "NULL";
    }
     else
    {
        xd::DateTime dt(datetime);

        if (dt.isNull())
        {
            data->m_text = "NULL";
        }
         else
        {
            char buf[14];
            sprintf(buf, "'%d/%d/%d'", dt.getYear(), dt.getMonth(), dt.getDay());

            data->m_text = buf;
        }
    }

    return true;
}

bool SqlServerRowInserter::putNull(xd::objhandle_t column_handle)
{
    SqlServerInsertData* data = (SqlServerInsertData*)column_handle;

    if (!data)
    {
        return false;
    }

    data->m_text = "NULL";

    return true;
}

bool SqlServerRowInserter::startInsert(const std::wstring& col_list)
{
    xd::Structure s = m_set->getStructure();

    std::vector<std::wstring> columns;
    std::vector<std::wstring>::iterator it;
    std::wstring field_list;

    kl::parseDelimitedList(col_list, columns, L',');


    if (0 == wcscmp(col_list.c_str(), L"*"))
    {
        columns.clear();

        size_t i, col_count = s.getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            columns.push_back(s->getColumnName(i));
        }
    }


    m_insert_data.clear();

    for (it = columns.begin(); it != columns.end(); ++it)
    {
        const xd::ColumnInfo& col_info = s->getColumnInfo(*it);
        if (col_info.isNull())
            return false;

        field_list += col_info.name;

        if (it+1 != columns.end())
        {
            field_list += L",";
        }

        SqlServerInsertData d;
        d.m_col_name = col_info.name;
        d.m_xd_type = col_info.type;
        d.m_xd_width = col_info.width;
        d.m_xd_scale = col_info.scale;
        d.m_text = "NULL";

        m_insert_data.push_back(d);
    }


    m_insert_stub = "INSERT INTO ";
    m_insert_stub += kl::tostring(m_set->m_tablename);
    m_insert_stub += "(";
    m_insert_stub += kl::tostring(field_list);
    m_insert_stub += ") VALUES(";

    m_inserting = true;

    return true;
}

bool SqlServerRowInserter::insertRow()
{
    if (!m_inserting)
    {
        return false;
    }


    // make the insert statement
    std::vector<SqlServerInsertData>::iterator it;
    std::vector<SqlServerInsertData>::iterator begin_it = m_insert_data.begin();
    std::vector<SqlServerInsertData>::iterator end_it = m_insert_data.end();


    
    m_insert_stmt[0] = 0;
    char* insert_ptr;
    

    insert_ptr = m_insert_stmt;
    strcpy(insert_ptr, m_insert_stub.c_str());
    insert_ptr += m_insert_stub.length();

    for (it = begin_it; it != end_it; ++it)
    {
        if (it != begin_it)
        {
            *insert_ptr = ',';
            insert_ptr++;
        }

        strcpy(insert_ptr, it->m_text.c_str());
        insert_ptr += it->m_text.length();
    }

    *insert_ptr = ')';
    *(insert_ptr+1) = 0;


    // execute the insert statement

    if (tds_submit_query(m_tds, m_insert_stmt) == TDS_FAIL)
    {
        return false;
    }

    int result_type;
    while (tds_process_tokens(m_tds, &result_type, NULL, TDS_HANDLE_ALL) == TDS_SUCCEED)
    {
    }


    for (it = begin_it; it != end_it; ++it)
    {
        it->m_text = "NULL";
    }

    return true;
    
/*
    m_insert_stmt = m_insert_stub;

    for (it = begin_it; it != end_it; ++it)
    {
        if (it != begin_it)
        {
            m_insert_stmt += ',';
        }

        m_insert_stmt += it->m_text;
    }

    m_insert_stmt += ')';


    // execute the insert statement
    if (tds_submit_query(m_tds, m_insert_stmt.c_str()) == TDS_FAIL)
    {
        return false;
    }

    int result_type;
    while (tds_process_result_tokens(m_tds, &result_type) == TDS_SUCCEED)
    {
    }

    for (it = begin_it; it != end_it; ++it)
    {
        it->m_text = "NULL";
    }

    return true;
*/
}

bool SqlServerRowInserter::flush()
{
    return true;
}

void SqlServerRowInserter::finishInsert()
{
    if (!m_inserting)
        return;

    m_inserting = false;
}




