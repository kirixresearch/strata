/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2004-09-01
 *
 */


#ifndef __XDCOMMON_MODVERSION_H
#define __XDCOMMON_MODVERSION_H


#include "build_config.h"

#ifdef APP_VERSION_MAJOR
#define MODVERSION_MAJOR            APP_VERSION_MAJOR
#define MODVERSION_MINOR            APP_VERSION_MINOR
#define MODVERSION_SUBMINOR         APP_VERSION_SUBMINOR
#define MODVERSION_BUILDSERIAL      APP_VERSION_BUILDSERIAL
#else
#define MODVERSION_MAJOR            4
#define MODVERSION_MINOR            5
#define MODVERSION_SUBMINOR         4
#define MODVERSION_BUILDSERIAL      0
#endif


// -- version parts for the resource file (usually do not modify these) --
#define MOD_RCVER_PART1   MODVERSION_MAJOR
#define MOD_RCVER_PART2   MODVERSION_MINOR
#define MOD_RCVER_PART3   MODVERSION_SUBMINOR
#define MOD_RCVER_PART4   MODVERSION_BUILDSERIAL


#endif

