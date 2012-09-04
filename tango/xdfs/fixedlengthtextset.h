/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
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


class FixedLengthDefinition : public tango::IFixedLengthDefinition
{
    XCM_CLASS_NAME("xdfs.FixedLengthDefinition")
    XCM_BEGIN_INTERFACE_MAP(FixedLengthDefinition)
        XCM_INTERFACE_ENTRY(tango::IFixedLengthDefinition)
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

    virtual tango::IStructurePtr getSourceStructure() { return m_source_structure; }
    virtual tango::IStructurePtr getDestinationStructure() { return m_dest_structure; }
    virtual tango::IStructurePtr getStructure() { return xcm::null; }
    
    virtual bool modifySourceStructure(tango::IStructure* struct_config, tango::IJob* job) { return false; }
    virtual bool modifyDestinationStructure(tango::IStructure* struct_config, tango::IJob* job) { return false; }
    virtual bool modifyStructure(tango::IStructure* struct_config, tango::IJob* job) { return false; }
        
    virtual void setBeginningSkipCharacterCount(size_t new_val) { m_skip_chars = new_val; }
    virtual size_t getBeginningSkipCharacterCount() { return m_skip_chars; }
    
    virtual void setRowWidth(size_t new_val) { m_row_width = new_val; }
    virtual size_t getRowWidth() { return m_row_width; }
    
    virtual void setLineDelimited(bool new_val) { m_file_type = new_val ? LineDelimited : FixedWidth; }
    virtual bool isLineDelimited() { return (m_file_type == LineDelimited) ? true : false; }
    
public:
    tango::IStructurePtr m_source_structure;
    tango::IStructurePtr m_dest_structure;
    std::wstring m_line_delimiters; // character array containing one or more line delimiters
    size_t m_row_width;             // user-specified width of each row
    int m_file_type;                // fixed or line-delimited (see fixedlengthtextset.h)
    size_t m_skip_chars;            // number of chars to skip at the beginning of the file
};


class FixedLengthTextSet : public CommonBaseSet,
                           public tango::IFixedLengthDefinition
{
friend class FsDatabase;
friend class FixedLengthTextIterator;
friend class FixedLengthTextRowInserter;

    XCM_CLASS_NAME("xdfs.FixedLengthTextSet")
    XCM_BEGIN_INTERFACE_MAP(FixedLengthTextSet)
        XCM_INTERFACE_ENTRY(CommonBaseSet)
        XCM_INTERFACE_ENTRY(tango::IFixedLengthDefinition)
    XCM_END_INTERFACE_MAP()

public:

    FixedLengthTextSet();
    ~FixedLengthTextSet();
    bool init(tango::IDatabasePtr db,
              const std::wstring& filename);

    void setCreateStructure(tango::IStructurePtr structure);

    // -- tango::ISet interface --

    unsigned int getSetFlags();
    std::wstring getSetId();

    tango::IRowInserterPtr getRowInserter();
    tango::IRowDeleterPtr getRowDeleter();

    int insert(tango::IIteratorPtr source_iter,
               const std::wstring& where_condition,
               int max_rows,
               tango::IJob* job);

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    tango::rowpos_t getRowCount();

    // -- tango::IFixedLengthDefinition interface --

    bool saveConfiguration();
    bool deleteConfiguration();
    
    tango::IStructurePtr getSourceStructure();
    tango::IStructurePtr getDestinationStructure();
    tango::IStructurePtr getStructure();
    
    bool modifySourceStructure(tango::IStructure* struct_config, tango::IJob* job);
    bool modifyDestinationStructure(tango::IStructure* struct_config, tango::IJob* job);
    bool modifyStructure(tango::IStructure* struct_config, tango::IJob* job);
        
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

    tango::IDatabasePtr m_database;
    std::wstring m_path;
    std::wstring m_configfile_path;

    FixedLengthDefinition* m_definition;
    
    tango::rowpos_t m_row_count;    // number of rows in the file
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

class FixedLengthTextRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdfs.FixedLengthTextRowInserter")
    XCM_BEGIN_INTERFACE_MAP(FixedLengthTextRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    FixedLengthTextRowInserter(FixedLengthTextSet* set);
    ~FixedLengthTextRowInserter();

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



