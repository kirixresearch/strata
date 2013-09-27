/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-09-03
 *
 */


#include "database.h"
#include "iterator.h"
#include "inserter.h"


OdbcRowInserter::OdbcRowInserter(OdbcDatabase* db, const std::wstring& table)
{
    m_database = db;
    m_database->ref();

    m_table = table;
    m_db_type = m_database->getDatabaseType();

    m_inserting = false;

    // allocate the statement handle
    m_conn = 0;
    m_stmt = 0;
}

OdbcRowInserter::~OdbcRowInserter()
{
    if (m_stmt)
    {
        SQLCloseCursor(m_stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, m_stmt);
        m_stmt = 0;
    }

    if (m_conn)
    {
        SQLDisconnect(m_conn);
        SQLFreeConnect(m_conn);
        m_conn = 0;
    }
    
    
    std::vector<OdbcInsertFieldData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete *it;

    m_database->unref();
}

xd::objhandle_t OdbcRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<OdbcInsertFieldData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->m_name.c_str(), column_name.c_str()))
            return (xd::objhandle_t)(*it);
    }

    return 0;
}

xd::IColumnInfoPtr OdbcRowInserter::getInfo(xd::objhandle_t column_handle)
{
    return xcm::null;
}

bool OdbcRowInserter::putRawPtr(xd::objhandle_t column_handle,
                                const unsigned char* value,
                                int length)
{
    return false;
}

