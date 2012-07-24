/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-01-23
 *
 */


#include "kscript.h"
#include "jsmath.h"
#include "util.h"
#include <cmath>


namespace kscript
{


// (CLASS) Math
// Category: Core
// Description: A collection of math-related static functions.
// Remarks: The Math object provides common math functionality through 
//     a collection of static functions on the object.

void Math::constructor(ExprEnv* env, Value* retval)
{
}

// TODO: document math constants

void Math::bindStatic(ExprParser* parser)
{
    unsigned char attrs = Value::attrDontDelete | Value::attrDontEnum | Value::attrReadOnly;

    ValueObject* obj = parser->getClassInfo(L"Math")->getClassObject()->getObject();
    obj->createMember(L"E", attrs)->setValue(2.7182818284590452354);
    obj->createMember(L"LN10", attrs)->setValue(2.302585092994046);
    obj->createMember(L"LN2", attrs)->setValue(0.6931471805599453);
    obj->createMember(L"LOG2E", attrs)->setValue(1.4426950408889634);
    obj->createMember(L"LOG10E", attrs)->setValue(0.4342944819032518);
    obj->createMember(L"SQRT1_2", attrs)->setValue(0.7071067811865476);
    obj->createMember(L"SQRT2", attrs)->setValue(1.4142135623730951);
    obj->createMember(L"PI", attrs)->setValue(3.1415926535897932);
}

// (METHOD) Math.abs
// Description: Returns the absolute value of a number.
//
// Syntax: static function Math.abs(number : Number) : Number
//
// Remarks: Returns the absolute value of a |number|.
//
// Param(number): The |number| for which to return the absolute value.
//
// Returns: Returns the absolute value of a |number|.

void Math::abs(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    Value* p0 = env->getParam(0);

    if (p0->getType() == kscript::Value::typeDouble)
    {
        retval->setDouble(::fabs(p0->getDouble()));
    }
     else if (p0->getType() == kscript::Value::typeInteger)
    {
        retval->setInteger(::abs(p0->getInteger()));
    }
     else
    {
        Value n;
        p0->toNumber(&n);
        
        retval->setDouble(::fabs(n.getDouble()));
    }
}

// (METHOD) Math.acos
// Description: Returns the arc cosine of a number.
//
// Syntax: static function Math.acos(number : Number) : Number
//
// Remarks: Returns the arc cosine of a |number|, where |number| must
//     be between -1 and 1.  If |number| is not between -1 and 1, the
//     function returns NaN.
//
// Param: The |number| for which to return the arc cosine.
//
// Returns: Returns the arc cosine of a |number|.

void Math::acos(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(::acos(env->getParam(0)->getDouble()));
}

// (METHOD) Math.asin
// Description: Returns the arc sine of a number.
//
// Syntax: static function Math.asin(number : Number) : Number
//
// Remarks: Returns the arc sine of a |number|, where |number| must
//     be between -1 and 1.  If |number| is not between -1 and 1, the
//     function returns NaN.
//
// Param(number): The |number| for which to return the arc sine.
//
// Returns: Returns the arc sine of a |number|.

void Math::asin(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(::asin(env->getParam(0)->getDouble()));
}

// (METHOD) Math.atan
// Description: Returns the arc tangent of a number.
//
// Syntax: static function Math.atan(number : Number) : Number
//
// Remarks: Returns the arc tangent of a |number|.
//
// Param(number): The |number| for which to return the arc tangent.
//
// Returns: Returns the arc tangent of a |number|.

void Math::atan(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(::atan(env->getParam(0)->getDouble()));
}

// (METHOD) Math.atan2
// Description: Returns the arc tangent of a number.
//
// Syntax: static function Math.atan2(y : Number,
//                                    x : Number) : Number
//
// Remarks: Returns the arc tangent of y/x.
//
// Param(x): The y-coordinate.
// Param(y): The x-coordinate.
//
// Returns: Returns the arc tangent of y/x.

void Math::atan2(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 2)
    {
        retval->setNaN();
        return;
    }
    
    Value* v0 = env->getParam(0);
    Value* v1 = env->getParam(1);

    double p0 = env->getParam(0)->getDouble();
    double p1 = env->getParam(1)->getDouble();

    if (v0->isPositiveInfinity())
    {
        if (v1->isPositiveInfinity())
        {
            retval->setDouble(3.141592653589793/4);
            return;
        }
         else if (v1->isNegativeInfinity())
        {
            retval->setDouble(3*3.141592653589793/4);
            return;
        }
    }
     else if (v0->isNegativeInfinity())
    {
        if (v1->isPositiveInfinity())
        {
            retval->setDouble(-3.141592653589793/4);
            return;
        }
         else if (v1->isNegativeInfinity())
        {
            retval->setDouble(-3*3.141592653589793/4);
            return;
        }
    }
    
    
    retval->setDouble(::atan2(p0, p1));
}

