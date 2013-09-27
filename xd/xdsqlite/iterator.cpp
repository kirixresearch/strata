/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include "sqlite3.h"

#include <kl/portable.h>
#include <kl/utf8.h>
#include <xd/xd.h>
#include "database.h"
#include "inserter.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommonsql/xdcommonsql.h"


inline xd::rowid_t rowidCreate(xd::tableord_t table_ordinal,
                                  xd::rowpos_t row_num)
{
    xd::rowid_t r;
    r = ((xd::rowid_t)table_ordinal) << 36;
    r |= row_num;
    return r;
}

inline xd::rowpos_t rowidGetRowPos(xd::rowid_t rowid)
{
    #ifdef _MSC_VER
    return (rowid & 0xfffffffff);
    #else
    return (rowid & 0xfffffffffLL);
    #endif
}

inline xd::tableord_t rowidGetTableOrd(xd::rowid_t rowid)
{
    return (rowid >> 36);
}



const std::string empty_string = "";
const std::wstring empty_wstring = L"";


SlIterator::SlIterator(SlDatabase* database)
{
    m_eof = false;
    m_stmt = NULL;
    m_oid = 0;
    m_ordinal = 0;

    m_database = database;
    m_sqlite = database->m_sqlite;
    m_database->ref();
}

SlIterator::~SlIterator()
{
    if (m_stmt)
    {
        sqlite3_finalize(m_stmt);
    }


    m_database->unref();
}

bool SlIterator::init(const std::wstring& _query)
{
/*
    // add rowid to the select statement
    const wchar_t* q = _query.c_str();
    while (iswspace(*q))
        q++;
    if (0 == wcsncasecmp(q, L"SELECT", 6))
        q += 6;
    while (iswspace(*q))
        q++;

    std::wstring query;
    query = L"SELECT oid,";
    query += q;
*/
    
    if (!m_tablename.empty())
    {
        // if we are iterating on a simple table, we can
        // get better type information by querying this from the db

        m_table_structure = m_database->describeTable(m_tablename);
        if (m_table_structure.isNull())
            return false;
    }


    // prepare the sql query

    std::string ascsql = kl::tostring(_query);

    int rc =  sqlite3_prepare(m_sqlite,
                              ascsql.c_str(),
                              ascsql.length(),
                              &m_stmt,
                              NULL);

    if (rc != SQLITE_OK)
        return false;

    skip(1);

    
    m_columns.clear();
    int i, col_count = sqlite3_column_count(m_stmt);

    for (i = 0; i < col_count; ++i)
    {
        SlDataAccessInfo dai;

        dai.name = kl::towstring((char*)sqlite3_column_name(m_stmt, i));
        dai.sqlite_type = sqlite3_column_type(m_stmt, i);
        dai.col_ordinal = i;

        switch (dai.sqlite_type)
        {
            case SQLITE_INTEGER: dai.xd_type = xd::typeInteger; break;
            case SQLITE_FLOAT:   dai.xd_type = xd::typeDouble; break;
            case SQLITE_BLOB:    dai.xd_type = xd::typeBinary; break;
            case SQLITE_TEXT:    dai.xd_type = xd::typeCharacter; break;
        }

        dai.width = 30;
        dai.scale = 0;

        xd::IColumnInfoPtr colinfo;
        if (m_table_structure.isOk())
        {
            xd::IColumnInfoPtr colinfo = m_table_structure->getColumnInfo(dai.name);
            if (colinfo.isOk())
            {
                dai.xd_type = colinfo->getType();
                dai.width = colinfo->getWidth();
                dai.scale = colinfo->getScale();
            }
        }
        
        m_columns.push_back(dai);
    }

    return true;
}


void SlIterator::setTable(const std::wstring& tbl)
{
    m_path = tbl;
}


std::wstring SlIterator::getTable()
{
    return m_path;
}

xd::rowpos_t SlIterator::getRowCount()
{
    return 0;
}

xd::IDatabasePtr SlIterator::getDatabase()
{
    return m_database;
}

xd::IIteratorPtr SlIterator::clone()
{
    return xcm::null;
}

unsigned int SlIterator::getIteratorFlags()
{
    return xd::ifForwardOnly;
}

void SlIterator::clearFieldData()
{
}

void SlIterator::skip(int delta)
{
    int i, rc;
    for (i = 0; i < delta; ++i)
    {
        while (1)
        {
            rc = sqlite3_step(m_stmt);
            if (rc == SQLITE_ROW)
            {
                // record rowid
                m_oid = sqlite3_column_int64(m_stmt, 0);
                break;
            }
             else if (rc == SQLITE_DONE)
            {
                m_eof = true;
                m_oid = 0;
                return;
            }
             else if (rc == SQLITE_BUSY)
            {
                continue;
            }
             else
            {
                return;
            }
        }    
    }
}

void SlIterator::goFirst()
{

}

void SlIterator::goLast()
{

}

xd::rowid_t SlIterator::getRowId()
{
    return rowidCreate(m_ordinal, m_oid);
}

