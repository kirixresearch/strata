/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-03-14
 *
 */


#ifndef __APP_APPCONTROLLER_H
#define __APP_APPCONTROLLER_H


class DbDoc;
class StandardToolbar;
class FormatToolbar;
class LinkBar;
class ImportInfo;
class ExportInfo;
class RelDiagramWatcher;
class JobGaugeUpdateTimer;
class ScriptCommandEventRouter;
class ScriptHostParams;
class BorderPanelHelper;
class ColorPanelHelper;
class wxWebPostData;


struct AppScriptError
{
    int line;
    int offset;
    int code;
    wxString file;
    wxString message;
};


struct HiddenSiteInfo
{
    wxString site_name;
    unsigned int site_flags;
    int x, y, w, h;
};



enum
{
    // these masks only affect the webdoc as of right now
    appOpenForceNewWindow    = 1 << 1,
    appOpenActivateNewWindow = 1 << 2,
    appOpenAsTable = 1 << 3,
    appOpenAsText = 1 << 4,
    appOpenAsWeb = 1 << 5,
    
    appOpenDefault = appOpenActivateNewWindow
};



class AppController : public wxEvtHandler,
                      public xcm::signal_sink
{

public:

    AppController();
    ~AppController();

    bool init();

    IDocumentSitePtr getColumnListPanelSite();
    IDocumentSitePtr getMarkManagerPanelSite();
    ScriptCommandEventRouter* getScriptCommandRouter();
    StandardToolbar* getStandardToolbar();
    FormatToolbar* getFormatToolbar();

    bool checkLicense(bool show_warnings = false);
    void checkForUpdates(bool show_full_gui = true);
    void resetToDefaultPerspective();
    void doPreferenceRefresh();
    void refreshDbDoc();
    void refreshLinkBar();
    void updateTitle();
    void updateURLToolbar();
    void updateQuickFilterToolBarItem();
    void updateViewMenu(IDocumentSitePtr doc_site);
    
    // -- open commands --
    bool openAny(const wxString& location,
                 int open_mask = appOpenDefault,
                 int* site_id = NULL);
    bool openBookmark(const wxString& location,
                 int open_mask = appOpenDefault,
                 int* site_id = NULL);
    bool openReport(const wxString& location, int* site_id = NULL);
    bool openQuery(const wxString& location, int* site_id = NULL);
    bool openScript(const wxString& location, int* site_id = NULL);
    bool openSet(const wxString& location, int* site_id = NULL);
    bool openDataLink(const wxString& location, int* site_id = NULL);
    bool openTemplate(const wxString& location);
    bool openWeb(const wxString& location,
                 wxWebPostData* post_data = NULL,
                 int open_mask = appOpenDefault,
                 int* site_id = NULL);
    bool openExcel(const wxString& location, int* site_id = NULL);
    bool openAccess(const wxString& location);
    bool openPackage(const wxString& location);
    bool setActiveChildByLocation(const wxString& location, int* site_id);
    
    // -- new commands --
    bool newReport(int* site_id = NULL);
    bool newQuery(int* site_id = NULL);
    bool newTable(int* site_id = NULL);
    bool newScript(int* site_id = NULL);
    
    // -- execute commands --
    IJobPtr executeScript(const wxString& location,
                               ScriptHostParams* params = NULL,
                               AppScriptError* error = NULL,
                               bool async = true);
    IJobPtr executeCode(const wxString& source_code,
                             ScriptHostParams* params = NULL,
                             AppScriptError* error = NULL);
    IJobPtr execute(const wxString& location);

    // -- print commands --
    bool print(const wxString& location);
    
    // -- project commands --
    bool createDefaultProject();
    void createDefaultLinks();
    bool openProject(tango::IDatabasePtr database);
    bool openProject(const wxString& location,
                     const wxString& uid,
                     const wxString& password);
    bool createProject(const wxString& location,
                       const wxString& db_name,
                       bool open_project = false);
    bool closeProject();
    bool cleanProject();
    bool doReadOnlyCheck();
    void createMountPoint(const wxString& conn_str,
                          const wxString& mount_path,
                          bool refresh_tree = true);
    
    // -- show commands --
    void showAbout();
    void showCreateExternalConnectionWizard();
    void showImportWizard(const ImportInfo& info,
                          const wxString& location = wxEmptyString);
    void showExportWizard(const ExportInfo& info,
                          const wxString& location = wxEmptyString);
    void showCreateTable();
    void showOptionsDialog();
    void showProjectManager();
    void showRelationshipManager();
    void showLicenseManager();
    void showJobScheduler();
    void showJobManager(bool show = true);
    void showExtensionManager();
    void showProjectProperties();
    void showFindPanel();
    void showFindInFilesPanel();
    void showReplacePanel();
    void showColumnListPanel();
    void showMarkManagerPanel();
    void showStartupPage();
    void showProjectPanel(bool show = true, bool focus = false);
    void showConsolePanel(bool show = true, bool focus = false);
    void toggleFullScreen();
    void toggleLock();
    void toggleAllPanels();
    void toggleProjectPanel();
    void toggleConsolePanel();
    void toggleColumnListPanel();
    void toggleMarkManagerPanel();
    void toggleRelationshipPanel();
    void toggleStatusBar();
    void toggleToolbar(wxWindowID id);
    
    void printConsoleText(const wxString& text);
    
    // -- relationship commands --
    void updateTableDocRelationshipSync(int tabledoc_sync_state);
    int getRelationshipSync();
    void setRelationshipSync(int tabledoc_sync_state);

    // signal-connected functions
    void onBorderPanelDestructing(kcl::BorderPanel* panel);
    void onColorPanelDestructing(kcl::ColorPanel* panel);
    void onCustomColorsRequested(std::vector<wxColor>& colors);
    void onCustomColorsChanged(std::vector<wxColor> colors);

    void onOpenDataViewFinished(IJobPtr query_job);

private:

    // event handlers

    void onNew(wxCommandEvent& evt);
    void onNewTab(wxCommandEvent& evt);
    void onNewTable(wxCommandEvent& evt);
    void onNewReport(wxCommandEvent& evt);
    void onNewQuery(wxCommandEvent& evt);
    void onNewScript(wxCommandEvent& evt);
    void onStartRecord(wxCommandEvent& evt);
    void onStopRecord(wxCommandEvent& evt);
    void onCreateBookmark(wxCommandEvent& evt);
    void onOpenFile(wxCommandEvent& evt);
    void onOpenDatabase(wxCommandEvent& evt);
    void onOpenLocation(wxCommandEvent& evt);
    void onOpenLocationComplete(wxCommandEvent& evt);
    void onOpenURL(wxCommandEvent& evt);
    void onOpenProject(wxCommandEvent& evt);
    void onCloseProject(wxCommandEvent& evt);
    void onImportData(wxCommandEvent& evt);
    void onExportData(wxCommandEvent& evt);
    void onConnectExternal(wxCommandEvent& evt);
    void onCloseAllChildren(wxCommandEvent& evt);
    void onCloseChild(wxCommandEvent& evt);
    void onStop(wxCommandEvent& evt);
    void onQuit(wxCommandEvent& evt);
    void onAbout(wxCommandEvent& evt);
    void onToggleFullScreen(wxCommandEvent& evt);
    void onToggleLock(wxCommandEvent& evt);
    void onToggleAllPanels(wxCommandEvent& evt);
    void onToggleProjectPanel(wxCommandEvent& evt);
    void onToggleStatusBar(wxCommandEvent& evt);
    void onToggleConsolePanel(wxCommandEvent& evt);
    void onToggleColumnListPanel(wxCommandEvent& evt);
    void onToggleMarkManagerPanel(wxCommandEvent& evt);
    void onToggleRelationshipPanel(wxCommandEvent& evt);
    void onToggleToolbar(wxCommandEvent& evt);
    void onShowFindPanel(wxCommandEvent& evt);
    void onShowColumnListPanel(wxCommandEvent& evt);
    void onShowProjectPanel(wxCommandEvent& evt);
    void onShowConsolePanel(wxCommandEvent& evt);
    void onMergeTable(wxCommandEvent& evt);
    void onSplitTable(wxCommandEvent& evt);
    void onRemoveDupRecs(wxCommandEvent& evt);
    void onDupPayWizard(wxCommandEvent& evt);
    void onDupPayAnalysis(wxCommandEvent& evt);
    void onEditOptions(wxCommandEvent& evt);
    void onHelp(wxCommandEvent& evt);
    void onReference(wxCommandEvent& evt);
    void onResources(wxCommandEvent& evt);
    void onSupport(wxCommandEvent& evt);
    void onContactUs(wxCommandEvent& evt);
    void onHome(wxCommandEvent& evt);
    void onHomeMiddleClick(wxAuiToolBarEvent& evt);
    void onLicenseManager(wxCommandEvent& evt);
    void onCheckForUpdates(wxCommandEvent& evt);
    void onPrint(wxCommandEvent& evt);
    void onTreeRefresh(wxCommandEvent& evt);
    void onPrintConsoleText(wxCommandEvent& evt);
    void onJobScheduler(wxCommandEvent& evt);
    void onJobManager(wxCommandEvent& evt);
    void onExtensionManager(wxCommandEvent& evt);
    void onProjectProperties(wxCommandEvent& evt);
    void onCleanProject(wxCommandEvent& evt);
    void onRelationshipManager(wxCommandEvent& evt);
    void onSetRelationshipSync(wxCommandEvent& evt);
    void onCascade(wxCommandEvent& evt);
    void onTileHorizontal(wxCommandEvent& evt);
    void onTileVertical(wxCommandEvent& evt);
    void onNextChild(wxCommandEvent& evt);
    void onPreviousChild(wxCommandEvent& evt);
    void onCurrentChild(wxCommandEvent& evt);
    void onSetFindCtrlFocus(wxCommandEvent& evt);
    void onFindPrev(wxCommandEvent& evt);
    void onFindNext(wxCommandEvent& evt);
    void onViewChanged(wxCommandEvent& evt);
    void onViewSwitcher(wxCommandEvent& evt);
    void onCutCopyPaste(wxCommandEvent& evt);
    void onBookmarkDropDown(wxAuiToolBarEvent& evt);

    void onViewSwitcherDropDown(wxAuiToolBarEvent& evt);
    void onFillColorDropDown(wxAuiToolBarEvent& evt);
    void onLineColorDropDown(wxAuiToolBarEvent& evt);
    void onTextColorDropDown(wxAuiToolBarEvent& evt);
    void onBorderDropDown(wxAuiToolBarEvent& evt);
    void onRelationshipSyncDropDown(wxAuiToolBarEvent& evt);
    void onToolbarRightClick(wxAuiToolBarEvent& evt);
    
    void onUpdateUI_EnableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_EnableIfNoChildren(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableIfNoChildren(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableIfNoProject(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableIfNoJobs(wxUpdateUIEvent& evt);
    void onUpdateUI_ToggleFullScreen(wxUpdateUIEvent& evt);
    void onUpdateUI_ToggleToolbar(wxUpdateUIEvent& evt);
    void onUpdateUI_ToggleStatusBar(wxUpdateUIEvent& evt);
    void onUpdateUI_ToggleProjectPanel(wxUpdateUIEvent& evt);
    void onUpdateUI_ToggleConsolePanel(wxUpdateUIEvent& evt);
    void onUpdateUI_ToggleColumnListPanel(wxUpdateUIEvent& evt);
    void onUpdateUI_ToggleMarkManagerPanel(wxUpdateUIEvent& evt);
    void onUpdateUI_ToggleRelationshipPanel(wxUpdateUIEvent& evt);
    void onUpdateUI_ToggleLock(wxUpdateUIEvent& evt);
    void onUpdateUI_RelationshipSync(wxUpdateUIEvent& evt);
    void onUpdateUI_ViewSource(wxUpdateUIEvent& evt);
    void onUpdateUI_ViewDesign(wxUpdateUIEvent& evt);
    void onUpdateUI_ViewLayout(wxUpdateUIEvent& evt);
    void onUpdateUI_StartRecord(wxUpdateUIEvent& evt);
    void onUpdateUI_StopRecord(wxUpdateUIEvent& evt);

private:

    void onFrameClose(wxCloseEvent& evt);
    void onFrameSize(wxSizeEvent& evt);
    void onFrameDestroy();
    void onFrameBarRightClick();
    void onFrameEvent(FrameworkEvent& evt);
    void onActiveChildChanged(IDocumentSitePtr doc_site);
    void onFrameChildContextMenu(IDocumentSitePtr doc_site);
    void onStatusBarItemLeftClick(IStatusBarItemPtr item);
    void onStatusBarItemLeftDblClick(IStatusBarItemPtr item);
    void onStatusBarRefresh();
    
    bool checkForRunningJobs(bool exit_message = false);
    bool checkForTemporaryFiles();
    void doViewSwitcher(bool drop_down_menu);

private:

    IFramePtr m_frame;
    IDocumentSitePtr m_columnlistpanel_site;
    IDocumentSitePtr m_markmanagerpanel_site;
    
    IStatusBarItemPtr m_doc_text_item;
    IStatusBarItemPtr m_job_text_item;
    IStatusBarItemPtr m_job_separator_item;
    
    DbDoc* m_dbdoc;
    StandardToolbar* m_project_toolbar;
    FormatToolbar* m_format_toolbar;
    LinkBar* m_linkbar;
    JobGaugeUpdateTimer* m_job_update_timer;
    RelDiagramWatcher* m_rel_diagram_watcher;
    ScriptCommandEventRouter* m_script_command_router;

    wxMenu* m_view_menu;    // stored so we can insert/remove items
    wxString m_frame_caption;

    BorderPanelHelper* m_borderpanel_helper;  // helper for border panel popups    
    ColorPanelHelper* m_colorpanel_helper;  // helper for color panel popups
    
    std::vector<HiddenSiteInfo> m_hidden_sites;
    int m_relationship_sync;
    bool m_data_locked;

    DECLARE_EVENT_TABLE()
};



#endif


