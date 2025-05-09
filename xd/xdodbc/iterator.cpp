/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-09-03
 *
 */


#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <kl/klib.h>
#include <xd/xd.h>
#include "database.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "../xdcommon/localrowcache.h"
#include "../xdcommon/cmnbaseset.h"
#include "../xdcommon/extfileinfo.h"


const std::string empty_string = "";
const std::wstring empty_wstring = L"";


OdbcIterator::OdbcIterator(OdbcDatabase* database)
{
    m_database = database;
    m_database->ref();

    m_env = m_database->m_env;
    m_db_type =  m_database->m_db_type;

    m_conn = 0;
    m_stmt = 0;

    m_row_pos = 0;
    m_row_count = -1;  // -1 means unknown
    m_eof = false;
    
    m_bidirectional = false;
    
    m_cache_active = false;
    m_cache_dbrowpos = 0;
}

OdbcIterator::~OdbcIterator()
{
    // free up each structure containing relation info
    std::vector<OdbcIteratorRelInfo>::iterator rit;
    for (rit = m_relations.begin(); rit != m_relations.end(); ++rit)
    {
        std::vector<OdbcIteratorRelField>::iterator fit;

        // free up each field part making up a relationship
        for (fit = rit->fields.begin(); fit != rit->fields.end(); ++fit)
            releaseHandle(fit->left_handle);
    }


    // free up odbc handles

    if (m_stmt)
    {
        SQLCloseCursor(m_stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, m_stmt);
    }

    if (m_conn)
    {
        SQLDisconnect(m_conn);
        SQLFreeConnect(m_conn);
        m_conn = 0;
    }


    // clean up field vector and expression vector

    std::vector<OdbcDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
        delete (*it);

    m_database->unref();
}

