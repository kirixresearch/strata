/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#ifndef __KXINTER_KXINTER_H
#define __KXINTER_KXINTER_H


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


// -- c/c++ library includes --
#include <cmath>
#include <clocale>


// -- product name --

#define PRODUCT_NAME    wxT("Kirix Research Conversion Utility")
#define PRODUCT_VERSION wxT("0.8")

#define PALADIN_APP_TAG         "KCU"
#define PALADIN_TRIAL_TAG       "KCU_0_9"

// -- wxWindows includes --

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/dnd.h>
#include <wx/clipbrd.h>
#include <wx/splash.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/wizard.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/filedlg.h>
#include <wx/fontdlg.h>
#include <wx/colordlg.h>
#include <wx/notebook.h>
#include <wx/generic/treectlg.h>

#ifdef WIN32
#include <wx/msw/helpchm.h>
#else
#include <wx/html/helpctrl.h> 
#endif


#include <xcm/xcm.h>
#include <kl/klib.h>

#include "tango.h"

// -- KxInter includes --
#include "../kcl/kcl.h"
#include "../cfw/cfw.h"
#include "../kappcmn/connectionmgr.h"
#include "../paladin/paladin.h"
#include "bitmapmgr.h"
#include "util.h"
#include "app.h"


extern KxInterApp* g_app;


#endif

