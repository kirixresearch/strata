/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
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


namespace xcm
{




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


};


#endif

