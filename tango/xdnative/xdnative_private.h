/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2002-04-29
 *
 */


#ifndef __XDNATIVE_XDNATIVE_PRIVATE_H
#define __XDNATIVE_XDNATIVE_PRIVATE_H


#include <kl/string.h>
#include "../xdcommon/jobinfo.h"
#include "../xdcommon/tango_private.h"
#include "../xdcommonsql/xdcommonsql.h"


// aggregate function types

enum
{
    GroupFunc_None = 0,
    GroupFunc_First = 1,
    GroupFunc_Last = 2,
    GroupFunc_Min = 3,
    GroupFunc_Max = 4,
    GroupFunc_Sum = 5,
    GroupFunc_Avg = 6,
    GroupFunc_Count = 7,
    GroupFunc_MaxDistance = 8,
    GroupFunc_Stddev = 9,
    GroupFunc_Variance = 10,
    GroupFunc_Merge = 11,
    GroupFunc_GroupID = 12
};


// rowid creation/extraction functions

inline tango::rowid_t rowidCreate(tango::tableord_t table_ordinal,
                                  tango::rowpos_t row_num)
{
    tango::rowid_t r;
    r = ((tango::rowid_t)table_ordinal) << 36;
    r |= row_num;
    return r;
}

inline tango::rowpos_t rowidGetRowPos(tango::rowid_t rowid)
{
    #ifdef _MSC_VER
    return (rowid & 0xfffffffff);
    #else
    return (rowid & 0xfffffffffLL);
    #endif
}

inline tango::tableord_t rowidGetTableOrd(tango::rowid_t rowid)
{
    return (rowid >> 36);
}


xcm_interface ITable;
xcm_interface IRelationInternal;
xcm_interface IXdnativeSet;
xcm_interface IXdnativeSetEvents;
xcm_interface INodeValue;
xcm_interface IIndex;
xcm_interface IIndexIterator;

class BaseIterator;
class BaseSet;
class OfsFile;


XCM_DECLARE_SMARTPTR(ITable)
XCM_DECLARE_SMARTPTR(IRelationInternal)
XCM_DECLARE_SMARTPTR(IXdnativeSet)
XCM_DECLARE_SMARTPTR(IXdnativeSetEvents)
XCM_DECLARE_SMARTPTR(INodeValue)


xcm_interface INodeValue : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdnative.INodeValue")

public:

    virtual bool write() = 0;

    virtual const std::wstring& getName() = 0;
    virtual int getType() = 0;

    virtual const std::wstring& getString() = 0;
    virtual void setString(const std::wstring& value) = 0;
    virtual double getDouble() = 0;
    virtual void setDouble(double value) = 0;
    virtual int getInteger() = 0;
    virtual void setInteger(int value) = 0;
    virtual bool getBoolean() = 0;
    virtual void setBoolean(bool value) = 0;

    virtual unsigned int getChildCount() = 0;
    virtual const std::wstring& getChildName(unsigned int idx) = 0;
    virtual INodeValuePtr getChildByIdx(unsigned int idx) = 0;
    virtual INodeValuePtr getChild(const std::wstring& name,
                                  bool create_if_not_exist) = 0;
    virtual INodeValuePtr createChild(const std::wstring& name) = 0;
    virtual bool getChildExist(const std::wstring& name) = 0;
    virtual bool renameChild(const std::wstring& name,
                             const std::wstring& new_name) = 0;
    virtual bool deleteChild(const std::wstring& name) = 0;
    virtual bool deleteAllChildren() = 0;
};


xcm_interface IRelationInternal : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdnative.IRelationInternal")

public:

    virtual void setLeftSetId(const std::wstring& new_val) = 0;
    virtual std::wstring getLeftSetId() = 0;

    virtual void setRightSetId(const std::wstring& new_val) = 0;
    virtual std::wstring getRightSetId() = 0;
};



xcm_interface ITableEvents : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdnative.ITableEvents")

