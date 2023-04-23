/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-05-15
 *
 */


#ifndef H_KSCRIPT_JSREGEXP_H
#define H_KSCRIPT_JSREGEXP_H


#include "../kl/include/kl/regex.h"


#include "jsobject.h"


namespace kscript
{


class RegExp : public Object
{
public:

    BEGIN_KSCRIPT_DERIVED_CLASS("RegExp", RegExp, Object)
        KSCRIPT_METHOD("constructor", RegExp::constructor)
        KSCRIPT_METHOD("exec", RegExp::exec)
        KSCRIPT_METHOD("test", RegExp::test)
        KSCRIPT_METHOD("toString", RegExp::toString)
    END_KSCRIPT_CLASS()

public:

    RegExp();
    ~RegExp();
    
    static void staticConstructor(ExprEnv* env, void* param, Value* retval);

    void constructor(ExprEnv* env, Value* retval);
    void exec(ExprEnv* env, Value* retval);
    void test(ExprEnv* env, Value* retval);
    void toString(ExprEnv* env, Value* retval);

    void toString(Value* val);

    bool assign(const std::wstring& expr, const std::wstring& flags);
    
public:

    klregex::wregex m_regex;
};




};


#endif

