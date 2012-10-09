/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2001-09-04
 *
 */


#ifndef __XCM_XCMVALUE_H
#define __XCM_XCMVALUE_H


#include <xcm/typeinfo.h>
#include <vector>
#include <string>


namespace xcm
{


typedef void* ObjRef;

#ifdef _MSC_VER
typedef __int64 xcm_int64;
typedef unsigned __int64 xcm_uint64;
#else
typedef long long xcm_int64;
typedef unsigned long long xcm_uint64;
#endif

class value
{

public:

    value();
    value(const value& c);
    value& operator=(const value& c);


    value(bool b)
    {
        name = NULL;
        setBoolean(b);
    }

    value(const char* s)
    {
        name = NULL;
        setString(s);
    }

    value(double d)
    {
        name = NULL;
        setDouble(d);
    }

    value(void* o)
    {
        name = NULL;
        setObjectRef(o);
    }

    value(std::string& s)
    {
        name = NULL;
        setString(s.c_str());
    }

    value(int i)
    {
        name = NULL;
        setInteger(i);
    }

    value(unsigned int i)
    {
        name = NULL;
        clear();
        type = typeUint32;
        val_uint32 = i;
    }

    value(xcm_int64 i)
    {
        name = NULL;
        setInt64(i);
    }

    value(xcm_uint64 i)
    {
        name = NULL;
        clear();
        type = typeUint64;
        val_uint64 = i;
    }

    ~value()
    {
        clear();
    }

    void clear();
    

    operator value*()
    {
        return this;
    }

    operator const char*()
    {
        return val_string;
    }

    operator int()
    {
        return val_int32;
    }

    operator unsigned int()
    {
        return val_uint32;
    }

    operator bool()
    {
        return val_bool;
    }

    operator double()
    {   
        return val_double;
    }

    operator xcm_int64()
    {
        return val_int64;
    }

    operator xcm_uint64()
    {
        return val_uint64;
    }

    int getType()
    {
        return type;
    }

    int getException()
    {
        if (type != typeException)
            return exNoException;

        return val_int32;
    }

    void setType(int new_type)
    {
        type = new_type;
    }

    const char* getString()
    {
        return val_string;
    }

    void setString(const char* s)
    {
        clear();
        type = typeString;
        val_string = strdup(s);
    }

    int getInteger()
    {
        return val_int32;
    }

    void setInteger(int i)
    {
        clear();
        type = typeInt32;
        val_int32 = i;
    }

    void setDouble(double d)
    {
        clear();
        type = typeDouble;
        val_double = d;
    }

    void setInt64(xcm_int64 i)
    {
        clear();
        type = typeInt64;
        val_int64 = i;
    }

    double getDouble()
    {
        return val_double;
    }

    bool getBoolean()
    {
        return val_bool;
    }

    void setBoolean(bool b)
    {
        clear();
        type = typeBoolean;
        val_bool = b;
    }

    // error checking
    void setException(int exception_type)
    {
        if (exception_type == exNoException)
        {
            type = typeVoid;
            val_int32 = 0;
        }
         else
        {
            type = typeException;
            val_int32 = exception_type;
        }
    }

    bool success()
    {
        return (type != typeException);
    }

    bool failure()
    {
        return (type == typeException);
    }

    // array/structure support

    size_t getArraySize()
    {
        return arr.size();
    }

    value* getArrayElement(int index)
    {
        return arr[index];
    }

    void addArrayElement(value v)
    {
        addStructElement(NULL, v);
    }

    void addStructElement(const char* name, value v)
    {
        value* newv = new value;
        *newv = v;
        if (name)
        {
            newv->name = strdup(name);
        }
        arr.push_back(newv);
    }

    value* getStructElement(const char* name)
    {
        for (std::vector<value*>::iterator it = arr.begin();
            it != arr.end(); it++)
        {
            if (!strcmp((*it)->name, name))
            {
                return *it;
            }
        }
        return NULL;
    }

    // object support
    template <class T>
    void setObjectRef(T* obj)
    {
        clear();
        type = typeObjectRef;
        val_objref = (ObjRef)obj;
    }

    void setObjectRef(ObjRef objref)
    {
        clear();
        type = typeObjectRef;
        val_objref = objref;
    }

    ObjRef getObjectRef()
    {
        return val_objref;
    }
    
public:
    int type;
    char* name;
    std::vector<value*> arr;

    union
    {
        bool val_bool;
        int val_int32;
        unsigned int val_uint32;
        xcm_int64 val_int64;
        xcm_uint64 val_uint64;
        char* val_string;
        double val_double;
        ObjRef val_objref;
    };
};

typedef value Object;
typedef value Structure;



};



#endif

