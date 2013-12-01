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
#include <kl/thread.h>

IMPLEMENT_APP(SdconnApp)

SdconnApp* g_app;


bool SdconnApp::OnInit()
{
    m_config.init(L"Sdserv", L"Sdserv");

    g_app = this;

    wxImage::AddHandler(new wxPNGHandler);


    startServer();

    MainFrame* frame = new MainFrame(_T("SD Connector"), wxPoint(50,50), wxSize(520,560));
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








class ServerThread : public kl::Thread
{
public:

    ServerThread() : kl::Thread()
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
    Sdserv& sdserv = g_app->getSdserv();

    // default settings
    sdserv.setOption(L"sdserv.server_type", L"websockets");
    sdserv.setOption(L"websockets.ssl", L"true");
    sdserv.setOption(L"websockets.server", L"dataex.goldprairie.com");
    sdserv.setOption(L"sdserv.database", L"default");
    sdserv.setOption(L"database.default", L"xdprovider=xdnative;database=C:\\Users\\bwilliams\\Documents\\Gold Prairie Projects\\Default Project;user id=admin;Password=");
    sdserv.setOption(L"login.username", L"test@test.com");
    sdserv.setOption(L"login.password", L"test99");

    // start the job in a thread
    ServerThread* server_thread = new ServerThread();

    if (server_thread->create() != 0)
        return false;

    return true;
}


