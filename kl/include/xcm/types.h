/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2001-08-25
 *
 */

#ifndef __XCM_TYPES_H
#define __XCM_TYPES_H


namespace xcm
{

// -- int64 support --
#ifdef _MSC_VER
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
typedef long long int64_t;
typedef unsigned long long uint64_t;
#endif


typedef uint64_t threadid_t;


};


#endif
