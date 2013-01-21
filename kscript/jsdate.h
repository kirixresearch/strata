/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-10
 *
 */


#ifndef __KSCRIPT_JSDATE_H
#define __KSCRIPT_JSDATE_H


#include "jsobject.h"


namespace kscript
{


class Date : public Object
{
public:

    BEGIN_KSCRIPT_DERIVED_CLASS("Date", Date, Object)
        KSCRIPT_METHOD("constructor", Date::constructor)
        KSCRIPT_METHOD("getDate", Date::getDate)
        KSCRIPT_METHOD("getDay", Date::getDay)
        KSCRIPT_METHOD("getMonth", Date::getMonth)
        KSCRIPT_METHOD("getFullYear", Date::getFullYear)
        KSCRIPT_METHOD("getYear", Date::getYear)
        KSCRIPT_METHOD("getHours", Date::getHours)
        KSCRIPT_METHOD("getMinutes", Date::getMinutes)
        KSCRIPT_METHOD("getSeconds", Date::getSeconds)
        KSCRIPT_METHOD("getMilliseconds", Date::getMilliseconds)
        KSCRIPT_METHOD("valueOf", Date::getTime)
        KSCRIPT_METHOD("getTime", Date::getTime)
        KSCRIPT_METHOD("getTimezoneOffset", Date::getTimezoneOffset)
        KSCRIPT_METHOD("getUTCDate", Date::getUTCDate)
        KSCRIPT_METHOD("getUTCDay", Date::getUTCDay)
        KSCRIPT_METHOD("getUTCMonth", Date::getUTCMonth)
        KSCRIPT_METHOD("getUTCFullYear", Date::getUTCFullYear)
        KSCRIPT_METHOD("getUTCHours", Date::getUTCHours)
        KSCRIPT_METHOD("getUTCMinutes", Date::getUTCMinutes)
        KSCRIPT_METHOD("getUTCSeconds", Date::getUTCSeconds)
        KSCRIPT_METHOD("getUTCMilliseconds", Date::getUTCMilliseconds)
        KSCRIPT_STATIC_METHOD("parse", Date::parse)
        KSCRIPT_STATIC_METHOD("UTC", Date::UTC)
        KSCRIPT_METHOD("setDate", Date::setDate)
        KSCRIPT_METHOD("setFullYear", Date::setFullYear)
        KSCRIPT_METHOD("setHours", Date::setHours)
        KSCRIPT_METHOD("setMilliseconds", Date::setMilliseconds)
        KSCRIPT_METHOD("setMinutes", Date::setMinutes)
        KSCRIPT_METHOD("setMonth", Date::setMonth)
        KSCRIPT_METHOD("setSeconds", Date::setSeconds)
        KSCRIPT_METHOD("setTime", Date::setTime)
        KSCRIPT_METHOD("setYear", Date::setYear)
        KSCRIPT_METHOD("setUTCDate", Date::setUTCDate)
        KSCRIPT_METHOD("setUTCMonth", Date::setUTCMonth)
        KSCRIPT_METHOD("setUTCFullYear", Date::setUTCFullYear)
        KSCRIPT_METHOD("setUTCHours", Date::setUTCHours)
        KSCRIPT_METHOD("setUTCMinutes", Date::setUTCMinutes)
        KSCRIPT_METHOD("setUTCSeconds", Date::setUTCSeconds)
        KSCRIPT_METHOD("setUTCMilliseconds", Date::setUTCMilliseconds)
        KSCRIPT_METHOD("toGMTString", Date::toGMTString)
        KSCRIPT_METHOD("toLocaleString", Date::toLocaleString)
        KSCRIPT_METHOD("toString", Date::toString)
        KSCRIPT_METHOD("toUTCString", Date::toUTCString)
        KSCRIPT_METHOD("toTimeString", Date::toTimeString)
        KSCRIPT_METHOD("toDateString", Date::toDateString)
        KSCRIPT_METHOD("toLocaleTimeString", Date::toTimeString)
        KSCRIPT_METHOD("toLocaleDateString", Date::toDateString)
    END_KSCRIPT_CLASS()

