/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-05-27
 *
 */


#ifndef __KSCRIPT_JSSTRING_H
#define __KSCRIPT_JSSTRING_H


#include "jsobject.h"


namespace kscript
{


class String : public Object
{
    BEGIN_KSCRIPT_DERIVED_CLASS("String", String, Object)
        KSCRIPT_METHOD("constructor", String::constructor)
        KSCRIPT_STATIC_METHOD("valueOf",           String::toString)
        KSCRIPT_STATIC_METHOD("toString",          String::toString)
        KSCRIPT_STATIC_METHOD("charAt",            String::charAt)
        KSCRIPT_STATIC_METHOD("charCodeAt",        String::charCodeAt)
        KSCRIPT_STATIC_METHOD("concat",            String::concat)
        KSCRIPT_STATIC_METHOD("indexOf",           String::indexOf)
        KSCRIPT_STATIC_METHOD("lastIndexOf",       String::lastIndexOf)
        KSCRIPT_STATIC_METHOD("localeCompare",     String::localeCompare)
        KSCRIPT_STATIC_METHOD("match",             String::match)
        KSCRIPT_STATIC_METHOD("replace",           String::replace)
        KSCRIPT_STATIC_METHOD("search",            String::search)   
        KSCRIPT_STATIC_METHOD("slice",             String::slice)
        KSCRIPT_STATIC_METHOD("split",             String::split) 
        KSCRIPT_STATIC_METHOD("substring",         String::substring)
        KSCRIPT_STATIC_METHOD("toLowerCase",       String::toLowerCase)
        KSCRIPT_STATIC_METHOD("toUpperCase",       String::toUpperCase)
        KSCRIPT_STATIC_METHOD("toLocaleUpperCase", String::toUpperCase)
        KSCRIPT_STATIC_METHOD("toLocaleLowerCase", String::toLowerCase)    
        KSCRIPT_STATIC_METHOD("trim",              String::trim)
        KSCRIPT_STATIC_METHOD("setCharAt",         String::setCharAt)
        KSCRIPT_STATIC_METHOD("substr",            String::substr)
        KSCRIPT_STATIC_METHOD("compare",           String::compare)
        KSCRIPT_STATIC_METHOD("frequency",         String::frequency)
        KSCRIPT_STATIC_METHOD          ("fromCharCode",      String::fromCharCode)
        KSCRIPT_STATIC_METHOD          ("sprintf",           String::sprintf)
    END_KSCRIPT_CLASS()
    
public:

    String();
        
    static void staticConstructor(
                            ExprEnv* env,
                            void* param,
                            Value* retval);
                            
    static Value* lookupStringMethod(const std::wstring& method);
                             
    void constructor(ExprEnv* env, Value* retval);

    Value* getMember(const std::wstring& name);
    
    void toString(Value* retval);
    void toNumber(Value* retval);
    void toPrimitive(Value* retval, int preferred_type);
    
    
    
    //  functions in the standard
    static void fromCharCode(ExprEnv* env, void* param, Value* retval);
    static void toString(ExprEnv* env, void* param, Value* retval);
    static void charAt(ExprEnv* env, void* param, Value* retval);
    static void charCodeAt(ExprEnv* env, void* param, Value* retval);
    static void concat(ExprEnv* env, void* param, Value* retval);
    static void indexOf(ExprEnv* env, void* param, Value* retval);
    static void lastIndexOf(ExprEnv* env, void* param, Value* retval);
    static void localeCompare(ExprEnv* env, void* param, Value* retval);
    static void match(ExprEnv* env, void* param, Value* retval);
    static void replace(ExprEnv* env, void* param, Value* retval);
    static void search(ExprEnv* env, void* param, Value* retval);
    static void setCharAt(ExprEnv* env, void* param, Value* retval);
    static void slice(ExprEnv* env, void* param, Value* retval);
    static void split(ExprEnv* env, void* param, Value* retval);
    static void substring(ExprEnv* env, void* param, Value* retval);
    static void toLowerCase(ExprEnv* env, void* param, Value* retval);
    static void toUpperCase(ExprEnv* env, void* param, Value* retval);
    static void trim(ExprEnv* env, void* param, Value* retval);
    static void sprintf(ExprEnv* env, void* param, Value* retval);

    // functions not in the standard
    static void substr(ExprEnv* env, void* param, Value* retval);
    static void compare(ExprEnv* env, void* param, Value* retval);
    static void frequency(ExprEnv* env, void* param, Value* retval);

public:

    Value val;
};




// other string-related functions
void js_escape(ExprEnv* env, void* param, Value* retval);
void js_encodeURI(ExprEnv* env, void* param, Value* retval);
void js_encodeURIComponent(ExprEnv* env, void* param, Value* retval);
void js_unescape(ExprEnv* env, void* param, Value* retval);
void js_decodeURI(ExprEnv* env, void* param, Value* retval);
void js_decodeURIComponent(ExprEnv* env, void* param, Value* retval);



};


#endif

