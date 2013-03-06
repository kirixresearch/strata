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
#include "inserter.h"
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
    return m_database->describeTable(m_tablename);
}

bool PgsqlSet::modifyStructure(tango::IStructure* struct_config,
                               tango::IJob* job)
{
    return true;
}

tango::IRowInserterPtr PgsqlSet::getRowInserter()
{
    PgsqlRowInserter* inserter = new PgsqlRowInserter(m_database, m_tablename);
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

    tango::IAttributesPtr attr = m_database->getAttributes();
    std::wstring quote_openchar = attr->getStringAttribute(tango::dbattrIdentifierQuoteOpenChar);
    std::wstring quote_closechar = attr->getStringAttribute(tango::dbattrIdentifierQuoteCloseChar);    
    
    query = L"SELECT * FROM ";
    query += quote_openchar;
    query += m_tablename;
    query += quote_closechar;

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


