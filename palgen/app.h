/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Code Generator Client
 * Author:   Benjamin I. Williams
 * Created:  2003-04-27
 *
 */


#ifndef H_PALGEN_APP_H
#define H_PALGEN_APP_H


#include <wx/wx.h>


#if wxUSE_GUI!=0
class PalGenApp : public wxApp
#else
class PalGenApp : public wxAppConsole
#endif
{
private:
    bool OnInit();
    int OnExit();

#if wxUSE_GUI==0
    int OnRun();
#endif
};


DECLARE_APP(PalGenApp);


#endif

