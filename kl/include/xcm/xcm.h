/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2001-08-10
 *
 */


#ifndef __XCM_XCM_H
#define __XCM_XCM_H

#include <vector>
#include <string>


#define xcm_interface class


#include <xcm/macro.h>
#include <xcm/xcmvalue.h>
#include <xcm/xcmthread.h>
#include <xcm/typeinfo.h>
#include <xcm/signal.h>


namespace xcm
{

xcm_interface IObject
{
    XCM_INTERFACE_NAME("xcm.IObject")
    XCM_NO_TYPE_INFO()

public:

    virtual void ref() = 0;
    virtual void unref() = 0;
    virtual int get_ref_count() = 0;
    virtual void* query_interface(const char* interface_name) = 0;
};



class refcount_holder
{
public:

    #if defined(__LP64__) || defined(_M_X64)
    long long xcm_ref_count;
    #else
    long xcm_ref_count;
    #endif

    refcount_holder()
    {
        xcm_ref_count = 0;
    }
};




xcm::IObject* create_instance(const std::string& class_name);
xcm::class_info* get_class_info(const std::string& class_name);
xcm::class_info* get_class_info(IObject* instance);
const char* get_last_error();

class path_list
{
public:
    static std::vector<std::wstring>& get();
    static void add(const std::wstring& path);
};

}; // namespace xcm






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
};


}; // namespace xcm




#include <xcm/smartptr.h>



#endif