bool SlIterator::bof()
{
    return false;
}

bool SlIterator::eof()
{
    if (m_eof)
    {
        return true;
    }

    return false;
}

bool SlIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool SlIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool SlIterator::setPos(double pct)
{
    return false;
}

double SlIterator::getPos()
{
    return 0.0;
}

void SlIterator::goRow(const xd::rowid_t& rowid)
{
}

xd::IStructurePtr SlIterator::getStructure()
{
    if (m_structure.isOk())
        return m_structure->clone();

    Structure* s = new Structure;

    std::vector<SlDataAccessInfo>::iterator it;
    for (it = m_columns.begin(); it != m_columns.end(); ++it)
    {
        ColumnInfo* col = new ColumnInfo;
        col->setName(it->name);
        col->setType(it->xd_type);
        col->setWidth(it->width);
        col->setScale(it->scale);
        col->setColumnOrdinal(it->col_ordinal);
        s->addColumn(static_cast<xd::IColumnInfo*>(col));
    }
    
    m_structure = static_cast<xd::IStructure*>(s);

    return m_structure->clone();

}

void SlIterator::refreshStructure()
{
}

bool SlIterator::modifyStructure(xd::IStructure* struct_config,
                                   xd::IJob* job)
{
    return false;
}


xd::objhandle_t SlIterator::getHandle(const std::wstring& expr)
{
    std::vector<SlDataAccessInfo>::iterator it;
    for (it = m_columns.begin();
         it != m_columns.end();
         ++it)
    {
        if (0 == wcscasecmp(it->name.c_str(),
                            expr.c_str()))
        {
            return (xd::objhandle_t)(&(*it));
        }
    }

    return 0;
}

bool SlIterator::releaseHandle(xd::objhandle_t data_handle)
{
    if (!data_handle)
        return false;
        
    return true;
}

xd::IColumnInfoPtr SlIterator::getInfo(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    ColumnInfo* col = new ColumnInfo;
    col->setName(dai->name);
    col->setType(dai->xd_type);
    col->setWidth(dai->width);
    col->setScale(dai->scale);
    col->setColumnOrdinal(dai->col_ordinal);
    
    return static_cast<xd::IColumnInfo*>(col);
}

int SlIterator::getType(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;
    return dai->xd_type;
}

int SlIterator::getRawWidth(xd::objhandle_t data_handle)
{
    return 0;
}

const unsigned char* SlIterator::getRawPtr(xd::objhandle_t data_handle)
{
    return NULL;
}

const std::string& SlIterator::getString(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
    {
        return empty_string;
    }
     else
    {
        dai->result_str = kl::tostring(kl::fromUtf8((char*)sqlite3_column_text(m_stmt, dai->col_ordinal)));
    }

    return dai->result_str;
}

const std::wstring& SlIterator::getWideString(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
    {
        return empty_wstring;
    }
     else
    {
        dai->result_wstr = kl::fromUtf8((char*)sqlite3_column_text(m_stmt, dai->col_ordinal));
    }

    return dai->result_wstr;

}

xd::datetime_t SlIterator::getDateTime(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
    {
        return 0;
    }

    xd::datetime_t dt, tm;
    
    int y = 0, m = 0, d = 0, hh = 0, mm = 0, ss = 0;
    char buf[25];
    char* coltext = (char*)sqlite3_column_text(m_stmt, dai->col_ordinal);
    buf[0] = 0;
    if (coltext)
    {
        strncpy(buf, coltext, 24);
        buf[24] = 0;
    }
    int len = strlen(buf);


    if (len >= 10)
    {
        buf[4] = 0;
        buf[7] = 0;
        buf[10] = 0;
        buf[13] = 0;
        buf[16] = 0;

        y = atoi(buf);
        m = atoi(buf+5);
        d = atoi(buf+8);

        if (len >= 19)
        {
            hh = atoi(buf+11);
            mm = atoi(buf+14);
            ss = atoi(buf+17);
        }

        // some quick checks
        if (y == 0)
            return 0;
        if (m < 1 || m > 12)
            return 0;
        if (d < 1 || d > 31)
            return 0;
    }


    dt = dateToJulian(y,m,d);

    tm = ((hh*3600000) +
          (mm*60000) +
          (ss*1000));

    dt <<= 32;
    dt |= tm;

    return dt;
}

double SlIterator::getDouble(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0.0;

    return sqlite3_column_double(m_stmt, dai->col_ordinal);
}

int SlIterator::getInteger(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0;

    return sqlite3_column_int(m_stmt, dai->col_ordinal);
}

bool SlIterator::getBoolean(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0;
        
    return (sqlite3_column_int(m_stmt, dai->col_ordinal) == 0) ? false : true;
}

bool SlIterator::isNull(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;
    if (!dai)
        return true;

    if (sqlite3_column_type(m_stmt, dai->col_ordinal) == SQLITE_NULL)
        return true;

    return false;
}



