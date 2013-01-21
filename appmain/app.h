/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
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
    IJobQueuePtr getJobQueue();
    IJobQueuePtr getScriptJobQueue();
    
    wxFrame* getMainWindow();
    JobScheduler* getJobScheduler();
    AppController* getAppController();
    IAppPreferencesPtr getAppPreferences();
    IAppPreferencesPtr getAppDefaultPreferences();
    IAppConfigPtr getAppConfig();
    wxHelpControllerBase* getHelpController();
    wxPrintPaperDatabase* getPaperDatabase();
    ExtensionMgr* getExtensionMgr();
    wxCmdLineParser* getCommandLine();
    
    wxArrayString getFontNames();
    wxString getBookmarkFolder();
    wxString getInstallPath();
    wxString getAppDataPath();
    
    IFramePtr getMainFrame();
    void setMainFrame(IFramePtr frame);

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

    IAppPreferencesPtr m_app_preferences;
    IAppPreferencesPtr m_app_default_preferences;
    IJobQueuePtr m_job_queue;
    IJobQueuePtr m_script_job_queue;
    
    IFrame* m_frame;
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

    AppMacroRecorder();
    AppMacroRecorder& operator<<(const wxString& s);
    AppMacroRecorder& operator<<(const char* s);

    bool isRecording();
    void setRecording(bool recording);
    void setIndent(int indent);
    std::vector<wxString>& getLines();
    void reset();
    
private:

    void addLine(const wxString& s);

private:

    std::vector<wxString> m_lines;
    int m_indent;
    bool m_recording;
};


extern MainApp* g_app;
extern AppMacroRecorder g_macro;

#endif
