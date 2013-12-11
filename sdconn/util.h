/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2013-12-11
 *
 */


#ifndef __SDCONN_UTIL_H
#define __SDCONN_UTIL_H



wxString makeValidObjectName(const wxString& str, xd::IDatabasePtr db);
void resizeStaticText(wxStaticText* text, int width = -1);
wxSize getMaxTextSize(wxStaticText* st0,
                      wxStaticText* st1,
                      wxStaticText* st2 = NULL,
                      wxStaticText* st3 = NULL,
                      wxStaticText* st4 = NULL,
                      wxStaticText* st5 = NULL,
                      wxStaticText* st6 = NULL);


#endif

