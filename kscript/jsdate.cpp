/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2005-03-25
 *
 */


#include "kscript.h"
#include "jsdate.h"
#include "util.h"
#include "../kl/include/kl/regex.h"
#include <ctime>
#include <cmath>
#include <limits>


#ifdef __APPLE__
#include "../kl/include/kl/portable.h"
#include "../kl/include/kl/string.h"
#define isnan std::isnan
#endif


// remove VC9 warnings - eventually commenting this
// out and implementing specific fixes for these warnings
// is a good idea.
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


#ifdef _MSC_VER
#define isnan _isnan
#define strcasecmp stricmp
#define wcscasecmp wcsicmp
#define wcsncasecmp wcsnicmp
#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef swprintf
#define swprintf _snwprintf
#endif
#endif


#ifdef WIN32
#include <windows.h>
#endif


namespace kscript
{


// time constants
const double hoursPerDay = 24.0;
const double minutesPerHour = 60.0;
const double secondsPerMinute = 60.0;
const double msPerSecond = 1000.0;
const double msPerMinute = 60000.0;
const double msPerHour = 3600000.0;
const double msPerDay = 86400000.0;


// time functions
static double toInt32(double d)
{
    //return doubleToInt32(d);
    return d;
}

static double esTimeClip(double time)
{
    // if the any of the parameters aren't finite, return nan
    static const double nan = std::numeric_limits<double>::quiet_NaN();

    if (isnan(time) || isinf(time))
        return nan;

    if (fabs(time) > 8.64e15)
        return nan;

    double result = toInt32(time);
    return result;
}

static double esHourFromTime(double t)
{
    double result = fmod(floor(t/msPerHour), hoursPerDay);
    if (result < 0)
        result += hoursPerDay;

    return result;
}

static double esMinFromTime(double t)
{
    double result = fmod(floor(t/msPerMinute), minutesPerHour);
    if (result < 0)
        result += minutesPerHour;

    return result;
}

static double esSecFromTime(double t)
{
    double result = fmod(floor(t/msPerSecond), secondsPerMinute);
    if (result < 0)
        result += secondsPerMinute;

    return result;
}

static double msFromTime(double t)
{
    double result = fmod(t, msPerSecond);
    if (result < 0)
        result += msPerSecond;

    return result;
}

static double esDay(double t)
{
    double result = floor(t/msPerDay);
    return result;
}

static double esTimeWithinDay(double t)
{
    double result = fmod(t, msPerDay);
    if (result < 0)
        result += msPerDay;

    return result;
}

static double esDaysInYear(double y)
{
    if (fmod(y, 4) != 0)
        return 365;

    if (fmod(y, 4) == 0 && fmod(y, 100) != 0)
        return 366;

    if (fmod(y, 100) == 0 && fmod(y, 400) != 0)
        return 365;

    if (fmod(y, 400) == 0)
        return 366;

    return -1;
}

static double esDayFromYear(double y)
{
    double result = 365*(y-1970) + floor((y-1969)/4) - 
                      floor((y-1901)/100) + floor((y-1601)/400);
    return result;
}

static double esTimeFromYear(double y)
{
    double result = msPerDay*esDayFromYear(y);
    return result;
}

static double esYearFromTime(double t)
{
    // to get the formula for the return value of this funtion, 
    // solve esTimeFromYear(y) <= t for y, such that y is the largest
    // integer that still statisfies this inequality; so we have:
    //
    // t >= msPerDay * (365*(y-1970) + floor((y-1969)/4) - floor((y-1901)/100) 
    //      + floor((y-1601)/400))
    //
    // setting floor equal to the value inside floor minus some delta < 1,
    // we have:
    //
    // y  = (400*(t/msPerDay) + 400d1 - 400d2 + 400d3 + 193)/146097 + 1970
    //
    // where d1, d2, and d3 are deltas >= 0 and < 1; this gives us a maximum
    // y of:
    //
    // y = floor((400*(t/msPerDay) + 993)/146097 + 1970)
    //
    // after we calculate y, we need only find if esTimeFromYear(y) <= t;
    // if it isn't, subtract 1 from y

    double y = floor((400*(t/msPerDay) + 993)/146097 + 1970);
    if (esTimeFromYear(y) > t)
        y -= 1;

    return y;
}

static double esDayWithinYear(double t)
{
    double result = esDay(t) - esDayFromYear(esYearFromTime(t));
    return result;
}

static double esInLeapYear(double t)
{
    double result = esDaysInYear(esYearFromTime(t));

    if (result == 365)
        return 0;
    if (result == 366)
        return 1;

    return -1;
}

static double esMonthFromTime(double t)
{
    double d = esDayWithinYear(t);
    double leap = esInLeapYear(t);
    
    if (d >= 0 && d < 31)
        return 0;

    if (d >= 31 && d < 59 + leap)
        return 1;

    if (d >= 59 + leap && d < 90 + leap)
        return 2;

    if (d >= 90 + leap && d < 120 + leap)
        return 3;

    if (d >= 120 + leap && d < 151 + leap)
        return 4;

    if (d >= 151 + leap && d < 181 + leap)
        return 5;

    if (d >= 181 + leap && d < 212 + leap)
        return 6;

    if (d >= 212 + leap && d < 243 + leap)
        return 7;

    if (d >= 243 + leap && d < 273 + leap)
        return 8;

    if (d >= 273 + leap && d < 304 + leap)
        return 9;

    if (d >= 304 + leap && d < 334 + leap)
        return 10;

    if (d >= 334 + leap && d < 365 + leap)
        return 11;

    return -1;
}

static double esDateFromTime(double t)
{
    double m = esMonthFromTime(t);
    double d = esDayWithinYear(t);
    double leap = esInLeapYear(t);

    if (m == 0)
        return d + 1;

    if (m == 1)
        return d - 30;

    if (m == 2)
        return d - 58 - leap;

    if (m == 3)
        return d - 89 - leap;

    if (m == 4)
        return d - 119 - leap;

    if (m == 5)
        return d - 150 - leap;

    if (m == 6)
        return d - 180 - leap;

    if (m == 7)
        return d - 211 - leap;

    if (m == 8)
        return d - 242 - leap;

    if (m == 9)
        return d - 272 - leap;

    if (m == 10)
        return d - 303 - leap;

    if (m == 11)
        return d - 333 - leap;

    return -1;
}

static double esWeekDay(double t)
{
    double result = fmod(esDay(t) + 4, 7);
    if (result < 0)
        result += 7;

    return result;
}

static double esMakeTime(double hour, 
                         double min, 
                         double sec, 
                         double ms)
{
    // if the any of the parameters aren't finite, return nan
    static const double nan = std::numeric_limits<double>::quiet_NaN();

    if (isnan(hour) || isinf(hour))
        return nan;
    if (isnan(min) || isinf(min))
        return nan;
    if (isnan(sec) || isinf(sec))
        return nan;
    if (isnan(ms) || isinf(ms))
        return nan;

    double int_hour = toInt32(hour);
    double int_min = toInt32(min);
    double int_sec = toInt32(sec);
    double int_ms = toInt32(ms);

    double result = int_hour*msPerHour + int_min*msPerMinute + int_sec*msPerSecond + int_ms;
    return result;
}

static double esMakeDay(double year,
                        double month,
                        double day)
{
    // if the any of the parameters aren't finite, return nan
    static const double nan = std::numeric_limits<double>::quiet_NaN();

    if (isnan(year) || isinf(year))
        return nan;
    if (isnan(month) || isinf(month))
        return nan;
    if (isnan(day) || isinf(day))
        return nan;

    double int_year = toInt32(year);
    double int_month = toInt32(month);
    double int_day = toInt32(day);

    int_year = int_year + floor(int_month/12);
    int_month = fmod(int_month,12);
    if (int_month < 0)
        int_month += 12;

    // calculate the milliseconds up to the beginning
    // of the year
    double ms_year = esTimeFromYear(int_year);

    // calculate the milliseconds up to the beginning
    // of the month
    double ms_month = 0;
    if (int_month >= 0 && int_month < 12)
    {
        static int days_in_month[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
        ms_month = (days_in_month[(int)int_month] + esInLeapYear(int_year))*msPerDay;
    }

    // make sure DateFromTime(t) is 1
    double t = ms_year + ms_month;
    if (esDateFromTime(t) != 1)
        t += msPerDay;

    // return the day
    double result = esDay(t) + int_day - 1;
    return result;
}

static double esMakeDate(double day, double time)
{
    // if the any of the parameters aren't finite, return nan
    static const double nan = std::numeric_limits<double>::quiet_NaN();

    if (isnan(day) || isinf(day))
        return nan;
    if (isnan(time) || isinf(time))
        return nan;

    double result = day*msPerDay + time;
    return result;
}

static double esLocalTZA()
{
    // returns the timezone offset in milliseconds, which is 
    // the number of milliseconds to add to the local time
    // to get to GMT; the timezone offset is always calculated
    // with standard time, not with daylight savings time, even
    // if daylight savings time is in effect; it follows that 
    // timezones east of the prime meridian and west of the 
    // international dateline will have positive offsets, while 
    // timezones west of prime meridian and east of the 
    // international dateline will have negative offsets

    static int tzoff_ms = 0x0fffffff;
    
    if (tzoff_ms == 0x0fffffff)
    {
        struct tm lt;
        struct tm gmt;

        // as a reference time, use Jan 1, 2000 in seconds since
        // Jan 1, 1970; use Jan 1, 2000 for two reasons: 1) it's
        // not in DST, so we can find out what the timezone offset
        // is without DST, and 2) it's sufficiently past Jan 1, 1970
        // that we can calculate the offset for timezones east of
        // UTC
        time_t ref_time = static_cast<time_t>(946706400);   // seconds since Jan 1, 1970

        #ifdef WIN32
        // these are thread-safe with the MT C runtime lib
        #if (_MSC_VER < 1300)
        lt = *localtime(&ref_time);
        gmt = *gmtime(&ref_time);
        #else
        // these are thread-safe with the MT C runtime lib
        localtime_s(&lt, &ref_time);
        gmtime_s(&gmt, &ref_time);
        #endif
        #else
        // use the reentrant versions on other platforms
        localtime_r(&ref_time, &lt);
        gmtime_r(&ref_time, &gmt);
        #endif

        time_t t_local = mktime(&lt);
        time_t t_gmt = mktime(&gmt);

        tzoff_ms = (int)(t_local - t_gmt)*1000;
    }

    return tzoff_ms;
}

static double getEquivalentYear(double year)
{
    // note: this function returns an equivalent year from
    // the given year, where the equivalent year begins
    // on the same day of the week, is a leap year if the
    // input year is and isn't otherwise, and is after 1970
    // and before 2000

    static int equivalent_year[2][7] =
    {
        {1978, 1973, 1974, 1975, 1981, 1971, 1977},     // non-leap years
        {1984, 1996, 1980, 1992, 1976, 1988, 1972}      // leap years
    };

    double d = esDayFromYear(year);
    d = fmod(d + 4, 7);

    if (d < 0)
        d += 7;

    int day = (int)d;
    int leapyear = esDaysInYear(year) > 365 ? 1 : 0;

    return equivalent_year[leapyear][day];
}

static double esDaylightSavingTA(double t)
{
    if (isnan(t))
        return 0;

    double equivalent_time = t;
    double year = esYearFromTime(t);
    double equivalent_year = getEquivalentYear(year);

    if (year != equivalent_year)
    {
        // find the month and day from the time
        double month = esMonthFromTime(t);
        double day = esDateFromTime(t);

        // set the date
        double newday = esMakeDay(equivalent_year, month, day);
        double newtime = esTimeWithinDay(t);
        equivalent_time = esTimeClip(esMakeDate(newday, newtime));
    }

    if (isnan(equivalent_time))
        return 0;

    // see if DST was in effect for the given time
    struct tm lt;
    time_t t_local = static_cast<time_t>(equivalent_time/1000);

    #ifdef WIN32
    // these are thread-safe with the MT C runtime lib
    #if (_MSC_VER < 1300)
    lt = *localtime(&t_local);
    #else
    localtime_s(&lt, &t_local);
    #endif
    #else
    // use the reentrant versions on other platforms
    localtime_r(&t_local, &lt);
    #endif

    return (lt.tm_isdst > 0 ? 1 : 0)*msPerHour;
}

static double esLocalTime(double t)
{
    double result = t + esLocalTZA() + esDaylightSavingTA(t);
    return result;
}

static double esUTC(double t)
{
    double result = t - esLocalTZA() - esDaylightSavingTA(t - esLocalTZA());
    return result;
}

static double utcDateTimeToMs(double year,
                              double month,
                              double day,
                              double hour,
                              double minute,
                              double second,
                              double ms)
{
    double int_year = toInt32(year);
    if (!isnan(year) && int_year >= 0 && int_year <= 99)
        year = 1900 + int_year;

    double newday = esMakeDay(year, month, day);
    double newtime = esMakeTime(hour, minute, second, ms);

    return esTimeClip(esMakeDate(newday, newtime));
}

static double localDateTimeToMs(double year,
                                double month,
                                double day,
                                double hour,
                                double minute,
                                double second,
                                double ms)
{
    double int_year = toInt32(year);
    if (!isnan(year) && int_year >= 0 && int_year <= 99)
        year = 1900 + int_year;

    double newday = esMakeDay(year, month, day);
    double newtime = esMakeTime(hour, minute, second, ms);

    return esTimeClip(esUTC(esMakeDate(newday, newtime)));
}

static void msToUTCDateTime(double mstime,
                            double* year,
                            double* month,
                            double* day,
                            double* hour,
                            double* minute,
                            double* second,
                            double* ms)
{
    double t = mstime;
    *year = esYearFromTime(t);
    *month = esMonthFromTime(t);
    *day = esDateFromTime(t);
    *hour = esHourFromTime(t);
    *minute = esMinFromTime(t);
    *second = esSecFromTime(t);
    *ms = msFromTime(t);
}

static void msToLocalDateTime(double mstime,
                              double* year,
                              double* month,
                              double* day,
                              double* hour,
                              double* minute,
                              double* second,
                              double* ms)
{
    double t = esLocalTime(mstime);
    *year = esYearFromTime(t);
    *month = esMonthFromTime(t);
    *day = esDateFromTime(t);
    *hour = esHourFromTime(t);
    *minute = esMinFromTime(t);
    *second = esSecFromTime(t);
    *ms = msFromTime(t);
}

static std::wstring dateToString(int year,
                                 int month,
                                 int day,
                                 int hour,
                                 int minute,
                                 int second,
                                 int ms,
                                 int tzoff)
{
    static const wchar_t* months[] = { L"Jan", L"Feb", L"Mar",
                                       L"Apr", L"May", L"Jun",
                                       L"Jul", L"Aug", L"Sep",
                                       L"Oct", L"Nov", L"Dec" };
                                       
    static const wchar_t* dows[] = { L"Sun", L"Mon", L"Tue", L"Wed", 
                                     L"Thu", L"Fri", L"Sat" };
    
    

    // TODO: lot's of conversion going on here; check 
    // instances where dateToString() is being used and 
    // streamline
    int dow = (int)esWeekDay(utcDateTimeToMs(year, month, day, 0, 0, 0, 0));
    if (dow < 0 || dow >= 7) 
        dow = 0;

    if (month < 0 || month >= 12)
        month = 0;

    const wchar_t* mon_str = months[month];
    const wchar_t* dow_str = dows[dow];
    
    wchar_t buf[255];
    
    char sign;
    if (tzoff <= 0)
        sign = '+';
         else
        sign = '-';
    int t = abs(tzoff);
    
    int hour_offset = (int)(t/msPerHour);
    int min_offset = (int)(((double)(t%(int)msPerHour)*60)/msPerHour);
    
    if (year == -1)
    {
        if (t == 0)
        {
            swprintf(buf, 255, L"%02d:%02d:%02d GMT",
                      hour, minute, second);
        }
         else
        {
            swprintf(buf, 255, L"%02d:%02d:%02d GMT%c%02d%02d",
                      hour, minute, second, sign, hour_offset, min_offset);
        }
    }
     else if (hour == -1)
    {
        swprintf(buf, 255, L"%ls %ls %02d %04d",
                  dow_str, mon_str, day, year);
    }
     else
    {
        if (t == 0)
        {
            swprintf(buf, 255, L"%ls %ls %02d %04d %02d:%02d:%02d GMT",
                      dow_str, mon_str, day, year, hour, minute, second);
        }
         else
        {
            swprintf(buf, 255, L"%ls %ls %02d %04d %02d:%02d:%02d GMT%c%02d%02d",
                      dow_str, mon_str, day, year, hour, minute, second, sign, hour_offset, min_offset);
        }
    }
    
    return buf;
}

// (CLASS) Date
// Category: Core
// Derives: Object
// Description: A class that represents a date and time.
// Remarks: A class that represents a date and time.

Date::Date()
{
    m_t = 0;
}

Date::~Date()
{
}

void Date::staticConstructor(kscript::ExprEnv* env,
                             void* param,
                             kscript::Value* retval)
{
    Date* d = new Date;
    int orig_param_count = env->m_param_count;
    env->m_param_count = 0;
    d->constructor(env, retval);
    env->m_param_count = orig_param_count;
    d->toString(env, retval);
    delete d;
}

// (CONSTRUCTOR) Date.constructor
// Description: Creates a new Date object.
//
// Syntax: Date()
// Syntax: Date(milliseconds : Number)
// Syntax: Date(text : String)
// Syntax: Date(year : Number, 
//              month : Number,
//              day : Number,
//              hour : Number,
//              minute : Number,
//              second : Number,
//              millisecond : Number)
//
// Remarks: Creates a new Date object. If no argument is passed, the
//     constructor will create a new Date object set to the current time.
//     If one numeric argument, is passed, the constructor will create a 
//     new Date object set to |milliseconds| milliseconds after 1/1/1970 00:00:00.
//     If a String |text| is passed as the only argument, the constructor will
//     parse |text| in the same way Date.parse() does, and create a Date object
//     accordingly.  Finally, each portion of a date can be specified, in which
//     case the constructor will create a Date object with the given |year|, |month|,
//     |day|, |hour|, |minute|, |second|, and |millisecond|, where the third
//     through seventh parameters are optional.
//
// Param(milliseconds): The number of milliseconds between 1/1/1970 00:00:00 and the 
//     date represented by the new Date.
// Param(text): A string that specifies the new Date
// Param(year): The |year| of the new Date.
// Param(month): The |month| of the new Date.
// Param(day): The |day| of the new Date. Optional.
// Param(hour): The |hour| of the new Date. Optional.
// Param(minute): The |minute| of the new Date. Optional.
// Param(second): The |second| of the new Date. Optional.
// Param(millisecond): The |millisecond| of the new Date. Optional.

void Date::constructor(ExprEnv* env, Value* retval)
{
    int gt_year, gt_month, gt_day,
        gt_hour, gt_minute, gt_second, gt_milliseconds;
    
    #ifdef WIN32
    // we can get greater granularity with GetSystemTime
    SYSTEMTIME st;
    GetSystemTime(&st);
    gt_year = st.wYear;
    gt_month = st.wMonth - 1;   // convert to 0-based month index
    gt_day = st.wDay;
    gt_hour = st.wHour;
    gt_minute = st.wMinute;
    gt_second = st.wSecond;
    gt_milliseconds = st.wMilliseconds;
    #else
    struct tm gt;
    time_t t;
    time(&t);
    gt = *gmtime(&t);
    gt_year = gt.tm_year + 1900;
    gt_month = gt.tm_mon;
    gt_day = gt.tm_mday;
    gt_hour = gt.tm_hour;
    gt_minute = gt.tm_min;
    gt_second = gt.tm_sec;
    gt_milliseconds = 0;
    #endif

    m_t = 0;
    
    if (env->getParamCount() == 0)
    {
        m_t = utcDateTimeToMs(gt_year,
                              gt_month,
                              gt_day,
                              gt_hour,
                              gt_minute,
                              gt_second,
                              gt_milliseconds);
    }
     else if (env->getParamCount() == 1)
    {
        Value* param_p = env->getParam(0);
        
        // try to get the input parameter as a number; this 
        // allows us to get the numeric value of input dates 
        // in the constructor, without converting them to a 
        // string and parsing it
        Value param;
        param_p->toPrimitive(&param, Value::typeDouble);

        if (param.isString())
        {
            parse(env, NULL, retval);
            m_t = retval->getDouble();
        }
         else
        {
            Value v;
            param.toNumber(&v);
            m_t = esTimeClip(v.getDouble());
        }
    }
     else
    {
        double year = env->getParam(0)->getDouble();
        double month = env->getParam(1)->getDouble();

        double day = 1;
        if (env->getParamCount() > 2)
            day = env->getParam(2)->getDouble();

        double hour = 0;
        if (env->getParamCount() > 3)
            hour = env->getParam(3)->getDouble();

        double minute = 0;
        if (env->getParamCount() > 4)
            minute = env->getParam(4)->getDouble();

        double second = 0;
        if (env->getParamCount() > 5)
            second = env->getParam(5)->getDouble();

        double ms = 0;
        if (env->getParamCount() > 6)
            ms = env->getParam(6)->getDouble();

        m_t = localDateTimeToMs(year, month, day, hour, minute, second, ms);
    }
}

void Date::setDateTime(
                       int year,
                       int month,
                       int day,
                       int hour,
                       int minute,
                       int second,
                       int ms)
{
    m_t = localDateTimeToMs(year, month, day, hour, minute, second, ms);
}

void Date::setUTCDateTime(
                       int year,
                       int month,
                       int day,
                       int hour,
                       int minute,
                       int second,
                       int ms)
{
    m_t = utcDateTimeToMs(year, month, day, hour, minute, second, ms);
}

void Date::getDateTime(
                       int* year,
                       int* month,
                       int* day,
                       int* hour,
                       int* minute,
                       int* second,
                       int* ms)
{
    double yy, mm, dd, h, m, s, mls;
    msToLocalDateTime(m_t, &yy, &mm, &dd, &h, &m, &s, &mls);

    *year = (int)toInt32(yy);
    *month = (int)toInt32(mm);
    *day = (int)toInt32(dd);
    *hour = (int)toInt32(h);
    *minute = (int)toInt32(m);
    *second = (int)toInt32(s);
    *ms = (int)toInt32(mls);
}

void Date::getUTCDateTime(
                       int* year,
                       int* month,
                       int* day,
                       int* hour,
                       int* minute,
                       int* second,
                       int* ms)
{
    double yy, mm, dd, h, m, s, mls;
    msToUTCDateTime(m_t, &yy, &mm, &dd, &h, &m, &s, &mls);

    *year = (int)toInt32(yy);
    *month = (int)toInt32(mm);
    *day = (int)toInt32(dd);
    *hour = (int)toInt32(h);
    *minute = (int)toInt32(m);
    *second = (int)toInt32(s);
    *ms = (int)toInt32(mls);
}

// (METHOD) Date.getFullYear
// Description: Returns the year.
//
// Syntax: function Date.getFullYear() : Number
//
// Remarks: Returns the year of the date in local time in four-digit notation.
//
// Returns: Returns the year of the date in local time in four-digit notation.


void Date::getFullYear(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esYearFromTime(esLocalTime(m_t)));
}

// (METHOD) Date.getYear
// Description: Returns the year.
//
// Syntax: function Date.getYear() : Number
//
// Remarks: Returns the year of the date in local time in two-digit 
//     notation, relative to 1900.
//
// Returns: Returns the year of the date in local time in two-digit notation.

void Date::getYear(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esYearFromTime(esLocalTime(m_t)) - 1900);
}

