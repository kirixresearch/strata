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
    tango::objhandle_t left_handle;
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
                      public tango::ICacheRowUpdate,
                      public tango::IIteratorRelation
{
    friend class PgsqlDatabase;
    friend class PgsqlSet;

    XCM_CLASS_NAME("xdpgsql.Iterator")
    XCM_BEGIN_INTERFACE_MAP(PgsqlIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
        XCM_INTERFACE_ENTRY(tango::ICacheRowUpdate)
        XCM_INTERFACE_ENTRY(tango::IIteratorRelation)
    XCM_END_INTERFACE_MAP()

public:

    PgsqlIterator(PgsqlDatabase* database, PgsqlSet* set = NULL);
    ~PgsqlIterator();
    
    bool init(const std::wstring& query);

    // tango::IIterator interface implementation

    tango::ISetPtr getSet();
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

    tango::IColumnInfoPtr colinfoFromDAI(PgsqlDataAccessInfo* dai);
    void skipWithCache(int delta);
    void saveRowToCache();
    void readRowFromCache(tango::rowpos_t row);
    void clearFieldData();

private:

    std::vector<PgsqlDataAccessInfo*> m_fields;
    std::vector<PgsqlDataAccessInfo*> m_exprs;
    std::vector<PgsqlIteratorRelInfo> m_relations;

    PgsqlDatabase* m_database;
    PgsqlSet* m_set;
    tango::IStructurePtr m_structure;
    PGconn* m_conn;
    PGresult* m_res;

    LocalRowCache m_cache;
    LocalRow m_cache_row;
    bool m_cache_active;
    tango::rowpos_t m_cache_dbrowpos;
    
    bool m_bidirectional;
 
    bool m_eof;
    tango::rowpos_t m_row_pos;
    int m_block_row;
};


#endif





