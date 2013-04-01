/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-09-03
 *
 */


#include <kl/klib.h>
#include "tango.h"
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommon/sqlcommon.h"
#include "../xdcommon/localrowcache.h"


const SQLINTEGER row_array_size = 1000;

const std::string empty_string = "";
const std::wstring empty_wstring = L"";


OdbcIterator::OdbcIterator()
{
    m_env = 0;
    m_conn = 0;
    m_stmt = 0;

    m_db_type = -1;

    m_row_pos = 0;
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
}

bool OdbcIterator::init(const std::wstring& query)
{
    // allocate the connection
    SQLAllocConnect(m_env, &m_conn);

    // set the odbc cursor library to only be used if necessary
    //SQLSetConnectOption(m_conn, SQL_ODBC_CURSORS, SQL_CUR_USE_IF_NEEDED);
    //SQLSetConnectOption(m_conn, SQL_ODBC_CURSORS, SQL_CUR_USE_DRIVER);

    // MS Access supports bidirectional
    if (m_db_type == tango::dbtypeAccess /*|| m_db_type == tango::dbtypeMySql*/ )
    {
        m_bidirectional = true;
    }
    
    IOdbcDatabasePtr odb = m_database;
    if (odb.isNull())
        return false;
        
    SQLRETURN retval = odb->connect(m_conn);
    

    if (retval == SQL_NO_DATA || retval == SQL_ERROR || retval == SQL_INVALID_HANDLE)
    {
        // failed
        odb->errorSqlConn(m_conn);
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
        retval = SQLSetStmtAttr(m_stmt,
                                SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC,
                                0);
    }
     else
    {
        retval = SQLSetStmtAttr(m_stmt,
                                SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                                0);
    }
    
    retval = SQLSetStmtAttr(m_stmt,
                            SQL_ATTR_CONCURRENCY,
                            (SQLPOINTER)SQL_CONCUR_READ_ONLY,
                            0);

    retval = SQLExecDirect(m_stmt, sqlt(query), SQL_NTS);

    if (retval == SQL_NO_DATA || retval == SQL_ERROR || retval == SQL_INVALID_HANDLE)
    {
        #ifdef _DEBUG
        testSqlStmt(m_stmt);
        #endif

        odb->errorSqlStmt(m_stmt);
        
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
    int col_tango_type;

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
        col_tango_type = sql2tangoType(col_type);
        
        if (col_tango_type == tango::typeInvalid)
            continue;


        if (col_type == SQL_LONGVARCHAR ||
            col_type == SQL_WLONGVARCHAR)
        {
            // we don't know the width of this field, so set it to 512
            col_width = 512;
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

        if (m_db_type == tango::dbtypeExcel &&
            col_scale == 0 &&
            (col_tango_type == tango::typeDouble ||
             col_tango_type == tango::typeNumeric))
        {
            // excel odbc drivers always return 0 for column scale,
            // so we will set it to a more acceptable value
            col_scale = 2;
        }

        if (m_db_type == tango::dbtypeAccess &&
            col_tango_type == tango::typeCharacter)
        {
            // access always uses Wide Characters
            col_tango_type = tango::typeWideCharacter;
        }

        // handle column scale
        if (col_tango_type != tango::typeNumeric &&
            col_tango_type != tango::typeDouble)
        {
            col_scale = 0;
        }

        if (col_tango_type == tango::typeNumeric)
        {
            // numeric fields have a max width of 18
            col_width > 18 ? col_width = 18 : 0;
        }

        OdbcDataAccessInfo* field = new OdbcDataAccessInfo;
        field->name = col_name;
        field->odbc_type = col_type;
        field->type = col_tango_type;
        field->width = col_width;
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
            case tango::typeCharacter:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_CHAR,
                                    (*it)->str_val,
                                    ((*it)->width+1)*sizeof(char),
                                    &(*it)->indicator);
                break;
            }

            case tango::typeWideCharacter:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_WCHAR,
                                    (*it)->wstr_val,
                                    ((*it)->width+1)*sizeof(wchar_t),
                                    &(*it)->indicator);
                break;
            }

            case tango::typeNumeric:
            case tango::typeDouble:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_DOUBLE,
                                    &(*it)->dbl_val,
                                    sizeof(double),
                                    &(*it)->indicator);
                break;
            }

            case tango::typeInteger:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_SLONG,
                                    &(*it)->int_val,
                                    sizeof(int),
                                    &(*it)->indicator);
                break;
            }

            case tango::typeDate:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_DATE,
                                    &(*it)->date_val,
                                    sizeof(SQL_DATE_STRUCT),
                                    &(*it)->indicator);
                break;
            }

            case tango::typeDateTime:
            {
                retval = SQLBindCol(m_stmt,
                                    (*it)->ordinal,
                                    SQL_C_TIMESTAMP,
                                    &(*it)->datetime_val,
                                    sizeof(SQL_TIMESTAMP_STRUCT),
                                    &(*it)->indicator);

                break;
            }

            case tango::typeBoolean:
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



    // if m_set is null, create a placeholder set
    if (m_set.isNull())
    {
        // create set and initialize variables
        OdbcSet* set = new OdbcSet;
        set->m_env = m_env;
        set->m_database = m_database;
        set->m_db_type = m_db_type;
        set->m_tablename = getTableNameFromSql(query);
        set->m_filter_query = true;

        // initialize Odbc connection for this set
        if (!set->init())
        {
            return false;
        }

        m_set = static_cast<tango::ISet*>(set);
    }



    refreshStructure();
    
    return true;
}

