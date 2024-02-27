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


#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <xd/xd.h>
#include "database.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "util.h"



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


DuckdbIterator::DuckdbIterator(DuckdbDatabase* database)
{
    m_eof = false;

    m_oid = 0;
    m_ordinal = 0;
    m_position = 1;
    m_row_count = -1;  // -1 means unknown
    m_mode = DuckdbIterator::modeSqliteResult;

    m_database = database;
    m_database->ref();

    m_conn = m_database->getPoolConnection();
}

DuckdbIterator::~DuckdbIterator()
{
    m_database->freePoolConnection(m_conn);
    m_database->unref();
}

bool DuckdbIterator::init(const std::wstring& _query)
{
    if (!m_tablename.empty())
    {
        // if we are iterating on a simple table, we can
        // get better type information by querying this from the db

        m_table_structure = m_database->describeTable(m_tablename);
        if (m_table_structure.isNull())
            return false;
    }

    std::string sql = (const char*)kl::toUtf8(_query);

    m_query_result = m_conn->Query(sql);
    if (m_query_result->HasError())
    {
        m_query_result.release();
        return false;
    }

    m_result.setQueryResult(m_query_result);


    // prepare the sql query
    initColumns();
    skip(1);

    return true;
}

void DuckdbIterator::initColumns()
{
    m_columns.clear();
    size_t i, col_count = m_result.GetColumnCount();

    for (i = 0; i < col_count; ++i)
    {
        DuckdbDataAccessInfo dai;

        duckdb::LogicalType& logical_type = m_result.GetColumnType(i);
        duckdb::PhysicalType physical_type = logical_type.InternalType();

        dai.name = kl::fromUtf8(m_result.GetColumnName(i));
        dai.sqlite_type = 0;
        dai.col_ordinal = (int)i;
        dai.calculated = false;

        dai.width = 30;
        dai.scale = 0;

        switch (logical_type.id())
        {
            default:
            case duckdb::LogicalTypeId::VARCHAR:
                dai.xd_type = xd::typeCharacter;
                break;

            case duckdb::LogicalTypeId::DECIMAL:
            {
                uint8_t width = 15, scale = 0;
                logical_type.GetDecimalProperties(width, scale);

                dai.xd_type = xd::typeNumeric;
                dai.scale = scale;
                break;
            }
            
            case duckdb::LogicalTypeId::FLOAT:
            case duckdb::LogicalTypeId::DOUBLE:
                dai.xd_type = xd::typeDouble;
                break;

            case duckdb::LogicalTypeId::TINYINT:
            case duckdb::LogicalTypeId::SMALLINT:
            case duckdb::LogicalTypeId::INTEGER:
            case duckdb::LogicalTypeId::BIGINT:
                dai.xd_type = xd::typeInteger;
                break;

            case duckdb::LogicalTypeId::BOOLEAN:
                dai.xd_type = xd::typeBoolean;
                break;

            case duckdb::LogicalTypeId::DATE:
                dai.xd_type = xd::typeDate;
                break;

            case duckdb::LogicalTypeId::TIMESTAMP:
            case duckdb::LogicalTypeId::TIMESTAMP_SEC:
            case duckdb::LogicalTypeId::TIMESTAMP_MS:
            case duckdb::LogicalTypeId::TIMESTAMP_NS:
                dai.xd_type = xd::typeDateTime;
                break;
        }

        /*
        switch (physical_type)
        {
            default:
            case duckdb::PhysicalType::VARCHAR:
                dai.xd_type = xd::typeCharacter;
                break;

            case duckdb::PhysicalType::FLOAT:
            case duckdb::PhysicalType::DOUBLE:
                dai.xd_type = xd::typeDouble;
                break;

            case duckdb::PhysicalType::INT8:
            case duckdb::PhysicalType::INT16:
            case duckdb::PhysicalType::INT32:
            case duckdb::PhysicalType::INT64:
                dai.xd_type = xd::typeInteger;
                break;

            case duckdb::PhysicalType::BOOL:
                dai.xd_type = xd::typeBoolean;
                break;
        }
        */



        if (m_table_structure.isOk())
        {
            const xd::ColumnInfo& colinfo = m_table_structure.getColumnInfo(dai.name);
            if (colinfo.isOk())
            {
                dai.xd_type = colinfo.type;
                dai.width = colinfo.width;
                dai.scale = colinfo.scale;
                dai.calculated = colinfo.calculated;
            }
        }

        m_columns.push_back(dai);
    }

}

