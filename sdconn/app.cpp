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
#include <wx/stdpaths.h>
#include "app.h"
#include "mainframe.h"
#include <kl/thread.h>
#include <kl/file.h>
#include <kl/crypt.h>


IMPLEMENT_APP(SdconnApp)

SdconnApp* g_app;


bool SdconnApp::OnInit()
{
    g_app = this;

    // init config
    m_config.init(kl::towstring(APP_COMPANY_KEY), kl::towstring(APP_CONFIG_KEY));

    // set database path
    std::wstring data_folder = getAppDataPath();
    data_folder += PATH_SEPARATOR_STR;
    data_folder += L"data";

    if (!xf_get_directory_exist(data_folder))
        xf_mkdir(data_folder);

    m_database_connection_str = L"Xdprovider=xdfs;Database=" + data_folder;


    // init database

    xcm::ptr<xd::IDatabaseMgr> dbmgr = xd::getDatabaseMgr();

    if (dbmgr.isNull())
        return false;

    m_database = dbmgr->open(m_database_connection_str);

    wxImage::AddHandler(new wxPNGHandler);


    startServer();

    MainFrame* frame = new MainFrame(_T("SD Connector"), wxPoint(50,50), wxSize(550,580));
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




wxString SdconnApp::getAppDataPath()
{
    // on windows by default, this directory is stored in the user's 
    // Application Data directory.  On English language systems, this 
    // is usually C:\Document and Settings\<user>\Application Data
    // The app's data path is, by default, put in the <Company>
    // subdirectory inside this folder.

    wxString retval;
    
    wxStandardPaths& sp = wxStandardPaths::Get();
    wxString default_appdata_path = sp.GetUserConfigDir();
    
    if (default_appdata_path.Right(1) != PATH_SEPARATOR_STR)
        default_appdata_path += PATH_SEPARATOR_CHAR;
    default_appdata_path += APP_COMPANY_KEY;
    
    // if the <AppData>/<Company> subdir doesn't exist, create it
    if (!xf_get_directory_exist(towstr(default_appdata_path)))
        xf_mkdir(towstr(default_appdata_path));
    
    return default_appdata_path;
}




class ServerThread : public kl::thread
{
public:

    ServerThread() : kl::thread()
    {

    }

    ~ServerThread()
    {
    }

    unsigned int entry()
    {
        #ifdef _MSC_VER
        static int counter = 0;
        counter += 100;
        // ensures that the rand() is truly random in this thread
        int seed = (int)time(NULL);
        seed += (int)clock();
        seed += counter;
        srand(seed);
        #endif
    

        Sdserv& sdserv = g_app->getSdserv();
        sdserv.runServer();

        return 0;
    }

    void exit()
    {
    }
};


bool SdconnApp::startServer()
{

    kl::config& config = g_app->getConfig();
    std::wstring user = config.read(L"Settings/User");
    std::wstring password = config.read(L"Settings/Password");
    std::wstring server = config.read(L"Settings/Server", kl::towstring(DEFAULT_SERVER));
    if (password.length() > 0)
    {
        password = kl::decryptString(password, kl::towstring(PASSWORD_KEY));
    }


    if (user.length() > 0 && server.length() > 0)
    {

        // default settings
        Sdserv& sdserv = g_app->getSdserv();
        sdserv.setOption(L"sdserv.server_type", L"websockets");
        sdserv.setOption(L"websockets.ssl", L"true");
        sdserv.setOption(L"websockets.server", "dataex.goldprairie.com");
        sdserv.setOption(L"sdserv.database", L"default");
        sdserv.setOption(L"database.default", m_database_connection_str);
        sdserv.setOption(L"login.username", user);
        sdserv.setOption(L"login.password", password);


        // start the job in a thread
        ServerThread* server_thread = new ServerThread();

        if (server_thread->create() != 0)
            return false;

        return true;
    }
     else
    {
        return false;
    }
}
