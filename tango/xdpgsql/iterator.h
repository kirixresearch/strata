/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-03-01
 *
 */


#ifndef __XDPGSQL_ITERATOR_H
#define __XDPGSQL_ITERATOR_H


#if _MSC_VER < 1300
#define SQLLEN SQLINTEGER
#define SQLULEN SQLUINTEGER
#endif

#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/localrowcache.h"
#include "../xdcommon/keylayout.h"
#include "../../kscript/kscript.h"




struct PgsqlDataAccessInfo
{
    // metadata
    int pg_type;
    
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



    PgsqlDataAccessInfo()
    {
        pg_type = -1;
        
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
    }

    ~PgsqlDataAccessInfo()
    {
        delete expr;
        delete key_layout;
    }
    
    bool isCalculated()
    {
        return (expr_text.length() > 0) ? true : false;
    }
};


struct PgsqlIteratorRelField
{
    xd::objhandle_t left_handle;
    int left_type;
    std::wstring right_field;
};

class PgsqlIteratorRelInfo
{
public:

    std::wstring relation_id;
    std::vector<PgsqlIteratorRelField> fields;
};

class PgsqlIterator : public CommonBaseIterator,
                      public xd::ICacheRowUpdate,
                      public xd::IIteratorRelation
{
    friend class PgsqlDatabase;

    XCM_CLASS_NAME("xdpgsql.Iterator")
    XCM_BEGIN_INTERFACE_MAP(PgsqlIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
        XCM_INTERFACE_ENTRY(xd::ICacheRowUpdate)
        XCM_INTERFACE_ENTRY(xd::IIteratorRelation)
    XCM_END_INTERFACE_MAP()

public:

    PgsqlIterator(PgsqlDatabase* database);
    ~PgsqlIterator();
    
    bool init(const std::wstring& query);
    bool init(PGconn* conn, PGresult* res);

    // xd::IIterator

    void setTable(const std::wstring& table);
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

    std::vector<PgsqlDataAccessInfo*> m_fields;
    std::vector<PgsqlDataAccessInfo*> m_exprs;
    std::vector<PgsqlIteratorRelInfo> m_relations;

    PgsqlDatabase* m_database;
    xd::IStructurePtr m_structure;
    PGconn* m_conn;
    PGresult* m_res;

    LocalRowCache m_cache;
    LocalRow m_cache_row;
    bool m_cache_active;
    xd::rowpos_t m_cache_dbrowpos;
    
    bool m_server_side_cursor;
 
    bool m_eof;
    xd::rowpos_t m_row_pos;

    xd::rowpos_t m_block_start;
    int m_block_row;
    int m_block_rowcount;

    std::wstring m_path;
};


#endif





