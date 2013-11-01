/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2001-11-15
 *
 */


#include "appmain.h"
#include "app.h"
#include "apphook.h"
#include "appcontroller.h"
#include "dbdoc.h"
#include "bookmarkfs.h"
#include "dlgprojectmgr.h"
#include "jobscheduler.h"
#include "jsonconfig.h"
#include "../paladin/paladin.h"
#include "../webconnect/webcontrol.h"
#include "connectionmgr.h"
#include "extensionmgr.h"
#include "panelconsole.h"
#include "toolbars.h"
#include "scripthost.h"
#include <wx/fs_zip.h>
#include <wx/url.h>
#include <wx/stdpaths.h>
#include <wx/fontenum.h>
#include <wx/paper.h>
#include <wx/tooltip.h>
#include <wx/dir.h>
#include <map>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif


MainApp* g_app = NULL;
AppMacroRecorder g_macro;
paladin::Authentication* g_auth = NULL;
wxLocale g_locale;
time_t g_app_start_time = 0;



#include "appmain.h"


static wxString getAppDataPath()
{
    // try to get the application data path from the registry
    IAppPreferencesPtr prefs = g_app->getAppPreferences();

    // on windows by default, this directory is stored in the user's 
    // Application Data directory.  On English language systems, this 
    // is usually C:\Document and Settings\<user>\Application Data
    // The app's data path is, by default, put in the <Company>
    // subdirectory inside this folder.

    wxString retval;
    
    wxStandardPaths sp;
    wxString default_appdata_path = sp.GetUserConfigDir();
    
    if (default_appdata_path.Right(1) != PATH_SEPARATOR_STR)
        default_appdata_path += PATH_SEPARATOR_CHAR;
    default_appdata_path += APP_COMPANY_KEY;
    
    // if the <AppData>/<Company> subdir doesn't exist, create it
    if (!xf_get_directory_exist(towstr(default_appdata_path)))
        xf_mkdir(towstr(default_appdata_path));
    
    // if the <AppData>/<Company>/<appname> subdir doesn't exist, create it
    default_appdata_path += PATH_SEPARATOR_CHAR;
    default_appdata_path += APP_CONFIG_KEY;
    if (!xf_get_directory_exist(towstr(default_appdata_path)))
    {
        // migrate old standard path setting to new value
        prefs->remove(wxT("standard_paths.application_data"));
        xf_mkdir(towstr(default_appdata_path));
    }
    
    if (!prefs->exists(wxT("standard_paths.application_data")))
    {
        retval = default_appdata_path;
        prefs->setString(wxT("standard_paths.application_data"),
                         default_appdata_path);
        prefs->flush();
    }
     else
    {
        retval = prefs->getString(wxT("standard_paths.application_data"),
                                  default_appdata_path);
    }
    
    return retval;
}





#ifdef __WXMSW__

class AppTaskBarIcon : public wxTaskBarIcon
{
public:

    enum
    {
        ID_RestoreApp = 10000,
        ID_ExitApp,
    };

private:

    void onMenuRestore(wxCommandEvent& )
    {
        g_app->showApp(true);
    }

    void onMenuExit(wxCommandEvent& )
    {
        wxFrame* frame = g_app->getMainWindow();
        if (frame)
        {
            frame->Close();
        }
    }

    void onRButtonUp(wxTaskBarIconEvent&)
    {
        wxMenu menu;
        menu.Append(ID_RestoreApp, _("&Show Main Window"));
        menu.AppendSeparator();
        menu.Append(ID_ExitApp, _T("E&xit"));

        PopupMenu(&menu);
    }

