/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-31
 *
 */


#ifdef WIN32
#include <windows.h>
#endif


#include "kl/memory.h"


namespace kl
{


// -- memory api --

#ifdef WIN32

    unsigned long getTotalPhysMemory()
    {
        MEMORYSTATUS memStatus;
        memStatus.dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus(&memStatus);
        return memStatus.dwTotalPhys;
    }

    unsigned long getFreePhysMemory()
    {
        MEMORYSTATUS memStatus;
        memStatus.dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus(&memStatus);
        return memStatus.dwAvailPhys;
    }

#else

    unsigned long getTotalPhysMemory()
    {
        return 268435456;
    }

    unsigned long getFreePhysMemory()
    {
        return 268435456;
    }
    
#endif


};
