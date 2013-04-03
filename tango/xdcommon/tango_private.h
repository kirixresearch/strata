/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2009-01-15
 *
 */


#ifndef __XDCOMMON_TANGO_PRIVATE_H
#define __XDCOMMON_TANGO_PRIVATE_H


xcm_interface IIteratorKeyAccess;
XCM_DECLARE_SMARTPTR(IIteratorKeyAccess)

xcm_interface ISetRestoreDeleted;
XCM_DECLARE_SMARTPTR(ISetRestoreDeleted)

xcm_interface IXdsqlTable;
XCM_DECLARE_SMARTPTR(IXdsqlTable)


xcm_interface IIteratorKeyAccess : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IIteratorKeyAccess")

public:

    virtual void* getKey() = 0;
    virtual int getKeyLength() = 0;
    virtual bool setKeyFilter(const void* key, int len) = 0;
    virtual void getKeyFilter(const void** key, int* len) = 0;
    virtual bool setFirstKey() = 0;
};



xcm_interface ISetEvents : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.ISetEvents")

public:

    virtual void onSetDomainUpdated() = 0;   // (right now only used with BookmarkSet)
    virtual void onSetStructureUpdated() = 0;
    virtual void onSetRelationshipsUpdated() = 0;
    virtual void onSetRowUpdated(tango::rowid_t rowid) = 0;
    virtual void onSetRowDeleted(tango::rowid_t rowid) = 0;
};


xcm_interface ISetRestoreDeleted : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.ISetRestoreDeleted")

public:

    virtual bool restoreDeleted() = 0;
};


xcm_interface IXdsqlTable : public xcm::IObject
{
    XCM_INTERFACE_NAME("tango.IXdsqlTable")

public:

    virtual tango::IIteratorPtr createIterator(const std::wstring& columns,
                                               const std::wstring& expr,
                                               tango::IJob* job) = 0;
    virtual tango::IStructurePtr getStructure() = 0;
};


#endif