public:

    virtual void onTableRowUpdated(tango::rowid_t rowid) = 0;
    virtual void onTableRowDeleted(tango::rowid_t rowid) = 0;
    virtual void onTableRowCountUpdated() = 0;
};


xcm_interface ITable : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdnative.ITable")

public:

    virtual bool addEventHandler(ITableEvents* handler) = 0;
    virtual bool removeEventHandler(ITableEvents* handler) = 0;

    virtual std::wstring getFilename() = 0;
    virtual std::wstring getMapFilename() = 0;

    virtual bool open(const std::wstring& filename,
                      tango::tableord_t ordinal) = 0;

    virtual bool reopen(bool exclusive) = 0;

    virtual void close() = 0;

    virtual tango::IStructurePtr getStructure() = 0;
    virtual tango::tableord_t getTableOrdinal() = 0;
    virtual tango::rowpos_t getRowCount(tango::rowpos_t* deleted_row_cnt) = 0;

    virtual tango::IRowDeleterPtr getRowDeleter() = 0;
    virtual bool isRowDeleted(tango::rowpos_t row) = 0;
    virtual bool restoreDeleted() = 0;

    virtual int getRowWidth() = 0;

    virtual int getRows(unsigned char* buf,
                        tango::rowpos_t* rowpos_arr,
                        int skip,
                        tango::rowpos_t start_row,
                        int row_count,
                        bool direction,
                        bool include_deleted) = 0;

    virtual bool getRow(tango::rowpos_t row,
                        unsigned char* buf) = 0;

    virtual bool writeRow(tango::rowpos_t row,
                          unsigned char* buf) = 0;

    virtual bool writeColumnInfo(int col,
                                 const std::wstring& col_name,
                                 int type,
                                 int width,
                                 int scale) = 0;

    virtual int appendRows(unsigned char* buf,
                           int row_count) = 0;

    virtual unsigned long long getStructureModifyTime() = 0;
    virtual bool setStructureModified() = 0;
};




xcm_interface IXdnativeSet : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdnative.IXdnativeSet")

public:

    virtual std::wstring getSetId() = 0;
    virtual IXdnativeSet* getRawXdnativeSetPtr() = 0;

    virtual void setObjectPath(const std::wstring& path) = 0;
    virtual std::wstring getObjectPath() = 0;

    virtual tango::rowpos_t getRowCount() = 0;
    virtual tango::IStructurePtr getStructure() = 0;

    virtual bool addEventHandler(IXdnativeSetEvents* handler) = 0;
    virtual bool removeEventHandler(IXdnativeSetEvents* handler) = 0;

    virtual void appendCalcFields(tango::IStructure* s) = 0;
    virtual void onOfsPathChanged(const std::wstring& new_path) = 0;
    virtual void onRelationshipsUpdated() = 0;
    
    virtual tango::IIteratorPtr createIterator(const std::wstring& columns,
                                               const std::wstring& order,
                                               tango::IJob* job) = 0;

    virtual bool modifyStructure(tango::IStructure* struct_config, tango::IJob* job) = 0;
    virtual tango::IRowInserterPtr getRowInserter() = 0;

    virtual tango::IIndexInfoEnumPtr getIndexEnum() = 0;
    virtual tango::IIndexInfoPtr createIndex(const std::wstring& name,
                                             const std::wstring& expr,
                                             tango::IJob* job) = 0;
    virtual bool renameIndex(const std::wstring& name,
                             const std::wstring& new_name) = 0;
    virtual  bool deleteIndex(const std::wstring& name) = 0;
};




xcm_interface IXdnativeSetEvents : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdnative.IXdnativeSetEvents")

public:

    virtual void onSetDomainUpdated() = 0;   // (right now only used with BookmarkSet)
    virtual void onSetStructureUpdated() = 0;
    virtual void onSetRelationshipsUpdated() = 0;
    virtual void onSetRowUpdated(tango::rowid_t rowid) = 0;
    virtual void onSetRowDeleted(tango::rowid_t rowid) = 0;
};



#endif

