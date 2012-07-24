/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <wx/wx.h>
#include <xcm/xcm.h>
#include <kl/klib.h>
#include "../kcl/kcl.h"

#include "prefs.h"
#include "framework.h"
#include "framework_private.h"
#include "prefs_private.h"
#include "jobqueue.h"
#include "jobqueue_private.h"
#include "fspanel.h"
#include "fspanel_private.h"
#include "fsitems_private.h"


XCM_BEGIN_STATIC_MODULE(cfw)
    XCM_BEGIN_CLASS_MAP()
        XCM_CLASS_ENTRY(cfw::MainFrame)
        XCM_CLASS_ENTRY(cfw::AppPreferences)
        XCM_CLASS_ENTRY(cfw::MemoryAppPreferences)
        XCM_CLASS_ENTRY(cfw::JobStat)
        XCM_CLASS_ENTRY(cfw::JobInfo)
        XCM_CLASS_ENTRY(cfw::JobQueue)
        XCM_CLASS_ENTRY(cfw::FsPanel)
        XCM_CLASS_ENTRY(cfw::DirectoryFsItem)
        XCM_CLASS_ENTRY(cfw::GenericFsItem)
    XCM_END_CLASS_MAP()
XCM_END_STATIC_MODULE(cfw)