bool DuckdbIterator::init(const xd::QueryParams& qp)
{
    std::wstring columns = qp.columns;
    if (columns.length() == 0)
        columns = L"*";


    std::wstring quoted_object_name = xdGetTablenameFromPath(qp.from, true);

    std::wstring sql = L"SELECT %columns% FROM %table%";
    kl::replaceStr(sql, L"%columns%", columns);
    kl::replaceStr(sql, L"%table%", quoted_object_name);

    if (qp.where.length() > 0)
        sql += L" WHERE " + qp.where;

    if (qp.order.length() > 0)
        sql += L" ORDER BY " + qp.order;

    setTable(xdSanitizePath(qp.from));

    return init(sql);
}


void DuckdbIterator::setTable(const std::wstring& tbl)
{
    m_path = tbl;
    m_tablename = tbl;
}


std::wstring DuckdbIterator::getTable()
{
    return m_path;
}

xd::rowpos_t DuckdbIterator::getRowCount()
{
    return m_row_count >= 0 ? m_row_count : 0;
}

xd::IDatabasePtr DuckdbIterator::getDatabase()
{
    return m_database;
}

xd::IIteratorPtr DuckdbIterator::clone()
{
    return xcm::null;
}

unsigned int DuckdbIterator::getIteratorFlags()
{
    unsigned int flags;

    if (m_mode == DuckdbIterator::modeOffsetLimit || m_mode == DuckdbIterator::modeRowidRange)
        flags = 0;
    else
        flags = xd::ifForwardOnly;

    if (m_row_count >= 0)
    {
        flags |= xd::ifFastRowCount;
    }

    return flags;
}

void DuckdbIterator::clearFieldData()
{
}

void DuckdbIterator::skip(int delta)
{
    while (delta > 0)
    {
        if (m_result.next())
        {
            --delta;
            m_position++;
        }
        else
        {
            m_eof = (delta > 0) ? true : false;
            break;
        }
    }


    size_t i, col_count = m_result.GetColumnCount();

    for (i = 0; i < col_count; ++i)
    {
        m_columns[i].value = m_result.GetValue(i);
    }
    
/*
    if (m_mode == DuckdbIterator::modeOffsetLimit || m_mode == DuckdbIterator::modeRowidRange)
    {
        m_position += delta;
        loadRow();
        return;
    }

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

        m_position++;
    }
    */
}

void DuckdbIterator::loadRow()
{
/*
    // first, see if the row is already in our cache
    if (m_cache.getRow(m_position, m_cache_row))
    {
        // row is loaded, we're all set
        return;
    }


    LocalRow2 row;

    for (i = 0; i < cnt; ++i)
    {
        text = sqlite3_column_text(stmt, i);
        LocalRowValue v;
        v.setData(text, strlen((const char*)text) + 1);
        row.setColumnData(i, v);
    }

    m_cache.putRow(rown, row);

    */
}


void DuckdbIterator::goFirst()
{
    if (m_mode == DuckdbIterator::modeOffsetLimit || m_mode == DuckdbIterator::modeRowidRange)
    {
        m_position = 1;
        loadRow();
    }

}

void DuckdbIterator::goLast()
{

}

xd::rowid_t DuckdbIterator::getRowId()
{
    return rowidCreate(m_ordinal, m_oid);
}

bool DuckdbIterator::bof()
{
    return false;
}

bool DuckdbIterator::eof()
{
    if (m_eof)
    {
        return true;
    }

    return false;
}

bool DuckdbIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool DuckdbIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool DuckdbIterator::setPos(double pct)
{
    return false;
}

double DuckdbIterator::getPos()
{
    return 0.0;
}

void DuckdbIterator::goRow(const xd::rowid_t& rowid)
{
    if (m_mode == DuckdbIterator::modeOffsetLimit || m_mode == DuckdbIterator::modeRowidRange)
    {
        m_position = (long long)rowid;
        loadRow();
    }
    
}

