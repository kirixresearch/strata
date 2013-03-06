/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2001-11-02
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "util.h"
#include <kl/string.h>
#include <kl/portable.h>
#include <kl/math.h>
#include <kl/regex.h>
#include <kl/md5.h>
#include <cstdio>
#include <cmath>
#include <ctime>
#include "../../kscript/kscript.h"
#include "../xdcommon/xdcommon.h"


#ifdef WIN32
#include <windows.h>
#endif


#ifdef REGEXP_MADDOCK
#include "../xdregex/include/regex.h"
#endif


#ifndef _MSC_VER
#include <netdb.h>
#include <arpa/inet.h>
#endif


#ifdef _MSC_VER
#include <cfloat>
#define isnan _isnan
#endif



// date comparison helper

// < 0 date1 less than date2 
// 0 date1 identical to date2 
// > 0 date1 greater than date2 

int dtCompare(kscript::ExprDateTime* d1, kscript::ExprDateTime* d2)
{
    if (d1->date < d2->date)
        return -1;
    if (d1->date == d2->date)
    {
        if (d1->time < d2->time)
            return -1;
        if (d1->time == d2->time)
            return 0;
    }
    return 1;
}

int dtCompare(kscript::Value* vd1, kscript::Value* vd2)
{
    kscript::ExprDateTime d1, d2;
    vd1->getDateTime(&d1);
    vd2->getDateTime(&d2);
    return dtCompare(&d1, &d2);
}


void func_abs(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_eval_params[0]->getType() == kscript::Value::typeDouble)
    {
        retval->setDouble(fabs(env->m_eval_params[0]->getDouble()));
    }
     else if (env->m_eval_params[0]->getType() == kscript::Value::typeInteger)
    {
        retval->setInteger(abs(env->m_eval_params[0]->getInteger()));
    }
     else
    {
        retval->setInteger(0);
    }
}

void func_sign(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setInteger(kl::dblcompare(env->m_eval_params[0]->getDouble(), 0.0));
}

void func_exp(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setDouble(exp(env->m_eval_params[0]->getDouble()));
}

void func_ln(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    double d = env->m_eval_params[0]->getDouble();
    if (kl::dblcompare(d, 0.0) <= 0)
    {
        retval->setDouble(0);
        return;
    }

    retval->setDouble(log(d));
}

void func_log(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    double x = env->m_eval_params[0]->getDouble();
    double base = 10.0;

    if (env->m_param_count > 1)
    {
        base = env->m_eval_params[1]->getDouble();
    }

    if (kl::dblcompare(base, 1) <= 0)
    {
        retval->setDouble(0);
        return;
    }

    double d = (log(x)/log(base));

    retval->setDouble(d);
}

void func_power(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    double x = env->m_eval_params[0]->getDouble();
    double y = env->m_eval_params[1]->getDouble();

    /*
    if (kl::dblcompare(x, 0) < 0 &&
        kl::dblcompare(y, 0) < 0)
    {
        // if both the base and the exponent are < 0,
        // the return result is an error
        retval->setDouble(0);
        return;
    }
    */

    double d = pow(x, y);
    retval->setDouble(d);
}

void func_cos(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setDouble(cos(env->m_eval_params[0]->getDouble()));
}

void func_acos(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    double x = env->m_eval_params[0]->getDouble();

    /*
    if (kl::dblcompare(x, -1.0) < 0 ||
        kl::dblcompare(x, 1.0) > 0)
    {
        // floating error
        retval->setDouble(0);
        return;
    }
    */

    double d = acos(x);

    retval->setDouble(d);
}

void func_sin(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setDouble(sin(env->m_eval_params[0]->getDouble()));
}

void func_asin(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    double x = env->m_eval_params[0]->getDouble();

    /*
    if (kl::dblcompare(x, -1.0) < 0 ||
        kl::dblcompare(x, 1.0) > 0)
    {
        // floating error
        retval->setDouble(0);
        return;
    }
    */

    double d = asin(x);
    retval->setDouble(d);
}

void func_tan(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    double x = env->m_eval_params[0]->getDouble();
    double d = tan(x);
    retval->setDouble(d);
}

void func_atan(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setDouble(atan(env->m_eval_params[0]->getDouble()));
}

void func_rand(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    double i;
    double d = 0.0;
    d = rand();
    d = d*d;
    d = modf(d/1000000, &i);

    retval->setDouble(d);
}


void func_pi(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setDouble(3.14159265358979);
}

void func_ceiling(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_param_count == 2)
    {
        double p0 = env->m_eval_params[0]->getDouble();
        double power = pow((double)10, env->m_eval_params[1]->getInteger());
        p0 *= power;
        p0 = ceil(p0);
        p0 /= power;
        retval->setDouble(p0);
    }
     else
    {
        retval->setDouble(ceil(env->m_eval_params[0]->getDouble()));
    }
}

void func_floor(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_param_count == 2)
    {
        double p0 = env->m_eval_params[0]->getDouble();
        double power = pow((double)10, env->m_eval_params[1]->getInteger());
        p0 *= power;
        p0 = floor(p0);
        p0 /= power;
        retval->setDouble(p0);
    }
     else
    {
        retval->setDouble(floor(env->m_eval_params[0]->getDouble()));
    }
}

void func_trunc(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    double d = env->m_eval_params[0]->getDouble();

    int dec = 0;
    if (env->m_param_count > 1)
    {
        dec = env->m_eval_params[1]->getInteger();
    }
    
        
    double p = pow((double)10, dec);

    d *= p;
    if (d >= 0)
    {
        d = floor(d);
    }
     else
    {
        d = ceil(d);
    }
    d /= p;

    retval->setDouble(d);
}


void func_round(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    int dec = 0;

    if (env->m_param_count > 1)
    {
        dec = env->m_eval_params[1]->getInteger();
    }

    retval->setDouble(kl::dblround(env->m_eval_params[0]->getDouble(), dec));
}



void func_substr(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* str = env->m_eval_params[0]->getString();
    int str_len = wcslen(str);
    int p1 = env->m_eval_params[1]->getInteger();

    // handle negative offset, i.e. substr('hello', -2, 2) should return 'lo'
    if (p1 < 0)
        p1 = str_len + p1 + 1;

    if (p1 <= 0 || p1 > str_len)
    {
        retval->setString(L"");
        return;
    }
    
    const wchar_t* offset = str+p1-1;

    if (env->m_param_count == 2)
    {
        if (offset <= str+str_len)
            retval->setString(offset);
             else
            retval->setString(L"");
        return;
    }


    int p2 = env->m_eval_params[2]->getInteger();

    if (p2 <= 0)
    {
        retval->setString(L"");
        return;
    }

    if (p2 > str_len)
    {
        p2 = str_len;
    }

    // allocate string for return value
    retval->setStringLen(p2);
    retval->setType(kscript::Value::typeString);
    wchar_t* result = retval->getString();

    *result = 0;

    // if offset is before beginning of string, start at beginning
    if (offset < str)
        offset = str;

    // if offset if after the end of the string, return empty string
    if (offset > str+str_len-1)
    {
        retval->setString(L"");
        return;
    }

    wcsncpy(result, offset, p2);
    result[p2] = 0;
}

void func_len(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* str = env->m_eval_params[0]->getString();
    int result = wcslen(str);
    retval->setInteger(result);
}


void func_iif(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_eval_params[0]->getBoolean())
    {
        env->m_eval_params[1]->eval(env, retval);
    }
     else
    {
        env->m_eval_params[2]->eval(env, retval);
    }
}

void func_ltrim(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* p0 = env->m_eval_params[0]->getString();
    const wchar_t* p1 = NULL;
    
    if (env->m_param_count == 2)
    {
        p1 = env->m_eval_params[1]->getString();
    }
    
    if (p1)
    {
        while (*p0 && wcschr(p1, *p0))
            p0++;
    }
     else
    {
        while (iswspace(*p0))
            p0++;
    }

    retval->setString(p0);
}


int internal_rtrim(wchar_t* result, const wchar_t* str, const wchar_t* trim_chars)
{
    int len = wcslen(str);
    if (len == 0)
    {
        *result = 0;
        return 0;
    }

    const wchar_t* last_char = str+len-1;

    if (trim_chars)
    {
        while (last_char >= str && *last_char && wcschr(trim_chars, *last_char))
        {
            last_char--;
        }
    }
     else
    {
        while (last_char >= str && isspace(*last_char))
        {
            last_char--;
        }
    }

    if (last_char < str)
    {
        *result = 0;
        return 0;
    }

    int result_length = last_char-str+1;
    memcpy(result, str, result_length * sizeof(wchar_t));
    result[result_length] = 0;
    
    return result_length;
}

