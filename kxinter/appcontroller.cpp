/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-15
 *
 */


#include "kxinter.h"
#include "appcontroller.h"
#include "datadoc.h"
#include "treecontroller.h"
#include "treemodel.h"
#include "panelconsole.h"
#include "griddoc.h"
#include "panelcopytables.h"
#include "../kappcmn/dlgconnectionwizard.h"
#include "../kappcmn/dlgauth.h"


enum
{
    // -- File Menu --
    ID_Open = ID_App_LastCommand + 1,
    ID_NewConnection,
    ID_NewDirectory,
    ID_NewDbConnection,
    ID_CopyTables,
    ID_Exit,
    
    // -- Window Menu --
    ID_Cascade,
    ID_TileHorizontal,
    ID_TileVertical,

    // -- Help Menu --
    ID_LicenseManager,
    ID_About,

    // -- Other --
    ID_New,
    ID_ToggleAllPanels,
    ID_ToggleTreePanel
};


BEGIN_EVENT_TABLE(AppController, wxEvtHandler)
    EVT_MENU(ID_NewConnection, AppController::onNewConnection)
    EVT_MENU(ID_Exit, AppController::onExit)
    EVT_MENU(ID_Cascade, AppController::onCascade)
    EVT_MENU(ID_TileHorizontal, AppController::onTileHorizontal)
    EVT_MENU(ID_TileVertical, AppController::onTileVertical)
    EVT_MENU(ID_ToggleAllPanels, AppController::onToggleAllPanels)
    EVT_MENU(ID_ToggleTreePanel, AppController::onToggleTreePanel)
    EVT_MENU(ID_About, AppController::onAbout)
    EVT_MENU(ID_New, AppController::onNew)
    EVT_MENU(ID_Open, AppController::onOpen)
    EVT_MENU(ID_CopyTables, AppController::onCopyTables)
END_EVENT_TABLE()



extern paladin::Authentication* g_auth;


kcl::ToolBar* AppController::createStandardToolbar()
{
    kcl::ToolBar* toolbar_standard = new kcl::ToolBar(g_app->getMainFrame()->getFrameWindow(), 2001, wxDefaultPosition, wxDefaultSize);
    toolbar_standard->AddTool(ID_NewConnection, GET_XPM(xpm_new), wxNullBitmap, false, 0);
    toolbar_standard->AddTool(ID_Open, GET_XPM(xpm_fileopen), wxNullBitmap, false, 0);
    //toolbar_standard->AddTool(ID_App_Save, GET_XPM(xpm_save), wxNullBitmap, false, 0);
    //toolbar_standard->AddTool(ID_App_Print, GET_XPM(xpm_print), wxNullBitmap, false, 0);
    toolbar_standard->AddSeparator();
    toolbar_standard->AddTool(ID_App_Cut, GET_XPM(xpm_cut), wxNullBitmap, false, 0);
    toolbar_standard->AddTool(ID_App_Copy, GET_XPM(xpm_copy), wxNullBitmap, false, 0);
    toolbar_standard->AddTool(ID_App_Paste, GET_XPM(xpm_paste), wxNullBitmap, false, 0);
    toolbar_standard->AddSeparator();
    toolbar_standard->AddTool(ID_ToggleAllPanels, GET_XPM(xpm_toggleallpanels), wxNullBitmap, false, 0);
    toolbar_standard->AddTool(ID_ToggleTreePanel, GET_XPM(xpm_treepanel), wxNullBitmap, true, 0);
    toolbar_standard->AddSeparator();
    toolbar_standard->AddTool(ID_Cascade, GET_XPM(xpm_cascade), wxNullBitmap, false, 0);
    toolbar_standard->AddTool(ID_TileHorizontal, GET_XPM(xpm_tilehorz), wxNullBitmap, false, 0);
    toolbar_standard->AddTool(ID_TileVertical, GET_XPM(xpm_tilevert), wxNullBitmap, false, 0);
    toolbar_standard->Realize();

    toolbar_standard->SetToolShortHelp(ID_New, _("New Connection"));
    //toolbar_standard->SetToolShortHelp(ID_OpenProject, _("Open Project"));
    //toolbar_standard->SetToolShortHelp(ID_App_Save, _("Save"));
    //toolbar_standard->SetToolShortHelp(ID_App_Print, _("Print"));
    toolbar_standard->SetToolShortHelp(ID_App_Cut, _("Cut"));
    toolbar_standard->SetToolShortHelp(ID_App_Copy, _("Copy"));
    toolbar_standard->SetToolShortHelp(ID_App_Paste, _("Paste"));
    toolbar_standard->SetToolShortHelp(ID_ToggleAllPanels, _("Show/Hide Docked Panels"));
    toolbar_standard->SetToolShortHelp(ID_ToggleTreePanel, _("Show/Hide Connection Manager"));
    toolbar_standard->SetToolShortHelp(ID_Cascade, _("Cascade"));
    toolbar_standard->SetToolShortHelp(ID_TileHorizontal, _("Tile Windows Horizontally"));
    toolbar_standard->SetToolShortHelp(ID_TileVertical, _("Tile Windows Vertically"));

    return toolbar_standard;
}



