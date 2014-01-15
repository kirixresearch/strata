/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-04-16
 *
 */


#ifndef __XDMYSQL_ITERATOR_H
#define __XDMYSQL_ITERATOR_H


#include <kl/xcm.h>
#include "../../kscript/kscript.h"
#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/keylayout.h"
#include "../xdcommon/localrowcache.h"

// -- utility functions --

int mysql2xdType(int mysql_type);


struct MysqlDataAccessInfo
{
    // metadata
    std::wstring name;
    int type;
    int width;
    int scale;
    int ordinal;

    int mysql_type;

    // expression stuff
    std::wstring expr_text;
    kscript::ExprParser* expr;
    KeyLayout* key_layout;
    kscript::Value expr_result;
    std::wstring wstr_result;
    std::string str_result;

    MysqlDataAccessInfo()
    {
        expr = NULL;
        key_layout = NULL;
    }

    ~MysqlDataAccessInfo()
    {
        delete expr;
        delete key_layout;
    }
    
    bool isCalculated() const
    {
        return (expr_text.length() > 0) ? true : false;
    }
};




class MysqlIterator : public CommonBaseIterator,
                      public xd::ICacheRowUpdate
{
friend class MysqlDatabase;
friend class MySqlSet;

    XCM_CLASS_NAME("xdmysql.Iterator")
    XCM_BEGIN_INTERFACE_MAP(MysqlIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
        XCM_INTERFACE_ENTRY(xd::ICacheRowUpdate)
    XCM_END_INTERFACE_MAP()

    xd::IDatabase* cmniterGetDatabase() { return static_cast<xd::IDatabase*>(m_database); }

public:

    MysqlIterator(MysqlDatabase* database);
    ~MysqlIterator();
    bool init(const std::wstring& query);

    void setMySqlTable(const std::wstring& tbl) { m_table = tbl; }

    void setTable(const std::wstring& table);
    std::wstring getTable();
    xd::rowpos_t getRowCount();
    xd::IDatabasePtr getDatabase();
    xd::IIteratorPtr clone();

    unsigned int getIteratorFlags();
    void setIteratorFlags(unsigned int mask, unsigned int value);
    
    void skip(int delta);
    void goFirst();
    void goLast();
    double getPos();
    xd::rowid_t getRowId();
    bool bof();
    bool eof();
    bool seek(const unsigned char* key, int length, bool soft);
    bool seekValues(const wchar_t* arr[], size_t arr_size, bool soft);
    bool setPos(double pct);
    void goRow(const xd::rowid_t& rowid);

    xd::IStructurePtr getStructure();
    void refreshStructure();
    bool modifyStructure(xd::IStructure* struct_config, xd::IJob* job);

    xd::objhandle_t getHandle(const std::wstring& expr);
    xd::IColumnInfoPtr getInfo(xd::objhandle_t data_handle);
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

    // -- xd::ICacheRowUpdate --

    bool updateCacheRow(xd::rowid_t rowid,
                        xd::ColumnUpdateInfo* info,
                        size_t info_size);
                                
private:

    void skipWithCache(int delta);
    void saveRowToCache();
    void readRowFromCache(xd::rowpos_t row);
    
private:

    MysqlDatabase* m_database;

    MYSQL* m_data;
    MYSQL_RES* m_res;
    MYSQL_ROW m_row;           // holds the results of each row
    unsigned long* m_lengths;  // lenghts of the items in row
    int m_row_arr_size;        // number of elements in m_row
    xd::rowpos_t m_row_pos;

    std::vector<MysqlDataAccessInfo*> m_fields;
    std::vector<MysqlDataAccessInfo*> m_exprs;

    LocalRowCache m_cache;
    LocalRow m_cache_row;
    MYSQL_ROW m_cache_row_ptrs;  // holds the results of a row
                                 // when retrieved from cache
    bool m_cache_active;

    std::wstring m_path;
    std::wstring m_table;
};







#endif









