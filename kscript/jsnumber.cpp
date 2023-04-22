/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-01-23
 *
 */


#include "kscript.h"
#include "jsnumber.h"
#include "util.h"
#include <cmath>
#include <limits>


#ifdef _MSC_VER
#define swprintf _snwprintf
#endif


// remove VC9 warnings - eventually commenting this
// out and implementing specific fixes for these warnings
// is a good idea.
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


namespace kscript
{


static Value* getThisNumber(ExprEnv* env)
{
    Value* ethis = env->getThis();
    if (!ethis)
        return NULL;
        
    if (ethis->isObject())
    {
        ValueObject* vobj = ethis->getObject();
        if (vobj->isKindOf(Number::staticGetClassId()))
            return &((Number*)vobj)->val;
    }
    
    return ethis;
}


// (CLASS) Number
// Category: Core
// Derives: Object
// Description: A class that represents numeric-typed data.
// Remarks: A wrapper class for the numeric-typed data.

Number::Number()
{
    val.setDouble(0.0);
}

// (CONSTRUCTOR) Number.constructor
// Description: Creates a new Number object.
//
// Syntax: Number()
// Syntax: Number(value : Number)
//
// Remarks: Creates a new Number object from |value|. If no value is
//     specified, the initial |value| of the Number object is 0.
//
// Param(value): The |value| of the new Number object.

void Number::constructor(ExprEnv* env, Value* retval)
{
    if (env->getParamCount() == 0)
    {
        val.setDouble(0.0);
        return;
    }
        
    Value* param = env->getParam(0);
    if (param->isUndefined())
        val.setNaN();
    else if (param->isNull())
        val.setDouble(0.0);
    else
        param->toNumber(&val);
}

void Number::staticConstructor(ExprEnv* env, void*, Value* retval)
{
    if (env->getParamCount() == 0)
    {
        retval->setDouble(0.0);
        return;
    }
    
    Value* param = env->getParam(0);
    if (param->isUndefined())
        retval->setNaN();
    else if (param->isNull())
        retval->setDouble(0.0);
    else
        param->toNumber(retval);
}

// TODO: document constant parameters
void Number::bindStatic(ExprParser* parser)
{
    unsigned char attrs = Value::attrDontDelete | Value::attrDontEnum | Value::attrReadOnly;
    
    ValueObject* obj = parser->getClassInfo(L"Number")->getClassObject()->getObject();
    Value value;

    // NaN
    value.setNaN();
    obj->createMember(L"NaN", attrs)->setValue(value).setAttributes(attrs);
    
    // Infinity
    value.setPositiveInfinity();
    obj->createMember(L"POSITIVE_INFINITY", attrs)->setValue(value).setAttributes(attrs);
    obj->createMember(L"Infinity", attrs)->setValue(value).setAttributes(attrs);
    
    // Negative Infinity
    value.setNegativeInfinity();
    obj->createMember(L"NEGATIVE_INFINITY", attrs)->setValue(value).setAttributes(attrs);

    // Maximum Value
    value.setDouble(1.7976931348623157e308);
    obj->createMember(L"MAX_VALUE", attrs)->setValue(value).setAttributes(attrs);

    // Minimum Value
    value.setDouble(5e-324);
    obj->createMember(L"MIN_VALUE", attrs)->setValue(value).setAttributes(attrs);
}

Value* Number::getMember(const std::wstring& name)
{
    if (ValueObject::getMemberExists(name))
        return ValueObject::getMember(name);
            
    Value* member = ValueObject::getMember(name);
    
    Value* func = lookupNumberMethod(name);
    if (func == NULL)
        return member;
        
    member->setValue(func);
    return member;
}




struct NumberMethod
{
    const wchar_t* name;
    Value val;
    