void func_rtrim(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* p0 = env->m_eval_params[0]->getString();
    const wchar_t* p1 = NULL;

    if (env->m_param_count == 2)
    {
        p1 = env->m_eval_params[1]->getString();
    }

    // make sure there's enough space
    retval->setStringLen(env->m_eval_params[0]->getDataLen());
    
    // do right trim and adjust final string length
    int final_length = internal_rtrim(retval->getString(), p0, p1);
    retval->setStringLen(final_length);
}


void func_chr(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t ch[2];
    ch[0] = ' ';
    ch[1] = 0;
    
    if (env->m_param_count > 0)
        ch[0] = (wchar_t)env->m_eval_params[0]->getInteger();
    
    retval->setString(ch, 1);
}


void func_trim(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* p0 = env->m_eval_params[0]->getString();
    const wchar_t* p1 = NULL;

    if (env->m_param_count == 2)
    {
        p1 = env->m_eval_params[1]->getString();
    }

    if (p1)
    {
        while (*p0 && wcschr(p1, *p0))
            p0++;
    }
     else
    {
        while (iswspace(*p0))
            p0++;
    }

    if (!*p0)
    {
        retval->setString(L"");
        return;
    }

    // make sure there's enough space
    retval->setStringLen(env->m_eval_params[0]->getDataLen());
    
    // trim right and readjust length
    int final_length = internal_rtrim(retval->getString(), p0, p1);
    retval->setStringLen(final_length);
}


void func_lpad(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t* str = env->m_eval_params[0]->getString();
    int fill_len = env->m_eval_params[1]->getInteger();

    const wchar_t* fill_str = L" ";
    int fill_i = 0;
    int fill_size = 1;

    if (fill_len <= 0 || fill_len > tango::max_character_width)
    {
        retval->setString(L"");
        return;
    }

    if (env->m_param_count > 2)
    {
        wchar_t* p2 = env->m_eval_params[2]->getString();
        if (p2)
        {
            int p2_len = wcslen(p2);
            if (p2_len > 0)
            {
                fill_str = p2;
                fill_size = p2_len;
            }
        }
    }

    retval->setStringLen(fill_len);
    wchar_t* result = retval->getString();

    int copy_len = wcslen(str);
    if (copy_len > fill_len)
        copy_len = fill_len;
    for (int i = 0; i < fill_len; ++i)
    {
        result[i] = *(fill_str + fill_i);
        if (++fill_i == fill_size)
            fill_i = 0;
    }
    memcpy(result+(fill_len-copy_len), str, copy_len * sizeof(wchar_t));
    *(result+fill_len) = 0;
}


void func_rpad(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t* str = env->m_eval_params[0]->getString();
    int fill_len = env->m_eval_params[1]->getInteger();

    const wchar_t* fill_str = L" ";
    int fill_i = 0;
    int fill_size = 1;

    if (fill_len <= 0 || fill_len > tango::max_character_width)
    {
        retval->setString(L"");
        return;
    }

    if (env->m_param_count > 2)
    {
        wchar_t* p2 = env->m_eval_params[2]->getString();
        if (p2)
        {
            int p2_len = wcslen(p2);
            if (p2_len > 0)
            {
                fill_str = p2;
                fill_size = p2_len;
            }
        }
    }

    int copy_len = wcslen(str);

    retval->setStringLen(fill_len);

    wchar_t* result = retval->getString();

    if (copy_len > fill_len)
        copy_len = fill_len;
    memcpy(result, str, copy_len * sizeof(wchar_t));
    for (int i = copy_len; i < fill_len; ++i)
    {
        result[i] = *(fill_str + fill_i);
        if (++fill_i == fill_size)
            fill_i = 0;
    }
    *(result+fill_len) = 0;
}

void func_isnull(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_eval_params[0]->getType() == kscript::Value::typeNull)
        retval->setBoolean(true);
         else
        retval->setBoolean(false);
}

void func_empty(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    int type = env->m_eval_params[0]->getType();

    if (type == kscript::Value::typeString)
    {
        wchar_t* str = env->m_eval_params[0]->getString();
        while (iswspace(*str))
           str++;

        retval->setBoolean(*str ? false : true);
    }
     else if (type == kscript::Value::typeDouble)
    {
        retval->setBoolean(kl::dblcompare(env->m_eval_params[0]->getDouble(), 0.0) == 0 ? true : false);
    }
     else if (type == kscript::Value::typeInteger)
    {
        retval->setBoolean(env->m_eval_params[0]->getInteger() == 0 ? true : false);
    }
     else if (type == kscript::Value::typeDateTime)
    {
        kscript::ExprDateTime dt;
        dt.date = 0;
        dt.time = 0;
        env->m_eval_params[0]->getDateTime(&dt);
        retval->setBoolean((dt.date == 0 && dt.time == 0) ? true : false);
    }
     else
    {
        retval->setBoolean(false);
    }
}




static void msToDateTime(double mstime,
                         int* year,
                         int* month,
                         int* day,
                         int* hour,
                         int* minute,
                         int* second,
                         int* ms)
{
    int tempi;
    
    tempi = (int)(mstime / 86400000.0);
    mstime -= (double(tempi) * 86400000.0);

    int j = dateToJulian(1970,1,1) + tempi;
    julianToDate(j, year, month, day);

    tempi = (int)(mstime / 3600000.0);
    mstime -= (double(tempi) * 3600000.0);
    *hour = tempi;

    tempi = (int)(mstime / 60000.0);
    mstime -= (double(tempi) * 60000.0);
    *minute = tempi;

    tempi = (int)(mstime / 1000.0);
    mstime -= (double(tempi) * 1000.0);
    *second = tempi;

    *ms = (int)mstime;
}

void func_date(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprDateTime dt;
    dt.date = 0;
    dt.time = 0;

    if (env->m_param_count <= 2)
    {
        if (env->m_eval_params[0]->isNumber())
        {
            // unix timestamp*1000 milliseconds
            double d = env->m_eval_params[0]->getDouble();
            
            int yy,mm,dd,h,m,s,ms;
            msToDateTime(d, &yy, &mm, &dd, &h, &m, &s, &ms);
            retval->setDateTime(yy, mm, dd, h, m, s, ms);
            return;
        }
        
        kscript::str2date(&dt,
            env->m_eval_params[0]->getString(),
            env->m_param_count > 1 ? env->m_eval_params[1]->getString() : NULL);
    }
     else
    {
        int y,m,d;
        y = env->m_eval_params[0]->getInteger();
        m = env->m_eval_params[1]->getInteger();
        d = env->m_eval_params[2]->getInteger();

        // if year == 0 and month == 0 and day == 0, then
        // a null date is required

        if (y != 0 || m != 0 || d != 0)
        {
            dt.date = dateToJulian(y, m, d);
        }

        if (dt.date && env->m_param_count == 6)
        {
            int h = env->m_eval_params[3]->getInteger();
            int m = env->m_eval_params[4]->getInteger();
            int s = env->m_eval_params[5]->getInteger();

            dt.time = (s*1000) + (m*60000) + (h*3600000);
        }
    }

    retval->setDateTime(dt.date, dt.time);
}


void func_curdate(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    time_t ts;
    struct tm t;
    
    time(&ts);
    localtime_r(&ts, &t);

    retval->setDateTime(1900+t.tm_year, t.tm_mon+1, t.tm_mday,
                        0,0,0);
}

void func_curdatetime(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    time_t ts;
    struct tm t;
    
    time(&ts);
    localtime_r(&ts, &t);

    retval->setDateTime(1900+t.tm_year, t.tm_mon+1, t.tm_mday,
                        t.tm_hour, t.tm_min, t.tm_sec);
}

void func_striptime(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprDateTime d = env->getParam(0)->getDateTime();
    retval->setDateTime(d.date, 0);
}


void func_min(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_eval_params[0]->isNumber())
    {
        double d = env->m_eval_params[0]->getDouble();
        double c;
        for (size_t i = 1; i < env->m_param_count; i++)
        {
            c = env->m_eval_params[i]->getDouble();
            if (c < d)
                d = c;
        }
        retval->setDouble(d);
        return;
    }

    if (env->m_eval_params[0]->getType() == kscript::Value::typeDateTime)
    {
        kscript::ExprDateTime d;
        kscript::ExprDateTime c;
        env->m_eval_params[0]->getDateTime(&d);
        for (size_t i = 1; i < env->m_param_count; i++)
        {
            env->m_eval_params[i]->getDateTime(&c);
            if (dtCompare(&c, &d) < 0)
                d = c;
        }

        retval->setDateTime(d.date, d.time);
        return;
    }

    if (env->m_eval_params[0]->getType() == kscript::Value::typeString)
    {
        wchar_t* d;
        wchar_t* c;
        d = env->m_eval_params[0]->getString();
        for (size_t i = 1; i < env->m_param_count; i++)
        {
            c = env->m_eval_params[i]->getString();
            if (wcscmp(c, d) < 0)
                d = c;
        }

        retval->setString(d);
        return;
    }
}

