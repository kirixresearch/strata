/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Aaron L. Williams
 * Created:  2009-12-22
 *
 */


#ifndef __XDCLIENT_ITERATOR_H
#define __XDCLIENT_ITERATOR_H


#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/keylayout.h"
#include "../../kscript/kscript.h"


struct HttpDataAccessInfo
{
    std::wstring name;
    int type;
    int width;
    int scale;
    int ordinal;
    bool nulls_allowed;
    bool calculated;
    std::wstring expr_text;

    // expression stuff
    kscript::ExprParser* expr;
    KeyLayout* key_layout;
    kscript::Value expr_result;
    std::wstring wstr_result;
    std::string str_result;

    HttpDataAccessInfo()
    {
        name = L"";
        type = tango::typeUndefined;
        width = 0;
        scale = 0;
        ordinal = 0;
        nulls_allowed = false;
        calculated = false;
        expr_text = L"";

        expr = NULL;
        key_layout = NULL;
        wstr_result = L"";
        str_result = "";
    }
    
    ~HttpDataAccessInfo()
    {
        delete expr;
        delete key_layout;
    }
    
    bool isCalculated() const
    {
        return calculated;
    }
};

struct HttpCacheRow
{
    std::vector<std::wstring> values;
};


class ClientIterator : public CommonBaseIterator,
                       public tango::ICacheRowUpdate
{
friend class ClientDatabase;
friend class ClientSet;

    XCM_CLASS_NAME("xdclient.Iterator")
    XCM_BEGIN_INTERFACE_MAP(ClientIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
        XCM_INTERFACE_ENTRY(tango::ICacheRowUpdate)
    XCM_END_INTERFACE_MAP()

public:

    ClientIterator(ClientDatabase* database, ClientSet* set);
    ~ClientIterator();
    bool init(const std::wstring& handle, const std::wstring& url_query);

    // tango::IIterator interface implementation

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

    // tango::ICacheRowUpdate

    bool updateCacheRow(tango::rowid_t rowid,
                        tango::ColumnUpdateInfo* info,
                        size_t info_size);

private:

    bool refreshDataAccessInfo();
    void clearDataAccessInfo();

    bool populateRowCache(int start = 0, int limit = 0);
    void clearRowCache();
    
    bool rowInCache(int row);
    bool rowCachePopulated();

private:

    std::vector<HttpDataAccessInfo*> m_fields;
    std::vector<HttpDataAccessInfo*> m_exprs;

    std::vector<HttpCacheRow*> m_cache_rows;
    std::wstring m_url_query;

    ClientDatabase* m_database;
    ClientSet* m_set;
    std::wstring m_handle;

    tango::rowid_t m_current_row;
    tango::rowid_t m_cache_row_count;
    tango::rowid_t m_cache_start;
    tango::rowid_t m_cache_limit;
    bool m_cache_populated;
    
    tango::datetime_t m_result_date;
    std::wstring m_result_wstring;
    std::string m_result_string;
    double m_result_double;
    int m_result_integer;
    bool m_result_boolean;
};


#endif

