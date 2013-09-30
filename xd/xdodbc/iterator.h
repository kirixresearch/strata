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

int sql2xdType(SQLSMALLINT sql_type);
SQLSMALLINT tango2sqlType(int xd_type);
SQLSMALLINT xd2sqlCType(int xd_type);



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
        type = xd::typeUndefined;
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
    xd::objhandle_t left_handle;
    int left_type;
    std::wstring right_field;
};

class OdbcIteratorRelInfo
{
public:

    std::wstring relation_id;
    std::vector<OdbcIteratorRelField> fields;
};


class OdbcSet;
class OdbcIterator : public CommonBaseIterator,
                     public xd::ICacheRowUpdate,
                     public xd::IIteratorRelation
{
    friend class OdbcDatabase;
    friend class OdbcSet;

    XCM_CLASS_NAME("xdodbc.Iterator")
    XCM_BEGIN_INTERFACE_MAP(OdbcIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
        XCM_INTERFACE_ENTRY(xd::ICacheRowUpdate)
        XCM_INTERFACE_ENTRY(xd::IIteratorRelation)
    XCM_END_INTERFACE_MAP()

public:

    OdbcIterator(OdbcDatabase* database);
    ~OdbcIterator();
    
    bool init(const std::wstring& query);

    // xd::IIterator

    void setTable(const std::wstring& tbl);
    std::wstring getTable();
    xd::rowpos_t getRowCount();
    xd::IDatabasePtr getDatabase();
    xd::IIteratorPtr clone();

    void setIteratorFlags(unsigned int mask, unsigned int value);
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

    // xd::IIteratorRelation

    xd::IIteratorPtr getChildIterator(xd::IRelationPtr relation);
    xd::IIteratorPtr getFilteredChildIterator(xd::IRelationPtr relation);

    // xd::ICacheRowUpdate

    bool updateCacheRow(xd::rowid_t rowid,
                        xd::ColumnUpdateInfo* info,
                        size_t info_size);
                                
private:

    xd::IColumnInfoPtr colinfoFromDAI(OdbcDataAccessInfo* dai);
    void skipWithCache(int delta);
    void saveRowToCache();
    void readRowFromCache(xd::rowpos_t row);
    void clearFieldData();

private:

    std::vector<OdbcDataAccessInfo*> m_fields;
    std::vector<OdbcDataAccessInfo*> m_exprs;
    std::vector<OdbcIteratorRelInfo> m_relations;

    OdbcDatabase* m_database;
    xd::IStructurePtr m_structure;

    LocalRowCache m_cache;
    LocalRow m_cache_row;
    bool m_cache_active;
    xd::rowpos_t m_cache_dbrowpos;
    
    HENV m_env;
    HDBC m_conn;
    HSTMT m_stmt;
    bool m_bidirectional;
 
    bool m_eof;
    xd::rowpos_t m_row_pos;


    std::wstring m_path;

    // info from OdbcDatabase
    int m_db_type;
};


#endif