tango::ISetPtr OdbcIterator::getSet()
{
    return m_set;
}

tango::rowpos_t OdbcIterator::getRowCount()
{
    return 0;
}

tango::IDatabasePtr OdbcIterator::getDatabase()
{
    return m_database;
}

tango::IIteratorPtr OdbcIterator::clone()
{
    return xcm::null;
}


void OdbcIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    m_cache_active = ((mask & value & tango::ifReverseRowCache) != 0) ? true : false;
}
    
    
unsigned int OdbcIterator::getIteratorFlags()
{
    // if iterator is bidirectional, then return 0, meaning
    // backwards and forwards scrolling is ok
    if (m_bidirectional)
    {
        return 0;
    }
    
    // if we have a unidirectional iterator, but the back-scroll
    // cache is on, then we still can scroll back
    if (m_cache_active)
    {
        return 0;
    }
    
    // otherwise, indicate that this iterator is forward-only
    return tango::ifForwardOnly;
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
            case tango::typeCharacter:
                if ((*it)->indicator == SQL_NTS)
                    width = strlen((*it)->str_val);
                     else
                    width = (*it)->indicator;
                    
                if (width >= (*it)->width)
                    width = (*it)->width;
                    
                m_cache.appendColumnData((unsigned char*)((*it)->str_val),
                                         width*sizeof(char));
                break;

            case tango::typeWideCharacter:
                if ((*it)->indicator == SQL_NTS)
                    width = wcslen((*it)->wstr_val) * sizeof(wchar_t);
                     else
                    width = (*it)->indicator;
                
                if (width >= (*it)->width * (int)sizeof(wchar_t))
                    width = (*it)->width * (int)sizeof(wchar_t);
                    
                m_cache.appendColumnData((unsigned char*)((*it)->wstr_val),
                                          width);
                break;

            case tango::typeNumeric:
            case tango::typeDouble:
                m_cache.appendColumnData((unsigned char*)&((*it)->dbl_val),
                                         sizeof(double));
                break;

            case tango::typeInteger:
                m_cache.appendColumnData((unsigned char*)&((*it)->int_val),
                                         sizeof(int));
                break;

            case tango::typeDate:
                m_cache.appendColumnData((unsigned char*)&((*it)->date_val),
                                         sizeof(SQL_DATE_STRUCT));
                break;

            case tango::typeDateTime:
                m_cache.appendColumnData((unsigned char*)&((*it)->datetime_val),
                                         sizeof(SQL_TIMESTAMP_STRUCT));
                break;

            case tango::typeBoolean:
                m_cache.appendColumnData((unsigned char*)&((*it)->bool_val),
                                         sizeof(bool));
                break;
        }
    }

    m_cache.finishRow();
}

