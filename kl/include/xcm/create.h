/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2001-08-11
 *
 */


#ifndef __XCM_CREATE_H
#define __XCM_CREATE_H


#ifndef WIN32
#include <dlfcn.h>
#endif


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <vector>
#include <string>


extern "C"
{
    XCM_EXPORT xcm::IObject* xcm_create_instance(const char* obj_class);
};


namespace xcm
{


typedef void* XCM_MODULE;


class static_module_base
{
public:

    virtual void* xcm_module_runtime(bool bCreateInstance, const char* obj_class) = 0;
};


std::wstring getProgramPath();
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


class static_module_list
{
public:

    static std::vector<static_module_base*>& get();
    static void add(static_module_base* mod);
};


};


#endif

