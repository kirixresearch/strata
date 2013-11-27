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
#include <vector>
#include <map>


class SdconnApp : public wxApp
{
public:
    SdconnApp() : wxApp() {}
    
    wxBitmap getBitmap(const wxString& image_name);

public:
    virtual bool OnInit();

private:


};

extern SdconnApp* g_app;
