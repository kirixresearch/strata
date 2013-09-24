/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#ifndef __XDFS_XBASEITERATOR_H
#define __XDFS_XBASEITERATOR_H


#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/keylayout.h"
#include "../../kscript/kscript.h"
#include "xbase.h"


// -- XbaseDataAccessInfo struct declaration --

struct XbaseDataAccessInfo
{
    // metadata
    char xbase_type;

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

    XbaseDataAccessInfo()
    {
        xbase_type = 0;

        name = L"";
        type = tango::typeUndefined;
        width = 0;
        scale = 0;
        ordinal = 0;
        nulls_allowed = false;
        expr_text = L"";

        expr = NULL;
        key_layout = NULL;
        wstr_result = L"";
        str_result = "";
    }
    
    ~XbaseDataAccessInfo()
    {
        delete expr;
        delete key_layout;
    }
    
    bool isCalculated() const
    {
        return (expr_text.length() > 0) ? true : false;
    }
};


// XbaseIterator class declaration

class XbaseIterator : public CommonBaseIterator
{
friend class XbaseSet;

    XCM_CLASS_NAME("xdfs.XbaseIterator")
    XCM_BEGIN_INTERFACE_MAP(XbaseIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
    XCM_END_INTERFACE_MAP()

public:

    XbaseIterator();
    ~XbaseIterator();
    bool init(tango::IDatabasePtr db,
              XbaseSet* set,
              const std::wstring& filename);

    // tango::IIterator

    void setTable(const std::wstring& tbl);
    std::wstring getTable();
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

private:

    tango::IDatabasePtr m_database;
    XbaseSet* m_set;

    XbaseFile m_file;

    std::vector<XbaseDataAccessInfo*> m_fields;
    std::vector<XbaseDataAccessInfo*> m_exprs;

    int m_current_row;
    bool m_bof;
    bool m_eof;
};




#endif