void OdbcIterator::readRowFromCache(tango::rowpos_t row)
{
    m_cache.goRow((tango::rowpos_t)row);
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
            case tango::typeCharacter:
                memcpy((*it)->str_val, data, data_size);
                break;

            case tango::typeWideCharacter:
                memcpy((*it)->wstr_val, data, data_size);
                //data_size /= sizeof(wchar_t);
                (*it)->indicator = data_size;
                break;


            case tango::typeNumeric:
            case tango::typeDouble:
                (*it)->dbl_val = *(double*)data;
                break;

            case tango::typeInteger:
                (*it)->int_val = *(int*)data;
                break;

            case tango::typeDate:
                memcpy(&((*it)->date_val), data, sizeof(SQL_DATE_STRUCT));
                break;

            case tango::typeDateTime:
                memcpy(&((*it)->datetime_val), data, sizeof(SQL_TIMESTAMP_STRUCT));
                break;

            case tango::typeBoolean:
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
    
    if ((tango::rowpos_t)desired_row < m_cache.getRowCount())
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

tango::rowid_t OdbcIterator::getRowId()
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

void OdbcIterator::goRow(const tango::rowid_t& rowid)
{
}

double OdbcIterator::getPos()
{
    return (double)(long long)m_row_pos;
}

tango::IStructurePtr OdbcIterator::getStructure()
{
    if (m_structure.isOk())
        return m_structure->clone();

    tango::IStructurePtr set_structure;
    
    if (m_set)
    {
        set_structure = m_set->getStructure();
    }


    Structure* s = new Structure;

    std::vector<OdbcDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        tango::IColumnInfoPtr col;
     
        // try to use the column info from the
        // set's structure, if possible

        if (set_structure)
        {
            col = set_structure->getColumnInfo((*it)->name);
        }

        if (col.isOk())
        {
            col->setColumnOrdinal((*it)->ordinal - 1);
            s->addColumn(col);
        }
         else
        {
            if ((*it)->isCalculated())
            {
                tango::IColumnInfoPtr col;
                col = static_cast<tango::IColumnInfo*>(new ColumnInfo);
                col->setName((*it)->name);
                col->setType((*it)->type);
                col->setWidth((*it)->width);
                col->setScale((*it)->scale);
                col->setExpression((*it)->expr_text);
                col->setCalculated(true);
                col->setColumnOrdinal((*it)->ordinal - 1);
                s->addColumn(col);
            }
             else
            {
                // generate column info from the
                // field info from the query result
                tango::IColumnInfoPtr col;

                col = createColInfo(m_db_type,
                                    (*it)->name,
                                    (*it)->odbc_type,
                                    (*it)->width,
                                    (*it)->scale,
                                    (*it)->expr_text,
                                    -1);

                col->setColumnOrdinal((*it)->ordinal - 1);
                s->addColumn(col);
            }
        }
    }
    
    m_structure = static_cast<tango::IStructure*>(s);

    return m_structure->clone();
}

