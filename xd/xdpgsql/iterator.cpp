/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#include "libpq-fe.h"
#include <xd/xd.h>
#include "database.h"
#include "iterator.h"
#include "../xdcommon/xdcommon.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "../xdcommon/localrowcache.h"
#include <kl/portable.h>
#include <kl/string.h>
#include <kl/utf8.h>
#include <kl/md5.h>

const int row_array_size = 1000;

const std::string empty_string = "";
const std::wstring empty_wstring = L"";


PgsqlIterator::PgsqlIterator(PgsqlDatabase* database)
{
    m_conn = NULL;
    m_res = NULL;
    m_pager_res = NULL;

    m_database = database;
    m_database->ref();

    m_row_pos = 0;
    m_block_row = 0;
    m_eof = false;
    
    m_mode = modeResult;
    
    m_cache_active = false;
    m_cache_dbrowpos = 0;

    m_row_count = (xd::rowpos_t)-1;
}

PgsqlIterator::~PgsqlIterator()
{
    // free up each structure containing relation info

    std::vector<PgsqlIteratorRelInfo>::iterator rit;
    for (rit = m_relations.begin(); rit != m_relations.end(); ++rit)
    {
        std::vector<PgsqlIteratorRelField>::iterator fit;

        // free up each field part making up a relationship
        for (fit = rit->fields.begin(); fit != rit->fields.end(); ++fit)
            releaseHandle(fit->left_handle);
    }


    // clean up field vector and expression vector

    std::vector<PgsqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
        delete (*it);

    for (it = m_exprs.begin(); it != m_exprs.end(); ++it)
        delete (*it);

    
    if (m_res)
        PQclear(m_res);

    if (m_pager_res)
        PQclear(m_pager_res);

    if (m_conn)
    {
        if (m_mode == modeCursor)
            PQexec(m_conn, "END");
        m_database->closeConnection(m_conn);
    }

    if (m_database)
        m_database->unref();
}



bool PgsqlIterator::init(PGconn* conn, const xd::QueryParams& qp, const xd::FormatDefinition* fd)
{
    PGresult* res;

    m_conn = conn;
    m_table = pgsqlGetTablenameFromPath(qp.from);

    // create a sql query

    std::wstring query = L"SELECT * FROM ";
    query += pgsqlQuoteIdentifierIfNecessary(m_table);

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

    // first, get a rough feel for how many rows are in the result
    std::wstring command = L"explain " + query;
    const char* info = NULL;
    res = PQexec(conn, kl::toUtf8(command));
    xd::rowpos_t rowcnt = (xd::rowpos_t)-1;

    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
    {
        info = PQgetvalue(res, 0, 0);
        const char* rows = strstr(info, "rows=");
        if (rows)
            rowcnt = atoi(rows+5);
    }
    PQclear(res);



    // check if the results will be less than 50000 rows; if so, then
    // pull the entire result set down to a PGresult

    if (rowcnt != (xd::rowpos_t)-1 && rowcnt <= 50000)
    {
        // there is a manageable amount of rows -- just run the query
        m_mode = modeResult;

        PGresult* res = PQexec(conn, kl::toUtf8(query));
        if (!res)
            return false;

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            PQclear(res);
            return false;
        }

        m_row_count = PQntuples(res);

        return init(conn, res, fd);
    }



    // find the primary key for this table, if any
    {
        std::wstring prikey;

        command = L"SELECT pg_attribute.attname, format_type(pg_attribute.atttypid, pg_attribute.atttypmod) FROM pg_index, pg_class, pg_attribute WHERE "
                  L"pg_class.oid = '" + m_table + L"'::regclass AND indrelid = pg_class.oid AND pg_attribute.attrelid = pg_class.oid AND pg_attribute.attnum = any(pg_index.indkey) AND indisprimary";
        res = PQexec(conn, kl::toUtf8(command));
        if (!res)
            return false;

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            PQclear(res);
            return false;
        }

        if (PQntuples(res) == 1)
            m_primary_key = kl::towstring(PQgetvalue(res, 0, 0));
             else
            m_primary_key = L"ctid";

        PQclear(res);
    }





    // just using start/limit with primary key works pretty well on tables <= 400,000 records
    if (rowcnt != (xd::rowpos_t)-1 && rowcnt <= 400000 && qp.where.length() == 0 && qp.order.length() == 0)
    {
        if (rowcnt != (xd::rowpos_t)-1 && rowcnt <= 3000000)
        {
            command = L"SELECT count(*) from " + m_table;
            res = PQexec(conn, kl::toUtf8(command));
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) < 1)
            {
                PQclear(res);
                return false;
            }

            m_row_count = atoi(PQgetvalue(res,0,0));
        }

        m_mode = modeOffsetLimit;

        m_block_start = 1;
        m_block_row = 0;
        m_block_rowcount = 0;
        goFirst();

        return init(conn, m_res, fd);
    }
     else if (rowcnt != (xd::rowpos_t)-1 && rowcnt <= 40000000 && m_primary_key != L"ctid" && qp.where.length() == 0 && qp.order.length() == 0)
    {
        m_mode = modePagingTable;

        m_pager_step = 5000;

        std::wstring pagertbl = xd::getTemporaryPath();
        
        command = L"SELECT key from (SELECT row_number() OVER (ORDER BY " + m_primary_key + L") AS rownum, " +
                    m_primary_key + L" AS key FROM " + m_table + L") a WHERE ((rownum-1) % " + kl::itowstring(m_pager_step) + L") = 0";

        m_pager_res = PQexec(conn, kl::toUtf8(command));

        // grab the number of rows

        int ntuples = PQntuples(m_pager_res);
        if (ntuples == 0)
        {
            PQclear(res);
            PQclear(m_pager_res);
            m_pager_res = NULL;

            return false;
        }

        m_row_count = ((xd::rowpos_t)(ntuples-1) * m_pager_step);

        command = L"SELECT COUNT(*) FROM " + m_table + L" WHERE " + m_primary_key + L" >= " + kl::towstring(PQgetvalue(m_pager_res, ntuples-1, 0));
        PGresult* res = PQexec(conn, kl::toUtf8(command));
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            PQclear(res);
            PQclear(m_pager_res);
            m_pager_res = NULL;
            return false;
        }

        m_row_count += atoi(PQgetvalue(res, 0, 0));
        PQclear(res);

        m_block_start = 1;
        m_block_row = 0;
        m_block_rowcount = 0;
        goFirst();

        return init(conn, m_res, fd);
    }
     else
    {
        m_mode = modeRowidPager;

        m_pager_step = 1;

        std::wstring pagertbl = xd::getTemporaryPath();
        std::wstring sql;
        
        sql = L"SELECT ctid from " + m_table;

        if (qp.where.length() > 0)
        {
            sql += L" WHERE ";
            sql += qp.where;
        }

        if (qp.order.length() > 0)
        {
            sql += L" ORDER BY ";
            sql += qp.order;
        }

        m_pager_res = PQexec(conn, kl::toUtf8(sql));

        // grab the number of rows
        m_row_count = PQntuples(m_pager_res);

        m_block_start = 1;
        m_block_row = 0;
        m_block_rowcount = 0;
        goFirst();

        return init(conn, m_res, fd);
    }

