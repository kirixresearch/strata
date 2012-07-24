/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-05-27
 *
 */


#ifndef __KSCRIPT_JSNUMBER_H
#define __KSCRIPT_JSNUMBER_H


#include "jsobject.h"


namespace kscript
{


class Number : public Object
{
    BEGIN_KSCRIPT_DERIVED_CLASS("Number", Number, Object)
        KSCRIPT_METHOD("constructor", Number::constructor)
        KSCRIPT_STATIC_METHOD("toExponential", Number::toExponential)
        KSCRIPT_STATIC_METHOD("toFixed", Number::toFixed)
        KSCRIPT_STATIC_METHOD("toPrecision", Number::toPrecision)
        KSCRIPT_STATIC_METHOD("valueOf", Number::valueOf)
        KSCRIPT_STATIC_METHOD("toString", Number::toString)
        KSCRIPT_STATIC_METHOD("toLocaleString", Number::toLocaleString)
    END_KSCRIPT_CLASS()
    
public:

    Number();
    
    static void bindStatic(ExprParser* parser);
    static void staticConstructor(ExprEnv* env,
                                  void* param,
                                  Value* retval);
                                  
    void constructor(ExprEnv* env, Value* retval);

    Value* getMember(const std::wstring& name);


    //  functions in the standard
    static void toExponential(ExprEnv* env, void* param, Value* retval);
    static void toFixed(ExprEnv* env, void* param, Value* retval);
    static void toPrecision(ExprEnv* env, void* param, Value* retval);
    static void valueOf(ExprEnv* env, void* param, Value* retval);
    static void toString(ExprEnv* env, void* param, Value* retval);
    static void toLocaleString(ExprEnv* env, void* param, Value* retval);

    void toNumber(Value* retval);
    void toString(Value* retval);
    void toPrimitive(Value* retval, int preferred_type);

    static Value* lookupNumberMethod(const std::wstring& method);

public:

    Value val;
};




};


#endif