bool OdbcIterator::init(const std::wstring& query)
{
    // allocate the connection
    SQLAllocConnect(m_env, &m_conn);

    // set the odbc cursor library to only be used if necessary
    //SQLSetConnectOption(m_conn, SQL_ODBC_CURSORS, SQL_CUR_USE_IF_NEEDED);
    //SQLSetConnectOption(m_conn, SQL_ODBC_CURSORS, SQL_CUR_USE_DRIVER);

    // MS Access supports bidirectional
    if (m_db_type == xd::dbtypeAccess /*|| m_db_type == xd::dbtypeMySql*/ )
    {
        m_bidirectional = true;
    }
    
    SQLRETURN retval = m_database->connect(m_conn);
    

    if (retval == SQL_NO_DATA || retval == SQL_ERROR || retval == SQL_INVALID_HANDLE)
    {
        // failed
        m_database->errorSqlConn(m_conn);
        return false;
    }


    // now that we have a connection, get the field info and bind the columns

    SQLAllocStmt(m_conn, &m_stmt);

    // set the attributes for the statement below
    //SQLSetStmtAttr(m_stmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0);
    //SQLSetStmtAttr(m_stmt, SQL_ATTR_USE_BOOKMARKS, SQL_UB_OFF, 0);
    //SQLSetStmtAttr(m_stmt, SQL_ATTR_ROWS_FETCHED_PTR, &m_rows_fetched, 0);
    //SQLSetStmtAttr(m_stmt, SQL_ATTR_ROW_ARRAY_SIZE, (void*)row_array_size, 0);
    
    if (m_bidirectional)
    {
        SQLSetStmtAttr(m_stmt,
                    SQL_ATTR_CURSOR_TYPE,
                    (SQLPOINTER)SQL_CURSOR_STATIC,
                    0);
    }
     else
    {
        SQLSetStmtAttr(m_stmt,
                    SQL_ATTR_CURSOR_TYPE,
                    (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                    0);
    }
    
    SQLSetStmtAttr(m_stmt,
                SQL_ATTR_CONCURRENCY,
                (SQLPOINTER)SQL_CONCUR_READ_ONLY,
                0);

    retval = SQLExecDirect(m_stmt, sqlt(query), SQL_NTS);

    if (retval == SQL_NO_DATA || retval == SQL_ERROR || retval == SQL_INVALID_HANDLE)
    {
        #ifdef _DEBUG
        testSqlStmt(m_stmt);
        #endif

        m_database->errorSqlStmt(m_stmt);
        
        // failed
        return false;
    }


    std::wstring col_name;
    col_name.reserve(80);
    SQLSMALLINT col_name_length;
    SQLSMALLINT col_type;
    SQLULEN col_width;
    SQLSMALLINT col_scale;
    SQLSMALLINT col_nullable;
    int col_xd_type;

    SQLSMALLINT i;
    SQLSMALLINT col_count;
    SQLNumResultCols(m_stmt, &col_count);

    for (i = 1; i <= col_count; ++i)
    {
        SQLTCHAR col_buf[512];

        retval = SQLDescribeCol(m_stmt,
                                i,
                                (SQLTCHAR*)col_buf,
                                512,
                                &col_name_length,
                                &col_type,
                                &col_width,
                                &col_scale,
                                &col_nullable);

        if (retval != SQL_SUCCESS)
        {
            #ifdef _DEBUG
            testSqlStmt(m_stmt);
            #endif
            continue;
        }

        col_name = sql2wstring(col_buf);
        col_xd_type = sql2xdType(col_type);
        
        if (col_xd_type == xd::typeInvalid)
            continue;


        if (col_type == SQL_LONGVARCHAR || 
            col_type == SQL_WLONGVARCHAR || 
            ((col_xd_type == xd::typeCharacter || col_xd_type == xd::typeWideCharacter) && col_width == 0))
        {
            // we don't know the width of this field, so set it to 255
            col_width = 255;
        }



        if (col_type == SQL_GUID ||
            col_type == SQL_BINARY || 
            col_type == SQL_VARBINARY ||
            col_type == SQL_LONGVARBINARY)
        {
            // limit the maximum column width for a binary field
            if (col_width <= 0 || col_width > 8192)
                col_width = 8192;
        }

        if (m_db_type == xd::dbtypeExcel &&
            col_scale == 0 &&
            (col_xd_type == xd::typeDouble ||
             col_xd_type == xd::typeNumeric))
        {
            // excel odbc drivers always return 0 for column scale,
            // so we will set it to a more acceptable value
            col_scale = 2;
        }

        if (m_db_type == xd::dbtypeAccess &&
            col_xd_type == xd::typeCharacter)
        {
            // access always uses Wide Characters
            col_xd_type = xd::typeWideCharacter;
        }

        // handle column scale
        if (col_xd_type != xd::typeNumeric &&
            col_xd_type != xd::typeDouble)
        {
            col_scale = 0;
        }

        if (col_xd_type == xd::typeNumeric)
        {
            // numeric fields have a max width of 18
            col_width > 18 ? col_width = 18 : 0;
        }

        OdbcDataAccessInfo* field = new OdbcDataAccessInfo;
        field->name = col_name;
        field->odbc_type = col_type;
        field->type = col_xd_type;
        field->width = (int)col_width;
        field->scale = col_scale;
        field->ordinal = i;

        field->str_val = new char[(col_width+1)*sizeof(char)];
        field->wstr_val = new wchar_t[(col_width+1)*sizeof(wchar_t)];
        memset(field->str_val, 0, (col_width+1)*sizeof(char));
        memset(field->wstr_val, 0, (col_width+1)*sizeof(wchar_t));
        field->int_val = 0;
        field->dbl_val = 0.0;
        field->bool_val = 0;
        memset(&field->date_val, 0, sizeof(SQL_DATE_STRUCT));
        memset(&field->datetime_val, 0, sizeof(SQL_TIMESTAMP_STRUCT));

        m_fields.push_back(field);
    }


    // bind the columns to their return values

    std::vector<OdbcDataAccessInfo*>::iterator it;

    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        switch ((*it)->type)
        {
            case xd::typeCharacter:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_CHAR,
                                    (*it)->str_val,
                                    ((*it)->width+1)*sizeof(char),
                                    &(*it)->indicator);
                break;
            }

            case xd::typeWideCharacter:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_WCHAR,
                                    (*it)->wstr_val,
                                    ((*it)->width+1)*sizeof(wchar_t),
                                    &(*it)->indicator);
                break;
            }

            case xd::typeNumeric:
            case xd::typeDouble:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_DOUBLE,
                                    &(*it)->dbl_val,
                                    sizeof(double),
                                    &(*it)->indicator);
                break;
            }

            case xd::typeInteger:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_SLONG,
                                    &(*it)->int_val,
                                    sizeof(int),
                                    &(*it)->indicator);
                break;
            }

            case xd::typeDate:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_DATE,
                                    &(*it)->date_val,
                                    sizeof(SQL_DATE_STRUCT),
                                    &(*it)->indicator);
                break;
            }

            case xd::typeDateTime:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_TIMESTAMP,
                                    &(*it)->datetime_val,
                                    sizeof(SQL_TIMESTAMP_STRUCT),
                                    &(*it)->indicator);

                break;
            }

            case xd::typeBoolean:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_BIT,
                                    &(*it)->bool_val,
                                    sizeof(bool),
                                    &(*it)->indicator);
                break;
            }
        }
    }

    if (m_table.length() > 0)
    {
        std::wstring object_id = m_database->getObjectIdFromTableName(m_table);
        xd::IAttributesPtr attr = m_database->getAttributes();
        std::wstring config_file_path = ExtFileInfo::getConfigFilenameFromSetId(attr->getStringAttribute(xd::dbattrDefinitionDirectory), object_id);
        CommonBaseSet cbs;
        cbs.setConfigFilePath(config_file_path);
        
        xd::Structure s;
        cbs.appendCalcFields(s);

        for (auto& col : s.columns)
        {
            OdbcDataAccessInfo* dai = new OdbcDataAccessInfo;
            dai->name = col.name;
            dai->type = col.type;
            dai->width = col.width;
            dai->scale = col.scale;
            dai->ordinal = m_fields.size()+1;
            dai->str_val = new char[(dai->width+1)*sizeof(char)];
            dai->wstr_val = new wchar_t[(dai->width+1)*sizeof(wchar_t)];
            memset(dai->str_val, 0, (dai->width+1)*sizeof(char));
            memset(dai->wstr_val, 0, (dai->width+1)*sizeof(wchar_t));
            dai->int_val = 0;
            dai->dbl_val = 0.0;
            dai->bool_val = 0;
            memset(&dai->date_val, 0, sizeof(SQL_DATE_STRUCT));
            memset(&dai->datetime_val, 0, sizeof(SQL_TIMESTAMP_STRUCT));
            dai->expr_text = col.expression;
            m_fields.push_back(dai);
        }

        // now parse expressions in calculated fields

        for (auto& field : m_fields)
        {
            if (field->expr_text.length() > 0)
            {
                field->expr = parse(field->expr_text);
            }
        }
    }




    // position cursor at the beginning of the table
    clearFieldData();


    
    if (m_bidirectional)
    {
        goFirst();
    }
     else
    {
        skip(1);
        m_row_pos = 0;
    }


    
    return true;
}