/*
        m_mode = modeSequenceTable;

        std::wstring pagertbl = xd::getTemporaryPath();
        std::wstring command = L"CREATE TABLE " + pagertbl + L" AS SELECT (row_number() OVER (ORDER BY " + m_primary_key + L"))-1 AS xdpgsql_rownum, " + m_primary_key + L" AS xdpgsql_key FROM " + m_table;

        res = PQexec(conn, kl::toUtf8(command));
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            PQclear(res);
            return false;
        }

        // grab the number of rows

        const char* rows_affected = PQcmdTuples(res);

        if (strlen(rows_affected) > 0)
            m_row_count = atoi(rows_affected);
        PQclear(res);


        res = PQexec(conn, kl::toUtf8(L"ALTER TABLE " + pagertbl + L" ADD PRIMARY KEY (xdpgsql_rownum)"));
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            PQclear(res);
            return false;
        }
        PQclear(res);



        m_pager = pagertbl;

        m_block_start = 1;
        m_block_row = 0;
        m_block_rowcount = 0;
        goFirst();

        return init(conn, m_res, fd);
    */



    /*
        m_mode = modePagingTable;

        std::wstring pagertbl = xd::getTemporaryPath();
        std::wstring command = L"CREATE TABLE " + pagertbl + L" AS SELECT xdpgsql_rownum-1 as xdpgsql_rownum, xdpgsql_key from (SELECT row_number() OVER (ORDER BY " + m_primary_key + L") AS xdpgsql_rownum, " +
                               m_primary_key + L" AS xdpgsql_key FROM " + m_table + L") a WHERE ((xdpgsql_rownum-1) % 10000) = 0";

        res = PQexec(conn, kl::toUtf8(command));
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            PQclear(res);
            return false;
        }

        // grab the number of rows


        const char* rows_affected = PQcmdTuples(res);

        if (strlen(rows_affected) > 0)
        {
            unsigned int rows = (atoi(rows_affected)-1) * 10000;
            PQclear(res);

            command = L"SELECT COUNT(*) FROM " + m_table + L" WHERE " + m_primary_key + L" >= (SELECT MAX(xdpgsql_key) FROM " + pagertbl + L")";
            res = PQexec(conn, kl::toUtf8(command));
            if (PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                PQclear(res);
                return false;
            }

            rows += atoi(PQgetvalue(res, 0, 0));

            m_row_count = rows;
        }
         else
        {
            PQclear(res);
        }

        
        res = PQexec(conn, kl::toUtf8(L"ALTER TABLE " + pagertbl + L" ADD PRIMARY KEY (xdpgsql_rownum)"));
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            PQclear(res);
            return false;
        }

        m_pager = pagertbl;

        m_block_start = 1;
        m_block_row = 0;
        m_block_rowcount = 0;
        goFirst();

        return init(conn, m_res, fd);
        */


}

