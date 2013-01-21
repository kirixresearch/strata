/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-05-27
 *
 */


#ifndef __KSCRIPT_JSBOOLEAN_H
#define __KSCRIPT_JSBOOLEAN_H


#include "jsobject.h"


namespace kscript
{


class Boolean : public Object
{
    BEGIN_KSCRIPT_DERIVED_CLASS("Boolean", Boolean, Object)

        KSCRIPT_METHOD("constructor", Boolean::constructor)
        KSCRIPT_STATIC_METHOD("valueOf", Boolean::valueOf)
        KSCRIPT_STATIC_METHOD("toString", Boolean::toString)
        
        KSCRIPT_CONSTANT_INTEGER("length", 1);

    END_KSCRIPT_CLASS()

public:

    Boolean();
    
    void constructor(ExprEnv* env, Value* retval);
    static void staticConstructor(kscript::ExprEnv* env,
                                  void* param,
                                  kscript::Value* retval);
    static Value* lookupBooleanMethod(const std::wstring& method);
        
    static void toString(ExprEnv* env, void* param, Value* retval);
    static void valueOf(ExprEnv* env, void* param, Value* retval);

    void toPrimitive(Value* retval, int preferred_type);
    void toNumber(Value* retval);
    void toString(Value* retval);

public:

    Value val;
};





};


#endif