bool OdbcIterator::init(const xd::QueryParams& qp)
{
    std::wstring query;
    query.reserve(1024);

    std::wstring tablename = getTablenameFromOfsPath(qp.from);
    m_table = tablename;

    if (m_database->m_db_type == xd::dbtypeAccess)
    {
        xd::Structure s = m_database->describeTable(qp.from);
        if (s.isNull())
            return xcm::null;

        size_t i, cnt = s.getColumnCount();

        query = L"SELECT ";
        for (i = 0; i < cnt; ++i)
        {
            const xd::ColumnInfo& colinfo = s.getColumnInfoByIdx(i);

            if (colinfo.calculated)
                continue;

            if (i != 0)
            {
                query += L",";
            }

            if (colinfo.type == xd::typeNumeric || colinfo.type == xd::typeDouble)
            {
                query += L"IIF(ISNUMERIC([";
                query += colinfo.name;
                query += L"]),VAL(STR([";
                query += colinfo.name;
                query += L"])),null) AS ";
                query += L"[";
                query += colinfo.name;
                query += L"] ";
            }
            else
            {
                query += L"[";
                query += colinfo.name;
                query += L"]";
            }
        }

        query += L" FROM ";
        query += L"[";
        query += tablename;
        query += L"]";
    }
    else if (m_database->m_db_type == xd::dbtypeExcel)
    {
        query = L"SELECT * FROM ";
        query += L"\"";
        query += tablename;
        query += L"$\"";
    }
    else if (m_database->m_db_type == xd::dbtypeOracle)
    {
        xd::Structure s = m_database->describeTable(qp.from);

        size_t i, cnt = s.getColumnCount();

        query = L"SELECT ";
        for (i = 0; i < cnt; ++i)
        {
            if (i != 0)
                query += L",";

            query += s.getColumnName(i);
        }

        query += L" FROM ";
        query += tablename;
    }
    else if (m_database->m_db_type == xd::dbtypeSqlServer)
    {
        xd::IAttributesPtr attr = m_database->getAttributes();
        std::wstring quote_openchar = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
        std::wstring quote_closechar = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

        query = L"SELECT * FROM ";
        query += quote_openchar;
        query += tablename;
        query += quote_closechar;
        query += L" WITH (NOLOCK)";
    }
    else
    {
        xd::IAttributesPtr attr = m_database->getAttributes();
        std::wstring quote_openchar = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
        std::wstring quote_closechar = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);

        query = L"SELECT * FROM ";
        query += quote_openchar;
        query += tablename;
        query += quote_closechar;
    }

    if (qp.where.length() > 0)
    {
        query += L" WHERE ";
        query += qp.where;
    }

    if (qp.order.length() > 0)
    {
        query += L" ORDER BY ";
        query += qp.order;
    }

    bool res = init(query);
    if (!res)
        return false;

    if (m_database->m_db_type == xd::dbtypeSqlServer && (qp.executeFlags & xd::sqlBrowse) && qp.where.length() == 0)
    {
        // calculate row count
        HDBC conn = NULL;
        SQLAllocConnect(m_env, &conn);

        SQLRETURN retval = m_database->connect(conn);

        if (SQL_SUCCEEDED(retval))
        {
            HSTMT stmt;
            SQLAllocStmt(conn, &stmt);

            xd::IAttributesPtr attr = m_database->getAttributes();
            std::wstring quote_openchar = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
            std::wstring quote_closechar = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);
            std::wstring query = L"select count(*) from " + quote_openchar + tablename + quote_closechar;
            retval = SQLExecDirect(stmt, sqlt(query), SQL_NTS);

            if (SQL_SUCCEEDED(retval))
            {
                SQLFetch(stmt);

                unsigned long row_count = 0;
                if (SQL_SUCCEEDED(SQLGetData(stmt, 1, SQL_C_ULONG, &row_count, 0, NULL)))
                {
                    m_row_count = row_count;
                }
            }

            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        }

        SQLDisconnect(conn);
    }


    return true;
}






void OdbcIterator::setTable(const std::wstring& tbl)
{
    m_path = tbl;
}

std::wstring OdbcIterator::getTable()
{
    return m_path;
}

xd::rowpos_t OdbcIterator::getRowCount()
{
    return m_row_count >= 0 ? m_row_count : 0;
}

xd::IDatabasePtr OdbcIterator::getDatabase()
{
    return m_database;
}

xd::IIteratorPtr OdbcIterator::clone()
{
    return xcm::null;
}


void OdbcIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    m_cache_active = ((mask & value & xd::ifReverseRowCache) != 0) ? true : false;
}
    
    
unsigned int OdbcIterator::getIteratorFlags()
{
    unsigned int flags = 0;

    if (m_row_count >= 0)
    {
        flags |= xd::ifFastRowCount;
    }


    // if iterator is bidirectional, then return 0, meaning
    // backwards and forwards scrolling is ok
    if (m_bidirectional)
    {
        return flags;
    }
    
    // if we have a unidirectional iterator, but the back-scroll
    // cache is on, then we still can scroll back
    if (m_cache_active)
    {
        return flags;
    }
    
    // otherwise, indicate that this iterator is forward-only
    return flags | xd::ifForwardOnly;
}

void OdbcIterator::clearFieldData()
{
    std::vector<OdbcDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        (*it)->str_result = "";
        (*it)->wstr_result = L"";
        if ((*it)->str_val)
            (*it)->str_val[0] = 0;
        if ((*it)->wstr_val)
            (*it)->wstr_val[0] = 0;
        (*it)->int_val = 0;
        (*it)->dbl_val = 0.0;
        (*it)->bool_val = 0;
        memset(&(*it)->date_val, 0, sizeof(SQL_DATE_STRUCT));
        memset(&(*it)->time_val, 0, sizeof(SQL_TIME_STRUCT));
        memset(&(*it)->datetime_val, 0, sizeof(SQL_TIMESTAMP_STRUCT));
    }
}