bool PgsqlIterator::init(PGconn* conn, const std::wstring& query, const xd::FormatDefinition* fd)
{
    bool use_server_side_cursor = true;

    if (use_server_side_cursor)
    {
        PGresult* res;
        int status;

        // first, get a rough feel for how many rows are in the result
        std::wstring command = L"explain " + query;
        const char* info = NULL;
        res = PQexec(conn, kl::toUtf8(command));
        xd::rowpos_t rowcnt = (xd::rowpos_t)-1;

        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        {
            info = PQgetvalue(res, 0, 0);
            const char* rows = strstr(info, "rows=");
            if (rows)
                rowcnt = atoi(rows+5);
        }
        PQclear(res);


        if (rowcnt != (xd::rowpos_t)-1 && rowcnt < 20000)
        {
            m_mode = modeResult;

            PGresult* res = PQexec(conn, kl::toUtf8(query));
            if (!res)
                return false;

            if (PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                PQclear(res);
                m_database->closeConnection(conn);
                return false;
            }

            // don't need the connection anymore, since we have the entire rowset
            m_database->closeConnection(conn);
            conn = NULL;
            m_row_count = PQntuples(res);

            return init(conn, res, fd);
        }





        res = PQexec(conn, "BEGIN");
        status = PQresultStatus(res);
        PQclear(res);

        if (status != PGRES_COMMAND_OK)
        {
            m_database->closeConnection(conn);
            return false;
        }

        res = PQexec(conn, kl::toUtf8(L"DECLARE xdpgsqlcursor SCROLL CURSOR FOR " + query));
        status = PQresultStatus(res);
        PQclear(res);

        if (status != PGRES_COMMAND_OK)
        {
            use_server_side_cursor = false;

            PQexec(conn, "END");
            m_database->closeConnection(conn);
            return false;
        }


        if (rowcnt != (xd::rowpos_t)-1)
        {
            if (rowcnt <= 2000000)
            {
                // for tables under 2,000,000 rows, use the approximation
                m_row_count = rowcnt;
            }

            if (rowcnt < 200000)
            {
                // if there are under 200,000 rows approximately, find out the precise number
                // by scrolling to the end of the cursor

                res = PQexec(conn, "MOVE 210000 in xdpgsqlcursor");
                info = PQcmdStatus(res);
                if (PQresultStatus(res) == PGRES_COMMAND_OK && info)
                    m_row_count = atoi(info+5);
                PQclear(res);
                res = PQexec(conn, "MOVE ABSOLUTE 0 in xdpgsqlcursor");
            }
        }
        

        if (use_server_side_cursor)
        {
            res = PQexec(conn, "FETCH 1 from xdpgsqlcursor");
            m_block_start = 1;
            m_block_row = 0;
            m_block_rowcount = PQntuples(res);
            m_mode = modeCursor;
        }

        return init(conn, res, fd);
    } 
     else
    {
        m_mode = modeResult;

        PGresult* res = PQexec(conn, kl::toUtf8(query));
        if (!res)
            return false;

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            PQclear(res);
            m_database->closeConnection(conn);
            return false;
        }

        if (!init(conn, res, fd))
            return false;
    }

    return true;
}


