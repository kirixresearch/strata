/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Aaron L. Williams
 * Created:  2012-11-14
 *
 */


#include "kl/thread.h"

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif


namespace kl
{

#ifdef WIN32

int thread_create(thread_t *thread, const thread_t *attr,
                  void *(*start_routine) (void *), void *arg)
{
    return tcrOK;
}

void thread_sleep(unsigned int milliseconds)
{
    ::Sleep(milliseconds);
}

#else

int thread_create(thread_t *thread, const thread_t *attr,
                  void *(*start_routine) (void *), void *arg)
{
    // TODO: implement
}

void thread_sleep(unsigned int milliseconds)
{
    // TODO: implement
}

#endif

};

