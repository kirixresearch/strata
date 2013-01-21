/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-06-15
 *
 */


#include "kl/math.h"
#include "kl/string.h"


namespace kl
{


std::wstring dbltostr(double d)
{
    if (dblcompare(d, 0.0) == 0)
    {
        return L"0";
    }
    
    bool exponent_notation = false;
    if (fabs(d) >= 10e21)
        exponent_notation = true;
    
    wchar_t str[255];

    for (int i = 0; i < 15; ++i)
    {
        if (exponent_notation)
        {
            swprintf(str, 255, L"%.*e", i, d);
            wchar_t* e = wcschr(str, 'e');
            if (e)
            {
                wchar_t* d = (e+2);
                wchar_t* p = d;
                while (*p == '0')
                    ++p;
                if (*p)
                {
                    while (*p)
                    {
                        *d = *p;
                        ++p;
                        ++d;
                    }
                    *d = 0;
                }
                 else
                {
                    *d = '0';
                    *(d+1) = 0;
                }
            }
        }
         else
        {
            swprintf(str, 255, L"%.*f", i, d);
        }

        if (dblcompare(kl::wtof(str), d) == 0)
            return str;
    }
    
    swprintf(str, 255, L"%.15f", d);
    return str;
}













// Statistics class for calculating mean, variance, and 
// standard deviation
Statistics::Statistics()
{
    init();
}

Statistics::~Statistics()
{
}

unsigned int Statistics::count()
{
    return m_count;
}

double Statistics::mean()
{
    if (m_count < 1)
        return 0.0;

    return m_new_m;
}

double Statistics::variance()
{
    if (m_count < 2)
        return 0.0;

    return m_new_s/(m_count - 1);
}

double Statistics::stddev()
{
    return sqrt(variance());
}

void Statistics::push(double x)
{
    // note: statistics are calculated using Welford's method;
    // more information in Donald Knuth's Art of Computer Programming,
    // Volume 2, 3rd edition

    m_count++;

    if (m_count == 1)
    {
        m_new_m = x;
        m_new_s = 0.0;
    }
    else
    {
        m_new_m = m_old_m + (x - m_old_m)/m_count;
        m_new_s = m_old_s + (x - m_old_m)*(x - m_new_m);
    }
    
    m_old_m = m_new_m;
    m_old_s = m_new_s;
}

void Statistics::init()
{
    m_count = 0;
    m_new_m = 0.0;
    m_new_s = 0.0;
    m_old_m = 0.0;
    m_old_s = 0.0;
}


};
