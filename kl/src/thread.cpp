/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Aaron L. Williams
 * Created:  2012-11-14
 *
 */


#include <kl/thread.h>

#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#endif

#ifdef __linux__
#include <sys/syscall.h>
#endif

#ifdef __APPLE__
#include <mach/mach.h>
#endif

namespace kl
{

thread_t g_mainthread_id = thread_getcurrentid();

#ifdef WIN32
#define STDCALL __stdcall
#else
#define STDCALL 
#endif

// define use of C runtime library
#if defined(_MT)
#undef KL_CRT
#define KL_CRT
#endif


static unsigned int STDCALL thread_entry_proxy(void* t)
{
    thread* lthread = static_cast<thread*>(t);
    lthread->entry();
    lthread->exit();

    if (lthread->isAutoDelete())
    {
        delete lthread;
    }
    
    // TODO: more comprehensive return code
    return 0;
}


thread::thread()
{
}

thread::~thread()
{
}

int thread::create()
{
    thread_t handle;
    return thread_create(&handle, NULL, thread_entry_proxy, this);
}

void thread::sleep(unsigned int milliseconds)
{
    thread_sleep(milliseconds);
}

bool thread::isMain()
{
    return thread_ismain();
}

unsigned int thread::entry()
{
    return 0;
}

void thread::exit()
{
}







// thread class implemetation


#ifdef WIN32

mutex::mutex()
{
    ::InitializeCriticalSection((PCRITICAL_SECTION)m_data);
}

mutex::~mutex()
{
    ::DeleteCriticalSection((PCRITICAL_SECTION)m_data);
}

void mutex::lock()
{
    ::EnterCriticalSection((PCRITICAL_SECTION)m_data);
}

void mutex::unlock()
{
    ::LeaveCriticalSection((PCRITICAL_SECTION)m_data);
}

#else

mutex::mutex()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init((pthread_mutex_t*)m_data, &attr);
}

mutex::~mutex()
{
    pthread_mutex_destroy((pthread_mutex_t*)m_data);
}

void mutex::lock()
{
    pthread_mutex_lock((pthread_mutex_t*)m_data);
}

void mutex::unlock()
{
    pthread_mutex_unlock((pthread_mutex_t*)m_data);
}



#endif







#ifdef WIN32

int thread_create(thread_t* thread, const thread_t* attr,
                  unsigned (KLTHREAD_CALLING_CONVENTION *start_routine) (void*), void* arg)
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

thread_t thread_getcurrentid()
{
    return (thread_t)::GetCurrentThreadId();
}

bool thread_ismain()
{
    return (g_mainthread_id == thread_getcurrentid()) ? true : false;
}

#else

int thread_create(thread_t* thread, const thread_t* attr,
                  unsigned (KLTHREAD_CALLING_CONVENTION *start_routine) (void*), void* arg)
{
    int res = pthread_create((pthread_t*)thread, NULL, (void*(*)(void*))start_routine, arg);
    switch (res)
    {
        case 0:
            return tcrOK;
        case EAGAIN:
            return tcrEAGAIN;
        case EINVAL:
            return tcrEINVAL;
        case EPERM:
            return tcrEPERM;
        default:
            return tcrUNKNOWN;
    }
}

void thread_sleep(unsigned int milliseconds)
{
    ::usleep(milliseconds*1000);
}

#ifdef __APPLE__

thread_t thread_getcurrentid()
{
    return (thread_t)mach_thread_self();
}

bool thread_ismain()
{
    return (g_mainthread_id == thread_getcurrentid()) ? true : false;
}

#else

thread_t thread_getcurrentid()
{
    return (thread_t)syscall(SYS_gettid); 
}

bool thread_ismain()
{
    return (syscall(SYS_gettid) == syscall(SYS_getpid)) ? true : false;
}

#endif



#endif

};

