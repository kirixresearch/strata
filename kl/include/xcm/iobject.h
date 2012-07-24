/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2001-08-10
 *
 */


#ifndef __XCM_IOBJECT_H
#define __XCM_IOBJECT_H


#define xcm_interface class


namespace xcm
{




xcm_interface IObject
{
    XCM_INTERFACE_NAME("xcm.IObject")

    XCM_BEGIN_TYPE_INFO(IObject)
        XCM_METHOD_V0(ref, "void ref()")
        XCM_METHOD_V0(unref, "void unref()")
        XCM_METHOD_R1(query_interface, "void* query_interface(ccstring interface_name)")
    XCM_END_TYPE_INFO()

public:

    virtual void ref() = 0;
    virtual void unref() = 0;
    virtual int get_ref_count() = 0;
    virtual void* query_interface(const char* interface_name) = 0;
};



xcm_interface IRuntime
{
    XCM_INTERFACE_NAME("xcm.IRuntime");
    XCM_NO_TYPE_INFO()

public:

    virtual void invoke(const char* method, xcm::value* params, int param_count, xcm::value* retval) = 0;
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





};



#endif