// (METHOD) Date.getMonth
// Description: Returns the month.
//
// Syntax: function Date.getMonth() : Number
//
// Remarks: Returns the month of the year of date in local time. 0 means January, 11 December.
//
// Returns: Returns the month of the year of date in local time. 0 means January, 11 December.

void Date::getMonth(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esMonthFromTime(esLocalTime(m_t)));
}

// (METHOD) Date.getDate
// Description: Returns the day of the month.
//
// Syntax: function Date.getDate() : Number
//
// Remarks: Returns the day of the month of date in local time, where the first day of the month is a 1.
//
// Returns: Returns the day of the month of date in local time, where the first day of the month is a 1.

void Date::getDate(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esDateFromTime(esLocalTime(m_t)));
}

// (METHOD) Date.getDay
// Description: Returns the day of the week.
//
// Syntax: function Date.getDay() : Number
//
// Remarks: Returns the day of the week of date in local time where 0 corresponds to Sunday
//     and 6 corresponds to Saturday.
//
// Returns: Returns the day of the week of date in local time where 0 corresponds to Sunday
//     and 6 corresponds to Saturday.

void Date::getDay(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esWeekDay(esLocalTime(m_t)));
}

// (METHOD) Date.getHours
// Description: Returns the hours.
//
// Syntax: function Date.getHours() : Number
//
// Remarks: Returns the hour of day of date in local time where 0 corresponds to midnight
//     and 23 corresponds to 11 p.m.
//
// Returns: Returns the hour of day of date in local time where 0 corresponds to midnight
//     and 23 corresponds to 11 p.m.