bool PgsqlIterator::init(PGconn* conn, PGresult* res, const xd::FormatDefinition* fd)
{
    m_conn = conn;
    m_res = res;

    std::wstring col_name;
    col_name.reserve(80);
    int col_pg_type;
    int col_xd_type;
    int col_width = 20;
    int col_scale = 0;
    int fmod;
    bool col_nullable = true;

    int i;
    int col_count = PQnfields(m_res);

    for (i = 0; i < col_count; ++i)
    {
        col_name = kl::towstring(PQfname(m_res, i));
        col_pg_type = PQftype(m_res, i);
        col_xd_type = pgsqlToXdType(col_pg_type);
        col_width = 255;
        fmod = PQfmod(m_res, i);

        if (col_xd_type == xd::typeNumeric || col_xd_type == xd::typeDouble)
        {
            if (fmod == -1)
            {
                col_width = 12;
                col_scale = 4;
            }
             else
            {
                fmod -= 4;
                col_width = (fmod >> 16);
                col_scale = (fmod & 0xffff);
            }
        }
         else if (col_xd_type == xd::typeCharacter || col_xd_type == xd::typeWideCharacter)
        {
            col_width = fmod - 4;
            col_scale = 0;
        }


        PgsqlDataAccessInfo* field = new PgsqlDataAccessInfo;
        field->name = col_name;
        field->pg_type = col_pg_type;
        field->type = col_xd_type;
        field->width = col_width;
        field->scale = col_scale;
        field->ordinal = i;

        m_fields.push_back(field);
    }


    if (fd && fd->columns.size() > 0)
    {
        std::vector<xd::ColumnInfo>::const_iterator it;

        for (it = fd->columns.cbegin(); it != fd->columns.cend(); ++it)
        {
            PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
            dai->name = it->name;
            dai->type = it->type;
            dai->width = it->width;
            dai->scale = it->scale;
            dai->ordinal = m_fields.size();
            dai->expr_text = it->expression;
            dai->expr = parse(it->expression);
                
            m_fields.push_back(dai);

            // parse() uses getStructure(), and m_structure needs to be
            // cleared out so that one calcfield can reference another calcfield

            m_structure.clear(); 
        }

        m_structure.clear();
    }
    


    // close the connection unless we need it (server-side cursor)
    if (m_conn && m_mode != modeCursor)
    {
        m_database->closeConnection(m_conn);
        m_conn = NULL;
    }

    return true;
}

void PgsqlIterator::setTable(const std::wstring& table)
{
    m_path = table;
}


std::wstring PgsqlIterator::getTable()
{
    // m_path is usually empty, but if the iterator specifically represents a
    // concrete table object, we can express that here
    return m_path;
}

void PgsqlIterator::setIteratorFlags(unsigned int mask, unsigned int value)
{
    m_cache_active = ((mask & value & xd::ifReverseRowCache) != 0) ? true : false;
}
    
unsigned int PgsqlIterator::getIteratorFlags()
{
    if (m_row_count != (xd::rowpos_t)-1)
        return xd::ifFastRowCount;
    
    return 0;
}

xd::rowpos_t PgsqlIterator::getRowCount()
{
    if (m_row_count != (xd::rowpos_t)-1)
        return m_row_count;
    return 0;
}

xd::IDatabasePtr PgsqlIterator::getDatabase()
{
    return static_cast<xd::IDatabase*>(m_database);
}

xd::IIteratorPtr PgsqlIterator::clone()
{
    return xcm::null;
}


