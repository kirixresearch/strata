/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2005-05-12
 *
 */


#ifndef __XDFS_FIXEDLENGTHTEXTITERATOR_H
#define __XDFS_FIXEDLENGTHTEXTITERATOR_H


#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/keylayout.h"
#include "../../kscript/kscript.h"



// -- FixedLengthDataAccessInfo struct declaration --

struct FixedLengthDataAccessInfo
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
    bool calculated;
    std::wstring expr_text;
    bool visible;           // is part of visible structure (via getStructure())

    // expression stuff
    kscript::ExprParser* expr;
    KeyLayout* key_layout;
    kscript::Value expr_result;
    std::wstring wstr_result;
    std::string str_result;

    FixedLengthDataAccessInfo()
    {
        ttb_type = 0;

        name = L"";
        type = xd::typeUndefined;
        offset = 0;
        width = 0;
        scale = 0;
        ordinal = 0;
        nulls_allowed = false;
        visible = false;
        calculated = false;
        expr_text = L"";

        expr = NULL;
        key_layout = NULL;
        wstr_result = L"";
        str_result = "";
    }
    
    ~FixedLengthDataAccessInfo()
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


// FixedLengthTextIterator class declaration
class FixedLengthTextSet;

class FixedLengthTextIterator : public CommonBaseIterator
{
friend class FixedLengthTextSet;

    XCM_CLASS_NAME("xdfs.FixedLengthTextIterator")
    XCM_BEGIN_INTERFACE_MAP(FixedLengthTextIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
        XCM_INTERFACE_ENTRY(xd::IIteratorRelation)
    XCM_END_INTERFACE_MAP()

    xd::IDatabase* cmniterGetDatabase() { return static_cast<xd::IDatabase*>(m_database); }

public:

    FixedLengthTextIterator(FsDatabase* database);
    ~FixedLengthTextIterator();

    bool init(FixedLengthTextSet* set,  const std::wstring& filename, const std::wstring& columns);
    bool init(FixedLengthTextSet* set,  FixedLengthTable* table, const std::wstring& columns);
    bool initFromBuffer(FixedLengthTextSet* set, FixedLengthTable* table, unsigned char* buffer, const std::wstring& columns);

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

    xd::Structure getStructure();
    xd::Structure getParserStructure();
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

protected:

    static void func_rawvalue(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    void onParserInit(kscript::ExprParser* parser);

private:

    void updatePosition();

private:

    FsDatabase* m_database;
    FixedLengthTextSet* m_set;
    FixedLengthTable m_file;
    FixedLengthTable* m_table; // usually a pointer to m_file
    FixedLengthRow m_row;
    std::wstring m_columns;

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
    bool m_buffer_wrapper_mode;

    std::vector<FixedLengthDataAccessInfo*> m_fields;
    std::vector<FixedLengthDataAccessInfo*> m_exprs;
};




#endif