    void set(const wchar_t* _name, ExprBindFunc _func)
    {
        name = _name;
        val.setFunction(_func);
    }
};

const int NUMBER_METHOD_COUNT = 6;


//static
Value* Number::lookupNumberMethod(const std::wstring& method)
{
    static NumberMethod number_methods[NUMBER_METHOD_COUNT];
    if (number_methods[0].val.isUndefined())
    {
       number_methods[0].set(L"valueOf",        Number::valueOf);
       number_methods[1].set(L"toFixed",        Number::toFixed);
       number_methods[2].set(L"toPrecision",    Number::toPrecision);
       number_methods[3].set(L"toExponential",  Number::toExponential);
       number_methods[4].set(L"toString",       Number::toString);
       number_methods[5].set(L"toLocaleString", Number::toLocaleString);
       // when you add a method, make sure to increment NUMBER_METHOD_COUNT above
    }
    
    for (size_t i = 0; i < NUMBER_METHOD_COUNT; ++i)
    {
        if (method == number_methods[i].name)
            return &number_methods[i].val;
    }
    
    return NULL;
}






// (METHOD) Number.valueOf
// Description: Returns the primitive number value represented by 
//     the number.
//
// Syntax: function Number.valueOf() : Number
//
// Remarks: Returns the primitive number value represented by 
//     the number.
//
// Returns: Returns the primitive number value represented by 
//     the number.

void Number::valueOf(ExprEnv* env, void* param, Value* retval)
{
    Value* val = getThisNumber(env);
    if (val)
        val->eval(NULL, retval);
         else
        retval->setNaN();
}


// (METHOD) Number.toString
// Description: Returns a string that represents the number.
//
// Syntax: function Number.toString() : String
// Syntax: function Number.toString(base : Number) : String
//
// Remarks: Returns a string that represents the number in a given
//     |base|. If |base| is omitted, then base 10 is used to create
//     the string that represents the number.
//
// Param(base): The |base| in which the number is to be shown.
//
// Returns: Returns a string that represents the number.


void Number::toString(ExprEnv* env, void* param, Value* retval)
{
    int radix = 10;

    if (env->getParamCount() > 0)
    {
        Value* p = env->getParam(0);
        if (!p->isUndefined())
            radix = p->getInteger();
    }

    // if the radix is outside the bounds, set a range error
    if (radix < 2 || radix > 36)
    {
        env->setRuntimeError(rterrorRangeError);
        return;
    }
    
    // if we don't have a value to convert, we're done
    Value* val = getThisNumber(env);
    if (!val)
    {
        retval->setNull();
        return;
    }

    double number = val->getDouble();

    // if the radix is base 10, or the number is zero, NaN,
    // or +-infinity, represent the number as usual
    if (radix == 10 || number == 0 || 
        val->isNaN() || val->isAnyInfinity())
    {
        val->toString(retval);
        return;
    }

    // create an array of digits for bases between 2 and 36
    static const wchar_t* digits[36] = {L"0", L"1", L"2", L"3", L"4", L"5",
                                        L"6", L"7", L"8", L"9", L"a", L"b",
                                        L"c", L"d", L"e", L"f", L"g", L"h",
                                        L"i", L"j", L"k", L"l", L"m", L"n",
                                        L"o", L"p", L"q", L"r", L"s", L"t",
                                        L"u", L"v", L"w", L"x", L"y", L"z"};

    std::wstring result;

    // get the sign
    bool negative = (number < 0);
    number = fabs(number);

    // convert the integer portion of the number
    double integer = floor(number);
    while (1)
    {
        const wchar_t* digit_char = digits[(int)fmod(integer, radix)];
        result.insert(0, digit_char);
        integer = floor(integer/radix);

        if (integer <= 0)
            break;
    }

    // convert the decimal portion
    double decimal = number - floor(number);
    double r = radix;

    if (decimal != 0)
    {
        result.append(L".");

        while (1)
        {
            // break when we have enough digits to represent
            // to approximately represent the original number
            if (1/r < 1e-17)
                break;

            double d = decimal*radix;
            const wchar_t* digit_char = digits[(int)floor(d)];
            result.append(digit_char);
            decimal = d - floor(d);

            r = r*radix;
        }
    }

    // prepend the sign
    if (negative)
        result.insert(0, L"-");

    retval->setString(result.c_str());
}


// (METHOD) Number.toLocaleString
// Description: Returns a string that represents the number in a local format.
//
// Syntax: function Number.toLocaleString() : String
//
// Remarks: Returns a string that represents the number in a local format.
//
// Returns: Returns a string that represents the number in a local format.

void Number::toLocaleString(ExprEnv* env, void* param, Value* retval)
{
    // TODO: implement locale-specific string
    Value* val = getThisNumber(env);
    if (val)
        val->toString(retval);
         else
        retval->setNull();
}






std::wstring dbltostr_ndigits(double d, int digits)
{
    wchar_t str[255];
    int i;
        
    for (i = 0; i <= digits; ++i)
    {
        swprintf(str, 255, L"%.*f", i, d);

        if (dblcompare(wtof(str), d) == 0)
            break;
    }
    
    std::wstring result = str;

    while (i < digits)
    {
        if (i == 0)
            result += L".";
        result += L"0";
        ++i;
    }
    
    return result;
}



// (METHOD) Number.toFixed
// Description: Returns a string with a certain precision that 
//     represents the number.
//
// Syntax: function Number.toFixed() : String
// Syntax: function Number.toFixed(digits : Number) : String
//
// Remarks: Returns a string that represents the number. Note: toFixed() 
//     does not use exponential notation and shows the number of |digits| 
//     after the decimal point. If no |digits| argument is specified, zero 
//     digits are shown.
//
// Param(digits): The number of |digits| to be shown after the decimal point.
//
// Returns: Returns a string with a certain precision that 
//     represents the number.


void Number::toFixed(ExprEnv* env, void* param, Value* retval)
{
    // if we don't have a valid value object, return NaN
    Value* val = getThisNumber(env);
    if (!val)
    {
        retval->setNaN();
        return;
    }
    
    // if the value of this number is NaN, undefined, infinity
    // of -infinity, use return the respective string for these
    // using toString
    if (val->isNaN() || val->isUndefined() || val->isAnyInfinity())
    {
        val->toString(retval);
        return;
    }

    // get the input parameter
    int digits = 0;
    
    Value* v = env->getParam(0);
    if (v->isNull() || v->isUndefined())
        digits = 0;
         else
        digits = v->getInteger();
        
    if (digits < 0)
        digits = 0;

    if (digits > 512)
        digits = 512;

/*
    // return a string representation of thevalue; use a buffer size 
    // of 1024 so the full range of double can be expressed 
    // (~ 10^-308 to 10^308)
    wchar_t buf[1024];
    swprintf(buf, 1024, L"%.*f", digits, val->getDouble());
    retval->setString(buf);
*/
    retval->setString(dbltostr_ndigits(dblround(val->getDouble(), digits), digits));
}

// (METHOD) Number.toExponential
// Description: Returns a string that represents the number
//     in exponential notation.
//
// Syntax: function Number.toExponential() : String
// Syntax: function Number.toExponential(digits : Number) : String
//
// Remarks: Returns a string in exponential notation with one digit 
//     before the decimal point and the specified number of |digits| 
//     after the decimal point.
//
// Param(digits): The number of |digits| to be shown after the 
//     decimal point.
//
// Returns: Returns a string that represents the number
//     in exponential notation.


void Number::toExponential(ExprEnv* env, void* param, Value* retval)
{
    // if we don't have a valid value object, return NaN
    Value* val = getThisNumber(env);
    if (!val)
    {
        retval->setNaN();
        return;
    }

    // if the value of this number is NaN, undefined, infinity
    // of -infinity, use return the respective string for these
    // using toString
    if (val->isNaN() || val->isUndefined() || val->isAnyInfinity())
    {
        val->toString(retval);
        return;
    }

    // get the input parameter
    int digits = 0;

    Value* val_digits = env->getParam(0);
    if (val_digits->isNull() || val_digits->isUndefined())
        digits = 0; // print as many digits as are needed
         else
        digits = val_digits->getInteger();
        
    if (digits < 0)
        digits = 0;

    if (digits > 512)
        digits = 512;

    // convert the double to a string
    wchar_t buf[1024];
    swprintf(buf, 1024, L"%.*e", digits, val->getDouble());
    buf[1023] = 0;
    std::wstring result(buf);

    // get the mantissa
    int pos_e = result.find_first_of(L"e");

    // get the exponent and trim the leading zeros
    // of the exponent value
    std::wstring exponent_temp = result.substr(pos_e+1);
    std::wstring exponent;

    std::wstring::iterator it, it_end;
    it_end = exponent_temp.end();

    bool remove_zero = true;
    for (it = exponent_temp.begin(); it != it_end; ++it)
    {
        wchar_t ch = *it;
        if (ch == L'0' && remove_zero)
            continue;

        if (ch != L'+' && ch != L'-' && ch != L'0')
            remove_zero = false;

        exponent += *it;
    }
    
   // if we only have one digit, we only have a + or -,
    // meaning the exponent was zero and we trimmed off
    // all zeros; so put one zero back
    if (exponent.length() == 1)
        exponent += L'0';
    
    
    // calculate our own mantissa from the original double to avoid
    // rounding problems
    int exp = wtoi(exponent.c_str());
    double d = val->getDouble();
    d /= pow10(exp);
    
    if (val_digits->isUndefined())
    {
        result = dbltostr(d);
    }
     else
    {
        result = dbltostr_ndigits(dblround(d, digits), digits);
    }
    
    
    result.append(L"e");
    result.append(exponent);

    // return the result
    retval->setString(result);
}

// (METHOD) Number.toPrecision
// Description: Returns a string that represents the number
//     to a specified number of significant digits.
//
// Syntax: function Number.toPrecision() : String
// Syntax: function Number.toPrecision(digits : Number) : String
//
// Remarks: Returns a string in fixed format to the specified
//     number of significant |digits|.  If the number of
//     significant |digits| isn't specified, return the number
//     in the same way as Number.toString().
//
// Param(digits): The number of significant |digits| to be shown 
//     in the output.
//
// Returns: Returns a string that represents the number to a 
//     specified number of significant digits.

void Number::toPrecision(ExprEnv* env, void* param, Value* retval)
{
    // if we don't have a valid value object, return NaN
    Value* val = getThisNumber(env);
    if (!val)
    {
        retval->setNaN();
        return;
    }

    // if the value of this number is NaN, undefined, infinity
    // of -infinity, use return the respective string for these
    // using toString
    if (val->isNaN() || val->isUndefined() || val->isAnyInfinity())
    {
        val->toString(retval);
        return;
    }

    // get the input parameter
    int digits = 0;
    
    Value* v = env->getParam(0);
    if (v->isNull() || v->isUndefined())
    {
        val->toString(retval);
        return;
    }

    digits = v->getInteger();
        
    if (digits < 0)
        digits = 0;

    if (digits > 512)
        digits = 512;

    // return a string representation of the value;
    // TODO: need to actually limit precision
    val->toString(retval);
}

void Number::toNumber(Value* retval)
{
    val.eval(NULL, retval);
}

void Number::toString(Value* retval)
{
    val.toString(retval);
}

void Number::toPrimitive(Value* retval, int preferred_type)
{
    toNumber(retval);
}






};



