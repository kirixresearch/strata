/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2002-08-28
 *
 */


#ifndef __XCM_XCMERROR_H
#define __XCM_XCMERROR_H


namespace xcm
{


typedef int result;


inline bool succeeded(result res)
{
    return (res >= 0 ? true : false);
}

inline bool failed(result res)
{
    return (res < 0 ? true : false);
}



};


#endif