void PgsqlIterator::skip(int delta)
{
    m_row_pos += delta;

    if (m_row_pos >= m_block_start && m_row_pos < m_block_start + m_block_rowcount)
    {
        m_eof = false;
        m_block_row = (int)(m_row_pos - m_block_start);
        return;
    }

    if (m_mode == modeCursor)
    {
        m_eof = false;

        if (m_row_pos == m_block_start + m_block_rowcount)
        {
            // no need to reposition, just get the next block

            PQclear(m_res);
            m_res = PQexec(m_conn, "FETCH 100 from xdpgsqlcursor");
            m_block_start += m_block_rowcount;
            m_block_rowcount = PQntuples(m_res);
            m_block_row = 0;
            if (m_block_rowcount == 0)
                m_eof = true;
        }
         else
        {
            PQclear(m_res);
            m_res = NULL;

            // reposition
            char q[80];
            snprintf(q, 80, "MOVE ABSOLUTE %d from xdpgsqlcursor", (int)(m_row_pos-1));
            m_res = PQexec(m_conn, q);
            PQclear(m_res);

            m_res = PQexec(m_conn, "FETCH 100 from xdpgsqlcursor");
            m_block_start = m_row_pos;
            m_block_rowcount = PQntuples(m_res);
            m_block_row = 0;
            if (m_block_rowcount == 0)
                m_eof = true;
       }
    }
     else if (m_mode == modeResult)
    {
        m_block_row += delta;
        m_eof = (m_block_row >= PQntuples(m_res));
    }
     else if (m_mode == modeOffsetLimit)
    {
        PQclear(m_res);
        m_res = NULL;

        std::wstring sql = L"SELECT * from " + m_table + L" ORDER BY " + m_primary_key + L" OFFSET " + kl::itowstring(m_row_pos-1) + L" LIMIT 100";

        PGconn* conn = m_database->createConnection();
        m_res = PQexec(conn, kl::toUtf8(sql));
        m_database->closeConnection(conn);

        m_block_start = m_row_pos;
        m_block_rowcount = PQntuples(m_res);
        m_block_row = 0;
        if (m_block_rowcount == 0)
            m_eof = true;
    }
     else if (m_mode == modePagingTable)
    {
        PQclear(m_res);
        m_res = NULL;
        
        PGconn* conn = m_database->createConnection();

        int start_block = (m_row_pos-1) / m_pager_step;
        int start_offset = (m_row_pos-1) % m_pager_step;

        if (start_block < PQntuples(m_pager_res))
        {
            const char* key = PQgetvalue(m_pager_res, start_block, 0);

            std::wstring sql = L"SELECT * from " + m_table + L" WHERE " + m_primary_key + L" >= " + kl::towstring(key) + L" ORDER BY " + m_primary_key + L" OFFSET " + kl::itowstring(start_offset) + L" LIMIT 100";

            m_res = PQexec(conn, kl::toUtf8(sql));
            m_block_start = m_row_pos;
            m_block_rowcount = PQntuples(m_res);
            m_block_row = 0;
            if (m_block_rowcount == 0)
                m_eof = true;

        }
         else
        {
            m_block_start = m_row_pos;
            m_block_rowcount = 0;
            m_block_row = 0;
            m_eof = true;
        }

        m_database->closeConnection(conn);
    }
     else if (m_mode == modeRowidPager)
    {
        PQclear(m_res);
        m_res = NULL;

        int i;
        int row_start = m_row_pos-1;
        int row_end = row_start + 100;

//SELECT tbl.* from (VALUES ('(0,1)'::tid,0),('(0,2)'::tid,1),('(0,3)'::tid,2),('(0,4)'::tid,3),('(0,5)'::tid,4),('(0,6)'::tid,5)) AS rows (rowid, ordering) INNER JOIN (SELECT *, ctid AS xdpgsql_rowid FROM aa WHERE ctid IN ('(0,1)'::tid,'(0,2)'::tid,'(0,3)'::tid,'(0,4)'::tid,'(0,5)'::tid,'(0,6)'::tid)) AS tbl on rows.rowid=tbl.xdpgsql_rowid ORDER BY rows.ordering;
        
        std::wstring sql = L"SELECT tbl.* FROM (VALUES ";

        for (i = row_start; i < row_end; ++i)
        {
            const char* val = PQgetvalue(m_pager_res, i, 0);
            if (!val)
            {
                row_end = i;
                break;
            }

            if (i > row_start)
                sql += L",";
            sql += (L"('" + kl::towstring(val) + L"'::tid," + kl::itowstring(i-row_start) + L")");
        }

        sql += L")  AS rows (rowid, ordering) INNER JOIN (SELECT *, ctid AS xdpgsql_rowid FROM " + m_table + L" WHERE ctid IN (";
        for (i = row_start; i < row_end; ++i)
        {
            if (i > row_start)
                sql += L",";
            sql += L"'" + kl::towstring(PQgetvalue(m_pager_res, i, 0)) + L"'::tid";
        }

        sql += L")) AS tbl on rows.rowid=tbl.xdpgsql_rowid ORDER BY rows.ordering";

        PGconn* conn = m_database->createConnection();
        m_res = PQexec(conn, kl::toUtf8(sql));
        m_database->closeConnection(conn);

        m_block_start = m_row_pos;
        m_block_rowcount = PQntuples(m_res);
        m_block_row = 0;
        if (m_block_rowcount == 0)
            m_eof = true;

        /*
    (1,1),
    (3,2),
    (2,3),
    (4,4)
) as x (id, ordering) on c.id = x.id
order by x.ordering
        */

        /*
        if (start_block < PQntuples(m_pager_res))
        {
            const char* key = PQgetvalue(m_pager_res, start_block, 0);


            m_res = PQexec(conn, kl::toUtf8(sql));
            m_block_start = m_row_pos;
            m_block_rowcount = PQntuples(m_res);
            m_block_row = 0;
            if (m_block_rowcount == 0)
                m_eof = true;

        }
         else
        {
            m_block_start = m_row_pos;
            m_block_rowcount = 0;
            m_block_row = 0;
            m_eof = true;
        }
        */


    }

    /*
     else if (m_mode == modePagingTable)
    {
        PQclear(m_res);
        m_res = NULL;
        
        PGconn* conn = m_database->createConnection();


        int start_offset = (m_row_pos-1) % 10000;
        int start_block = ((m_row_pos-1) - start_offset);

        std::wstring sql = L"SELECT xdpgsql_rownum from " + m_pager + L" WHERE xdpgsql_rownum=" + kl::itowstring(start_block);
        PGresult* res = PQexec(conn, kl::toUtf8(sql));
        bool found = ((PQresultStatus(res) == PGRES_TUPLES_OK) && PQntuples(res) == 1) ? true : false;
        PQclear(res);

        if (found)
        {
            sql = L"SELECT * from " + m_table + L" WHERE " + m_primary_key + L" >= (SELECT xdpgsql_key FROM " + m_pager + L" WHERE xdpgsql_rownum=" + kl::itowstring(start_block) + 
                  L") ORDER BY " + m_primary_key + L" OFFSET " + kl::itowstring(start_offset) + L" LIMIT 100";

            m_res = PQexec(conn, kl::toUtf8(sql));
            m_block_start = m_row_pos;
            m_block_rowcount = PQntuples(m_res);
            m_block_row = 0;
            if (m_block_rowcount == 0)
                m_eof = true;
        }
         else
        {
            m_block_start = m_row_pos;
            m_block_rowcount = 0;
            m_block_row = 0;
            m_eof = true;
        }



        m_database->closeConnection(conn);
    }
    */
     else if (m_mode = modeSequenceTable)
    {
        PQclear(m_res);

        int start_row = m_row_pos-1;
        int end_row = m_row_pos + 100;

        //std::wstring sql = L"SELECT t.* from " + m_table + L" t INNER JOIN " + m_pager + L" pager ON pager.xdpgsql_key = t." + m_primary_key +
        //                 L" WHERE pager.xdpgsql_rownum >= " + kl::itowstring(start_row) + L" AND pager.xdpgsql_rownum < " + kl::itowstring(end_row);
        //                 L" ORDER BY pager.xdpgsql_rownum";

        std::wstring sql = L"SELECT t.* from " + m_table + L" t WHERE " + m_primary_key + L" IN (SELECT xdpgsql_key FROM " + m_pager + 
                           L" WHERE xdpgsql_rownum >= " + kl::itowstring(start_row) + L" AND xdpgsql_rownum < " + kl::itowstring(end_row) + L") ORDER BY " + m_primary_key;


        PGconn* conn = m_database->createConnection();
        m_res = PQexec(conn, kl::toUtf8(sql));
        m_database->closeConnection(conn);

        m_block_start = m_row_pos;
        m_block_rowcount = PQntuples(m_res);
        m_block_row = 0;
        if (m_block_rowcount == 0)
            m_eof = true;
    }
}