    void onLButtonDClick(wxTaskBarIconEvent&)
    {
        g_app->showApp(true);
    }

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(AppTaskBarIcon, wxTaskBarIcon)
    EVT_MENU(AppTaskBarIcon::ID_RestoreApp, AppTaskBarIcon::onMenuRestore)
    EVT_MENU(AppTaskBarIcon::ID_ExitApp, AppTaskBarIcon::onMenuExit)
    EVT_TASKBAR_RIGHT_UP(AppTaskBarIcon::onRButtonUp)
    EVT_TASKBAR_LEFT_DCLICK(AppTaskBarIcon::onLButtonDClick)
END_EVENT_TABLE()

#endif




#ifdef __WXMSW__

static BOOL CALLBACK EnumAllWindowsProc(HWND hwnd, LPARAM lParam)
{
    wchar_t buf[512];
    GetWindowTextW(hwnd, buf, 511);
    buf[511] = 0;
    
    if (0 != wcsstr(buf, APPLICATION_NAME) && 0 == wcsstr(buf, L" Help"))
    {
        GetClassNameW(hwnd, buf, 511);

        if (wcslen(buf) > 2 && 0 == wcsncmp(buf, L"wx", 2))
        {
            HWND* h = (HWND*)lParam;
            *h = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}


HWND FindApplicationWindow()
{
    HWND h = 0;
    EnumWindows(EnumAllWindowsProc, (LPARAM)&h);
    return h;
}

#endif


class LicenseTimer : public wxTimer
{
public:

    void Notify()
    {
        int auth_result = g_auth->checkAuth();
        switch (auth_result)
        {
            case paladin::errNone:
                return;

            default:
            case paladin::errAuthFailed:
                this->Stop();
                appMessageBox(_("Your license for this application has expired.  Please renew your license."),
                              APPLICATION_NAME,
                              wxOK | wxICON_INFORMATION | wxCENTER);
                break;

            case paladin::errClockModified:
                this->Stop();
                appMessageBox(_("The clock was set back on your computer.  Please reset the clock to the normal time or re-license this software."),
                              APPLICATION_NAME,
                              wxOK | wxICON_INFORMATION | wxCENTER);
                break;
        }

        g_app->getMainFrame()->closeAll(true);
        g_app->getMainWindow()->Close(true);
    }
};

LicenseTimer* g_license_timer = NULL;





class UpdateTimer : public wxTimer
{
public:

    void Notify();
};

UpdateTimer* g_update_timer = NULL;


void UpdateTimer::Notify()
{
    AppController* controller = g_app->getAppController();
    if (!controller)
        return;
        
    // user does not want updates
    if (!getAppPrefsBoolean(wxT("general.updater.check_for_updates")))
        return;
        
    time_t last_update = (time_t)getAppPrefsLong(wxT("general.updater.last_check"));
    time_t now_time = time(NULL);
    
    struct tm last_tm, now_tm;
    localtime_r(&now_time, &now_tm);
    localtime_r(&last_update, &last_tm);
    
    if (now_tm.tm_year == last_tm.tm_year &&
        now_tm.tm_mon == last_tm.tm_mon &&
        now_tm.tm_mday == last_tm.tm_mday)
    {
        // we've already checked today -- slow down the timer
        // to once an hour
        g_update_timer->Stop();
        g_update_timer->Start(3600000);
        return;
    }
    
    // don't check until program has been running 5 minutes
    if (now_time - g_app_start_time < 300)
        return;
    
    // check for update (this function will update
    // the "general.updater.last_check" preference
    controller->checkForUpdates(false /* show_full_gui */);
}



// -- app instantiation --

#if APP_CONSOLE==0
    IMPLEMENT_APP(MainApp)
    BEGIN_EVENT_TABLE(MainApp, wxApp)
        EVT_ACTIVATE_APP(MainApp::onActivateApp)
    END_EVENT_TABLE()
#else
    IMPLEMENT_APP_CONSOLE(MainApp)
    BEGIN_EVENT_TABLE(MainApp, wxAppConsole)
    END_EVENT_TABLE()
#endif

MainApp::MainApp()
{
    m_frame = NULL;
    m_app_controller = NULL;
    m_frame_wnd = NULL;
    m_database = xcm::null;
    m_help_controller = NULL;
    m_extension_mgr = NULL;
    m_job_scheduler = NULL;
    m_web_server = NULL;
    m_paper_database = NULL;
    m_dbdoc = NULL;
    m_linkbar = NULL;
    m_command_line = NULL;
    m_is_service = false;
    m_is_service_config = false;
#ifdef __WXMSW__
    m_taskbar_icon = NULL;
#endif
}

void MainApp::processIdle()
{
#if APP_CONSOLE==0
    this->ProcessIdle();
#endif
}


bool MainApp::OnInit()
{
#ifdef _DEBUG
    wxLog::AddTraceMask(wxT("ole"));
#endif

    srand((unsigned)time(NULL));
    g_app = this;

    // get our install location
    wxStandardPaths sp;
    m_install_path = sp.GetExecutablePath().BeforeLast(PATH_SEPARATOR_CHAR);

    // initialize the paladin object (checks for license authenticity)
    g_auth = paladin::createAuthObject(kl::tostring(APP_COMPANY_KEY).c_str(),
                                       PALADIN_APP_TAG,
                                       PALADIN_APP_TAG APP_PALADIN_TEMP_LICENSE_COUNTER);
    
    if (argc > 1)
    {
        if (g_app->getCommandLine()->Found(wxT("s")))
        {
            m_is_service = true;
        }

        if (g_app->getCommandLine()->Found(wxT("svccfg")))
        {
            m_is_service_config = true;
        }
    }


#if APP_GUI==1
    // the below code is necessary for xdoracle to load properly
    // but it's not a good solution to change the directory just
    // to make this work.  We need to change the directory, load
    // xdoracle and then change it back, at the very least.  The
    // above APP_GUI #if was added to make the console mode work
    // on short notice without having to solve the problem below
    // Eventually, this should be fixed properly
    
#ifdef __WXGTK__
    // this should allow oracle instant client to find
    // its support drivers on linux
    chdir(tostr(m_install_path).c_str());
#endif

#endif

    // set application start time
    g_app_start_time = time(NULL);
    
    // initialize xcm
    xcm::path_list::add(towstr(m_install_path));

    // add current directory to xcm path list
    wxString cwd = ::wxGetCwd();
    xcm::path_list::add(towstr(cwd));
    
    
    // no logging
#ifndef _DEBUG
    wxLog::EnableLogging(false);
    suppressConsoleLogging();
#endif


    // initialize i18n (internationalization/language support)
    wxString i18n_base_path;
    i18n_base_path = m_install_path;
    i18n_base_path += PATH_SEPARATOR_STR;
    i18n_base_path += wxT("..");
    i18n_base_path += PATH_SEPARATOR_STR;
    i18n_base_path += wxT("i18n");
    if (!wxDir::Exists(i18n_base_path))
    {
        // try old location: <bin_dir>/i18n/de
        i18n_base_path = m_install_path;
        i18n_base_path += PATH_SEPARATOR_STR;
        i18n_base_path += wxT("i18n");
    }
    
    g_locale.Init(wxLANGUAGE_DEFAULT);
    g_locale.AddCatalogLookupPathPrefix(i18n_base_path);
    g_locale.AddCatalog(wxT("messages"));
    
    wxSocketBase::Initialize();

    #if APP_GUI==1

    // initialize help controller
    wxString help_path = m_install_path;
    #ifdef __WXMSW__
    help_path += wxT("\\..\\help\\help.chm");
    #else
    wxFileSystem::AddHandler(new wxZipFSHandler);
    help_path += wxT("/../help/help.zip");
    #endif

    if (xf_get_file_exist(towstr(help_path)))
    {
        #ifdef __WXMSW__
        m_help_controller = new wxCHMHelpController;
        #else
        m_help_controller = new wxHtmlHelpController;
        #endif
        
        m_help_controller->Initialize(help_path);
    }

    // allow access to various image formats
    wxImage::AddHandler(new wxPNGHandler);
    wxImage::AddHandler(new wxJPEGHandler);
    wxImage::AddHandler(new wxGIFHandler);
    wxImage::AddHandler(new wxXPMHandler);
    wxImage::AddHandler(new wxICOHandler);
     
    // create the paper database
    m_paper_database = new wxPrintPaperDatabase;
    populatePaperDatabaseClean();

    // populate the font names array
    wxFontEnumerator fonts;
    fonts.EnumerateFacenames();
    m_font_names = fonts.GetFacenames();

    // try to find the default gui font facename in the list
    bool found = false;
    wxFont default_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    wxString default_facename = default_font.GetFaceName();
    int i, count = m_font_names.GetCount();
    for (i = 0; i < count; ++i)
    {
        if (default_facename.CmpNoCase(m_font_names.Item(i)) == 0)
        {
            found = true;
            break;
        }
    }
    
    // if we couldn't find the default facename, add it
    if (!found)
        m_font_names.Add(default_facename);
    
    // sort the font facenames
    m_font_names.Sort();

    // initialize bitmap manager
    wxString bmpres_path = m_install_path;
    bmpres_path += PATH_SEPARATOR_STR;
    bmpres_path += wxT("imgres.zip");

    defineCommandsMain();
    apphookInitCommands();
    BitmapMgr::initBitmapMgr();
    initIdBitmapMap();
    BitmapMgr::getBitmapMgr()->init(bmpres_path);


    #endif // APP_GUI==1


    // create an app preferences object for our defaults
    m_app_default_preferences = createMemoryAppPreferencesObject();
    if (!m_app_default_preferences)
    {
        appMessageBox(wxT("Could not create a MemoryAppPreferences object"));
        return false;
    }


    #if APP_GUI==1
    initDefaultPreferences();
    #endif
    
    
    // create an app preferences object
    m_app_preferences = createAppPreferencesObject();
    if (!m_app_preferences)
    {
        appMessageBox(wxT("Could not create an AppPreferences object"));
        return FALSE;
    }

    m_app_preferences->init(APP_COMPANY_KEY, APP_CONFIG_KEY);


    // create job queues
    m_job_queue = createJobQueueObject();
    m_script_job_queue = createJobQueueObject();
    m_frame_wnd = NULL;


    // create the extension manager
    m_extension_mgr = new ExtensionMgr;
    
    // create the job scheduler
    m_job_scheduler = new JobScheduler;
    m_job_scheduler->setInterval(21);

    // initialize web client engine
    initWebClient();

    // create the application's controller
    m_app_controller = new AppController;
    
    if (argc > 1)
    {
        // this is used in wxmsw mode to allow the GUI version of the program
        // to run scripts from the command line

        if (xf_get_file_exist(towstr(argv[1])))
        {
#if APP_GUI==1
            wxFrame* f = new wxFrame(NULL, -1, wxT(""));
            if (!runCommandLineScript())
                return FALSE;
            SetExitOnFrameDelete(false);
            return TRUE;
#endif
        }
    }
    
    if (!m_app_controller->init())
    {
        delete m_app_controller;
        m_app_controller = NULL;
        return false;
    }
    
    // start the license, update and document update timers
#if APP_GUI==1
    startLicenseTimer();
    startUpdateTimer();
#endif

    return true;
}


void MainApp::initWebClient()
{
    #if APP_GUI==1
    
    // add some common plugin directories to MOZ_PLUGIN_PATH
    #ifdef __WXMSW__
    wxString program_files_dir;
    if (!::wxGetEnv(wxT("ProgramFiles(x86)"), &program_files_dir))
    {
        ::wxGetEnv(wxT("ProgramFiles"), &program_files_dir);
    }
    if (program_files_dir.Length() == 0 || program_files_dir.Last() != '\\')
        program_files_dir += "\\";
    
    wxString system_dir;
    TCHAR system_buf[255];
    ::GetSystemDirectory(system_buf, 255);
    system_dir = system_buf;
    if (system_dir.Length() == 0 || system_dir.Last() != '\\')
        system_dir += "\\";

    
    wxString dir1 = program_files_dir;
    dir1 += wxT("Mozilla Firefox\\plugins");
    wxWebControl::AddPluginPath(dir1);
    
    wxString dir2 = system_dir;
    dir2 += wxT("Macromed\\Flash");
    wxWebControl::AddPluginPath(dir2);
    
    #else
    #endif
    


    // find out web controls engine path
    wxString web_engine_path = m_install_path;
    web_engine_path = web_engine_path.BeforeLast(PATH_SEPARATOR_CHAR);
    web_engine_path += PATH_SEPARATOR_CHAR;
    web_engine_path += wxT("xr");
    
    
    // on vista and windows 7, sometimes old versions of the files
    // compreg.dat and xpti.dat from previous versions of windows
    // can cause problems
    
    #ifdef __WXMSW__
    
    OSVERSIONINFO os_ver;
    os_ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ::GetVersionEx(&os_ver);
    if (os_ver.dwMajorVersion >= 6) // vista, windows 7, or greater
    {
        wxString s = wxStandardPaths::Get().GetUserLocalDataDir();
        s = s.BeforeLast('\\'); // strip off app name
        s += wxT("\\VirtualStore");
        if (web_engine_path.Length() > 2 && web_engine_path.GetChar(1) == ':')
        {
            s += web_engine_path.substr(2);
            if (::wxDirExists(s))
            {
                wxString fname;
                fname = s + wxT("\\components\\compreg.dat");
                if (::wxFileExists(fname))
                    ::wxRemoveFile(fname);
                fname = s + wxT("\\components\\xpti.dat");
                if (::wxFileExists(fname))
                    ::wxRemoveFile(fname);
            }
        }

    }
    
    #endif // defined(__WXMSW__)
    
    
    wxString app_data_path = this->getAppDataPath();
    if (app_data_path.Right(1) != PATH_SEPARATOR_STR)
        app_data_path += PATH_SEPARATOR_CHAR;
    app_data_path += wxT("Browser");
    if (!xf_get_directory_exist(towstr(app_data_path)))
        xf_mkdir(towstr(app_data_path));

    wxWebControl::SetProfilePath(app_data_path);

    wxWebControl::InitEngine(web_engine_path);
    
    #endif //  APP_GUI==1
}




void MainApp::populatePaperDatabaseClean()
{
    // populate our paper database with clean names and no duplicate sizes
    size_t i, count = wxThePrintPaperDatabase->GetCount();
    for (i = 0; i < count; ++i)
    {
        wxPrintPaperType* paper = wxThePrintPaperDatabase->Item(i);
        if (!paper)
            continue;
        
        // we'll add the custom item later
        if (paper->GetId() == wxPAPER_NONE)
            continue;
        
        // don't allow duplication paper sizes in the paper database
        if (m_paper_database->GetCount() > 0)
        {
            wxPrintPaperType* already_exists;
            already_exists = m_paper_database->FindPaperType(paper->GetSize());
            if (already_exists)
                continue;
        }
        
        // only append names that have a comma in them... we do this
        // because the names in the paper database are formatted as
        // such: "Letter, 8 1/2 x 11 in", and there are also sizes
        // that we don't want to include such as "11 x 17 in"
        wxString name = paper->GetName();
        if (name.Find(wxT(',')) == wxNOT_FOUND)
            continue;
        
        wxString clean_name = name.BeforeFirst(wxT(','));
        m_paper_database->AddPaperType(paper->GetId(),
                                       clean_name,
                                       paper->GetWidth(),
                                       paper->GetHeight());
    }
}


// app uninitialization

int MainApp::OnExit()
{
    apphookOnExit();

    BitmapMgr::uninitBitmapMgr();
   
    delete m_job_scheduler;
    delete m_help_controller;
    delete m_paper_database;
    delete m_app_controller;
    delete m_extension_mgr;
    delete m_command_line;

#ifdef __WXMSW__
    if (m_taskbar_icon)
    {
        m_taskbar_icon->RemoveIcon();
        delete m_taskbar_icon;
    }
#endif

    delete g_license_timer;
    delete g_update_timer;
    delete g_auth;
    g_auth = NULL;

    m_app_preferences->flush();
    m_app_preferences.clear();

    return wxAppBaseClass::OnExit();
}


/*
class TestTimer : public wxTimer
{
public:

    void Notify()
    {
        printf("Hello!!!\n");
    }
};
*/

// this function is only used in console mode;  After tha
// "main script" finishes, it instructs the main thread's
// event loop to exit

static void onMainScriptFinished(jobs::IJobPtr job)
{
    g_app->ExitMainLoop();
}

static void func_console_print(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wxString message = env->m_eval_params[0]->getString();
    message += "\n";
    wxPrintf("%s", (const wxChar*)message.c_str());
}

bool MainApp::runCommandLineScript()
{
    if (argc <= 1)
    {
        // no script to run -- return success
        return true;
    }
    
    wxString path = argv[1];
    if (path.Freq(PATH_SEPARATOR_CHAR) == 0)
    {
        path = ::wxGetCwd();
        if (path.Length() == 0 || path.Last() != PATH_SEPARATOR_CHAR)
            path += PATH_SEPARATOR_CHAR;
        path += argv[1];
    }


    xd::IDatabasePtr db = getDatabase();
    if (db.isNull())
    {
        // no database, try an xdfs next
        db.create_instance("xdfs.Database");
        setDatabase(db);
    }
    
    ScriptHostParams* params = new ScriptHostParams;
    params->print_function.setFunction(func_console_print);
    AppScriptError error;
    jobs::IJobPtr job = getAppController()->executeScript(path, params, &error);
    if (error.code != 0)
    {
        if (error.file.length() == 0)
            error.file = path;
            
#if APP_CONSOLE==1
        #ifdef _UNICODE
        wprintf(L"%ls(%d) : %ls\n", (const wchar_t*)error.file.c_str(), error.line+1, (const wchar_t*)error.message.c_str());
        #else
        wprintf(L"%s(%d) : %s\n", (const wchar_t*)error.file.c_str(), error.line+1, (const wchar_t*)error.message.c_str());
        #endif
#else
        wchar_t str[512];
        swprintf(str, 512, L"%ls(%d) : %ls\n", (const wchar_t*)error.file.c_str(), error.line+1, (const wchar_t*)error.message.c_str());
        appMessageBox(str);
#endif

        return false;
    }

    if (job.isNull())
    {
        wprintf(L"An error occurred while compiling the script.\n");
        return false;
    }

    job->sigJobFinished().connect(onMainScriptFinished);

    return true;
}


int MainApp::OnRun()
{
    #if APP_CONSOLE==1
    
    if (argc > 1)
    {
        if (!runCommandLineScript())
            return 0;
    }

    return MainLoop();
    
    #else
    
    return wxAppBaseClass::OnRun();
    
    #endif
}

void MainApp::startLicenseTimer()
{
    if (!g_license_timer)
    {
        g_license_timer = new LicenseTimer;
    }
    
    g_license_timer->Start(30000);  // every 30 seconds
}

void MainApp::stopLicenseTimer()
{
    if (g_license_timer)
        g_license_timer->Stop();
}

void MainApp::startUpdateTimer()
{
    if (!g_update_timer)
    {
        g_update_timer = new UpdateTimer;
    }
    
    // start once every 10 minutes;
    // the timer itself will modify this time interval
    g_update_timer->Start(600000);
}

void MainApp::stopUpdateTimer()
{
    if (g_update_timer)
        g_update_timer->Stop();
}



void MainApp::onActivateApp(wxActivateEvent& evt)
{
    #ifdef __WXMSW__
    if (evt.GetActive())
    {
        if (m_frame)
        {
            // if application frame was activated by the mouse being clicked
            // on the DbDoc window, don't attempt to activate any child
            // window, because this will cause drag-and-drop operations
            // to not work properly (ex. dragging a table from the DbDoc to
            // the relationship manager)
            DbDoc* dbdoc = getDbDoc();
            if (dbdoc)
            {
                wxWindow* dbdocwnd = dbdoc->getDocWindow();
                if (dbdocwnd)
                {
                    wxRect rect = dbdocwnd->GetScreenRect();
                    wxPoint pt = ::wxGetMousePosition();
                    if (rect.Contains(pt))
                    {
                        evt.Skip();
                        return;
                    }
                }
            }



            IDocumentSitePtr site = m_frame->getActiveChild();
            if (site)
            {
                IDocumentPtr doc = site->getDocument();
                if (doc)
                {
                    doc->setDocumentFocus();
                }
            }
        }
    }
     else
    {
    }
    #endif

    evt.Skip();
}


bool MainApp::getJobsActive()
{
    if (m_job_queue.isNull())
        return false;
    return m_job_queue->getJobsActive();
}

jobs::IJobQueuePtr MainApp::getJobQueue()
{
    return m_job_queue;
}

jobs::IJobQueuePtr MainApp::getScriptJobQueue()
{
    return m_script_job_queue;
}

JobScheduler* MainApp::getJobScheduler()
{
    return m_job_scheduler;
}

wxArrayString MainApp::getFontNames()
{
    return m_font_names;
}



IFramePtr MainApp::getMainFrame()
{
    return m_frame;
}

AppController* MainApp::getAppController()
{
    return m_app_controller;
}

IAppPreferencesPtr MainApp::getAppPreferences()
{
    return m_app_preferences;
}

IAppPreferencesPtr MainApp::getAppDefaultPreferences()
{
    return m_app_default_preferences;
}

IAppConfigPtr MainApp::getAppConfig()
{
    return createAppConfigObject();
}

wxFrame* MainApp::getMainWindow()
{
    return m_frame_wnd;
}

wxPrintPaperDatabase* MainApp::getPaperDatabase()
{
    return m_paper_database;
}

wxHelpControllerBase* MainApp::getHelpController()
{
    return m_help_controller;
}

ExtensionMgr* MainApp::getExtensionMgr()
{
    return m_extension_mgr;
}

wxString MainApp::getInstallPath()
{
    return m_install_path;
}

wxString MainApp::getAppDataPath()
{
    return ::getAppDataPath();
}

wxCmdLineParser* MainApp::getCommandLine()
{
    if (m_command_line)
        return m_command_line;

    static const wxCmdLineEntryDesc cmd_line_desc[] =
    {
        { wxCMD_LINE_SWITCH, "n", "noext",   "don't start extensions automatically" },
        { wxCMD_LINE_SWITCH, "s", "svc",     "used when starting application in service mode" },
        { wxCMD_LINE_SWITCH, "c", "svccfg",  "configure the service (windows only)" },
        { wxCMD_LINE_PARAM,  NULL, NULL, "input file", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE | wxCMD_LINE_PARAM_OPTIONAL },
        { wxCMD_LINE_NONE }
    };

    m_command_line = new wxCmdLineParser(cmd_line_desc, argc, argv);
    m_command_line->Parse(false);
    return m_command_line;
}

void MainApp::setMainFrame(IFramePtr frame)
{
    m_frame = frame;

    if (m_frame)
        m_frame_wnd = m_frame->getFrameWindow();
         else
        m_frame_wnd = NULL;
}

xd::IDatabasePtr MainApp::getDatabase()
{
    return m_database;
}

void MainApp::setDatabase(xd::IDatabasePtr database)
{
    m_database = database;

    if (m_database)
    {
        // load the jobs scheduled to run for this database
        m_job_scheduler->load();
    }
}


std::wstring MainApp::getDbDriver()
{
    if (m_database.isNull())
        return L"";

    xcm::class_info* class_info = xcm::get_class_info(m_database.p);
    std::wstring class_name = kl::towstring(class_info->get_name());

    return kl::beforeFirst(class_name,'.');
}

void MainApp::setDatabaseLocation(const wxString& string)
{
    m_db_location = string;
}

wxString MainApp::getDatabaseLocation()
{
    return m_db_location;
}

DbDoc* MainApp::getDbDoc()
{
    return m_dbdoc;
}

void MainApp::setDbDoc(DbDoc* dbdoc)
{
    m_dbdoc = dbdoc;
}

LinkBar* MainApp::getLinkBar()
{
    return m_linkbar;
}

void MainApp::setLinkBar(LinkBar* linkbar)
{
    m_linkbar = linkbar;
}

bool MainApp::isDatabaseOpen()
{
    return m_database.p ? true : false;
}

bool MainApp::isDatabaseReadOnly()
{
    return m_app_preferences->getBoolean(wxT("app.data_locked"), true);
}

wxString MainApp::getProjectName()
{
    if (m_database.isNull())
        return wxT("");

    xd::IAttributesPtr attr = m_database->getAttributes();
    if (attr.isNull())
        return wxT("");

    return attr->getStringAttribute(xd::dbattrDatabaseName);
}

void MainApp::setProjectName(const wxString& name)
{
    if (m_database.isNull())
        return;

    xd::IAttributesPtr attr = m_database->getAttributes();
    if (attr.isNull())
        return;

    wxString old_project_name = attr->getStringAttribute(xd::dbattrDatabaseName);

    attr->setStringAttribute(xd::dbattrDatabaseName, towstr(name));

    ProjectMgr projmgr;
    int idx = projmgr.getIdxFromLocation(getDatabaseLocation());
    if (idx != -1)
    {
        projmgr.modifyProjectEntry(idx, name, wxEmptyString, wxEmptyString, wxEmptyString);
    }
}



#ifdef __WXMSW__

void MainApp::showApp(bool show)
{
    if (m_frame)
    {
        // create task bar icon
    
#ifdef __WXMSW__

        if (!show)
        {
            AppBusyCursor bc;
            if (!m_taskbar_icon)
            {
                m_taskbar_icon = new AppTaskBarIcon;
            }
            
            wxIcon logo;
            logo.LoadFile(wxT("AA_APPICON_9"), wxBITMAP_TYPE_ICO_RESOURCE);
            
            m_taskbar_icon->SetIcon(logo, wxEmptyString);
        }
         else
        {
            if (m_taskbar_icon)
            {
                m_taskbar_icon->RemoveIcon();
            }
        }
#endif

        wxFrame* frame = m_frame->getFrameWindow();
        
        // solves a bug when another process
        // shows our window without us knowing it:
        // see MainApp::OnInit()
        
        frame->Show(!show);
        frame->Show(show);

        if (frame->IsIconized())
            frame->Iconize(false);
    }
}


#endif



AppMacroRecorder::AppMacroRecorder()
{
    m_indent = 0;
    m_recording = false;
}

AppMacroRecorder& AppMacroRecorder::operator<<(const wxString& s)
{
    if (!m_recording)
        return *this;
        
    addLine(s);
    return *this;
}

AppMacroRecorder& AppMacroRecorder::operator<<(const char* s)
{
    if (!m_recording)
        return *this;

    wxString line = wxString::From8BitData(s);
    addLine(line);

    return *this;
}

bool AppMacroRecorder::isRecording()
{
    return m_recording;
}

void AppMacroRecorder::setRecording(bool recording)
{
    m_recording = recording;
}

void AppMacroRecorder::setIndent(int indent)
{
    m_indent = indent;
}

std::vector<wxString>& AppMacroRecorder::getLines()
{
    return m_lines;
}

void AppMacroRecorder::reset()
{
    m_lines.clear();
}

void AppMacroRecorder::addLine(const wxString& s)
{
    wxString line;
    line.Append(wxT(' '), m_indent);
    line += s;
    m_lines.push_back(line);

    // TODO: temporary echo to console; remove later
    IFramePtr frame = g_app->getMainFrame();
    if (frame.isOk())
    {
        IDocumentSitePtr site = frame->lookupSite(wxT("ConsolePanel"));
        if (site.isOk())
        {
            IConsolePanelPtr console = site->getDocument();
            if (console.isOk())
                console->print(line + wxT("\n"));
        }
    }
}



#if 0
// this code is never run, but we absolutely require
// this strings to be translated (e.g. Ctrl into Strg
// for german), so that our accelerators work properly;
// this ensures that these strings land in our .po file
_("ctrl")
_("shift")
_("alt")

// these are stock labels used in wxWidgets which need to be translated
_("&About")
_("&Apply")
_("&Cancel")
_("&Yes")
_("&No")
_("&OK")
_("&Save")
_("Save &As...")
#endif

