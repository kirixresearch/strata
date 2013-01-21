/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#include "kxinter.h"
#include "app.h"
#include "datadoc.h"
#include "xpmregister.h"
#include "datadoc.h"
#include "appcontroller.h"


XCM_USING_STATIC_MODULE(cfw)


KxInterApp* g_app = NULL;
paladin::Authentication* g_auth = NULL;


// -- app instantiation --

IMPLEMENT_APP(KxInterApp)





static wxString getProgramPath()
{
    char buf[512];
    char* slash;

#if defined(WIN32)
    GetModuleFileNameA(NULL, buf, 511);
    slash = strrchr(buf, '\\');
    if (slash)
    {
        *slash = 0;
    }
#elif defined(__LINUX__)
    int res;
    res = readlink("/proc/self/exe", buf, 512);
    if (res == -1)
        return wxT("");
    buf[res] = 0;
    slash = strrchr(buf, '/');
    if (slash)
    {
        *slash = 0;
    }
#else
    return wxT("");
#endif

#ifdef _UNICODE
    return wxString::From8BitData(buf);
#else
    return buf;
#endif
}


#ifdef WIN32

static BOOL CALLBACK EnumAllWindowsProc(HWND hwnd, LPARAM lParam)
{
    wxString s = PRODUCT_NAME;
    char title[512];
    strcpy(title, s.mbc_str());


    char buf[512];
    GetWindowTextA(hwnd, buf, 511);
    if (!memcmp(buf, title, strlen(title)) && strstr(buf, " Help") == 0)
    {
        GetClassNameA(hwnd, buf, 511);

        if (!memcmp(buf, "wx", 2))
        {
            HWND* h = (HWND*)lParam;
            *h = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}


HWND FindKxInterWindow()
{
    HWND h = 0;
    EnumWindows(EnumAllWindowsProc, (LPARAM)&h);
    return h;
}

#endif


KxInterApp::KxInterApp() : wxApp()
{
    m_app_controller = NULL;
}


bool KxInterApp::OnInit()
{
    // -- single instance checker: detect currently running kpp applications,
    //    and if we find one, terminate this process and show the other kpp's
    //    window --

    #ifdef WIN32
    HWND kxinter_hwnd = FindKxInterWindow();
    if (kxinter_hwnd)
    {
        SetForegroundWindow(kxinter_hwnd);
        return false;
    }
    #endif


    g_app = this;

    g_auth = paladin::createAuthObject("KirixResearch", PALADIN_APP_TAG, PALADIN_TRIAL_TAG);

    xcm::path_list::add(towstr(getProgramPath()));


    // -- make sure an application data directory exists --

    #ifdef WIN32
        m_appdata_path = _tgetenv(_T("APPDATA"));
        if (m_appdata_path.IsEmpty())
        {
            m_appdata_path = wxT("C:\\");
            m_appdata_path += PRODUCT_NAME;
            m_appdata_path += wxT("\\");
        }
         else
        {
            if (m_appdata_path.Last() != wxT('\\'))
            {
                m_appdata_path += wxT("\\");
            }

            m_appdata_path += wxT("KirixResearch");

            if (!xf_get_directory_exist(towstr(m_appdata_path)))
            {
                xf_mkdir(towstr(m_appdata_path));
            }

            m_appdata_path += wxT("\\");
            m_appdata_path += PRODUCT_NAME;

            if (!xf_get_directory_exist(towstr(m_appdata_path)))
            {
                xf_mkdir(towstr(m_appdata_path));
            }
        }
    #else
        m_appdata_path = wxString::From8BitData(getenv("HOME"));
        if (m_appdata_path.IsEmpty())
        {
            m_appdata_path = wxT("/");
        }

        if (m_appdata_path.Last() != wxT("/"))
        {
            m_appdata_path += wxT("/");
        }

        m_appdata_path += wxT(".interchange");

        if (!xf_get_directory_exist(towstr(m_appdata_path)))
        {
            xf_mkdir(towstr(m_appdata_path));
        }
    #endif


    // -- create a job queue --
    m_job_queue.create_instance("cfw.JobQueue");

    // -- create an app preferences object --
    m_app_preferences.create_instance("cfw.AppPreferences");
    m_app_preferences->init(wxT("KirixResearch"), wxT("Interchange"));

    // -- create our connection manager --
    m_connection_mgr = new ConnectionMgr;

    // -- create an instance of XbaseDatabase --
    m_xbase_db.create_instance("xdxbase.Database");

    // -- create an instance of TextDelimitedDatabase --
    m_textdelimited_db.create_instance("xdtextdelimited.Database");

    // -- initialize bitmap list --
    registerPixmaps();

    // -- create and initialize appcontroller --
    m_app_controller = new AppController;
    if (!m_app_controller->init())
    {
        delete m_app_controller;
        m_app_controller = NULL;
        return false;
    }

    return true;
}


int KxInterApp::OnExit()
{
    if (m_app_controller)
    {
        delete m_app_controller;
        m_app_controller = NULL;
    }

    return wxApp::OnExit();
}


void KxInterApp::setMainFrame(cfw::IFramePtr frame)
{
    m_frame = frame;

    if (m_frame)
    {
        m_frame_wnd = m_frame->getFrameWindow();
    }
     else
    {
        m_frame_wnd = NULL;
    }
}


cfw::IFramePtr KxInterApp::getMainFrame()
{
    return m_frame;
}


wxFrame* KxInterApp::getMainWindow()
{
    return m_frame_wnd;
}

AppController* KxInterApp::getAppController()
{
    return m_app_controller;
}


cfw::IAppPreferencesPtr KxInterApp::getAppPreferences()
{
    return m_app_preferences;
}


TreeController* KxInterApp::getTreeController()
{
    return m_app_controller->getTreeController();
}


tango::IDatabasePtr KxInterApp::getXbaseDatabase()
{
    return m_xbase_db;
}


tango::IDatabasePtr KxInterApp::getDelimitedTextDatabase()
{
    return m_textdelimited_db;
}


cfw::IJobQueuePtr KxInterApp::getJobQueue()
{
    return m_job_queue;
}


ConnectionMgr* KxInterApp::getConnectionMgr()
{
    return m_connection_mgr;
}


wxString KxInterApp::getAppDataPath()
{
    return m_appdata_path;
}