void OdbcIterator::refreshStructure()
{
    tango::IStructurePtr set_structure = m_set->getStructure();
    if (set_structure.isNull())
        return;

    // find changed/deleted calc fields
    int i, col_count;
    for (i = 0; i < (int)m_fields.size(); ++i)
    {
        if (!m_fields[i]->isCalculated())
            continue;
            
        delete m_fields[i]->expr;
        m_fields[i]->expr = NULL;

        tango::IColumnInfoPtr col = set_structure->getColumnInfo(m_fields[i]->name);
        if (col.isNull())
        {
            m_fields.erase(m_fields.begin() + i);
            i--;
            continue;
        }
  
        m_fields[i]->type = col->getType();
        m_fields[i]->width = col->getWidth();
        m_fields[i]->scale = col->getScale();
        m_fields[i]->expr_text = col->getExpression();
        m_fields[i]->str_val = NULL;
        m_fields[i]->wstr_val = NULL;
    }
    
    // find new calc fields
    
    col_count = set_structure->getColumnCount();
    
    std::vector<OdbcDataAccessInfo*>::iterator it;
    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr col;
        
        col = set_structure->getColumnInfoByIdx(i);
        if (!col->getCalculated())
            continue;
            
        bool found = false;
        
        for (it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            if (!(*it)->isCalculated())
                continue;

            if (0 == wcscasecmp((*it)->name.c_str(), col->getName().c_str()))
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            // add new calc field
            OdbcDataAccessInfo* dai = new OdbcDataAccessInfo;
            dai->name = col->getName();
            dai->type = col->getType();
            dai->width = col->getWidth();
            dai->scale = col->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = col->getExpression();
            dai->expr = NULL;
                
            dai->str_val = NULL;
            dai->wstr_val = NULL;
            dai->int_val = 0;
            dai->dbl_val = 0.0;
            dai->bool_val = 0;
            memset(&dai->date_val, 0, sizeof(SQL_DATE_STRUCT));
            memset(&dai->datetime_val, 0, sizeof(SQL_TIMESTAMP_STRUCT));

            m_fields.push_back(dai);
        }
    }
    
    // parse all expressions
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        (*it)->expr = parse((*it)->expr_text);
    }
    
    
    // let our m_structure cache regenerate from m_fields
    m_structure.clear();
}

bool OdbcIterator::modifyStructure(tango::IStructure* struct_config,
                                   tango::IJob* job)
{
    IStructureInternalPtr struct_int = struct_config;

    std::vector<StructureAction>& actions = struct_int->getStructureActions();
    std::vector<StructureAction>::iterator it;
    std::vector<OdbcDataAccessInfo*>::iterator it2;
    
    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionDelete)
            continue;

        for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
        {
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
            {
                OdbcDataAccessInfo* dai = *(it2);
                m_fields.erase(it2);
                delete dai;
                break;
            }
        }
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionModify)
            continue;

        for (it2 = m_fields.begin();
             it2 != m_fields.end();
             ++it2)
        {
            if (0 == wcscasecmp(it->m_colname.c_str(), (*it2)->name.c_str()))
            {
                if (it->m_params->getName().length() > 0)
                {
                    std::wstring new_name = it->m_params->getName();
                    kl::makeUpper(new_name);
                    (*it2)->name = new_name;
                }

                if (it->m_params->getType() != -1)
                {
                    (*it2)->type = it->m_params->getType();
                }

                if (it->m_params->getWidth() != -1)
                {
                    (*it2)->width = it->m_params->getWidth();
                }

                if (it->m_params->getScale() != -1)
                {
                    (*it2)->scale = it->m_params->getScale();
                }

                if (it->m_params->getExpression().length() > 0)
                {
                    if ((*it2)->expr)
                        delete (*it2)->expr;
                    (*it2)->expr_text = it->m_params->getExpression();
                    (*it2)->expr = parse(it->m_params->getExpression());
                }
            }
        }
    }

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action != StructureAction::actionCreate)
            continue;

        if (it->m_params->getExpression().length() > 0)
        {
            OdbcDataAccessInfo* dai = new OdbcDataAccessInfo;
            dai->name = it->m_params->getName();
            dai->type = it->m_params->getType();
            dai->width = it->m_params->getWidth();
            dai->scale = it->m_params->getScale();
            dai->ordinal = m_fields.size();
            dai->expr_text = it->m_params->getExpression();
            dai->expr = parse(it->m_params->getExpression());
                
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
    m_structure.clear();
    
    return true;
}



