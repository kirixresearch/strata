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

#include <xcm/macro.h>
#include <xcm/xcmvalue.h>
#include <xcm/xcmthread.h>
#include <xcm/typeinfo.h>

#define xcm_interface class

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



#include <xcm/smartptr.h>
#include <xcm/xcmvector.h>
#include <xcm/signal.h>


#endif

