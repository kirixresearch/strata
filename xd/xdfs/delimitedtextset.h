/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-29
 *
 */


#ifndef __XDFS_DELIMITEDTEXTSET_H
#define __XDFS_DELIMITEDTEXTSET_H


#include "baseset.h"
#include "../xdcommon/util.h"
#include "delimitedtext.h"


bool parseDelimitedStringDate(const std::wstring& str,
                              int* year = NULL,
                              int* month = NULL,
                              int* day = NULL);


class DelimitedTextSet : public XdfsBaseSet,
                         public IXdfsSet,
                         public IXdsqlTable
{
friend class FsDatabase;
friend class DelimitedTextIterator;
friend class DelimitedTextRowInserter;

    XCM_CLASS_NAME("xdfs.DelimitedTextSet")
    XCM_BEGIN_INTERFACE_MAP(DelimitedTextSet)
        XCM_INTERFACE_ENTRY(XdfsBaseSet)
        XCM_INTERFACE_ENTRY(IXdfsSet)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
    XCM_END_INTERFACE_MAP()

public:

    DelimitedTextSet(FsDatabase* database);
    ~DelimitedTextSet();

    bool init(const std::wstring& filename, const xd::FormatDefinition& def);
    
    std::wstring getSetId();

    xd::IRowInserterPtr getRowInserter();
    IXdsqlRowDeleterPtr getRowDeleter() { return xcm::null; }
    bool getFormatDefinition(xd::FormatDefinition* def);

    xd::IIteratorPtr createIterator(const std::wstring& columns,
                                    const std::wstring& expr,
                                    xd::IJob* job);

    xd::rowpos_t getRowCount();

    bool restoreDeleted() { return false; }

    bool updateRow(xd::rowid_t rowid,
                   xd::ColumnUpdateInfo* info,
                   size_t info_size) { return false; }

    xd::IStructurePtr getStructure();

    bool modifyStructure(xd::IStructure* struct_config, xd::IJob* job);
    bool determineColumns(int check_rows, int max_seconds, xd::IJob* job);
    bool modifyStructure(xd::IStructurePtr structure, xd::IJob* job) { return false; }
 
private:

    bool loadConfigurationFromDataFile();
    bool determineIfFirstRowIsHeader();
    void populateColumnNameMatchVector();

private:

    DelimitedTextFile m_file;
    std::wstring m_path;
    xd::FormatDefinition m_def;
    
};


struct DelimitedTextDataAccessInfo;
class DelimitedTextRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdfs.DelimitedTextRowInserter")
    XCM_BEGIN_INTERFACE_MAP(DelimitedTextRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    DelimitedTextRowInserter(DelimitedTextSet* set);
    ~DelimitedTextRowInserter();

    xd::objhandle_t getHandle(const std::wstring& column_name);
    xd::IColumnInfoPtr getInfo(xd::objhandle_t column_handle);

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

    DelimitedTextSet* m_set;
    DelimitedTextFile* m_file;  // pointer to m_set's DelimitedTextFile;

    std::vector<DelimitedTextDataAccessInfo*> m_fields;
    bool m_inserting;

    DelimitedTextRow m_row;
};

#endif
