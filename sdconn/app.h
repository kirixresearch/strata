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
#include <kl/config.h>
#include "../sdservlib/sdservlib.h"

class SdconnApp : public wxApp
{
public:
    SdconnApp() : wxApp() {}
    
    wxBitmap getBitmap(const wxString& image_name);
    kl::Config& getConfig() { return m_config; }

public:
    virtual bool OnInit();

private:

    kl::Config m_config;
    Sdserv m_sdserv;
};

extern SdconnApp* g_app;

#define GETBMP(bmp) g_app->getBitmap(#bmp)