AppController::AppController()
{
}


bool AppController::init()
{
    // -- get saved windows dimensions --
    long x, y, w, h, screen_x, screen_y;
    bool maximized;


    screen_x = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
    screen_y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);


    cfw::IAppPreferencesPtr prefs = g_app->getAppPreferences();
    x = prefs->getLong(wxT("app.window_dimensions.xpos"), 0);
    y = prefs->getLong(wxT("app.window_dimensions.ypos"), 0);
    w = prefs->getLong(wxT("app.window_dimensions.width"), screen_x);
    h = prefs->getLong(wxT("app.window_dimensions.height"), screen_y);
    maximized = prefs->getBoolean(wxT("app.window_dimensions.maximized"), false);
    

    if (x < 0 || x > screen_x-100)
        x = 0;
    if (y < 0 || y > screen_y-100)
        y = 0;
    if (w < 100)
        w = 600;
    if (h < 100)
        h = 400;
    if (w > screen_x)
        w = screen_x;
    if (h > screen_y)
        h = screen_y;

    // -- create the main frame --
    m_frame.create_instance("cfw.MainFrame");
    if (m_frame.isNull())
        return false;
    
    m_frame->create(NULL, PRODUCT_NAME, x, y, w, h);
    m_frame->addWindowEventHandler(this);

    g_app->SetTopWindow(m_frame->getFrameWindow());
    g_app->setMainFrame(m_frame);

    if (maximized)
    {
        m_frame->getFrameWindow()->Maximize();
    }


    if (!checkLicense(true))
        return false;


    // -- load registry settings --
    loadRegistry();

    // -- create the tree controller --
    m_tree_controller = new TreeController;

    // -- create console panel to add to the binder panel --
    ConsolePanel* console_panel = new ConsolePanel;

    // -- create job list panel to add to the binder panel --
    cfw::IJobListPanelPtr joblist_panel;
    joblist_panel.create_instance("cfw.JobListPanel");
    joblist_panel->setJobQueue(g_app->getJobQueue());
    joblist_panel->setUpdateInterval(5000);

    // -- create binderpanel and dock it --
    cfw::IBinderPanelPtr binder_panel;
    binder_panel.create_instance("cfw.BinderPanel");
    binder_panel->addPage(m_frame, console_panel, _("Console"), GET_XPM(xpm_consolepanel_small));
    binder_panel->addPage(m_frame, joblist_panel, _("Jobs"), GET_XPM(xpm_job_small));

    m_binderpanel_site = m_frame->createSite(binder_panel,
                                             cfw::sitetypeDockable | cfw::dockBottom,
                                             0, 0, 140, 140);

    // -- create menus --
    cfw::wxBitmapMenu* menuNew = new cfw::wxBitmapMenu;
    menuNew->Append(ID_NewConnection, _("Remote Database Connection..."), GET_XPM(xpm_new));
    menuNew->Append(ID_NewDbConnection, _("Project Connection..."), GET_XPM(xpm_new));
    menuNew->Append(ID_NewDirectory, _("Directory Shortcut..."), GET_XPM(xpm_new));

    cfw::wxBitmapMenu* menuFile = new cfw::wxBitmapMenu;
    menuFile->Append(1, _("New"), menuNew);
    menuFile->Append(ID_Open, _("&Open..."));
    menuFile->Append(ID_CopyTables, _("&Copy Tables..."));
    menuFile->AppendSeparator();
    menuFile->Append(ID_Exit, _("E&xit"));

    cfw::wxBitmapMenu* menuWindow = new cfw::wxBitmapMenu;
    menuWindow->Append(ID_Cascade, _("&Cascade"), GET_XPM(xpm_cascade));
    menuWindow->Append(ID_TileHorizontal, _("Tile &Horizontally"), GET_XPM(xpm_tilehorz));
    menuWindow->Append(ID_TileVertical, _("&Tile Vertically"), GET_XPM(xpm_tilevert));

    cfw::wxBitmapMenu* menuHelp = new cfw::wxBitmapMenu;
    menuHelp->Append(ID_LicenseManager, _("&License Manager..."));
    menuHelp->AppendSeparator();
    menuHelp->Append(ID_About, _("&About..."));

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, _("&File"));
    menuBar->Append(menuWindow, _("&Window"));
    menuBar->Append(menuHelp, _("&Help"));


    // -- create toolbars --

    m_frame->dockWindow(createStandardToolbar(),
                        cfw::sitetypeDockable | cfw::dockTop | cfw::siteNoResize,
                        -1, -1, 0, 0);

    m_frame->setMenuBar(menuBar, 1);
    m_frame->show(true);

    return true;
}


