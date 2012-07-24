/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-04-18
 *
 */


#ifndef __XDORACLE_ITERATOR_H
#define __XDORACLE_ITERATOR_H


#include "../../kscript/kscript.h"
#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/keylayout.h"
#include "../xdcommon/localrowcache.h"


struct OracleDataAccessInfo
{
    // -- metadata --
    std::wstring name;
    int oracle_charset;
    int oracle_type;
    int tango_type;
    int width;
    int precision;
    int scale;
    int ordinal;

    // -- expression stuff --
    KeyLayout* key_layout;
    std::wstring expr_text;
    kscript::ExprParser* expr;
    kscript::Value expr_result;
    std::wstring wstr_result;
    std::string str_result;

    // -- data --
    char* str_val;
    wchar_t* wstr_val;
    int int_val;
    double dbl_val;
    bool bool_val;
    unsigned char date_val[7];

    // -- define variables --
    OCIDefine* define;
    sb2 indicator;
    ub2 str_len;

    OracleDataAccessInfo()
    {
        str_val = NULL;
        wstr_val = NULL;
        expr = NULL;
        key_layout = NULL;
    }

    ~OracleDataAccessInfo()
    {
        delete[] str_val;
        delete[] wstr_val;

        delete expr;
        delete key_layout;
    }
    
    bool isCalculated() const
    {
        return (expr_text.length() > 0) ? true : false;
    }
};


class OracleIterator : public CommonBaseIterator,
                       public tango::ICacheRowUpdate
{
friend class OracleDatabase;
friend class OracleSet;

    XCM_CLASS_NAME("xdoracle.Iterator")
    XCM_BEGIN_INTERFACE_MAP(OracleIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
        XCM_INTERFACE_ENTRY(tango::ICacheRowUpdate)
    XCM_END_INTERFACE_MAP()

public:

    OracleIterator();
    ~OracleIterator();
    bool init(const std::wstring& query);

    // -- IIterator interface implementation --

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
    void goRow(const tango::rowid_t& row);

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

private:

    void skipWithCache(int delta);
    void saveRowToCache();
    void readRowFromCache(tango::rowpos_t row);

    // -- tango::ICacheRowUpdate --

    bool updateCacheRow(tango::rowid_t rowid,
                        tango::ColumnUpdateInfo* info,
                        size_t info_size);
private:

    tango::IDatabasePtr m_database;
    tango::ISetPtr m_set;

    OCIEnv* m_env;
    OCISvcCtx* m_svc;
    OCIStmt* m_stmt;
    OCIError* m_err;

    std::vector<OracleDataAccessInfo*> m_fields;
    std::vector<OracleDataAccessInfo*> m_exprs;

    tango::rowpos_t m_row_pos;
    LocalRowCache m_cache;
    LocalRow m_cache_row;

    std::wstring m_name;
    bool m_eof;
    bool m_cache_active;
};




#endif









