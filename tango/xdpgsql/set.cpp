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
#include "set.h"
#include "iterator.h"
#include "../xdcommon/dbfuncs.h"
#include "../xdcommon/extfileinfo.h"
#include "../xdcommon/structure.h"
#include <kl/md5.h>


PgsqlSet::PgsqlSet(PgsqlDatabase* database)
{
    m_database = database;
    m_database->ref();

    m_filter_query = false;
    m_tablename = L"";
    
    m_cached_row_count = (tango::rowpos_t)-1;
}

PgsqlSet::~PgsqlSet()
{
    m_database->unref();
}

bool PgsqlSet::init()
{
    return true;
}



void PgsqlSet::setObjectPath(const std::wstring& path)
{
    m_path = path;
}

std::wstring PgsqlSet::getObjectPath()
{
    if (m_path.empty())
        return m_tablename;
        
    return m_path;
}


unsigned int PgsqlSet::getSetFlags()
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

std::wstring PgsqlSet::getSetId()
{
    if (m_set_id.length() == 0)
    {
        std::wstring id = L"xdpgsql:";
        
        std::wstring server = m_database->getServer();
        kl::makeLower(server);

        id += server;
        id += L":";
        
        
        std::wstring table_name = m_tablename;
        kl::makeLower(table_name);
            
        id += table_name;
        
        m_set_id = kl::md5str(id);
    }
    
    return m_set_id;
}





tango::IStructurePtr PgsqlSet::getStructure()
{
    // create new tango::IStructure
    Structure* s = new Structure;

    m_structure = static_cast<tango::IStructure*>(s);

    tango::IStructurePtr ret = m_structure->clone();
    appendCalcFields(ret);
    return ret;
}

bool PgsqlSet::modifyStructure(tango::IStructure* struct_config,
                              tango::IJob* job)
{
    return true;
}

tango::IRowInserterPtr PgsqlSet::getRowInserter()
{
    PgsqlRowInserter* inserter = new PgsqlRowInserter(this);
    return static_cast<tango::IRowInserter*>(inserter);
}

tango::IRowDeleterPtr PgsqlSet::getRowDeleter()
{
    return xcm::null;
}

int PgsqlSet::insert(tango::IIteratorPtr source_iter,
                    const std::wstring& where_condition,
                    int max_rows,
                    tango::IJob* job)
{
    return xdcmnInsert(source_iter, this, where_condition, max_rows, job);
}

tango::IIteratorPtr PgsqlSet::createIterator(const std::wstring& columns,
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

    if (m_where_condition.length() > 0)
    {
        query += L" WHERE ";
        query += m_where_condition;
    }

    if (expr.length() > 0)
    {
        query += L" ORDER BY ";
        query += expr;
    }
    
    // create an iterator based on our select statement
    PgsqlIterator* iter = new PgsqlIterator(m_database, this);

    // initialize Odbc connection for this set
    if (!iter->init(query))
    {
        return xcm::null;
    }

    return static_cast<tango::IIterator*>(iter);
}

tango::rowpos_t PgsqlSet::getRowCount()
{
    if (m_cached_row_count != (tango::rowpos_t)-1)
        return m_cached_row_count;


    tango::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);     
    
    std::wstring query;
    query += L"SELECT COUNT(*) AS xdpgsqlres FROM ";
    query += quote_openchar;
    query += m_tablename;
    query += quote_closechar;


    PgsqlIterator* iter = new PgsqlIterator(m_database, this);
    iter->ref();

    // initialize Odbc connection for this set
    if (!iter->init(query))
    {
        iter->unref();
        return 0;
    }

    iter->goFirst();
    tango::objhandle_t h = iter->getHandle(L"xdpgsqlres");
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

    return result;
}



void PgsqlSet::setWhereCondition(const std::wstring& condition)
{
    m_where_condition = condition;
}
















PgsqlRowInserter::PgsqlRowInserter(PgsqlSet* set)
{
    m_set = set;
    m_set->ref();

    m_inserting = false;

}

PgsqlRowInserter::~PgsqlRowInserter()
{
    std::vector<PgsqlInsertFieldData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete *it;

    m_set->unref();
}

tango::objhandle_t PgsqlRowInserter::getHandle(const std::wstring& column_name)
{
    std::vector<PgsqlInsertFieldData*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (!wcscasecmp((*it)->m_name.c_str(), column_name.c_str()))
            return (tango::objhandle_t)(*it);
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

bool PgsqlRowInserter::putWideString(tango::objhandle_t column_handle,
                                    const std::wstring& value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
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

bool PgsqlRowInserter::putDouble(tango::objhandle_t column_handle,
                                double value)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_indicator = 0;
    f->m_dbl_val = value;
    f->m_int_val = (int)value;

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

    // exception case for Microsoft Excel
    if (m_set->m_db_type == tango::dbtypeExcel)
        f->m_dbl_val = value;

    f->m_indicator = 0;
    f->m_int_val = value;
    f->m_dbl_val = value;

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

    f->m_indicator = 0;
    f->m_bool_val = value;
    f->m_int_val = value ? 1 : 0;
    f->m_dbl_val = value ? 1.0 : 0.0;

    return true;
}

bool PgsqlRowInserter::putDateTime(tango::objhandle_t column_handle,
                                  tango::datetime_t datetime)
{
/*
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_indicator = 0;

    if (datetime == 0)
    {
        // null date
        f->m_indicator = 1;
        return true;
    }


    if (true)  // date
    {
        tango::DateTime dt(datetime);

        f->m_date_val.year = dt.getYear();
        f->m_date_val.month = dt.getMonth();
        f->m_date_val.day = dt.getDay();

        // SQL Server cannot handle dates less than 1753
        if (f->m_date_val.year < 1753)
        {
            f->m_indicator = 1;
        }
    }
     else if (true) //datetime
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
            f->m_indicator = 1;
        }
    }


    */
    return true;
}

bool PgsqlRowInserter::putNull(tango::objhandle_t column_handle)
{
    PgsqlInsertFieldData* f = (PgsqlInsertFieldData*)column_handle;
    if (!f)
    {
        return false;
    }

    f->m_indicator = 1;

    return false;
}

bool PgsqlRowInserter::startInsert(const std::wstring& col_list)
{
    return true;
}

bool PgsqlRowInserter::insertRow()
{
    if (!m_inserting)
        return false;

    return true;
}

void PgsqlRowInserter::finishInsert()
{
}

bool PgsqlRowInserter::flush()
{
    return true;
}






