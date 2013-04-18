/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2008-08-13
 *
 */


#ifndef __XDSQLITE_SET_H
#define __XDSQLITE_SET_H


class SlSet : public tango::ISet
{

friend class SlDatabase;
friend class SlIterator;
friend class SlRowInserter;

    XCM_CLASS_NAME("xdsqlite.Set")
    XCM_BEGIN_INTERFACE_MAP(SlSet)
        XCM_INTERFACE_ENTRY(tango::ISet)
    XCM_END_INTERFACE_MAP()

public:

    SlSet(SlDatabase* database);
    ~SlSet();
    
    bool init();

    std::wstring getSetId();

    tango::IStructurePtr getStructure();

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    tango::rowpos_t getRowCount();

private:

    xcm::mutex m_object_mutex;
    tango::IStructurePtr m_structure;

    SlDatabase* m_database;
    
    sqlite3* m_sqlite;

    std::wstring m_tablename;
    tango::tableord_t m_ordinal;
    int m_row_count;
    
    std::wstring m_set_id;
};



class SlRowInserterData
{
public:

    std::wstring name;
    int idx;
    int type;
    int length;
    char* str_data;
    size_t buf_len;
    tango::IColumnInfoPtr colinfo;
};


class SlRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdsl.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(SlRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()

public:

    SlRowInserter(SlSet* set);
    ~SlRowInserter();

    tango::objhandle_t getHandle(const std::wstring& column_name);
    tango::IColumnInfoPtr getInfo(tango::objhandle_t column_handle);

    bool putRawPtr(tango::objhandle_t column_handle, const unsigned char* value, int length);
    bool putString(tango::objhandle_t column_handle, const std::string& value);
    bool putWideString(tango::objhandle_t column_handle, const std::wstring& value);
    bool putDouble(tango::objhandle_t column_handle, double value);
    bool putInteger(tango::objhandle_t column_handle, int value);
    bool putBoolean(tango::objhandle_t column_handle, bool value);
    bool putDateTime(tango::objhandle_t column_handle, tango::datetime_t datetime);
    bool putNull(tango::objhandle_t column_handle);

    bool startInsert(const std::wstring& col_list);
    bool insertRow();
    void finishInsert();

    bool flush();


private:

    SlSet* m_set;
    sqlite3_stmt* m_stmt;
    bool m_inserting;

    std::vector<SlRowInserterData> m_fields;
};




#endif



