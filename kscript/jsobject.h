/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-06-16
 *
 */


#ifndef __KSCRIPT_JSOBJECT_H
#define __KSCRIPT_JSOBJECT_H


namespace kscript
{


class Object : public ValueObject
{
    BEGIN_KSCRIPT_CLASS("Object", Object)
        KSCRIPT_METHOD("constructor", Object::constructor)
        KSCRIPT_STATIC_METHOD("toString", Object::toString)
        KSCRIPT_STATIC_METHOD("toLocaleString", Object::toLocaleString)
        KSCRIPT_STATIC_METHOD("valueOf", Object::valueOf)
        KSCRIPT_STATIC_METHOD("hasOwnProperty", Object::hasOwnProperty)
        KSCRIPT_STATIC_METHOD("propertyIsEnumerable", Object::propertyIsEnumerable)
        KSCRIPT_STATIC_METHOD("isPrototypeOf", Object::isPrototypeOf)
    END_KSCRIPT_CLASS()

public:

    Object();
    ~Object();
    
    void constructor(ExprEnv* env, Value* retval);
    static void staticConstructor(ExprEnv* env, void* param, Value* retval);
    static bool customConstructor(int class_id, ExprEnv* env, void* param, Value* retval);

    static void toString(ExprEnv* env, void* param, Value* retval);
    static void toLocaleString(ExprEnv* env, void* param, Value* retval);
    static void valueOf(ExprEnv* env, void* param, Value* retval);
    static void hasOwnProperty(ExprEnv* env, void* param, Value* retval);
    static void propertyIsEnumerable(ExprEnv* env, void* param, Value* retval);
    static void isPrototypeOf(ExprEnv* env, void* param, Value* retval);
    
    void toString(Value* val);
};


};


#endif

