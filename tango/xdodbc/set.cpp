/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-09-03
 *
 */


#include <kl/klib.h>
#include <kl/md5.h>
#include "database.h"
#include "set.h"
#include "iterator.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/extfileinfo.h"


OdbcSet::OdbcSet()
{
    m_env = 0;
    m_conn = 0;

    m_db_type = -1;
    m_filter_query = false;
    m_tablename = L"";
    
    m_cached_row_count = (tango::rowpos_t)-1;
}

OdbcSet::~OdbcSet()
{
    if (m_conn)
    {
        SQLDisconnect(m_conn);
        SQLFreeConnect(m_conn);
        m_conn = 0;
    }
}

bool OdbcSet::init()
{
    // allocate the connection
    SQLAllocConnect(m_env, &m_conn);

    IOdbcDatabasePtr odb = m_database;
    SQLRETURN retval = odb->connect(m_conn);

    if (retval == SQL_NO_DATA ||
        retval == SQL_ERROR ||
        retval == SQL_INVALID_HANDLE)
    {
        // failed
        return false;
    }

    // set the set info filename
    if (m_tablename.length() > 0)
    {
        tango::IAttributesPtr attr = m_database->getAttributes();
        std::wstring definition_path = attr->getStringAttribute(tango::dbattrDefinitionDirectory);

        setConfigFilePath(ExtFileInfo::getConfigFilenameFromSetId(definition_path, getSetId()));
    }
    
    
    // enable transactions
    //SQLSetConnectAttr(m_conn, SQL_ATTR_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);

    return true;
}



void OdbcSet::setObjectPath(const std::wstring& path)
{
    m_path = path;
}

std::wstring OdbcSet::getObjectPath()
{
    if (m_path.empty())
        return m_tablename;
        
    return m_path;
}


unsigned int OdbcSet::getSetFlags()
{
    if (m_filter_query)
        return 0;
        
    if (m_cached_row_count != (tango::rowpos_t)-1)
        return tango::sfFastRowCount;
 
    switch (m_db_type)
    {
        case tango::dbtypeMySql:
        case tango::dbtypeSqlServer:
        case tango::dbtypeExcel:
            return tango::sfFastRowCount;
        case tango::dbtypeAccess:
        default:
            return 0;
    }

    return 0;
}

std::wstring OdbcSet::getSetId()
{
    if (m_set_id.length() == 0)
    {
        IOdbcDatabasePtr odb = m_database;
        std::wstring id = L"xdodbc:";
        
        if (odb->getUsingDsn())
        {
            id += L"dsn:";
        }
        
        
        
        std::wstring server_or_path;
        
        if (m_db_type == tango::dbtypeAccess ||
            m_db_type == tango::dbtypeExcel)
        {
            server_or_path = odb->getPath();
            #ifdef WIN32
            kl::makeLower(server_or_path);
            #endif
        }
         else
        {
            server_or_path = odb->getServer();
            kl::makeLower(server_or_path);
        }
        
        id += server_or_path;
        id += L":";
        
        
        std::wstring table_name = m_tablename;
        
        if (m_db_type != tango::dbtypeOracle)
            kl::makeLower(table_name);
            
        id += table_name;
        
        m_set_id = kl::md5str(id);
    }
    
    return m_set_id;
}


struct TempInfo
{
    tango::IColumnInfoPtr colinfo;
    std::wstring colname;
    char buf[255];
    SQLLEN indicator;
    int max_dec;
};

