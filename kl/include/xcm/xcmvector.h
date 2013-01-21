/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2002-09-25
 *
 */


#ifndef __XCM_XCMVECTOR_H
#define __XCM_XCMVECTOR_H


#include <vector>
#include <algorithm>


namespace xcm
{


template <class T>
class IVector : public xcm::IObject
{
    XCM_INTERFACE_NAME("xcm.IVector")

public:

    virtual size_t size() = 0;
    virtual T getItem(size_t idx) = 0;
    virtual bool setItem(size_t idx, const T& item) = 0;
    virtual bool deleteAt(size_t idx) = 0;
    virtual bool deleteItem(const T& item) = 0;
    virtual void append(const T& item) = 0;
    virtual void insert(size_t idx, const T& item) = 0;
    virtual void clear() = 0;
    virtual xcm::ptr<IVector<T> > clone() = 0;
    virtual void lock() = 0;
    virtual void unlock() = 0;
};


template <class T>
class IVectorImpl : public IVector<T>
{

XCM_CLASS_NAME("xcm.IVectorImpl")
XCM_BEGIN_INTERFACE_MAP(IVectorImpl<T> )
    XCM_INTERFACE_ENTRY(xcm::IVector<T>)
XCM_END_INTERFACE_MAP()

private:

    xcm::mutex m_obj_mutex;
    std::vector<T> m_vec;

public:

    IVectorImpl()
    {
    }

    virtual ~IVectorImpl()
    {
    }

    size_t size()
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        
        return m_vec.size();
    }

    T getItem(size_t idx)
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        
        return m_vec[idx];
    }

    bool setItem(size_t idx, const T& item)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        if (idx >= m_vec.size())
            return false;

        m_vec[idx] = item;

        return true;
    }

    bool deleteAt(size_t idx)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        if (idx >= m_vec.size())
            return false;
            
        m_vec.erase(m_vec.begin() + idx);
        return true;
    }

    bool deleteItem(const T& item)
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        
        typename std::vector<T>::iterator it;
        it = std::find(m_vec.begin(), m_vec.end(), item);
        if (it == m_vec.end())
            return false;
        m_vec.erase(it);
        return true;
    }

    void append(const T& item)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_vec.push_back(item);
    }

    void insert(size_t idx, const T& item)
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_vec.insert(m_vec.begin() + idx, item);
    }

    void clear()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        m_vec.clear();
    }

    xcm::ptr<IVector<T> > clone()
    {
        XCM_AUTO_LOCK(m_obj_mutex);

        IVectorImpl<T>* new_vec = new IVectorImpl<T>;
        for (typename std::vector<T>::iterator it = m_vec.begin();
             it != m_vec.end(); ++it)
        {
            new_vec->m_vec.push_back(*it);
        }
        
        return new_vec;
    }

    void setVector(std::vector<T>& vec)
    {
        XCM_AUTO_LOCK(m_obj_mutex);
        
        m_vec.clear();
        m_vec.reserve(vec.size());
        for (typename std::vector<T>::iterator it = vec.begin();
             it != vec.end(); ++it)
        {
            m_vec.push_back(*it);
        }
    }

    void lock()
    {
        m_obj_mutex.lock();
    }

    void unlock()
    {
        m_obj_mutex.unlock();
    }

};




};  // namespace xcm


#endif