void OdbcIterator::saveRowToCache()
{
    if (m_row_pos < m_cache.getRowCount())
    {
        // row already saved
        return;
    }

    if (m_eof)
    {
        // don't save rows past the eof
        return;
    }


    m_cache.createRow();
    
    
    std::vector<OdbcDataAccessInfo*>::iterator it, it_begin, it_end;
    it_begin = m_fields.begin();
    it_end = m_fields.end();

    int width;

    // place the row into the cache
    for (it = it_begin; it != it_end; ++it)
    {
        // work only on physical odbc fields
        if ((*it)->expr_text.length() > 0 || (*it)->expr)
            continue;

        if ((*it)->indicator == SQL_NULL_DATA)
        {
            m_cache.appendNullColumn();
            continue;
        }

        switch ((*it)->type)
        {
            case xd::typeCharacter:
                if ((*it)->indicator == SQL_NTS)
                    width = (int)strlen((*it)->str_val);
                     else
                    width = (int)((*it)->indicator);
                    
                if (width >= (*it)->width)
                    width = (*it)->width;
                    
                m_cache.appendColumnData((unsigned char*)((*it)->str_val),
                                         width*sizeof(char));
                break;

            case xd::typeWideCharacter:
                if ((*it)->indicator == SQL_NTS)
                    width = (int)wcslen((*it)->wstr_val) * sizeof(wchar_t);
                     else
                    width = (int)((*it)->indicator);
                
                if (width >= (*it)->width * (int)sizeof(wchar_t))
                    width = (*it)->width * (int)sizeof(wchar_t);
                    
                m_cache.appendColumnData((unsigned char*)((*it)->wstr_val),
                                          width);
                break;

            case xd::typeNumeric:
            case xd::typeDouble:
                m_cache.appendColumnData((unsigned char*)&((*it)->dbl_val),
                                         sizeof(double));
                break;

            case xd::typeInteger:
                m_cache.appendColumnData((unsigned char*)&((*it)->int_val),
                                         sizeof(int));
                break;

            case xd::typeDate:
                m_cache.appendColumnData((unsigned char*)&((*it)->date_val),
                                         sizeof(SQL_DATE_STRUCT));
                break;

            case xd::typeDateTime:
                m_cache.appendColumnData((unsigned char*)&((*it)->datetime_val),
                                         sizeof(SQL_TIMESTAMP_STRUCT));
                break;

            case xd::typeBoolean:
                m_cache.appendColumnData((unsigned char*)&((*it)->bool_val),
                                         sizeof(bool));
                break;
        }
    }

    m_cache.finishRow();
}

void OdbcIterator::readRowFromCache(xd::rowpos_t row)
{
    m_cache.goRow((xd::rowpos_t)row);
    m_cache.getRow(m_cache_row);

    bool is_null;
    unsigned int data_size;
    unsigned char* data;

    std::vector<OdbcDataAccessInfo*>::iterator it, it_begin, it_end;
    it_begin = m_fields.begin();
    it_end = m_fields.end();
    int col = 0;
    
    // read cached data into the dai values
    for (it = it_begin; it != it_end; ++it)
    {
        // work only on physical odbc fields
        if ((*it)->expr_text.length() > 0 || (*it)->expr)
            continue;

        data = m_cache_row.getColumnData(col, &data_size, &is_null);
        ++col;
        
        if (!data)
        {
            is_null = true;
        }
        
        if (is_null)
        {
            (*it)->indicator = SQL_NULL_DATA;
            continue;
        }
         else
        {
            (*it)->indicator = data_size;
        }
        
        switch ((*it)->type)
        {
            case xd::typeCharacter:
                memcpy((*it)->str_val, data, data_size);
                break;

            case xd::typeWideCharacter:
                memcpy((*it)->wstr_val, data, data_size);
                //data_size /= sizeof(wchar_t);
                (*it)->indicator = data_size;
                break;


            case xd::typeNumeric:
            case xd::typeDouble:
                (*it)->dbl_val = *(double*)data;
                break;

            case xd::typeInteger:
                (*it)->int_val = *(int*)data;
                break;

            case xd::typeDate:
                memcpy(&((*it)->date_val), data, sizeof(SQL_DATE_STRUCT));
                break;

            case xd::typeDateTime:
                memcpy(&((*it)->datetime_val), data, sizeof(SQL_TIMESTAMP_STRUCT));
                break;

            case xd::typeBoolean:
                (*it)->bool_val = *(bool*)data;
                break;
        }
    }
}

void OdbcIterator::skipWithCache(int delta)
{    
    if (delta == 0)
        return;
    
    if (!m_cache.isOk())
    {
        if (!m_cache.init())
        {
            // init failed, deactivate cache
            m_cache_active = false;
        }
    }   

    // save row to cache if necessary
    saveRowToCache();
    
    // clear out our current dai values
    clearFieldData();
    
    
    long long desired_row = ((long long)m_row_pos) + delta;
    if (desired_row < 0)
        desired_row = 0;
    
    if ((xd::rowpos_t)desired_row < m_cache.getRowCount())
    {
        readRowFromCache(desired_row);

        m_row_pos = desired_row;
        m_eof = false;
    }
     else
    {        
        int i;
        SQLRETURN r;

        // fetch the row from the database, adding new rows
        // to the cache along the way
        
        for (i = 0; i < delta; ++i)
        {
            saveRowToCache();
            
            r = SQLFetch(m_stmt);
            m_row_pos++;
            if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
            {
                #ifdef _DEBUG
                testSqlStmt(m_stmt);
                #endif
                
                m_eof = true;
                return;
            }
        }        
    }
}

void OdbcIterator::skip(int delta)
{
    if (m_cache_active)
    {
        skipWithCache(delta);
        return;
    }


    if (m_eof)
        return;
    
    if (delta == 0)
        return;
    
    SQLRETURN r;
    int i;
    
    if (m_bidirectional)
    {
        if (delta < 0)
        {
            for (i = 0; i < -delta; ++i)
            {
                clearFieldData();
                
                r = SQLFetchScroll(m_stmt, SQL_FETCH_PRIOR, 0);
                
                if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
                {
                    m_eof = true;
                    return;
                }
            }
        }
         else
        {
            for (i = 0; i < delta; ++i)
            {
                clearFieldData();
                
                r = SQLFetchScroll(m_stmt, SQL_FETCH_NEXT, 0);
                
                if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
                {
                    m_eof = true;
                    return;
                }
            }
        }
    }
     else
    {
        for (i = 0; i < delta; ++i)
        {
            clearFieldData();
            
            r = SQLFetch(m_stmt);
            m_row_pos++;
            
            if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
            {
                #ifdef _DEBUG
                testSqlStmt(m_stmt);
                #endif
                
                m_eof = true;
                return;
            }
        }

    }
}

