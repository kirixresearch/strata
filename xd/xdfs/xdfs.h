/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2013-04-19
 *
 */


#ifndef H_XDFS_XDFS_H
#define H_XDFS_XDFS_H

#include <kl/string.h>
#include <kl/file.h>
#include <xd/xd.h>
#include "../xdcommonsql/xdcommonsql.h"
#include "../xdcommon/errorinfo.h"


xcm_interface IXdfsSet;
XCM_DECLARE_SMARTPTR(IXdfsSet)


xcm_interface IXdfsSet : public xcm::IObject
{
    XCM_INTERFACE_NAME("xdfs.IXdfsSet")

public:

    virtual xd::IRowInserterPtr getRowInserter() = 0;
    virtual bool getFormatDefinition(xd::FormatDefinition* def) = 0;
};



#endif

