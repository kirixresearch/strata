/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Aaron L. Williams
 * Created:  2012-11-14
 *
 */


#ifndef __KL_THREAD_H
#define __KL_THREAD_H


namespace kl
{


typedef unsigned long thread_t;


enum ThreadCreateReturn
{
    tcrOK = 0,
    tcrEAGAIN = 1,
    tcrEINVAL = 2,
    tcrEPERM = 3
};


int thread_create(thread_t *thread, const thread_t *attr,
                  void *(*start_routine) (void *), void *arg);

};


#endif

