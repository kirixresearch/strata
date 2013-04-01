/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-09-03
 *
 */


#ifndef __XDODBC_ITERATOR_H
#define __XDODBC_ITERATOR_H


#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/localrowcache.h"
#include "../xdcommon/keylayout.h"
#include "../../kscript/kscript.h"


// conversion functions for tango and sql types

int sql2tangoType(SQLSMALLINT sql_type);
SQLSMALLINT tango2sqlType(int tango_type);
SQLSMALLINT tango2sqlCType(int tango_type);



struct OdbcDataAccessInfo
{
    // metadata
    int odbc_type;
    
    std::wstring name;
    int type;
    int width;
    int scale;
    int ordinal;
    bool nulls_allowed;
    std::wstring expr_text;

    // expression stuff
    kscript::ExprParser* expr;
    KeyLayout* key_layout;
    kscript::Value expr_result;
    std::wstring wstr_result;
    std::string str_result;

    // data
    char* str_val;
    wchar_t* wstr_val;
    int int_val;
    double dbl_val;
    unsigned char bool_val;
    SQL_DATE_STRUCT date_val;
    SQL_TIME_STRUCT time_val;
    SQL_TIMESTAMP_STRUCT datetime_val;
    SQLLEN indicator;

    OdbcDataAccessInfo()
    {
        odbc_type = -1;
        
        name = L"";
        type = tango::typeUndefined;
        width = 0;
        scale = 0;
        ordinal = 0;
        nulls_allowed = false;
        expr_text = L"";

        expr = NULL;
        key_layout = NULL;
        str_result = "";
        wstr_result = L"";

        str_val = NULL;
        wstr_val = NULL;
        int_val = 0;
        dbl_val = 0.0;
        bool_val = 0;
        indicator = 0;
        memset(&date_val, 0, sizeof(SQL_DATE_STRUCT));
        memset(&time_val, 0, sizeof(SQL_TIME_STRUCT));
        memset(&datetime_val, 0, sizeof(SQL_TIMESTAMP_STRUCT));
    }

    ~OdbcDataAccessInfo()
    {
        delete[] str_val;
        delete[] wstr_val;
        
        delete expr;
        delete key_layout;
    }
    
    bool isCalculated()
    {
        return (expr_text.length() > 0) ? true : false;
    }
};


struct OdbcIteratorRelField
{
    tango::objhandle_t left_handle;
    int left_type;
    std::wstring right_field;
};

class OdbcIteratorRelInfo
{
public:

    std::wstring relation_id;
    std::vector<OdbcIteratorRelField> fields;
};

class OdbcIterator : public CommonBaseIterator,
                     public tango::ICacheRowUpdate,
                     public tango::IIteratorRelation
{
    friend class OdbcDatabase;
    friend class OdbcSet;

    XCM_CLASS_NAME("xdodbc.Iterator")
    XCM_BEGIN_INTERFACE_MAP(OdbcIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
        XCM_INTERFACE_ENTRY(tango::ICacheRowUpdate)
        XCM_INTERFACE_ENTRY(tango::IIteratorRelation)
    XCM_END_INTERFACE_MAP()

public:

    OdbcIterator(OdbcDatabase* database, OdbcSet* set = NULL);
    ~OdbcIterator();
    
    bool init(const std::wstring& query);

    // tango::IIterator interface implementation

    tango::ISetPtr getSet();
    tango::rowpos_t getRowCount();
    tango::IDatabasePtr getDatabase();
    tango::IIteratorPtr clone();

    void setIteratorFlags(unsigned int mask, unsigned int value);
    unsigned int getIteratorFlags();

    void skip(int delta);
    void goFirst();
    void goLast();
    tango::rowid_t getRowId();
    bool bof();
    bool eof();
    bool seek(const unsigned char* key, int length, bool soft);
    bool seekValues(const wchar_t* arr[], size_t arr_size, bool soft);
    bool setPos(double pct);
    double getPos();
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

    // tango::IIteratorRelation

    tango::ISetPtr getChildSet(tango::IRelationPtr relation);
    tango::IIteratorPtr getChildIterator(tango::IRelationPtr relation);

    // tango::ICacheRowUpdate

    bool updateCacheRow(tango::rowid_t rowid,
                        tango::ColumnUpdateInfo* info,
                        size_t info_size);
                                
private:

    tango::IColumnInfoPtr colinfoFromDAI(OdbcDataAccessInfo* dai);
    void skipWithCache(int delta);
    void saveRowToCache();
    void readRowFromCache(tango::rowpos_t row);
    void clearFieldData();

private:

    std::vector<OdbcDataAccessInfo*> m_fields;
    std::vector<OdbcDataAccessInfo*> m_exprs;
    std::vector<OdbcIteratorRelInfo> m_relations;

    OdbcDatabase* m_database;
    OdbcSet* m_set;
    tango::IStructurePtr m_structure;

    LocalRowCache m_cache;
    LocalRow m_cache_row;
    bool m_cache_active;
    tango::rowpos_t m_cache_dbrowpos;
    
    HENV m_env;
    HDBC m_conn;
    HSTMT m_stmt;
    bool m_bidirectional;
 
    bool m_eof;
    tango::rowpos_t m_row_pos;

    // info from OdbcDatabase
    int m_db_type;
};


#endif





