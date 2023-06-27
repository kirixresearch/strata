/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#ifndef H_XDDUCKDB_ITERATOR_H
#define H_XDDUCKDB_ITERATOR_H

#include "duckdbinc.h"

#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/localrowcache2.h"


struct SlDataAccessInfo
{
    std::wstring name;
    int sqlite_type;
    int xd_type;
    int width;
    int scale;
    int col_ordinal;
    char* buf;
    int datalen;
    bool calculated;

    std::string result_str;
    std::wstring result_wstr;
};


class DuckdbIterator : public CommonBaseIterator
{
friend class DuckdbDatabase;
friend class SlSet;

    XCM_CLASS_NAME("xdsqlite.Iterator")
    XCM_BEGIN_INTERFACE_MAP(DuckdbIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
    XCM_END_INTERFACE_MAP()

    xd::IDatabase* cmniterGetDatabase() { return static_cast<xd::IDatabase*>(m_database); }

    enum
    {
        modeSqliteResult = 0,
        modeOffsetLimit = 1,
        modeRowidRange = 2,
        modeRowidTable = 3
    };

private:
    void clearFieldData();

public:

    DuckdbIterator(DuckdbDatabase* database);
    ~DuckdbIterator();

    bool init(const std::wstring& query);
    bool init(const xd::QueryParams& qp);

    // xd::IIterator

    void setTable(const std::wstring& tbl);
    std::wstring getTable();

    xd::rowpos_t getRowCount();
    xd::IDatabasePtr getDatabase();
    xd::IIteratorPtr clone();

    unsigned int getIteratorFlags();

    void skip(int delta);
    void goFirst();
    void goLast();
    xd::rowid_t getRowId();
    bool bof();
    bool eof();
    bool seek(const unsigned char* key, int length, bool soft);
    bool seekValues(const wchar_t* arr[], size_t arr_size, bool soft);
    bool setPos(double pct);
    double getPos();
    void goRow(const xd::rowid_t& rowid);
    void loadRow();

    xd::Structure getStructure();
    bool refreshStructure();
    bool modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job);

    xd::objhandle_t getHandle(const std::wstring& expr);
    xd::ColumnInfo getInfo(xd::objhandle_t data_handle);
    int getType(xd::objhandle_t data_handle);
    bool releaseHandle(xd::objhandle_t data_handle);

    const unsigned char* getRawPtr(xd::objhandle_t data_handle);
    int getRawWidth(xd::objhandle_t data_handle);
    const std::string& getString(xd::objhandle_t data_handle);
    const std::wstring& getWideString(xd::objhandle_t data_handle);
    xd::datetime_t getDateTime(xd::objhandle_t data_handle);
    double getDouble(xd::objhandle_t data_handle);
    int getInteger(xd::objhandle_t data_handle);
    bool getBoolean(xd::objhandle_t data_handle);
    bool isNull(xd::objhandle_t data_handle);

private:

    void initColumns(duckdb_connection conn);

private:

    DuckdbDatabase* m_database;
    xd::Structure m_structure;
    xd::Structure m_table_structure;

    std::vector<SlDataAccessInfo> m_columns;

    xd::tableord_t m_ordinal;
    std::wstring m_tablename;
    std::wstring m_path;

    long long m_row_count;
    long long m_oid;
    long long m_position;
    bool m_eof;
    duckdb::Connection* m_conn;

    int m_mode;
    LocalRowCache2 m_cache;
    LocalRow2 m_cache_row;
    xd::QueryParams m_qp;

    std::unique_ptr<duckdb::QueryResult> m_query_result;
    DuckResult m_result;
};

#endif
