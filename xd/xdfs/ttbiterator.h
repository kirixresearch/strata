/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#ifndef __XDFS_TTBITERATOR_H
#define __XDFS_TTBITERATOR_H


#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/keylayout.h"
#include "../../kscript/kscript.h"



// -- TtbDataAccessInfo struct declaration --

struct TtbDataAccessInfo
{
    // metadata
    char ttb_type;

    std::wstring name;
    int type;
    int offset;
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

    TtbDataAccessInfo()
    {
        ttb_type = 0;

        name = L"";
        type = xd::typeUndefined;
        offset = 0;
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
    
    ~TtbDataAccessInfo()
    {
        delete expr;
        delete key_layout;
    }
    
    bool isCalculated() const
    {
        return (expr_text.length() > 0) ? true : false;
    }

    bool isColumn() const
    {
        return (name.length() > 0) ? true : false;
    }
};


// TtbIterator class declaration
class TtbSet;

class TtbIterator : public CommonBaseIterator
{
friend class TtbSet;

    XCM_CLASS_NAME("xdfs.TtbIterator")
    XCM_BEGIN_INTERFACE_MAP(TtbIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
    XCM_END_INTERFACE_MAP()

    xd::IDatabase* cmniterGetDatabase() { return static_cast<xd::IDatabase*>(m_database); }

public:

    TtbIterator(FsDatabase* database);
    ~TtbIterator();

    bool init(TtbSet* set,
              const std::wstring& filename);

    bool init(TtbSet* set,
              TtbTable* table);

    // xd::IIterator

    void setTable(const std::wstring& tbl);
    std::wstring getTable();
    xd::rowpos_t getRowCount();
    xd::IDatabasePtr getDatabase();
    xd::IIteratorPtr clone();

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

private:

    void updatePosition();

private:

    FsDatabase* m_database;
    TtbSet* m_set;
    TtbTable m_file;
    TtbTable* m_table; // usually a pointer to m_file

    xd::tableord_t m_table_ord;
    xd::rowid_t m_rowid;
    int m_buf_rowcount;
    int m_buf_pos;
    int m_read_ahead_rowcount;
    int m_table_rowwidth;
    xd::rowpos_t m_row_num;    // sequential row number for ROWNUM()
    xd::rowpos_t* m_rowpos_buf;
    unsigned char* m_buf;
    unsigned char* m_rowptr;
    bool m_include_deleted;
    bool m_bof;
    bool m_eof;

    std::vector<TtbDataAccessInfo*> m_fields;
    std::vector<TtbDataAccessInfo*> m_exprs;
};




#endif









