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
                         public tango::IDelimitedTextSet
{
friend class FsDatabase;
friend class DelimitedTextIterator;
friend class DelimitedTextRowInserter;

    XCM_CLASS_NAME("xdfs.DelimitedTextSet")
    XCM_BEGIN_INTERFACE_MAP(DelimitedTextSet)
        XCM_INTERFACE_ENTRY(CommonBaseSet)
        XCM_INTERFACE_ENTRY(tango::IDelimitedTextSet)
    XCM_END_INTERFACE_MAP()

public:

    DelimitedTextSet();
    ~DelimitedTextSet();
    bool init(tango::IDatabasePtr db,
              const std::wstring& filename);

    void setCreateStructure(tango::IStructurePtr structure);
    
    // -- tango::ISet interface --

    unsigned int getSetFlags();
    std::wstring getSetId();

    tango::IRowInserterPtr getRowInserter();

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    tango::rowpos_t getRowCount();

    // -- tango::IDelimitedTextSet interface --

    bool saveConfiguration();
    bool deleteConfiguration();
    
    tango::IStructurePtr getSourceStructure();
    tango::IStructurePtr getDestinationStructure();
    tango::IStructurePtr getStructure();

    bool modifySourceStructure(tango::IStructure* struct_config, tango::IJob* job);
    bool modifyDestinationStructure(tango::IStructure* struct_config, tango::IJob* job);
    bool modifyStructure(tango::IStructure* struct_config, tango::IJob* job);
    
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
    
    tango::IIteratorPtr createSourceIterator(tango::IJob* job);
    
    bool determineColumns(int check_rows, tango::IJob* job);

private:

    bool loadConfigurationFromDataFile();
    bool loadConfigurationFromConfigFile();
    bool determineFirstRowHeader();
    void populateColumnNameMatchVector();
    void updateColumnNames();

private:

    tango::IDatabasePtr m_database;
    DelimitedTextFile m_file;
    std::wstring m_configfile_path;

    tango::IStructurePtr m_source_structure;
    tango::IStructurePtr m_dest_structure;
    
    // variables to describe the file structure
    std::wstring m_delimiters;
    std::wstring m_line_delimiters;
    std::wstring m_text_qualifier;
    bool m_first_row_column_names;
    bool m_discover_first_row_column_names;
    
    std::vector<ColumnNameMatch> m_colname_matches;
};


struct DelimitedTextDataAccessInfo;
class DelimitedTextRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdfs.DelimitedTextRowInserter")
    XCM_BEGIN_INTERFACE_MAP(DelimitedTextRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    DelimitedTextRowInserter(DelimitedTextSet* set);
    ~DelimitedTextRowInserter();

    tango::objhandle_t getHandle(const std::wstring& column_name);
    tango::IColumnInfoPtr getInfo(tango::objhandle_t column_handle);

    bool putRawPtr(tango::objhandle_t column_handle, const unsigned char* value, int length);
    bool putString(tango::objhandle_t column_handle, const std::string& value);
    bool putWideString(tango::objhandle_t column_handle, const std::wstring& value);
    bool putDouble(tango::objhandle_t column_handle, double value);
    bool putInteger(tango::objhandle_t column_handle, int value);
    bool putBoolean(tango::objhandle_t column_handle, bool value);
    bool putDateTime(tango::objhandle_t column_handle, tango::datetime_t value);
    bool putRowBuffer(const unsigned char* value);
    bool putNull(tango::objhandle_t column_handle);

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