void func_avg(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_eval_params[0]->isNumber())
    {
        double total = 0.0;
        for (size_t i = 0; i < env->m_param_count; i++)
        {
            total += env->m_eval_params[i]->getDouble();
        }
        total = (total / (double)env->m_param_count);
        retval->setDouble(total);
        return;
    }

    retval->setDouble(0);
}

void func_sum(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_eval_params[0]->isNumber())
    {
        double total = 0.0;
        for (size_t i = 0; i < env->m_param_count; i++)
        {
            total += env->m_eval_params[i]->getDouble();
        }
        retval->setDouble(total);
        return;
    }

    retval->setDouble(0);
}

void func_stddev(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kl::Statistics stats;

    if (env->m_eval_params[0]->isNumber())
    {
        double total = 0.0;
        for (size_t i = 0; i < env->m_param_count; i++)
        {
            stats.push(env->m_eval_params[i]->getDouble());
        }
    }

    retval->setDouble(stats.stddev());
}

void func_variance(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kl::Statistics stats;

    if (env->m_eval_params[0]->isNumber())
    {
        double total = 0.0;
        for (size_t i = 0; i < env->m_param_count; i++)
        {
            stats.push(env->m_eval_params[i]->getDouble());
        }
    }

    retval->setDouble(stats.variance());
}

void func_max(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->m_eval_params[0]->isNumber())
    {
        double d = env->m_eval_params[0]->getDouble();
        double c;
        for (size_t i = 1; i < env->m_param_count; i++)
        {
            c = env->m_eval_params[i]->getDouble();
            if (c > d)
                d = c;
        }
        retval->setDouble(d);
        return;
    }

    if (env->m_eval_params[0]->getType() == kscript::Value::typeDateTime)
    {
        kscript::ExprDateTime d;
        kscript::ExprDateTime c;
        env->m_eval_params[0]->getDateTime(&d);
        for (size_t i = 1; i < env->m_param_count; i++)
        {
            env->m_eval_params[i]->getDateTime(&c);
            if (dtCompare(&c, &d) > 0)
                d = c;
        }

        retval->setDateTime(d.date, d.time);
        return;
    }

    if (env->m_eval_params[0]->getType() == kscript::Value::typeString)
    {
        wchar_t* d;
        wchar_t* c;
        d = env->m_eval_params[0]->getString();
        for (size_t i = 1; i < env->m_param_count; i++)
        {
            c = env->m_eval_params[i]->getString();
            if (wcscmp(c, d) > 0)
                d = c;
        }

        retval->setString(d);
        return;
    }
}


void func_int(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    double d = env->m_eval_params[0]->getDouble();

    if (d > 0)
    {
        d = floor(d);
    }
     else
    {
        d = ceil(d);
    }

    retval->setDouble(d);
}


void func_strpart(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t* str = env->m_eval_params[0]->getString();
    int target = env->m_eval_params[1]->getInteger();
    const wchar_t* delimiters = L" ";

    if (env->m_param_count >= 3)
    {
        delimiters = env->m_eval_params[2]->getString();
        if (wcslen(delimiters) == 0)
            delimiters = L" ";
    }

    bool ignore_consecutive_delims = true;

    if (env->m_param_count >= 4)
    {
        ignore_consecutive_delims = env->m_eval_params[3]->getBoolean();
    }

    if (target <= 0)
    {
        retval->setString(L"");
        return;
    }

    wchar_t* start = NULL;
    wchar_t* end = NULL;

    int token = 1;
    start = str;

    if (ignore_consecutive_delims)
    {
        while (*str)
        {
            if (wcschr(delimiters, *str))
            {
                if (token == target)
                {
                    end = str;
                    break;
                }

                while (*str && wcschr(delimiters, *str))
                    str++;
                
                if (!*str)
                    break;

                token++;
                start = str;
            }
            str++;
        }
    }
     else
    {
        while (*str)
        {
            if (wcschr(delimiters, *str))
            {
                if (token == target)
                {
                    end = str;
                    break;
                }

                token++;
                start = str+1;
            }
            str++;
        }
    }


    if (target > token)
    {
        retval->setString(L"");
        return;
    }

    if (end == NULL)
        end = str;

    retval->setString(start, end-start);
}


void func_reverse(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t* str = env->m_eval_params[0]->getString();
    int str_len = env->m_eval_params[0]->getDataLen();

    retval->setStringLen(str_len);

    wchar_t* c = str+str_len-1;
    wchar_t* r = retval->getString();

    while (c >= str)
    {
        *r = *c;
        c--;
        r++;
    }
    *r = 0;
}


void func_space(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    int n = env->m_eval_params[0]->getInteger();
    int i;

    if (n <= 0)
    {
        retval->setString(L"");
        return;
    }

    retval->setStringLen(n);
    wchar_t* result = retval->getString();
    for (i = 0; i < n; ++i)
        *(result+n) = L' ';
    *(result+n) = 0;
}

void func_str(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    int param_type = env->m_eval_params[0]->getType();

    if (param_type == kscript::Value::typeBoolean)
    {
        if (env->m_eval_params[0]->getBoolean())
        {
            retval->setString(L"T");
        }
         else
        {
            retval->setString(L"F");
        }
    }
     else if (param_type == kscript::Value::typeDateTime)
    {
        // handle date-to-string conversion (converts a date to YYYYMMDD)
        wchar_t buf[64];
        int y, m, d;
        kscript::ExprDateTime dt;

        env->m_eval_params[0]->getDateTime(&dt);
        julianToDate(dt.date, &y, &m, &d);

        if (dt.date == 0)
        {
            retval->setString(L"00000000");
            return;
        }

        swprintf(buf, 64, L"%04d%02d%02d", y, m, d);
        retval->setString(buf);
        return;
    }
     else
    {
        int fill_dec = 0;

        double number = env->m_eval_params[0]->getDouble();

        if (env->m_param_count >= 2)
        {
            fill_dec = env->m_eval_params[1]->getInteger();
        }

        if (fill_dec < 0 || fill_dec > 12)
            fill_dec = 12;

        wchar_t buf[128];
        swprintf(buf, 128, L"%.*f", fill_dec, number);

        retval->setString(buf);
    }
}








static void dbltowstr(wchar_t* str, double d)
{
    for (int i = 0; i < 15; ++i)
    {
        swprintf(str, 255, L"%.*f", i, d);
        if (kl::dblcompare(kl::wtof(str), d) == 0)
            return;
    }
    swprintf(str, 255, L"%.5f", d);
}

static wchar_t getDecimalChar()
{
    static wchar_t ch = 0;
    
    if (!ch)
    {
        lconv* l = localeconv();
        ch = (unsigned char)*l->decimal_point;
    }

    return ch;
}

static wchar_t getThousandsSeparatorChar()
{
    static wchar_t ch = 0;
    
    if (!ch)
    {
        lconv* l = localeconv();
        ch = (unsigned char)*l->thousands_sep;
    }

    return ch;
}


inline wchar_t getLeftDigit(const wchar_t* ch, const wchar_t* dec, size_t digit)
{
    const wchar_t* digit_ptr = dec - digit;
    if (digit_ptr < ch || *digit_ptr < L'0' || *digit_ptr > L'9')
        return L'0';
    return *digit_ptr;
}

inline wchar_t getRightDigit(const wchar_t* ch, const wchar_t* dec, size_t digit)
{
    size_t l = wcslen(dec);
    if (digit >= l)
        return L'0';
    return *(dec+digit);
}

