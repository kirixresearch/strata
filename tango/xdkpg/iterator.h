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



#include "../xdcommon/cmnbaseiterator.h"
#include "../xdcommon/keylayout.h"
#include "../../kscript/kscript.h"
#include <kl/file.h>

class PkgStreamReader;


struct KpgDataAccessInfo
{
    // metadata
    int pg_type;
    
    std::wstring name;
    int type;
    int width;
    int scale;
    int ordinal;
    int buf_width;
    int offset;
    bool nulls_allowed;
    std::wstring expr_text;

    // expression stuff
    kscript::ExprParser* expr;
    KeyLayout* key_layout;
    kscript::Value expr_result;
    std::wstring wstr_result;
    std::string str_result;



    KpgDataAccessInfo()
    {
        pg_type = -1;
        
        name = L"";
        type = tango::typeUndefined;
        width = 0;
        scale = 0;
        ordinal = 0;
        offset = 0;
        buf_width = 0;
        nulls_allowed = false;
        expr_text = L"";

        expr = NULL;
        key_layout = NULL;
        str_result = "";
        wstr_result = L"";
    }

    ~KpgDataAccessInfo()
    {
        delete expr;
        delete key_layout;
    }
    
    bool isCalculated()
    {
        return (expr_text.length() > 0) ? true : false;
    }
};


struct KpgIteratorRelField
{
    tango::objhandle_t left_handle;
    int left_type;
    std::wstring right_field;
};

class KpgIteratorRelInfo
{
public:

    std::wstring relation_id;
    std::vector<KpgIteratorRelField> fields;
};

class KpgIterator : public CommonBaseIterator
{
    friend class KpgDatabase;
    friend class KpgSet;

    XCM_CLASS_NAME("xdkpg.Iterator")
    XCM_BEGIN_INTERFACE_MAP(KpgIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
    XCM_END_INTERFACE_MAP()

public:

    KpgIterator(KpgDatabase* database, KpgSet* set = NULL);
    ~KpgIterator();
    
    bool init();

    // tango::IIterator

    tango::ISetPtr getSet();
    std::wstring getTable();
    tango::rowpos_t getRowCount();
    tango::IDatabasePtr getDatabase();
    tango::IIteratorPtr clone();

    void setIteratorFlags(unsigned int mask, unsigned int value);
    unsigned int getIteratorFlags();

    void skip(int delta);
    void skipForward();
    void skipBackward();

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

    std::vector<KpgDataAccessInfo*> m_fields;
    std::vector<KpgDataAccessInfo*> m_exprs;
    std::vector<KpgIteratorRelInfo> m_relations;

    KpgDatabase* m_database;
    KpgSet* m_set;
    PkgStreamReader* m_reader;
    tango::IStructurePtr m_structure;

    std::vector<xf_off_t> m_block_offsets;
    size_t m_cur_block;

    tango::rowpos_t m_row_pos;
    unsigned char* m_data;
    int m_data_len;
    unsigned char* m_row;
    bool m_eof;

    int m_row_width;
};


#endif