xd::Structure DuckdbIterator::getStructure()
{
    if (m_structure.isOk())
        return m_structure;

    std::vector<DuckdbDataAccessInfo>::iterator it;
    for (it = m_columns.begin(); it != m_columns.end(); ++it)
    {
        xd::ColumnInfo col;
        col.name = it->name;
        col.type = it->xd_type;
        col.width = it->width;
        col.scale = it->scale;
        col.column_ordinal = it->col_ordinal;
        col.calculated = it->calculated;
        m_structure.createColumn(col);
    }
    
    return m_structure;
}

bool DuckdbIterator::refreshStructure()
{
    return true;
}

bool DuckdbIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    return false;
}


xd::objhandle_t DuckdbIterator::getHandle(const std::wstring& expr)
{
    std::vector<DuckdbDataAccessInfo>::iterator it;
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

bool DuckdbIterator::releaseHandle(xd::objhandle_t data_handle)
{
    if (!data_handle)
        return false;
        
    return true;
}

xd::ColumnInfo DuckdbIterator::getInfo(xd::objhandle_t data_handle)
{
    DuckdbDataAccessInfo* dai = (DuckdbDataAccessInfo*)data_handle;

    xd::ColumnInfo colinfo;
    colinfo.name = dai->name;
    colinfo.type = dai->xd_type;
    colinfo.width = dai->width;
    colinfo.scale = dai->scale;
    colinfo.column_ordinal = dai->col_ordinal;
    colinfo.calculated = dai->calculated;

    return colinfo;
}

int DuckdbIterator::getType(xd::objhandle_t data_handle)
{
    DuckdbDataAccessInfo* dai = (DuckdbDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;

    return dai->xd_type;
}


int DuckdbIterator::getRawWidth(xd::objhandle_t data_handle)
{
    return 0;
}

const unsigned char* DuckdbIterator::getRawPtr(xd::objhandle_t data_handle)
{
    return NULL;
}

const std::string& DuckdbIterator::getString(xd::objhandle_t data_handle)
{
    DuckdbDataAccessInfo* dai = (DuckdbDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
    {
        return empty_string;
    }
     else
    {
        dai->result_str = dai->value.ToString();
    }

    return dai->result_str;
}

const std::wstring& DuckdbIterator::getWideString(xd::objhandle_t data_handle)
{
    DuckdbDataAccessInfo* dai = (DuckdbDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
    {
        return empty_wstring;
    }
    else
    {
        dai->result_str = dai->value.ToString();
        dai->result_wstr = kl::fromUtf8(dai->result_str);
    }

    return dai->result_wstr;

}

xd::datetime_t DuckdbIterator::getDateTime(xd::objhandle_t data_handle)
{
    DuckdbDataAccessInfo* dai = (DuckdbDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
    {
        return 0;
    }

    int32_t year = 0, month = 0, day = 0;
    int32_t hour = 0, minute = 0, second = 0;
    duckdb::Date::Convert(dai->value.GetValue<duckdb::date_t>(), year, month, day);

    xd::datetime_t dt = dateToJulian(year, month, day);

    xd::datetime_t tm = ((hour * 3600000) +
         (minute * 60000) +
         (second * 1000));

    dt <<= 32;
    dt |= tm;

    return dt;


    /*


    if (m_mode == DuckdbIterator::modeOffsetLimit || m_mode == DuckdbIterator::modeRowidRange)
    {
        // TODO:
        return 0;
    }


    xd::datetime_t dt, tm;
    
    int y = 0, m = 0, d = 0, hh = 0, mm = 0, ss = 0;
    char buf[25];
    const char* coltext = (const char*)sqlite3_column_text(m_stmt, dai->col_ordinal);
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
    */
    return 0;
}

double DuckdbIterator::getDouble(xd::objhandle_t data_handle)
{
    DuckdbDataAccessInfo* dai = (DuckdbDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0.0;

    return dai->value.GetValue<double>();
}

int DuckdbIterator::getInteger(xd::objhandle_t data_handle)
{
    DuckdbDataAccessInfo* dai = (DuckdbDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0;

    return dai->value.GetValue<int>();
}

bool DuckdbIterator::getBoolean(xd::objhandle_t data_handle)
{
    DuckdbDataAccessInfo* dai = (DuckdbDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return false;

    return dai->value.GetValue<bool>();
}

bool DuckdbIterator::isNull(xd::objhandle_t data_handle)
{
    DuckdbDataAccessInfo* dai = (DuckdbDataAccessInfo*)data_handle;
    if (!dai)
        return true;

    return dai->value.IsNull();
}