    Date();
    ~Date();
    
    static void staticConstructor(ExprEnv* env,
                                  void* param,
                                  Value* retval);
                        
    void constructor(ExprEnv* env, Value* retval);
    void getDate(ExprEnv* env, Value* retval);
    void getDay(ExprEnv* env, Value* retval);
    void getMonth(ExprEnv* env, Value* retval);
    void getFullYear(ExprEnv* env, Value* retval);
    void getYear(ExprEnv* env, Value* retval);
    void getHours(ExprEnv* env, Value* retval);
    void getMinutes(ExprEnv* env, Value* retval);
    void getSeconds(ExprEnv* env, Value* retval);
    void getMilliseconds(ExprEnv* env, Value* retval);
    void getTime(ExprEnv* env, Value* retval);
    void getTimezoneOffset(ExprEnv* env, Value* retval);
    void getUTCDate(ExprEnv* env, Value* retval);
    void getUTCDay(ExprEnv* env, Value* retval);
    void getUTCMonth(ExprEnv* env, Value* retval);
    void getUTCFullYear(ExprEnv* env, Value* retval);
    void getUTCHours(ExprEnv* env, Value* retval);
    void getUTCMinutes(ExprEnv* env, Value* retval);
    void getUTCSeconds(ExprEnv* env, Value* retval);
    void getUTCMilliseconds(ExprEnv* env, Value* retval);
    static void parse(ExprEnv* env, void* param, Value* retval);
    static void UTC(ExprEnv* env, void* param, Value* retval);
    void setDate(ExprEnv* env, Value* retval);
    void setFullYear(ExprEnv* env, Value* retval);
    void setHours(ExprEnv* env, Value* retval);
    void setMilliseconds(ExprEnv* env, Value* retval);
    void setMinutes(ExprEnv* env, Value* retval);
    void setMonth(ExprEnv* env, Value* retval);
    void setSeconds(ExprEnv* env, Value* retval);
    void setTime(ExprEnv* env, Value* retval);
    void setYear(ExprEnv* env, Value* retval);
    void setUTCDate(ExprEnv* env, Value* retval);
    void setUTCMonth(ExprEnv* env, Value* retval);
    void setUTCFullYear(ExprEnv* env, Value* retval);
    void setUTCHours(ExprEnv* env, Value* retval);
    void setUTCMinutes(ExprEnv* env, Value* retval);
    void setUTCSeconds(ExprEnv* env, Value* retval);
    void setUTCMilliseconds(ExprEnv* env, Value* retval);
    void toGMTString(ExprEnv* env, Value* retval);
    void toLocaleString(ExprEnv* env, Value* retval);
    void toString(ExprEnv* env, Value* retval);
    void toUTCString(ExprEnv* env, Value* retval);
    void toTimeString(ExprEnv* env, Value* retval);
    void toDateString(ExprEnv* env, Value* retval);

    void setDateTime(
                     int year,
                     int month,
                     int day,
                     int hour = 0,
                     int minute = 0,
                     int second = 0,
                     int ms = 0);

    void setUTCDateTime(
                     int year,
                     int month,
                     int day,
                     int hour = 0,
                     int minute = 0,
                     int second = 0,
                     int ms = 0);

    void getDateTime(
                     int* year,
                     int* month,
                     int* day,
                     int* hour,
                     int* minute,
                     int* second,
                     int* ms);
                     
    void getUTCDateTime(
                     int* year,
                     int* month,
                     int* day,
                     int* hour,
                     int* minute,
                     int* second,
                     int* ms);

    void toPrimitive(Value* retval, int preferred_type);
    void toNumber(Value* retval);
    void toString(Value* retval);

private:

    void setInvalid();
    bool isValid();
    bool checkSetParams(ExprEnv* env, Value* retval);

private:

    double m_t;
};


bool parseJsDateTime(const wchar_t* input,
                     int* year,
                     int* month,
                     int* day,
                     int* hour,
                     int* minute,
                     int* second);

};




#endif

