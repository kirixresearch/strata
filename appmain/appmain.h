/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2001-11-15
 *
 */


#ifndef __APP_APPMAIN_H
#define __APP_APPMAIN_H


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#pragma warning(disable : 4996)
#endif


// version information

#include "appversion.h"

// c/c++ library includes
#include <cmath>
#include <clocale>
#include <string>
#include <vector>


// wxWidgets includes

#include <wx/wx.h>
#include <wx/tokenzr.h>
#include <wx/datetime.h>
#include <wx/dnd.h>
#include <wx/clipbrd.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/sashwin.h>
#include <wx/cmdline.h>
#include <wx/combo.h>
#include <wx/config.h>

#ifdef WIN32
#include <wx/msw/helpchm.h>
#include <wx/taskbar.h>
#else
#include <wx/html/helpctrl.h> 
#endif


// database/utility includes
#include <xcm/xcm.h>
#include <kl/klib.h>
#include <kl/json.h>
#include "tango.h"
#include "commands.h"

// framework/control includes
#include "../kcl/kcl.h"
#include "../jobs/jobs.h"
#include "../webconnect/webcontrol.h"
#include "../webconnect/webframe.h"

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
#include "app.h"

#endif