void Date::getHours(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esHourFromTime(esLocalTime(m_t)));
}

// (METHOD) Date.getMinutes
// Description: Returns the minutes.
//
// Syntax: function Date.getMinutes() : Number
//
// Remarks: Returns the minutes of the hour of the date in local time.
//
// Returns: Returns the minutes of the hour of the date in local time.

void Date::getMinutes(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esMinFromTime(esLocalTime(m_t)));
}

// (METHOD) Date.getSeconds
// Description: Returns the seconds.
//
// Syntax: function Date.getSeconds() : Number
//
// Remarks: Returns the seconds field of the date in local time.
//
// Returns: Returns the seconds field of the date in local time.

void Date::getSeconds(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esSecFromTime(esLocalTime(m_t)));
}

// (METHOD) Date.getMilliseconds
// Description: Returns the milliseconds.
//
// Syntax: function Date.getMilliseconds() : Number
//
// Remarks: Returns the milliseconds field of the date in local time.
//
// Returns: Returns the milliseconds field of the date in local time.

void Date::getMilliseconds(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(msFromTime(esLocalTime(m_t)));
}

// (METHOD) Date.getTime
// Description: Returns the milliseconds since 1/1/1970 00:00:00.
//
// Syntax: function Date.getTime() : Number
//
// Remarks: Returns the number of milliseconds between 1/1/1970 00:00:00 and the date.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and the date.

