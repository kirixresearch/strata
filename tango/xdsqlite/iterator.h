/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#ifndef __XDSQLITE_ITERATOR_H
#define __XDSQLITE_ITERATOR_H


#include "../xdcommon/cmnbaseiterator.h"


struct SlDataAccessInfo
{
    std::wstring name;
    int col_ordinal;
    char* buf;
    int datalen;

    std::string result_str;
    std::wstring result_wstr;

    tango::IColumnInfoPtr colinfo;
};


class SlIterator : public CommonBaseIterator
{
friend class SlDatabase;
friend class SlSet;

    XCM_CLASS_NAME("xdsqlite.Iterator")
    XCM_BEGIN_INTERFACE_MAP(SlIterator)
        XCM_INTERFACE_ENTRY(tango::IIterator)
    XCM_END_INTERFACE_MAP()

private:
    void clearFieldData();

public:

    SlIterator();
    ~SlIterator();
    bool init(const std::wstring& query);

    // -- tango::IIterator interface implementation --

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

private:

    std::vector<SlDataAccessInfo> m_columns;

    tango::IDatabasePtr m_database;
    SlDatabase* m_dbint;
    
    tango::ISetPtr m_set;
    tango::IStructurePtr m_set_structure;
    tango::tableord_t m_ordinal;
    std::wstring m_tablename;

    tango::tango_int64_t m_oid;
    bool m_eof;
    sqlite3* m_db;
    sqlite3_stmt* m_stmt;
};




#endif