void OdbcIterator::goFirst()
{
    if (m_cache_active)
    {
        int delta = (int)-((long long)m_row_pos);
        skip(delta);
        return;
    }
    
    if (m_bidirectional)
    {
        m_row_pos = 0;
        clearFieldData();
        SQLRETURN r = SQLFetchScroll(m_stmt, SQL_FETCH_FIRST, 0);
        
        m_eof = (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) ? true : false;

        #ifdef _DEBUG
        if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
            testSqlStmt(m_stmt);
        #endif
    }
}

void OdbcIterator::goLast()
{
}

xd::rowid_t OdbcIterator::getRowId()
{
    return 0;
}

bool OdbcIterator::bof()
{
    return false;
}

bool OdbcIterator::eof()
{
    if (m_eof)
    {
        return true;
    }

    return false;
}

bool OdbcIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool OdbcIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool OdbcIterator::setPos(double pct)
{
    return false;
}

void OdbcIterator::goRow(const xd::rowid_t& rowid)
{
}

double OdbcIterator::getPos()
{
    return (double)(long long)m_row_pos;
}

xd::Structure OdbcIterator::getStructure()
{
    if (m_structure.isOk())
        return m_structure;

    std::vector<OdbcDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((*it)->isCalculated())
        {
            xd::ColumnInfo col;

            col.name = (*it)->name;
            col.type = (*it)->type;
            col.width = (*it)->width;
            col.scale = (*it)->scale;
            col.expression = (*it)->expr_text;
            col.calculated = true;
            col.column_ordinal = (*it)->ordinal - 1;

            m_structure.createColumn(col);
        }
         else
        {
            // generate column info from the
            // field info from the query result
            xd::ColumnInfo col = createColInfo(m_db_type,
                                               (*it)->name,
                                               (*it)->odbc_type,
                                               (*it)->width,
                                               (*it)->scale,
                                               (*it)->expr_text,
                                               -1);

            col.column_ordinal = (*it)->ordinal - 1;
            m_structure.createColumn(col);
        }
    }
    
    return m_structure;
}

bool OdbcIterator::refreshStructure()
{
    if (m_table.length() > 0)
    {
        // force getStructure to re-read the structure
        m_structure.clear();

        // remove all calculated fields because we will re-add them below
        for (size_t i = 0; i < m_fields.size(); )
        {
            if (m_fields[i]->expr_text.length() > 0)
            {
                //delete m_fields[i]; // free memory if field is being removed
                m_fields.erase(m_fields.begin() + i); // erase the element and adjust the iterator
            }
            else
            {
                ++i;
            }
        }


        std::wstring object_id = m_database->getObjectIdFromTableName(m_table);
        xd::IAttributesPtr attr = m_database->getAttributes();
        std::wstring config_file_path = ExtFileInfo::getConfigFilenameFromSetId(attr->getStringAttribute(xd::dbattrDefinitionDirectory), object_id);
        CommonBaseSet cbs;
        cbs.setConfigFilePath(config_file_path);

        xd::Structure s;
        cbs.appendCalcFields(s);

        for (auto& col : s.columns)
        {
            OdbcDataAccessInfo* dai = new OdbcDataAccessInfo;
            dai->name = col.name;
            dai->type = col.type;
            dai->width = col.width;
            dai->scale = col.scale;
            dai->ordinal = m_fields.size() + 1;
            dai->str_val = new char[(dai->width + 1) * sizeof(char)];
            dai->wstr_val = new wchar_t[(dai->width + 1) * sizeof(wchar_t)];
            memset(dai->str_val, 0, (dai->width + 1) * sizeof(char));
            memset(dai->wstr_val, 0, (dai->width + 1) * sizeof(wchar_t));
            dai->int_val = 0;
            dai->dbl_val = 0.0;
            dai->bool_val = 0;
            memset(&dai->date_val, 0, sizeof(SQL_DATE_STRUCT));
            memset(&dai->datetime_val, 0, sizeof(SQL_TIMESTAMP_STRUCT));
            dai->expr_text = col.expression;
            m_fields.push_back(dai);
        }

        // now parse expressions in calculated fields

        for (auto& field : m_fields)
        {
            if (field->expr_text.length() > 0)
            {
                field->expr = parse(field->expr_text);
            }
        }
    }

    return true;
}

bool OdbcIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    std::vector<xd::StructureModify::Action>::const_iterator it;
    std::vector<OdbcDataAccessInfo*>::iterator it2;
    
    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionDelete)
            continue;

        for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
        {
            if (kl::iequals(it->column, (*it2)->name))
            {
                OdbcDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // handle modify
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionModify)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (kl::iequals(it->column, (*it2)->name))
            {
                if (it->params.mask & xd::ColumnInfo::maskName)
                {
                    std::wstring new_name = it->params.name;
                    kl::makeUpper(new_name);
                    (*it2)->name = new_name;
                }

                if (it->params.mask & xd::ColumnInfo::maskType)
                {
                    (*it2)->type = it->params.type;
                }

                if (it->params.mask & xd::ColumnInfo::maskWidth)
                {
                    (*it2)->width = it->params.width;
                }

                if (it->params.mask & xd::ColumnInfo::maskScale)
                {
                    (*it2)->scale = it->params.scale;
                }

                if (it->params.mask & xd::ColumnInfo::maskExpression)
                {
                    if ((*it2)->expr)
                        delete (*it2)->expr;
                    (*it2)->expr_text = it->params.expression;
                    (*it2)->expr = parse(it->params.expression);
                }
            }
        }
    }

    // handle create
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionCreate)
            continue;

        if (it->params.expression.length() > 0)
        {
            OdbcDataAccessInfo* dai = new OdbcDataAccessInfo;
            dai->name = it->params.name;
            dai->type = it->params.type;
            dai->width = it->params.width;
            dai->scale = it->params.scale;
            dai->ordinal = (int)m_fields.size();
            dai->expr_text = it->params.expression;
            dai->expr = parse(it->params.expression);
                
            dai->str_val = new char[(dai->width+1)*sizeof(char)];
            dai->wstr_val = new wchar_t[(dai->width+1)*sizeof(wchar_t)];
            memset(dai->str_val, 0, (dai->width+1)*sizeof(char));
            memset(dai->wstr_val, 0, (dai->width+1)*sizeof(wchar_t));
            dai->int_val = 0;
            dai->dbl_val = 0.0;
            dai->bool_val = 0;
            memset(&dai->date_val, 0, sizeof(SQL_DATE_STRUCT));
            memset(&dai->datetime_val, 0, sizeof(SQL_TIMESTAMP_STRUCT));

            m_fields.push_back(dai);
        }
    }
    
    // the next call to getStructure() will refresh m_structure
    m_structure = xd::Structure();
    
    return true;
}