void Date::getTime(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(m_t);
}

// (METHOD) Date.getTimezoneOffset
// Description: Returns the number of minutes difference between local and UTC time.
//
// Syntax: function Date.getTimezoneOffset() : Number
//
// Remarks: Returns the number of minutes between GMT and the local time.
//
// Returns: Returns the number of minutes between GMT and the local time.

void Date::getTimezoneOffset(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble((m_t - esLocalTime(m_t))/msPerMinute);
}

// (METHOD) Date.getUTCFullYear
// Description: Returns the year in universal time.
//
// Syntax: function Date.getUTCFullYear() : Number
//
// Remarks: Returns the year of the date in UTC in four-digit notation.
//
// Returns: Returns the year of the date in UTC in four-digit notation.

void Date::getUTCFullYear(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esYearFromTime(m_t));
}

// (METHOD) Date.getUTCMonth
// Description: Returns the month in universal time.
//
// Syntax: function Date.getUTCMonth() : Number
//
// Remarks: Returns the month of the year of date in UTC where 0 corresponds to
//     January and 11 corresponds to December.
//
// Returns: Returns the month of the year of date in UTC where 0 corresponds to
//     January and 11 corresponds to December.

void Date::getUTCMonth(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esMonthFromTime(m_t));
}

// (METHOD) Date.getUTCDate
// Description: Returns the day of month in universal time.
//
// Syntax: function Date.getUTCDate() : Number
//
// Remarks: Returns the day of the month of date in UTC where the first day of the month 
//     is given by 1.
//
// Returns: Returns the day of the month of date in UTC where the first day of the month 
//     is given by 1.

void Date::getUTCDate(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esDateFromTime(m_t));
}

// (METHOD) Date.getUTCDay
// Description: Returns the day of the week in universal time.
//
// Syntax: function Date.getUTCDay() : Number
//
// Remarks: Returns the day of the week of date in UTC where 0 corresponds to Sunday
//     and 6 corresponds to Saturday.
//
// Returns: Returns the day of the week of date in UTC where 0 corresponds to Sunday
//     and 6 corresponds to Saturday.

void Date::getUTCDay(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esWeekDay(m_t));
}

// (METHOD) Date.getUTCHours
// Description: Returns the hours in universal time.
//
// Syntax: function Date.getUTCHours() : Number
//
// Remarks: Returns the hour of day of date in UTC where 0 corresponds to
//     midnight and 23 corresponds to 11 p.m.
//
// Returns: Returns the hour of day of date in UTC where 0 corresponds to
//     midnight and 23 corresponds to 11 p.m.

void Date::getUTCHours(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esHourFromTime(m_t));
}

// (METHOD) Date.getUTCMinutes
// Description: Returns the minutes in universal time.
//
// Syntax: function Date.getUTCMinutes() : Number
//
// Remarks: Returns the minutes of the hour of the date in UTC.
//
// Returns: Returns the minutes of the hour of the date in UTC.

void Date::getUTCMinutes(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }
    
    retval->setDouble(esMinFromTime(m_t));
}

// (METHOD) Date.getUTCSeconds
// Description: Returns the seconds in universal time.
//
// Syntax: function Date.getUTCSeconds() : Number
//
// Remarks: Returns the seconds of the minute of the date in UTC.
//
// Returns: Returns the seconds of the minute of the date in UTC.

void Date::getUTCSeconds(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(esSecFromTime(m_t));
}

// (METHOD) Date.getUTCMilliseconds
// Description: Returns the milliseconds in universal time.
//
// Syntax: function Date.getUTCMilliseconds() : Number
//
// Remarks: Returns the milliseconds of the seconds of the date in UTC.
//
// Returns: Returns the milliseconds of the seconds of the date in UTC.

void Date::getUTCMilliseconds(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(msFromTime(m_t));
}

// (METHOD) Date.parse
// Description: Parses the argument and returns the number of milliseconds between 1/1/1970 00:00:00.
//
// Syntax: static function Date.parse(text : String) : Number
//
// Remarks: Parses the date described by |text| and returns the number of milliseconds between 
//     1/1/1970 00:00:00 and that date.
//
// Param(text): The |text| describing the date of which the milliseconds after 
//     1/1/1970 00:00:00 are to be returned.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and the 
//     date described by |text|.

void Date::parse(ExprEnv* env, void* param, Value* retval)
{
    int pyy, pmm, pdd, ph, pm, ps;
    const wchar_t* input = NULL;

    if (env->m_param_count < 1 ||
        env->m_eval_params[0]->getType() != Value::typeString)
    {
        retval->setNaN();
        return;
    }


    if (!parseJsDateTime(env->m_eval_params[0]->getString(),
                       &pyy, &pmm, &pdd, &ph, &pm, &ps))
    {
        retval->setNaN();
        return;
    }
    
    // parse JsDateTime returns months with an offset of 1,
    // but localDateTimeToMs uses months with an offset of 0,
    // so adjust the month parameter in the input
    double res = localDateTimeToMs(pyy, (double)pmm - 1.0, pdd, ph, pm, ps, 0);
    retval->setDouble(res);
}

// (METHOD) Date.UTC
// Description: Returns the number of milliseconds between 1/1/1970 00:00:00 and the argument.
//
// Syntax: static function Date.UTC(year : Number,
//                                  month : Number,
//                                  day : Number,
//                                  hour : Number,
//                                  minute : Number,
//                                  second : Number,
//                                  millisecond : Number)
//
// Remarks: Returns the milliseconds after 1/1/1970 00:00:00 of the date specified 
//     by the parameters. This date is interpreted to be in UTC.
//
// Param(year): The |year| of the Date.
// Param(month): The |month| of the Date.
// Param(day): The |day| of the Date. Optional.
// Param(hour): The |hour| of the Date. Optional.
// Param(minute): The |minute| of the Date. Optional.
// Param(second): The |second| of the Date. Optional.
// Param(millisecond): The |millisecond| of the Date. Optional.
//
// Returns: Returns the milliseconds after 1/1/1970 00:00:00 of the date specified 
//     by the parameters. This date is interpreted to be in UTC.

void Date::UTC(ExprEnv* env, void* param, Value* retval)
{
    double year = env->getParam(0)->getDouble();
    double month = env->getParam(1)->getDouble();

    double day = 1;
    if (env->getParamCount() > 2)
        day = env->getParam(2)->getDouble();

    double hour = 0;
    if (env->getParamCount() > 3)
        hour = env->getParam(3)->getDouble();

    double minute = 0;
    if (env->getParamCount() > 4)
        minute = env->getParam(4)->getDouble();

    double second = 0;
    if (env->getParamCount() > 5)
        second = env->getParam(5)->getDouble();

    double ms = 0;
    if (env->getParamCount() > 6)
        ms = env->getParam(6)->getDouble();

    retval->setDouble(utcDateTimeToMs(year, month, day, hour, minute, second, ms));
}

