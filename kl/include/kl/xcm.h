/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2001-08-10
 *
 */


#ifndef H_KL_XCM_H
#define H_KL_XCM_H


#include <vector>
#include <cstring>
#include <string>
#include <algorithm>
#include <kl/thread.h>
#include <kl/signal.h>


// remove VC9 warnings - eventually commenting this out and implementing
// specific fixes for these warnings is a good idea

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif



#define xcm_interface class


// export macros 

#ifdef WIN32
    // this works with both gcc and msvc under win32
    #define XCM_EXPORT    __declspec(dllexport)
    #define XCM_STDCALL   _stdcall
#else
    #define XCM_EXPORT
    #define XCM_STDCALL
#endif


// smart ptr macros

#define XCM_DECLARE_SMARTPTR(interface_type) \
    typedef xcm::ptr<interface_type > interface_type##Ptr;

#define XCM_DECLARE_SMARTPTR2(interface_type, smartptr_name) \
    typedef xcm::ptr<interface_type > smartptr_name;


// module macros

#define XCM_BEGIN_DYNAMIC_MODULE(module_name) \
    extern "C" { \

#define XCM_END_DYNAMIC_MODULE(module_name) \
        XCM_EXPORT xcm::IObject* xcm_create_instance(const char* obj_class) { \
            return (xcm::IObject*)xcm_module_runtime(true, obj_class); \
        } \
        XCM_EXPORT xcm::class_info* xcm_get_class_info(const char* obj_class) { \
            return (xcm::class_info*)xcm_module_runtime(false, obj_class); \
        } \
    }


// class map macros

#define XCM_BEGIN_CLASS_MAP()    \
    void* xcm_module_runtime(bool bCreateInstance, const char* obj_class) {

#define XCM_CLASS_ENTRY(cpp_class)    \
    if (!strcmp(obj_class, cpp_class::xcm_get_class_name())) {    \
        if (bCreateInstance) \
        { \
            cpp_class* obj = new cpp_class;    \
            return static_cast<xcm::IObject*>(obj->query_interface(xcm::IObject::xcm_get_interface_name())); \
        } \
         else \
        { \
            return cpp_class::xcm_get_class_info(); \
        } \
    } 

#define XCM_END_CLASS_MAP()    \
        return NULL; \
    }


#ifdef _MSC_VER

    #define XCM_REFCOUNT_IMPL() \
        private: \
            xcm::refcount_holder m_refcount_holder;    \
        public:    \
            virtual void ref() { \
                xcm::interlocked_increment(&m_refcount_holder.xcm_ref_count); \
            } \
            virtual void unref() { \
                if (xcm::interlocked_decrement(&m_refcount_holder.xcm_ref_count) == 0) \
                    delete this; \
            } \
            virtual int get_ref_count() { \
                int result = (int)xcm::interlocked_increment(&m_refcount_holder.xcm_ref_count); \
                result--; \
                xcm::interlocked_decrement(&m_refcount_holder.xcm_ref_count); \
                return result; \
            }
#else

    #define XCM_REFCOUNT_IMPL() \
        private: \
            xcm::refcount_holder m_refcount_holder;    \
            kl::mutex m_refcount_mutex; \
        public:    \
            virtual void ref() { \
                m_refcount_mutex.lock(); \
                m_refcount_holder.xcm_ref_count++; \
                m_refcount_mutex.unlock(); \
            } \
            virtual void unref() { \
                m_refcount_mutex.lock(); \
                if (--m_refcount_holder.xcm_ref_count == 0) { \
                    m_refcount_mutex.unlock(); \
                    delete this; \
                    return; \
                } \
                m_refcount_mutex.unlock(); \
            } \
            virtual int get_ref_count() { \
                m_refcount_mutex.lock(); \
                int result = m_refcount_holder.xcm_ref_count; \
                m_refcount_mutex.unlock(); \
                return result; \
            }
#endif

#define XCM_REFCOUNT_NOTHREADSAFE_IMPL() \
    private: \
        xcm::refcount_holder m_refcount_holder;    \
    public:    \
        virtual void ref() { \
            m_refcount_holder.xcm_ref_count++; \
        } \
        virtual void unref() { \
            if (--m_refcount_holder.xcm_ref_count == 0) \
                delete this; \
        } \
        virtual int get_ref_count() { \
            return m_refcount_holder.xcm_ref_count; \
        }

#define XCM_NOREFCOUNT_IMPL() \
    public: \
        virtual void ref() { } \
        virtual void unref() { } \
        virtual int get_ref_count() { return 10; }

#define XCM_CLASS_NAME(class_name)    \
    public: \
        static const char* xcm_get_class_name() { return class_name; } \
        XCM_REFCOUNT_IMPL()
        