AppController::~AppController()
{
}


void AppController::showLicenseManager()
{
    DlgAuth auth(g_app->getMainWindow(),
                 PRODUCT_NAME,
                 PRODUCT_VERSION,
                 g_auth,
                 g_app->getAppPreferences());
    auth.ShowModal();
}

bool AppController::checkLicense(bool show_warnings)
{
    if (g_auth->checkAuth() != paladin::errNone)
    {
        showLicenseManager();

        if (g_auth->checkAuth() != paladin::errNone)
        {
            return false;
        }
    }


    int days_left = g_auth->getDaysLeft();
    if (days_left >= 0 && days_left <= 10 && show_warnings)
    {
        wxString message = wxString::Format(_("Your license will expire in %d day(s).  If you would like to renew your license now,\nopen the license manager.  The license manager can be found in the Help menu."), days_left);
        wxMessageBox(message,
                     _("License Manager"),
                     wxOK | wxICON_INFORMATION | wxCENTER,
                     g_app->getMainWindow());

        return true;
    }

    return true;
}


void AppController::onFrameDestroy()
{
    saveRegistry();


    wxFrame* frame_wnd = g_app->getMainWindow();

    if (frame_wnd->IsIconized())
    {
        frame_wnd->Iconize(false);
    }

    // -- main window dimensions --

    cfw::IAppPreferencesPtr prefs = g_app->getAppPreferences();

    wxPoint pos = frame_wnd->GetPosition();
    wxSize size = frame_wnd->GetSize();
    bool maximized = frame_wnd->IsMaximized();

    prefs->setLong(wxT("app.window_dimensions.xpos"), pos.x);
    prefs->setLong(wxT("app.window_dimensions.ypos"), pos.y);
    prefs->setLong(wxT("app.window_dimensions.width"), size.GetWidth());
    prefs->setLong(wxT("app.window_dimensions.height"), size.GetHeight());
    prefs->setBoolean(wxT("app.window_dimensions.maximized"), maximized);
    prefs->flush();

    m_frame.clear();
    m_treepanel_site.clear();
    m_binderpanel_site.clear();

    g_app->setMainFrame(xcm::null);
}


TreeController* AppController::getTreeController()
{
    return m_tree_controller;
}


void AppController::onAbout(wxCommandEvent& event)
{
    cfw::appMessageBox(wxT(PRODUCT_NAME + " - Copyright (c) 2004, Kirix Research, LLC.  All rights reserved."),
                  _("About"),
                  wxICON_INFORMATION);
}

void AppController::onExit(wxCommandEvent& event)
{
    g_app->getMainWindow()->Destroy();
}


void AppController::onToggleAllPanels(wxCommandEvent& event)
{
    toggleAllPanels();
}


void AppController::onToggleTreePanel(wxCommandEvent& event)
{
    toggleTreePanel();
}


void AppController::onCascade(wxCommandEvent& event)
{
    g_app->getMainFrame()->cascade();
}


void AppController::onTileHorizontal(wxCommandEvent& event)
{
    g_app->getMainFrame()->tileHorizontal();
}


void AppController::onTileVertical(wxCommandEvent& event)
{
    g_app->getMainFrame()->tileVertical();
}


void AppController::toggleAllPanels()
{
/*
    cfw::IDocumentSiteEnumPtr docsites;
    cfw::IDocumentSitePtr site;
    cfw::IDocumentPtr doc;
    wxWindow* docwnd;

    docsites = m_frame->getDocumentSites(cfw::sitetypeDockable);

    std::vector<cfw::IDocumentSitePtr> all_sites;
    bool show = true;

    int site_count = docsites->size();
    for (int i = 0; i < site_count; ++i)
    {
        site = docsites->getItem(i);

        if (site->getSiteType() & cfw::dockFloating)
            continue;

        if (site->getVisible())
        {
            show = false;
        }

        doc = site->getDocument();
        if (doc.isOk())
        {
            docwnd = doc->getDocumentWindow();
            if (docwnd)
            {
                if (!docwnd->IsKindOf(CLASSINFO(wxToolBar)) &&
                    !docwnd->IsKindOf(&kcl::ToolBar::sm_toolbar_class_info))
                {
                    all_sites.push_back(site);
                }
            }
        }
    }

    std::vector<cfw::IDocumentSitePtr>::iterator site_it;
    for (site_it = all_sites.begin(); site_it != all_sites.end(); ++site_it)
    {
        if (show)
        {
            if (!(*site_it)->getVisible())
            {
                (*site_it)->setVisible(true);
            }
        }
         else
        {
            if ((*site_it)->getVisible())
            {
                (*site_it)->setVisible(false);
            }
        }
    }
*/
}