void OdbcSet::fixAccessStructure(tango::IStructurePtr& s)
{
    // because Access can store arbitrary numeric scales (decimal digits
    // to the right of the decimal point), we must find out the correct
    // scale by performing a query to look at the data itself

    std::wstring query;
    tango::IColumnInfoPtr colinfo;
    int col_count = s->getColumnCount();
    int i;
    std::vector<TempInfo> cols;
    std::vector<TempInfo>::iterator it;

    query = L"SELECT TOP 100 ";
    
    bool first = true;
    for (i = 0; i < col_count; ++i)
    {
        colinfo = s->getColumnInfoByIdx(i);
        if (colinfo->getType() == tango::typeNumeric ||
            colinfo->getType() == tango::typeDouble)
        {
            if (!first)
                query += L",";
            first = false;
            query += L"STR([";
            query += colinfo->getName();
            query += L"])";
            TempInfo ti;
            ti.colinfo = colinfo;
            ti.max_dec = 0;
            ti.colname = colinfo->getName();
            cols.push_back(ti);
        }
    }

    query += L" FROM [";
    query += m_tablename;
    query += L"]";



    HSTMT stmt = NULL;
    SQLRETURN retval = 0;
    SQLAllocStmt(m_conn, &stmt);

    retval = SQLSetStmtAttr(stmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                            0);

    retval = SQLSetStmtAttr(stmt,
                            SQL_ATTR_CONCURRENCY,
                            (SQLPOINTER)SQL_CONCUR_READ_ONLY,
                            0);

    retval = SQLExecDirect(stmt,
                           sqlt(query),
                           SQL_NTS);

    if (retval == SQL_NO_DATA ||
        retval == SQL_ERROR ||
        retval == SQL_INVALID_HANDLE)
    {
        // failed (make no changes to the structure)
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }

    SQLSMALLINT result_col_count = 0;
    SQLNumResultCols(stmt, &result_col_count);

    if (result_col_count != cols.size())
    {
        // failed (make no changes to the structure)
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }

    // bind the columns to their return values
    i = 1;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        retval = SQLBindCol(stmt,
                            i++,
                            SQL_C_CHAR,
                            it->buf,
                            255,
                            &it->indicator);
        if (retval != SQL_SUCCESS)
        {
            // failed (make no changes to the structure)
            SQLCloseCursor(stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            return;
        }
    }


    while (SQLFetch(stmt) == SQL_SUCCESS)
    {
        for (it = cols.begin(); it != cols.end(); ++it)
        {
            if (it->indicator == SQL_NULL_DATA)
                continue;
            char *p = strchr(it->buf, '.');
            if (p)
            {
                int offset = p - it->buf;
                int len = strlen(it->buf);
                int decimal_places = len-(offset+1);
                if (decimal_places > it->max_dec)
                    it->max_dec = decimal_places;
            }
        }
    }

#ifdef _DEBUG
    testSqlStmt(stmt);
#endif

    SQLCloseCursor(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    for (it = cols.begin(); it != cols.end(); ++it)
    {
        it->colinfo->setScale(it->max_dec);
    }







    // now grab scale from SQLColAttribute()

    query = L"SELECT TOP 20 ";
    
    first = true;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        if (!first)
            query += L",";
        first = false;
        query += L"[";
        query += it->colname;
        query += L"]";
    }

    query += L" FROM [";
    query += m_tablename;
    query += L"]";



    stmt = NULL;
    retval = 0;
    SQLAllocStmt(m_conn, &stmt);

    retval = SQLSetStmtAttr(stmt,
                            SQL_ATTR_CURSOR_TYPE,
                            (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY,
                            0);

    retval = SQLSetStmtAttr(stmt,
                            SQL_ATTR_CONCURRENCY,
                            (SQLPOINTER)SQL_CONCUR_READ_ONLY,
                            0);

    retval = SQLExecDirect(stmt,
                           sqlt(query),
                           SQL_NTS);

    if (retval == SQL_NO_DATA ||
        retval == SQL_ERROR ||
        retval == SQL_INVALID_HANDLE)
    {
        // failed (make no changes to the structure)
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }

    result_col_count = 0;
    SQLNumResultCols(stmt, &result_col_count);

    if (result_col_count != cols.size())
    {
        // failed (make no changes to the structure)
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }

    // bind the columns to their return values
    i = 1;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        SQLLEN scale = 0;

        if (SQL_SUCCESS != SQLColAttribute(stmt,
                                           i++,
                                           SQL_DESC_SCALE,
                                           0,
                                           0,
                                           0,
                                           &scale))
        {
            // failed (make no changes to the structure)
            SQLCloseCursor(stmt);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            return;
        }

        if (scale > it->max_dec)
            it->max_dec = scale;
    }

    SQLCloseCursor(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    for (it = cols.begin(); it != cols.end(); ++it)
    {
        it->colinfo->setScale(it->max_dec);
    }
}




tango::IStructurePtr OdbcSet::getStructure()
{
    if (m_structure.isOk())
    {
        tango::IStructurePtr ret = m_structure->clone();
        appendCalcFields(ret);
        return ret;
    }

    // create new tango::IStructure
    Structure* s = new Structure;

    wchar_t col_name[255];
    short int col_type;
    int col_width;
    short int col_scale;
    short int datetime_sub;

    SQLLEN col_name_ind = SQL_NULL_DATA;
    SQLLEN col_type_ind = SQL_NULL_DATA;
    SQLLEN col_width_ind = SQL_NULL_DATA;
    SQLLEN col_scale_ind = SQL_NULL_DATA;
    SQLLEN datetime_sub_ind = SQL_NULL_DATA;


    HSTMT stmt = 0;
    SQLAllocHandle(SQL_HANDLE_STMT, m_conn, &stmt);


    std::wstring schema;
    std::wstring tablename;

    if (m_tablename.find(L'.') != -1)
    {
        schema = kl::beforeFirst(m_tablename, L'.');
        tablename = kl::afterFirst(m_tablename, L'.');
    }
     else
    {
        tablename = m_tablename;
    }

    if (m_db_type == tango::dbtypeExcel)
    {
        // excel wants a $ at the end
        tablename += L"$";
    }


    SQLRETURN r;
    if (m_db_type == tango::dbtypeExcel ||
        m_db_type == tango::dbtypeAccess ||
        schema.empty())
    {
        // excel and access drivers cannot tolerate even an 
        // empty string for parameter 4 of SQLColumns();
        // it wants it to be NULL

        // even SQL Server, if a schema is not specified,
        // wants a NULL instead of an empty string.  This
        //  is taken care of by the 'schema.isEmpty()' check above

        r = SQLColumns(stmt,
                       NULL,
                       0,
                       NULL,
                       0,
                       sqlt(tablename),
                       SQL_NTS,
                       NULL,
                       0);
    }
     else
    {
        r = SQLColumns(stmt,
                       NULL,
                       0,
                       sqlt(schema),
                       SQL_NTS,
                       sqlt(tablename),
                       SQL_NTS,
                       NULL,
                       0);
    }



    #ifdef _DEBUG
    if (r != SQL_SUCCESS)
    {
        testSqlStmt(stmt);
    }
    #endif


    if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
    {
        // bind columns in result set to buffers
        
        int c_ord = 5;
        if (m_db_type == tango::dbtypeOracle)
        {
            c_ord = 14;
        }

        // bind column name
        r = SQLBindCol(stmt,
                       4,
                       SQL_C_WCHAR,
                       col_name,
                       255*sizeof(wchar_t),
                       &col_name_ind);

        // bind column type
        r = SQLBindCol(stmt,
                       c_ord,
                       SQL_C_SSHORT,
                       &col_type,
                       sizeof(short int),
                       &col_type_ind);

        // bind column width
        r = SQLBindCol(stmt,
                       7,
                       SQL_C_SLONG,
                       &col_width,
                       sizeof(long),
                       &col_width_ind);

        // bind column scale
        r = SQLBindCol(stmt,
                       9,
                       SQL_C_SSHORT,
                       &col_scale,
                       sizeof(short int),
                       &col_scale_ind);

        if (m_db_type == tango::dbtypeOracle)
        {
            r = SQLBindCol(stmt,
                           15,
                           SQL_C_SSHORT,
                           &datetime_sub,
                           sizeof(short int),
                           &datetime_sub_ind);
        }

        int i = 0;

        // populate the structure
        while (1)
        {
            col_name[0] = 0;
            col_type = 0;
            col_width = 0;
            col_scale = 0;

            r = SQLFetch(stmt);
            
            #ifdef _DEBUG
            testSqlStmt(stmt);
            #endif

            if (r == SQL_ERROR || r == SQL_NO_DATA)
            {
                // failed
                break;
            }

            if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
            {
                std::wstring wcol_name;
                wcol_name = col_name;

                tango::IColumnInfoPtr col;
                
                col = createColInfo(m_db_type,
                                    wcol_name,
                                    col_type,
                                    col_width,
                                    col_scale,
                                    L"",
                                      (datetime_sub_ind == SQL_NULL_DATA) ?
                                         -1 : datetime_sub);

                if (col.isNull())
                    continue;

                col->setColumnOrdinal(i);
                i++;

                s->addColumn(col);
            }
             else
            {
                // failed
                break;
            }
        }
    }

    if (stmt)
    {
        SQLCloseCursor(stmt);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

    
    m_structure = s;

    if (m_db_type == tango::dbtypeAccess)
    {
        fixAccessStructure(m_structure);
    }

    tango::IStructurePtr ret = m_structure->clone();
    appendCalcFields(ret);
    return ret;
}

bool OdbcSet::modifyStructure(tango::IStructure* struct_config,
                              tango::IJob* job)
{
    bool done_flag = false;
    if (!CommonBaseSet::modifyStructure(struct_config, &done_flag))
        return false;
    if (done_flag)
        return true;
    
    unsigned int processed_action_count = 0;

    tango::IStructurePtr current_struct = getStructure();
    IStructureInternalPtr s = struct_config;
    std::vector<StructureAction>& actions = s->getStructureActions();
    std::vector<StructureAction>::iterator it;

    std::wstring command;
    command.reserve(1024);

    tango::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);

    // handle delete
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action == StructureAction::actionDelete)
        {
            command = L"ALTER TABLE ";
            command += quote_openchar;
            command += m_tablename;
            command += quote_closechar;
            command += L" DROP COLUMN ";
            command += quote_openchar;
            command += it->m_colname;
            command += quote_closechar;

            xcm::IObjectPtr result_obj;
            m_database->execute(command, 0, result_obj, NULL);
            command = L"";
        }
    }

    // handle modify
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action == StructureAction::actionModify)
        {
            if (m_db_type == tango::dbtypeSqlServer)
            {
                // column rename for SQL Server
                if (it->m_colname != it->m_params->getName())
                {
                    // note: in following case, SQL server escapes table and field 
                    // as a single unit; so [table.field], not [table].[field]

                    command = L"sp_rename ";
                    command += quote_openchar;
                    command += getTablenameFromOfsPath(m_tablename);
                    command += L".";
                    command += it->m_colname;
                    command += quote_closechar;
                    command += L",";
                    command += quote_openchar;
                    command += it->m_params->getName();
                    command += quote_closechar;
                    command += L",";
                    command += quote_openchar;
                    command += L"COLUMN";
                    command += quote_closechar;

                    xcm::IObjectPtr result_obj;
                    m_database->execute(command, 0, result_obj, NULL);
                    command = L"";
                }

                // TODO: handle general rename case and additional parameter 
                // modification
            }
        }
    }

    int i;
    int col_count = current_struct->getColumnCount();
    tango::IColumnInfoPtr col_info;
    bool found = false;

    // handle create
    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (it->m_action == StructureAction::actionCreate)
        {
            for (i = 0; i < col_count; ++i)
            {
                col_info = current_struct->getColumnInfoByIdx(i);

                if (!wcscasecmp(col_info->getName().c_str(), it->m_params->getName().c_str()))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::wstring fieldname = L"";
                fieldname += quote_openchar;
                fieldname += it->m_params->getName();
                fieldname += quote_closechar;
            
                command = L"ALTER TABLE ";
                command += quote_openchar;
                command += m_tablename;
                command += quote_closechar;
                command += L" ADD ";
                command += createOdbcFieldString(fieldname,
                                                 it->m_params->getType(),
                                                 it->m_params->getWidth(),
                                                 it->m_params->getScale(), 
                                                 false, 
                                                 m_database->getDatabaseType());

                xcm::IObjectPtr result_obj;
                m_database->execute(command, 0, result_obj, NULL);
                command = L"";
            }

            found = false;
        }
    }

    // reset the structure so it will be refresh 
    // by getStructure
    m_structure = xcm::null;

    return true;
}

