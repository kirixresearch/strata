/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2001-08-25
 *
 */


#ifndef __XCM_MACRO_H
#define __XCM_MACRO_H


// export macros 

#ifdef WIN32
    // this works with both gcc and msvc under win32
    #define XCM_EXPORT    __declspec(dllexport)
    #define XCM_STDCALL   _stdcall
#else
    #define XCM_EXPORT
    #define XCM_STDCALL
#endif


// remove VC9 warnings - eventually commenting this
// out and implementing specific fixes for these warnings
// is a good idea.
#ifdef _MSC_VER
#pragma warning(disable : 4996)
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



// class macros

namespace xcm
{
    enum ClassRuntimeFuncs
    {
        crfQueryInterface = 0,
        crfGetClassInfo = 1,
        crfInvokeMethod = 2
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

};




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
            xcm::mutex m_refcount_mutex; \
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
        xcm::mutex m_refcount_mutex; \
    public: \
        static const char* xcm_get_class_name() { return class_name; } \

#define XCM_BEGIN_INTERFACE_MAP(class_name)    \
    public: \
    static void* xcm_class_runtime(int func, class_name* obj, const char* interface_name, const char* method, xcm::value* params, xcm::value* retval) { \
        static xcm::class_info clsinfo; \
        if (func == xcm::crfQueryInterface && interface_name == NULL) \
            func = xcm::crfGetClassInfo; \
        if (func == xcm::crfGetClassInfo) {\
            if (*clsinfo.get_name()) \
                return (void*)&clsinfo; \
            clsinfo.set_name(xcm_get_class_name()); \
        }

#define XCM_INTERFACE_ENTRY(interface_type)    \
        if (func == xcm::crfGetClassInfo) \
            clsinfo.add_interface_info(interface_type::xcm_get_type_info()); \
        if (interface_name && !strcmp(xcm::IObject::xcm_get_interface_name(), interface_name)) { \
            if (func == xcm::crfQueryInterface) { \
                obj->ref(); \
                return static_cast<xcm::IObject*>(static_cast<interface_type*>(obj)); \
            } \
        } \
        if (interface_name && !strcmp(interface_type::xcm_get_interface_name(), interface_name)) { \
            if (func == xcm::crfQueryInterface) { \
                obj->ref();    \
                return static_cast<interface_type*>(obj); \
            } \
            if (func == xcm::crfInvokeMethod) { \
                interface_type::xcm_interface_runtime( \
                    static_cast<interface_type*>(obj), \
                    method, \
                    params, \
                    retval); \
                return NULL; \
            } \
        }

#define XCM_INTERFACE_CHAIN(base_class) \
        { \
            void* res = base_class::xcm_class_runtime(func, obj, interface_name, method, params, retval); \
            if (func == xcm::crfGetClassInfo) \
            { \
                clsinfo.add_chain_info((xcm::class_info*)res); \
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
        return (xcm::class_info*)xcm_class_runtime(xcm::crfGetClassInfo, NULL, NULL, NULL, NULL, NULL); \
    } \
    void* query_interface(const char* interface_name) { \
        return xcm_class_runtime(xcm::crfQueryInterface, this, interface_name, NULL, NULL, NULL); \
    } \
    void invoke(const char* method, xcm::value* params, int param_count, xcm::value* retval) { \
        char intr_part[255]; \
        char method_part[255]; \
        const char* cc = strstr(method, "::"); \
        if (!cc) \
            return; \
        strncpy(intr_part, method, cc-method); \
        intr_part[cc-method] = 0; \
        strcpy(method_part, cc+2); \
        xcm_class_runtime(xcm::crfInvokeMethod, this, intr_part, method_part, params, retval); \
    }


// anonymous class support

#define XCM_ANONYMOUS_CLASS(class_name) \
    public: \
    class_name* xcm_class_runtime(int func, class_name* obj, const char* interface_name, const char* method, xcm::value* params, xcm::value* retval) { \
        if (func == xcm::crfQueryInterface) { \
            obj->ref(); \
            return obj; \
        } \
        return NULL; \
    } \
    static const char* xcm_get_interface_name() { \
        return ""; \
    } \
    void* query_interface(const char* interface_name) { \
        return xcm_class_runtime(xcm::crfQueryInterface, this, interface_name, NULL, NULL, NULL); \
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


// type info macros

#define XCM_NO_TYPE_INFO()    \
    public: \
    static void* xcm_interface_runtime(void* pI, const char* call_method, xcm::value* params, xcm::value* retval) { return NULL; } \
    static xcm::interface_info* xcm_get_type_info() { return NULL; }

#endif


