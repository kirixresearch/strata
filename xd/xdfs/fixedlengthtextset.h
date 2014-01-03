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
#include "../xdcommon/cmnbaseset.h"


class FixedLengthDefinition : public xd::IFixedLengthDefinition
{
    XCM_CLASS_NAME("xdfs.FixedLengthDefinition")
    XCM_BEGIN_INTERFACE_MAP(FixedLengthDefinition)
        XCM_INTERFACE_ENTRY(xd::IFixedLengthDefinition)
    XCM_END_INTERFACE_MAP()
    
public:

    enum FileType
    {
        FixedWidth = 100,
        LineDelimited = 101
    };

    enum CharacterEncoding
    {
        AsciiEncoding = 500,
        EbcdicEncoding = 501
    };
    
public:

    FixedLengthDefinition();
    
    virtual bool saveConfiguration() { return false; }
    virtual bool deleteConfiguration() { return false; }

    virtual xd::IStructurePtr getSourceStructure() { return m_source_structure; }
    virtual xd::IStructurePtr getDestinationStructure() { return m_dest_structure; }
    virtual xd::IStructurePtr getStructure() { return xcm::null; }
    
    virtual bool modifySourceStructure(xd::IStructure* struct_config, xd::IJob* job) { return false; }
    virtual bool modifyDestinationStructure(xd::IStructure* struct_config, xd::IJob* job) { return false; }
    virtual bool modifyStructure(xd::IStructure* struct_config, xd::IJob* job) { return false; }
        
    virtual void setBeginningSkipCharacterCount(size_t new_val) { m_skip_chars = new_val; }
    virtual size_t getBeginningSkipCharacterCount() { return m_skip_chars; }
    
    virtual void setRowWidth(size_t new_val) { m_row_width = new_val; }
    virtual size_t getRowWidth() { return m_row_width; }
    
    virtual void setLineDelimited(bool new_val) { m_file_type = new_val ? LineDelimited : FixedWidth; }
    virtual bool isLineDelimited() { return (m_file_type == LineDelimited) ? true : false; }
    
public:
    xd::IStructurePtr m_source_structure;
    xd::IStructurePtr m_dest_structure;
    std::wstring m_line_delimiters; // character array containing one or more line delimiters
    size_t m_row_width;             // user-specified width of each row
    int m_file_type;                // fixed or line-delimited (see fixedlengthtextset.h)
    size_t m_skip_chars;            // number of chars to skip at the beginning of the file
};


class FixedLengthTextSet : public CommonBaseSet,
                           public IXdfsSet,
                           public IXdsqlTable,
                           public xd::IFixedLengthDefinition
{
friend class FsDatabase;
friend class FixedLengthTextIterator;
friend class FixedLengthTextRowInserter;

    XCM_CLASS_NAME("xdfs.FixedLengthTextSet")
    XCM_BEGIN_INTERFACE_MAP(FixedLengthTextSet)
        XCM_INTERFACE_ENTRY(CommonBaseSet)
        XCM_INTERFACE_ENTRY(IXdfsSet)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
        XCM_INTERFACE_ENTRY(xd::IFixedLengthDefinition)
    XCM_END_INTERFACE_MAP()

public:

    FixedLengthTextSet(FsDatabase* db);
    ~FixedLengthTextSet();

    bool init(const std::wstring& filename);

    void setCreateStructure(xd::IStructurePtr structure);

    xd::IRowInserterPtr getRowInserter();
    IXdsqlRowDeleterPtr getRowDeleter() { return xcm::null; }

    xd::IIteratorPtr createIterator(const std::wstring& columns,
                                    const std::wstring& order,
                                    xd::IJob* job);

    xd::rowpos_t getRowCount();

    bool restoreDeleted() { return false; }

    bool updateRow(xd::rowid_t rowid,
                   xd::ColumnUpdateInfo* info,
                   size_t info_size) { return false; }

    // xd::IFixedLengthDefinition

    bool saveConfiguration();
    bool deleteConfiguration();
    
    xd::IStructurePtr getSourceStructure();
    xd::IStructurePtr getDestinationStructure();
    xd::IStructurePtr getStructure();
    
    bool modifySourceStructure(xd::IStructure* struct_config, xd::IJob* job);
    bool modifyDestinationStructure(xd::IStructure* struct_config, xd::IJob* job);
    bool modifyStructure(xd::IStructure* struct_config, xd::IJob* job);
        
    void setBeginningSkipCharacterCount(size_t new_val);
    size_t getBeginningSkipCharacterCount();
    
    void setRowWidth(size_t new_val);
    size_t getRowWidth();
    
    void setLineDelimited(bool new_val);
    bool isLineDelimited();

private:

    bool loadConfiguration();
    int guessFileType();

private:

    FsDatabase* m_database;
    std::wstring m_path;
    std::wstring m_configfile_path;

    FixedLengthDefinition* m_definition;
    
    xd::rowpos_t m_row_count;    // number of rows in the file
    xf_off_t m_file_size;           // size of the file
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



