/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2009-01-15
 *
 */


#ifndef __XDCOMMON_XD_PRIVATE_H
#define __XDCOMMON_XD_PRIVATE_H


namespace xd
{

xcm_interface IIteratorKeyAccess;
XCM_DECLARE_SMARTPTR(IIteratorKeyAccess)

xcm_interface IIteratorKeyAccess : public xcm::IObject
{
    XCM_INTERFACE_NAME("xd.IIteratorKeyAccess")

public:

    virtual void* getKey() = 0;
    virtual int getKeyLength() = 0;
    virtual bool setKeyFilter(const void* key, int len) = 0;
    virtual void getKeyFilter(const void** key, int* len) = 0;
    virtual bool setFirstKey() = 0;
};



}; // namespace xd



#endif

