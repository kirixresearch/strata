/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2001-11-15
 *
 */


#ifndef H_APP_APPMAIN_H
#define H_APP_APPMAIN_H


#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


// version information

#include "appversion.h"

// c/c++ library includes
#include <cmath>
#include <clocale>
#include <string>
#include <vector>
#include <regex>

// wxWidgets includes

#include <wx/wx.h>
#include <wx/tokenzr.h>
#include <wx/datetime.h>
#include <wx/dnd.h>
#include <wx/dataobj.h>
#include <wx/clipbrd.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/sashwin.h>
#include <wx/cmdline.h>
#include <wx/combo.h>
#include <wx/bmpcbox.h>
#include <wx/config.h>
#include <wx/filename.h>
#include <wx/except.h>

#ifdef WIN32
#include <wx/msw/helpchm.h>
#include <wx/taskbar.h>
#else
#include <wx/html/helpctrl.h> 
#endif


// database/utility includes
#include <kl/xcm.h>
#include <kl/klib.h>
#include <kl/json.h>
#include <kl/thread.h>
#include <xd/xd.h>
#include "commands.h"

// framework/control includes
#include "../kcl/kcl.h"
#include "../jobs/jobs.h"

// application includes

#include "connectionmgr.h"
#include "jobqueue.h"
#include "framework.h"
#include "prefs.h"
#include "util.h"
#include "fspanel.h"
#include "statusbar.h"
#include "appconfig.h"
#include "appprefs.h"
#include "commandmgr.h"
#include "bitmapmgr.h"
#include "dociface.h"
#include "bookmarkfs.h"
#include "app.h"


#endif

