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


#include "../xdcommon/cmnbaseset.h"
#include "../xdcommon/util.h"
#include "delimitedtext.h"


bool parseDelimitedStringDate(const std::wstring& str,
                              int* year = NULL,
                              int* month = NULL,
                              int* day = NULL);


// This class is used to handle renames.  The following scenario caused problems:
// fields DAVE,BEN,FIELD1 - DAVE would get renamed to Field1, and Field1 would
// get renamed to Field3, but so would the first column too, due to naming conflict.
// This class allows for an intermediary -- a third match string which will become
// the temporary column name to ensure there are no naming conflicts.

class ColumnNameMatch
{
public:

    ColumnNameMatch()
    {
        user_name = L"";
        file_name = L"";
        match_name = getUniqueString();
    }
    
    ColumnNameMatch(const ColumnNameMatch& c)
    {
        user_name = c.user_name;
        file_name = c.file_name;
        match_name = c.match_name;
    }

    ColumnNameMatch& operator=(const ColumnNameMatch& c)
    {
        user_name = c.user_name;
        file_name = c.file_name;
        match_name = c.match_name;
        return *this;
    }
    
public:

    std::wstring user_name;
    std::wstring file_name;
    std::wstring match_name;
};


class DelimitedTextSet : public CommonBaseSet,
                         public IXdfsSet,
                         public IXdsqlTable,
                         public xd::IDelimitedTextSet
{
friend class FsDatabase;
friend class DelimitedTextIterator;
friend class DelimitedTextRowInserter;

    XCM_CLASS_NAME("xdfs.DelimitedTextSet")
    XCM_BEGIN_INTERFACE_MAP(DelimitedTextSet)
        XCM_INTERFACE_ENTRY(CommonBaseSet)
        XCM_INTERFACE_ENTRY(IXdfsSet)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
        XCM_INTERFACE_ENTRY(xd::IDelimitedTextSet)
    XCM_END_INTERFACE_MAP()

public:

    DelimitedTextSet(FsDatabase* database);
    ~DelimitedTextSet();

    bool init(const std::wstring& filename);

    void setCreateStructure(xd::IStructurePtr structure);
    
    std::wstring getSetId();

    xd::IRowInserterPtr getRowInserter();
    IXdsqlRowDeleterPtr getRowDeleter() { return xcm::null; }

    xd::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       xd::IJob* job);

    xd::rowpos_t getRowCount();

    bool restoreDeleted() { return false; }

    bool updateRow(xd::rowid_t rowid,
                   xd::ColumnUpdateInfo* info,
                   size_t info_size) { return false; }

    // xd::IDelimitedTextSet

    bool saveConfiguration();
    bool deleteConfiguration();
    
    xd::IStructurePtr getSourceStructure();
    xd::IStructurePtr getDestinationStructure();
    xd::IStructurePtr getStructure();

    bool modifySourceStructure(xd::IStructure* struct_config, xd::IJob* job);
    bool modifyDestinationStructure(xd::IStructure* struct_config, xd::IJob* job);
    bool modifyStructure(xd::IStructure* struct_config, xd::IJob* job);
    
    // this function should only be used by outside callers as it
    // also modifies values in the m_colname_matches vector
    bool renameSourceColumn(const std::wstring& source_col,
                            const std::wstring& new_val);
    
    void setDelimiters(const std::wstring& new_val,
                       bool refresh_structure);
    std::wstring getDelimiters();
    
    void setLineDelimiters(const std::wstring& new_val,
                           bool refresh_structure);
    std::wstring getLineDelimiters();
    
    void setTextQualifier(const std::wstring& new_val,
                          bool refresh_structure);
    std::wstring getTextQualifier();
    
    void setDiscoverFirstRowColumnNames(bool new_val);
    void setFirstRowColumnNames(bool new_val);
    bool isFirstRowColumnNames();
    
    xd::IIteratorPtr createSourceIterator(xd::IJob* job);
    
    bool determineColumns(int check_rows, xd::IJob* job);

private:

    bool loadConfigurationFromDataFile();
    bool loadConfigurationFromConfigFile();
    bool determineFirstRowHeader();
    void populateColumnNameMatchVector();
    void updateColumnNames();

private:

    FsDatabase* m_database;
    DelimitedTextFile m_file;
    std::wstring m_configfile_path;

    xd::IStructurePtr m_source_structure;
    xd::IStructurePtr m_dest_structure;
    
    // variables to describe the file structure
    std::wstring m_delimiters;
    std::wstring m_line_delimiters;
    std::wstring m_text_qualifier;
    bool m_first_row_column_names;
    bool m_discover_first_row_column_names;
    
    std::vector<ColumnNameMatch> m_colname_matches;
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
    bool putRowBuffer(const unsigned char* value);
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



