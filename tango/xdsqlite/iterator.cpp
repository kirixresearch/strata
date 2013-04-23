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
#include "tango.h"
#include "database.h"
#include "inserter.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommonsql/xdcommonsql.h"


inline tango::rowid_t rowidCreate(tango::tableord_t table_ordinal,
                                  tango::rowpos_t row_num)
{
    tango::rowid_t r;
    r = ((tango::rowid_t)table_ordinal) << 36;
    r |= row_num;
    return r;
}

inline tango::rowpos_t rowidGetRowPos(tango::rowid_t rowid)
{
    #ifdef _MSC_VER
    return (rowid & 0xfffffffff);
    #else
    return (rowid & 0xfffffffffLL);
    #endif
}

inline tango::tableord_t rowidGetTableOrd(tango::rowid_t rowid)
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
    
    

    // prepare the sql query

    std::string ascsql = kl::tostring(query);

    int rc =  sqlite3_prepare(m_sqlite,
                              ascsql.c_str(),
                              ascsql.length(),
                              &m_stmt,
                              NULL);

    if (rc != SQLITE_OK)
        return false;

    skip(1);

    
    m_columns.clear();
    int col_count = sqlite3_column_count(m_stmt);
    int i;

    // (first column is the oid)
    for (i = 1; i < col_count; ++i)
    {
        SlDataAccessInfo dai;

        dai.name = kl::towstring((char*)sqlite3_column_name(m_stmt, i));
        dai.col_ordinal = i;

        if (m_set_structure)
        {   
            dai.colinfo = m_set_structure->getColumnInfo(dai.name);
        }

        if (dai.colinfo.isNull())
        {
            dai.colinfo = new ColumnInfo;
            // fill this out
        }

        m_columns.push_back(dai);
    }

    return true;
}

std::wstring SlIterator::getTable()
{
    // TODO: implement
    return L"";
}

tango::rowpos_t SlIterator::getRowCount()
{
    return 0;
}

tango::IDatabasePtr SlIterator::getDatabase()
{
    return m_database;
}

tango::IIteratorPtr SlIterator::clone()
{
    return xcm::null;
}

unsigned int SlIterator::getIteratorFlags()
{
    return tango::ifForwardOnly;
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

tango::rowid_t SlIterator::getRowId()
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

void SlIterator::goRow(const tango::rowid_t& rowid)
{
}

tango::IStructurePtr SlIterator::getStructure()
{
    return m_set_structure;
}

void SlIterator::refreshStructure()
{
}

bool SlIterator::modifyStructure(tango::IStructure* struct_config,
                                   tango::IJob* job)
{
    return false;
}


tango::objhandle_t SlIterator::getHandle(const std::wstring& expr)
{
    std::vector<SlDataAccessInfo>::iterator it;
    for (it = m_columns.begin();
         it != m_columns.end();
         ++it)
    {
        if (0 == wcscasecmp(it->name.c_str(),
                            expr.c_str()))
        {
            return (tango::objhandle_t)(&(*it));
        }
    }

    return 0;
}

bool SlIterator::releaseHandle(tango::objhandle_t data_handle)
{
    if (!data_handle)
        return false;
        
    return true;
}

tango::IColumnInfoPtr SlIterator::getInfo(tango::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;
    if (!dai)
        return xcm::null;

    return dai->colinfo->clone();
}

int SlIterator::getType(tango::objhandle_t data_handle)
{
    tango::IColumnInfoPtr colinfo = getInfo(data_handle);
    if (colinfo.isNull())
        return tango::typeInvalid;

    return colinfo->getType();
}

int SlIterator::getRawWidth(tango::objhandle_t data_handle)
{
    return 0;
}

const unsigned char* SlIterator::getRawPtr(tango::objhandle_t data_handle)
{
    return NULL;
}

const std::string& SlIterator::getString(tango::objhandle_t data_handle)
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

const std::wstring& SlIterator::getWideString(tango::objhandle_t data_handle)
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

tango::datetime_t SlIterator::getDateTime(tango::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
    {
        return 0;
    }

    tango::datetime_t dt, tm;
    
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

double SlIterator::getDouble(tango::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0.0;

    return sqlite3_column_double(m_stmt, dai->col_ordinal);
}

int SlIterator::getInteger(tango::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0;

    return sqlite3_column_int(m_stmt, dai->col_ordinal);
}

bool SlIterator::getBoolean(tango::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0;
        
    return (sqlite3_column_int(m_stmt, dai->col_ordinal) == 0) ? false : true;
}

bool SlIterator::isNull(tango::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;
    if (!dai)
        return true;

    if (sqlite3_column_type(m_stmt, dai->col_ordinal) == SQLITE_NULL)
        return true;

    return false;
}



