/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-29
 *
 */


#ifndef __XDFS_DELIMITEDTEXTITERATOR_H
#define __XDFS_DELIMITEDTEXTITERATOR_H


#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/keylayout.h"
#include "../../kscript/kscript.h"
#include "delimitedtext.h"



struct DelimitedTextDataAccessInfo
{
    // metadata
    std::wstring name;
    int type;
    int width;
    int scale;
    int ordinal;
    bool nulls_allowed;
    std::wstring expr_text;

    // -- expression stuff --
    kscript::ExprParser* expr;
    KeyLayout* key_layout;
    kscript::Value expr_result;
    std::wstring wstr_result;
    std::string str_result;

    DelimitedTextDataAccessInfo()
    {
        name = L"";
        type = xd::typeUndefined;
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
    
    ~DelimitedTextDataAccessInfo()
    {
        delete expr;
        delete key_layout;
    }
    
    bool isCalculated() const
    {
        return (expr_text.length() > 0) ? true : false;
    }
};


class FsDatabase;
class DelimitedSourceBindInfo;

class DelimitedTextIterator : public CommonBaseIterator
{
friend class DelimitedTextSet;

    XCM_CLASS_NAME("xdfs.DelimitedTextIterator")
    XCM_BEGIN_INTERFACE_MAP(DelimitedTextIterator)
        XCM_INTERFACE_ENTRY(xd::IIterator)
        XCM_INTERFACE_ENTRY(xd::IIteratorRelation)
    XCM_END_INTERFACE_MAP()

    xd::IDatabase* cmniterGetDatabase() { return (xd::IDatabase*)m_database; }

public:

    DelimitedTextIterator(FsDatabase* db);
    ~DelimitedTextIterator();

    bool init(DelimitedTextSet* set, const std::wstring& columns);

    void setTable(const std::wstring& tbl);
    std::wstring getTable();
    xd::rowpos_t getRowCount();
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

    xd::Structure getStructure();
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
    
public:

    FsDatabase* m_database;
    DelimitedTextSet* m_set;
    std::wstring m_columns;
    
    DelimitedTextFile m_file;
    std::vector<DelimitedTextDataAccessInfo*> m_fields;
    std::vector<DelimitedTextDataAccessInfo*> m_exprs;
    std::vector<DelimitedSourceBindInfo*> m_src_bindings;

    bool m_bof;
    bool m_eof;
};




#endif