// (METHOD) Math.ceil
// Description: Returns the smallest integer greater than a number.
//
// Syntax: static function Math.ceil(number : Number) : Integer
//
// Remarks: Returns the smallest integer greater than a |number|.
//
// Param(number): The |number| for which to find the smallest integer greater than it.
//
// Returns: Returns the smallest integer greater than the |number|.

void Math::ceil(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setDouble(0.0);
        return;
    }

    retval->setDouble(::ceil(env->getParam(0)->getDouble()));
}

// (METHOD) Math.cos
// Description: Returns the cosine of a number.
//
// Syntax: static function Math.cos(number : Number) : Number
//
// Remarks: Returns the cosine of a |number|.
//
// Param(number): The |number| for which to return the cosine.
//
// Returns: Returns the cosine of a |number|.

void Math::cos(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(::cos(env->getParam(0)->getDouble()));
}

// (METHOD) Math.exp
// Description: Returns e to the power of a number.
//
// Syntax: static function Math.exp(number : Number) : Number
//
// Remarks: Returns e to the power of a |number|.
//
// Param(number): The |number| to raise e to the power of.
//
// Returns: Returns e to the power of a |number|.

void Math::exp(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(::exp(env->getParam(0)->getDouble()));
}

// (METHOD) Math.floor
// Description: Returns the largest integer less than a number.
//
// Syntax: static function Math.floor(number : Number) : Integer
//
// Remarks: Returns the largest integer less than a |number|.
//
// Param(number): The |number| for which to find the largest integer less than it.
//
// Returns: Returns the largest integer less than a |number|.

void Math::floor(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(::floor(env->getParam(0)->getDouble()));
}

// (METHOD) Math.log
// Description: Returns the natural logarithm of a number.
//
// Syntax: static function Math.log(number : Number) : Number
//
// Remarks: Returns the natural logarithm of a |number|, when the |number|
//     is greater than 0.  If the |number| is less than 0, the function returns
//     NaN.
//
// Param: The |number| for which to find the natural logarithm.
//
// Returns: Returns the natural logarithm of a |number|.

void Math::log(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(::log(env->getParam(0)->getDouble()));
}

// (METHOD) Math.max
// Description: Returns the maximum number from a series of numbers.
//
// Syntax: static function Math.max(num_0 : Number, 
//                                  ..., 
//                                  num_n : Number) : Number
//
// Remarks: Returns the maximum number of a series of input numbers, specified
//     by |num_0|, ..., |num_n|.  If no input numbers are specified, the function
//     returns -Infinity.
//
// Param(num_0, ..., num_n): A series of numbers for which to find the maximum.
//
// Returns: The maximum number of a series of numbers.

void Math::max(ExprEnv* env, void* param, Value* retval)
{
    size_t i, param_count = env->getParamCount();
    int run_int, test_int;
    double run_dbl, test_dbl;
    bool all_ints = true;

    if (param_count == 0)
    {
        retval->setNegativeInfinity();
        return;
    }

    for (i = 0; i < param_count; ++i)
    {
        Value* param = env->getParam(i);
        if (param->getType() != Value::typeInteger)
            all_ints = false;

        if (param->isNaN())
        {
            retval->setNaN();
            return;
        }

        test_int = param->getInteger();
        test_dbl = param->getDouble();

        if (i == 0)
        {
            run_int = test_int;
            run_dbl = test_dbl;
        }
         else
        {
            if (test_int > run_int)
                run_int = test_int;
            if (test_dbl > run_dbl)
                run_dbl = test_dbl;
        }
    }

    if (all_ints)
    {
        retval->setInteger(run_int);
    }
     else
    {
        retval->setDouble(run_dbl);
    }
}

// (METHOD) Math.min
// Description: Returns the minimum number from a series of numbers.
//
// Syntax: static function Math.min(num_0 : Number, 
//                                  ..., 
//                                  num_n : Number) : Number
//
// Remarks: Returns the minimum number of a series of input numbers, specified
//     by |num_0|, ..., |num_n|.  If no input numbers are specified, the function
//     returns +Infinity.
//
// Param(num_0, ..., num_n): A series of numbers for which to find the minimum.
//
// Returns: The minimum number of a series of numbers.

