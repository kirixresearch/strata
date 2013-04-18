/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2003-05-22
 *
 */


#ifndef __XDDRIZZLE_SET_H
#define __XDDRIZZLE_SET_H


#include <xcm/xcm.h>
#include "../xdcommon/cmnbaseset.h"


class DrizzleSet : public CommonBaseSet
{
friend class DrizzleDatabase;
friend class DrizzleIterator;
friend class DrizzleRowInserter;

    XCM_CLASS_NAME("xddrizzle.Set")
    XCM_BEGIN_INTERFACE_MAP(DrizzleSet)
        XCM_INTERFACE_ENTRY(tango::ISet)
    XCM_END_INTERFACE_MAP()

public:

    DrizzleSet();
    virtual ~DrizzleSet();
    
    bool init();
    
    std::wstring getSetId();

    tango::IStructurePtr getStructure();

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                      const std::wstring& expr,
                                      tango::IJob* job);

    tango::rowpos_t getRowCount();

private:

    tango::IDatabasePtr m_database;
    drizzle_st* m_drizzle;
    
    std::wstring m_tablename;
    xcm::mutex m_object_mutex;
};




class DrizzleInsertData
{
public:
    std::wstring m_col_name;
    int m_tango_type;
    int m_tango_width;
    int m_tango_scale;

    std::wstring m_text;
    bool m_specified;
};



class DrizzleRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xddrizzle.RowInserter")
    XCM_BEGIN_INTERFACE_MAP(DrizzleRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    DrizzleRowInserter(DrizzleSet* set);
    ~DrizzleRowInserter();

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

    void flushRow();

    DrizzleSet* m_set;
    drizzle_st* m_drizzle;
    drizzle_con_st* m_con;
    
    std::string m_asc_insert_stmt;
    std::wstring m_insert_stmt;
    std::wstring m_insert_stub;

    std::wstring m_quote_openchar;
    std::wstring m_quote_closechar;

    bool m_inserting;

    std::vector<DrizzleInsertData> m_insert_data;
};


#endif
