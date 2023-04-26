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
    m_position = 1;
    m_row_count = -1;  // -1 means unknown
    m_mode = SlIterator::modeSqliteResult;

    m_database = database;
    m_database->ref();

    m_sqlite = m_database->getPoolDatabase();
}

SlIterator::~SlIterator()
{
    if (m_stmt)
    {
        sqlite3_finalize(m_stmt);
    }

    m_database->freePoolDatabase(m_sqlite);
    m_database->unref();
}

bool SlIterator::init(const std::wstring& _query)
{
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

    int rc =  sqlite3_prepare_v2(m_sqlite,
                              ascsql.c_str(),
                              ascsql.length(),
                              &m_stmt,
                              NULL);

    if (rc != SQLITE_OK)
        return false;

    skip(1);

    initColumns(m_stmt);

    return true;
}

void SlIterator::initColumns(sqlite3_stmt* stmt)
{
    m_columns.clear();
    int i, col_count = sqlite3_column_count(stmt);

    for (i = 0; i < col_count; ++i)
    {
        SlDataAccessInfo dai;

        dai.name = kl::towstring((char*)sqlite3_column_name(stmt, i));
        dai.sqlite_type = sqlite3_column_type(stmt, i);
        dai.col_ordinal = i;
        dai.calculated = false;

        switch (dai.sqlite_type)
        {
            case SQLITE_INTEGER: dai.xd_type = xd::typeInteger; break;
            case SQLITE_FLOAT:   dai.xd_type = xd::typeDouble; break;
            case SQLITE_BLOB:    dai.xd_type = xd::typeBinary; break;
            case SQLITE_TEXT:    dai.xd_type = xd::typeCharacter; break;
        }

        dai.width = 30;
        dai.scale = 0;

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

class SlFetchRowCountBackground : public kl::thread
{
public:

    SlFetchRowCountBackground(SlDatabase* database, const std::wstring quoted_object_name)
    {
        m_mutex.lock();

        m_database = database;
        m_database->ref();
        m_sqlite = NULL;
        m_row_count = -1;
        m_max_rowid = -1;
        m_done = false;

        m_mutex.unlock();

        m_quoted_object_name = kl::toUtf8(quoted_object_name);
    }

    virtual ~SlFetchRowCountBackground()
    {
    }

    virtual bool isAutoDelete()
    {
        return false; // this thread object should not be deleted when the thread exits
    }

    unsigned int entry()
    {
        sqlite3* sqlite = m_database->getPoolDatabase();

        m_mutex.lock();
        m_sqlite = sqlite;
        m_mutex.unlock();

        std::string sql;
        sqlite3_stmt* stmt = NULL;
        
        // get max row id
        sql = "select max(rowid) from " + m_quoted_object_name;

        sqlite3_prepare_v2(sqlite, sql.c_str(), -1, &stmt, NULL);
        if (stmt)
        {
            int res = sqlite3_step(stmt);

            if (res == SQLITE_ROW)
            {
                long long max_rowid = sqlite3_column_int64(stmt, 0);

                m_mutex.lock();
                m_max_rowid = max_rowid;
                m_mutex.unlock();
            }

            sqlite3_finalize(stmt);
        }

        // get record count in table
        sql = "select count(rowid) from " + m_quoted_object_name;

        sqlite3_prepare_v2(sqlite, sql.c_str(), -1, &stmt, NULL);
        if (stmt)
        {
            int res = sqlite3_step(stmt);

            if (res == SQLITE_ROW)
            {
                long long row_count = sqlite3_column_int64(stmt, 0);

                m_mutex.lock();
                m_row_count = row_count;
                m_mutex.unlock();
            }

            sqlite3_finalize(stmt);
        }

        m_mutex.lock();
        m_sqlite = NULL;
        m_mutex.unlock();

        m_database->freePoolDatabase(sqlite);

        m_mutex.lock();
        m_done = true;
        m_mutex.unlock();

        return 0;
    }

    void cancel()
    {
        m_mutex.lock();
        if (m_sqlite)
        {
            sqlite3_interrupt(m_sqlite);
        }
        m_mutex.unlock();
    }

    bool isDone()
    {
        bool done;

        m_mutex.lock();
        done = m_done;
        m_mutex.unlock();

        return done;
    }

    long long getRowCount()
    {
        long long row_count;

        m_mutex.lock();
        row_count = m_row_count;
        m_mutex.unlock();

        return row_count;
    }

    long long getMaxRowid()
    {
        long long max_rowid;

        m_mutex.lock();
        max_rowid = m_max_rowid;
        m_mutex.unlock();

        return max_rowid;
    }

private:
    
    kl::mutex m_mutex;
    SlDatabase* m_database;
    sqlite3* m_sqlite;
    long long m_row_count;
    long long m_max_rowid;
    std::string m_quoted_object_name;
    bool m_done;
};


bool SlIterator::init(const xd::QueryParams& qp)
{
    std::wstring columns = qp.columns;
    if (columns.length() == 0)
        columns = L"*";

    std::wstring quoted_object_name = sqliteGetTablenameFromPath(qp.from, true);

    std::wstring sql = L"SELECT %columns% FROM %table%";
    kl::replaceStr(sql, L"%columns%", columns);
    kl::replaceStr(sql, L"%table%", quoted_object_name);

    if (qp.where.length() > 0)
        sql += L" WHERE " + qp.where;

    if (qp.order.length() > 0)
        sql += L" ORDER BY " + qp.order;

    setTable(qp.from);


    if (qp.executeFlags & xd::sqlBrowse)
    {
        // there are three strategies for a browse cursor/iterator:
        // 1) select chunks with limit + offset
        // 2) select chunks with rowid >= x and rowid <= y (only works when table has no deleted records)
        // 2) creating a temporary table with rowids

        SlFetchRowCountBackground* job = new SlFetchRowCountBackground(m_database, quoted_object_name);
        job->create();

        // wait up to eight seconds
        for (int i = 0; i < 800; ++i)
        {
            kl::thread::sleep(10);

            if (job->isDone())
            {
                break;
            }
        }

        if (!job->isDone())
        {
            job->cancel();
            while (!job->isDone())
            {
                kl::thread::sleep(10);
            }
        }


        long long row_count = -1;
        long long max_rowid = -1;
        if (job->isDone())
        {
            row_count = job->getRowCount();
            max_rowid = job->getMaxRowid();
        }

        delete job;

        if (row_count != -1 && max_rowid != -1)
        {
           m_row_count = row_count;

           if (max_rowid == row_count)
           {
               m_mode = SlIterator::modeRowidRange;
           }
           else
           {
               m_mode = SlIterator::modeOffsetLimit;
           }

           m_qp = qp;
           m_position = 1;
           loadRow();
           return true;
        }
    }

    return init(sql);
}


void SlIterator::setTable(const std::wstring& tbl)
{
    m_path = tbl;
    m_tablename = tbl;
}


std::wstring SlIterator::getTable()
{
    return m_path;
}

xd::rowpos_t SlIterator::getRowCount()
{
    return m_row_count >= 0 ? m_row_count : 0;
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
    unsigned int flags;

    if (m_mode == SlIterator::modeOffsetLimit || m_mode == SlIterator::modeRowidRange)
        flags = 0;
    else
        flags = xd::ifForwardOnly;

    if (m_row_count >= 0)
    {
        flags |= xd::ifFastRowCount;
    }

    return flags;
}

void SlIterator::clearFieldData()
{
}

void SlIterator::skip(int delta)
{
    if (m_mode == SlIterator::modeOffsetLimit || m_mode == SlIterator::modeRowidRange)
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
}

void SlIterator::loadRow()
{
    // first, see if the row is already in our cache
    if (m_cache.getRow(m_position, m_cache_row))
    {
        // row is loaded, we're all set
        return;
    }

    const long long page_size = 500;
    long long rown, page_start = ((m_position - 1) / page_size) * page_size;

    std::wstring columns = m_qp.columns;
    if (columns.length() == 0)
        columns = L"*";

    std::wstring quoted_object_name = sqliteGetTablenameFromPath(m_qp.from, true);

    std::wstring sql = L"SELECT %columns% FROM %table%";
    kl::replaceStr(sql, L"%columns%", columns);
    kl::replaceStr(sql, L"%table%", quoted_object_name);

    if (m_mode == SlIterator::modeRowidRange)
    {
        sql += kl::stdswprintf(L" WHERE rowid >= %lld AND rowid <= %lld", page_start+1, page_start+page_size);
    }

    sql += L" ORDER BY rowid";

    if (m_mode == SlIterator::modeOffsetLimit)
    {
        sql += kl::stdswprintf(L" LIMIT %lld OFFSET %lld", page_size, page_start);
    }

    std::string asql = (const char*)kl::toUtf8(sql);

    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(m_sqlite, asql.c_str(), -1, &stmt, NULL);
    if (stmt)
    {
        int i, cnt = sqlite3_column_count(stmt);

        int res;
        const unsigned char* text;

        rown = page_start+1;
        while (true)
        {
            res = sqlite3_step(stmt);

            if (m_columns.size() == 0)
            {
                initColumns(stmt);
            }

            if (res == SQLITE_ROW)
            {
                LocalRow2 row;

                for (i = 0; i < cnt; ++i)
                {
                    text = sqlite3_column_text(stmt, i);
                    LocalRowValue v;
                    v.setData(text, strlen((const char*)text) + 1);
                    row.setColumnData(i, v);
                }

                m_cache.putRow(rown, row);

                ++rown;
            }
            else if (res == SQLITE_DONE)
            {
                break;
            }
        }

        sqlite3_finalize(stmt);
    }
}


void SlIterator::goFirst()
{
    if (m_mode == SlIterator::modeOffsetLimit || m_mode == SlIterator::modeRowidRange)
    {
        m_position = 1;
        loadRow();
    }

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
    if (m_mode == SlIterator::modeOffsetLimit || m_mode == SlIterator::modeRowidRange)
    {
        m_position = (long long)rowid;
        loadRow();
    }
    
}

xd::Structure SlIterator::getStructure()
{
    if (m_structure.isOk())
        return m_structure;

    std::vector<SlDataAccessInfo>::iterator it;
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

bool SlIterator::refreshStructure()
{
    return true;
}

bool SlIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
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

xd::ColumnInfo SlIterator::getInfo(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    xd::ColumnInfo colinfo;
    colinfo.name = dai->name;
    colinfo.type = dai->xd_type;
    colinfo.width = dai->width;
    colinfo.scale = dai->scale;
    colinfo.column_ordinal = dai->col_ordinal;
    colinfo.calculated = dai->calculated;

    return colinfo;
}

int SlIterator::getType(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;

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
        const unsigned char* ptr = sqlite3_column_text(m_stmt, dai->col_ordinal);

        if (m_mode == SlIterator::modeSqliteResult)
        {
            ptr = sqlite3_column_text(m_stmt, dai->col_ordinal);
        }
        else
        {
            LocalRowValue& val = m_cache_row.getColumnData(dai->col_ordinal);
            ptr = val.getData();
        }

        
        dai->result_str = kl::tostring(kl::fromUtf8((const char*)ptr));
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
        const unsigned char* ptr = sqlite3_column_text(m_stmt, dai->col_ordinal);

        if (m_mode == SlIterator::modeSqliteResult)
        {
            ptr = sqlite3_column_text(m_stmt, dai->col_ordinal);
        }
        else
        {
            LocalRowValue& val = m_cache_row.getColumnData(dai->col_ordinal);
            ptr = val.getData();
        }



        dai->result_wstr = kl::fromUtf8((const char*)ptr);
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

    if (m_mode == SlIterator::modeOffsetLimit || m_mode == SlIterator::modeRowidRange)
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
}

double SlIterator::getDouble(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0.0;

    if (m_mode == SlIterator::modeOffsetLimit || m_mode == SlIterator::modeRowidRange)
    {
        // TODO:
        return 0.0;
    }

    return sqlite3_column_double(m_stmt, dai->col_ordinal);
}

int SlIterator::getInteger(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0;

    if (m_mode == SlIterator::modeOffsetLimit || m_mode == SlIterator::modeRowidRange)
    {
        // TODO:
        return 0;
    }

    return sqlite3_column_int(m_stmt, dai->col_ordinal);
}

bool SlIterator::getBoolean(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;

    if (m_eof || isNull(data_handle))
        return 0;

    if (m_mode == SlIterator::modeOffsetLimit || m_mode == SlIterator::modeRowidRange)
    {
        // TODO:
        return false;
    }
        
    return (sqlite3_column_int(m_stmt, dai->col_ordinal) == 0) ? false : true;
}

bool SlIterator::isNull(xd::objhandle_t data_handle)
{
    SlDataAccessInfo* dai = (SlDataAccessInfo*)data_handle;
    if (!dai)
        return true;

    if (m_mode == SlIterator::modeOffsetLimit || m_mode == SlIterator::modeRowidRange)
    {
        LocalRowValue& val = m_cache_row.getColumnData(dai->col_ordinal);
        return val.isNull();
    }

    if (sqlite3_column_type(m_stmt, dai->col_ordinal) == SQLITE_NULL)
        return true;

    return false;
}