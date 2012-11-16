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
#include <process.h>
#else
#include <pthread.h>
#endif


namespace kl
{


#ifdef WIN32
unsigned int g_mainthread_id = ::GetCurrentThreadId();
#else
unsigned int g_mainthread_id = 0;   // TODO: implement
#endif

// define use of C runtime library
#if defined(_MT)
#undef KL_CRT
#define KL_CRT
#endif


Thread::Thread()
{
}

Thread::~Thread()
{
}

int Thread::create()
{
    return tcrOK;
}

int Thread::run()
{
    return tcrOK;
}

void Thread::sleep(unsigned int milliseconds)
{
    thread_sleep(milliseconds);
}

bool Thread::isMain()
{
    return thread_ismain();
}



#ifdef WIN32


int thread_create(thread_t *thread, const thread_t *attr,
                  unsigned (__stdcall *start_routine) (void *), void *arg)
{
    // use of the c runtime library should use a _beginthreadex instead
    // of CreateThread; see: http://support.microsoft.com/kb/104641/en-us
    
#ifdef KL_CRT
    *thread = ::_beginthreadex(NULL, 0, start_routine, arg, 0, NULL);
#else
    // TODO: implement CreateThread()
#endif

    // TODO: get error code for thread and return it
    return tcrOK;
}

void thread_sleep(unsigned int milliseconds)
{
    ::Sleep(milliseconds);
}

unsigned int thread_getcurrentid()
{
    return ::GetCurrentThreadId();
}

unsigned int thread_getid(thread_t *thread)
{
    // TODO: implement
    return 0;
}

bool thread_ismain()
{
    if (g_mainthread_id == thread_getcurrentid())
        return true;

    return false;
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

unsigned int thread_getcurrentid()
{
    // TODO: implement
}

unsigned int thread_getid(thread_t *thread)
{
    // TODO: implement
}

bool thread_ismain()
{
    // TODO: implement
}

#endif

};

