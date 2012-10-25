/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2001-11-15
 *
 */


#ifndef __APP_APP_H
#define __APP_APP_H



class JobScheduler;
class AppController;
class AppTaskBarIcon;
class ExtensionMgr;
class DbDoc;
class WebServer;
class wxHelpController;
class wxHelpControllerBase;
class wxCHMHelpController;
class wxPrintPaperDatabase;



#if APP_CONSOLE==1
#define wxAppBaseClass wxAppConsole
#else
#define wxAppBaseClass wxApp
#endif

class MainApp : public wxAppBaseClass
{

public:

    MainApp();

    bool getJobsActive();
    cfw::IJobQueuePtr getJobQueue();
    cfw::IJobQueuePtr getScriptJobQueue();
    
    wxFrame* getMainWindow();
    JobScheduler* getJobScheduler();
    AppController* getAppController();
    cfw::IAppPreferencesPtr getAppPreferences();
    cfw::IAppPreferencesPtr getAppDefaultPreferences();
    IAppConfigPtr getAppConfig();
    wxHelpControllerBase* getHelpController();
    wxPrintPaperDatabase* getPaperDatabase();
    ExtensionMgr* getExtensionMgr();
    wxCmdLineParser* getCommandLine();
    
    wxArrayString getFontNames();
    wxString getBookmarkFolder();
    wxString getInstallPath();
    wxString getAppDataPath();
    
    cfw::IFramePtr getMainFrame();
    void setMainFrame(cfw::IFramePtr frame);

    tango::IDatabasePtr getDatabase();
    void setDatabase(tango::IDatabasePtr database);

    std::wstring getDbDriver();

    void setDatabaseLocation(const wxString& string);
    wxString getDatabaseLocation();

    DbDoc* getDbDoc();
    void setDbDoc(DbDoc* dbdoc);

    wxString getProjectName();
    void setProjectName(const wxString& name);

    bool isDatabaseOpen();
    bool isDatabaseReadOnly();
    bool isService() const { return m_is_service; }
    bool isServiceConfig() const { return m_is_service_config; }

    void startLicenseTimer();   // 'check for valid license' timer
    void stopLicenseTimer();

    void startUpdateTimer();    // 'check for updates' timer
    void stopUpdateTimer();
    
#ifdef WIN32
    // this function either hides or shows the application
    // and allows background processing with the app icon
    // in the 'system tray'
    void showApp(bool show);
#endif

    void processIdle();
    
private:

    void initWebClient();
    void populatePaperDatabaseClean();
    bool runCommandLineScript();
    
    bool OnInit();
    int OnExit();
    int OnRun();
    
    void onActivateApp(wxActivateEvent& evt);

private:

    cfw::IAppPreferencesPtr m_app_preferences;
    cfw::IAppPreferencesPtr m_app_default_preferences;
    cfw::IJobQueuePtr m_job_queue;
    cfw::IJobQueuePtr m_script_job_queue;
    
    cfw::IFrame* m_frame;
    JobScheduler* m_job_scheduler;
    AppController* m_app_controller;
    wxPrintPaperDatabase* m_paper_database;
    ExtensionMgr* m_extension_mgr;
    WebServer* m_web_server;
    wxFrame* m_frame_wnd;
    wxCmdLineParser* m_command_line;
    DbDoc* m_dbdoc;
    
    wxArrayString m_font_names;
    
    tango::IDatabasePtr m_database;
    wxString m_db_location;
    
    wxString m_install_path;
    bool m_inited;
    bool m_is_service;
    bool m_is_service_config;

#ifdef __WXMSW__
    wxCHMHelpController* m_help_controller;
    AppTaskBarIcon* m_taskbar_icon;
#else
    wxHtmlHelpController* m_help_controller;
#endif

    DECLARE_EVENT_TABLE()
};


class AppMacroRecorder
{
public:

    AppMacroRecorder() { m_indent = 0; m_recording = false; }
    AppMacroRecorder& operator<<(const wxString& s)
    {
        if (!m_recording)
            return *this;
            
        wxString line;
        line.Append(wxT(' '), m_indent);
        line += s;
        m_lines.push_back(line);
        return *this;
    }
    AppMacroRecorder& operator<<(const char* s)
    {
        if (!m_recording)
            return *this;

        wxString line;
        line.Append(wxT(' '), m_indent);
        line += wxString::From8BitData(s);
        m_lines.push_back(line);
        return *this;
    }

    bool isRecording() {return m_recording;}
    void setRecording(bool recording) { m_recording = recording; }
    void setIndent(int indent) { m_indent = indent; }
    std::vector<wxString>& getLines() { return m_lines; }
    void reset() { m_lines.clear(); }
    
private:

    std::vector<wxString> m_lines;
    int m_indent;
    bool m_recording;
};


extern MainApp* g_app;
extern AppMacroRecorder g_macro;

#endif