tango::IRowInserterPtr OdbcSet::getRowInserter()
{
    OdbcRowInserter* inserter = new OdbcRowInserter(this);
    return static_cast<tango::IRowInserter*>(inserter);
}

tango::IRowDeleterPtr OdbcSet::getRowDeleter()
{
    return xcm::null;
}

int OdbcSet::insert(tango::IIteratorPtr source_iter,
                    const std::wstring& where_condition,
                    int max_rows,
                    tango::IJob* job)
{
    return xdcmnInsert(source_iter, this, where_condition, max_rows, job);
}

tango::IIteratorPtr OdbcSet::createIterator(const std::wstring& columns,
                                            const std::wstring& expr,
                                            tango::IJob* job)
{
    std::wstring query;
    query.reserve(1024);


    if (m_db_type == tango::dbtypeAccess)
    {
        tango::IStructurePtr s = getStructure();
        int i, cnt;
        
        cnt = s->getColumnCount();

        query = L"SELECT ";
        for (i = 0; i < cnt; ++i)
        {
            tango::IColumnInfoPtr colinfo;
            colinfo = s->getColumnInfoByIdx(i);

            if (colinfo->getCalculated())
                continue;
                
            if (i != 0)
            {
                query += L",";
            }
            
            if (colinfo->getType() == tango::typeNumeric ||
                colinfo->getType() == tango::typeDouble)
            {
                query += L"IIF(ISNUMERIC([";
                query += colinfo->getName();
                query += L"]),VAL(STR([";
                query += colinfo->getName();
                query += L"])),null) AS ";
                query += L"[";
                query += colinfo->getName();
                query += L"] ";
            }
             else
            {
                query += L"[";
                query += colinfo->getName();
                query += L"]";
            }
        }

        query += L" FROM ";
        query += L"[";
        query += m_tablename;
        query += L"]";
    }
     else if (m_db_type == tango::dbtypeExcel)
    {
        query = L"SELECT * FROM ";
        query += L"\"";
        query += m_tablename;
        query += L"$\"";
    }
     else if (m_db_type == tango::dbtypeOracle)
    {
        tango::IStructurePtr s = getStructure();
        int i, cnt;
        
        cnt = s->getColumnCount();

        query = L"SELECT ";
        for (i = 0; i < cnt; ++i)
        {
            if (i != 0)
            {
                query += L",";
            }

            query += s->getColumnName(i);
        }
        query += L" FROM ";
        query += m_tablename;
    }
     else
    {
        tango::IAttributesPtr attr = m_database->getAttributes();
        std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
        std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);    
    
        query = L"SELECT * FROM ";
        query += quote_openchar;
        query += m_tablename;
        query += quote_closechar;
    }

    if (expr.length() > 0)
    {
        query += L" ORDER BY ";
        query += expr;
    }
    
    // create an iterator based on our select statement
    OdbcIterator* iter = new OdbcIterator;
    iter->m_env = m_env;
    iter->m_database = m_database;
    iter->m_set = this;
    iter->m_db_type = m_db_type;

    // initialize Odbc connection for this set
    if (!iter->init(query))
    {
        return xcm::null;
    }

    return static_cast<tango::IIterator*>(iter);
}

