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
#include "fixedlengthtextset.h"


class FixedLengthTextIterator;

struct FixedLengthTextDataAccessInfo
{
    FixedLengthTextIterator* iter;

    int src_offset;
    int src_width;
    int src_encoding;
    
    std::wstring name;
    int type;
    int width;
    int scale;
    int ordinal;
    bool nulls_allowed;
    bool calculated;
    std::wstring expr_text;

    // -- expression stuff --
    kscript::ExprParser* expr;
    KeyLayout* key_layout;
    kscript::Value expr_result;
    std::wstring wstr_result;
    std::string str_result;

    FixedLengthTextDataAccessInfo()
    {
        src_offset = 0;
        src_width = 0;
        src_encoding = 0;
        
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
    
    ~FixedLengthTextDataAccessInfo()
    {
        delete expr;
        delete key_layout;
    }
    
    bool isCalculated() const
    {
        return calculated;
    }
};




class FixedLengthTextIterator : public CommonBaseIterator
{
friend class FixedLengthTextSet;

    XCM_CLASS_NAME("xdfs.FixedLengthTextIterator")
    XCM_BEGIN_INTERFACE_MAP(FixedLengthTextIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
    XCM_END_INTERFACE_MAP()

public:

    FixedLengthTextIterator();
    ~FixedLengthTextIterator();

    bool init(tango::IDatabasePtr db,
              FixedLengthTextSet* set,
              const std::wstring& columns);

    // -- IIterator interface implementation --

    tango::ISetPtr getSet();
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
    
    void updateDaiEntry(FixedLengthTextDataAccessInfo* dai);

private:

    bool internalEof(xf_off_t offset);
    
    size_t getCurrentRowLength();

    wchar_t getChar(xf_off_t row, xf_off_t col);
    wchar_t getCharAtOffset(xf_off_t offset);
    wchar_t getDelimitedChar(xf_off_t row, xf_off_t col);
    wchar_t getFixedChar(xf_off_t row, xf_off_t col);

    kscript::ExprParser* createCastingExprParser();

    static bool script_host_parse_hook(kscript::ExprParseHookInfo& info);

private:

    // override from the CommonBaseIterator.  This function
    // gets the field names used in the expression parser
    virtual tango::IStructurePtr getParserStructure();

private:

    tango::IDatabasePtr m_database;
    FixedLengthTextSet* m_set;

    std::wstring m_columns_string;
    std::vector<std::wstring> m_columns;
    
    std::vector<FixedLengthTextDataAccessInfo*> m_source_fields; // source fields in the fixed length
    std::vector<FixedLengthTextDataAccessInfo*> m_fields;        // dest fields (real)
    std::vector<FixedLengthTextDataAccessInfo*> m_exprs;         // expressions parsed with getHandle()

    std::wstring m_path;
    xf_file_t m_file;
    xf_off_t m_file_size;

    int m_file_type;                // fixed or line-delimited (see fixedlengthtextset.h)
    std::wstring m_line_delimiters; // character array containing one or more delimiters
    
    tango::rowpos_t m_cur_row;      // the current row that we're on
    xf_off_t m_cur_row_offset;      // offset where the current row begins
    size_t m_cur_row_length;        // length of the current row
    size_t m_row_width;             // user-specified width of each row
    size_t m_skip_chars;            // number of chars to skip at the beginning of the file

    size_t m_chunk_size;            // size of data inside m_buf
    xf_off_t m_chunk_offset;        // offset of m_buf in the file

    unsigned char* m_buf;
};


#endif