tango::objhandle_t OdbcIterator::getHandle(const std::wstring& expr)
{
    std::vector<OdbcDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->name.c_str(), expr.c_str()))
            return (tango::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        OdbcDataAccessInfo* dai = new OdbcDataAccessInfo;
        dai->expr = NULL;
        dai->expr_text = expr;
        dai->type = tango::typeBinary;
        dai->key_layout = new KeyLayout;

        if (!dai->key_layout->setKeyExpr(static_cast<tango::IIterator*>(this),
                                    expr.substr(4),
                                    false))
        {
            delete dai;
            return 0;
        }
        
        m_exprs.push_back(dai);
        return (tango::objhandle_t)dai;
    }
    
    
    kscript::ExprParser* parser = parse(expr);
    if (!parser)
    {
        return (tango::objhandle_t)0;
    }

    OdbcDataAccessInfo* dai = new OdbcDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2tangoType(parser->getType());
    m_exprs.push_back(dai);

    return (tango::objhandle_t)dai;
}

bool OdbcIterator::releaseHandle(tango::objhandle_t data_handle)
{
    std::vector<OdbcDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
        {
            return true;
        }
    }

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
    {
        if ((tango::objhandle_t)(*it) == data_handle)
        {
            delete (*it);
            m_exprs.erase(it);
            return true;
        }
    }

    return false;
}

tango::IColumnInfoPtr OdbcIterator::getInfo(tango::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xcm::null;
    }

    // try to get the column information from the set structure

    if (m_structure.isNull())
    {
        tango::IStructurePtr s = getStructure();
    }

    if (m_structure.isOk())
    {
        tango::IColumnInfoPtr colinfo;
        colinfo = m_structure->getColumnInfo(dai->name);
        if (colinfo.isOk())
        {
            return colinfo->clone();
        }
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

int OdbcIterator::getType(tango::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return tango::typeInvalid;
    }
    
    return dai->type;
}

int OdbcIterator::getRawWidth(tango::objhandle_t data_handle)
{
    OdbcDataAccessInfo* dai = (OdbcDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* OdbcIterator::getRawPtr(tango::objhandle_t data_handle)
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

const std::string& OdbcIterator::getString(tango::objhandle_t data_handle)
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
        width = strlen(dai->str_val);
         else
        width = dai->indicator;

    if (width > dai->width)
        width = dai->width;
    
    if (m_db_type == tango::dbtypeMySql &&
        (dai->odbc_type == SQL_BINARY || dai->odbc_type == SQL_VARBINARY || dai->odbc_type == SQL_LONGVARBINARY))
    {
        decodeHexString(dai->str_val, dai->str_result);
        return dai->str_result;   
    }
    
    if (dai->type == tango::typeCharacter)
    {
        dai->str_result.assign(dai->str_val, width);
        return dai->str_result;
    }
     else if (dai->type == tango::typeWideCharacter)
    {
        dai->wstr_result.assign(dai->wstr_val, width);
        dai->str_result = kl::tostring(dai->wstr_result);
        return dai->str_result;
    }

    return empty_string;
}

const std::wstring& OdbcIterator::getWideString(tango::objhandle_t data_handle)
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
        width = wcslen(dai->wstr_val);
         else
        width = (dai->indicator/sizeof(wchar_t));

    if (dai->type == tango::typeCharacter)
    {
        dai->wstr_result = kl::towstring(getString(data_handle));
        return dai->wstr_result;
    }
     else if (dai->type == tango::typeWideCharacter)
    {
        dai->wstr_result.assign(dai->wstr_val, width);
        return dai->wstr_result;
    }

    return empty_wstring;
}

tango::datetime_t OdbcIterator::getDateTime(tango::objhandle_t data_handle)
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

        tango::datetime_t dt;
        dt = edt.date;
        dt <<= 32;
        if (dai->type == tango::typeDateTime)
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


    if (dai->type == tango::typeDate)
    {
        if (dai->date_val.year == 0)
            return 0;

        tango::DateTime dt(dai->date_val.year,
                           dai->date_val.month,
                           dai->date_val.day);
        return dt;
    }
     else
    {    
        if (dai->datetime_val.year == 0)
            return 0;

        tango::DateTime dt(dai->datetime_val.year,
                           dai->datetime_val.month,
                           dai->datetime_val.day,
                           dai->datetime_val.hour,
                           dai->datetime_val.minute,
                           dai->datetime_val.second,
                           dai->datetime_val.fraction/1000000);
        return dt;
    }
}

