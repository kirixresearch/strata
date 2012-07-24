/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-01-23
 *
 */


#ifndef __KSCRIPT_JSMATH_H
#define __KSCRIPT_JSMATH_H


namespace kscript
{


class Math : public ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("Math", Math)
        KSCRIPT_STATIC_METHOD("abs", Math::abs)
        KSCRIPT_STATIC_METHOD("acos", Math::acos)
        KSCRIPT_STATIC_METHOD("asin", Math::asin)
        KSCRIPT_STATIC_METHOD("atan", Math::atan)
        KSCRIPT_STATIC_METHOD("atan2", Math::atan2)
        KSCRIPT_STATIC_METHOD("ceil", Math::ceil)
        KSCRIPT_STATIC_METHOD("cos", Math::cos)
        KSCRIPT_STATIC_METHOD("exp", Math::exp)
        KSCRIPT_STATIC_METHOD("floor", Math::floor)
        KSCRIPT_STATIC_METHOD("log", Math::log)
        KSCRIPT_STATIC_METHOD("max", Math::max)
        KSCRIPT_STATIC_METHOD("min", Math::min)
        KSCRIPT_STATIC_METHOD("pow", Math::pow)
        KSCRIPT_STATIC_METHOD("random", Math::random)
        KSCRIPT_STATIC_METHOD("round", Math::round)
        KSCRIPT_STATIC_METHOD("sin", Math::sin)
        KSCRIPT_STATIC_METHOD("sqrt", Math::sqrt)
        KSCRIPT_STATIC_METHOD("tan", Math::tan)
    END_KSCRIPT_CLASS()

    void constructor(ExprEnv* env, Value* retval);
    static void bindStatic(ExprParser* parser);
    static void abs(ExprEnv* env, void* param, Value* retval);
    static void acos(ExprEnv* env, void* param, Value* retval);
    static void asin(ExprEnv* env, void* param, Value* retval);
    static void atan(ExprEnv* env, void* param, Value* retval);
    static void atan2(ExprEnv* env, void* param, Value* retval);
    static void ceil(ExprEnv* env, void* param, Value* retval);
    static void cos(ExprEnv* env, void* param, Value* retval);
    static void exp(ExprEnv* env, void* param, Value* retval);
    static void floor(ExprEnv* env, void* param, Value* retval);
    static void log(ExprEnv* env, void* param, Value* retval);
    static void max(ExprEnv* env, void* param, Value* retval);
    static void min(ExprEnv* env, void* param, Value* retval);
    static void pow(ExprEnv* env, void* param, Value* retval);
    static void random(ExprEnv* env, void* param, Value* retval);
    static void round(ExprEnv* env, void* param, Value* retval);
    static void sin(ExprEnv* env, void* param, Value* retval);
    static void sqrt(ExprEnv* env, void* param, Value* retval);
    static void tan(ExprEnv* env, void* param, Value* retval);
};




};


#endif

