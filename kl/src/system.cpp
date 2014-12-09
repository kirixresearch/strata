/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2012-09-21
 *
 */


#include "kl/portable.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace kl
{


void millisleep(int milliseconds)
{
#ifdef WIN32
    ::Sleep(milliseconds);
#else
    ::usleep(milliseconds*1000);
#endif
}





};


