/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2001-09-04
 *
 */


#ifndef __XCM_TYPEINFO_H
#define __XCM_TYPEINFO_H


#include <string>


namespace xcm
{


enum valueTypes
{
    typeException = -1,
    typeVoid = 0,
    typeInt32 = 1,
    typeInt64 = 2,
    typeUint32 = 3,
    typeUint64 = 4,
    typeBoolean = 5,
    typeString = 6,
    typeDouble = 7,
    typeBase64 = 8,
    typeStruct = 20,
    typeArray = 21,
    typeStructure = 22,
    typeObjectRef = 23,
    typePtr = 24
};


enum valueException
{
    exMethodNotFound = -1,
    exMethodBadParams = -2,
    exNoException = 0
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


};

#endif