void func_tochar_number(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t strnum[255];
    wchar_t* dec;
    wchar_t* p;
    wchar_t digit;
    size_t left_format_digits = 0;  // number of left digits in the format string
    size_t right_format_digits = 0; // number of right digits in the format string
    size_t total_left_digits = 0;   // number of left digits in the number
    size_t total_right_digits = 0;  // number of right digits in the number
    size_t zero_left_digits = 0;   // number of left digits in the number
    size_t zero_right_digits = 0;  // number of right digits in the number
    size_t l, r;
    bool left;
    bool sign_left = true;      // sign is on the left side
    bool sign_always = false;
    bool padding = true;        // true if padding is active
    bool is_negative = false;
    bool digit_encountered = false;
    bool digit_printed = false;
    wchar_t currency_sign = 0;

    // count the number of positions to the left
    // of the decimal in the format string
    p = env->getParam(1)->getString();
    left = true;
    while (*p)
    {
        if (*p == '.' || *p == 'D' || *p == 'd')
            left = false;
        if (*p == '9' || *p == '0')
        {
            digit_encountered = true;
            if (left)
                left_format_digits++;
                 else
                right_format_digits++;
        }
        if (*p == '$' || *p == (wchar_t)8364 /* euro symbol */)
        {
            currency_sign = *p;
        }
        if (*p == 'S' || *p == 's')
        {
            sign_always = true;
            if (!digit_encountered)
                sign_left = true;
                 else
                sign_left = false;
        }
        if ((*p == 'M' || *p == 'm') &&
            (*(p+1) == 'I' || *(p+1) == 'i'))
        {
            if (!digit_encountered)
                sign_left = true;
                 else
                sign_left = false;
            p += 2;
            continue;
        }
        if (left)
        {
            if (*p == '0')  
                zero_left_digits++;
            if (*p == '9' && zero_left_digits > 0)
                zero_left_digits++;
        }
         else
        {
            if (*p == '0')  
                zero_right_digits = right_format_digits;
        }
        
        ++p;
    }



    // find decimal point in the input number
    dbltowstr(strnum, kl::dblround(env->getParam(0)->getDouble(), right_format_digits));
    dec = NULL;
    p = strnum;
    left = true;
    while (*p)
    {
        if (*p >= '0' && *p <= '9')
        {
            if (left)
                total_left_digits++;
                 else
                total_right_digits++;
        }
        if (*p == '.' || *p == ',')
        {
            left = false;
            dec = p;
        }
        ++p;
    }
    if (!dec)
        dec = p;
    
    
    // if the number of left decimals surpasses the number of
    // spaces in the number format, return an overflow string
    if (total_left_digits > left_format_digits)
    {
        retval->setString(L"######");
        return;
    }
    
    
    if (kl::dblcompare(env->getParam(0)->getDouble(), 0.0) < 0)
        is_negative = true;


    std::wstring result;
    result.reserve(20);
    
    l = left_format_digits;
    r = 0;
    
    p = env->getParam(1)->getString();
    left = true;

    while (*p)
    {
        if ((*p == 'F' || *p == 'f') &&
            (*(p+1) == 'M' || *(p+1) == 'm'))
        {
            padding = false;
            p += 2;
            continue;
        } 
         else
        if (*p == '.' || *p == 'D' || *p == 'd')
        {
            if (*p == '.')
                result += *p;
                 else
                result += getDecimalChar();
            left = false;
            digit_printed = true;
        }
         else if (*p == ',' || *p == 'G' || *p == 'g')
        {
            if (!digit_printed)
            {
                if (padding)
                    result += L' ';
            }
             else
            {
                if (*p == ',')
                    result += *p;
                     else
                    result += getThousandsSeparatorChar();
            }
        }
         else if (*p == '9' || *p == '0')
        {
            if (left)
            {
                digit = getLeftDigit(strnum, dec, l);
                

                
                if (sign_left && !digit_printed &&
                    (l <= zero_left_digits || digit != '0'))
                {
                    if (is_negative)
                    {
                        result += L'-';
                    }
                     else
                    {
                        if (sign_always)
                        {
                            result += L'+';
                        }
                         else
                        {
                            if (padding)
                                result += L' ';
                        }    
                    }
                    
                    if (currency_sign)
                        result += currency_sign;
                }
            
                
                
                if (l > zero_left_digits)
                {
                    if (digit != '0' || digit_printed)
                    {
                        result += digit;
                        digit_printed = true;
                    }
                     else
                    {
                        if (padding)
                            result += ' ';
                    }
                }
                 else
                {
                    result += digit;
                    digit_printed = true;
                }
                
                --l;
            }
             else
            {
                ++r;
                digit = getRightDigit(strnum, dec, r);
                
                if (r > zero_right_digits)
                {
                    if (digit != '0' || r <= total_right_digits)
                    {
                        result += digit;
                    }
                     else
                    {
                        if (padding)
                            result += '0';
                    }
                }
                 else
                {
                    result += digit;
                }
            }

        }


        
        ++p;
    }


    if (!sign_left)
    {
        if (is_negative)
        {
            result += L'-';
        }
         else
        {
            if (sign_always)
            {
                result += L'+';
            }
             else
            {
                if (padding)
                    result += L' ';
            }    
        }
    }


    retval->setString(result);

}






inline bool isFormatString(const wchar_t* format_str,
                           const wchar_t* format_identifier,
                           bool ignore_case = false)
{
    wchar_t c1, c2;
    while (*format_str && *format_identifier)
    {
        c1 = *format_str;
        c2 = *format_identifier;
        if (ignore_case)
        {
            c1 = towupper(c1);
            c2 = towupper(c2);
        }
        if (c1 != c2)
            return false;
        ++format_str;
        ++format_identifier;
    }
    
    if (!*format_identifier)
        return true;
        
    return false;
}

static size_t getMaxLocaleMonthLength()
{
    static size_t len = 0;
    
    if (len == 0)
    {
        wchar_t buf[255];
        struct tm tm;
        tm.tm_sec = 0;
        tm.tm_min = 0;
        tm.tm_hour = 6;
        tm.tm_mday = 1;
        tm.tm_year = 108;
        tm.tm_isdst = 0;
        for (int month = 0; month < 12; ++month)
        {
            tm.tm_mon = month;
            wcsftime(buf, 64, L"%B", &tm);
            size_t l = wcslen(buf);
            if (l > len)
                len = l;
        }
    }
    
    return len;
}

static size_t getMaxLocaleWeekdayLength()
{
    static size_t len = 0;
    
    if (len == 0)
    {
        wchar_t buf[255];
        struct tm tm;
        tm.tm_sec = 0;
        tm.tm_min = 0;
        tm.tm_hour = 6;
        tm.tm_mday = 1;
        tm.tm_mon = 5; // june
        tm.tm_year = 108; // 2008
        tm.tm_isdst = 0;
        for (int dw = 0; dw < 7; ++dw)
        {
            tm.tm_mday = dw+1;
            tm.tm_wday = dw;
            wcsftime(buf, 64, L"%A", &tm);
            size_t l = wcslen(buf);
            if (l > len)
                len = l;
        }
    }
    
    return len;
}

