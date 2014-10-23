/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   David Z. Williams
 * Created:  2003-12-28
 *
 */


#ifndef __XDFS_XBASESET_H
#define __XDFS_XBASESET_H


#include "baseset.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "xbase.h"


// utility functions

inline char xd2xbaseType(int xd_type)
{
    switch (xd_type)
    {
        case xd::typeCharacter:      return 'C';
        case xd::typeWideCharacter:  return 'C';
        case xd::typeNumeric:        return 'N';
        case xd::typeDouble:         return 'B';
        case xd::typeDate:           return 'D';
        case xd::typeDateTime:       return 'T';
        case xd::typeBoolean:        return 'L';
        case xd::typeInteger:        return 'I';
    }

    return ' ';
}

inline int xbase2xdType(unsigned char xbase_type)
{
    switch (xbase_type)
    {
        case 'C':   return xd::typeCharacter;
        case 'N':   return xd::typeNumeric;
        case 'Y':   return xd::typeNumeric;
        case 'F':   return xd::typeDouble;
        case 'B':   return xd::typeDouble;
        case 'D':   return xd::typeDate;
        case 'T':   return xd::typeDateTime;
        case 'L':   return xd::typeBoolean;
        case 'I':   return xd::typeInteger;
    }

    return xd::typeInvalid;
}




class XbaseSet : public XdfsBaseSet,
                 public IXdfsSet,
                 public IXdsqlTable
{
friend class FsDatabase;
friend class XbaseIterator;
friend class XbaseRowInserter;

    XCM_CLASS_NAME("xdfs.XbaseSet")
    XCM_BEGIN_INTERFACE_MAP(XbaseSet)
        XCM_INTERFACE_ENTRY(XdfsBaseSet)
        XCM_INTERFACE_ENTRY(IXdfsSet)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
    XCM_END_INTERFACE_MAP()

public:

    XbaseSet(FsDatabase* db);
    ~XbaseSet();

    bool init(const std::wstring& filename);

    std::wstring getSetId();

    xd::Structure getStructure();

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
                   size_t info_size);

    bool modifyStructure(const xd::StructureModify& mod_params, xd::IJob* job);
 
private:

    XbaseFile m_file;
    std::wstring m_filename;
};



class XbaseRowInserter : public xd::IRowInserter
{
    XCM_CLASS_NAME("xdfs.XbaseRowInserter")
    XCM_BEGIN_INTERFACE_MAP(XbaseRowInserter)
        XCM_INTERFACE_ENTRY(xd::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    XbaseRowInserter(XbaseSet* set);
    ~XbaseRowInserter();

    xd::objhandle_t getHandle(const std::wstring& column_name);

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
    XbaseSet* m_set;
    XbaseFile* m_file;  // pointer to m_set's XbaseFile;
    
    std::vector<XbaseField*> m_fields;
    bool m_inserting;
};




#endif