xd::objhandle_t OdbcIterator::getHandle(const std::wstring& expr)
{
    std::vector<OdbcDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (xd::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        OdbcDataAccessInfo* dai = new OdbcDataAccessInfo;
        dai->expr = NULL;
        dai->expr_text = expr;
        dai->type = xd::typeBinary;
        dai->key_layout = new KeyLayout;

        if (!dai->key_layout->setKeyExpr(static_cast<xd::IIterator*>(this),
                                    expr.substr(4),
                                    false))
        {
            delete dai;
            return 0;
        }
        
        m_exprs.push_back(dai);
        return (xd::objhandle_t)dai;
    }
    
    
    kscript::ExprParser* parser = parse(expr);
    if (!parser)
    {
        return (xd::objhandle_t)0;
    }

    OdbcDataAccessInfo* dai = new OdbcDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2xdType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool OdbcIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<OdbcDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
        {
            return true;
        }
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        if ((xd::objhandle_t)(*it) == data_handle)
        {
            delete (*it);
            m_exprs.erase(it);
            return true;
        }
    }

    return false;
}

xd::ColumnInfo OdbcIterator::getInfo(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::ColumnInfo();

    // try to get the column information from the table structure

    if (m_structure.isNull())
    {
        // cause m_structure to be populated; see getStructure() for details
        xd::Structure s = getStructure();
    }

    if (m_structure.isOk())
    {
        const xd::ColumnInfo& colinfo = m_structure.getColumnInfo(dai->name);
        if (colinfo.isOk())
            return colinfo;
    }


    // generate column information from our internal info

    return createColInfo(m_db_type,
                         dai->name,
                         dai->type,
                         dai->width,
                         dai->scale,
                         dai->expr_text,
                         -1);
}

int OdbcIterator::getType(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;
    
    return dai->type;
}

int OdbcIterator::getRawWidth(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* OdbcIterator::getRawPtr(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return NULL;
    }
    
    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return (const unsigned char*)dai->str_val;
}


static unsigned char hex2byte(unsigned char b1, unsigned char b2)
{
    if (b1 >= '0' && b1 <= '9')
        b1 -= '0';
    else if (b1 >= 'A' && b1 <= 'F')
        b1 = b1 - 55;
    else
        return 0;
    
    if (b2 >= '0' && b2 <= '9')
        b2 -= '0';
    else if (b2 >= 'A' && b2 <= 'F')
        b2 = b2 - 55;
    else
        return 0;
    
    return (b1*16)+b2;
}

static void decodeHexString(const char* buf, std::string& out)
{
    out = "";
    char ch1, ch2;
    
    while (*buf)
    {
        ch1 = (char)towupper(*buf);
        
        ++buf;
        if (!*buf)
            break;
            
        ch2 = (char)towupper(*buf);
    
        out += (char)hex2byte((unsigned char)ch1, (unsigned char)ch2);
    
        ++buf;
    }
}

const std::string& OdbcIterator::getString(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_string;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        dai->str_result = kl::tostring(dai->expr_result.getString());
        return dai->str_result;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return empty_string;
    }

    int width = 0;

    if (dai->indicator == SQL_NULL_DATA)
        return empty_string;
    if (dai->indicator == SQL_NTS)
        width = (int)strlen(dai->str_val);
         else
        width = (int)(dai->indicator);

    if (width > dai->width)
        width = dai->width;
    
    if (m_db_type == xd::dbtypeMySql &&
        (dai->odbc_type == SQL_BINARY || dai->odbc_type == SQL_VARBINARY || dai->odbc_type == SQL_LONGVARBINARY))
    {
        decodeHexString(dai->str_val, dai->str_result);
        return dai->str_result;   
    }
    
    if (dai->type == xd::typeCharacter)
    {
        dai->str_result.assign(dai->str_val, width);
        return dai->str_result;
    }
     else if (dai->type == xd::typeWideCharacter)
    {
        dai->wstr_result.assign(dai->wstr_val, width);
        dai->str_result = kl::tostring(dai->wstr_result);
        return dai->str_result;
    }

    return empty_string;
}

const std::wstring& OdbcIterator::getWideString(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return empty_wstring;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        dai->wstr_result = dai->expr_result.getString();
        return dai->wstr_result;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return empty_wstring;
    }

    int width = 0;

    if (dai->indicator == SQL_NULL_DATA)
        return empty_wstring;
    if (dai->indicator == SQL_NTS)
        width = (int)wcslen(dai->wstr_val);
         else
        width = (int)(dai->indicator/sizeof(wchar_t));

    if (dai->type == xd::typeCharacter)
    {
        dai->wstr_result = kl::towstring(getString(data_handle));
        return dai->wstr_result;
    }
     else if (dai->type == xd::typeWideCharacter)
    {
        dai->wstr_result.assign(dai->wstr_val, width);
        return dai->wstr_result;
    }

    return empty_wstring;
}