void AppController::toggleTreePanel()
{
    bool visible = m_treepanel_site->getVisible();
    m_treepanel_site->setVisible(!visible);
}


void AppController::onNew(wxCommandEvent& event)
{
    DataDoc* doc = new DataDoc;

    m_frame->createSite(doc,
                        cfw::sitetypeNormal,
                        0, 0, -1, -1);
}


void AppController::onOpen(wxCommandEvent& event)
{
    wxString filter;

    filter =  _("EBCDIC files (*.ebc)|*.ebc|All Files (*.*)|*.*|");
    wxFileDialog dlg(m_frame->getFrameWindow(), _("Open File(s)"), wxT(""), wxT(""), filter, wxOPEN|wxFILE_MUST_EXIST|wxHIDE_READONLY);

    if (dlg.ShowModal() == wxID_OK)
    {
        openDataDoc(dlg.GetPath());
    }

}


void AppController::openDataDoc(const wxString& filename)
{
    DataDoc* doc = new DataDoc;

    doc->open(filename);

    m_frame->createSite(doc,
                        cfw::sitetypeNormal,
                        0, 0, -1, -1);
}


void AppController::openTableDoc(tango::IDatabasePtr db, const wxString& filename)
{
    wxBusyCursor bc;

    tango::ISetPtr set = db->openSet(towstr(filename));
    if (set.isNull())
    {
        return;
    }

    tango::IIteratorPtr iter = set->createIterator(L"", L"", NULL);


    cfw::IDocumentSitePtr site;
    GridDoc* gd = new GridDoc;
    site = g_app->getMainFrame()->createSite(gd, cfw::sitetypeNormal, -1, -1, -1, -1);

    if (db == g_app->getXbaseDatabase() || db == g_app->getDelimitedTextDatabase())
    {
        gd->setName(filename);
    }
     else
    {
        gd->setName(filename + wxT(" on ") + towx(db->getDatabaseName()));
    }

    if (!gd->setIterator(iter))
    {
        gd->Destroy();
        return;
    }
}


void AppController::showJobInfo(int job_id)
{
    cfw::IJobPtr job = g_app->getJobQueue()->lookupJob(job_id);
    if (job)
    {
        cfw::IJobInfoPanelPtr panel;
        panel.create_instance("cfw.JobInfoPanel");
        panel->init(g_app->getJobQueue(), job->getJobInfo());
        cfw::IDocumentSitePtr site = g_app->getMainFrame()->createSite(panel, cfw::sitetypeModeless, -1, -1, 420, 230);
        site->setMinSize(300, 230);
    }
}

void onDlgConnectionWizardFinished(DlgConnectionWizard* dlg)
{
    ConnectionInfo info = dlg->getConnectionInfo();
    ConnectionMgr* conn_mgr = g_app->getConnectionMgr();
    IConnectionPtr conn = conn_mgr->createConnection();

    conn->setDescription(info.description);
    conn->setType(info.type);
    conn->setHost(info.server);
    conn->setPort(info.port);
    conn->setDatabase(info.database);
    conn->setUsername(info.username);
    conn->setPassword(info.password);
    conn->setPath(info.path);
    conn->setFilter(info.filter);

    g_app->getAppController()->saveRegistry();
    g_app->getTreeController()->populate();
}


void AppController::onNewConnection(wxCommandEvent& event)
{
    cfw::IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("DlgConnectionWizard"));
    if (site.isNull())
    {
        DlgConnectionWizard* cw = new DlgConnectionWizard(dbtypeExternalConnection|dbtypeXdnative);
        cw->sigConnectionWizardFinished.connect(&onDlgConnectionWizardFinished);

        site = g_app->getMainFrame()->createSite(cw, cfw::sitetypeModeless, -1, -1, 430, 480);
        site->setName(wxT("DlgConnectionWizard"));
    }
     else
    {
        if (!site->getVisible())
        {
            site->setVisible(true);
        }
    }
}


void AppController::onCopyTables(wxCommandEvent& event)
{
    cfw::IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("CopyTablesPanel"));
    if (site.isNull())
    {
        CopyTablesPanel* copy_panel = new CopyTablesPanel;
        site = g_app->getMainFrame()->createSite(copy_panel, cfw::sitetypeModeless, -1, -1, 480, 380);
        site->setName(wxT("CopyTablesPanel"));
    }
     else
    {
        if (!site->getVisible())
        {
            site->setVisible(true);
        }
    }
}


bool AppController::loadRegistry()
{
    // -- load the connections from  the registry --
    g_app->getConnectionMgr()->loadRegistry(g_app->getAppPreferences());
    return true;
}


bool AppController::saveRegistry()
{
    // -- save the connections to the registry --
    g_app->getConnectionMgr()->saveRegistry(g_app->getAppPreferences());
    return true;
}




