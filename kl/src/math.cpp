/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-06-15
 *
 */


#include "kl/math.h"


namespace kl
{


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
