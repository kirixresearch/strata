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


#include "../xdcommon/cmnbaseset.h"
#include "../xdcommonsql/xdcommonsql.h"
#include "xbase.h"


// utility functions

inline char tango2xbaseType(int tango_type)
{
    switch (tango_type)
    {
        case tango::typeCharacter:      return 'C';
        case tango::typeWideCharacter:  return 'C';
        case tango::typeNumeric:        return 'N';
        case tango::typeDouble:         return 'B';
        case tango::typeDate:           return 'D';
        case tango::typeDateTime:       return 'T';
        case tango::typeBoolean:        return 'L';
        case tango::typeInteger:        return 'I';
    }

    return ' ';
}

inline int xbase2tangoType(unsigned char xbase_type)
{
    switch (xbase_type)
    {
        case 'C':   return tango::typeCharacter;
        case 'N':   return tango::typeNumeric;
        case 'Y':   return tango::typeNumeric;
        case 'F':   return tango::typeDouble;
        case 'B':   return tango::typeDouble;
        case 'D':   return tango::typeDate;
        case 'T':   return tango::typeDateTime;
        case 'L':   return tango::typeBoolean;
        case 'I':   return tango::typeInteger;
    }

    return tango::typeInvalid;
}




class XbaseSet : public CommonBaseSet,
                 public IXdfsSet,
                 public IXdsqlTable
{
friend class FsDatabase;
friend class XbaseRowInserter;

    XCM_CLASS_NAME("xdfs.XbaseSet")
    XCM_BEGIN_INTERFACE_MAP(XbaseSet)
        XCM_INTERFACE_ENTRY(CommonBaseSet)
        XCM_INTERFACE_ENTRY(IXdfsSet)
        XCM_INTERFACE_ENTRY(IXdsqlTable)
    XCM_END_INTERFACE_MAP()

public:

    XbaseSet();
    ~XbaseSet();

    bool init(tango::IDatabasePtr db, const std::wstring& filename);

    std::wstring getSetId();

    tango::IStructurePtr getStructure();
    bool modifyStructure(tango::IStructure* struct_config, 
                         tango::IJob* job);

    tango::IRowInserterPtr getRowInserter();

    tango::IIteratorPtr createIterator(const std::wstring& columns,
                                       const std::wstring& expr,
                                       tango::IJob* job);

    tango::rowpos_t getRowCount();

    bool restoreDeleted() { return false; }

    bool updateRow(tango::rowid_t rowid,
                   tango::ColumnUpdateInfo* info,
                   size_t info_size);

private:

    tango::IDatabasePtr m_database;
    XbaseFile m_file;
};



class XbaseRowInserter : public tango::IRowInserter
{
    XCM_CLASS_NAME("xdfs.XbaseRowInserter")
    XCM_BEGIN_INTERFACE_MAP(XbaseRowInserter)
        XCM_INTERFACE_ENTRY(tango::IRowInserter)
    XCM_END_INTERFACE_MAP()


public:

    XbaseRowInserter(XbaseSet* set);
    ~XbaseRowInserter();

    tango::objhandle_t getHandle(const std::wstring& column_name);
    tango::IColumnInfoPtr getInfo(tango::objhandle_t column_handle);

    bool putRawPtr(tango::objhandle_t column_handle,
                   const unsigned char* value,
                   int length);

    bool putString(tango::objhandle_t column_handle,
                   const std::string& value);

    bool putWideString(tango::objhandle_t column_handle,
                       const std::wstring& value);

    bool putDouble(tango::objhandle_t column_handle,
                   double value);

    bool putInteger(tango::objhandle_t column_handle,
                    int value);

    bool putBoolean(tango::objhandle_t column_handle,
                    bool value);

    bool putDateTime(tango::objhandle_t column_handle,
                     tango::datetime_t value);

    bool putRowBuffer(const unsigned char* value);
    bool putNull(tango::objhandle_t column_handle);

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