void PgsqlIterator::goFirst()
{
    if (m_block_start == 1 && m_block_rowcount > 0)
    {
        m_row_pos = 1;
        m_block_row = 0;
        m_eof = false;
        return;
    }

    if (m_mode == modeCursor)
    {
        PQclear(m_res);

        m_res = PQexec(m_conn, "MOVE ABSOLUTE 0 from xdpgsqlcursor");
        PQclear(m_res);

        m_res = PQexec(m_conn, "FETCH 100 from xdpgsqlcursor");
        m_row_pos = 1;
        m_block_start = 1;
        m_block_rowcount = PQntuples(m_res);
        m_block_row = 0;
        m_eof = (m_block_row >= PQntuples(m_res));
    }
     else if (m_mode == modeResult)
    {
        m_row_pos = 1;
        m_block_row = 0;
        m_eof = (m_block_row >= PQntuples(m_res));
    }
     else if (m_mode == modeOffsetLimit)
    {
        std::wstring sql = L"SELECT * from " + m_table + L" ORDER BY " + m_primary_key + L" OFFSET 0 LIMIT 100";
        PGconn* conn = m_database->createConnection();
        m_res = PQexec(conn, kl::toUtf8(sql));
        m_database->closeConnection(conn);

        m_row_pos = 1;
        m_block_row = 0;
        m_eof = (m_block_row >= PQntuples(m_res));
    }
     else
    {
        skip(1 - m_row_pos);
    }

    /*
     else if (m_mode = modePagingTable)
    {
        PQclear(m_res);

        int start_row = m_row_pos;
        int end_row = m_row_pos + 100;

        std::wstring sql = L"SELECT t.* from " + m_table + L" t INNER JOIN " + m_pager + L" pager ON pager.xdpgsql_key = t." + m_primary_key +
                           L" WHERE pager.xdpgsql_rownum >= 1 AND pager.xdpgsql_rownum < 100 "
                           L" ORDER BY pager.xdpgsql_rownum";

        PGconn* conn = m_database->createConnection();
        m_res = PQexec(conn, kl::toUtf8(sql));
        m_database->closeConnection(conn);

        m_row_pos = 1;
        m_block_row = 0;
        m_eof = (m_block_row >= PQntuples(m_res));
    }
    */
}

void PgsqlIterator::goLast()
{
}

xd::rowid_t PgsqlIterator::getRowId()
{
    return m_row_pos;
}

bool PgsqlIterator::bof()
{
    return false;
}

bool PgsqlIterator::eof()
{
    return m_eof;
}

bool PgsqlIterator::seek(const unsigned char* key, int length, bool soft)
{
    return false;
}

bool PgsqlIterator::seekValues(const wchar_t* arr[], size_t arr_size, bool soft)
{
    return false;
}

bool PgsqlIterator::setPos(double pct)
{
    return false;
}

void PgsqlIterator::goRow(const xd::rowid_t& rowid)
{
}

double PgsqlIterator::getPos()
{
    return (double)(long long)m_row_pos;
}