#define XCM_CLASS_NAME_NOTHREADSAFE(class_name)    \
    public: \
        static const char* xcm_get_class_name() { return class_name; } \
        XCM_REFCOUNT_NOTHREADSAFE_IMPL()

#define XCM_CLASS_NAME_NOREFCOUNT(class_name) \
    public: \
        static const char* xcm_get_class_name() { return class_name; } \
        XCM_NOREFCOUNT_IMPL()

#define XCM_CLASS_NAME_CUSTOMREFCOUNT(class_name) \
    private: \
        xcm::refcount_holder m_refcount_holder;    \
        kl::mutex m_refcount_mutex; \
    public: \
        static const char* xcm_get_class_name() { return class_name; } \

#define XCM_BEGIN_INTERFACE_MAP(class_name)    \
    public: \
    static void* xcm_class_runtime(int func, class_name* obj, const char* interface_name) { \
        static xcm::class_info clsinfo; \
        if (func == xcm::crfQueryInterface && interface_name == NULL) \
            func = xcm::crfGetClassInfo; \
        if (func == xcm::crfGetClassInfo) {\
            if (*clsinfo.get_name()) \
                return (void*)&clsinfo; \
            clsinfo.set_name(xcm_get_class_name()); \
        }

#define XCM_INTERFACE_ENTRY(interface_type)    \
        if (interface_name && 0 == strcmp(xcm::IObject::xcm_get_interface_name(), interface_name)) { \
            if (func == xcm::crfQueryInterface) { \
                obj->ref(); \
                return static_cast<xcm::IObject*>(static_cast<interface_type*>(obj)); \
            } \
        } \
        if (interface_name && 0 == strcmp(interface_type::xcm_get_interface_name(), interface_name)) { \
            if (func == xcm::crfQueryInterface) { \
                obj->ref();    \
                return static_cast<interface_type*>(obj); \
            } \
        }

#define XCM_INTERFACE_CHAIN(base_class) \
        { \
            void* res = base_class::xcm_class_runtime(func, obj, interface_name); \
            if (func == xcm::crfGetClassInfo) \
            { \
            } \
             else \
            { \
                if (res) \
                { \
                    return res; \
                } \
            } \
        }

#define XCM_END_INTERFACE_MAP()    \
        if (func == xcm::crfGetClassInfo) \
            return (void*)&clsinfo; \
        return NULL; \
    } \
    static xcm::class_info* xcm_get_class_info() { \
        return (xcm::class_info*)xcm_class_runtime(xcm::crfGetClassInfo, NULL, NULL); \
    } \
    void* query_interface(const char* interface_name) { \
        return xcm_class_runtime(xcm::crfQueryInterface, this, interface_name); \
    }


// interface macros

#define XCM_INTERFACE_NAME(intf_name)    \
    public:    \
    static const char* xcm_get_interface_name() { return intf_name; }

// signal macros

#define XCM_DECLARE_SIGNAL0(sig_name)   \
            virtual xcm::signal0& sig_name() = 0;

#define XCM_DECLARE_SIGNAL1(sig_name, p1)   \
            virtual xcm::signal1<p1>& sig_name() = 0;

#define XCM_DECLARE_SIGNAL2(sig_name, p1, p2)   \
            virtual xcm::signal2<p1,p2>& sig_name() = 0;

#define XCM_DECLARE_SIGNAL3(sig_name, p1, p2, p3)   \
            virtual xcm::signal3<p1,p2,p3>& sig_name() = 0;

#define XCM_DECLARE_SIGNAL4(sig_name, p1, p2, p3, p4)   \
            virtual xcm::signal4<p1,p2,p3,p4>& sig_name() = 0;

