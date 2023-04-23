/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2004-06-26
 *
 */


#ifndef H_KCANVAS_KCANVAS_INT_H
#define H_KCANVAS_KCANVAS_INT_H


// remove VC9 warnings - eventually commenting this
// out and implementing specific fixes for these warnings
// is a good idea.
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <limits>

#include <wx/wx.h>
#include <wx/file.h>
#include <wx/clipbrd.h>
#include <wx/dcps.h>
#include <wx/gdicmn.h>
#include <wx/image.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/renderer.h>
#include <wx/tokenzr.h>
#include <wx/dnd.h>
//#include <wx/pdfdoc.h>
#include <wx/mstream.h>
#include <wx/sstream.h>

#include <kl/xcm.h>
#include <kl/klib.h>
#include <kl/base64.h>
#include <kl/utf8.h>

#include "kcanvas.h"
#include "component.h"
#include "property.h"
#include "range.h"
#include "graphicsobj.h"

#ifdef WIN32
#include <windows.h>
#include <wx/fontutil.h>
#endif

namespace kcanvas
{


};  // namespace kcanvas


#endif