void func_tochar_datetime(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    tango::DateTime dt;
    const wchar_t* format;
    const wchar_t* f;
    wchar_t buf[64];
    std::wstring res;
    bool padding = true;
    
    kscript::ExprDateTime edt;
    env->getParam(0)->getDateTime(&edt);
    tango::datetime_t tdt = edt.date;
    tdt <<= 32;
    tdt |= edt.time;
    dt.setDateTime(tdt);


    // if we have a null date, return an empty string
    if (dt.isNull())
    {
        retval->setString(L"");
        return;
    }

    struct tm tm;
    tm.tm_sec = dt.getSecond();
    tm.tm_min = dt.getMinute();
    tm.tm_hour = dt.getHour();
    tm.tm_mday = dt.getDay();
    tm.tm_mon = dt.getMonth()-1;
    tm.tm_year = dt.getYear()-1900;
    tm.tm_wday = getDayOfWeek(dt.getYear(), dt.getMonth(), dt.getDay());

    if (env->getParamCount() >= 2)
        format = env->getParam(1)->getString();
         else
        format = L"YYYY-MM-DD";
    
    res.reserve(32);
    
    f = format;
    while (*f)
    {
        if (isFormatString(f, L"FM", true))
        {
            // turn padding off
            padding = false;
            f += 2;
        }
         else if (isFormatString(f, L"SYYYY"))
        {
            swprintf(buf, 64, L"%c%04d", dt.getYear() < 0 ? '-':' ', abs(dt.getYear()));
            buf[5] = 0;
            if (padding)
                res += buf;
                 else
                res += (buf+1);
            f += 5;
        }
         else if (isFormatString(f, L"YYYY"))
        {
            swprintf(buf, 64, L"%04d", abs(dt.getYear()));
            buf[4] = 0;
            res += buf;
            f += 4;
        }
         else if (isFormatString(f, L"YYY"))
        {
            swprintf(buf, 64, L"%03d", dt.getYear() % 1000);
            buf[3] = 0;
            res += buf;
            f += 3;
        }
         else if (isFormatString(f, L"YY"))
        {
            swprintf(buf, 64, L"%02d", dt.getYear() % 100);
            buf[2] = 0;
            res += buf;
            f += 2;
        }
         else if (isFormatString(f, L"Y"))
        {
            swprintf(buf, 64, L"%d", dt.getYear() % 10);
            buf[1] = 0;
            res += buf;
            f += 1;
        }
         else if (isFormatString(f, L"MM"))
        {
            swprintf(buf, 64, L"%02d", dt.getMonth());
            buf[2] = 0;
            res += buf;
            f += 2;
        }
         else if (isFormatString(f, L"DDTH", true))
        {
            int dd = dt.getDay();
            swprintf(buf, 64, L"%02d", dd);
            buf[2] = 0;
            
            if ((dd >= 1 && dd <= 3) ||
                (dd >= 21 && dd <= 23) ||
                (dd > 30))
            {
                switch (dd % 10)
                {
                    case 1: wcscat(buf, L"ST"); break;
                    case 2: wcscat(buf, L"ND"); break;
                    case 3: wcscat(buf, L"RD"); break;
                    default: wcscat(buf, L"TH"); break;
                }
            }
             else
            {
                wcscat(buf, L"TH");
            }
            
            if (*f == 'd')
                kl::makeLower(buf);
            
            res += buf;
            f += 4;
        }
         else if (isFormatString(f, L"DD", true))
        {
            swprintf(buf, 64, L"%02d", dt.getDay());
            buf[2] = 0;
            res += buf;
            f += 2;
        }
         else if (isFormatString(f, L"DY", true) || isFormatString(f, L"DAY", true))
        {
            int offset = 0;
            
            if (isFormatString(f, L"DAY", true))
            {
                wcsftime(buf, 64, L"%A", &tm);
                kl::makeUpper(buf);
                if (padding)
                {
                    size_t max_len = getMaxLocaleWeekdayLength();
                    while (wcslen(buf) < max_len)
                        wcscat(buf, L" ");
                }
                offset = 3;
            }
             else
            {
                wcsftime(buf, 64, L"%a", &tm);
                kl::makeUpper(buf);
                offset = 2;
            }
            
            if (*f == 'd')
            {
                // "dy"/"day"
                kl::makeLower(buf);
            }
             else
            {
                if (*(f+1) == 'a' || *(f+1) == 'y')
                {
                    // "Month"
                    kl::makeLower(buf);
                    buf[0] = towupper(buf[0]);
                }
            }

            res += buf;
            f += offset;
        }
         else if (isFormatString(f, L"MON", true) || isFormatString(f, L"MONTH", true))
        {
            int offset = 0;
            
            if (isFormatString(f, L"MONTH", true))
            {
                wcsftime(buf, 64, L"%B", &tm);
                kl::makeUpper(buf);
                if (padding)
                {
                    size_t max_len = getMaxLocaleMonthLength();
                    while (wcslen(buf) < max_len)
                        wcscat(buf, L" ");
                }
                offset = 5;
            }
             else
            {
                wcsftime(buf, 64, L"%b", &tm);
                kl::makeUpper(buf);
                offset = 3;
            }
            
            if (*f == 'm')
            {
                // "month"
                kl::makeLower(buf);
            }
             else
            {
                if (*(f+1) == 'o')
                {
                    // "Month"
                    kl::makeLower(buf);
                    buf[0] = towupper(buf[0]);
                }
            }

            res += buf;
            f += offset;
        }
         else if (isFormatString(f, L"HH24", true))
        {
            swprintf(buf, 64, L"%02d", dt.getHour());
            buf[2] = 0;
            res += buf;
            f += 4;
        }
         else if (isFormatString(f, L"HH12", true))
        {
            int hh = dt.getHour();
            if (hh > 12)
                hh -= 12;
            swprintf(buf, 64, L"%02d", hh);
            buf[2] = 0;
            res += buf;
            f += 4;
        }
         else if (isFormatString(f, L"HH", true))
        {
            int hh = dt.getHour();
            if (hh > 12)
                hh -= 12;
            swprintf(buf, 64, L"%02d", hh);
            buf[2] = 0;
            res += buf;
            f += 2;
        }
         else if (isFormatString(f, L"MI", true))
        {
            swprintf(buf, 64, L"%02d", dt.getMinute());
            buf[2] = 0;
            res += buf;
            f += 2;
        } 
         else if (isFormatString(f, L"SS", true))
        {
            swprintf(buf, 64, L"%02d", dt.getSecond());
            buf[2] = 0;
            res += buf;
            f += 2;
        }
         else if (isFormatString(f, L"PM", true) ||
                  isFormatString(f, L"AM", true))
        {
            if (towlower(*f) != *f)
            {
                // upper case
                if (dt.getHour() > 12)
                    res += L"PM";
                     else
                    res += L"AM";
            }
             else
            {
                // upper case
                if (dt.getHour() > 12)
                    res += L"pm";
                     else
                    res += L"am";
            }
            f += 2;
        }
         else if (isFormatString(f, L"P.M.", true) ||
                  isFormatString(f, L"A.M.", true))
        {
            if (towlower(*f) != *f)
            {
                // upper case
                if (dt.getHour() > 12)
                    res += L"P.M.";
                     else
                    res += L"A.M.";
            }
             else
            {
                // upper case
                if (dt.getHour() > 12)
                    res += L"p.m.";
                     else
                    res += L"a.m.";
            }
            f += 4;
        }
         else if (isFormatString(f, L"RM", true))
        {
            switch (dt.getMonth())
            {
                default:
                case  1: wcscpy(buf, L"I");    break;
                case  2: wcscpy(buf, L"II");   break;
                case  3: wcscpy(buf, L"III");  break;
                case  4: wcscpy(buf, L"IV");   break;
                case  5: wcscpy(buf, L"V");    break;
                case  6: wcscpy(buf, L"VI");   break;
                case  7: wcscpy(buf, L"VI");   break;
                case  8: wcscpy(buf, L"VIII"); break;
                case  9: wcscpy(buf, L"IX");   break;
                case 10: wcscpy(buf, L"X");    break;
                case 11: wcscpy(buf, L"XI");   break;
                case 12: wcscpy(buf, L"XII");  break;
            }
            
            if (padding)
            {
                while (wcslen(buf) < 4)
                    wcscat(buf, L" ");
            }
            
            if (*f == 'r')
            {
                kl::makeLower(buf);
            }
             else
            {
                if (*(f+1) == 'm')
                {
                    // Rm
                    kl::makeLower(buf);
                    buf[0] = towupper(buf[0]);
                }
            }
            
            res += buf;
            f += 2;
        }
         else
        {
            if (*f == '"')
            {
                ++f;
                while (*f)
                {
                    if (*f == '"')
                    {
                        ++f;
                        if (*f == '"')
                            res += L'"';
                             else
                            break;
                    }
                     else
                    {
                        res += *f;
                    }
                    
                    ++f;
                }
            }
             else
            {
                if (!iswalpha(*f))
                {
                    res += *f;
                    f++;
                }
                 else
                {
                    // invalid character
                    retval->setString(L"");
                    return;
                }
            }
        }
        
    }
    
    
    retval->setString(res);
}






void func_tochar(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setString(L"");
        return;
    }
    
    if (env->m_eval_params[0]->isNumber())
    {
        func_tochar_number(env, param, retval);
    }
     else if (env->m_eval_params[0]->getType() == kscript::Value::typeDateTime)
    {
        func_tochar_datetime(env, param, retval);
    }
}



void func_lower(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t* p0 = env->m_eval_params[0]->getString();
    wchar_t* dest;

    retval->setStringLen(env->m_eval_params[0]->getDataLen());
    retval->setType(kscript::Value::typeString);
    dest = retval->getString();
    wcscpy(dest, p0);

    while (*dest)
    {
        *dest = towlower(*dest);
        dest++;
    }
}

void func_upper(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t* p0 = env->m_eval_params[0]->getString();
    wchar_t* dest;

    retval->setStringLen(env->m_eval_params[0]->getDataLen());
    dest = retval->getString();
    wcscpy(dest, p0);

    while (*dest)
    {
        *dest = towupper(*dest);
        dest++;
    }
}

void func_val(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setDouble(kl::nolocale_wtof(env->m_eval_params[0]->getString()));
}

void func_year(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprDateTime dt;
    env->m_eval_params[0]->getDateTime(&dt);

    if (dt.date == 0)
    {
        retval->setInteger(0);
        return;
    }

    int year, month, day;
    julianToDate(dt.date, &year, &month, &day);
    retval->setInteger(year);
}


void func_month(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprDateTime dt;
    env->m_eval_params[0]->getDateTime(&dt);

    if (dt.date == 0)
    {
        retval->setInteger(0);
        return;
    }

    int year, month, day;
    julianToDate(dt.date, &year, &month, &day);
    retval->setInteger(month);
}


void func_day(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprDateTime dt;
    env->m_eval_params[0]->getDateTime(&dt);

    if (dt.date == 0)
    {
        retval->setInteger(0);
        return;
    }

    int year, month, day;
    julianToDate(dt.date, &year, &month, &day);
    retval->setInteger(day);
}

void func_hour(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprDateTime dt;
    env->m_eval_params[0]->getDateTime(&dt);

    retval->setInteger((dt.time+1)/3600000);
}

void func_minute(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprDateTime dt;
    env->m_eval_params[0]->getDateTime(&dt);

    dt.time -= ((dt.time+1)/3600000)*3600000;

    retval->setInteger(dt.time/60000);
}

void func_second(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprDateTime dt;
    env->m_eval_params[0]->getDateTime(&dt);

    dt.time -= ((dt.time+1)/3600000)*3600000;
    dt.time -= (dt.time/60000)*60000;
    retval->setInteger(dt.time/1000);
}



