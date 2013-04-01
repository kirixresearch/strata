/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-03-17
 *
 */


#ifndef __XDSQLSERVER_ITERATOR_H
#define __XDSQLSERVER_ITERATOR_H


#include "../../kscript/kscript.h"
#include "../xdcommon/cmnbaseiterator.h"


#ifdef WIN32
#include "tdsiconv.h"
#endif


struct SqlServerDataAccessInfo
{
    // -- metadata --
    std::wstring m_name;
    int m_type;
    int m_width;
    int m_scale;
    int m_ordinal;

    // -- expression stuff --
    kscript::ExprParser* m_expr;
    kscript::Value m_expr_result;
    std::wstring m_wstr_result;
    std::string m_str_result;

    unsigned char* m_convert_buf;

    SqlServerDataAccessInfo()
    {
        m_expr = NULL;
        m_convert_buf = NULL;
    }

    ~SqlServerDataAccessInfo()
    {
        if (m_expr)
        {
            delete m_expr;
        }

        if (m_convert_buf)
        {
            free(m_convert_buf);
        }
    }
};




class SqlServerIterator : public CommonBaseIterator
{
friend class SqlServerDatabase;
friend class SqlServerSet;

    XCM_CLASS_NAME("xdsqlserver.Iterator")
    XCM_BEGIN_INTERFACE_MAP(SqlServerIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
    XCM_END_INTERFACE_MAP()

public:

    SqlServerIterator();
    ~SqlServerIterator();
    bool init(const std::wstring& query);

    // tango::IIterator

    tango::ISetPtr getSet();
    tango::rowpos_t getRowCount();
    tango::IDatabasePtr getDatabase();
    tango::IIteratorPtr clone();

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

    tango::IDatabasePtr m_database;
    tango::ISetPtr m_set;

    TDSSOCKET* m_tds;
    TDSCONNECTION* m_connect_info;
    TDSCONTEXT* m_context;

    std::vector<SqlServerDataAccessInfo*> m_fields;
    std::vector<SqlServerDataAccessInfo*> m_exprs;

    std::wstring m_name;
    bool m_eof;
};



#endif