void Math::min(ExprEnv* env, void* param, Value* retval)
{
    size_t i, param_count = env->getParamCount();
    int run_int, test_int;
    double run_dbl, test_dbl;
    bool all_ints = true;

    if (param_count == 0)
    {
        retval->setPositiveInfinity();
        return;
    }
    
    for (i = 0; i < param_count; ++i)
    {
        Value* param = env->getParam(i);
        if (param->getType() != Value::typeInteger)
            all_ints = false;

        if (param->isNaN())
        {
            retval->setNaN();
            return;
        }
        
        test_int = param->getInteger();
        test_dbl = param->getDouble();

        if (i == 0)
        {
            run_int = test_int;
            run_dbl = test_dbl;
        }
         else
        {
            if (test_int < run_int)
                run_int = test_int;
            if (test_dbl < run_dbl)
                run_dbl = test_dbl;
        }
    }

    if (all_ints)
    {
        retval->setInteger(run_int);
    }
     else
    {
        retval->setDouble(run_dbl);
    }
}

// (METHOD) Math.pow
// Description: Returns a number raised to the power of another number.
//
// Syntax: static function Math.pow(base : Number,
//                                  exponent : Number) : Number
//
// Remarks: Returns |base| to the power of |exponent|.
//
// Param(base): The |base|.
// Param(exponent): The |exponent|.
//
// Returns: Returns |base| to the power of |exponent|.

void Math::pow(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 2)
    {
        retval->setNaN();
        return;
    }
    
    Value* p0 = env->getParam(0);
    Value* p1 = env->getParam(1);
    
    double d0 = env->getParam(0)->getDouble();
    double d1 = env->getParam(1)->getDouble();

    if (p1->isNaN())
    {
        retval->setNaN();
        return;
    }
    
    if (p0->isNaN())
    {
        if (0 == dblcompare(d1, 0.0))
            retval->setDouble(1.0);
             else
            retval->setNaN();
        return;
    }

    if (dblcompare(::fabs(d0), 1.0) == 0)
    {
        if (p1->isAnyInfinity())
        {
            retval->setNaN();
            return;
        }
    }

    retval->setDouble(::pow(d0, d1));
}

// (METHOD) Math.random
// Description: Returns a random number.
//
// Syntax: static function Math.random() : Number
//
// Remarks: Returns a random number between 0 and 1.
//
// Returns: Returns a random number between 0 and 1.

void Math::random(ExprEnv* env, void* param, Value* retval)
{
    double i;
    double d = 0.0;
    d = rand();
    d = d*d;
    d = modf(d/1000000, &i);

    retval->setDouble(d);
}

// (METHOD) Math.round
// Description: Returns the integer nearest to a number.
//
// Syntax: static function Math.round(number : Number) : Integer
//
// Remarks: Returns the integer nearest to the specified |number|. If
//     the given number is exactly half way between two integers, the
//     function will round up, i.e. X.5 becomes X+1.
//
// Returns: Returns the integer nearest to the specified |number|.

void Math::round(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    Value* p0 = env->getParam(0);

    if (p0->isNaN())
    {
        retval->setNaN();
        return;
    }

    if (p0->getType() == Value::typeInteger)
    {
        retval->setInteger(p0->getInteger());
    }
     else if (p0->getType() == Value::typeDouble)
    {
        double d = p0->getDouble();
        d = d + 0.5;
        d = ::floor(d);
        retval->setDouble(d);
    }
     else
    {
        retval->setInteger(0);
    }

}

// (METHOD) Math.sin
// Description: Returns the sine of a number.
//
// Syntax: static function Math.sin(number : Number) : Number
//
// Remarks: Returns the sine of a |number|.
//
// Param(number): The |number| for which to return the sine.
//
// Returns: Returns the sine of a |number|.

void Math::sin(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(::sin(env->getParam(0)->getDouble()));
}

// (METHOD) Math.sqrt
// Description: Returns the square root of a number.
//
// Syntax: static function Math.sqrt(number : Number) : Number
//
// Remarks: Returns the square root of a |number|. If the |number|
//     is less than 0, the function returns NaN.
//
// Param(number): The |number| for which to calculate the square root.
//
// Returns: Returns the square root of a |number|.

void Math::sqrt(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(::sqrt(env->getParam(0)->getDouble()));
}

// (METHOD) Math.tan
// Description: Returns the tangent of a number.
//
// Syntax: static function Math.tan(number : Number) : Number
//
// Remarks: Returns the tangent of a |number|.
//
// Param(number): The |number| for which to return the tangent.
//
// Returns: Returns the tangent of a |number|.

void Math::tan(ExprEnv* env, void* param, Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(::tan(env->getParam(0)->getDouble()));
}



}