void func_dayofweek(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    int y, m, d;
    kscript::ExprDateTime dt;

    env->m_eval_params[0]->getDateTime(&dt);

    if (dt.date == 0)
    {
        retval->setInteger(0);
        return;
    }

    julianToDate(dt.date, &y, &m, &d);

    retval->setInteger(getDayOfWeek(y, m, d)+1);
}

void func_dayname(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t buf[64];
    int y, m, d;
    kscript::ExprDateTime dt;
    struct tm time_struct;

    env->m_eval_params[0]->getDateTime(&dt);

    if (dt.date == 0)
    {
        retval->setString(L"");
        return;
    }

    julianToDate(dt.date, &y, &m, &d);

    memset(&time_struct, 0, sizeof(struct tm));
    time_struct.tm_wday = getDayOfWeek(y, m, d);
    if (time_struct.tm_wday < 0 || time_struct.tm_wday >  6)
    {
        retval->setString(L"");
        return;
    }
    
    wcsftime(buf, 64, L"%A", &time_struct);

    retval->setString(buf);
}

void func_monthname(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t buf[64];
    int y, m, d;
    kscript::ExprDateTime dt;
    struct tm time_struct;

    env->m_eval_params[0]->getDateTime(&dt);

    if (dt.date == 0)
    {
        retval->setString(L"");
        return;
    }


    julianToDate(dt.date, &y, &m, &d);

    memset(&time_struct, 0, sizeof(struct tm));
    time_struct.tm_mon = m-1;
    wcsftime(buf, 64, L"%B", &time_struct);

    retval->setString(buf);
}


void func_ascend(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprElement* p0 = env->m_params[0];
    kscript::Value* v0 = env->m_eval_params[0];

    switch (p0->getType())
    {
        case kscript::Value::typeString:
        {
            // calculate length for the key
            env->m_params[1]->eval(env, env->m_eval_params[1]);
            int len = env->m_eval_params[1]->getInteger();

            p0->eval(env, retval);
            retval->setType(kscript::Value::typeBinary);
        }
        break;

        case kscript::Value::typeDateTime:
        {
            kscript::ExprDateTime dt;
            p0->eval(env, v0);
            v0->getDateTime(&dt);

            unsigned char dest[8];
            dest[0] = (dt.date >> 24) & 0xff;
            dest[1] = (dt.date >> 16) & 0xff;
            dest[2] = (dt.date >> 8) & 0xff;
            dest[3] = dt.date & 0xff;
            dest[4] = (dt.time >> 24) & 0xff;
            dest[5] = (dt.time >> 16) & 0xff;
            dest[6] = (dt.time >> 8) & 0xff;
            dest[7] = dt.time & 0xff;
            retval->setBinary(dest, 8, 8);
        }
        break;

        case kscript::Value::typeDouble:
        {
            p0->eval(env, v0);

            double d = v0->getDouble();
            unsigned char result[sizeof(double)];
            unsigned char* src = (unsigned char*)&d;
            unsigned char* dest = result;

            int i = 8;
            while (i--)
            {
                *dest = *(src+i);
                dest++;
            }

            if (result[0] & 0x80)
            {
                for (i = 0; i < 8; i++)
                {
                    result[i] = ~result[i];
                }
            }
             else
            {
                result[0] |= 0x80;
            }

            retval->setBinary(result, sizeof(double));
        }
        break;

        case kscript::Value::typeInteger:
        {
            p0->eval(env, v0);

            unsigned int i = (unsigned int)v0->getInteger();
            unsigned char result[sizeof(int)];

            result[0] = (i >> 24) & 0xff;
            result[1] = (i >> 16) & 0xff;
            result[2] = (i >> 8) & 0xff;
            result[3] = (i & 0xff);

            if (result[0] & 0x80)
            {
                result[0] &= 0x7f;
            }
             else
            {
                result[0] |= 0x80;
            }

            retval->setBinary(result, sizeof(int));
        }
        break;

        case kscript::Value::typeBoolean:
        {
            p0->eval(env, v0);

            unsigned char c = (v0->getBoolean() ? 2 : 1);
            retval->setBinary(&c, 1);
        }
        break;

    }
}

// does the same thing as ASCEND() except returns an ascii representation
void func_asckey(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprElement* p0 = env->m_params[0];
    kscript::Value* v0 = env->m_eval_params[0];

    switch (p0->getType())
    {
        case kscript::Value::typeString:
        {
            // calculate length for the key
            env->m_params[1]->eval(env, env->m_eval_params[1]);
            int len = env->m_eval_params[1]->getInteger();

            p0->eval(env, v0);
            std::string s = kl::tostring(v0->getString());

            retval->setType(kscript::Value::typeBinary);
            retval->allocMem(len);
            retval->setDataLen(len);
            unsigned char* result = retval->getBinary();
            memset(result, 0, len);
            
            unsigned int cpylen = (unsigned int)len;
            if (s.length() < cpylen)
                cpylen = s.length();
            memcpy(result, s.c_str(), cpylen);
        }
        break;

        default:
            func_ascend(env, param, retval);
    }
}

void func_descend(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    func_ascend(env, NULL, retval);
    unsigned char* str = retval->getBinary();
    int i = retval->getDataLen();

    while (1)
    {
        if (--i < 0)
            break;
        *(str+i) = ~*(str+i);
    }
}



// clean types

enum
{
    CleanType_All = 0,
    CleanType_Prefix = 1,
    CleanType_Suffix = 2,
    CleanType_PrefixSuffix = 3
};

#define IS_W_DIGIT(c) (((c) >= L'0') && ((c) <= '9'))

void func_clean(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    int clean_type = 0;
    bool zero_char = false;

    if (env->m_param_count > 2)
    {
        zero_char = env->m_eval_params[2]->getBoolean();
    }

    if (env->m_param_count > 1)
    {
        clean_type = env->m_eval_params[1]->getInteger();
    }

    wchar_t* str = env->m_eval_params[0]->getString();
    int str_len = env->m_eval_params[0]->getDataLen();

    if (str_len <= 0)
    {
        // if they gave us an empty string, give them back an empty string

        retval->setString(L"");
        return;
    }

    wchar_t* start = str;
    wchar_t* end = start + str_len - 1;

    // clean prefix
    if (clean_type == CleanType_All ||
        clean_type == CleanType_Prefix ||
        clean_type == CleanType_PrefixSuffix)
    {
        while (!IS_W_DIGIT(*start) || (zero_char && *start == L'0'))
        {
            start++;
            if (!*start)
            {
                retval->setString(L"");
                return;
            }
        }
    }

    // clean suffix
    if (clean_type == CleanType_All ||
        clean_type == CleanType_Suffix ||
        clean_type == CleanType_PrefixSuffix)
    {
        while (!IS_W_DIGIT(*end) || (zero_char && *end == L'0'))
        {
            if (end == start)
            {
                retval->setString(L"");
                return;
            }
            end--;
        }
    }

    // clean interior
    if (clean_type == CleanType_All)
    {
        retval->setStringLen(end-start+10);
        wchar_t* result = retval->getString();
        wchar_t* p;
        wchar_t* out = result;

        for (p = start; p <= end; ++p)
        {
            if (zero_char)
            {
                // skip zero
                if (*p == L'0')
                    continue;
            }

            if (IS_W_DIGIT(*p))
            {
                *out = *p;
                out++;
            }
        }

        *out = L'\0';
        retval->setStringLen(out-result);
        return;
    }


    retval->setString(start, end-start+1);
}

void func_filterdigits(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    // note: this function is similar to clean, except that it doesn't have
    // all the various flags; it simply removes all non-numeric characters 
    // and returns the resulting digits as a string

    wchar_t* str = env->m_eval_params[0]->getString();
    int str_len = env->m_eval_params[0]->getDataLen();

    if (str_len <= 0)
    {
        retval->setString(L"");
        return;
    }

    std::wstring result;
    while (*str)
    {
        if (IS_W_DIGIT(*str))
            result.append(str,1);
        str++;
    }

    retval->setString(result);
}

void func_filterchars(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->getParamCount() < 2)
    {
        retval->setNull();
        return;
    }
    
    wchar_t* str = env->m_eval_params[0]->getString();
    wchar_t* chars = env->m_eval_params[1]->getString();
    int str_len = env->m_eval_params[0]->getDataLen();

    std::wstring result;
    while (*str)
    {
        if (wcschr(chars, *str))
            result.append(str,1);
        str++;
    }

    retval->setString(result);
}

void func_stripchars(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->getParamCount() < 2)
    {
        retval->setNull();
        return;
    }
    
    wchar_t* str = env->m_eval_params[0]->getString();
    wchar_t* chars = env->m_eval_params[1]->getString();
    int str_len = env->m_eval_params[0]->getDataLen();

    std::wstring result;
    while (*str)
    {
        if (!wcschr(chars, *str))
            result.append(str,1);
        str++;
    }

    retval->setString(result);
}

