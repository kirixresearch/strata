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


#include <xcm/xcm.h>
#include "../../kscript/kscript.h"
#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/keylayout.h"
#include "../xdcommon/localrowcache.h"

// -- utility functions --

int mysql2tangoType(int mysql_type);


struct MySqlDataAccessInfo
{
    // -- metadata --
    std::wstring name;
    int type;
    int width;
    int scale;
    int ordinal;

    int mysql_type;

    // -- expression stuff --
    std::wstring expr_text;
    kscript::ExprParser* expr;
    KeyLayout* key_layout;
    kscript::Value expr_result;
    std::wstring wstr_result;
    std::string str_result;

    MySqlDataAccessInfo()
    {
        expr = NULL;
        key_layout = NULL;
    }

    ~MySqlDataAccessInfo()
    {
        delete expr;
        delete key_layout;
    }
    
    bool isCalculated() const
    {
        return (expr_text.length() > 0) ? true : false;
    }
};




class MySqlIterator : public CommonBaseIterator,
                      public tango::ICacheRowUpdate
{
friend class MySqlDatabase;
friend class MySqlSet;

    XCM_CLASS_NAME("xdmysql.Iterator")
    XCM_BEGIN_INTERFACE_MAP(MySqlIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
        XCM_INTERFACE_ENTRY(tango::ICacheRowUpdate)
    XCM_END_INTERFACE_MAP()

public:

    MySqlIterator();
    ~MySqlIterator();
    bool init(const std::wstring& query);

    // -- IIterator interface implementation --

    tango::ISetPtr getSet();
    tango::IDatabasePtr getDatabase();
    tango::IIteratorPtr clone();

    unsigned int getIteratorFlags();
    void setIteratorFlags(unsigned int mask, unsigned int value);
    
    void skip(int delta);
    void goFirst();
    void goLast();
    double getPos();
    tango::rowid_t getRowId();
    bool bof();
    bool eof();
    bool seek(const unsigned char* key, int length, bool soft);
    bool seekValues(const wchar_t* arr[], size_t arr_size, bool soft);
    bool setPos(double pct);
    void goRow(const tango::rowid_t& rowid);

    tango::IStructurePtr getStructure();
    void refreshStructure();
    bool modifyStructure(tango::IStructure* struct_config, tango::IJob* job);

    tango::objhandle_t getHandle(const std::wstring& expr);
    tango::IColumnInfoPtr getInfo(tango::objhandle_t data_handle);
    int getType(tango::objhandle_t data_handle);
    bool releaseHandle(tango::objhandle_t data_handle);

    const unsigned char* getRawPtr(tango::objhandle_t data_handle);
    int getRawWidth(tango::objhandle_t data_handle);
    const std::string& getString(tango::objhandle_t data_handle);
    const std::wstring& getWideString(tango::objhandle_t data_handle);
    tango::datetime_t getDateTime(tango::objhandle_t data_handle);
    double getDouble(tango::objhandle_t data_handle);
    int getInteger(tango::objhandle_t data_handle);
    bool getBoolean(tango::objhandle_t data_handle);
    bool isNull(tango::objhandle_t data_handle);

    // -- tango::ICacheRowUpdate --

    bool updateCacheRow(tango::rowid_t rowid,
                        tango::ColumnUpdateInfo* info,
                        size_t info_size);
                                
private:

    void skipWithCache(int delta);
    void saveRowToCache();
    void readRowFromCache(tango::rowpos_t row);
    
private:

    tango::IDatabasePtr m_database;
    tango::ISetPtr m_set;

    MYSQL* m_data;
    MYSQL_RES* m_res;
    MYSQL_ROW m_row;           // holds the results of each row
    unsigned long* m_lengths;  // lenghts of the items in row
    int m_row_arr_size;        // number of elements in m_row
    tango::rowpos_t m_row_pos;

    std::vector<MySqlDataAccessInfo*> m_fields;
    std::vector<MySqlDataAccessInfo*> m_exprs;

    LocalRowCache m_cache;
    LocalRow m_cache_row;
    MYSQL_ROW m_cache_row_ptrs;  // holds the results of a row
                                 // when retrieved from cache
    bool m_cache_active;
};






#endif









