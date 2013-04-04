/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2003-04-29
 *
 */


#ifndef __XDCOMMON_XDCOMMON_H
#define __XDCOMMON_XDCOMMON_H


#include "tango.h"
#include "structure.h"
#include "columninfo.h"
#include "jobinfo.h"
#include "util.h"


// platform definitions


#ifdef WIN32
#define PATH_SEPARATOR_CHAR    L'\\'
#define PATH_SEPARATOR_STR     L"\\"
#else
#define PATH_SEPARATOR_CHAR    L'/'
#define PATH_SEPARATOR_STR     L"/"
#endif


#endif