void func_sortchars(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t* p0_str = env->m_eval_params[0]->getString();
    int p0_len = env->m_eval_params[0]->getDataLen();

    retval->setStringLen(p0_len);
    wchar_t* s_start = retval->getString();
    wchar_t* s_end = s_start+p0_len;

    wcscpy(s_start, p0_str);
    if (p0_len == 0)
        return;

    std::sort(s_start, s_end);
}


void func_left(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t* p0 = env->m_eval_params[0]->getString();
    int p0_len = env->m_eval_params[0]->getDataLen();
    int p1 = env->m_eval_params[1]->getInteger();

    if (p1 <= 0 || p1 > tango::max_character_width)
    {
        retval->setString(L"");
        return;
    }

    retval->setStringLen(p1);
    wchar_t* resp = retval->getString();
    wcsncpy(resp, p0, p1);
    *(resp+p1) = 0;
    retval->setStringLen(wcslen(resp));
}


void func_right(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wchar_t* p0 = env->m_eval_params[0]->getString();
    int p0_len = env->m_eval_params[0]->getDataLen();
    int p1 = env->m_eval_params[1]->getInteger();

    int off = p0_len-p1;
    if (off < 0)
    {
        off = 0;
    }

    if (p1 < 0 || p1 > tango::max_character_width)
    {
        retval->setString(L"");
        return;
    }

    retval->setStringLen(p0_len-off);
    wcscpy(retval->getString(), p0+off);

    if (p1 < p0_len)
    {
        *(retval->getString()+p1) = 0;
        retval->setStringLen(p1);
    }
}


void func_contains(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setBoolean(wcsstr(env->m_eval_params[0]->getString(),
                              env->m_eval_params[1]->getString()) ? true : false);
}


void func_lfind(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* p0 = env->m_eval_params[0]->getString();
    const wchar_t* p1 = env->m_eval_params[1]->getString();
    const wchar_t* p;
    int occurance = 1;
    int p1_len = wcslen(p1);

    if (env->m_param_count == 3)
    {
        occurance = env->m_eval_params[2]->getInteger();
        if (occurance <= 0 || occurance > tango::max_character_width)
        {
            // invalid occurance value specified
            retval->setInteger(0);
            return;
        }
    }

    const wchar_t* start = p0;

    while (1)
    {
        p = wcsstr(start, p1);
        if (!p)
        {
            // could not find string
            retval->setInteger(0);
            return;
        }

        if (--occurance == 0)
        {
            retval->setInteger((p-p0)+1);
            return;
        }

        start = p+p1_len;
    }
}

void func_rfind(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* p0 = env->m_eval_params[0]->getString();
    const wchar_t* p1 = env->m_eval_params[1]->getString();
    const wchar_t* p;
    int occurance = 1;
    int p1_len = wcslen(p1);

    if (env->m_param_count == 3)
    {
        occurance = env->m_eval_params[2]->getInteger();
        if (occurance <= 0 || occurance > tango::max_character_width)
        {
            // invalid occurance value specified
            retval->setInteger(0);
            return;
        }
    }

    p = p0+wcslen(p0)-p1_len;

    // check if search string is longer than input string
    if (p < p0)
    {
        retval->setInteger(0);
        return;
    }

    while (1)
    {
        if (wcsncmp(p, p1, p1_len))
        {
            if (p == p0)
            {
                // could not find string
                retval->setInteger(0);
                return;
            }

            p--;
            continue;
        }

        if (--occurance == 0)
        {
            retval->setInteger((p-p0)+1);
            return;
        }

        p--;

        if (p < p0)
        {
            // could not find string
            retval->setInteger(0);
            return;
        }
    }
}   



void func_repeat(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* p0 = env->m_eval_params[0]->getString();
    int p1 = env->m_eval_params[1]->getInteger();
    int p0_len = env->m_eval_params[0]->getDataLen();

    if (p0_len == 0 || p1 <= 0 || p1 > tango::max_character_width)
    {
        retval->setString(L"");
        return;
    }

    retval->setStringLen(p1);
    wchar_t* outstr = retval->getString();
    
    for (int i = 0; i < p1; ++i)
    {
        *outstr = *(p0 + (i % p0_len));
        outstr++;
    }

    *outstr = 0;
}


void func_translate(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* p0 = env->m_eval_params[0]->getString();
    const wchar_t* p1 = env->m_eval_params[1]->getString();
    const wchar_t* p2 = env->m_eval_params[2]->getString();

    int p0_len = env->m_eval_params[0]->getDataLen();
    int p2_len = env->m_eval_params[2]->getDataLen();

    retval->setStringLen(p0_len);
    wchar_t* outstr = retval->getString();

    const wchar_t* inp = p0;
    wchar_t* outp = outstr;

    const wchar_t* cmpc;

    while (*inp)
    {
        cmpc = wcschr(p1, *inp);
        if (cmpc)
        {
            int off = cmpc-p1;
            if (off < p2_len)
            {
                *outp = *(p2+off);
                outp++;
            }
        }
         else
        {
            *outp = *inp;
            outp++;
        }

        inp++;
    }

    *outp = 0;
}


void func_replace(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* p0 = env->m_eval_params[0]->getString();
    const wchar_t* p1 = env->m_eval_params[1]->getString();
    const wchar_t* p2 = env->m_eval_params[2]->getString();

    int p0_len = env->m_eval_params[0]->getDataLen();
    int p1_len = env->m_eval_params[1]->getDataLen();
    int p2_len = env->m_eval_params[2]->getDataLen();

    const wchar_t* inp;
    wchar_t* outp;


    // if the search string length is greater than the input
    // string length, there is definitely no match, so just
    // return the input string
    if (p1_len > p0_len || p1_len == 0)
    {
        retval->setStringLen(p0_len);
        memcpy(retval->getString(), p0, (p0_len+1) * sizeof(wchar_t));
        return;
    }

    // alloc memory for the resulting string

    if (p2_len == p1_len)
    {
        // if the length of our replace string is equal to
        // than the length of the search string, then the length
        // will remain the same

        retval->setStringLen(p0_len);
    }
     else
    {
        // calculate the length of the output string
        int out_len = 0;

        inp = p0;
        while (*inp)
        {
            if (0 == wcsncmp(inp, p1, p1_len))
            {
                out_len += p2_len;
                inp += p1_len;
                continue;
            }

            out_len++;
            inp++;
        }

        retval->setStringLen(out_len);
    }

    retval->setType(kscript::Value::typeString);
    outp = retval->getString();

    inp = p0;
    while (*inp)
    {
        if (p1_len > 0 && !wcsncmp(inp, p1, p1_len))
        {
            if (p2_len > 0)
            {
                memcpy(outp, p2, p2_len * sizeof(wchar_t));
            }

            outp += p2_len;
            inp += p1_len;
        }
         else
        {
            *outp = *inp;
            outp++;
            inp++;
        }
    }

    *outp = 0;
}






void func_initcap(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* p0 = env->m_eval_params[0]->getString();
    int p0_len = env->m_eval_params[0]->getDataLen();

    retval->setStringLen(p0_len);
    wchar_t* outstr = retval->getString();

    bool make_upper = true;
    while (*p0)
    {
        if (make_upper)
        {
            *outstr = towupper(*p0);
            make_upper = false;
        }
         else
        {
            *outstr = towlower(*p0);
        }

        if (!iswalpha(*p0))
        {
            make_upper = true;
        }

        p0++;
        outstr++;
    }

    *outstr = 0;
}

void func_soundex(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    //                              ABCDEFGHIJKLMNOPQRSTUVWXYZ
    static const wchar_t *table = L"01230120022455012623010202";

    const wchar_t* p0 = env->m_eval_params[0]->getString();

    int max_len = 5;
    if (env->m_param_count > 1)
        max_len = env->m_eval_params[1]->getInteger();
        
    retval->setStringLen(max_len);
    wchar_t* start = retval->getString();
    wchar_t* outp = start;
    const wchar_t* inp = p0;
        
    while (*inp && !iswalpha(*inp))
            ++inp;

    if (!*inp)
    {
        *outp = 0;
        return;
    }

    *outp++ = towupper(*inp++);

    int count = 0;

    max_len--;
    while (*inp && count < max_len)
    {
        if (iswalpha(*inp) && *inp != *(inp-1))
        {
            int c = (int)towupper(*inp);
            c -= (int)'A';
            
            if (c >= 0 && c < 26)
            {
                *outp = table[c];

                if (*outp != '0')
                {
                    ++outp;
                    ++count;
                }
            }
        }

        ++inp;
    }

    *outp = 0;
    retval->setStringLen(outp - start);
}



