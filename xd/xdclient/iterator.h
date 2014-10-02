/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
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
        type = xd::typeUndefined;
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

struct ClientCacheRow
{
    std::vector<std::wstring> values;
};



xcm_interface IClientIterator : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdclient.IClientIterator")

public:

    virtual std::wstring getHandle() = 0;
    virtual ClientDatabase* getClientDatabase() = 0;
};


XCM_DECLARE_SMARTPTR(IClientIterator)


class ClientIterator : public CommonBaseIterator,
                       public xd::ICacheRowUpdate,
                       public IClientIterator
{
friend class ClientDatabase;
friend class ClientSet;

    XCM_CLASS_NAME("xdclient.Iterator")
    XCM_BEGIN_INTERFACE_MAP(ClientIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
        XCM_INTERFACE_ENTRY(xd::ICacheRowUpdate)
        XCM_INTERFACE_ENTRY(IClientIterator)
    XCM_END_INTERFACE_MAP()

    xd::IDatabase* cmniterGetDatabase() { return static_cast<xd::IDatabase*>(m_database); }

public:

    ClientIterator(ClientDatabase* database);
    ~ClientIterator();

    bool init(const std::wstring& handle, const std::wstring& url_query);
    bool init(const xd::QueryParams& qp);

    // xd::IIterator

    void setTable(const std::wstring& tbl);
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

    // xd::ICacheRowUpdate

    bool updateCacheRow(xd::rowid_t rowid,
                        xd::ColumnUpdateInfo* info,
                        size_t info_size);

    // IClientIterator

    std::wstring getHandle() { return m_handle; }
    ClientDatabase* getClientDatabase() { return m_database; }

private:

    bool refreshDataAccessInfo();
    void clearDataAccessInfo();
    void clearCache();

private:

    std::vector<HttpDataAccessInfo*> m_fields;
    std::vector<HttpDataAccessInfo*> m_exprs;

    std::vector<ClientCacheRow> m_cache_rows;
    ClientCacheRow* m_current_row_ptr;

    std::wstring m_url_query;

    ClientDatabase* m_database;
    std::wstring m_handle;

    xd::Structure m_structure;

    xd::rowid_t m_current_row;
    xd::rowid_t m_cache_row_count;
    xd::rowid_t m_cache_start;

    xd::rowpos_t m_row_count;
};


#endif