xd::datetime_t OdbcIterator::getDateTime(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        kscript::ExprDateTime edt = dai->expr_result.getDateTime();

        xd::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        if (dai->type == xd::typeDateTime)
            dt |= edt.time;

        return dt;
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0;
    }

    if (dai->indicator == SQL_NULL_DATA)
        return 0;


    if (dai->type == SQL_TYPE_TIME)
    {
        // not yet supported
        return 0;
    }


    if (dai->type == xd::typeDate)
    {
        if (dai->date_val.year == 0)
            return 0;

        xd::DateTime dt(dai->date_val.year,
                           dai->date_val.month,
                           dai->date_val.day);
        return dt;
    }
     else
    {    
        if (dai->datetime_val.year == 0)
            return 0;

        xd::DateTime dt(dai->datetime_val.year,
                           dai->datetime_val.month,
                           dai->datetime_val.day,
                           dai->datetime_val.hour,
                           dai->datetime_val.minute,
                           dai->datetime_val.second,
                           dai->datetime_val.fraction/1000000);
        return dt;
    }
}

double OdbcIterator::getDouble(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0.0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getDouble();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0.0;
    }

    if (dai->indicator == SQL_NULL_DATA)
        return 0.0;

    if (dai->type == xd::typeNumeric ||
        dai->type == xd::typeDouble)
    {
        return dai->dbl_val;
    }
     else if (dai->type == xd::typeInteger)
    {
        return (double)dai->int_val;
    }

    return 0;
}

int OdbcIterator::getInteger(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return 0;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getInteger();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return 0;
    }

    if (dai->indicator == SQL_NULL_DATA)
        return 0;

    if (dai->type == xd::typeInteger)
    {
        return dai->int_val;
    }
     else if (dai->type == xd::typeNumeric ||
              dai->type == xd::typeDouble)
    {
        return (int)dai->dbl_val;
    }

    return 0;
}

bool OdbcIterator::getBoolean(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return false;
    }

    if (dai->expr)
    {
        dai->expr->eval(&dai->expr_result);
        return dai->expr_result.getBoolean();
    }

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return false;
    }

    if (dai->indicator == SQL_NULL_DATA)
        return false;

    return dai->bool_val ? true : false;
}

bool OdbcIterator::isNull(xd::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return true;
    }

    if (dai->expr)
        return false;

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return true;
    }

    if (dai->indicator == SQL_NULL_DATA)
        return true;

    return false;
}

/*
xd::IxSetPtr OdbcIterator::getChildSet(xd::IRelationPtr relation)
{
    if (eof())
        return xcm::null;

    xd::IxSetPtr setptr = relation->getRightSetPtr();
    IOdbcSetPtr set = setptr;
    if (set.isNull())
        return xcm::null;


    OdbcIteratorRelInfo* info = NULL;
    std::vector<OdbcIteratorRelInfo>::iterator it;
    for (it = m_relations.begin(); it != m_relations.end(); ++it)
    {
        if (it->relation_id == relation->getRelationId())
        {
            info = &(*it);
            break;
        }
    }

    if (!info)
    {
        OdbcIteratorRelInfo relinfo;
        relinfo.relation_id = relation->getRelationId();


        std::vector<std::wstring> left_parts, right_parts;
        size_t i, cnt;

        kl::parseDelimitedList(relation->getLeftExpression(),  left_parts, L',', true);
        kl::parseDelimitedList(relation->getRightExpression(), right_parts, L',', true);

        // the number of parts in the left expression must match the count in the right expression
        if (left_parts.size() != right_parts.size())
            return xcm::null; 

        cnt = right_parts.size();
        for (i = 0; i < cnt; ++i)
        {
            OdbcIteratorRelField f;
            f.right_field = right_parts[i];
            f.left_handle = getHandle(left_parts[i]);
            if (!f.left_handle)
                return xcm::null;
            f.left_type = ((OdbcDataAccessInfo*)(f.left_handle))->type;
            
            relinfo.fields.push_back(f);
        }

        m_relations.push_back(relinfo);
        info = &(*m_relations.rbegin());
    }


    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);


    std::wstring expr;

    // build expression
    std::vector<OdbcIteratorRelField>::iterator fit;
    for (fit = info->fields.begin(); fit != info->fields.end(); ++fit)
    {
        if (expr.length() > 0)
            expr += L" AND ";
        expr += quote_openchar + fit->right_field + quote_closechar + L"=";



        switch (fit->left_type)
        {
            case xd::typeCharacter:
            case xd::typeWideCharacter:
                expr += L"'";
                expr += getWideString(fit->left_handle);
                expr += L"'";
                break;
            case xd::typeInteger:
                expr += kl::itowstring(getInteger(fit->left_handle));
                break;
            case xd::typeNumeric:
            case xd::typeDouble:
                expr += kl::dbltostr(getDouble(fit->left_handle));
                break;
            case xd::typeDate:
            {
                xd::datetime_t dt = getDateTime(fit->left_handle);
                if (dt == 0)
                {
                    expr += L"NULL";
                }
                 else
                {
                    xd::DateTime d(dt);
                    expr += kl::stdswprintf(L"{d '%04d-%02d-%02d'}", d.getYear(), d.getMonth(), d.getDay());
                }
                break;
            }
            case xd::typeDateTime:
            {
                xd::datetime_t dt = getDateTime(fit->left_handle);
                if (dt == 0)
                {
                    expr += L"NULL";
                }
                 else
                {
                    xd::DateTime d(dt);
                    expr += kl::stdswprintf(L"{ts '%04d-%02d-%02d %02d:%02d:%02d.%03d'}", d.getYear(), d.getMonth(), d.getDay(), d.getHour(), d.getMinute(), d.getSecond(), d.getMillisecond());
                }
                break;
            }
        }

    }



    set->setWhereCondition(expr);

    return setptr;
}
*/

xd::IIteratorPtr OdbcIterator::getChildIterator(xd::IRelationPtr relation)
{
    return getFilteredChildIterator(relation);
}


