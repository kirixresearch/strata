/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  SD Connector
 * Author:   Benjamin I. Williams
 * Created:  2013-11-27
 *
 */

#include <wx/wx.h>
#include "app.h"
#include "mainframe.h"

IMPLEMENT_APP(SdconnApp)

SdconnApp* g_app;


bool SdconnApp::OnInit()
{
    m_config.init(L"Sdserv", L"Sdserv");

    g_app = this;

    wxImage::AddHandler(new wxPNGHandler);

    MainFrame* frame = new MainFrame(_T("SD Connector"), wxPoint(50,50), wxSize(420,500));
    frame->Center();
    frame->Show(true);

    SetTopWindow(frame);
    return TRUE;
}

wxBitmap lookupBitmap(const wxString& name); // in bitmaps.cpp
wxBitmap SdconnApp::getBitmap(const wxString& name)
{
    return lookupBitmap(name);
}