tango::rowpos_t OdbcSet::getRowCount()
{
    if (m_cached_row_count != (tango::rowpos_t)-1)
        return m_cached_row_count;

    std::wstring query;

    if (m_db_type == tango::dbtypeAccess)
    {
        query = L"SELECT COUNT(*) AS [xdodbcres] FROM ";
        query += L"[";
        query += m_tablename;
        query += L"]";
    }
     else if (m_db_type == tango::dbtypeExcel)
    {
        query += L"SELECT COUNT(*) AS \"xdodbcres\" FROM ";
        query += L"\"";
        query += m_tablename;
        query += L"$\"";
    }
     else if (m_db_type == tango::dbtypeSqlServer)
    {
        query += L"SELECT rows AS xdodbcres FROM sysindexes WHERE id = OBJECT_ID('";
        query += m_tablename;
        query += L"') AND indid < 2";
    }
     else
    {
        tango::IAttributesPtr attr = m_database->getAttributes();
        std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
        std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);     
    
        query += L"SELECT COUNT(*) AS xdodbcres FROM ";
        query += quote_openchar;
        query += m_tablename;
        query += quote_closechar;
    }

    OdbcIterator* iter = new OdbcIterator;
    iter->ref();
    iter->m_env = m_env;
    iter->m_database = m_database;
    iter->m_set = this;
    iter->m_db_type = m_db_type;

    // initialize Odbc connection for this set
    if (!iter->init(query))
    {
        iter->unref();
        return 0;
    }

    iter->goFirst();
    tango::objhandle_t h = iter->getHandle(L"xdodbcres");
    if (!h)
    {
        iter->unref();
        return 0;
    }

    tango::rowpos_t result = iter->getInteger(h);
    iter->releaseHandle(h);

    iter->unref();


    // always cache row count
    m_cached_row_count = result;

