/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
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


class Thread
{
public:
    Thread();
    virtual ~Thread();

    int create();
   
    static void sleep(unsigned int milliseconds);
    static bool isMain();

    virtual unsigned int entry();
    virtual void exit();
};


int thread_create(thread_t* thread, const thread_t* attr,
                  unsigned (*start_routine) (void *), void* arg);

void thread_sleep(unsigned int milliseconds);

unsigned int thread_getcurrentid();
unsigned int thread_getid(thread_t *thread);

bool thread_ismain();

};


#endif