void func_week(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::ExprDateTime dt;
    env->m_eval_params[0]->getDateTime(&dt);

    if (dt.date == 0)
    {
        retval->setInteger(0);
        return;
    }

    int year, month, day;
    julianToDate(dt.date, &year, &month, &day);

    int first_day_julian = dateToJulian(year, 1, 1);

    retval->setInteger(((dt.date - first_day_julian) / 7)+1);
}



void func_binhex(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    unsigned char* data = env->m_eval_params[0]->getBinary();
    int max_len = env->m_eval_params[0]->getDataLen();


    retval->setStringLen((max_len*3)-1);
    wchar_t* outstr = retval->getString();
    
    *outstr = 0;

    wchar_t buf[255];
    for (int i = 0; i < max_len; ++i)
    {
        swprintf(buf, 255, L"%02x%ls", *data, i+1 < max_len ? L" " : L"");
        data++;
        wcscat(outstr, buf);
    }
}




void func_crc64(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::Value* p0 = env->m_eval_params[0];

    retval->setStringLen(16);
    wchar_t* outstr = retval->getString();

    tango::tango_uint64_t result = 0;

    switch (p0->getType())
    {
        case kscript::Value::typeString:
        {
            wchar_t* s = p0->getString();
            crc64((unsigned char*)s, wcslen(s)*sizeof(wchar_t), &result);
        }
        break;

        case kscript::Value::typeBinary:
        {
            crc64(p0->getBinary(), p0->getDataLen(), &result);
            break;
        }
    }

    unsigned int low = (unsigned int)(result & 0xffffffff);
    unsigned int high = (unsigned int)((result >> 32) & 0xffffffff);
    swprintf(outstr, 20, L"%08x%08x", high, low);
}


void func_regexp_m(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
#ifdef REGEXP_MADDOCK
    static std::map<std::wstring, jm::wregex> xmap;

    const wchar_t* str = env->m_eval_params[0]->getString();
    const wchar_t* expr = env->m_eval_params[1]->getString();
    const wchar_t* rewrite = NULL;
    int rparam = 0;
    
    if (env->m_param_count > 2)
    {
        if (env->m_eval_params[2]->getType() == kscript::Value::typeString)
            rewrite = env->m_eval_params[2]->getString();
             else
            rparam = env->m_eval_params[2]->getInteger();
    }

    int expr_len = wcslen(expr);
    int str_len = wcslen(str);
    int rewrite_len = 0;

    if (rewrite)
    {
        rewrite_len = wcslen(rewrite);
    }

    if (expr_len == 0)
    {
        retval->setString(L"");
        return;
    }

    try
    {
        jm::wregex& regex = xmap[expr];
        jm::wcmatch match;

        if (regex.error_code() == REG_BADPAT)
        {
            retval->setString(L"");
            return;
        }

        if (regex.error_code() == REG_EMPTY)
        {
            if (REG_NOERROR != regex.set_expression(expr))
            {
                retval->setString(L"");
                return;
            }
        }

        if (jm::reg_search(str,
                           str + str_len,
                           match,
                           regex))
        {
            if (rewrite)
            {
                std::wstring result;
                result = jm::reg_format(match, rewrite);
                retval->setString(result.c_str(), result.length());
                return;
            }
             else
            {
                if (rparam < (int)match.size())
                {
                    const jm::sub_match<wchar_t const*>& m = match[rparam];

                    if (m.matched)
                    {
                        retval->setString(m.first, m.length());
                        return;
                    }
                }
            }
        }
    }
    catch(...)
    {
        xmap[expr].fail(REG_BADPAT);
        retval->setString(L"");
        return;
    }
#endif
    retval->setString(L"");
}






class RegexMap
{
public:

    bool search(const wchar_t* expr,
                const wchar_t* str,
                klregex::wmatch& matchres)
    {
        XCM_AUTO_LOCK(m_map_mutex);

        klregex::wregex& regex = m_map[expr];

        if (regex.empty())
        {
            if (!regex.assign(expr))
                return false;
        }
  
        return regex.search(str, str + wcslen(str), matchres);
    }

    int replace(const wchar_t* expr,
                std::wstring& str,
                const std::wstring& fmt)
    {
        XCM_AUTO_LOCK(m_map_mutex);

        klregex::wregex& regex = m_map[expr];

        if (regex.empty())
        {
            if (!regex.assign(expr))
                return false;
        }
  
        return regex.replace(str, fmt, true);
    }

    bool match(const wchar_t* expr,
               const wchar_t* str)
    {
        XCM_AUTO_LOCK(m_map_mutex);

        klregex::wregex& regex = m_map[expr];

        if (regex.empty())
        {
            if (!regex.assign(expr))
                return false;
        }
  
        return regex.match(str, str + wcslen(str));
    }

private:

    std::map<std::wstring, klregex::wregex> m_map;
    xcm::mutex m_map_mutex;
};

RegexMap g_regex_map;


void func_regexp(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* str = env->m_eval_params[0]->getString();
    const wchar_t* expr = env->m_eval_params[1]->getString();
    const wchar_t* rewrite = NULL;
    int rparam = 0;
    
    if (env->m_param_count > 2)
    {
        if (env->m_eval_params[2]->getType() == kscript::Value::typeString)
            rewrite = env->m_eval_params[2]->getString();
             else
            rparam = env->m_eval_params[2]->getInteger();
    }


    klregex::wmatch match;
    if (g_regex_map.search(expr, str, match))
    {
        if (rewrite)
        {
            std::wstring result;
            result = match.format(rewrite);
            retval->setString(result.c_str(), result.length());
            return;
        }
         else
        {
            if (rparam < (int)match.size())
            {
                const klregex::sub_match<wchar_t>& m = match[rparam];

                if (m.isValid())
                {
                    retval->setString(m.first, m.length());
                    return;
                }
            }
        }
    }

    retval->setString(L"");
}


void func_regexp_replace(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    std::wstring str = env->m_eval_params[0]->getString();
    const wchar_t* expr = env->m_eval_params[1]->getString();
    std::wstring replace = env->m_eval_params[2]->getString();

    g_regex_map.replace(expr, str, replace);

    retval->setString(str.c_str(),str.length());
}


void func_regexp_match(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    const wchar_t* str = env->m_eval_params[0]->getString();
    const wchar_t* expr = env->m_eval_params[1]->getString();

    klregex::wmatch match;
    if (g_regex_map.match(expr, str))
    {
        retval->setBoolean(true);
        return;
    }

    retval->setBoolean(false);
}

// gets a host name from an ip address

class ExprAddrCache
{
public:

    std::wstring getCachedHostName(const std::wstring& ip)
    {
        XCM_AUTO_LOCK(m_map_mutex);

        #ifdef WIN32
            static bool first_time = true;
            if (first_time)
            {
                WSADATA wsaData;
                if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
                {
                    return L"";
                }

                first_time = false;
            }
        #endif

        std::string addr = kl::tostring(ip);
        unsigned long l = inet_addr(addr.c_str());

        std::map<unsigned long, std::wstring>::iterator it;
        it = m_map.find(l);
        if (it != m_map.end())
        {
            return it->second;
        }

        struct hostent* h = gethostbyaddr((char*)&l, 4, AF_INET);
        if (!h)
        {
            m_map[l] = L"";
            return L"";
        }

        m_map[l] = kl::towstring(h->h_name);
        return m_map[l];
    }

private:
    xcm::mutex m_map_mutex;
    std::map<unsigned long, std::wstring> m_map;
};

ExprAddrCache m_expr_addr_cache;


void func_hostname(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::Value* p0 = env->m_eval_params[0];

    std::wstring waddr = p0->getString();

    retval->setString(m_expr_addr_cache.getCachedHostName(waddr).c_str());
}



void func_md5(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    kscript::Value* p0 = env->m_eval_params[0];
    std::wstring str = p0->getString();
    retval->setString(kl::md5str(str).c_str());
}





void func_json_extract(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 2)
        return;

    std::wstring code = L"(";
    code += env->getParam(0)->getString();
    code += L")";

    // append code for desired element
    const wchar_t* d = env->getParam(1)->getString();
    if (*d == '.' || *d == '[')
    {
        code += d;
    }
     else
    {
        code += L".";
        code += d;
    }
     

    kscript::ExprParser parser(kscript::optionLanguageECMAScript);
    if (!parser.parse(code))
        return;
    
    parser.eval(retval);
    if (retval->getType() != kscript::Value::typeString && !retval->isNull())
    {
        kscript::Value v;
        retval->toString(&v);
        v.eval(NULL, retval);
    }
}

void func_strdistance(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    retval->setNull();
    if (env->getParamCount() < 2)
        return;

    retval->setInteger(levenshtein(env->getParam(0)->getString(), env->getParam(1)->getString()));
}