#define XCM_IMPLEMENT_SIGNAL0(sig_name)   \
            xcm::signal0 sig_##sig_name; \
            xcm::signal0& sig_name() { return sig_##sig_name; }

#define XCM_IMPLEMENT_SIGNAL1(sig_name, p1)   \
            xcm::signal1<p1> sig_##sig_name; \
            xcm::signal1<p1>& sig_name() { return sig_##sig_name; }

#define XCM_IMPLEMENT_SIGNAL2(sig_name, p1, p2)   \
            xcm::signal2<p1,p2> sig_##sig_name; \
            xcm::signal2<p1,p2>& sig_name() { return sig_##sig_name; }

#define XCM_IMPLEMENT_SIGNAL3(sig_name, p1, p2, p3)   \
            xcm::signal3<p1,p2,p3> sig_##sig_name; \
            xcm::signal3<p1,p2,p3>& sig_name() { return sig_##sig_name; }

#define XCM_IMPLEMENT_SIGNAL4(sig_name, p1, p2, p3, p4)   \
            xcm::signal4<p1,p2,p3,p4> sig_##sig_name; \
            xcm::signal4<p1,p2,p3,p4>& sig_name() { return sig_##sig_name; }





namespace xcm
{

enum ClassRuntimeFuncs
{
    crfQueryInterface = 0,
    crfGetClassInfo = 1
};

#ifdef _MSC_VER
#if defined(__LP64__) || defined(_M_X64)
long long XCM_STDCALL interlocked_increment(long long*);
long long XCM_STDCALL interlocked_decrement(long long*);
#else
long XCM_STDCALL interlocked_increment(long*);
long XCM_STDCALL interlocked_decrement(long*);
#endif
#endif




xcm_interface IObject
{
    XCM_INTERFACE_NAME("xcm.IObject")

public:

    virtual void ref() = 0;
    virtual void unref() = 0;
    //virtual int get_ref_count() = 0;
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


class class_info
{
public:

    const char* get_name()
    {
        return m_name.c_str();
    }

    void set_name(const char* name)
    {
        m_name = name;
    }

private:

    std::string m_name;
};


class path_list
{
public:
    static std::vector<std::wstring>& get();
    static void add(const std::wstring& path);
};


xcm::IObject* create_instance(const std::string& class_name);
xcm::class_info* get_class_info(const std::string& class_name);
xcm::class_info* get_class_info(IObject* instance);
const char* get_last_error();



// Vector represented as a vptr interface

template <class T>
xcm_interface IVector : public xcm::IObject
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
        return m_vec.size();
    }

    T getItem(size_t idx)
    {
        return m_vec[idx];
    }

    bool setItem(size_t idx, const T& item)
    {
        if (idx >= m_vec.size())
            return false;

        m_vec[idx] = item;

        return true;
    }

    bool deleteAt(size_t idx)
    {
        if (idx >= m_vec.size())
            return false;
            
        m_vec.erase(m_vec.begin() + idx);
        return true;
    }

    bool deleteItem(const T& item)
    {
        typename std::vector<T>::iterator it;
        it = std::find(m_vec.begin(), m_vec.end(), item);
        if (it == m_vec.end())
            return false;
        m_vec.erase(it);
        return true;
    }

    void append(const T& item)
    {
        m_vec.push_back(item);
    }

    void insert(size_t idx, const T& item)
    {
        m_vec.insert(m_vec.begin() + idx, item);
    }

    void clear()
    {
        m_vec.clear();
    }

    void setVector(const std::vector<T>& vec)
    {
        m_vec.clear();
        m_vec.reserve(vec.size());
        typename std::vector<T>::const_iterator it, it_end = vec.cend();
        for (it = vec.cbegin(); it != it_end; ++it)
        {
            m_vec.push_back(*it);
        }
    }
};






// -- smart pointer implementation --

typedef unsigned int null_type;
const null_type null = 0;


class ptr_base
{
public:
    virtual ~ptr_base() { }
    virtual IObject* get_iobject() const = 0;
};

template <class T>
class ptr : public ptr_base
{
public:

    IObject* get_iobject() const
    {
        return static_cast<IObject*>(p);
    }

    T* p;

public:

    ptr()
    {
        p = 0;
    }

    ptr(const ptr_base& c)
    {
        IObject* iobj = c.get_iobject();
        if (iobj)
        {
            p = (T*)iobj->query_interface(T::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }
    }

    ptr(const ptr<T>& c)
    {
        p = c.p;
        if (p)
        {
            p->ref();
        }
    }

    ptr(T* const& _p)
    {
        p = _p;
        if (p)
        {
            p->ref();
        }
    }

    ptr(T* const& _p, bool addref)
    {
        p = _p;
        if (p && addref)
        {
            p->ref();
        }
    }

    ptr(IObject* _p)
    {
        if (_p)
        {
            p = (T*)_p->query_interface(T::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }
    }

    ptr(const null_type& null_value)
    {
        p = 0;
    }

    ~ptr()
    {
        if (p)
        {
            p->unref();
        }
    }

    operator IObject*() const
    {
        return static_cast<IObject*>(p);
    }

    operator T*() const
    {
        return p;
    }

    T* operator->() const
    {
        return p;
    }

    bool operator!() const
    {
        return (p == 0);
    }

    operator bool() const
    {
        return (p != 0);
    }

    bool operator()() const
    {
        return (p != 0);
    }

    ptr<T>& operator=(const ptr_base& c)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        IObject* iobj = c.get_iobject();
        if (iobj)
        {
            p = (T*)iobj->query_interface(T::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }

        return *this;
    }

    ptr<T>& operator=(const ptr<T>& c)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        p = c.p;
        if (p)
        {
            p->ref();
        }
        return *this;
    }

    ptr<T>& operator=(T* _p)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        p = _p;

        if (p)
        {
            p->ref();
        }

        return *this;
    }

    ptr<T>& operator=(IObject* _p)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        if (_p)
        {
            p = (T*)_p->query_interface(T::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }

        return *this;
    }

    ptr<T>& operator=(const null_type& null_value)
    {
        if (p)
        {
            p->unref();
        }
        p = 0;
        return *this;
    }

    bool operator==(const ptr<T>& rhs) const
    {
        return (p == rhs.p);
    }

    bool operator==(T* rhs) const
    {
        return (p == rhs);
    }

    bool operator==(IObject* rhs) const
    {
        return (static_cast<IObject*>(p) == rhs);
    }

    bool operator==(const null_type& null_value) const
    {
        return (p == 0);
    }

    bool operator!=(const ptr<T>& rhs) const
    {
        return (p != rhs.p);
    }

    bool operator!=(T* rhs) const
    {
        return (p != rhs);
    }

    bool operator!=(IObject* rhs) const
    {
        return (static_cast<IObject*>(p) != rhs);
    }

    bool operator!=(const null_type& null_value) const
    {
        return (p != 0);
    }

    bool isNull() const
    {
        return (p == 0);
    }

    bool isOk() const
    {
        return (p != 0);
    }

    void clear()
    {
        if (p)
        {
            p->unref();
        }
        p = 0;
    }

    bool create_instance(const char* class_name)
    {
        if (p)
        {
            p->unref();
            p = 0;
        }
        xcm::IObject* pObject;
        pObject = xcm::create_instance(class_name);
        if (!pObject)
            return false;
        p = (T*)pObject->query_interface(T::xcm_get_interface_name());
        pObject->unref();
        if (!p)
            return false;
        return true;
    }
};





class IObjectPtr : public ptr_base
{
public:

    IObject* get_iobject() const
    {
        return p;
    }

    IObject* p;

public:

    IObjectPtr()
    {
        p = 0;
    }

    IObjectPtr(const ptr_base& c)
    {
        IObject* iobj = c.get_iobject();
        if (iobj)
        {
            p = (IObject*)iobj->query_interface(IObject::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }
    }

    IObjectPtr(IObjectPtr& c)
    {
        p = c.p;
        if (p)
        {
            p->ref();
        }
    }

    IObjectPtr(IObject* const& _p)
    {
        p = _p;
        if (p)
        {
            p->ref();
        }
    }

    IObjectPtr(IObject* const& _p, bool addref)
    {
        p = _p;
        if (p && addref)
        {
            p->ref();
        }
    }

    IObjectPtr(IObject* _p)
    {
        if (_p)
        {
            p = (IObject*)_p->query_interface(IObject::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }
    }

    IObjectPtr(const null_type& null_value)
    {
        p = 0;
    }

    ~IObjectPtr()
    {
        if (p)
        {
            p->unref();
        }
    }

    operator IObject*() const
    {
        return p;
    }

    IObject* operator->() const
    {
        return p;
    }

    bool operator!() const
    {
        return (p == 0);
    }

    operator bool() const
    {
        return (p != 0);
    }

    bool operator()() const
    {
        return (p != 0);
    }

    IObjectPtr& operator=(const ptr_base& c)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        IObject* iobj = c.get_iobject();
        if (iobj)
        {
            p = (IObject*)iobj->query_interface(IObject::xcm_get_interface_name());
        }
         else
        {
            p = 0;
        }

        return *this;
    }

    IObjectPtr& operator=(const IObjectPtr& c)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        p = c.p;
        if (p)
        {
            p->ref();
        }
        return *this;
    }

    IObjectPtr& operator=(IObject* _p)
    {
        if (p)
        {
            // release old ptr
            p->unref();
        }

        p = _p;

        if (p)
        {
            p->ref();
        }

        return *this;
    }

    IObjectPtr& operator=(const null_type& null_value)
    {
        if (p)
        {
            p->unref();
        }
        p = 0;
        return *this;
    }

    bool operator==(const IObjectPtr& rhs) const
    {
        return (p == rhs.p);
    }

    bool operator==(IObject* rhs) const
    {
        return (p == rhs);
    }

    bool operator==(const null_type& null_value) const
    {
        return (p == 0);
    }

    bool operator!=(const IObjectPtr& rhs) const
    {
        return (p != rhs.p);
    }

    bool operator!=(IObject* rhs) const
    {
        return (p != rhs);
    }

    bool operator!=(const null_type& null_value) const
    {
        return (p != 0);
    }

    bool isNull() const
    {
        return (p == 0);
    }

    bool isOk() const
    {
        return (p != 0);
    }

    void clear()
    {
        if (p)
        {
            p->unref();
        }
        p = 0;
    }

    bool create_instance(const char* class_name)
    {
        if (p)
        {
            p->unref();
            p = 0;
        }
        p = xcm::create_instance(class_name);
        if (!p)
            return false;
        return true;
    }
};



}; // namespace xcm


#endif
