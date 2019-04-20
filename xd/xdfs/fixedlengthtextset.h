/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2005-05-12
 *
 */


#ifndef __XDFS_FIXEDLENGTHTEXTSET_H
#define __XDFS_FIXEDLENGTHTEXTSET_H


#include <kl/file.h>
#include "baseset.h"
#include "fixedlength.h"

class FixedLengthTextSet : public XdfsBaseSet,
                           public IXdfsSet,
                           public IXdsqlTable
{
friend class FsDatabase;
friend class FixedLengthTextIterator;
friend class FixedLengthTextRowInserter;

    XCM_CLASS_NAME("xdfs.FixedLengthTextSet")
    XCM_BEGIN_INTERFACE_MAP(FixedLengthTextSet)
        XCM_INTERFACE_ENTRY(XdfsBaseSet)
        XCM_INTERFACE_ENTRY(IXdfsSet)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
    XCM_END_INTERFACE_MAP()

public:

    FixedLengthTextSet(FsDatabase* db);
    ~FixedLengthTextSet();

    bool init(const std::wstring& filename, const xd::FormatDefinition& def);

    xd::IRowInserterPtr getRowInserter();
    IXdsqlRowDeleterPtr getRowDeleter() { return xcm::null; }
    bool getFormatDefinition(xd::FormatDefinition* def);

    xd::IIteratorPtr createIterator(const std::wstring& columns,
                                    const std::wstring& order,
                                    xd::IJob* job);

    xd::rowpos_t getRowCount();

    bool restoreDeleted() { return false; }

    bool updateRow(xd::rowid_t rowid,
                   xd::ColumnUpdateInfo* info,
                   size_t info_size) { return false; }

    xd::Structure getStructure();
    xd::Structure getStructureWithTransformations();
   
    bool modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job) { return false; }

    void setTemporary(bool value) { m_temporary = value; }

private:

    FixedLengthTable m_file;
    std::wstring m_path;
    xd::FormatDefinition m_def;
    
    xd::rowpos_t m_row_count;    // number of rows in the file
    xf_off_t m_file_size;        // size of the file
    bool m_temporary;
};




struct FixedLengthTextInsertData
{
    std::string m_colname;
    int m_type;
    int m_width;
    int m_scale;
    int m_str_width; // for date, int, double, etc. -- e.g. "01/01/2000" has
                     // a string width of 10, not 4)
    bool m_align_right;
    
    std::string m_str_val;
};

class FixedLengthTextRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdfs.FixedLengthTextRowInserter")
    XCM_BEGIN_INTERFACE_MAP(FixedLengthTextRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    FixedLengthTextRowInserter(FixedLengthTextSet* set);
    ~FixedLengthTextRowInserter();

    xd::objhandle_t getHandle(const std::wstring& column_name);

    bool putRawPtr(xd::objhandle_t column_handle, const unsigned char* value, int length);
    bool putString(xd::objhandle_t column_handle, const std::string& value);
    bool putWideString(xd::objhandle_t column_handle, const std::wstring& value);
    bool putDouble(xd::objhandle_t column_handle, double value);
    bool putInteger(xd::objhandle_t column_handle, int value);
    bool putBoolean(xd::objhandle_t column_handle, bool value);
    bool putDateTime(xd::objhandle_t column_handle, xd::datetime_t value);
    bool putNull(xd::objhandle_t column_handle);

    bool startInsert(const std::wstring& col_list);
    bool insertRow();
    void finishInsert();

    bool flush();

private:

    FixedLengthTextSet* m_set;
    xf_file_t m_file;

    char* m_buf;                    // buffer for rows
    char* m_buf_ptr;
    int m_row_width;
    xf_off_t m_file_pos;
    bool m_crlf;

    bool m_inserting;
    std::vector<FixedLengthTextInsertData*> m_insert_data;
};




#endif



