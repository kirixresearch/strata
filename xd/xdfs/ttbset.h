/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#ifndef __XDFS_TTBSET_H
#define __XDFS_TTBSET_H


#include "../xdcommon/cmnbaseset.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "ttbfile.h"

inline xd::rowid_t rowidCreate(xd::tableord_t table_ordinal,
                                  xd::rowpos_t row_num)
{
    xd::rowid_t r;
    r = ((xd::rowid_t)table_ordinal) << 36;
    r |= row_num;
    return r;
}

inline xd::rowpos_t rowidGetRowPos(xd::rowid_t rowid)
{
    return (rowid & 0xfffffffffLL);
}

inline xd::tableord_t rowidGetTableOrd(xd::rowid_t rowid)
{
    return (rowid >> 36);
}


class TtbSet : public CommonBaseSet,
               public IXdfsSet,
               public IXdsqlTable
{
friend class FsDatabase;
friend class TtbRowInserter;

    XCM_CLASS_NAME("xdfs.TtbSet")
    XCM_BEGIN_INTERFACE_MAP(TtbSet)
        XCM_INTERFACE_ENTRY(CommonBaseSet)
        XCM_INTERFACE_ENTRY(IXdfsSet)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
    XCM_END_INTERFACE_MAP()

public:

    TtbSet(FsDatabase* db);
    ~TtbSet();

    bool init(const std::wstring& filename);

    xd::IStructurePtr getStructure();
    bool modifyStructure(xd::IStructure* struct_config, 
                         xd::IJob* job);

    xd::IRowInserterPtr getRowInserter();
    IXdsqlRowDeleterPtr getRowDeleter() { return xcm::null; }

    xd::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       xd::IJob* job);

    bool restoreDeleted() { return false; }

    bool updateRow(xd::rowid_t rowid,
                   xd::ColumnUpdateInfo* info,
                   size_t info_size);

private:

    FsDatabase* m_database;
    TtbTable m_file;
    unsigned char* m_update_buf;
    TtbRow m_update_row;
};



class TtbInsertData
{
public:
    
    std::wstring name;
    int ordinal;
    int type;
    int width;
    int scale;
    int offset;
    bool nulls_allowed;

    xd::IColumnInfoPtr col;
};



class TtbRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdfs.TtbRowInserter")
    XCM_BEGIN_INTERFACE_MAP(TtbRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    TtbRowInserter(TtbSet* set);
    ~TtbRowInserter();

    xd::objhandle_t getHandle(const std::wstring& column_name);
    xd::IColumnInfoPtr getInfo(xd::objhandle_t column_handle);

    bool putRawPtr(xd::objhandle_t column_handle,
                   const unsigned char* value,
                   int length);

    bool putString(xd::objhandle_t column_handle,
                   const std::string& value);

    bool putWideString(xd::objhandle_t column_handle,
                       const std::wstring& value);

    bool putDouble(xd::objhandle_t column_handle,
                   double value);

    bool putInteger(xd::objhandle_t column_handle,
                    int value);

    bool putBoolean(xd::objhandle_t column_handle,
                    bool value);

    bool putDateTime(xd::objhandle_t column_handle,
                     xd::datetime_t value);

    bool putNull(xd::objhandle_t column_handle);

    bool startInsert(const std::wstring& col_list);
    bool insertRow();
    void finishInsert();

    bool flush();

private:

    TtbSet* m_set;
    TtbTable* m_file;  // pointer to m_set's TtbTable;
    TtbRow m_row;

    int m_row_width;
    int m_buf_row;
    std::wstring m_tbl_filename;
    unsigned char* m_buf;
    
    bool m_inserting;

    std::vector<TtbInsertData*> m_fields;
};



#endif