/*
    // for now, save the cached row count only for Access dbs
    // maybe later we'll expand this to other dbs
    if (m_db_type == tango::dbtypeAccess)
    {
        m_cached_row_count = result;
    }
*/
    
    return result;
}



























OdbcRowInserter::OdbcRowInserter(OdbcSet* set)
{
    m_set = set;
    m_set->ref();

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

    m_set->unref();
}

tango::objhandle_t OdbcRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<OdbcInsertFieldData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->m_name.c_str(), column_name.c_str()))
            return (tango::objhandle_t)(*it);
    }

    return 0;
}

tango::IColumnInfoPtr OdbcRowInserter::getInfo(tango::objhandle_t column_handle)
{
    return xcm::null;
}

bool OdbcRowInserter::putRawPtr(tango::objhandle_t column_handle,
                                const unsigned char* value,
                                int length)
{
    return false;
}

bool OdbcRowInserter::putString(tango::objhandle_t column_handle,
                                const std::string& value)
{
    OdbcInsertFieldData* f = (OdbcInsertFieldData*)column_handle;
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

bool OdbcRowInserter::putWideString(tango::objhandle_t column_handle,
                                    const std::wstring& value)
{
    OdbcInsertFieldData* f = (OdbcInsertFieldData*)column_handle;
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

bool OdbcRowInserter::putDouble(tango::objhandle_t column_handle,
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

bool OdbcRowInserter::putInteger(tango::objhandle_t column_handle,
                                 int value)
{
    OdbcInsertFieldData* f = (OdbcInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    // exception case for Microsoft Excel
    if (m_set->m_db_type == tango::dbtypeExcel)
        f->m_dbl_val = value;

    f->m_indicator = 0;
    f->m_int_val = value;
    f->m_dbl_val = value;

    return true;
}

bool OdbcRowInserter::putBoolean(tango::objhandle_t column_handle,
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

bool OdbcRowInserter::putDateTime(tango::objhandle_t column_handle,
                                  tango::datetime_t datetime)
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
        tango::DateTime dt(datetime);

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
            f->m_indicator = SQL_NULL_DATA;
        }
    }



    return true;
}

bool OdbcRowInserter::putNull(tango::objhandle_t column_handle)
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
    SQLAllocConnect(m_set->m_env, &m_conn);

    IOdbcDatabasePtr odb = m_set->m_database;
    r = odb->connect(m_conn);
        
    if (r == SQL_NO_DATA || r == SQL_ERROR || r == SQL_INVALID_HANDLE)
        return false;

    //r = SQLSetConnectAttr(m_conn, SQL_ATTR_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);

    r = SQLAllocHandle(SQL_HANDLE_STMT, m_conn, &m_stmt);


    m_inserting = true;


    // get the quote characters
    tango::IAttributesPtr attr = m_set->m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);


    tango::IStructurePtr s = m_set->getStructure();
    int col_count = s->getColumnCount();
    int i;

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

        tango::IColumnInfoPtr col_info = s->getColumnInfoByIdx(i);
        
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
    tablename += m_set->m_tablename;
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






