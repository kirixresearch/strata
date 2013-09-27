/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2003-09-25
 *
 */


#include <xcm/xcm.h>
#include "dbmgr.h"
#include "../xdcommon/connectionstr.h"


// these are the publicly creatable classes

XCM_BEGIN_DYNAMIC_MODULE(xdnative)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(DatabaseMgr)
    XCM_END_CLASS_MAP()
XCM_END_DYNAMIC_MODULE(xdnative)

