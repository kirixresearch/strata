/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2002-12-18
 *
 */


#ifndef __KL_MATH_H
#define __KL_MATH_H

#include <string>
#include <cmath>


namespace kl
{


inline double pow10(int r)
{
    static double d[21] = { 1.0e0,  1.0e1,  1.0e2,  1.0e3,  1.0e4,  1.0e5,  1.0e6,  1.0e7,
                            1.0e8,  1.0e9,  1.0e10, 1.0e11, 1.0e12, 1.0e13, 1.0e14, 1.0e15,
                            1.0e16, 1.0e17, 1.0e18, 1.0e19, 1.0e20 };

    if (r < 0 || r > 20)
        return pow((double)10.0, r);
    
    return d[r];
}


// double helpers

inline double dblround(double d, int dec)
{
    double p = kl::pow10(dec);
    d = d * p;
    d = d + 0.5;
    d = floor(d);
    d = d / p;
    return d;
}


const double def_epsilon = 0.000000000001;  // 1e-12
inline int dblcompare(double x1, double x2)
{
    if (x1 == x2)
        return 0;
        
    int exponent;
    double delta;
    double difference;

    (void)frexp(fabs(x1) > fabs(x2) ? x1 : x2, &exponent);

    delta = ldexp(def_epsilon, exponent);

    difference = x1 - x2;

    if (difference > delta)
        return 1;  // x1 > x2
    else if (difference < -delta) 
        return -1; // x1 < x2
    else
        return 0;  // x1 == x2
}

std::wstring dbltostr(double d);


// statistics class for calculating mean, variance, and 
// standard deviation

class statistics
{
public:

    statistics();
    virtual ~statistics();

    unsigned int count();
    double mean();
    double variance();
    double stddev();

    void push(double x);
    void init();

private:

    unsigned int m_count;
    double m_new_m;
    double m_new_s;
    double m_old_m;
    double m_old_s;
};


};


#endif