bool OdbcRowInserter::putString(xd::objhandle_t column_handle,
                                const std::string& value)
{
    OdbcInsertFieldData* f = (OdbcInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    if (f->m_tango_type == xd::typeWideCharacter)
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

bool OdbcRowInserter::putWideString(xd::objhandle_t column_handle,
                                    const std::wstring& value)
{
    OdbcInsertFieldData* f = (OdbcInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    if (f->m_tango_type == xd::typeCharacter)
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

bool OdbcRowInserter::putDouble(xd::objhandle_t column_handle,
                                double value)
{
    OdbcInsertFieldData* f = (OdbcInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_indicator = 0;
    f->m_dbl_val = value;
    f->m_int_val = (int)value;

    return true;
}

bool OdbcRowInserter::putInteger(xd::objhandle_t column_handle,
                                 int value)
{
    OdbcInsertFieldData* f = (OdbcInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    // exception case for Microsoft Excel
    if (m_db_type == xd::dbtypeExcel)
        f->m_dbl_val = value;

    f->m_indicator = 0;
    f->m_int_val = value;
    f->m_dbl_val = value;

    return true;
}

bool OdbcRowInserter::putBoolean(xd::objhandle_t column_handle,
                                 bool value)
{
    OdbcInsertFieldData* f = (OdbcInsertFieldData*)column_handle;
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

bool OdbcRowInserter::putDateTime(xd::objhandle_t column_handle,
                                  xd::datetime_t datetime)
{
    OdbcInsertFieldData* f = (OdbcInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_indicator = 0;

    if (datetime == 0)
    {
        // null date
        f->m_indicator = SQL_NULL_DATA;
        return true;
    }


    if (f->m_sql_c_type == SQL_C_DATE)
    {
        xd::DateTime dt(datetime);

        f->m_date_val.year = dt.getYear();
        f->m_date_val.month = dt.getMonth();
        f->m_date_val.day = dt.getDay();

        // SQL Server cannot handle dates less than 1753
        if (f->m_date_val.year < 1753)
        {
            f->m_indicator = SQL_NULL_DATA;
        }
    }
     else if (f->m_sql_c_type == SQL_C_TIMESTAMP)
    {
        xd::DateTime dt(datetime);

        f->m_datetime_val.year = dt.getYear();
        f->m_datetime_val.month = dt.getMonth();
        f->m_datetime_val.day = dt.getDay();
        f->m_datetime_val.hour = dt.getHour();
        f->m_datetime_val.minute = dt.getMinute();
        f->m_datetime_val.second = dt.getSecond();
        f->m_datetime_val.fraction = 0;
        if (f->m_tango_type == xd::typeDateTime)
        {
            f->m_datetime_val.fraction = dt.getMillisecond()*1000000;
        }

        // SQL Server cannot handle dates less than 1753
        if (f->m_datetime_val.year < 1753)
        {
            f->m_indicator = SQL_NULL_DATA;
        }
    }



    return true;
}

bool OdbcRowInserter::putNull(xd::objhandle_t column_handle)
{
    OdbcInsertFieldData* f = (OdbcInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_indicator = SQL_NULL_DATA;

    return false;
}

bool OdbcRowInserter::startInsert(const std::wstring& col_list)
{
    SQLRETURN r;
    
    // allocate a connection
    SQLAllocConnect(m_database->m_env, &m_conn);

    r = m_database->connect(m_conn);
        
    if (r == SQL_NO_DATA || r == SQL_ERROR || r == SQL_INVALID_HANDLE)
        return false;

    //r = SQLSetConnectAttr(m_conn, SQL_ATTR_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);

    r = SQLAllocHandle(SQL_HANDLE_STMT, m_conn, &m_stmt);


    m_inserting = true;


    // get the quote characters
    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);


    xd::IStructurePtr s = m_database->describeTable(m_table);
    if (s.isNull())
        return false;

    int i, col_count = s->getColumnCount();

    std::wstring field_list;
    std::wstring values_list;
    int tango_type;

    m_fields.clear();

    // create field data for each column
    for (i = 0; i < col_count; ++i)
    {
        if (i != 0)
        {
            field_list += L", ";
            values_list += L", ";
        }

        xd::IColumnInfoPtr col_info = s->getColumnInfoByIdx(i);
        
        field_list += quote_openchar;
        field_list += col_info->getName();
        field_list += quote_closechar;
        values_list += L"?";

        tango_type = col_info->getType();

        OdbcInsertFieldData* field = new OdbcInsertFieldData;
        field->m_name = col_info->getName();
        field->m_tango_type = tango_type;
        field->m_sql_c_type = tango2sqlCType(tango_type);
        field->m_sql_type = tango2sqlType(tango_type);
        field->m_width = col_info->getWidth();
        field->m_scale = col_info->getScale();
        field->m_idx = i+1;

        // initialize the storage elements for this column
        field->m_str_val = new char[field->m_width + 1];
        field->m_wstr_val = new wchar_t[field->m_width + 1];
        field->m_dbl_val = 0.0;
        field->m_int_val = 0;
        field->m_bool_val = 0;

        memset(field->m_str_val, 0, (field->m_width+1) * sizeof(char));
        memset(field->m_wstr_val, 0, (field->m_width+1) * sizeof(wchar_t));

        m_fields.push_back(field);
    }

    // quote the fieldname
    std::wstring tablename = L"";
    tablename += quote_openchar;
    tablename += m_table;
    tablename += quote_closechar;

    // create an insert statement
    wchar_t insert_stmt[65535];
    swprintf(insert_stmt, 65535, L"INSERT INTO %ls(%ls) VALUES(%ls)",
                                  tablename.c_str(),
                                  field_list.c_str(),
                                  values_list.c_str());

    std::wstring temp_insert_stmt = insert_stmt;
    r = SQLPrepare(m_stmt, sqlt(temp_insert_stmt), SQL_NTS);

    #ifdef _DEBUG
    std::string temps = kl::tostring(insert_stmt);
    testSqlStmt(m_stmt);
    #endif

    std::vector<OdbcInsertFieldData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        switch ((*it)->m_sql_c_type)
        {
            case SQL_C_CHAR:
            {
                r = SQLBindParameter(m_stmt, (*it)->m_idx,
                                             SQL_PARAM_INPUT,
                                             SQL_C_CHAR,
                                             SQL_VARCHAR,
                                             (*it)->m_width,
                                             0,
                                             (*it)->m_str_val,
                                             0,
                                             &(*it)->m_indicator);
                break;
            }

            case SQL_C_WCHAR:
            {
                r = SQLBindParameter(m_stmt, (*it)->m_idx,
                                             SQL_PARAM_INPUT,
                                             SQL_C_WCHAR,
                                             SQL_WVARCHAR,
                                             (*it)->m_width,
                                             0,
                                             (*it)->m_wstr_val,
                                             0,
                                             &(*it)->m_indicator);
                break;
            }

            case SQL_C_NUMERIC:
            {
                r = SQLBindParameter(m_stmt, (*it)->m_idx,
                                             SQL_PARAM_INPUT,
                                             SQL_C_NUMERIC,
                                             SQL_NUMERIC,
                                             sizeof(SQL_NUMERIC_STRUCT),
                                             0,
                                             &(*it)->m_num_val,
                                             0,
                                             &(*it)->m_indicator);
                break;
            }

            case SQL_C_DOUBLE:
            {
                r = SQLBindParameter(m_stmt, (*it)->m_idx,
                                             SQL_PARAM_INPUT,
                                             SQL_C_DOUBLE,
                                             SQL_DOUBLE,
                                             sizeof(double),
                                             0,
                                             &(*it)->m_dbl_val,
                                             0,
                                             &(*it)->m_indicator);
                break;
            }

            case SQL_C_SLONG:
            {
                r = SQLBindParameter(m_stmt, (*it)->m_idx,
                                             SQL_PARAM_INPUT,
                                             SQL_C_SLONG,
                                             SQL_INTEGER,
                                             sizeof(int),
                                             0,
                                             &(*it)->m_int_val,
                                             0,
                                             &(*it)->m_indicator);
                break;
            }

            case SQL_C_DATE:
            {
                r = SQLBindParameter(m_stmt, (*it)->m_idx,
                                             SQL_PARAM_INPUT,
                                             SQL_C_DATE,
                                             SQL_DATE,
                                             sizeof(SQL_DATE_STRUCT),
                                             0,
                                             &(*it)->m_date_val,
                                             sizeof(SQL_DATE_STRUCT),
                                             &(*it)->m_indicator);
                break;
            }

            case SQL_C_TIMESTAMP:
            {
                r = SQLBindParameter(m_stmt, (*it)->m_idx,
                                             SQL_PARAM_INPUT,
                                             SQL_C_TIMESTAMP,
                                             SQL_TIMESTAMP,
                                             23,
                                             0,
                                             &(*it)->m_datetime_val,
                                             sizeof(SQL_TIMESTAMP_STRUCT),
                                             &(*it)->m_indicator);
                break;
            }

            case SQL_C_BIT:
            {
                r = SQLBindParameter(m_stmt, (*it)->m_idx,
                                             SQL_PARAM_INPUT,
                                             SQL_C_BIT,
                                             SQL_BIT,
                                             (*it)->m_width,
                                             0,
                                             &(*it)->m_bool_val,
                                             0,
                                             &(*it)->m_indicator);
                break;
            }

            testSqlStmt(m_stmt);
        }
    }

    return true;
}

bool OdbcRowInserter::insertRow()
{
    if (!m_inserting)
    {
        return false;
    }

    SQLRETURN r;
    r = SQLExecute(m_stmt);

#ifdef _DEBUG
    testSqlStmt(m_stmt);
#endif

    return SQL_SUCCEEDED(r) ? true : false;
}

void OdbcRowInserter::finishInsert()
{
    m_inserting = false;

    //SQLEndTran(SQL_HANDLE_DBC, m_conn, SQL_COMMIT);
    
    if (m_stmt)
    {
        SQLCloseCursor(m_stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, m_stmt);
        m_stmt = 0;
    }

    if (m_conn)
    {
        SQLDisconnect(m_conn);
        SQLFreeConnect(m_conn);
        m_conn = 0;
    }
}

bool OdbcRowInserter::flush()
{
    //SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC, m_conn, SQL_COMMIT);
    //return (r == 0) ?  true : false;
    return true;
}






