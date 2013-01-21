/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#ifndef __KXINTER_UTIL_H
#define __KXINTER_UTIL_H


// -- platform definitions --


#ifdef WIN32
#define PATH_SEPARATOR_CHAR    wxT('\\')
#define PATH_SEPARATOR_STR     wxT("\\")
#else
#define PATH_SEPARATOR_CHAR    wxT('/')
#define PATH_SEPARATOR_STR     wxT("/")
#endif




inline int dateToJulian(int year, int month, int day)
{
    int a, y, m;

    a = (14-month)/12;
    y = year + 4800 - a;
    m = month + (12 * a) - 3;
    return (day + (((153*m)+2)/5) + (365*y) + (y/4) - (y/100) + (y/400) - 32045);
}


inline void julianToDate(int julian, int* year, int* month, int* day)
{
    int a, b, c, d, e, f;

    a = julian + 32044;
    b = ((4*a)+3)/146097;
    c = a - ((146097*b) / 4);
    d = ((4*c)+3)/1461;
    e = c - ((1461*d))/4;
    f = ((5*e)+2)/153;

    *day = e - (((153*f)+2)/5) + 1;
    *month = f + 3 - (12 * (f/10));
    *year = (100*b) + d - 4800 + (f/10);
}



inline void resizeStaticText(wxStaticText* text, int width = -1)
{
    wxWindow* parent = text->GetParent();

    int intended_width;

    if (width == -1)
    {
        wxSize s = parent->GetClientSize();
        intended_width = s.GetWidth() - 10;
    }
     else
    {
        intended_width = width;
    }

    // -- by default --
    int text_width, text_height;
    text->GetSize(&text_width, &text_height);

    int row_count = (text_width/intended_width) + 1;
    
    text->SetSize(intended_width, row_count*text_height);
}




#endif

