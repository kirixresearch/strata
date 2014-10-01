/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#ifndef __XDSQLITE_ITERATOR_H
#define __XDSQLITE_ITERATOR_H


#include "../xdcommon/cmnbaseiterator.h"


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

    std::string result_str;
    std::wstring result_wstr;
};


class SlIterator : public CommonBaseIterator
{
friend class SlDatabase;
friend class SlSet;

    XCM_CLASS_NAME("xdsqlite.Iterator")
    XCM_BEGIN_INTERFACE_MAP(SlIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
    XCM_END_INTERFACE_MAP()

    xd::IDatabase* cmniterGetDatabase() { return static_cast<xd::IDatabase*>(m_database); }

private:
    void clearFieldData();

public:

    SlIterator(SlDatabase* database);
    ~SlIterator();

    bool init(const std::wstring& query);

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

    xd::IStructurePtr getStructure();
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

    SlDatabase* m_database;
    xd::IStructurePtr m_structure;
    xd::IStructurePtr m_table_structure;

    std::vector<SlDataAccessInfo> m_columns;

    xd::tableord_t m_ordinal;
    std::wstring m_tablename;
    std::wstring m_path;

    long long m_oid;
    bool m_eof;
    sqlite3* m_sqlite;
    sqlite3_stmt* m_stmt;
};

#endif