double OdbcIterator::getDouble(tango::objhandle_t data_handle)
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

    if (dai->type == tango::typeNumeric ||
        dai->type == tango::typeDouble)
    {
        return dai->dbl_val;
    }
     else if (dai->type == tango::typeInteger)
    {
        return (double)dai->int_val;
    }

    return 0;
}

int OdbcIterator::getInteger(tango::objhandle_t data_handle)
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

    if (dai->type == tango::typeInteger)
    {
        return dai->int_val;
    }
     else if (dai->type == tango::typeNumeric ||
              dai->type == tango::typeDouble)
    {
        return (int)dai->dbl_val;
    }

    return 0;
}

bool OdbcIterator::getBoolean(tango::objhandle_t data_handle)
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

bool OdbcIterator::isNull(tango::objhandle_t data_handle)
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


tango::ISetPtr OdbcIterator::getChildSet(tango::IRelationPtr relation)
{
    if (eof())
        return xcm::null;

    tango::ISetPtr setptr = relation->getRightSetPtr();
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


    tango::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);


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
            case tango::typeCharacter:
            case tango::typeWideCharacter:
                expr += L"'";
                expr += getWideString(fit->left_handle);
                expr += L"'";
                break;
            case tango::typeInteger:
                expr += kl::itowstring(getInteger(fit->left_handle));
                break;
            case tango::typeNumeric:
            case tango::typeDouble:
                expr += kl::dbltostr(getDouble(fit->left_handle));
                break;
            case tango::typeDate:
            {
                tango::datetime_t dt = getDateTime(fit->left_handle);
                if (dt == 0)
                {
                    expr += L"NULL";
                }
                 else
                {
                    tango::DateTime d(dt);
                    expr += kl::stdswprintf(L"{d '%04d-%02d-%02d'}", d.getYear(), d.getMonth(), d.getDay());
                }
                break;
            }
            case tango::typeDateTime:
            {
                tango::datetime_t dt = getDateTime(fit->left_handle);
                if (dt == 0)
                {
                    expr += L"NULL";
                }
                 else
                {
                    tango::DateTime d(dt);
                    expr += kl::stdswprintf(L"{ts '%04d-%02d-%02d %02d:%02d:%02d.%03d'}", d.getYear(), d.getMonth(), d.getDay(), d.getHour(), d.getMinute(), d.getSecond(), d.getMillisecond());
                }
                break;
            }
        }

    }



    set->setWhereCondition(expr);

    return setptr;
}


tango::IIteratorPtr OdbcIterator::getChildIterator(tango::IRelationPtr relation)
{
    return xcm::null;
/*
    tango::ISetPtr set = getChildSet(relation);

    if (set.isNull())
        return xcm::null;

    return set->createIterator(L"", L"", NULL);
*/
}



// tango::ICacheRowUpdate::updateCacheRow()

bool OdbcIterator::updateCacheRow(tango::rowid_t rowid,
                                  tango::ColumnUpdateInfo* info,
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
            case tango::typeCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->str_val.c_str(),
                                    info->str_val.length());
                break;

            case tango::typeWideCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->wstr_val.c_str(),
                                    info->wstr_val.length()*sizeof(wchar_t));
                break;


            case tango::typeNumeric:
            case tango::typeDouble:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->dbl_val,
                                    sizeof(double));
                break;

            case tango::typeInteger:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)&info->int_val,
                                    sizeof(int));
                break;

            case tango::typeDate:
            {
                SQL_DATE_STRUCT date;
                
                tango::DateTime dt;
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
            
            case tango::typeDateTime:
            {
                SQL_TIMESTAMP_STRUCT datetime;
                
                tango::DateTime dt;
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

            case tango::typeBoolean:
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

