/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-12-14
 *
 */


#ifndef __KXINTER_APP_H
#define __KXINTER_APP_H


#include "kxinter.h"


class AppController;
class ConnectionMgr;
class TreeController;

class KxInterApp : public wxApp,
                   public xcm::signal_sink
{
public:

    KxInterApp();
    void setMainFrame(cfw::IFramePtr frame);
    cfw::IFramePtr getMainFrame();
    cfw::IJobQueuePtr getJobQueue();
    cfw::IAppPreferencesPtr getAppPreferences();
    wxFrame* getMainWindow();

    AppController* getAppController();
    ConnectionMgr* getConnectionMgr();
    TreeController* getTreeController();

    tango::IDatabasePtr getXbaseDatabase();
    tango::IDatabasePtr getDelimitedTextDatabase();

    wxString getAppDataPath();

private:

    wxFrame* m_frame_wnd;
    cfw::IFramePtr m_frame;
    cfw::IJobQueuePtr m_job_queue;
    cfw::IAppPreferencesPtr m_app_preferences;

    AppController* m_app_controller;
    ConnectionMgr* m_connection_mgr;

    tango::IDatabasePtr m_xbase_db;
    tango::IDatabasePtr m_textdelimited_db;

    wxString m_appdata_path;

private:
    
    bool OnInit();
    int OnExit();
};




#endif
