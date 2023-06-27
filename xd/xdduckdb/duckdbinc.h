/*!
 *
 * Copyright (c) 2022, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#ifndef H_XDDUCKDB_DUCKDBINC_H
#define H_XDDUCKDB_DUCKDBINC_H

#define DUCKDB_API
#pragma warning(push)
#pragma warning(disable:4244)
#include <duckdb.hpp>
#pragma warning(pop)




class DuckResult
{
public:

    DuckResult()
    {
        m_res = nullptr;
        m_chunk_pos = 0;
        m_chunk_size = 0;
    }

    DuckResult(duckdb::QueryResult* res)
    {
        setQueryResult(res);
    }

    DuckResult(std::unique_ptr<duckdb::QueryResult>& res)
    {
        setQueryResult(res);
    }

    DuckResult(std::unique_ptr<duckdb::MaterializedQueryResult>& res)
    {
        setQueryResult(res);
    }

    void setQueryResult(duckdb::QueryResult* res)
    {
        m_res = res->HasError() ? nullptr : res;
        m_chunk_pos = 0;
        m_chunk_size = 0;
    }

    void setQueryResult(const std::unique_ptr<duckdb::QueryResult>& res)
    {
        setQueryResult(res.get());
    }

    void setQueryResult(const std::unique_ptr<duckdb::MaterializedQueryResult>& res)
    {
        setQueryResult(res.get());
    }


    bool next()
    {
        if (!m_res)
        {
            return false;
        }

        m_chunk_pos++;

        if (m_chunk_pos >= m_chunk_size)
        {
            m_chunk = m_res->Fetch();
            m_chunk_pos = 0;

            if (m_chunk)
            {
                m_chunk_size = m_chunk->size();
            }
            else
            {
                m_chunk = nullptr;
                m_chunk_size = 0;
            }
        }

        return (m_chunk_pos >= m_chunk_size) ? false : true;
    }

    duckdb::Value GetValue(size_t col_idx)
    {
        return m_chunk->data[col_idx].GetValue(m_chunk_pos);
    }

    size_t GetColumnCount()
    {
        return (size_t)m_res->ColumnCount();
    }

    const std::string& GetColumnName(size_t col_idx)
    {
        return m_res->ColumnName(col_idx);
    }

    duckdb::LogicalType& GetColumnType(size_t col_idx)
    {
        return m_res->types[col_idx];
    }

private:

    duckdb::QueryResult* m_res;
    std::unique_ptr<duckdb::DataChunk> m_chunk;
    duckdb::idx_t m_chunk_pos;
    duckdb::idx_t m_chunk_size;
};




#endif
