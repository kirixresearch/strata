/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-15
 *
 */


#ifndef __KXINTER_APPCONTROLLER_H
#define __KXINTER_APPCONTROLLER_H


enum
{
    ID_App_FirstCommand = 20000,
    ID_App_Print = 20001,
    ID_App_Cut = 20002,
    ID_App_Copy = 20003,
    ID_App_Paste = 20004,
    ID_App_Save = 20005,
    ID_App_LastCommand = 20006
};


class TreeController;
class AppController : public wxEvtHandler,
                      public xcm::signal_sink
{
public:

    AppController();
    ~AppController();

    bool init();
    TreeController* getTreeController();
    bool loadRegistry();
    bool saveRegistry();

    void openDataDoc(const wxString& filename);
    void openTableDoc(tango::IDatabasePtr db, const wxString& filename);

    void showJobInfo(int job_id);
    void showLicenseManager();

    void onNewConnection(wxCommandEvent& event);

private:

    bool checkLicense(bool show_warnings);

    kcl::ToolBar* createStandardToolbar();

    void toggleAllPanels();
    void toggleTreePanel();

private:

    void onExit(wxCommandEvent& event);
    void onToggleAllPanels(wxCommandEvent& event);
    void onToggleTreePanel(wxCommandEvent& event);
    void onCascade(wxCommandEvent& event);
    void onTileHorizontal(wxCommandEvent& event);
    void onTileVertical(wxCommandEvent& event);
    void onNew(wxCommandEvent& event);
    void onOpen(wxCommandEvent& event);
    void onCopyTables(wxCommandEvent& event);
    void onAbout(wxCommandEvent& event);
    void onFrameDestroy();

private:

    cfw::IFramePtr m_frame;
    cfw::IDocumentSitePtr m_treepanel_site;
    cfw::IDocumentSitePtr m_binderpanel_site;

    TreeController* m_tree_controller;

    DECLARE_EVENT_TABLE()
};



#endif


