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


#define KL_AUTO_LOCK(mutex) kl::safe_mutex_locker kl_safe_mutex_locker(mutex)

namespace kl
{


typedef unsigned long thread_t;

#ifdef _MSC_VER
#define KLTHREAD_CALLING_CONVENTION __stdcall
#else
#define KLTHREAD_CALLING_CONVENTION
#endif


enum ThreadCreateReturn
{
    tcrOK = 0,
    tcrEAGAIN = 1,
    tcrEINVAL = 2,
    tcrEPERM = 3
};


class thread
{
public:
    thread();
    virtual ~thread();

    int create();
   
    static void sleep(unsigned int milliseconds);
    static bool isMain();

    virtual unsigned int entry();
    virtual void exit();
};


class mutex
{
public:

    mutex();
    ~mutex();

    void lock();
    void unlock();

private:

    // this is big enough for CRITICAL_SECTION and
    // pthread_mutex_t on all supported platforms;
    // additional platforms will have to be checked

    unsigned char m_data[40];
};



class safe_mutex_locker
{
public:

    safe_mutex_locker(mutex& m) : m_mutex(m)
    {
        m_mutex.lock();
    }

    ~safe_mutex_locker()
    {
        m_mutex.unlock();
    }

private:

    mutex& m_mutex;
};




int thread_create(thread_t* thread,
                  const thread_t* attr,
                  unsigned (KLTHREAD_CALLING_CONVENTION *start_routine) (void *),
                  void* arg);

void thread_sleep(unsigned int milliseconds);

unsigned int thread_getcurrentid();

unsigned int thread_getid(thread_t *thread);

bool thread_ismain();



};


#endif
