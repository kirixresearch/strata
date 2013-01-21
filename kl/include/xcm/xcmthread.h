/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2002-05-29
 *
 */


#ifndef __XCM_XCMTHREAD_H
#define __XCM_XCMTHREAD_H


#ifndef _MSC_VER
#include <pthread.h>
#endif


#define XCM_AUTO_LOCK(mutex) xcm::safe_mutex_locker xcm_safe_mutex_locker(mutex)


namespace xcm
{


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

threadid_t get_current_thread_id();






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







};


#endif