xd::Structure PgsqlIterator::getStructure()
{
    if (m_structure.isOk())
        return m_structure;

    std::vector<PgsqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        // skip internal field names
        if ((*it)->name.substr(0, 8) == L"xdpgsql_")
            continue;

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
            xd::ColumnInfo col = pgsqlCreateColInfo((*it)->name,
                                                    (*it)->pg_type,
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

bool PgsqlIterator::refreshStructure()
{
    xd::Structure set_structure = m_database->describeTable(getTable());
    if (set_structure.isNull())
        return false;

    // find changed/deleted calc fields
    size_t i, col_count;
    for (i = 0; i < m_fields.size(); ++i)
    {
        if (!m_fields[i]->isCalculated())
            continue;
            
        delete m_fields[i]->expr;
        m_fields[i]->expr = NULL;

        const xd::ColumnInfo& col = set_structure.getColumnInfo(m_fields[i]->name);
        if (col.isNull())
        {
            m_fields.erase(m_fields.begin() + i);
            i--;
            continue;
        }
  
        m_fields[i]->type = col.type;
        m_fields[i]->width = col.width;
        m_fields[i]->scale = col.scale;
        m_fields[i]->expr_text = col.expression;
    }
    
    // find new calc fields
    
    col_count = set_structure.getColumnCount();
    
    std::vector<PgsqlDataAccessInfo*>::iterator it;
    for (i = 0; i < col_count; ++i)
    {
        const xd::ColumnInfo& col = set_structure.getColumnInfoByIdx(i);
        if (!col.calculated)
            continue;
            
        bool found = false;
        
        for (it = m_fields.begin(); it != m_fields.end(); ++it)
        {
            if (!(*it)->isCalculated())
                continue;

            if (kl::iequals((*it)->name, col.name))
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            // add new calc field
            PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
            dai->name = col.name;
            dai->type = col.type;
            dai->width = col.width;
            dai->scale = col.scale;
            dai->ordinal = m_fields.size();
            dai->expr_text = col.expression;
            dai->expr = NULL;
            
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

    return true;
}

bool PgsqlIterator::modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job)
{
    std::vector<xd::StructureModify::Action>::const_iterator it;
    std::vector<PgsqlDataAccessInfo*>::iterator it2;
    
    // handle delete
    for (it = mod_params.actions.cbegin(); it != mod_params.actions.cend(); ++it)
    {
        if (it->action != xd::StructureModify::Action::actionDelete)
            continue;

        for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
        {
            if (kl::iequals(it->column, (*it2)->name))
            {
                PgsqlDataAccessInfo* dai = *(it2);
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

        for (it2 = m_fields.begin(); it2 != m_fields.end(); ++it2)
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
            PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
            dai->name = it->params.name;
            dai->type = it->params.type;
            dai->width = it->params.width;
            dai->scale = it->params.scale;
            dai->ordinal = m_fields.size();
            dai->expr_text = it->params.expression;
            dai->expr = parse(it->params.expression);
                
            m_fields.push_back(dai);
        }
    }
    
    // the next call to getStructure() will refresh m_structure
    m_structure.clear();
    
    return true;
}



xd::objhandle_t PgsqlIterator::getHandle(const std::wstring& expr)
{
    std::vector<PgsqlDataAccessInfo*>::iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it)
    {
        if (kl::iequals((*it)->name, expr))
            return (xd::objhandle_t)(*it);
    }

    // test for binary keys
    if (0 == wcsncasecmp(expr.c_str(), L"KEY:", 4))
    {
        PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
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

    PgsqlDataAccessInfo* dai = new PgsqlDataAccessInfo;
    dai->expr = parser;
    dai->expr_text = expr;
    dai->type = kscript2xdType(parser->getType());
    m_exprs.push_back(dai);

    return (xd::objhandle_t)dai;
}

bool PgsqlIterator::releaseHandle(xd::objhandle_t data_handle)
{
    std::vector<PgsqlDataAccessInfo*>::iterator it;
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

xd::ColumnInfo PgsqlIterator::getInfo(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
    {
        return xd::ColumnInfo();
    }

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

    return pgsqlCreateColInfo(dai->name,
                              dai->type,
                              dai->width,
                              dai->scale,
                              dai->expr_text,
                              -1);
}

int PgsqlIterator::getType(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
        return xd::typeInvalid;
    
    return dai->type;
}

int PgsqlIterator::getRawWidth(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai && dai->key_layout)
    {
        return dai->key_layout->getKeyLength();
    }
    
    return 0;
}

const unsigned char* PgsqlIterator::getRawPtr(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
        return NULL;
    
    if (dai->key_layout)
    {
        return dai->key_layout->getKey();
    }

    return NULL;
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

const std::string& PgsqlIterator::getString(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
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

    if (PQgetisnull(m_res, m_block_row, dai->ordinal))
        return empty_string;

    dai->wstr_result = kl::fromUtf8((PQgetvalue(m_res, m_block_row, dai->ordinal)));
    dai->str_result = kl::tostring(dai->wstr_result);
    return dai->str_result;
}

const std::wstring& PgsqlIterator::getWideString(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
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

    if (PQgetisnull(m_res, m_block_row, dai->ordinal))
        return empty_wstring;

    dai->wstr_result = kl::fromUtf8((PQgetvalue(m_res, m_block_row, dai->ordinal)));
    return dai->wstr_result;
}

xd::datetime_t PgsqlIterator::getDateTime(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
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

    if (PQgetisnull(m_res, m_block_row, dai->ordinal))
        return 0;

    const char* c = PQgetvalue(m_res, m_block_row, dai->ordinal);
    char buf[8];
    int y, m, d, h, mm, s;

    if (dai->type == xd::typeDate)
    {
        memcpy(buf, c, 4);
        buf[4] = 0;
        y = atoi(buf);

        memcpy(buf, c+5, 2);
        buf[2] = 0;
        m = atoi(buf);

        memcpy(buf, c+8, 2);
        buf[2] = 0;
        d = atoi(buf);

        if (y == 0)
            return 0;

        xd::DateTime dt(y, m, d);
        return dt;
    }
     else
    {
        memcpy(buf, c, 4);
        buf[4] = 0;
        y = atoi(buf);

        memcpy(buf, c+5, 2);
        buf[2] = 0;
        m = atoi(buf);

        memcpy(buf, c+8, 2);
        buf[2] = 0;
        d = atoi(buf);

        memcpy(buf, c+11, 2);
        buf[2] = 0;
        h = atoi(buf);

        memcpy(buf, c+14, 2);
        buf[2] = 0;
        mm = atoi(buf);

        memcpy(buf, c+17, 2);
        buf[2] = 0;
        s = atoi(buf);

        if (y == 0)
            return 0;

        xd::DateTime dt(y, m, d, h, mm, s, 0);
        return dt;
    }
}

double PgsqlIterator::getDouble(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
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

    if (PQgetisnull(m_res, m_block_row, dai->ordinal))
        return 0.0;

    const char* c = PQgetvalue(m_res, m_block_row, dai->ordinal);

    if (dai->type == xd::typeNumeric ||
        dai->type == xd::typeDouble)
    {
        return kl::nolocale_atof(c);
    }
     else if (dai->type == xd::typeInteger)
    {
        return atoi(c);
    }
     else
    {
        return 0.0;
    }
}

int PgsqlIterator::getInteger(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
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

    if (PQgetisnull(m_res, m_block_row, dai->ordinal))
        return 0;

    const char* c = PQgetvalue(m_res, m_block_row, dai->ordinal);

    if (dai->type == xd::typeNumeric ||
        dai->type == xd::typeDouble)
    {
        return (int)kl::nolocale_atof(c);
    }
     else if (dai->type == xd::typeInteger)
    {
        return atoi(c);
    }
     else
    {
        return 0;
    }
}

bool PgsqlIterator::getBoolean(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
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

    const char* c = PQgetvalue(m_res, m_block_row, dai->ordinal);

    return false;
    //return dai->bool_val ? true : false;
}

bool PgsqlIterator::isNull(xd::objhandle_t data_handle)
{
    PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)data_handle;
    if (dai == NULL)
        return true;

    if (dai->expr)
        return false;

    if (dai->isCalculated())
    {
        // calculated field with bad expr
        return true;
    }

    return PQgetisnull(m_res, m_block_row, dai->ordinal) ? true : false;
}



xd::IIteratorPtr PgsqlIterator::getChildIterator(xd::IRelationPtr relation)
{
    return xcm::null;
}

xd::IIteratorPtr PgsqlIterator::getFilteredChildIterator(xd::IRelationPtr relation)
{
    return xcm::null;
}


// xd::ICacheRowUpdate::updateCacheRow()

bool PgsqlIterator::updateCacheRow(xd::rowid_t rowid,
                                   xd::ColumnUpdateInfo* info,
                                   size_t info_size)
{
/*
    saveRowToCache();
    
    size_t i;
    for (i = 0; i < info_size; ++i, ++info)
    {
        PgsqlDataAccessInfo* dai = (PgsqlDataAccessInfo*)info->handle;
        int column = dai->ordinal - 1;
        
        if (info->null)
            m_cache.updateValue(m_row_pos, column, NULL, 0);
        

        switch (dai->type)
        {
            case xd::typeCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->str_val.c_str(),
                                    info->str_val.length());
                break;

            case xd::typeWideCharacter:
                m_cache.updateValue(m_row_pos,
                                    column,
                                    (unsigned char*)info->wstr_val.c_str(),
                                    info->wstr_val.length()*sizeof(wchar_t));
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

    */
    return true;
}

