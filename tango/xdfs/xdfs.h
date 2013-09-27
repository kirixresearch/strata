/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-04-19
 *
 */


#ifndef __XDFS_XDFS_H
#define __XDFS_XDFS_H

#include "../xdcommonsql/xdcommonsql.h"

xcm_interface IXdfsSet;
XCM_DECLARE_SMARTPTR(IXdfsSet)


xcm_interface IXdfsSet : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdfs.IXdfsSet")

public:

    virtual xd::IRowInserterPtr getRowInserter() = 0;

};



#endif

