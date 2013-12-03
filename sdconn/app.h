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
#include <xd/xd.h>


#ifdef WIN32
#define APP_COMPANY_KEY     "Sdconn"
#define APP_CONFIG_KEY      "Sdconn"
#else
#define APP_COMPANY_KEY     "sdconn"
#define APP_CONFIG_KEY      "sdconn"
#endif

#define DEFAULT_SERVER      ""
#define PASSWORD_KEY        "4$9akJlQ"

#define GETBMP(bmp) g_app->getBitmap(#bmp)

#ifdef WIN32
#define PATH_SEPARATOR_CHAR    wxT('\\')
#define PATH_SEPARATOR_STR     wxT("\\")
#else
#define PATH_SEPARATOR_CHAR    wxT('/')
#define PATH_SEPARATOR_STR     wxT("/")
#endif

inline const wxStdWideString& towstr(const wxString& str) { return str.ToStdWstring(); }

class SdconnApp : public wxApp
{
public:
    SdconnApp() : wxApp() {}
    
    wxBitmap getBitmap(const wxString& image_name);

    kl::Config& getConfig() { return m_config; }
    xd::IDatabasePtr& getDatabase() { return m_database; }
    Sdserv& getSdserv() { return m_sdserv; }
    wxString getAppDataPath();

    bool startServer();

public:
    virtual bool OnInit();

private:

    kl::Config m_config;
    xd::IDatabasePtr m_database;
    std::wstring m_database_connection_str;
    Sdserv m_sdserv;
};

extern SdconnApp* g_app;

