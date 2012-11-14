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


namespace kl
{

#ifdef WIN32

int thread_create(thread_t *thread, const thread_t *attr,
                  void *(*start_routine) (void *), void *arg)
{
    return tcrOK;
}

#else

int thread_create(thread_t *thread, const thread_t *attr,
                  void *(*start_routine) (void *), void *arg)
{
    return tcrOK;
}

#endif

};

