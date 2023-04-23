/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-06-12
 *
 */


#ifndef H_KSCRIPT_JSARRAY_H
#define H_KSCRIPT_JSARRAY_H


#include "jsobject.h"


namespace kscript
{


class Array : public Object
{
    BEGIN_KSCRIPT_DERIVED_CLASS("Array", Array, Object)
        KSCRIPT_METHOD("constructor", Array::constructor)
        KSCRIPT_METHOD("toString", Array::toString)
        KSCRIPT_METHOD("toLocaleString", Array::toLocaleString)
        KSCRIPT_STATIC_METHOD("concat", Array::concat)
        KSCRIPT_STATIC_METHOD("join", Array::join)
        KSCRIPT_STATIC_METHOD("pop", Array::pop)
        KSCRIPT_STATIC_METHOD("push", Array::push)
        KSCRIPT_STATIC_METHOD("reverse", Array::reverse)
        KSCRIPT_STATIC_METHOD("shift", Array::shift)
        KSCRIPT_STATIC_METHOD("slice", Array::slice)
        KSCRIPT_STATIC_METHOD("sort", Array::sort)
        KSCRIPT_STATIC_METHOD("splice", Array::splice)    
        KSCRIPT_STATIC_METHOD("unshift", Array::unshift)
    END_KSCRIPT_CLASS()
    
    Array();
    ~Array();
    
    static void staticConstructor(ExprEnv* env, void* param, Value* retval);
    
    void constructor(ExprEnv* env, Value* retval);
    void toString(ExprEnv* env, Value* retval);
    void toLocaleString(ExprEnv* env, Value* retval);
    
    static void concat(ExprEnv* env, void*, Value* retval);
    static void join(ExprEnv* env, void*, Value* retval);
    static void pop(ExprEnv* env, void*, Value* retval);
    static void push(ExprEnv* env, void*, Value* retval);
    static void reverse(ExprEnv* env, void*, Value* retval);
    static void slice(ExprEnv* env, void*, Value* retval);
    static void shift(ExprEnv* env, void*, Value* retval);
    static void sort(ExprEnv* env, void*, Value* retval);
    static void splice(ExprEnv* env, void*,Value* retval);
    static void unshift(ExprEnv* env, void* , Value* retval);
    
    Value* getMember(const std::wstring& name);
    
    void toString(Value* val);
    
private:

    static void reassignMemberIndexes(ValueObject* vthis);
    static void makeStringRetval(ValueObject* vthis,
                                 const std::wstring& delim,
                                 Value* retval);
    
    static void makeStringRetvalInternal(
                                 ValueObject* vthis,
                                 const std::wstring& delim,
                                 Value* retval,
                                 std::set<ValueObject*>& forbidden);
};


};


#endif