xd::IIteratorPtr OdbcIterator::getFilteredChildIterator(xd::IRelationPtr relation)
{
    if (eof())
        return xcm::null;

    std::wstring right_table = relation->getRightTable();
    if (right_table.empty())
        return xcm::null;
    right_table = kl::afterLast(right_table, '/');

    OdbcIteratorRelInfo* info = NULL;
    std::vector<OdbcIteratorRelInfo>::iterator it;
    for (it = m_relations.begin(); it != m_relations.end(); ++it)
    {
        if (it->relation_id == relation->getRelationId())
        {
            info = &(*it);
            break;
        }
    }

    if (!info)
    {
        OdbcIteratorRelInfo relinfo;
        relinfo.relation_id = relation->getRelationId();

        std::vector<std::wstring> left_parts, right_parts;
        size_t i, cnt;

        kl::parseDelimitedList(relation->getLeftExpression(), left_parts, L',', true);
        kl::parseDelimitedList(relation->getRightExpression(), right_parts, L',', true);

        // the number of parts in the left expression must match the count in the right expression
        if (left_parts.size() != right_parts.size())
            return xcm::null;

        cnt = right_parts.size();
        for (i = 0; i < cnt; ++i)
        {
            OdbcIteratorRelField f;
            f.right_field = right_parts[i];
            f.left_handle = getHandle(left_parts[i]);
            if (!f.left_handle)
                return xcm::null;
            f.left_type = ((OdbcDataAccessInfo*)(f.left_handle))->type;

            relinfo.fields.push_back(f);
        }

        m_relations.push_back(relinfo);
        info = &(*m_relations.rbegin());
    }


    xd::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(xd::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(xd::dbattrIdentifierQuoteCloseChar);


    std::wstring expr;

    // build expression
    std::vector<OdbcIteratorRelField>::iterator fit;
    for (fit = info->fields.begin(); fit != info->fields.end(); ++fit)
    {
        if (expr.length() > 0)
            expr += L" AND ";
        expr += quote_openchar + fit->right_field + quote_closechar + L"=";



        switch (fit->left_type)
        {
            case xd::typeCharacter:
            case xd::typeWideCharacter:
                expr += L"'";
                expr += getWideString(fit->left_handle);
                expr += L"'";
                break;
            case xd::typeInteger:
                expr += kl::itowstring(getInteger(fit->left_handle));
                break;
            case xd::typeNumeric:
            case xd::typeDouble:
                expr += kl::dbltostr(getDouble(fit->left_handle));
                break;
            case xd::typeDate:
            {
                xd::datetime_t dt = getDateTime(fit->left_handle);
                if (dt == 0)
                {
                    expr += L"NULL";
                }
                else
                {
                    xd::DateTime d(dt);
                    expr += kl::stdswprintf(L"{d '%04d-%02d-%02d'}", d.getYear(), d.getMonth(), d.getDay());
                }
                break;
            }
            case xd::typeDateTime:
            {
                xd::datetime_t dt = getDateTime(fit->left_handle);
                if (dt == 0)
                {
                    expr += L"NULL";
                }
                else
                {
                    xd::DateTime d(dt);
                    expr += kl::stdswprintf(L"{ts '%04d-%02d-%02d %02d:%02d:%02d.%03d'}", d.getYear(), d.getMonth(), d.getDay(), d.getHour(), d.getMinute(), d.getSecond(), d.getMillisecond());
                }
                break;
            }
        }

    }



    std::wstring query = L"SELECT * FROM ";
    query += quote_openchar;
    query += right_table;
    query += quote_closechar;


    if (expr.length() > 0)
    {
        query += L" WHERE ";
        query += expr;
    }


    // create an iterator based on our select statement
    OdbcIterator* iter = new OdbcIterator(m_database);

    if (!iter->init(query))
        return xcm::null;

    return static_cast<xd::IIterator*>(iter);
}


// xd::ICacheRowUpdate::updateCacheRow()

bool OdbcIterator::updateCacheRow(xd::rowid_t rowid,
                                  xd::ColumnUpdateInfo* info,
                                  size_t info_size)
{
    saveRowToCache();
    
    size_t i;
    for (i = 0; i < info_size; ++i, ++info)
    {
        OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)info->handle;
        int column = dai->ordinal - 1;
        
        if (info->null)
            m_cache.updateValue(m_row_pos, column, NULL, 0);
        

        switch (dai->type)
        {
            case xd::typeCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->str_val.c_str(),
                                    (int)info->str_val.length());
                break;

            case xd::typeWideCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->wstr_val.c_str(),
                                    (int)info->wstr_val.length()*sizeof(wchar_t));
                break;


            case xd::typeNumeric:
            case xd::typeDouble:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->dbl_val,
                                    sizeof(double));
                break;

            case xd::typeInteger:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->int_val,
                                    sizeof(int));
                break;

            case xd::typeDate:
            {
                SQL_DATE_STRUCT date;
                
                xd::DateTime dt;
                dt.setDateTime(info->date_val);
                
                date.year = dt.getYear();
                date.month = dt.getMonth();
                date.day = dt.getDay();
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&date,
                                    sizeof(SQL_DATE_STRUCT));
                break;
            }
            break;
            
            case xd::typeDateTime:
            {
                SQL_TIMESTAMP_STRUCT datetime;
                
                xd::DateTime dt;
                dt.setDateTime(info->date_val);
                
                datetime.year = dt.getYear();
                datetime.month = dt.getMonth();
                datetime.day = dt.getDay();
                datetime.hour = dt.getHour();
                datetime.minute = dt.getMinute();
                datetime.second = dt.getSecond();
                datetime.fraction = dt.getMillisecond() * 1000000;
                
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&datetime,
                                    sizeof(SQL_TIMESTAMP_STRUCT));
            }
            break;

            case xd::typeBoolean:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->bool_val,
                                    sizeof(bool));
                break;
        }
    }


    readRowFromCache(m_row_pos);


    return true;
}