// (METHOD) Date.setFullYear
// Description: Sets the year.
//
// Syntax: function Date.setFullYear(year : Number) : Number
//
// Remarks: Sets the new |year| of the date using a four-digit format and
//     returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.
//
// Param(year): The new |year| of the date in four-digit format.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setFullYear(ExprEnv* env, Value* retval)
{
    // get the time
    double t = esLocalTime(m_t);

    // if the time is NaN, set t to 0
    if (isnan(t))
        t = 0;

    // get the new month parameter
    double year = env->getParam(0)->getDouble();
    double month = env->getParam(1)->getDouble();
    double day = env->getParam(2)->getDouble();

    if (env->getParamCount() < 2)
        month = esMonthFromTime(t);
    if (env->getParamCount() < 3)
        day = esDateFromTime(t);

    // set the date
    double newday = esMakeDay(year, month, day);
    double newtime = esTimeWithinDay(t);
    m_t = esTimeClip(esUTC(esMakeDate(newday, newtime)));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setYear
// Description: Sets the year.
//
// Syntax: function Date.setYear(year : Number) : Number
//
// Remarks: Sets the new |year| of the date using a two-digit format and
//     returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.
//
// Param(year): The new |year| of the date in two-digit format.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setYear(ExprEnv* env, Value* retval)
{
    // get the time
    double t = esLocalTime(m_t);

    // if the time is NaN, set t to 0
    if (isnan(t))
        t = 0;

    // get the new month parameter
    double year = env->getParam(0)->getDouble();

    // if the input is NaN, set the this value to NaN,
    // and return NaN
    if (isnan(year))
    {
        setInvalid();
        retval->setNaN();
        return;
    }

    int int_year = (int)year;
    if (int_year >= 0 && int_year <= 99)
        year = int_year + 1900;

    double month = env->getParam(1)->getDouble();
    double day = env->getParam(2)->getDouble();

    if (env->getParamCount() < 2)
        month = esMonthFromTime(t);
    if (env->getParamCount() < 3)
        day = esDateFromTime(t);

    // set the date
    double newday = esMakeDay(year, month, day);
    double newtime = esTimeWithinDay(t);
    m_t = esTimeClip(esUTC(esMakeDate(newday, newtime)));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setMonth
// Description: Sets the month.
//
// Syntax: function Date.setMonth(month : Number) : Number
//
// Remarks: Sets the new |month| of the date and returns the number of 
//     milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(month): The new |month| of the date.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setMonth(ExprEnv* env, Value* retval)
{
    // get the time
    double t = esLocalTime(m_t);

    // get the new month parameter
    double month = env->getParam(0)->getDouble();
    double day = env->getParam(1)->getDouble();

    if (env->getParamCount() < 2)
        day = esDateFromTime(t);

    double year = esYearFromTime(t);

    // set the date
    double newday = esMakeDay(year, month, day);
    double newtime = esTimeWithinDay(t);
    m_t = esTimeClip(esUTC(esMakeDate(newday, newtime)));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setDate
// Description: Sets the day of the month.
//
// Syntax: function Date.setDate(day : Number) : Number
//
// Remarks: Sets the new |day| of the date and returns the number of 
//     milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(day): The new |day| of the date.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setDate(ExprEnv* env, Value* retval)
{
    // get the time
    double t = esLocalTime(m_t);

    // get the new date parameter
    double day = env->getParam(0)->getDouble();

    double year = esYearFromTime(t);
    double month = esMonthFromTime(t);

    // set the date
    double newday = esMakeDay(year, month, day);
    double newtime = esTimeWithinDay(t);
    m_t = esTimeClip(esUTC(esMakeDate(newday, newtime)));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setHours
// Description: Sets the hour.
//
// Syntax: function Date.setHours(hour : Number) : Number
//
// Remarks: Sets the new |hour| of the date and returns the number of 
//     milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(hour): The new |hour| of the date.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setHours(ExprEnv* env, Value* retval)
{
    // get the time
    double t = esLocalTime(m_t);

    // get the new time parameters
    double hour = env->getParam(0)->getDouble();
    double min = env->getParam(1)->getDouble();
    double sec = env->getParam(2)->getDouble();
    double ms = env->getParam(3)->getDouble();

    if (env->getParamCount() < 2)
        min = esMinFromTime(t);
    if (env->getParamCount() < 3)
        sec = esSecFromTime(t);
    if (env->getParamCount() < 4)
        ms = msFromTime(t);

    // set the time
    m_t = esTimeClip(esUTC(esMakeDate(esDay(t), esMakeTime(hour, min, sec, ms))));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setMinutes
// Description: Sets the minutes.
//
// Syntax: function Date.setMinutes(minute : Number) : Number
//
// Remarks: Sets the new |minute| of the date and returns the number of 
//     milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(minute): The new |minute| of the date.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setMinutes(ExprEnv* env, Value* retval)
{   
    // get the time
    double t = esLocalTime(m_t);

    // get the new time parameters
    double min = env->getParam(0)->getDouble();
    double sec = env->getParam(1)->getDouble();
    double ms = env->getParam(2)->getDouble();

    if (env->getParamCount() < 2)
        sec = esSecFromTime(t);
    if (env->getParamCount() < 3)
        ms = msFromTime(t);

    double hour = esHourFromTime(t);

    // set the time
    m_t = esTimeClip(esUTC(esMakeDate(esDay(t), esMakeTime(hour, min, sec, ms))));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setSeconds
// Description: Sets the seconds.
//
// Syntax: function Date.setSeconds(second : Number) : Number
//
// Remarks: Sets the new |second| of the date and returns the number of 
//     milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(second): The new |second| of the date.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setSeconds(ExprEnv* env, Value* retval)
{
    // get the time
    double t = esLocalTime(m_t);

    // get the new time parameters
    double sec = env->getParam(0)->getDouble();
    double ms = env->getParam(1)->getDouble();

    if (env->getParamCount() < 2)
        ms = msFromTime(t);

    double hour = esHourFromTime(t);
    double min = esMinFromTime(t);

    // set the time
    m_t = esTimeClip(esUTC(esMakeDate(esDay(t), esMakeTime(hour, min, sec, ms))));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setMilliseconds
// Description: Sets the milliseconds.
//
// Syntax: function Date.setMilliseconds(millisecond : Number) : Number
//
// Remarks: Sets the new |millisecond| of the date and returns the number of 
//     milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(millisecond): The new |millisecond| of the date.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setMilliseconds(ExprEnv* env, Value* retval)
{
    // get the time
    double t = esLocalTime(m_t);

    // get the new time parameters
    double ms = env->getParam(0)->getDouble();

    double hour = esHourFromTime(t);
    double min = esMinFromTime(t);
    double sec = esSecFromTime(t);

    // set the time
    m_t = esTimeClip(esUTC(esMakeDate(esDay(t), esMakeTime(hour, min, sec, ms))));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setTime
// Description: Sets the date using milliseconds since 1/1/1970 00:00:00.
//
// Syntax: function Date.setTime(milliseconds : Number) : Number
//
// Remarks: Sets the new number of |milliseconds| between 1/1/1970 00:00:00 and 
//     the date.
//
// Param(milliseconds): The new number of |milliseconds| of the date.
//
// Returns: Returns the new number of |milliseconds| of the date.

void Date::setTime(ExprEnv* env, Value* retval)
{
    // set the time
    double t = env->getParam(0)->getDouble();
    t = t < 0 ? ceil(t) : floor(t);
    m_t = esTimeClip(t);

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setUTCFullYear
// Description: Sets the year in universal time.
//
// Syntax: function Date.setUTCFullYear(year : Number) : Number
//
// Remarks: Sets the new |year| of the date using a four-digit format in
//     UTC and returns the number of milliseconds between 1/1/1970 00:00:00 
//     and the new date.
//
// Param(year): The new |year| of the date in four-digit format in UTC.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setUTCFullYear(ExprEnv* env, Value* retval)
{
    // get the time
    double t = m_t;

    // if the time is NaN, set t to 0
    if (isnan(t))
        t = 0;

    // get the new month parameter
    double year = env->getParam(0)->getDouble();
    double month = env->getParam(1)->getDouble();
    double day = env->getParam(2)->getDouble();

    if (env->getParamCount() < 2)
        month = esMonthFromTime(t);
    if (env->getParamCount() < 3)
        day = esDateFromTime(t);

    // set the date
    double newday = esMakeDay(year, month, day);
    double newtime = esTimeWithinDay(t);
    m_t = esTimeClip(esMakeDate(newday, newtime));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setUTCMonth
// Description: Sets the month in universal time.
//
// Syntax: function Date.setUTCMonth(month : Number) : Number
//
// Remarks: Sets the new |month| of the date in UTC and returns the number 
//     of milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(month): The new |month| of the date in UTC.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setUTCMonth(ExprEnv* env, Value* retval)
{
    // get the time
    double t = m_t;

    // get the new month parameter
    double month = env->getParam(0)->getDouble();
    double day = env->getParam(1)->getDouble();

    if (env->getParamCount() < 2)
        day = esDateFromTime(t);

    double year = esYearFromTime(t);

    // set the date
    double newday = esMakeDay(year, month, day);
    double newtime = esTimeWithinDay(t);
    m_t = esTimeClip(esMakeDate(newday, newtime));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setUTCDate
// Description: Sets the day of the month in universal time.
//
// Syntax: function Date.setUTCDate(day : Number) : Number
//
// Remarks: Sets the new |day| of the date in UTC and returns the number of 
//     milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(day): The new |day| of the date in UTC.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setUTCDate(ExprEnv* env, Value* retval)
{
    // get the time
    double t = m_t;

    // get the new date parameter
    double day = env->getParam(0)->getDouble();

    double year = esYearFromTime(t);
    double month = esMonthFromTime(t);

    // set the date
    double newday = esMakeDay(year, month, day);
    double newtime = esTimeWithinDay(t);
    m_t = esTimeClip(esMakeDate(newday, newtime));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setUTCHours
// Description: Sets the hour in universal time.
//
// Syntax: function Date.setUTCHours(hour : Number) : Number
//
// Remarks: Sets the new |hour| of the date in UTC and returns the number 
//     of milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(hour): The new |hour| of the date in UTC.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setUTCHours(ExprEnv* env, Value* retval)
{
    // get the time
    double t = m_t;

    // get the new time parameters
    double hour = env->getParam(0)->getDouble();
    double min = env->getParam(1)->getDouble();
    double sec = env->getParam(2)->getDouble();
    double ms = env->getParam(3)->getDouble();

    if (env->getParamCount() < 2)
        min = esMinFromTime(t);
    if (env->getParamCount() < 3)
        sec = esSecFromTime(t);
    if (env->getParamCount() < 4)
        ms = msFromTime(t);

    // set the time
    m_t = esTimeClip(esMakeDate(esDay(t), esMakeTime(hour, min, sec, ms)));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setUTCMinutes
// Description: Sets the minutes in universal time.
//
// Syntax: function Date.setUTCMinutes(minute : Number) : Number
//
// Remarks: Sets the new |minutes| of the date in UTC and returns the number 
//     of milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(minutes): The new |minutes| of the date in UTC.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setUTCMinutes(ExprEnv* env, Value* retval)
{
    // get the time
    double t = m_t;

    // get the new time parameters
    double min = env->getParam(0)->getDouble();
    double sec = env->getParam(1)->getDouble();
    double ms = env->getParam(2)->getDouble();

    if (env->getParamCount() < 2)
        sec = esSecFromTime(t);
    if (env->getParamCount() < 3)
        ms = msFromTime(t);

    double hour = esHourFromTime(t);

    // set the time
    m_t = esTimeClip(esMakeDate(esDay(t), esMakeTime(hour, min, sec, ms)));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setUTCSeconds
// Description: Sets the seconds in universal time.
//
// Syntax: function Date.setUTCSeconds(second : Number) : Number
//
// Remarks: Sets the new |second| of the date in UTC and returns the number 
//     of milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(second): The new |second| of the date in UTC.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setUTCSeconds(ExprEnv* env, Value* retval)
{
   // get the time
    double t = m_t;

    // get the new time parameters
    double sec = env->getParam(0)->getDouble();
    double ms = env->getParam(1)->getDouble();

    if (env->getParamCount() < 2)
        ms = msFromTime(t);

    double hour = esHourFromTime(t);
    double min = esMinFromTime(t);

    // set the time
    m_t = esTimeClip(esMakeDate(esDay(t), esMakeTime(hour, min, sec, ms)));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.setUTCMilliseconds
// Description: Sets the milliseconds in universal time.
//
// Syntax: function Date.setUTCMilliseconds(millisecond : Number) : Number
//
// Remarks: Sets the new |millisecond| of the date in UTC and returns the number of 
//     milliseconds between 1/1/1970 00:00:00 and the new date.
//
// Param(millisecond): The new |millisecond| of the date.
//
// Returns: Returns the number of milliseconds between 1/1/1970 00:00:00 and 
//     the new date.

void Date::setUTCMilliseconds(ExprEnv* env, Value* retval)
{
    // get the time
    double t = m_t;

    // get the new time parameters
    double ms = env->getParam(0)->getDouble();

    double hour = esHourFromTime(t);
    double min = esMinFromTime(t);
    double sec = esSecFromTime(t);

    // set the time
    m_t = esTimeClip(esMakeDate(esDay(t), esMakeTime(hour, min, sec, ms)));

    // return the time
    getTime(env, retval);
}

// (METHOD) Date.toGMTString
// Description: Returns a string representing the date.
//
// Syntax: function Date.toGMTString() : String
//
// Remarks: Returns a string representing date in GMT.
//
// Returns: Returns a string representing date in GMT.

void Date::toGMTString(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setString(L"Invalid Date");
        return;
    }
    
    double yy, mm, dd, h, m, s, ms;
    msToUTCDateTime(m_t, &yy, &mm, &dd, &h, &m, &s, &ms);

    std::wstring res = dateToString((int)yy, (int)mm, (int)dd, (int)h, (int)m, (int)s, (int)ms, 0);
    retval->setString(res);
}

// (METHOD) Date.toLocaleString
// Description: Returns a string representing the date.
//
// Syntax: function Date.toLocaleString() : String
//
// Remarks: Returns a string representing the date part of the date in local 
//     time in the local format.
//
// Returns: Returns a string representing the date part of the date in local 
//     time in the local format.

void Date::toLocaleString(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setString(L"Invalid Date");
        return;
    }

    struct tm l;

    time_t t;
    time(&t);
    l = *gmtime(&t);

    double yy, mm, dd, h, m, s, ms;
    msToLocalDateTime(m_t, &yy, &mm, &dd, &h, &m, &s, &ms);

    l.tm_sec = (int)s;
    l.tm_min = (int)m;
    l.tm_hour = (int)h;
    l.tm_mday = (int)dd;
    l.tm_mon = (int)mm;
    l.tm_year = (int)(yy-1900);

    wchar_t buf[255];
    wcsftime(buf, 254, L"%#c", &l);

    retval->setString(buf);
}

// (METHOD) Date.toString
// Description: Returns a string representing the date.
//
// Syntax: function Date.toString() : String
//
// Remarks: Returns a string representing the date in local time.
//
// Returns: Returns a string representing the date in local time.

void Date::toString(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setString(L"Invalid Date");
        return;
    }

    double yy, mm, dd, h, m, s, ms;
    msToLocalDateTime(m_t, &yy, &mm, &dd, &h, &m, &s, &ms);

    int timezone_adjustment = (int)(-1*(esLocalTZA() + esDaylightSavingTA(m_t)));
    std::wstring res = dateToString((int)yy, (int)mm, (int)dd, (int)h, (int)m, (int)s, (int)ms, (int)timezone_adjustment);
    retval->setString(res);
}

// (METHOD) Date.toUTCString
// Description: Returns a string representing the date.
//
// Syntax: function Date.toUTCString() : String
//
// Remarks: Returns a string representing the date in UTC.
//
// Returns: Returns a string representing the date in UTC.

void Date::toUTCString(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setString(L"Invalid Date");
        return;
    }

    double yy, mm, dd, h, m, s, ms;
    msToUTCDateTime(m_t, &yy, &mm, &dd, &h, &m, &s, &ms);
    
    std::wstring res = dateToString((int)yy, (int)mm, (int)dd, (int)h, (int)m, (int)s, (int)ms, 0);
    retval->setString(res);
}

// (METHOD) Date.toDateString
// Description: Returns a string representing the date.
//
// Syntax: function Date.toDateString() : String
//
// Remarks: Returns a string representing the date part of the date in local time.
//
// Returns: Returns a string representing the date part of the date in local time.

void Date::toDateString(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setString(L"Invalid Date");
        return;
    }

    double yy, mm, dd, h, m, s, ms;
    msToLocalDateTime(m_t, &yy, &mm, &dd, &h, &m, &s, &ms);

    int timezone_adjustment = (int)(-1*(esLocalTZA() + esDaylightSavingTA(m_t)));
    std::wstring res = dateToString((int)yy, (int)mm, (int)dd, -1, -1, -1, (int)ms, timezone_adjustment);
    retval->setString(res);
}

// (METHOD) Date.toTimeString
// Description: Returns a string representing the date.
//
// Syntax: function Date.toTimeString() : String
//
// Remarks: Returns a string representing the time part of the date in local time.
//
// Returns: Returns a string representing the time part of the date in local time.

void Date::toTimeString(ExprEnv* env, Value* retval)
{
    if (!isValid())
    {
        retval->setString(L"Invalid Date");
        return;
    }

    double yy, mm, dd, h, m, s, ms;
    msToLocalDateTime(m_t, &yy, &mm, &dd, &h, &m, &s, &ms);
    
    int timezone_adjustment = (int)(-1*(esLocalTZA() + esDaylightSavingTA(m_t)));
    std::wstring res = dateToString(-1, -1, -1, (int)h, (int)m, (int)s, (int)ms, timezone_adjustment);
    retval->setString(res);
}

void Date::toPrimitive(Value* retval, int preferred_type)
{
    if (preferred_type == Value::typeDouble || preferred_type == Value::typeInteger)
    {
        toNumber(retval);
        return;
    }
    
    toString(NULL, retval);
}

void Date::toNumber(Value* retval)
{
    retval->setDouble(m_t);
}

void Date::toString(Value* retval)
{
    toString(NULL, retval);
}

void Date::setInvalid()
{
    m_t = std::numeric_limits<double>::quiet_NaN();
}

bool Date::isValid()
{
    return isnan(m_t) ? false : true;
}

bool Date::checkSetParams(ExprEnv* env, Value* retval)
{
    size_t param_no;
    size_t param_count = env->getParamCount();

    for (param_no = 0; param_no < param_count; ++param_no)
    {
        Value* p = env->getParam(param_no);
        if (p->isNull() || p->isUndefined() || p->isNaN() || p->isAnyInfinity())
        {
            setInvalid();
            retval->setNaN();
            return false;
        }
    }

    return true;
}


class DT
{
public:

    DT()
    {
        static const double nan = std::numeric_limits<double>::quiet_NaN();

        yy = nan;
        mm = nan;
        dd = nan;
        h = 0;
        m = 0;
        s = 0;
        ms = 0;
        tz = nan;
    }

    double yy;
    double mm;
    double dd;
    double h;
    double m;
    double s;
    double ms;
    double tz;
};

static bool parseTime(const wchar_t* expr,
                      int* offset,
                      DT* dt)
{
    static const klregex::wregex regex_time(L"^[:]?(?<hour>\\d+)\\s*[:]\\s*(?<minute>\\d+)(\\s*[:]\\s*(?<second>\\d+))?(\\s*(?i:(?<ampm>(a|p)\\.?m\\.?)))?");

    // allow leading ":" to allow for dates of the form
    // 14/Apr/2008:21:58:40 +0000"

    klregex::wmatch matchres;
    if (!regex_time.search(expr, matchres))
        return false;

    const klregex::wsubmatch& hour_match = matchres[L"hour"];
    const klregex::wsubmatch& minute_match = matchres[L"minute"];
    const klregex::wsubmatch& second_match = matchres[L"second"];
    const klregex::wsubmatch& ampm_match = matchres[L"ampm"];

    if (hour_match.isValid())
        dt->h = wtof(hour_match.str().c_str());
    
    if (minute_match.isValid())
        dt->m = wtof(minute_match.str().c_str());

    if (second_match.isValid())
        dt->s = wtof(second_match.str().c_str());

    // limit the time from 00:00:00 to 23:59:59
    if (dt->h < 0 || dt->h >= 24)
        return false;

    if (dt->m < 0 || dt->m >= 60)
        return false;

    if (dt->s < 0 || dt->s >= 60)
        return false;

    if (ampm_match.isValid())
    {
        // if the hour is greater than 12, the am/pm
        // flag contradicts the time; return false
        if (dt->h > 12)
            return false;

        if (::towupper(*ampm_match.first) == L'P')
        {
            if (dt->h < 12)
                dt->h += 12;
        }
         else
        {
            // handle "12 am"
            if (dt->h == 12)
                dt->h = 0;
        }
    }

    // increment the offset
    *offset = (int)(matchres.p_unwind - expr);

    return true;
}

static bool parseTimezone(const wchar_t* expr,
                         int* offset,
                         DT* dt)
{
    // default offset
    *offset = 3;

    static const klregex::wregex regex_timezone(L"^[+-]\\d+");

    klregex::wmatch matchres;
    if (regex_timezone.search(expr, matchres))    // +0000, etc
    {
        // set the offset
        *offset = (int)(matchres.p_unwind - expr);

        // set the timezone
        int t = wtoi(matchres[0].str().c_str());
        int sign = t < 0 ? -1 : 1;
        t = abs(t);
        
        if (t < 25)
        {
            dt->tz = t * msPerHour;
        }
         else
        {
            dt->tz = ((t/100) * msPerHour) + (t%100) * msPerMinute;
        }

        dt->tz = -1 * sign * dt->tz;
    }
    else if (!wcsncmp(expr, L"GMT", 3))  // UTC
        dt->tz = 0 * msPerHour;
    else if (!wcsncmp(expr, L"UTC", 3))  // UTC
        dt->tz = 0 * msPerHour;
    else if (!wcsncmp(expr, L"UT", 2))   // UTC
    {
        dt->tz = 0;
        *offset = 2;
    }
    else if (!wcsncmp(expr, L"Z", 1))    // UTC
    {
        dt->tz = 0;
        *offset = 1;
    }
    else if (!wcsncmp(expr, L"ADT", 3))  // ADT Atlantic Standard Time (GMT -4 hours)
        dt->tz = 4 * msPerHour;
    else if (!wcsncmp(expr, L"AST", 3))  // AST Atlantic Daylight Saving Time (GMT -3 hours)
        dt->tz = 3 * msPerHour;
    else if (!wcsncmp(expr, L"EST", 3))  // EST Eastern Standard Time (GMT -5 hours)
        dt->tz = 5 * msPerHour;
    else if (!wcsncmp(expr, L"EDT", 3))  // EDT Eastern Daylight Saving Time (GMT -4 hours)
        dt->tz = 4 * msPerHour;
    else if (!wcsncmp(expr, L"CST", 3))  // CST Central Standard Time (GMT -6 hours)
        dt->tz = 6 * msPerHour;
    else if (!wcsncmp(expr, L"CDT", 3))  // CDT Central Daylight Saving Time (GMT -5 hours)
        dt->tz = 5 * msPerHour;
    else if (!wcsncmp(expr, L"MST", 3))  // MST Mountain Standard Time (GMT -7 hours)
        dt->tz = 7 * msPerHour;
    else if (!wcsncmp(expr, L"MDT", 3))  // MDT Mountain Daylight Saving Time (GMT -6 hours)
        dt->tz = 6 * msPerHour;
    else if (!wcsncmp(expr, L"PST", 3))  // PST Pacific Standard Time (GMT -8 hours)
        dt->tz = 8 * msPerHour;
    else if (!wcsncmp(expr, L"PDT", 3))  // PDT Pacific Daylight Saving Time (GMT -7 hours)
        dt->tz = 7 * msPerHour;
    else if (!wcsncmp(expr, L"AKST", 4)) // AKST Alaska Standard Time (GMT -9 hours)
    {
        dt->tz = 9 * msPerHour;
        *offset = 4;
    }
    else if (!wcsncmp(expr, L"AKDT", 4)) // AKDT Alaska Daylight Saving Time (GMT -8 hours)
    {
        dt->tz = 8 * msPerHour;
        *offset = 4;
    }
    else if (!wcsncmp(expr, L"HST", 3))  // HST Hawaiian Standard Time (GMT -10 hours)
        dt->tz = 10 * msPerHour;
    else
        *offset = -1;

    // couldn't find a timezone
    if (*offset == -1)
        return false;

    return true;
}

static bool parseDayname(const wchar_t* expr,
                         int* offset,
                         DT* dt)
{
    // look for the dayname to acknowledge it if it is specified
    static const klregex::wregex regex_dayname(L"^[[:alpha:]]{3,}[,]?");

    klregex::wmatch matchres;
    if (regex_dayname.search(expr, matchres))
    {
        std::wstring m = matchres[0].str();
        const wchar_t* match = m.c_str();

        if (!wcsncasecmp(match, L"MON", 3) ||
            !wcsncasecmp(match, L"TUE", 3) ||
            !wcsncasecmp(match, L"WED", 3) ||
            !wcsncasecmp(match, L"THU", 3) ||
            !wcsncasecmp(match, L"FRI", 3) ||
            !wcsncasecmp(match, L"SAT", 3) ||
            !wcsncasecmp(match, L"SUN", 3)
            )
        {
            *offset = (int)(matchres.p_unwind - expr);
            return true;
        }
    }

    return false;
}

static bool parseDate(const wchar_t* expr,
                      int* offset,
                      DT* dt)
{
    // if we've already matched on a date, don't
    // attempt to replace it
    if (!isnan(dt->yy) && !isnan(dt->mm) && !isnan(dt->dd))
        return false;

    klregex::wmatch matchres;

    // if we don't have any part of the date, try to get it
    // in one chunck
    if (isnan(dt->yy) && isnan(dt->mm) && isnan(dt->dd))
    {
        // try dates of the form param1/param2/param3, where the 
        // three parameters are interpreted as follows:
        //     1) MM/DD/YYYY:  0 <= param1 < 70 and param3 >= 100
        //     2) MM/DD/YY:    0 <= param1 < 70 and param3 < 100
        //     3) YY/MM/DD:    70 <= param1 < 100 and param2 < 70
        //     4) YYYY/MM/DD:  param1 >= 100 and param2 < 70
        // other combinations are invalid

        static const klregex::wregex regex_date1(L"^(?<param1>[0-9]+)[\\s,./-]+(?<param2>[0-9]+)[\\s,./-]+(?<param3>[0-9]+)");

        if (regex_date1.search(expr, matchres))
        {
            const klregex::wsubmatch& param1_match = matchres[L"param1"];
            const klregex::wsubmatch& param2_match = matchres[L"param2"];
            const klregex::wsubmatch& param3_match = matchres[L"param3"];

            double p1 = wtof(param1_match.str().c_str());
            double p2 = wtof(param2_match.str().c_str());
            double p3 = wtof(param3_match.str().c_str());

            // MM/DD/YYYY:  0 <= param1 < 70 and param3 >= 100
            if (p1 >= 0 && p1 < 70 && p3 >= 100)
            {
                dt->yy = p3;
                dt->mm = p1;
                dt->dd = p2;
            }

            // MM/DD/YY:    0 <= param1 < 70 and param3 < 100
            if (p1 >= 0 && p1 < 70 && p3 < 100)
            {
                dt->yy= p3 + 1900;
                dt->mm = p1;
                dt->dd = p2;
            }

            // YY/MM/DD:    70 <= param1 < 100 and param2 < 70
            if (p1 >= 70 && p1 < 100 && p2 < 70)
            {
                dt->yy = p1 + 1900;
                dt->mm = p2;
                dt->dd = p3;
            }

            // YYYY/MM/DD:  param1 >= 100 and param2 < 70
            if (p1 >= 100 && p2 < 70)
            {
                dt->yy = p1;
                dt->mm = p2;
                dt->dd = p3;
            }

            // if any of the year parameter is nan, the date
            // wasn't set; return false
            if (isnan(dt->yy))
                return false;

            // the date was set, so increment the offset and
            // return true
            *offset = (int)(matchres.p_unwind - expr);
            return true;
        }
    }

    // if we can't get the date in one chunk, try to get the
    // date in parts by looking for the year, month and day
    // separately

    // try to match on a month string; allow a comma or
    // period after the string
    static const klregex::wregex regex_string(L"^(?<string>[[:alpha:]]{3,})[\\s,./-]*");
    if (regex_string.search(expr, matchres))
    {
        const klregex::wsubmatch& string_match = matchres[L"string"];
        if (!string_match.isValid())
            return false;

        // if we already have a month parameter, we have an
        // inconsistency, so return false
        if (!isnan(dt->mm))
            return false;

        // extract the year, month, and day
        static const wchar_t* months[] = { L"JAN", L"FEB", L"MAR",
                                           L"APR", L"MAY", L"JUN",
                                           L"JUL", L"AUG", L"SEP",
                                           L"OCT", L"NOV", L"DEC" };

        std::wstring month = string_match.str();

        int j;
        for (j = 0; j < 12; ++j)
        {
            if (0 == wcsncasecmp(month.c_str(), months[j], 3))
            {
                dt->mm = j+1;
                *offset = (int)(matchres.p_unwind - expr);
                return true;
            }
        }

        // we matched on something, but not a month; return false
        return false;
    }

    // try to match on a year or day number (we'll find out which later);
    // allow a comma or period after the string
    static const klregex::wregex regex_number(L"^(?<number>[0-9]{1,6})[\\s,./-]*");
    if (regex_number.search(expr, matchres))
    {
        const klregex::wsubmatch& number_match = matchres[L"number"];
        if (!number_match.isValid())
            return false;

        if (isnan(dt->yy))
        {
            // if we don't have a year, take this number
            // as the year for now
            dt->yy = wtof(number_match.str().c_str());
        }
        else if (isnan(dt->dd))
        {
            // if we have a year, but not a day, take
            // this number as the day for now
            dt->dd = wtof(number_match.str().c_str());
        }
        else
        {
            // we matched on something, but we already have a
            // year and a day, so we don't know what this is;
            // return false
            return false;
        }

        // if we have a year and day number, reorder the year and
        // day numbers if necessary; we need to do this because we
        // assumed the first number we came to was a year, but it
        // might be the day

        if (!isnan(dt->yy) && !isnan(dt->dd))
        {
            bool valid = false;
            double p1 = dt->yy;
            double p2 = dt->dd;

            // p1=DD, p2=YYYY: 0 <= p1 < 40 and p2 >= 100
            if (p1 >= 0 && p1 < 40 && p2 >= 100)
            {
                valid = true;
                dt->yy = p2;
                dt->dd = p1;
            }

            // p1=DD, p2=YY: 0 <= p1 < 40 and p2 < 100
            if (p1 >= 0 && p1 < 40 && p2 < 100)
            {
                valid = true;
                dt->yy= p2 + 1900;
                dt->dd = p1;
            }

            // p1=YYYY, p2=DD: 0 <= p2 < 40 and p1 >= 100
            if (p2 >= 0 && p2 < 40 && p1 >= 100)
            {
                valid = true;
                dt->yy = p1;
                dt->dd = p2;
            }

            // p1=YY, p2=DD: 40 <= p1 < 100  and 0 <= p2 < 40
            if (p2 >= 0 && p2 < 40 && p1 >= 40 && p1 < 100)
            {
                valid = true;
                dt->yy= p1 + 1900;
                dt->dd = p2;
            }

            if (!valid)
                return false;
        }

        // adjust the offset
        *offset = (int)(matchres.p_unwind - expr);
        return true;
    }

    // we didn't match on anything
    return false;
}

bool parseJsDateTime(const wchar_t* input,
                     int* year,
                     int* month,
                     int* day,
                     int* hour,
                     int* minute,
                     int* second)
{
    // trim leading/trailing spaces from the string
    const wchar_t* start = input;
    const wchar_t* end = input + wcslen(input);
    
    if (end <= start)
        return false;

    // parse the string
    int offset = 0;
    DT dt;

    while (1)
    {
        offset = 0;

        // note: parse daynames after timezone, since daynames
        // may have a lot of variations and it's easier to
        // match specific timezones, then rule out daynames,
        // than vice versa
        if (parseTime(input, &offset, &dt))
            input += offset;
        else if (parseTimezone(input, &offset, &dt))
            input += offset;
        else if (parseDayname(input, &offset, &dt))
            input += offset;
        else if (parseDate(input, &offset, &dt))
            input += offset;
        else if (::iswspace(*input))  // white space
            ++input;
        else
            return false;

        if (!*input)
            break;
    }

    if (isnan(dt.yy) || isnan(dt.mm) || isnan(dt.dd))
        return false;

    // when months are parsed, they are 1-based, rather than zero-based;
    // however, the internal date functions use a zero-based month, so
    // subtract 1;
    dt.mm = dt.mm - 1;

    // get the time in milliseconds; if the timezone isn't specified, 
    // interpret the year, month, day, hour, minute, and second in local
    // time; otherwise, interpret these values in UTC and add in the 
    // timeoffset manually
    double time_ms = 0;

    if (isnan(dt.tz))
    {
        time_ms = localDateTimeToMs(dt.yy,
                                    dt.mm,
                                    dt.dd,
                                    dt.h,
                                    dt.m,
                                    dt.s,
                                    0);
    }
    else
    {
        time_ms = utcDateTimeToMs(dt.yy,
                                  dt.mm,
                                  dt.dd,
                                  dt.h,
                                  dt.m,
                                  dt.s,
                                  0);

        time_ms += dt.tz;
    }

    // convert the milliseconds back to local time
    msToLocalDateTime(time_ms,
                      &dt.yy,
                      &dt.mm,
                      &dt.dd,
                      &dt.h,
                      &dt.m,
                      &dt.s,
                      &dt.ms);

    // unlike the rest of the internal date functions, this function
    // is used elsewhere and returns 1-based months; so add 1 to the
    // month
    dt.mm = dt.mm + 1;

    if (year)
        *year = (int)dt.yy;
    if (month)
        *month = (int)dt.mm;
    if (day)
        *day = (int)dt.dd;
    if (hour)
        *hour = (int)dt.h;
    if (minute)
        *minute = (int)dt.m;
    if (second)
        *second = (int)dt.s;

    return true;
}


};

