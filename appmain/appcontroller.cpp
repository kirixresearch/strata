/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-03-14
 *
 */


//#define USE_XDSL


#include "appmain.h"
#include "apphook.h"
#include "appcontroller.h"
#include "bookmark.h"
#include "scripthost.h"
#include "scriptapp.h"
#include "dlgprojectmgr.h"
#include "dlgdatabasefile.h"
#include "extensionmgr.h"
#include "extensionpkg.h"
#include "moduleremoveduprec.h"
#include "reportdoc.h"
#include "importwizard.h"
#include "exportwizard.h"
#include "connectionwizard.h"
#include "paneloptions.h"
#include "panelmerge.h"
#include "panelsplit.h"
#include "paneljobscheduler.h"
#include "paneldatabaseinfo.h"
#include "panelrelationship.h"
#include "panelcolumnlist.h"
#include "panelmarkmgr.h"
#include "panelfind.h"
#include "panelconsole.h"
#include "paneljobmgr.h"
#include "panelextensionmgr.h"
#include "tabledoc.h"
#include "editordoc.h"
#include "jobscript.h"
#include "jobscheduler.h"
#include "jobimport.h"
#include "jobimportpkg.h"
#include "jobexportpkg.h"
#include "jsonconfig.h"
#include "querydoc.h"
#include "sqldoc.h"
#include "toolbars.h"
#include "textdoc.h"
#include "webdoc.h"
#include "structuredoc.h"
#include "transformationdoc.h"
#include "../paladin/paladin.h"
#include "dlgauth.h"
#include "dlgcustomprompt.h"
#include "inetauth.h"
#include "updater.h"
#include <wx/artprov.h>
#include <wx/mimetype.h>
#include <wx/fs_inet.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/choicdlg.h>
#include <wx/stopwatch.h>
#include <wx/paper.h>
#include <wx/print.h>
#include <kl/utf8.h>
#include <kl/thread.h>
#include "dbdoc.h"
#include "linkbar.h"
#include "dlglinkprops.h"
#include "pkgfile.h"


extern paladin::Authentication* g_auth;


enum
{
    ID_TreeRefresh = 19409,
    ID_LinkBarRefresh,
    ID_PrintConsoleText
};


enum
{
    // incrementing this number will cause perspective strings from
    // older version to NOT be loaded -- this is necessary when new
    // toolbars are added that are displayed be default, or default sizes
    // have been changed.  Incrementing this number will cause everybody's
    // UI to be reset to the default layout.
    ClearPerspectiveCounter = 24
};


BEGIN_EVENT_TABLE(AppController, wxEvtHandler)
    EVT_MENU(ID_Project_New, AppController::onNew)
    EVT_MENU(ID_Project_NewTab, AppController::onNewTab)
    EVT_MENU(ID_Project_NewTable, AppController::onNewTable)
    EVT_MENU(ID_Project_NewReport, AppController::onNewReport)
    EVT_MENU(ID_Project_NewQuery, AppController::onNewQuery)
    EVT_MENU(ID_Project_NewScript, AppController::onNewScript)
    EVT_MENU(ID_Project_OpenFile, AppController::onOpenFile)
    EVT_MENU(ID_Project_OpenDatabase, AppController::onOpenDatabase)
    EVT_MENU(ID_Project_OpenLocation, AppController::onOpenLocation)
    EVT_MENU(ID_Project_OpenLocationComplete, AppController::onOpenLocationComplete)
    EVT_MENU(ID_Project_RemoveDupRecs, AppController::onRemoveDupRecs)
    EVT_MENU(ID_App_OpenProject, AppController::onOpenProject)
    EVT_MENU(ID_Project_CloseProject, AppController::onCloseProject)
    EVT_MENU(ID_File_Close, AppController::onCloseChild)
    EVT_MENU(ID_File_CloseAll, AppController::onCloseAllChildren)
    EVT_MENU(ID_File_Stop, AppController::onStop)
    EVT_MENU(ID_App_StartRecord, AppController::onStartRecord)
    EVT_MENU(ID_App_StopRecord, AppController::onStopRecord)
    EVT_MENU(ID_App_Exit, AppController::onQuit)
    EVT_MENU(ID_App_About, AppController::onAbout)
    EVT_MENU(ID_View_ViewSwitcher, AppController::onViewSwitcher)
    EVT_MENU(ID_View_SwitchToSourceView, AppController::onViewChanged)
    EVT_MENU(ID_View_SwitchToDesignView, AppController::onViewChanged)
    EVT_MENU(ID_View_SwitchToLayoutView, AppController::onViewChanged)
    EVT_MENU(ID_Window_Cascade, AppController::onCascade)
    EVT_MENU(ID_Window_TileHorizontal, AppController::onTileHorizontal)
    EVT_MENU(ID_Window_TileVertical, AppController::onTileVertical)
    EVT_MENU(ID_Window_Next, AppController::onNextChild)
    EVT_MENU(ID_Window_Previous, AppController::onPreviousChild)
    EVT_MENU(ID_Window_Current, AppController::onCurrentChild)
    EVT_MENU(ID_Project_Help, AppController::onHelp)
    EVT_MENU(ID_Project_Reference, AppController::onReference)
    EVT_MENU(ID_Project_Resources, AppController::onResources)
    EVT_MENU(ID_Project_Support, AppController::onSupport)
    EVT_MENU(ID_Project_ContactUs, AppController::onContactUs)
    EVT_MENU(ID_Project_Home, AppController::onHome)
    EVT_AUITOOLBAR_MIDDLE_CLICK(ID_Project_Home, AppController::onHomeMiddleClick)
    EVT_MENU(ID_Frame_FindCtrlFocus, AppController::onSetFindCtrlFocus)
    EVT_MENU(ID_Edit_Cut, AppController::onCutCopyPaste)
    EVT_MENU(ID_Edit_Copy, AppController::onCutCopyPaste)
    EVT_MENU(ID_Edit_Paste, AppController::onCutCopyPaste)
    EVT_MENU(ID_Edit_Undo, AppController::onCutCopyPaste)
    EVT_MENU(ID_Edit_Redo, AppController::onCutCopyPaste)
    EVT_MENU(ID_Edit_Find, AppController::onShowFindPanel)
    EVT_MENU(ID_Edit_FindInFiles, AppController::onShowFindPanel)
    EVT_MENU(ID_Edit_Replace, AppController::onShowFindPanel)
    EVT_MENU(ID_Edit_FindPrev, AppController::onFindPrev)
    EVT_MENU(ID_Edit_FindNext, AppController::onFindNext)
    EVT_MENU(ID_File_Bookmark, AppController::onCreateBookmark) 
    EVT_MENU(ID_Frame_ShowColumnList, AppController::onShowColumnListPanel)
    EVT_MENU(ID_Frame_ShowProjectPanel, AppController::onShowProjectPanel)
    EVT_MENU(ID_Frame_ShowConsolePanel, AppController::onShowConsolePanel)
    EVT_MENU(ID_Frame_ToggleProjectPanel, AppController::onToggleProjectPanel)
    EVT_MENU(ID_Frame_ToggleConsolePanel, AppController::onToggleConsolePanel)
    EVT_MENU(ID_Frame_ToggleColumnListPanel, AppController::onToggleColumnListPanel)
    EVT_MENU(ID_Frame_ToggleMarkManagerPanel, AppController::onToggleMarkManagerPanel)
    EVT_MENU(ID_Frame_ToggleRelationshipManagerPanel, AppController::onToggleRelationshipPanel)
    EVT_MENU(ID_Frame_ToggleStandardToolbar, AppController::onToggleToolbar)
    EVT_MENU(ID_Frame_ToggleLinkToolbar, AppController::onToggleToolbar)
    EVT_MENU(ID_Frame_ToggleFormatToolbar, AppController::onToggleToolbar)
    EVT_MENU(ID_Frame_ToggleStatusBar, AppController::onToggleStatusBar)    
    EVT_MENU(ID_Frame_ToggleFullScreen, AppController::onToggleFullScreen)
    EVT_MENU(ID_App_ToggleProtect, AppController::onToggleLock)
    EVT_MENU(ID_Project_Import, AppController::onImportData)
    EVT_MENU(ID_Project_Export, AppController::onExportData)
    EVT_MENU(ID_Project_ConnectExternal, AppController::onConnectExternal)
    EVT_MENU(ID_App_JobScheduler, AppController::onJobScheduler)
    EVT_MENU(ID_App_JobManager, AppController::onJobManager)
    EVT_MENU(ID_App_ExtensionManager, AppController::onExtensionManager)
    EVT_MENU(ID_Project_ProjectProperties, AppController::onProjectProperties)
    EVT_MENU(ID_Project_CleanProject, AppController::onCleanProject)    
    EVT_MENU(ID_Project_Relationship, AppController::onRelationshipManager)
    EVT_MENU(ID_App_RelationshipSyncNone, AppController::onSetRelationshipSync)
    EVT_MENU(ID_App_RelationshipSyncFilter, AppController::onSetRelationshipSync)
    EVT_MENU(ID_App_RelationshipSyncSeek, AppController::onSetRelationshipSync)
    EVT_MENU(ID_Project_Merge, AppController::onMergeTable)
    EVT_MENU(ID_Project_Split, AppController::onSplitTable)
    EVT_MENU(ID_App_Options, AppController::onEditOptions)
    EVT_MENU(ID_App_LicenseManager, AppController::onLicenseManager)
    EVT_MENU(ID_App_CheckForUpdates, AppController::onCheckForUpdates)
    EVT_MENU(ID_TreeRefresh, AppController::onTreeRefresh)
    EVT_MENU(ID_LinkBarRefresh, AppController::onTreeRefresh)
    EVT_MENU(ID_PrintConsoleText, AppController::onPrintConsoleText)

    //EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_File_Bookmark, AppController::onBookmarkDropDown)
    EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_View_ViewSwitcher, AppController::onViewSwitcherDropDown)
    EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_Format_FillColor, AppController::onFillColorDropDown)
    EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_Format_LineColor, AppController::onLineColorDropDown)
    EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_Format_TextColor, AppController::onTextColorDropDown)
    EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_Format_Border, AppController::onBorderDropDown)
    EVT_AUITOOLBAR_TOOL_DROPDOWN(ID_App_ToggleRelationshipSync, AppController::onRelationshipSyncDropDown)
    EVT_AUITOOLBAR_RIGHT_CLICK(wxID_ANY, AppController::onToolbarRightClick)

    EVT_COMBOBOX(ID_Frame_UrlCtrl, AppController::onOpenURL)
    EVT_TEXT_ENTER(ID_Frame_UrlCtrl, AppController::onOpenURL)

    // update the frame commands; make sure the specific cases
    // precede the general update range
    EVT_UPDATE_UI(ID_Frame_ToggleFullScreen, AppController::onUpdateUI_ToggleFullScreen)
    EVT_UPDATE_UI(ID_Frame_ToggleStatusBar, AppController::onUpdateUI_ToggleStatusBar)
    EVT_UPDATE_UI(ID_Frame_ToggleProjectPanel, AppController::onUpdateUI_ToggleProjectPanel)    
    EVT_UPDATE_UI(ID_Frame_ToggleConsolePanel, AppController::onUpdateUI_ToggleConsolePanel)
    EVT_UPDATE_UI(ID_Frame_ToggleColumnListPanel, AppController::onUpdateUI_ToggleColumnListPanel)
    EVT_UPDATE_UI(ID_Frame_ToggleMarkManagerPanel, AppController::onUpdateUI_ToggleMarkManagerPanel)
    EVT_UPDATE_UI(ID_Frame_ToggleRelationshipManagerPanel, AppController::onUpdateUI_ToggleRelationshipPanel)
    EVT_UPDATE_UI(ID_Frame_ToggleStandardToolbar, AppController::onUpdateUI_ToggleToolbar)
    EVT_UPDATE_UI(ID_Frame_ToggleLinkToolbar, AppController::onUpdateUI_ToggleToolbar)    
    EVT_UPDATE_UI(ID_Frame_ToggleFormatToolbar, AppController::onUpdateUI_ToggleToolbar)
    EVT_UPDATE_UI_RANGE(ID_Frame_First, ID_Frame_Last, AppController::onUpdateUI_EnableAlways)

    // update the macro recording menu item
    EVT_UPDATE_UI(ID_App_StartRecord, AppController::onUpdateUI_StartRecord)        
    EVT_UPDATE_UI(ID_App_StopRecord, AppController::onUpdateUI_StopRecord)

    // update application commands; make sure the specific cases
    // precede the general update range
    EVT_UPDATE_UI(ID_App_ToggleProtect, AppController::onUpdateUI_ToggleLock)
    EVT_UPDATE_UI(ID_App_RelationshipSyncNone, AppController::onUpdateUI_RelationshipSync)
    EVT_UPDATE_UI(ID_App_RelationshipSyncFilter, AppController::onUpdateUI_RelationshipSync)  
    EVT_UPDATE_UI(ID_App_RelationshipSyncSeek, AppController::onUpdateUI_RelationshipSync)
    EVT_UPDATE_UI_RANGE(ID_App_First, ID_App_Last, AppController::onUpdateUI_EnableAlways)
    
    // disable the project commands if there isn't a project
    EVT_UPDATE_UI_RANGE(ID_Project_First, ID_Project_Last, AppController::onUpdateUI_DisableIfNoProject)

    // disable the special utilities if there isn't a project
    //EVT_UPDATE_UI_RANGE(ID_Special_First, ID_Special_Last, AppController::onUpdateUI_DisableIfNoProject)

    // disable the file commands if there's isn't a child
    EVT_UPDATE_UI(ID_File_Stop, AppController::onUpdateUI_DisableIfNoJobs)
    EVT_UPDATE_UI_RANGE(ID_File_First, ID_File_Last, AppController::onUpdateUI_DisableIfNoChildren)

    // check current view in the view menu
    EVT_UPDATE_UI(ID_View_SwitchToSourceView, AppController::onUpdateUI_ViewSource)
    EVT_UPDATE_UI(ID_View_SwitchToDesignView, AppController::onUpdateUI_ViewDesign)
    EVT_UPDATE_UI(ID_View_SwitchToLayoutView, AppController::onUpdateUI_ViewLayout)
    
    // disable the view commands if there's isn't a child
    EVT_UPDATE_UI_RANGE(ID_View_First, ID_View_Last, AppController::onUpdateUI_DisableIfNoChildren)

    // disable the window commands if there's isn't a child
    EVT_UPDATE_UI_RANGE(ID_Window_First, ID_Window_Last, AppController::onUpdateUI_DisableIfNoChildren)

    // disable the edit commands if there's isn't a child
    EVT_UPDATE_UI_RANGE(ID_Edit_Delete, ID_Edit_Last, AppController::onUpdateUI_DisableIfNoChildren)

    // disable the data commands if there's isn't a child 
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, AppController::onUpdateUI_DisableIfNoChildren)

    // disable the tablecommands if there's isn't a child
    EVT_UPDATE_UI_RANGE(ID_Table_First, ID_Table_Last, AppController::onUpdateUI_DisableIfNoChildren)

    // disable the data commands if there's isn't a child
    EVT_UPDATE_UI_RANGE(ID_Canvas_First, ID_Canvas_Last, AppController::onUpdateUI_DisableIfNoChildren)

    // disable the data commands if there's isn't a child
    EVT_UPDATE_UI_RANGE(ID_Format_First, ID_Format_Last, AppController::onUpdateUI_DisableIfNoChildren)
    

    // explicity disable the save, save as and bookmark if we don't have a project; this will
    // allow us to use the webdoc without the project, if we chose, but only be able to
    // save externally; TODO: this is mostly working, except with the web tables, which 
    // require a project, so if we want to allow this safely, we'll have to disable tables 
    // in the webdoc, since these require a project
    EVT_UPDATE_UI(ID_File_Save, AppController::onUpdateUI_DisableIfNoProject)
    EVT_UPDATE_UI(ID_File_SaveAs, AppController::onUpdateUI_DisableIfNoProject)
    EVT_UPDATE_UI(ID_File_Bookmark, AppController::onUpdateUI_DisableIfNoProject)
    
    // allow cut/copy/paste for dbdoc
    EVT_UPDATE_UI(ID_Edit_Cut, AppController::onUpdateUI_EnableIfNoChildren)
    EVT_UPDATE_UI(ID_Edit_Copy, AppController::onUpdateUI_EnableIfNoChildren)
    EVT_UPDATE_UI(ID_Edit_Paste, AppController::onUpdateUI_EnableIfNoChildren)
END_EVENT_TABLE()


static void postBorderSelectedEvent(IDocumentSitePtr active_child,
                                    wxAuiToolBarItem* item)
{
    // send the selected border to the active document through a wx event
    
    if (active_child.isNull())
        return;

    if (item == NULL)
        return;
        
    IDocumentPtr doc = active_child->getDocument();
    if (doc.isOk())
    {
        wxWindow* w = doc->getDocumentWindow();
        if (w != NULL)
        {
            // post a wx event for the active document to process -- 
            // the selected border is sent as a delimited list (;) of 
            // borders and properties (e.g. "border.bottom.style:border.style.none;..."
            wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, item->GetId());

            wxString border = wxEmptyString;
            int id = item->GetUserData();

            switch (id)
            {
                default:
                    {
                        // TODO: "default" is selected when the main toolbar icon is clicked; right 
                        // now this is hardwired to a bottom border because this is what's on the 
                        // toolbar item; however, the toolbar icon should change based on the last 
                        // selected item, like the color drop down, so that the default isn't always 
                        // a bottom border
                        border.Append(kcanvas::PROP_BORDER_BOTTOM_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;                
                
                case kcl::BorderPanel::border_noborder:
                    {
                        border.Append(kcanvas::PROP_BORDER_STYLE + wxT(":") + kcanvas::BORDER_STYLE_NONE);
                    }
                    break;

                case kcl::BorderPanel::border_bottom:
                    {
                        border.Append(kcanvas::PROP_BORDER_BOTTOM_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;

                case kcl::BorderPanel::border_left:
                    {
                        border.Append(kcanvas::PROP_BORDER_LEFT_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;

                case kcl::BorderPanel::border_right:
                    {
                        border.Append(kcanvas::PROP_BORDER_RIGHT_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;
/*
                case kcl::BorderPanel::border_bottom_dbl:
                    {
                        border.Append(kcanvas::PROP_BORDER_BOTTOM_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;

                case kcl::BorderPanel::border_bottom_thick:
                    {
                        border.Append(kcanvas::PROP_BORDER_BOTTOM_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;
*/
                case kcl::BorderPanel::border_bottom_top:
                    {
                        border.Append(kcanvas::PROP_BORDER_BOTTOM_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                        border.Append(wxT(";"));
                        border.Append(kcanvas::PROP_BORDER_TOP_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;
/*
                case kcl::BorderPanel::border_bottom_dbl_t:
                    {
                        border.Append(kcanvas::PROP_BORDER_BOTTOM_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;

                case kcl::BorderPanel::border_bottom_tk_t:
                    {
                        border.Append(kcanvas::PROP_BORDER_BOTTOM_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;

                case kcl::BorderPanel::borders_all:
                    {
                        border.Append(kcanvas::PROP_BORDER_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;
*/
                case kcl::BorderPanel::border_outside:
                    {
                        border.Append(kcanvas::PROP_BORDER_TOP_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                        border.Append(wxT(";"));                    
                        border.Append(kcanvas::PROP_BORDER_BOTTOM_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                        border.Append(wxT(";"));
                        border.Append(kcanvas::PROP_BORDER_LEFT_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                        border.Append(wxT(";"));
                        border.Append(kcanvas::PROP_BORDER_RIGHT_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }                
                    break;
/*
                case kcl::BorderPanel::border_thick_out:
                    {
                        border.Append(kcanvas::PROP_BORDER_TOP_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                        border.Append(wxT(";"));
                        border.Append(kcanvas::PROP_BORDER_BOTTOM_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                        border.Append(wxT(";"));
                        border.Append(kcanvas::PROP_BORDER_LEFT_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                        border.Append(wxT(";"));
                        border.Append(kcanvas::PROP_BORDER_RIGHT_STYLE + wxT(":") + kcanvas::BORDER_STYLE_SOLID);
                    }
                    break;
*/                    
            }
            
            evt.SetString(border);
            ::wxPostEvent(w, evt);
        }
    }
}

static void postColorSelectedEvent(IDocumentSitePtr active_child,
                                   wxAuiToolBarItem* item)
{
    // send the selected color to the active document through a wx event
    
    if (active_child.isNull())
        return;

    if (item == NULL)
        return;
        
    IDocumentPtr doc = active_child->getDocument();
    if (doc.isOk())
    {
        wxWindow* w = doc->getDocumentWindow();
        if (w != NULL)
        {
            // post a wx event for the active document to
            // process -- the selected color is passed as
            // an untyped extra long (to be unpacked later)
            wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, item->GetId());
            evt.SetExtraLong(item->GetUserData());
            ::wxPostEvent(w, evt);
        }
    }
}

class BorderPanelHelper : public xcm::signal_sink
{
public:

    BorderPanelHelper(const wxPoint& pt,
                     int anchor_corner,
                     wxAuiToolBar* toolbar = NULL,
                     int tool_id = -1)
    {
        m_toolbar = toolbar;
        m_tool_id = tool_id;
        
        if (m_toolbar != NULL)
            m_toolbar->SetToolSticky(m_tool_id, true);
        
        m_container = new kcl::PopupContainer(pt, anchor_corner);
        
        kcl::BorderPanel* panel = new kcl::BorderPanel(m_container);

        panel->sigBorderSelected.connect(this,
                              &BorderPanelHelper::onBorderPanelBorderSelected);
        panel->sigDestructing.connect(g_app->getAppController(),
                              &AppController::onBorderPanelDestructing);
        m_container->doPopup(panel);
    }
    
    ~BorderPanelHelper()
    {
        if (m_toolbar != NULL && m_tool_id != -1)
            m_toolbar->SetToolSticky(m_tool_id, false);
    }
    
    void onBorderPanelBorderSelected(int id)
    {
        if (m_toolbar == NULL || m_tool_id == -1)
            return;

        // find the toolbar item we clicked
        wxAuiToolBarItem* item = m_toolbar->FindTool(m_tool_id);
        
        // pass the id
        item->SetUserData((long)id);

        m_container->Show(false);
        m_container->Destroy();

        postBorderSelectedEvent(g_app->getMainFrame()->getActiveChild(), item);
    }

public:

    kcl::PopupContainer* m_container;
    
    wxAuiToolBar* m_toolbar;
    int m_tool_id;
};


class ColorPanelHelper : public xcm::signal_sink
{
public:

    ColorPanelHelper(const wxPoint& pt,
                     int anchor_corner,
                     int mode,
                     const kcl::ColorPanelLayout& layout,
                     const wxColour& default_color = *wxBLACK,
                     wxAuiToolBar* toolbar = NULL,
                     int tool_id = -1)
    {
        m_toolbar = toolbar;
        m_tool_id = tool_id;
        
        if (m_toolbar != NULL)
            m_toolbar->SetToolSticky(m_tool_id, true);
        
        m_container = new kcl::PopupContainer(pt, anchor_corner);
        
        kcl::ColorPanel* panel = new kcl::ColorPanel(m_container, layout);
        panel->setDefaultColor(default_color);
        panel->setMode(mode);
        panel->sigColorSelected.connect(this,
                              &ColorPanelHelper::onColorPanelColorSelected);
        panel->sigDestructing.connect(g_app->getAppController(),
                              &AppController::onColorPanelDestructing);
        panel->sigCustomColorsRequested.connect(g_app->getAppController(),
                              &AppController::onCustomColorsRequested);
        panel->sigCustomColorsChanged.connect(g_app->getAppController(),
                              &AppController::onCustomColorsChanged);
        m_container->doPopup(panel);
    }
    
    ~ColorPanelHelper()
    {
        if (m_toolbar != NULL && m_tool_id != -1)
            m_toolbar->SetToolSticky(m_tool_id, false);
    }
    
    void onColorPanelColorSelected(wxColor color, int mode)
    {
        if (m_toolbar == NULL || m_tool_id == -1)
            return;

        // find the toolbar item we clicked
        wxAuiToolBarItem* item = m_toolbar->FindTool(m_tool_id);
        
        // either create or update the user-data with the selected color
        if (item->GetUserData())
        {
            wxColor* c = (wxColor*)(item->GetUserData());
            *c = color;
        }
         else
        {
            item->SetUserData((long)(new wxColor(color)));
        }
        
        m_container->Show(false);
        m_container->Destroy();

        postColorSelectedEvent(g_app->getMainFrame()->getActiveChild(), item);
    }

public:

    kcl::PopupContainer* m_container;
    
    wxAuiToolBar* m_toolbar;
    int m_tool_id;
};




// -- AppController class implementation --

AppController::AppController()
{
    m_dbdoc = NULL;
    m_relationship_sync = tabledocRelationshipSyncNone;
    m_rel_diagram_watcher = NULL;
    m_job_update_timer = NULL;
    m_project_toolbar = NULL;
    m_linkbar = NULL;
    m_view_menu = NULL;
    m_script_command_router = NULL;
    m_data_locked = true;
    m_frame_caption = APPLICATION_NAME;
}

AppController::~AppController()
{        
    delete m_script_command_router;
    delete m_rel_diagram_watcher;
        
    if (m_job_update_timer)
    {
        m_job_update_timer->UnInit();
        delete m_job_update_timer;
    }
}


bool AppController::checkLicense(bool show_warnings)
{
    if (g_auth->checkAuth() != paladin::errNone)
    {
        // if authentication failed, try to install
        // a license to let the user evaluate the product
        // if this fails,  open the license
        // manager and give the user a chance to reauthenticate 

        #if APP_TRIAL_LICENSE_DAYS > 0
        g_auth->installLicense(APP_TRIAL_LICENSE_DAYS);
        #endif
        
        if (g_auth->checkAuth() != paladin::errNone)
            showLicenseManager();

        // if the last step failed, definitively return an error
        if (g_auth->checkAuth() != paladin::errNone)
            return false;
    }

    return true;
}


void AppController::checkForUpdates(bool full_gui)
{
    Updater::checkForUpdates(full_gui);
}

bool AppController::init()
{
    #if APP_CONSOLE==1
    return true;
    #endif

    IAppPreferencesPtr prefs = g_app->getAppPreferences();

    // get saved windows dimensions
    long x, y, w, h, screen_x, screen_y;
    bool maximized;
    std::wstring perspective;
    long clear_perspective_counter = 0;
    
    screen_x = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
    screen_y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
    screen_y -= getTaskBarHeight();

    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/WindowDimensions");
    config->read(L"XPos",                    &x,                         0);
    config->read(L"YPos",                    &y,                         0);
    config->read(L"Width",                   &w,                         screen_x);
    config->read(L"Height",                  &h,                         screen_y);
    config->read(L"Maximized",               &maximized,                 false);
    config->read(L"MainPerspective",         perspective,                L"");
    config->read(L"ClearPerspectiveCounter", &clear_perspective_counter, 0);


    // if there is a new perspective version out there,
    // don't use any existing one
    if (clear_perspective_counter < ClearPerspectiveCounter)
        perspective = L"";

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

    // set frame caption value
    if (g_app->isServiceConfig())
    {
        m_frame_caption = APPLICATION_NAME;
        m_frame_caption += wxT(" [Service Configuration]");
    }
     else
    {
        m_frame_caption = APPLICATION_NAME;
    }

    // must be initialized to NULL (we check these often)
    m_borderpanel_helper = NULL;    
    m_colorpanel_helper = NULL;

    // create the main frame
    m_frame = createFrameObject();
    if (m_frame.isNull())
        return false;
    
    m_frame->create(NULL, m_frame_caption, x, y, w, h);

#if defined(__WXMSW__)

    // load icon from windows resource
    wxIcon icon1(wxT("AA_APPICON_9"), wxBITMAP_TYPE_ICO_RESOURCE, 32, 32);
    wxIcon icon2(wxT("AA_APPICON_9"), wxBITMAP_TYPE_ICO_RESOURCE, 16, 16);
    wxIconBundle icons;
    icons.AddIcon(icon1);
    icons.AddIcon(icon2);
    m_frame->setIcons(icons);

#else

    // TODO: application icon for non-msw platforms
    
    //wxBitmap bmp_logo;
    //bmp_logo = GETBMP(logo_16);
    //wxIcon logo;
    //logo.CopyFromBitmap(bmp_logo);
    //m_frame->setIcon(logo);
    
#endif



    m_script_command_router = new ScriptCommandEventRouter;

    #if APP_CONSOLE==0
    g_app->SetTopWindow(m_frame->getFrameWindow());
    #endif
    
    g_app->setMainFrame(m_frame);

    m_frame->addWindowEventHandler(this);
    m_frame->addWindowEventHandler(m_script_command_router);

    // allow additional custom hook functionality
    apphookInitFrame();
    
    m_frame->sigFrameClose().connect(this, &AppController::onFrameClose);
    m_frame->sigFrameSize().connect(this, &AppController::onFrameSize);
    m_frame->sigFrameDestroy().connect(this, &AppController::onFrameDestroy);
    m_frame->sigFrameBarRightClick().connect(this, &AppController::onFrameBarRightClick);
    m_frame->sigFrameEvent().connect(this, &AppController::onFrameEvent);
    m_frame->sigActiveChildChanged().connect(this, &AppController::onActiveChildChanged);
    m_frame->sigSiteContextMenu().connect(this, &AppController::onFrameChildContextMenu);
    
    
    // add accelerators -- these allow certain functions to be accessed
    // without being represented on the menu; we create it here since
    // menu accelerators add onto the existing list of accelerators
    // NOTE: first parameter is a flag field indicating the state of 
    // the ALT, SHIFT, and CTRL keys; one of wxACCEL_ALT, wxACCEL_SHIFT, 
    // wxACCEL_CTRL or wxACCEL_NORMAL
    wxAcceleratorEntry entries[31];

    entries[0].Set(wxACCEL_SHIFT, WXK_DELETE, ID_Edit_Cut);   // alternate cut accelerator
    entries[1].Set(wxACCEL_CTRL, WXK_INSERT, ID_Edit_Copy);   // alternate copy accelerator
    entries[2].Set(wxACCEL_SHIFT, WXK_INSERT, ID_Edit_Paste); // alternate paste accelerator
    entries[3].Set(wxACCEL_NORMAL, WXK_F5, ID_File_Reload);   // alternate refresh accelerator
    entries[4].Set(wxACCEL_CTRL, (int)'A', ID_Edit_SelectAll);
    entries[5].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)'Z', ID_Edit_Redo);
    entries[6].Set(wxACCEL_CTRL, (int)'/', ID_Frame_FindCtrlFocus);
    entries[7].Set(wxACCEL_CTRL, (int)'L', ID_Project_OpenLocation);
    entries[8].Set(wxACCEL_CTRL, WXK_RETURN, ID_Project_OpenLocationComplete);
    entries[9].Set(wxACCEL_CTRL, WXK_TAB, ID_Window_Next);
    entries[10].Set(wxACCEL_CTRL, WXK_PAGEDOWN, ID_Window_Next);    // alternate next
    entries[11].Set(wxACCEL_CTRL, WXK_TAB, ID_Window_Previous);
    entries[12].Set(wxACCEL_CTRL, WXK_PAGEUP, ID_Window_Previous);  // alternate previous
    entries[13].Set(wxACCEL_CTRL, WXK_NUMPAD_ADD, ID_View_ZoomIn);
    entries[14].Set(wxACCEL_CTRL, WXK_NUMPAD_SUBTRACT, ID_View_ZoomOut);
    entries[15].Set(wxACCEL_NORMAL, WXK_F3, ID_Edit_FindNext);
    entries[16].Set(wxACCEL_SHIFT, WXK_F3, ID_Edit_FindPrev);
    entries[17].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)'G', ID_Edit_FindPrev);
    entries[18].Set(wxACCEL_ALT, WXK_LEFT, ID_File_Back);
    entries[19].Set(wxACCEL_ALT, WXK_RIGHT, ID_File_Forward);
    entries[20].Set(wxACCEL_CTRL, (int)'U', ID_View_ViewSwitcher);  // Ctrl-U cycles through the views
    entries[21].Set(wxACCEL_CTRL, WXK_CANCEL, ID_File_Stop);        // Ctrl-Break to stop reload/cancel jobs
    entries[22].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)'F', ID_Edit_FindInFiles);

    // following accelerators are designed to allow a user to jump to the project
    // panel, the console panel, or the child window without the panel toggling;
    // so, whereas Ctrl-B will toggle the project panel, Ctrl-Shift-B will only
    // open it and set the focus if it's closed; similarly with Ctrl-Shift-K for
    // the console panel; Ctrl-(Shift)-I moves the focus back to the child window    
    entries[23].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)'I', ID_Window_Current);
    entries[24].Set(wxACCEL_CTRL, (int)'I', ID_Window_Current);                             // alternate for setting focus to content    
    entries[25].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)'B', ID_Frame_ShowProjectPanel);
    entries[26].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)'K', ID_Frame_ShowConsolePanel);
    entries[27].Set(wxACCEL_CTRL, (int)';', ID_Frame_ShowConsolePanel);                     // alternate for show console panel
    entries[28].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)';', ID_Frame_ShowConsolePanel);     // alternate for show console panel


    // TODO: experimental accelerators for startings/stopping recording;
    // currently, these functions are for development only and not available 
    // in the menu
    entries[29].Set(wxACCEL_ALT, WXK_F1, ID_App_StartRecord);
    entries[30].Set(wxACCEL_ALT, WXK_F2, ID_App_StopRecord);


    wxAcceleratorTable accel(31, entries);
    m_frame->getFrameWindow()->SetAcceleratorTable(accel);

    // create menus
    wxMenu* submenuNew = new wxMenu;
    submenuNew->Append(ID_Project_NewTab, _("&Tab\tCtrl+T"));
    submenuNew->Append(ID_Project_NewTable, _("T&able"));
    submenuNew->Append(ID_Project_NewReport, _("&Report"));
    submenuNew->Append(ID_Project_NewQuery, _("&Query"));
    submenuNew->Append(ID_Project_NewScript, _("&Script"));
    
    wxMenu* menuFile = new wxMenu;
    menuFile->AppendSubMenu(submenuNew, _("&New"));
    menuFile->Append(ID_Project_OpenFile, _("&Open...\tCtrl+O"));
    menuFile->Append(ID_File_Close, _("&Close\tCtrl+W"));
    menuFile->AppendSeparator();
    menuFile->Append(ID_File_Save, _("&Save\tCtrl+S"));
    menuFile->Append(ID_File_SaveAs, _("Save &As..."));
    menuFile->Append(ID_File_SaveAsExternal, _("Save As Ex&ternal..."));
    menuFile->AppendSeparator();
    menuFile->Append(ID_File_Bookmark, _("&Bookmark This Location...\tCtrl+D"));
    menuFile->Append(ID_File_Reload, _("Refres&h\tCtrl+R"));
    menuFile->AppendSeparator();
    menuFile->Append(ID_Project_Import, _("&Import..."));
    menuFile->Append(ID_Project_Export, _("&Export..."));
    menuFile->Append(ID_Project_OpenDatabase, _("Create Co&nnection..."));
    menuFile->AppendSeparator();
    menuFile->Append(ID_App_OpenProject, _("P&rojects..."));
    menuFile->AppendSeparator();
    menuFile->Append(ID_File_PageSetup, _("Page Set&up..."));
    menuFile->Append(ID_File_Print, _("&Print...\tCtrl+P"));
    menuFile->AppendSeparator();
    menuFile->Append(ID_App_Exit, _("E&xit"));

    wxMenu* menuEdit = new wxMenu;
    menuEdit->Append(ID_Edit_Undo, _("&Undo\tCtrl+Z"));
    menuEdit->Append(ID_Edit_Redo, _("&Redo\tCtrl+Y"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_Edit_Cut, _("Cu&t\tCtrl+X"));
    menuEdit->Append(ID_Edit_Copy, _("&Copy\tCtrl+C"));
    menuEdit->Append(ID_Edit_Paste, _("&Paste\tCtrl+V"));
    menuEdit->Append(ID_Edit_Delete, _("&Delete"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_Edit_SelectAll, _("Select &All\tCtrl+A"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_Edit_Find, _("&Find...\tCtrl+F"));
    menuEdit->Append(ID_Edit_Replace, _("R&eplace...\tCtrl+H"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_Edit_GoTo, _("&Go To...\tCtrl+G"));
    menuEdit->AppendSeparator();
    menuEdit->AppendCheckItem(ID_App_ToggleProtect, _("Pr&otect Data"));

    // note: the accelerators for next/previous tab are also specified
    // in the accelerator table above; these will supercede those, so
    // this won't cause any problem; however, if we decide we don't want
    // the tab submenu, we can simply comment it out, and the hotkeys for 
    // tabs, which we definitely want, will still work
    wxMenu* menuViewTabs = new wxMenu;

    menuViewTabs->Append(ID_Window_Next, _("&Next\tCtrl+Tab"));
    menuViewTabs->Append(ID_Window_Previous, _("&Previous\tCtrl+Shift+Tab"));
    menuViewTabs->AppendSeparator();
    menuViewTabs->Append(ID_Window_TileHorizontal, _("New &Horizontal Group"));
    menuViewTabs->Append(ID_Window_TileVertical, _("New &Vertical Group"));
    //menuViewTabs->AppendSeparator();
    //menuViewTabs->Append(ID_Window_Cascade, _("No &Group"));

    // note: the accelerators for zoom in and zoom out are not valid
    // accelerators in the menu because of the ++/+- syntax; we put
    // these here for display; the actual accelerators are defined
    // in the accelerator table above
    wxMenu* menuViewZoom = new wxMenu;
          
    menuViewZoom->Append(ID_View_ZoomIn, _("&Increase\tCtrl++"));
    menuViewZoom->Append(ID_View_ZoomOut, _("&Decrease\tCtrl+-"));
    menuViewZoom->AppendSeparator();
    menuViewZoom->Append(ID_View_ZoomToFit, _("Size To &Fit"));
    menuViewZoom->Append(ID_View_ZoomToWidth, _("Size To &Width"));
    menuViewZoom->AppendSeparator();
    menuViewZoom->Append(ID_View_ZoomToActual, _("&Normal\tCtrl+0"));

    m_view_menu = new wxMenu;
    // NOTE: the document view switcher menu items will be prepended
    //       to this menu in the updateViewMenu() function
    m_view_menu->AppendSubMenu(menuViewTabs, _("&Tabs"));
    m_view_menu->AppendSubMenu(menuViewZoom, _("&Zoom"));
    m_view_menu->AppendSeparator();
    m_view_menu->AppendCheckItem(ID_Frame_ToggleStandardToolbar, _("&Navigation Toolbar"));
    m_view_menu->AppendCheckItem(ID_Frame_ToggleLinkToolbar, _("&Bookmarks Toolbar"));
    m_view_menu->AppendCheckItem(ID_Frame_ToggleFormatToolbar, _("F&ormat Toolbar"));
    m_view_menu->AppendSeparator();
    m_view_menu->AppendCheckItem(ID_Frame_ToggleStatusBar, _("St&atus Bar"));
    m_view_menu->AppendSeparator();
    m_view_menu->AppendCheckItem(ID_Frame_ToggleProjectPanel, _("&Project Panel\tCtrl+B"));
    m_view_menu->AppendCheckItem(ID_Frame_ToggleConsolePanel, _("&Console Panel\tCtrl+K"));
    m_view_menu->AppendCheckItem(ID_Frame_ToggleColumnListPanel, _("&Fields Panel"));
    m_view_menu->AppendCheckItem(ID_Frame_ToggleMarkManagerPanel, _("&Marks Panel"));
    m_view_menu->AppendSeparator();
    m_view_menu->AppendCheckItem(ID_Frame_ToggleFullScreen, _("F&ull Screen\tF11"));

    // create the data menu
    
    wxMenu* menuDataColumns = new wxMenu;
    menuDataColumns->Append(ID_Data_CreateDynamicField, _("Insert &Calculated Field..."));
    menuDataColumns->Append(ID_Data_ModifyDynamicField, _("&Edit Calculated Field..."));
    menuDataColumns->Append(ID_Data_MakeDynamicFieldStatic, _("Convert to Fixed &Field"));
    menuDataColumns->Append(ID_Data_DeleteField, _("&Delete Calculated Field"));
    menuDataColumns->AppendSeparator();
    menuDataColumns->Append(ID_Table_EditViews, _("&Insert Column..."));
    menuDataColumns->Append(ID_Table_HideColumn, _("&Hide Column"));
    
    wxMenu* menuDataGroups = new wxMenu;
    menuDataGroups->Append(ID_Table_InsertGroupBreak, _("&Insert Group Break"));
    menuDataGroups->Append(ID_Table_RemoveGroupBreak, _("&Remove Group Break"));
    menuDataGroups->AppendSeparator();
    menuDataGroups->Append(ID_Data_GroupRecords, _("&Group Records..."));

    wxMenu* menuData = new wxMenu;
    menuData->Append(ID_Data_Sort, _("&Sort..."));
    menuData->Append(ID_Data_Filter, _("&Filter...\tCtrl+J"));
    menuData->Append(ID_Data_RemoveSortFilter, _("&Remove Sort/Filter\tCtrl+M"));
    menuData->AppendSeparator();
    menuData->Append(ID_Data_EditIndexes, _("Edit &Indexes..."));
    menuData->AppendSeparator();
    menuData->AppendSubMenu(menuDataColumns, _("&Columns"));
    menuData->AppendSubMenu(menuDataGroups, _("&Groups"));
    menuData->AppendSeparator();
    menuData->Append(ID_Data_MarkRecords, _("&Mark Records..."));
    menuData->AppendSeparator();
    menuData->Append(ID_Data_CopyRecords, _("C&opy Records...\tCtrl+E"));
    menuData->Append(ID_Data_AppendRecords, _("&Append Records..."));
    menuData->Append(ID_Data_DeleteRecords, _("&Delete Records..."));
    menuData->Append(ID_Data_UpdateRecords, _("&Update Records..."));
    menuData->AppendSeparator();
    menuData->Append(ID_Data_Summary, _("Summari&ze"));

    wxMenu* menuToolsRelationshipSync = new wxMenu;
    menuToolsRelationshipSync->AppendRadioItem(ID_App_RelationshipSyncNone, _("&No Relationship Filter or Mark"));
    menuToolsRelationshipSync->AppendRadioItem(ID_App_RelationshipSyncFilter, _("&Filter Related Records"));
    menuToolsRelationshipSync->AppendRadioItem(ID_App_RelationshipSyncSeek, _("&Mark Related Records"));

    wxMenu* menuTools = new wxMenu;
    menuTools->Append(ID_Project_Relationship, _("&Relationships..."));
    menuTools->AppendSubMenu(menuToolsRelationshipSync, _("Rel&ated Records"));
    menuTools->AppendSeparator();
    menuTools->Append(ID_File_Run, _("Ru&n Script/Query\tAlt+Enter"));
    menuTools->Append(ID_File_Stop, _("&Cancel Job\tCtrl+Break"));
    menuTools->AppendSeparator();
    //menuTools->Append(ID_App_StartRecord, _("&Record Macro"));
    //menuTools->Append(ID_App_StopRecord, _("&Stop Recording"));
    //menuTools->AppendSeparator();
    menuTools->Append(ID_Project_Split, _("&Divide Table..."));
    menuTools->Append(ID_Project_Merge, _("&Merge Tables..."));
    menuTools->AppendSeparator();
    menuTools->Append(ID_Project_CleanProject, _("Compact &Project"));
    menuTools->AppendSeparator();
    menuTools->Append(ID_App_JobManager, _("&Jobs..."));
    menuTools->Append(ID_App_JobScheduler, _("Sc&hedule Jobs..."));
    menuTools->Append(ID_App_ExtensionManager, _("&Extensions..."));
    menuTools->AppendSeparator();
    menuTools->Append(ID_App_Options, _("&Options..."));
    
    // note: for online items, some menu items may not have target URLs 
    // (depending on application definitions in appversion.h; only show 
    // the menu items that have actual URLs associated with them
    wxMenu* menuHelp = new wxMenu;
    wxString help_item;
    bool append_help_contents_separator = false;
    bool append_help_resources_separator = false;
    bool append_help_update_separator = false;
    
    // local help contents should always exist; other help-related
    // items may or may not exist
    if (true)
    {
        menuHelp->Append(ID_Project_Help, _("&Help Contents\tF1"));
        append_help_contents_separator = true;
    }
    
    help_item = APP_WEBLOCATION_HELP;
    if (help_item.Length() > 0)
    {
        menuHelp->Append(ID_Project_Reference, _("&Online Help"));
        append_help_contents_separator = true;
    }
    help_item = APP_WEBLOCATION_DEVELOPER;
    if (help_item.Length() > 0)
    {
        menuHelp->Append(ID_Project_Resources, _("&Developer Resources"));
        append_help_contents_separator = true;
    }
    if (append_help_contents_separator)
        menuHelp->AppendSeparator();
    
    // help resources info
    help_item = APP_WEBLOCATION_SUPPORT;
    if (help_item.Length() > 0)
    {
        menuHelp->Append(ID_Project_Support, _("Support &Forums"));
        append_help_resources_separator = true;
    }
    help_item = APP_WEBLOCATION_CONTACT;
    if (help_item.Length() > 0)
    {
        menuHelp->Append(ID_Project_ContactUs, _("&Contact Us"));
        append_help_resources_separator = true;
    }
    if (append_help_resources_separator)        
        menuHelp->AppendSeparator();

    // help update info
    help_item = APP_INETAUTH_AUTHSERVERLIST;
    if (help_item.Length() > 0)
    {
        menuHelp->Append(ID_App_LicenseManager, _("&Activate..."));
        append_help_update_separator = true;        
    }
    help_item = APP_UPDATE_URL;
    if (help_item.Length() > 0)
    {
        menuHelp->Append(ID_App_CheckForUpdates, _("Check for &Updates..."));
        append_help_update_separator = true;
    }
    if (append_help_update_separator)
        menuHelp->AppendSeparator();

    // help about info; always include
    menuHelp->Append(ID_App_About, _("A&bout..."));


    
    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, _("&File"));
    menuBar->Append(menuEdit, _("&Edit"));
    menuBar->Append(m_view_menu, _("&View"));
    menuBar->Append(menuData, _("&Data"));
    menuBar->Append(menuTools, _("&Tools"));
    menuBar->Append(menuHelp, _("&Help"));
    
    // give the application hook a chance to managed the menu bar
    apphookInitMenuBar(menuBar);
    
    m_frame->setMenuBar(menuBar, 4);
    
    
    // create the standard toolbar and dock it
    m_project_toolbar = new StandardToolbar(m_frame->getFrameWindow());
    m_frame->dockWindow(m_project_toolbar,
                        _("Navigation Toolbar"),
                        sitetypeDockable |
                        dockTop |
                        siteNoResize,
                        0, -1, 0, 0);
    m_frame->getAuiManager().GetPane(m_project_toolbar).Name(wxT("NavigationToolbar")).Resizable().DockFixed();
    
    // create the linkbar and dock it
    m_linkbar = new LinkBar(m_frame->getFrameWindow());
    m_frame->dockWindow(m_linkbar,
                        _("Bookmarks Toolbar"),
                        sitetypeDockable |
                        dockTop |
                        siteNoResize,
                        1, -1, 0, 0);
    m_frame->getAuiManager().GetPane(m_linkbar).Name(wxT("BookmarksToolbar")).Resizable().DockFixed();
    
    // create the format toolbar and dock it
    m_format_toolbar = new FormatToolbar(m_frame->getFrameWindow());
    m_frame->dockWindow(m_format_toolbar,
                        _("Format Toolbar"),
                        sitetypeDockable |
                        dockTop |
                        siteNoResize |
                        siteHidden,
                        2, -1, 0, 0);
    m_frame->getAuiManager().GetPane(m_format_toolbar).Name(wxT("FormatToolbar")).Resizable().DockFixed();
    





    // create and add global items to IStatusBar
    IStatusBarPtr statusbar;
    statusbar = static_cast<IStatusBar*>(new StatusBar(m_frame));
    statusbar->sigItemLeftClick().connect(this, &AppController::onStatusBarItemLeftClick);
    statusbar->sigItemLeftDblClick().connect(this, &AppController::onStatusBarItemLeftDblClick);
    statusbar->sigRefresh().connect(this, &AppController::onStatusBarRefresh);

    
    wxDoubleClickGauge* job_progress_gauge;
    job_progress_gauge = new wxDoubleClickGauge(statusbar->getStatusBarCtrl(),
                                                -1,
                                                100,
                                                wxDefaultPosition,
                                                wxSize(130, 16));
    
    // NOTE: The extra padding that is added to these items has been added
    //       so that the spacing of these items matches the spacing of the
    //       static items on the far left of the LinkBar
    
    // add panel toggle statusbar items
    IStatusBarItemPtr item;
    item = statusbar->addItem(wxT("app_toggle_projectpanel"), StatusBar::LocationLeft);
    item->setBitmap(GETBMP(gf_project_16));
    item->setToolTip(_("Show/Hide Project Panel"));
    item->setPadding(3,1);
    
    item = statusbar->addItem(wxT("app_toggle_fieldspanel"), StatusBar::LocationLeft);
    item->setBitmap(GETBMP(gf_field_16));
    item->setToolTip(_("Show/Hide Fields Panel"));
    item->setPadding(0,1);
    
    item = statusbar->addItem(wxT("app_toggle_markspanel"), StatusBar::LocationLeft);
    item->setBitmap(GETBMP(gf_highlight_16));
    item->setToolTip(_("Show/Hide Marks Panel"));
    item->setPadding(0,1);
    
    item = statusbar->addItem(wxT("app_toggle_relationshipspanel"), StatusBar::LocationLeft);
    item->setBitmap(GETBMP(gf_related_field_16));
    item->setToolTip(_("Show/Hide Relationships Panel"));
    
    // add separator
    item = statusbar->addSeparator(wxEmptyString, StatusBar::LocationLeft);
    
    // add job statusbar items
    item = statusbar->addItem(wxT("app_job_failed"), StatusBar::LocationLeft);
    item->setBitmap(GETBMP(gf_exclamation_16));
    item->show(false);
    
    item = statusbar->addControl(job_progress_gauge, wxT("app_job_gauge"), StatusBar::LocationLeft);
    item->show(false);
    
    item = statusbar->addItem(wxT("app_job_text"), StatusBar::LocationLeft);
    item->setWidth(180);
    item->show(false);
    
    // store job text item for later use
    m_job_text_item = item;

    // add separator
    item = statusbar->addSeparator(wxT("app_job_text_separator"), StatusBar::LocationLeft);
    item->show(false);
    
    // store job separator item for later use
    m_job_separator_item = item;
    
    // add general text statusbar item
    item = statusbar->addItem(wxT("app_statusbar_text"), StatusBar::LocationLeft);
    item->setProportion(1);
    
    // store statusbar text item for later use
    m_doc_text_item = item;
    
    statusbar->populate();
    
    m_frame->setStatusBar(statusbar);
    
    m_frame->dockWindow(statusbar->getStatusBarCtrl(),
               wxEmptyString,
               sitetypeDockable |
               dockBottom,
               0, -1, 0, 0);
    m_frame->getAuiManager().GetPane(statusbar->getStatusBarCtrl()).Name(wxT("StatusBar")).Resizable().DockFixed(true);


    // make sure we create the job gauge update timer after all statubar items
    // since the job update timer references these items and stores a local
    // copy of them to alleviate processing time in its Notify() function

    // create a timer for updating the job gauge in the status bar
    m_job_update_timer = new JobGaugeUpdateTimer(m_frame->getStatusBar(),
                                                 g_app->getJobQueue(),
                                                 job_progress_gauge);
    m_job_update_timer->Start(100);







    // get the setting for m_data_locked ("Protect Data")
    
    m_data_locked = g_app->getAppPreferences()->getBoolean(wxT("app.data_locked"), true);
    

    IDocumentSitePtr site;

    // create console
    ConsolePanel* panel = new ConsolePanel;
    site = m_frame->createSite(panel,
                               sitetypeDockable | dockBottom | siteHidden,
                               0, 0, 600, 175);
    site->setCaption(_("Console"));
    site->setName(wxT("ConsolePanel"));

    
    // create dbdoc and dock it
    m_dbdoc = new DbDoc;
    m_dbdoc->initAsDocument();
    m_dbdoc->getDbDocSite()->setCaption(_("Project"));
    m_dbdoc->getDbDocSite()->setName(wxT("ProjectPanel"));
    g_app->setDbDoc(m_dbdoc);


    int state;

    bool flush = false;

    // set the home page(s)
    if (!prefs->exists(wxT("general.location.home")))
    {
        prefs->setString(wxT("general.location.home"),
                         getAppPrefsDefaultString(wxT("general.location.home")));
        flush = true;
    }
    
    if (flush)
        prefs->flush();
    

    // create the column list panel
    state = sitetypeDockable | dockLeft | siteHidden;
    ColumnListPanel* columnlist_panel = new ColumnListPanel;
    m_columnlistpanel_site = m_frame->createSite(columnlist_panel, state, 0, 0, 200, 360);
    m_columnlistpanel_site->setCaption(_("Fields"));
    m_columnlistpanel_site->setName(wxT("FieldsPanel"));
    
    // make sure the the column list is docked
    // below the project panel by default
    wxAuiPaneInfo& info = m_frame->getAuiManager().GetPane(wxT("FieldsPanel"));
    info.Position(1);
    
    
    
    
    // create the marks panel
    state = sitetypeDockable | dockRight | siteHidden;
    MarkMgrPanel* markmgr_panel = new MarkMgrPanel;
    m_markmanagerpanel_site = m_frame->createSite(markmgr_panel, state, 0, 0, 200, 400);
    m_markmanagerpanel_site->setCaption(_("Marks"));
    m_markmanagerpanel_site->setName(wxT("MarksPanel"));
    
    

    // load frame perspective
    
    if (perspective.length() > 0)
        m_frame->getAuiManager().LoadPerspective(towx(perspective), false);
    
    
    // create the relationship diagram watcher (watches for files that are
    // renamed in order to update its own internal structures)
    m_rel_diagram_watcher = new RelDiagramWatcher;
    
    // recalculate the frame's layout
    m_frame->refreshFrameLayout();

    if (maximized)
        m_frame->getFrameWindow()->Maximize();

    // check the license
    if (!checkLicense(true))
    {
        // we don't want to show a message here if the user pressed 'Cancel'
        // in the Activate panel since it's clear they want to exit
        
        if (m_frame.isOk())
            m_frame->getFrameWindow()->Close(true);

        return false;
    }

    g_app->processIdle();
    
    if (!m_frame)
    {
        // license was deactivated
        return false;
    }
    
    
    // reset to (and save) the default perspective (first time only)
    if (clear_perspective_counter < ClearPerspectiveCounter)
        resetToDefaultPerspective();


    // set initial preferences
    
    // NOTE: This must be at the end of AppController::init() because it sends
    //       a size event to the frame which recalculates the size of the all
    //       of the frame's children (must be done because of toolbar sizing)
    doPreferenceRefresh();

    // update the frame's titlebar
    updateTitle();

    // show the frame
    m_frame->getFrameWindow()->Update();
    m_frame->show(true);

    // if frame is maximized, this is necessary to prevent drawing
    // problems (for unknown reasons).  Problem started happening
    // on Oct 9th 2007 when work commenced on the new status bar;
    if (maximized)
        m_frame->refreshFrameLayout();
    

    // we need to add the find panel here (hidden, of course),
    // because we want to be able to add items to the find combobox;
    // this panel has to be created after the main frame, otherwise
    // it will get a task bar selector because it is the first top-
    // level window to be created
    FindPanel* find_panel = new FindPanel;
    
    site = m_frame->createSite(find_panel,
                               sitetypeModeless | siteHidden,
                               -1, -1, 340, 190);
    site->setMinSize(340,190);
    site->setName(wxT("FindPanel"));
    
    

    // create the default project if it doesn't already exist
    bool default_project_created = getAppPrefsBoolean(wxT("general.default_project_created"));
    if (!default_project_created)
        createDefaultProject();
    
    // show startup page(s) -- this must be done after the call
    // to createDefaultProject() above since that call opens
    // and closes the default project (which closes all of
    // the documents that have been opened)
    showStartupPage();

    // display the database manager dialog screen
    int startup_action = getAppPrefsLong(wxT("general.startup.default_action"));

    // we've removed the 'Do Nothing' startup action
    if (startup_action == prefStartupNone)
        startup_action = prefStartupProjectMgr;

    if (startup_action == prefStartupProjectMgr)
    {
        showProjectManager();
    }
     else if (startup_action == prefStartupOpenProject)
    {
        wxString connstr = getAppPrefsString(wxT("general.startup.connection"));
        
        // do a lookup of the startup project
        ProjectMgr projmgr;
        int idx = projmgr.getIdxFromEntryName(connstr);
        
        // if we have a startup project, try to load it
        if (idx != -1)
        {
            std::vector<ProjectInfo>& projects = projmgr.getProjectEntries();
            
            // if we can't open the project, show the project manager
            if (!openProject(projects[idx].location,
                             projects[idx].user_id,
                             projects[idx].passwd))
            {
                // we can't find the project, reset the default startup action
                prefs->setLong(wxT("general.startup.default_action"), prefStartupProjectMgr);
                prefs->flush();

                showProjectManager();
            }
        }
         else
        {
            // we can't find the project, reset the default startup action
            prefs->setLong(wxT("general.startup.default_action"), prefStartupProjectMgr);
            prefs->flush();
            
            showProjectManager();
        }
    }
    
    
    // if we don't have an open database yet, the user did not start with
    // a project open and exited the Project Manager dialog without
    // selecting a project, so bail out and exit the application

    if (g_app->getDatabase().isNull())
    {
        // post the event instead of calling Close().  This solves a problem
        // with an assertion while/after removing event handlers

        //m_frame->getFrameWindow()->Close(true);

        wxCloseEvent evt(wxEVT_CLOSE_WINDOW, m_frame->getFrameWindow()->GetId());
        evt.SetEventObject(m_frame->getFrameWindow());
        evt.SetCanVeto(false);
        ::wxPostEvent(m_frame->getFrameWindow(), evt);

        return true;
    }

    // populate the statusbar
    g_app->getMainFrame()->getStatusBar()->populate();

    // start any extensions that are supposed to be run on startup
    // but only if -n or --noext is not specified
    if (!g_app->getCommandLine()->Found(wxT("n")))
    {
        ExtensionMgr* ext_mgr = g_app->getExtensionMgr();
        ext_mgr->startAllExtensions();
    }
    
    return true;
}

void AppController::resetToDefaultPerspective()
{
    wxFrame* frame = g_app->getMainWindow();
    frame->Freeze();
    
    // make sure we reset the toolbar style
    long default_toolbar_style = getAppPrefsDefaultLong(wxT("general.toolbar.style"));
    g_app->getAppPreferences()->setLong(wxT("general.toolbar.style"), default_toolbar_style);
    g_app->getAppPreferences()->flush();
    
    std::wstring perspective;
    long x, y, w, h, screen_x, screen_y;
    bool maximized;
    
    screen_x = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
    screen_y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
    screen_y -= getTaskBarHeight();
    
    // read the default values (if they exist)
    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/WindowDimensions");
    config->read(L"DefaultXPos",        &x,           0);
    config->read(L"DefaultYPos",        &y,           0);
    config->read(L"DefaultWidth",       &w,           screen_x);
    config->read(L"DefaultHeight",      &h,           screen_y);
    config->read(L"DefaultMaximized",   &maximized,   false);
    config->read(L"DefaultPerspective", perspective,  L"");
    
    // load the default perspective
    g_app->getMainWindow()->SetSize(x,y,w,h);
    g_app->getMainFrame()->getAuiManager().LoadPerspective(perspective, true);
    
    // get the (updated) default perspective string from the aui manager
    perspective = m_frame->getAuiManager().SavePerspective();
    
    // write out the default values
    config->write(L"DefaultXPos",        x);
    config->write(L"DefaultYPos",        y);
    config->write(L"DefaultWidth",       w);
    config->write(L"DefaultHeight",      h);
    config->write(L"DefaultMaximized",   maximized);
    config->write(L"DefaultPerspective", perspective);
    
    frame->Thaw();
}

void AppController::doPreferenceRefresh()
{
    // toolbar and button prefs
    
    IAppPreferencesPtr prefs = g_app->getAppPreferences();

    long toolbar_prefs_setting, toolbar_style_flags;
    toolbar_prefs_setting = getAppPrefsLong(wxT("general.toolbar.style"));
    prefs->setLong(wxT("general.toolbar.style"), toolbar_prefs_setting);
    prefs->flush();
    
    // set the aui toolbar style
    switch (toolbar_prefs_setting)
    {
        case prefToolbarSmallIcons:         toolbar_style_flags = wxAUI_TB_DEFAULT_STYLE; break;
        default:
        case prefToolbarLargeIcons:         toolbar_style_flags = wxAUI_TB_DEFAULT_STYLE; break;
        case prefToolbarLargeIconsWithText: toolbar_style_flags = wxAUI_TB_TEXT; break;
    }
    
    // make sure the toolbar doesn't have a border
    toolbar_style_flags |= wxNO_BORDER;


    // update standard toolbar look-and-feel
    m_project_toolbar->setSmallIcons(toolbar_prefs_setting == prefToolbarSmallIcons ? true : false);
    m_project_toolbar->SetWindowStyleFlag(toolbar_style_flags);
    m_project_toolbar->Realize();
    wxAuiPaneInfo& pane = m_frame->getAuiManager().GetPane(m_project_toolbar);
    wxSize tb_min_size = m_project_toolbar->GetMinSize();
    pane.MinSize(tb_min_size).BestSize(tb_min_size);
    m_frame->refreshFrameLayout();


    // set the relationship sync type based on the preference
    wxString synctype = getAppPrefsString(wxT("general.relationship.synctype"));
    if (synctype == wxT("none"))
        setRelationshipSync(tabledocRelationshipSyncNone);
    if (synctype == wxT("filter"))
        setRelationshipSync(tabledocRelationshipSyncFilter);
    if (synctype == wxT("seek"))
        setRelationshipSync(tabledocRelationshipSyncSeek);


    // to reference what the Mozilla preference values are,
    // go to http://kb.mozillazine.org/About:config_entries

    // update the internet preferences
    wxWebPreferences webprefs = wxWebControl::GetPreferences();
    
    // set user agent
    
    wxString agent_extra;
    
    agent_extra = APP_CONFIG_KEY;
    agent_extra = wxT("/");

    agent_extra += wxString::Format(wxT("%d.%d.%d.%d"),
                            APP_VERSION_MAJOR,
                            APP_VERSION_MINOR,
                            APP_VERSION_SUBMINOR,
                            APP_VERSION_BUILDSERIAL);
    
    webprefs.SetStringPref( wxT("general.useragent.extra.kx"), agent_extra);
    
    
    // set proxy settings
    
    webprefs.SetStringPref( wxT("network.proxy.http"),       getAppPrefsString(wxT("internet.proxy.http")));
    webprefs.SetIntPref(    wxT("network.proxy.http_port"),  getAppPrefsLong(wxT("internet.proxy.http_port")));
    
    webprefs.SetStringPref( wxT("network.proxy.ftp"),        getAppPrefsString(wxT("internet.proxy.ftp")));
    webprefs.SetIntPref(    wxT("network.proxy.ftp_port"),   getAppPrefsLong(wxT("internet.proxy.ftp_port")));
    
    webprefs.SetStringPref( wxT("network.proxy.ssl"),        getAppPrefsString(wxT("internet.proxy.ssl")));
    webprefs.SetIntPref(    wxT("network.proxy.ssl_port"),   getAppPrefsLong(wxT("internet.proxy.ssl_port")));
    
    webprefs.SetStringPref( wxT("network.proxy.socks"),      getAppPrefsString(wxT("internet.proxy.socks")));
    webprefs.SetIntPref(    wxT("network.proxy.socks_port"), getAppPrefsLong(wxT("internet.proxy.socks_port")));
    
    int proxy_type = getAppPrefsLong(wxT("internet.proxy.type"));
    switch (proxy_type)
    {
        default:
        case prefProxyDirect:
            webprefs.SetIntPref(wxT("network.proxy.type"), 0 /* no proxy */);
            break;
        case prefProxyManual:
            webprefs.SetIntPref(wxT("network.proxy.type"), 1 /* manual proxy */);
            break;
        case prefProxyAutoDetect:
            webprefs.SetIntPref(wxT("network.proxy.type"), 4 /* auto-detect proxy */);
            break;
    }


    // set certificate warning preference; note: this setting is for disabling 
    // the SSL certificate warnings in certain situations where they might happen
    // heven though the security is known to exist; this is not part of the standard 
    // webpref settings that are allowed, nor is it exposed in the interface; 
    // TODO: research if there's some combination of native preferences that might 
    // work to accomplish this
    bool ignore_cert_errors = getAppPrefsBoolean(wxT("internet.ignore_cert_errors"));
    if (getAppPrefsBoolean(wxT("internet.ignore_cert_errors")))
        wxWebControl::SetIgnoreCertErrors(ignore_cert_errors);


    // set cookie preferences
    
    if (getAppPrefsBoolean(wxT("privacy.cookie.accept")))
        webprefs.SetIntPref(wxT("network.cookie.cookieBehavior"), 0 /* enable all cookies */);
         else
        webprefs.SetIntPref(wxT("network.cookie.cookieBehavior"), 2 /* disable all cookies */);
    
    webprefs.SetIntPref(wxT("network.cookie.lifetime.days"), getAppPrefsLong(wxT("privacy.cookie.lifetime_days")));
    
    int cookie_lifetime_policy = getAppPrefsLong(wxT("privacy.cookie.lifetime_policy"));
    switch (cookie_lifetime_policy)
    {
        default:
        case prefKeepCookiesUntilExpired:
            webprefs.SetIntPref(wxT("network.cookie.lifetimePolicy"), 3 /* keep for number of days specified in
                                                                           "privacy.cookie.lifetime_days" */);
            break;
        case prefKeepCookiesUntilExit:
            webprefs.SetIntPref(wxT("network.cookie.lifetimePolicy"), 2 /* accept for session only */);
            break;
    }
    
    
    // set popup preferences
    
    if (getAppPrefsBoolean(wxT("privacy.popups.block")))
        webprefs.SetIntPref(wxT("privacy.popups.policy"), 2 /* reject popups */);
         else
        webprefs.SetIntPref(wxT("privacy.popups.policy"), 1 /* accept popups */);
}

IDocumentSitePtr AppController::getMarkManagerPanelSite()
{
    return m_markmanagerpanel_site;
}

ScriptCommandEventRouter* AppController::getScriptCommandRouter()
{
    return m_script_command_router;
}

StandardToolbar* AppController::getStandardToolbar()
{
    return m_project_toolbar;
}

FormatToolbar* AppController::getFormatToolbar()
{
    return m_format_toolbar;
}

void AppController::onNew(wxCommandEvent& evt)
{
    IFramePtr main_frame = g_app->getMainFrame();
    wxFrame* main_window = g_app->getMainWindow();

    m_linkbar->SetToolSticky(evt.GetId(), true);
    
    // create the popup menu
    wxMenu menuPopup;
    
    wxMenuItem* m1 =  new wxMenuItem(&menuPopup, ID_Project_NewTab, _("Tab"));
    m1->SetBitmap(GETBMP(gf_document_16));
    menuPopup.Append(m1);
    
    wxMenuItem* m2 =  new wxMenuItem(&menuPopup, ID_Project_NewTable, _("Table"));
    m2->SetBitmap(GETBMP(gf_table_16));
    menuPopup.Append(m2);
    
    wxMenuItem* m3 =  new wxMenuItem(&menuPopup, ID_Project_NewReport, _("Report"));
    m3->SetBitmap(GETBMP(gf_report_16));
    menuPopup.Append(m3);
    
    wxMenuItem* m4 =  new wxMenuItem(&menuPopup, ID_Project_NewQuery, _("Query"));
    m4->SetBitmap(GETBMP(gf_query_16));
    menuPopup.Append(m4);
    
    wxMenuItem* m5 =  new wxMenuItem(&menuPopup, ID_Project_NewScript, _("Script"));
    m5->SetBitmap(GETBMP(gf_script_16));
    menuPopup.Append(m5);
    
    wxRect rect = m_linkbar->GetToolRect(evt.GetId());
    wxPoint pt = m_linkbar->ClientToScreen(rect.GetBottomLeft());
    pt = main_window->ScreenToClient(pt);
    
    CommandCapture* cc = new CommandCapture;
    main_window->PushEventHandler(cc);
    main_window->PopupMenu(&menuPopup, pt);
    int command = cc->getLastCommandId();
    main_window->PopEventHandler(true);

    // post an event to ourselves
    wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, command);
    ::wxPostEvent(this, e);

    // make sure the button is "un-stuck"
    m_linkbar->SetToolSticky(evt.GetId(), false);
}

void AppController::onNewTab(wxCommandEvent& evt)
{
    // create the new tab
    int state = sitetypeNormal;
    WebDoc* doc = new WebDoc;
    m_frame->createSite(doc, state, -1, -1, -1, -1);
    doc->openURI(wxT(""));

    // set the focus to the URL bar
    m_project_toolbar->m_url->SetFocus();
}

void AppController::onNewTable(wxCommandEvent& evt)
{
    showCreateTable();
}



static bool setReportCreateInfo(IDocumentSitePtr doc_site,
                                ReportCreateInfo& data)
{
    if (doc_site.isNull())
        return false;

    ITableDocPtr table_doc = doc_site->getDocument();
    if (table_doc.isNull())
        return false;

    kcl::Grid* grid = table_doc->getGrid();
    if (!grid)
        return false;

    // set the data fields
    int generate_fields = false;
    
    int i;
    int col_count = grid->getColumnCount();
    kcl::IModelPtr model = grid->getModel();
    if (model.isNull())
        return false;

    for (i = 0; i < col_count; ++i)
    {
        if (grid->isColumnSelected(i))
        {
            int model_idx = grid->getColumnModelIdx(i);
            if (model_idx == -1)
                continue;

            // determine the alignment
            wxString alignment;
            int type = model->getColumnInfo(model_idx)->getType();
            
            switch (type)
            {
                default:
                case kcl::Grid::typeCharacter:
                case kcl::Grid::typeBoolean:
                    alignment = kcanvas::ALIGNMENT_LEFT;
                    break;

                case kcl::Grid::typeDouble:
                case kcl::Grid::typeInteger:
                    alignment = kcanvas::ALIGNMENT_RIGHT;
                    break;

                case kcl::Grid::typeDate:
                case kcl::Grid::typeDateTime:
                    alignment = kcanvas::ALIGNMENT_CENTER;
                    break;
            };

            // set the field info
            ReportCreateField field;
            field.field_name = model->getColumnInfo(model_idx)->getName();
            field.caption = makeProperIfNecessary(field.field_name);
            field.alignment = alignment;
            field.column_width = grid->getColumnSize(i)*(kcanvas::CANVAS_MODEL_DPI/kcanvas::CANVAS_SCREEN_DPI);
            data.content_fields.push_back(field);

            generate_fields = true;
        }
    }

    // if no fields are selected, there's nothing to generate
    // the report with; return false
    if (!generate_fields)
        return false;

    // TODO: set the font? or use the defaults?
    //wxFont font = grid->GetFont();
    //data.font_facename = font.GetFaceName();
    //data.font_size = font.GetPointSize();

    // set the source
    tango::IIteratorPtr iter = table_doc->getIterator();
    if (iter.isNull())
        return false;

    data.iterator = iter;
    data.path = table_doc->getPath();
    data.sort_expr = table_doc->getSortOrder();
    data.filter_expr = table_doc->getFilter();

    return true;
}


void AppController::onNewReport(wxCommandEvent& evt)
{
    newReport();
}

void AppController::onNewQuery(wxCommandEvent& evt)
{
    newQuery();
}

void AppController::onNewScript(wxCommandEvent& evt)
{
    newScript();
}




void AppController::onStartRecord(wxCommandEvent& evt)
{
    g_macro.reset();
    g_macro.setIndent(0);
    g_macro.setRecording(true);
    
    wxDateTime now = wxDateTime::Now();
    
    g_macro << "// Generated Macro Script";
    g_macro << wxT("// ") + now.Format();
    g_macro << "";
    g_macro << "// invoke main macro function";
    g_macro << "main();";
    g_macro << "";
    g_macro << "";
    g_macro << "function main()";
    g_macro << "{";
    g_macro << "    var auto = HostServices.automation;";
    g_macro.setIndent(4);
}

void AppController::onStopRecord(wxCommandEvent& evt)
{
    // if we're not recording, we're done
    if (!g_macro.isRecording())
        return;

    g_macro.setIndent(0);
    g_macro << "}";
    g_macro << "";
    g_macro.setRecording(false);
    
    wxString text;
    std::vector<wxString>& lines = g_macro.getLines();
    std::vector<wxString>::iterator it;
    
    for (it = lines.begin(); it != lines.end(); ++it)
    {
        text += *it;
        text += wxT("\n");
    }
    
    g_macro.reset();
    
    int site_id = 0;
    if (!newScript(&site_id))
        return;
        
    IDocumentSitePtr site = g_app->getMainFrame()->lookupSiteById(site_id);
    IEditorDocPtr doc = site->getDocument();
    if (doc.isNull())
        return;
    
    doc->setText(text);
}




void AppController::onCreateBookmark(wxCommandEvent& evt)
{
    if (g_app->getDatabase().isNull())
    {
        appMessageBox(_("A project must be open in order to create a bookmark."),
                   APPLICATION_NAME,
                   wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }
    
    // make sure we have an open document
    IDocumentSitePtr site = m_frame->getActiveChild();
    if (site.isNull())
        return;
    
    IDocumentPtr doc = site->getDocument();
    if (doc.isNull())
        return;
    
    // get rid of unwanted characters
    wxString s = site->getCaption();
    trimUnwantedUrlChars(s);
    
    // get new bookmark path from a dialog
    LinkPropsDialog dlg(g_app->getMainWindow());
    dlg.setMode(LinkPropsDialog::ModeCreateSmall);
    dlg.setName(s);
    dlg.SetTitle(_("New Bookmark"));
    dlg.SetSize(360,145);
    dlg.SetMinSize(wxSize(370,165));
    dlg.CenterOnScreen();
    
    if (dlg.ShowModal() != wxID_OK)
        return;
    
    Bookmark::create(dlg.getPath(), doc->getDocumentLocation());
    m_linkbar->refresh();
}

void AppController::onOpenURL(wxCommandEvent& evt)
{
    // get the url string
    wxString url_str = evt.GetString();

    // open the url
    openAny(url_str);
}

static bool isValidOpenExtension(const wxString& ext)
{
    if (ext.IsEmpty() ||
        ext.CmpNoCase(wxT("csv"))  == 0 ||
        ext.CmpNoCase(wxT("icsv"))  == 0 ||
        ext.CmpNoCase(wxT("tsv"))  == 0 ||
        ext.CmpNoCase(wxT("prn"))  == 0 ||
        ext.CmpNoCase(wxT("txt"))  == 0 ||
        ext.CmpNoCase(wxT("ebc"))  == 0 ||
        ext.CmpNoCase(wxT("dbf"))  == 0 ||
        ext.CmpNoCase(wxT("sdb"))  == 0 ||
        ext.CmpNoCase(wxT("sqlite")) == 0 ||
        ext.CmpNoCase(wxT("mdb"))  == 0 ||
        ext.CmpNoCase(wxT("accdb")) == 0 ||
        ext.CmpNoCase(wxT("xls"))  == 0 ||
        ext.CmpNoCase(wxT("xlsx"))  == 0 ||
        ext.CmpNoCase(wxT("kpg"))  == 0 ||
        ext.CmpNoCase(wxT("phtml")) == 0 ||
        ext.CmpNoCase(wxT("html")) == 0 ||
        ext.CmpNoCase(wxT("htm"))  == 0 ||
        ext.CmpNoCase(wxT("hta"))  == 0 ||
        ext.CmpNoCase(wxT("css"))  == 0 ||
        ext.CmpNoCase(wxT("less"))  == 0 ||
        ext.CmpNoCase(wxT("tpl"))   == 0 ||
        ext.CmpNoCase(wxT("iim"))   == 0 ||
        ext.CmpNoCase(wxT("idl"))   == 0 ||
        ext.CmpNoCase(wxT("js"))   == 0 ||
        ext.CmpNoCase(wxT("json")) == 0)
    {
        return true;
    }
    
    return false;
}

// these extensions are specifically disallowed.  NOTE: this function
// is not the opposite of isValidOpenExtension()
static bool isInvalidOpenExtension(const wxString& ext)
{
    /*  the following executable extensions were found at
        http://antivirus.about.com/od/securitytips/a/fileextview.htm

        ADE - Microsoft Access Project Extension
        ADP - Microsoft Access Project
        BAS - Visual Basic Class Module
        BAT - Batch File
        CHM - Compiled HTML Help File
        CMD - Windows NT Command Script
        COM - MS-DOS Application
        CPL - Control Panel Extension
        CRT - Security Certificate
        DLL - Dynamic Link Library
        DOC - Word Documents
        DOT - Word Templates
        EXE - Application
        HLP - Windows Help File
        HTA - HTML Applications
        INF - Setup Information File
        INS - Internet Communication Settings
        ISP - Internet Communication Settings
        LNK - Shortcut
        MSC - Microsoft Common Console Document
        MSI - Windows Installer Package
        MSP - Windows Installer Patch
        MST - Visual Test Source File
        OCX - ActiveX Objects
        PCD - Photo CD Image
        PIF - Shortcut to MS-DOS Program
        POT - PowerPoint Templates
        PPT - PowerPoint Files
        REG - Registration Entries
        SCR - Screen Saver
        SCT - Windows Script Component
        SHB - Document Shortcut File
        SHS - Shell Scrap Object
        SYS - System Config/Driver
        VB  - VBScript File
        VBE - VBScript Encoded Script File
        VBS - VBScript Script File
        WSC - Windows Script Component
        WSF - Windows Script File
        WSH - Windows Scripting Host Settings File
    */

    if (ext.CmpNoCase(wxT("ADE")) == 0 ||
        ext.CmpNoCase(wxT("ADP")) == 0 ||
        ext.CmpNoCase(wxT("BAS")) == 0 ||
        ext.CmpNoCase(wxT("BAT")) == 0 ||
        ext.CmpNoCase(wxT("CHM")) == 0 ||
        ext.CmpNoCase(wxT("CMD")) == 0 ||
        ext.CmpNoCase(wxT("COM")) == 0 ||
        ext.CmpNoCase(wxT("CPL")) == 0 ||
        ext.CmpNoCase(wxT("CRT")) == 0 ||
        ext.CmpNoCase(wxT("DLL")) == 0 ||
        ext.CmpNoCase(wxT("DOC")) == 0 ||
        ext.CmpNoCase(wxT("DOT")) == 0 ||
        ext.CmpNoCase(wxT("EXE")) == 0 ||
        ext.CmpNoCase(wxT("HLP")) == 0 ||
        ext.CmpNoCase(wxT("HTA")) == 0 ||
        ext.CmpNoCase(wxT("INF")) == 0 ||
        ext.CmpNoCase(wxT("INS")) == 0 ||
        ext.CmpNoCase(wxT("ISP")) == 0 ||
        ext.CmpNoCase(wxT("LNK")) == 0 ||
        ext.CmpNoCase(wxT("MSC")) == 0 ||
        ext.CmpNoCase(wxT("MSI")) == 0 ||
        ext.CmpNoCase(wxT("MSP")) == 0 ||
        ext.CmpNoCase(wxT("MST")) == 0 ||
        ext.CmpNoCase(wxT("OCX")) == 0 ||
        ext.CmpNoCase(wxT("PCD")) == 0 ||
        ext.CmpNoCase(wxT("PIF")) == 0 ||
        ext.CmpNoCase(wxT("POT")) == 0 ||
        ext.CmpNoCase(wxT("PPT")) == 0 ||
        ext.CmpNoCase(wxT("REG")) == 0 ||
        ext.CmpNoCase(wxT("SCR")) == 0 ||
        ext.CmpNoCase(wxT("SCT")) == 0 ||
        ext.CmpNoCase(wxT("SHB")) == 0 ||
        ext.CmpNoCase(wxT("SHS")) == 0 ||
        ext.CmpNoCase(wxT("SYS")) == 0 ||
        ext.CmpNoCase(wxT("VB"))  == 0 ||
        ext.CmpNoCase(wxT("VBE")) == 0 ||
        ext.CmpNoCase(wxT("VBS")) == 0 ||
        ext.CmpNoCase(wxT("WSC")) == 0 ||
        ext.CmpNoCase(wxT("WSF")) == 0 ||
        ext.CmpNoCase(wxT("WSH")) == 0)
    {
        return true;
    }
    
    // this is a list of file extensions that we run into
    // on a regular basis that are not in the list above
    
    if (ext.CmpNoCase(wxT("ZIP")) == 0 ||
        ext.CmpNoCase(wxT("ISO")) == 0)
    {
        return true;
    }
    
    return false;
}

void AppController::onOpenFile(wxCommandEvent& evt)
{
    wxString path;
    wxString filter;
    wxString first_ext;
    wxString ext;

    IConnectionPtr conn;

    filter += _("All Files");
    filter += wxT(" (*.*)|*.*|");
    filter += _("All Text Files");
    filter += wxT(" (*.csv, *.tsv, *.prn, *.txt)|*.csv;*.tsv;*.prn;*.txt|");
    filter += _("Comma-Delimited Files");
    filter += wxT(" (*.csv)|*.csv|");
    filter += _("Tab-Delimited Files");
    filter += wxT(" (*.tsv)|*.tsv|");
    filter += _("Microsoft FoxPro/Xbase Files");
    filter += wxT(" (*.dbf)|*.dbf|");
    filter += _("Microsoft Access Files");
    filter += wxT(" (*.mdb, *.accdb)|*.mdb;*.accdb|");
    filter += _("Microsoft Excel Files");
    filter += wxT(" (*.xls, *.xlsx)|*.xls;*.xlsx|");
    filter += _("Package Files");
    filter += wxT(" (*.kpg)|*.kpg|");
    filter += _("Sqlite databases");
    filter += wxT(" (*.sdb, *.sqlite)|*.sdb;*.sqlite|");
    filter += _("HTML Files");
    filter += wxT(" (*.html, *.htm)|*.html;*.htm|");
    filter += _("Script Files");
    filter += wxT(" (*.js)|*.js|");
    filter.RemoveLast(); // get rid of the last pipe sign

    wxFileDialog dlg(g_app->getMainWindow(),
                 _("Open File(s)"),
                 wxEmptyString,
                 wxEmptyString,
                 filter,
                 wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    
    if (dlg.ShowModal() != wxID_OK)
        return;
    
    // show a busy cursor
    AppBusyCursor bc;

    // get paths from the open file dialog
    wxArrayString as;
    dlg.GetPaths(as);

    // check to make sure all extensions are the same
    size_t i = 0;
    size_t count = as.GetCount();
    for (i = 0; i < count; ++i)
    {
        path = as.Item(i);
        
        ext = path.AfterLast('.');
        
        if (i == 0)
        {
            first_ext = ext;
            continue;
        }
        
        // if all of the extensions are not the same, bail out
        if (ext.CmpNoCase(first_ext))
        {
            appMessageBox(_("More than one file type was selected.  Please select only one file type to continue."),
                               APPLICATION_NAME,
                               wxOK | wxICON_INFORMATION | wxCENTER);
            return;
        }
    }
    
    // open each set
    bool error_occurred = false;
    bool invalid_filetype = false;
    
    for (i = 0; i < count; ++i)
    {
        wxString url = filenameToUrl(as.Item(i));

        wxString ext = as.Item(i).AfterLast(wxT('.'));
        if (ext == as.Item(i))
            ext = wxT("");
            
        if (isValidOpenExtension(ext))
        {
            if (!openAny(url))
                error_occurred = true;
        }
         else
        {
            // check to see if this filetype is specifically disallowed,
            // if not, try opening it in the WebDoc as a fallback
            if (isInvalidOpenExtension(ext))
            {
                invalid_filetype = true;
            }
             else if (!openWeb(url))
            {
                invalid_filetype = true;
            }
        }
    }
    
    if (invalid_filetype)
    {
        wxString appname = APPLICATION_NAME;
        wxString msg = wxString::Format(_("One or more of the selected files could not be opened because it is not supported by %s."),
                                        appname.c_str());
        appMessageBox(msg,
                           APPLICATION_NAME,
                           wxOK | wxICON_INFORMATION | wxCENTER);
        return;
    }
    
    // let the user know if some of the files couldn't be opened
    if (error_occurred)
    {
        appMessageBox(_("One or more of the selected files could not be opened either because they are already in use or no longer exist."),
                           APPLICATION_NAME,
                           wxOK | wxICON_INFORMATION | wxCENTER);
    }
}

void AppController::onOpenDatabase(wxCommandEvent& evt)
{
    wxCommandEvent e;
    onConnectExternal(e);
}

void AppController::onOpenLocation(wxCommandEvent& evt)
{
    // note: this function sets the focus the URL bar

    // select the text in the URI bar
    m_project_toolbar->m_url->SetSelection(0,-1);
    
    // set the focus to the URI bar
    m_project_toolbar->m_url->SetFocus();
}

void AppController::onOpenLocationComplete(wxCommandEvent& evt)
{
    // note: this function takes the text entered in the URL, appends
    // "http://www." to the front and ".com" to the end and attempts
    // to open the resulting URL

    wxString text = m_project_toolbar->m_url->GetValue();
    text.Prepend(wxT("http://www."));
    text.Append(wxT(".com"));
    m_project_toolbar->m_url->SetValue(text);

    openAny(text);
}

void AppController::onOpenProject(wxCommandEvent& evt)
{
    if (!checkForRunningJobs())
        return;

    showProjectManager();
}

void AppController::onCloseProject(wxCommandEvent& evt)
{
    if (!checkForRunningJobs())
        return;

    IDocumentSiteEnumPtr docsites;
    IDocumentSitePtr site;
    ITableDocPtr table_doc;

    docsites = g_app->getMainFrame()->getDocumentSites(sitetypeModeless);
    if (docsites->size() > 0)
    {
        appMessageBox(_("Please close all panels before closing the project."),
                           APPLICATION_NAME,
                           wxICON_INFORMATION | wxCENTER | wxOK);
        return;
    }

    closeProject();
}

void AppController::onImportData(wxCommandEvent& evt)
{
    ImportInfo info;
    showImportWizard(info);
}

void AppController::onExportData(wxCommandEvent& evt)
{
    ExportInfo info;
    showExportWizard(info);
}

void AppController::onEditOptions(wxCommandEvent& evt)
{
    showOptionsDialog();
}

void AppController::onCloseAllChildren(wxCommandEvent& evt)
{
    m_frame->closeAll(false);
}

void AppController::onCloseChild(wxCommandEvent& evt)
{
    IDocumentSitePtr site = m_frame->getActiveChild();
    if (site)
    {
        g_app->getMainFrame()->closeSite(site);
    }
}

void AppController::onStop(wxCommandEvent& evt)
{
    IJobQueuePtr job_queue = g_app->getJobQueue();
    if (job_queue.isNull())
        return;
    
    if (!job_queue->getJobsActive())
        return;
    
    jobs::IJobInfoEnumPtr jobs;
    jobs = job_queue->getJobInfoEnum(jobStateRunning);
    if (jobs.isNull())
        return;
    
    if (jobs->size() > 1)
    {
        showJobManager();
        return;
    }
    
    jobs::IJobInfoPtr job_info = jobs->getItem(0);
    if (job_info.isNull())
        return;
    
    jobs::IJobPtr job = job_queue->lookupJob(job_info->getJobId());
    if (job.isOk())
        job->cancel();
}

void AppController::onQuit(wxCommandEvent& evt)
{
    m_frame->getFrameWindow()->Close();
}

void AppController::onAbout(wxCommandEvent& evt)
{
    showAbout();
}

void AppController::onToggleFullScreen(wxCommandEvent& evt)
{
    static bool message_shown = false;
    if (!message_shown)
    {
        message_shown = true;
        appMessageBox(_("To exit full screen mode, press F11."),
              APPLICATION_NAME,
              wxOK | wxICON_INFORMATION);
    }
    
    toggleFullScreen();
}

void AppController::onToggleLock(wxCommandEvent& evt)
{
    toggleLock();
}

void AppController::onToggleAllPanels(wxCommandEvent& evt)
{
    toggleAllPanels();
}

void AppController::onToggleProjectPanel(wxCommandEvent& evt)
{
    toggleProjectPanel();
}

void AppController::onToggleStatusBar(wxCommandEvent& evt)
{
    toggleStatusBar();
}

void AppController::onToggleConsolePanel(wxCommandEvent& evt)
{
    toggleConsolePanel();
}

void AppController::onToggleColumnListPanel(wxCommandEvent& evt)
{
    toggleColumnListPanel();
}

void AppController::onToggleMarkManagerPanel(wxCommandEvent& evt)
{
    toggleMarkManagerPanel();
}

void AppController::onToggleRelationshipPanel(wxCommandEvent& evt)
{
    toggleRelationshipPanel();
}

void AppController::onToggleToolbar(wxCommandEvent& evt)
{
    toggleToolbar(evt.GetId());
}

void AppController::onShowFindPanel(wxCommandEvent& evt)
{
    if (evt.GetId() == ID_Edit_Find)
        showFindPanel();
    else if (evt.GetId() == ID_Edit_FindInFiles)
        showFindInFilesPanel();
    else if (evt.GetId() == ID_Edit_Replace)
        showReplacePanel();
}

void AppController::onShowColumnListPanel(wxCommandEvent& evt)
{
    showColumnListPanel();
}

void AppController::onShowProjectPanel(wxCommandEvent& evt)
{
    showProjectPanel(true, true);
}

void AppController::onShowConsolePanel(wxCommandEvent& evt)
{
    showConsolePanel(true, true);
}

void AppController::onViewChanged(wxCommandEvent& evt)
{
    FrameworkEvent *e1, *e2;

    IFramePtr main_frame = g_app->getMainFrame();
    wxFrame* main_window = g_app->getMainWindow();
    
    bool is_allowed = true;

    // switch to the view that was selected
    int command = evt.GetId();
    if (command != 0)
    {
        e1 = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGING);
        e1->l_param = (unsigned long)command;
        e1->l_param2 = (unsigned long)(&is_allowed);
        main_frame->sendEvent(e1);
    }
    
    // don't allow the view to be changed
    if (!is_allowed)
        return;
        
    // switch to the view that was selected
    if (command != 0)
    {    
        e2 = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGED);
        e2->l_param = (unsigned long)command;
        main_frame->sendEvent(e2);
    }
}


void AppController::doViewSwitcher(bool drop_down_menu)
{
    FrameworkEvent *e1, *e2, *e3;
    
    IFramePtr main_frame = g_app->getMainFrame();
    wxFrame* main_window = g_app->getMainWindow();

    // get the list of available views from the active document
    ViewSwitcherList* list = new ViewSwitcherList;
    e1 = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_QUERY_AVAILABLE_VIEW);
    e1->o_param = (wxObject*)list;
    main_frame->sendEvent(e1);
    
    int id;
    wxString label;
    bool checked;
    
    wxMenu menuPopup;
    
    int next_id = -1;
    int command;
    
    size_t i, count = list->getCount();
    for (i = 0; i < count; ++i)
    {
        list->getItemInfo(i, &id, &label, &checked);
        
        wxMenuItem* item = menuPopup.AppendCheckItem(id, label);
        if (checked)
        {
            item->Check(true);
            
            // figure out which document is the "next" document
            if (next_id == -1)
            {
                // we're on the last item, so the
                // first item is the next item
                if (i == count-1)
                {
                    list->getItemInfo(0, &id, &label, &checked);
                    next_id = id;
                }
                 else
                {
                    list->getItemInfo(i+1, &id, &label, &checked);
                    next_id = id;
                }
            }
        }
    }
    
    if (drop_down_menu)
    {
        m_project_toolbar->SetToolSticky(ID_View_ViewSwitcher, true);
        
        // FIXME: The positioning of this menu is troublesome.  On Windows,
        //        if the menu is going to go offscreen, Windows moves
        //        the menu so that the menu no longer goes offscreen,
        //        however, this means that the menu no longer lines up
        //        with the toolbar item.  Ideally, we should measure
        //        the menu ourselves and determine if it should be
        //        popped up from the bottom left or bottom right
        
        // the dropdown arrow was clicked, show the view choices to the user
        wxRect rect = m_project_toolbar->GetToolRect(ID_View_ViewSwitcher);
        wxPoint pt(rect.GetRight()+1, rect.GetBottom());

        CommandCapture* cc = new CommandCapture;
        main_window->PushEventHandler(cc);
        main_window->PopupMenu(&menuPopup, pt);
        command = cc->getLastCommandId();
        main_window->PopEventHandler(true);

        // make sure the button is "un-stuck"
        m_project_toolbar->SetToolSticky(ID_View_ViewSwitcher, false);
    }
     else
    {
        // the main icon was clicked, move to the next view
        command = next_id;
    }


    bool is_allowed = true;

    // switch to the view that was selected
    if (command != 0)
    {
        e2 = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGING);
        e2->l_param = (unsigned long)command;
        e2->l_param2 = (unsigned long)(&is_allowed);
        main_frame->sendEvent(e2);
    }
    
    // don't allow the view to be changed
    if (!is_allowed)
        return;
        
    // switch to the view that was selected
    if (command != 0)
    {
        e3 = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGED);
        e3->l_param = (unsigned long)command;
        main_frame->sendEvent(e3);
    }
    
    // cleanup before we're done
    delete list;
}

void AppController::onViewSwitcher(wxCommandEvent& evt)
{
    doViewSwitcher(false);
}

void AppController::onBookmarkDropDown(wxAuiToolBarEvent& evt)
{
    if (evt.IsDropDownClicked())
    {
        wxFrame* main_window = g_app->getMainWindow();

        wxMenu menuPopup;

        menuPopup.Append(ID_File_Bookmark, _("&New Bookmark..."));
        menuPopup.Append(ID_File_ShareView, _("Share Data View..."));

        m_project_toolbar->SetToolSticky(ID_File_Bookmark, true);
        
        wxRect rect = m_project_toolbar->GetToolRect(ID_File_Bookmark);
        wxPoint pt(rect.GetRight()+1, rect.GetBottom());


        main_window->PopupMenu(&menuPopup, pt);


        m_project_toolbar->SetToolSticky(ID_File_Bookmark, false);



    }
     else
    {
        evt.Skip();
    }
}


void AppController::onViewSwitcherDropDown(wxAuiToolBarEvent& evt)
{
    if (evt.IsDropDownClicked())
    {
        doViewSwitcher(true);
    }
     else
    {
        evt.Skip();
    }
}

void AppController::onCutCopyPaste(wxCommandEvent& evt)
{
    wxWindow* focus_wnd = wxWindow::FindFocus();
    if (!focus_wnd)
    {
        evt.Skip();
        return;
    }

    wxWindow* dbdoc_window = m_dbdoc->getDocWindow();
    if (focus_wnd == dbdoc_window || focus_wnd->GetParent() == dbdoc_window)
    {
        if (evt.GetId() == ID_Edit_Cut)
        {
            m_dbdoc->doCut();
        }
         else if (evt.GetId() == ID_Edit_Copy)
        {
            m_dbdoc->doCopy();
        }
         else if (evt.GetId() == ID_Edit_Paste)
        {
            m_dbdoc->doPaste();
        }
    
        return;
    }

    if (evt.GetId() == ID_Edit_Cut)
    {
        if (focus_wnd->IsKindOf(CLASSINFO(wxTextCtrl)))
            ((wxTextCtrl*)focus_wnd)->Cut();
        else if (focus_wnd->IsKindOf(CLASSINFO(wxStyledTextCtrl)))
            ((wxStyledTextCtrl*)focus_wnd)->Cut();
    }
     else if (evt.GetId() == ID_Edit_Copy)
    {
        if (focus_wnd->IsKindOf(CLASSINFO(wxTextCtrl)))
            ((wxTextCtrl*)focus_wnd)->Copy();
        else if (focus_wnd->IsKindOf(CLASSINFO(wxStyledTextCtrl)))
            ((wxStyledTextCtrl*)focus_wnd)->Copy();
    }
     else if (evt.GetId() == ID_Edit_Paste)
    {
        if (focus_wnd->IsKindOf(CLASSINFO(wxTextCtrl)))
            ((wxTextCtrl*)focus_wnd)->Paste();
        else if (focus_wnd->IsKindOf(CLASSINFO(wxStyledTextCtrl)))
            ((wxStyledTextCtrl*)focus_wnd)->Paste();
    }
     else if (evt.GetId() == ID_Edit_Undo)
    {
        if (focus_wnd->IsKindOf(CLASSINFO(wxTextCtrl)))
            ((wxTextCtrl*)focus_wnd)->Undo();
        else if (focus_wnd->IsKindOf(CLASSINFO(wxStyledTextCtrl)))
            ((wxStyledTextCtrl*)focus_wnd)->Undo();
    }
     else if (evt.GetId() == ID_Edit_Redo)
    {
        if (focus_wnd->IsKindOf(CLASSINFO(wxTextCtrl)))
            ((wxTextCtrl*)focus_wnd)->Redo();
        else if (focus_wnd->IsKindOf(CLASSINFO(wxStyledTextCtrl)))
            ((wxStyledTextCtrl*)focus_wnd)->Redo();
    }
}

void AppController::onTextColorDropDown(wxAuiToolBarEvent& evt)
{
    wxAuiToolBar* format_toolbar = (wxAuiToolBar*)(evt.GetEventObject());
    
    if (evt.IsDropDownClicked())
    {
        // the dropdown arrow was clicked, show the color panel
        wxRect rect = evt.GetItemRect();
        wxPoint pt(rect.GetLeft(), rect.GetBottom());
        pt = format_toolbar->ClientToScreen(pt);

        format_toolbar->SetToolSticky(ID_Format_Border, false);
        format_toolbar->SetToolSticky(ID_Format_TextColor, false);
        format_toolbar->SetToolSticky(ID_Format_FillColor, false);
        format_toolbar->SetToolSticky(ID_Format_LineColor, false);

        m_colorpanel_helper = new ColorPanelHelper(
            pt,
            kcl::PopupContainer::fromTopLeft,
            kcl::ColorPanel::ModeText,
            kcl::ColorPanelLayout(kcl::ColorPanelLayout::DefaultColorButton,
                                  kcl::ColorPanelLayout::ColorGrid),
            *wxBLACK,
            format_toolbar,
            evt.GetId());
    }
     else
    {
        wxAuiToolBarItem* item = format_toolbar->FindTool(evt.GetToolId());
        postColorSelectedEvent(g_app->getMainFrame()->getActiveChild(), item);
    }
}

void AppController::onBorderDropDown(wxAuiToolBarEvent& evt)
{
    wxAuiToolBar* format_toolbar = (wxAuiToolBar*)(evt.GetEventObject());
    
    if (evt.IsDropDownClicked())
    {
        // the dropdown arrow was clicked, show the border panel
        wxRect rect = evt.GetItemRect();
        wxPoint pt(rect.GetLeft(), rect.GetBottom());
        pt = format_toolbar->ClientToScreen(pt);

        m_borderpanel_helper = new BorderPanelHelper(
            pt,
            kcl::PopupContainer::fromTopLeft,
            format_toolbar,
            evt.GetId());
    }
     else
    {
        wxAuiToolBarItem* item = format_toolbar->FindTool(evt.GetToolId());
        postBorderSelectedEvent(g_app->getMainFrame()->getActiveChild(), item);
    }
}

void AppController::onFillColorDropDown(wxAuiToolBarEvent& evt)
{
    wxAuiToolBar* format_toolbar = (wxAuiToolBar*)(evt.GetEventObject());
    
    if (evt.IsDropDownClicked())
    {
        // the dropdown arrow was clicked, show the color panel
        wxRect rect = evt.GetItemRect();
        wxPoint pt(rect.GetLeft(), rect.GetBottom());
        pt = format_toolbar->ClientToScreen(pt);

        format_toolbar->SetToolSticky(ID_Format_Border, false);
        format_toolbar->SetToolSticky(ID_Format_TextColor, false);
        format_toolbar->SetToolSticky(ID_Format_FillColor, false);
        format_toolbar->SetToolSticky(ID_Format_LineColor, false);

        m_colorpanel_helper = new ColorPanelHelper(
            pt,
            kcl::PopupContainer::fromTopLeft,
            kcl::ColorPanel::ModeFill,
            kcl::ColorPanelLayout(kcl::ColorPanelLayout::NoColorButton,
                                  kcl::ColorPanelLayout::ColorGrid),
            wxColor(255,255,178),
            format_toolbar,
            evt.GetId());
    }
     else
    {
        wxAuiToolBarItem* item = format_toolbar->FindTool(evt.GetToolId());
        postColorSelectedEvent(g_app->getMainFrame()->getActiveChild(), item);
    }
}

void AppController::onLineColorDropDown(wxAuiToolBarEvent& evt)
{
    wxAuiToolBar* format_toolbar = (wxAuiToolBar*)(evt.GetEventObject());
    
    if (evt.IsDropDownClicked())
    {
        // the dropdown arrow was clicked, show the color panel
        wxRect rect = evt.GetItemRect();
        wxPoint pt(rect.GetLeft(), rect.GetBottom());
        pt = format_toolbar->ClientToScreen(pt);

        format_toolbar->SetToolSticky(ID_Format_Border, false);
        format_toolbar->SetToolSticky(ID_Format_TextColor, false);
        format_toolbar->SetToolSticky(ID_Format_FillColor, false);
        format_toolbar->SetToolSticky(ID_Format_LineColor, false);

        m_colorpanel_helper = new ColorPanelHelper(
            pt,
            kcl::PopupContainer::fromTopLeft,
            kcl::ColorPanel::ModeLine,
            kcl::ColorPanelLayout(kcl::ColorPanelLayout::NoColorButton,
                                  kcl::ColorPanelLayout::ColorGrid),
            *wxBLUE,
            format_toolbar,
            evt.GetId());
    }
     else
    {
        wxAuiToolBarItem* item = format_toolbar->FindTool(evt.GetToolId());
        postColorSelectedEvent(g_app->getMainFrame()->getActiveChild(), item);
    }
}

void AppController::onRelationshipSyncDropDown(wxAuiToolBarEvent& evt)
{
    wxFrame* main_window = g_app->getMainWindow();
    int command;
    
    m_linkbar->SetToolSticky(ID_App_ToggleRelationshipSync, true);

    wxMenu menuPopup;
    menuPopup.AppendRadioItem(9800, _("&No Relationship Filter or Mark"));
    menuPopup.AppendRadioItem(9801, _("&Filter Related Records"));
    menuPopup.AppendRadioItem(9802, _("&Mark Related Records"));

    switch (m_relationship_sync)
    {
        case tabledocRelationshipSyncNone: menuPopup.Check(9800, true); break;
        case tabledocRelationshipSyncFilter: menuPopup.Check(9801, true); break;
        case tabledocRelationshipSyncSeek: menuPopup.Check(9802, true); break;
    }

    // the dropdown arrow was clicked, show the view choices to the user
    wxRect rect = m_linkbar->GetToolRect(ID_App_ToggleRelationshipSync);
    wxPoint pt(rect.GetLeft(), rect.GetBottom());

    pt = m_linkbar->ClientToScreen(pt);
    pt = main_window->ScreenToClient(pt);

    CommandCapture* cc = new CommandCapture;
    main_window->PushEventHandler(cc);
    main_window->PopupMenu(&menuPopup, pt);
    command = cc->getLastCommandId();
    main_window->PopEventHandler(true);

    // make sure the button is "un-stuck"
    m_linkbar->SetToolSticky(ID_App_ToggleRelationshipSync, false);
    
    
    switch (command)
    {
        default:   return;
        case 9800: setRelationshipSync(tabledocRelationshipSyncNone); break;
        case 9801: setRelationshipSync(tabledocRelationshipSyncFilter); break;
        case 9802: setRelationshipSync(tabledocRelationshipSyncSeek); break;
    }

    // set the relationship preference
    wxString synctype = wxT("none");
    if (m_relationship_sync == tabledocRelationshipSyncNone)
        synctype = wxT("none");
    if (m_relationship_sync == tabledocRelationshipSyncFilter)
        synctype = wxT("filter");
    if (m_relationship_sync == tabledocRelationshipSyncSeek)
        synctype = wxT("seek");

    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    prefs->setString(wxT("general.relationship.synctype"), synctype);
}

void AppController::onToolbarRightClick(wxAuiToolBarEvent& evt)
{
    onFrameBarRightClick();
}

void AppController::onSetFindCtrlFocus(wxCommandEvent& evt)
{
    // select the text in the find control
    m_project_toolbar->m_find->SetSelection(0,-1);
    
    // set the focus to the find control
    m_project_toolbar->m_find->SetFocus();
}

void AppController::onFindPrev(wxCommandEvent& evt)
{
    wxString find_str = m_project_toolbar->m_find->GetValue();
    if (find_str.IsEmpty())
        return;
    
    // fire an event to add this string to the find combobox dropdowns
    FrameworkEvent* cfw_evt = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_ADD_FIND_COMBO_ITEM);
    cfw_evt->s_param = find_str;
    g_app->getMainFrame()->postEvent(cfw_evt);

    IDocumentSitePtr doc_site = g_app->getMainFrame()->getActiveChild();
    if (doc_site.isNull())
        return;
    
    IDocumentPtr doc = doc_site->getDocument();
    if (doc.isNull())
        return;
    
    IFindTargetPtr find_target = doc;
    if (find_target.isNull())
        return;
    
    bool match_case, whole_word;
    getAppPrefsFindMatchCase(&match_case);
    getAppPrefsFindMatchWholeWord(&whole_word);

    find_target->findNextMatch(find_str,
                               false /* forward */,
                               match_case /* match_case */,
                               whole_word /* bool whole */);
}

void AppController::onFindNext(wxCommandEvent& evt)
{
    wxString find_str = m_project_toolbar->m_find->GetValue();
    if (find_str.IsEmpty())
        return;
    
    // fire an event to add this string to the find combobox dropdowns
    FrameworkEvent* cfw_evt = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_ADD_FIND_COMBO_ITEM);
    cfw_evt->s_param = find_str;
    g_app->getMainFrame()->postEvent(cfw_evt);

    IDocumentSitePtr doc_site = g_app->getMainFrame()->getActiveChild();
    if (doc_site.isNull())
        return;
    
    IDocumentPtr doc = doc_site->getDocument();
    if (doc.isNull())
        return;
    
    IFindTargetPtr find_target = doc;
    if (find_target.isNull())
        return;
    
    bool match_case, whole_word;
    getAppPrefsFindMatchCase(&match_case);
    getAppPrefsFindMatchWholeWord(&whole_word);

    find_target->findNextMatch(find_str,
                               true  /* forward */,
                               match_case /* match_case */,
                               whole_word /* bool whole */);
}

void AppController::onMergeTable(wxCommandEvent& evt)
{   
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("MergePanel"));
    if (site.isNull())
    {
        MergePanel* panel = new MergePanel;
        site = g_app->getMainFrame()->createSite(panel, sitetypeModeless, -1, -1, 460, 420);
        site->setMinSize(460,420);
        site->setName(wxT("MergePanel"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

void AppController::onRelationshipManager(wxCommandEvent& evt)
{
    showRelationshipManager();
}

void AppController::updateTableDocRelationshipSync(int tabledoc_sync_state)
{
    // iterate through document sites, and update tabledocs
    IDocumentSiteEnumPtr docsites;
    IDocumentSitePtr site;
    ITableDocPtr table_doc;

    docsites = g_app->getMainFrame()->getDocumentSites(sitetypeNormal);

    size_t i, site_count = docsites->size();
    for (i = 0; i < site_count; ++i)
    {
        site = docsites->getItem(i);
        table_doc = site->getDocument();
        if (table_doc.isOk())
        {
            table_doc->setRelationshipSync(tabledoc_sync_state);
        }
    }
}

void AppController::onSetRelationshipSync(wxCommandEvent& evt)
{
    switch (evt.GetId())
    {
        default:
        case ID_App_RelationshipSyncNone:
            setRelationshipSync(tabledocRelationshipSyncNone);
            break;

        case ID_App_RelationshipSyncFilter:
            setRelationshipSync(tabledocRelationshipSyncFilter);
            break;

        case ID_App_RelationshipSyncSeek:
            setRelationshipSync(tabledocRelationshipSyncSeek);
            break;
    }
}

void AppController::onSplitTable(wxCommandEvent& evt)
{
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("DividePanel"));
    if (site.isNull())
    {
        SplitPanel* panel = new SplitPanel;
        site = g_app->getMainFrame()->createSite(panel, sitetypeModeless, -1, -1, 420, 360);
        site->setMinSize(420,360);
        site->setName(wxT("DividePanel"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

void AppController::onRemoveDupRecs(wxCommandEvent& evt)
{
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("RemoveDuplicateRecordsWizard"));
    if (site.isNull())
    {
        RemoveDupRecWizard* wizard = new RemoveDupRecWizard;
        site = g_app->getMainFrame()->createSite(wizard, sitetypeModeless, -1, -1, 540, 440);
        site->setMinSize(540,440);
        site->setName(wxT("RemoveDuplicateRecordsWizard"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

void AppController::onCascade(wxCommandEvent& evt)
{
    m_frame->cascade();
}

void AppController::onTileHorizontal(wxCommandEvent& evt)
{
    m_frame->tileHorizontal();
}

void AppController::onTileVertical(wxCommandEvent& evt)
{
    m_frame->tileVertical();
}

void AppController::onNextChild(wxCommandEvent& evt)
{
    m_frame->goNextChild();
}

void AppController::onPreviousChild(wxCommandEvent& evt)
{
    m_frame->goPreviousChild();
}

void AppController::onCurrentChild(wxCommandEvent& evt)
{
    // sets the focus back to the current child window; useful
    // when entering commands in the console and it's necessary
    // to get back to the main document quickly using a hotkey

    IDocumentSitePtr active_child_site = g_app->getMainFrame()->getActiveChild();

    if (active_child_site.isOk())
    {
        IDocumentPtr active_child_doc = active_child_site->getDocument();
        if (active_child_doc.isOk())
            active_child_doc->setDocumentFocus();
    }
}

void AppController::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void AppController::onUpdateUI_EnableIfNoChildren(wxUpdateUIEvent& evt)
{
    if (m_frame.isOk())
    {
        evt.Enable(m_frame->getChildCount() == 0 ? true : false);
    }
}

void AppController::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}

void AppController::onUpdateUI_DisableIfNoChildren(wxUpdateUIEvent& evt)
{
    if (m_frame.isOk())
    {
        evt.Enable(m_frame->getChildCount() > 0 ? true : false);
    }
}

void AppController::onUpdateUI_DisableIfNoProject(wxUpdateUIEvent& evt)
{
    evt.Enable(g_app->isDatabaseOpen() ? true : false);
}

void AppController::onUpdateUI_DisableIfNoJobs(wxUpdateUIEvent& evt)
{
    evt.Enable(g_app->getJobsActive() ? true : false);
}

void AppController::onUpdateUI_ToggleFullScreen(wxUpdateUIEvent& evt)
{
    if (m_frame.isNull())
        return;

    evt.Check(m_frame->getFrameWindow()->IsFullScreen());
}

void AppController::onUpdateUI_ToggleToolbar(wxUpdateUIEvent& evt)
{
    if (m_frame.isNull())
        return;
    
    wxWindowID wnd_id = -1;
    
    switch (evt.GetId())
    {
        case ID_Frame_ToggleStandardToolbar: wnd_id = ID_Toolbar_Standard; break;
        case ID_Frame_ToggleLinkToolbar:     wnd_id = ID_Toolbar_Link; break;
        case ID_Frame_ToggleFormatToolbar:   wnd_id = ID_Toolbar_Format; break;
        default:
            return;
    }
    
    wxWindow* wnd = m_frame->findWindow(wnd_id);
    if (!wnd)
        return;
    
    evt.Check(wnd->IsShown());
}

void AppController::onUpdateUI_ToggleStatusBar(wxUpdateUIEvent& evt)
{
    if (m_frame.isNull())
        return;
    
    IStatusBarPtr statusbar = m_frame->getStatusBar();
    if (statusbar.isOk())
    {
        wxAuiToolBar* wnd = statusbar->getStatusBarCtrl();
        evt.Check(wnd->IsShown());
        return;
    }
    
    evt.Check(false);
}

void AppController::onUpdateUI_ToggleProjectPanel(wxUpdateUIEvent& evt)
{
    if (m_frame.isNull())
        return;
    
    if (!m_dbdoc)
        return;
    
    bool check = false;
    
    IDocumentSitePtr site;
    site = m_dbdoc->getDbDocSite();
    if (site.isOk())
        check = (site->getVisible() ? true : false);
    
    evt.Check(check);
}

void AppController::onUpdateUI_ToggleConsolePanel(wxUpdateUIEvent& evt)
{
    if (m_frame.isNull())
        return;
    
    bool check = false;
    
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("ConsolePanel"));
    if (site.isOk())
        check = (site->getVisible() ? true : false);
    
    evt.Check(check);
}

void AppController::onUpdateUI_ToggleColumnListPanel(wxUpdateUIEvent& evt)
{
    if (m_frame.isNull())
        return;
    
    bool check = false;
    
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("FieldsPanel"));
    if (site.isOk())
        check = (site->getVisible() ? true : false);
    
    evt.Check(check);
}

void AppController::onUpdateUI_ToggleMarkManagerPanel(wxUpdateUIEvent& evt)
{
    if (m_frame.isNull())
        return;
    
    bool check = false;
    
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("MarksPanel"));
    if (site.isOk())
        check = (site->getVisible() ? true : false);
    
    evt.Check(check);
}

void AppController::onUpdateUI_ToggleRelationshipPanel(wxUpdateUIEvent& evt)
{
    if (m_frame.isNull())
        return;
    
    bool check = false;
    
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("RelationshipsPanel"));
    if (site.isOk())
        check = (site->getVisible() ? true : false);
    
    evt.Check(check);
}

void AppController::onUpdateUI_ToggleLock(wxUpdateUIEvent& evt)
{
    evt.Check(m_data_locked);
}

void AppController::onUpdateUI_RelationshipSync(wxUpdateUIEvent& evt)
{
    int synctype = getRelationshipSync();
    switch (evt.GetId())
    {
        case ID_App_RelationshipSyncNone:
            synctype == tabledocRelationshipSyncNone ? evt.Check(true) : evt.Check(false);
            break;
            
        case ID_App_RelationshipSyncFilter:
            synctype == tabledocRelationshipSyncFilter ? evt.Check(true) : evt.Check(false);
            break;
            
        case ID_App_RelationshipSyncSeek:
            synctype == tabledocRelationshipSyncSeek ? evt.Check(true) : evt.Check(false);
            break;
    }
}

void AppController::onUpdateUI_ViewSource(wxUpdateUIEvent& evt)
{
    if (!m_frame)
        return;
        
    IDocumentSitePtr doc_site = m_frame->getActiveChild();
    if (!doc_site)
        return;
        
    IDocumentPtr doc = doc_site->getDocument();
    IWebDocPtr webdoc = doc;
    ITextDocPtr textdoc = doc;
    ISqlDocPtr sqldoc = doc;
    
    if (sqldoc.isOk())
        evt.Check(true);
    
    if (textdoc.isOk())
        evt.Check(true);
    
    if (webdoc.isOk())
        evt.Check(webdoc->isViewingSource());
}

void AppController::onUpdateUI_ViewDesign(wxUpdateUIEvent& evt)
{
    if (!m_frame)
        return;
        
    IDocumentSitePtr doc_site = m_frame->getActiveChild();
    if (!doc_site)
        return;
        
    IDocumentPtr doc = doc_site->getDocument();
    IReportDocPtr reportdoc = doc;
    ITransformationDocPtr transdoc = doc;
    IStructureDocPtr structdoc = doc;
    IQueryDocPtr querydoc = doc;
    
    evt.Check(transdoc.isOk() ||
              structdoc.isOk() ||
              querydoc.isOk());

    if (reportdoc.isOk())
        evt.Check(reportdoc->isDesignView());
}

void AppController::onUpdateUI_ViewLayout(wxUpdateUIEvent& evt)
{
    if (!m_frame)
        return;
        
    IDocumentSitePtr doc_site = m_frame->getActiveChild();
    if (!doc_site)
        return;
        
    IDocumentPtr doc = doc_site->getDocument();
    IWebDocPtr webdoc = doc;
    IReportDocPtr reportdoc = doc;
    ITableDocPtr tabledoc = doc;
    
    evt.Check(tabledoc.isOk());
    
    if (webdoc)
        evt.Check(!webdoc->isViewingSource());
    
    if (reportdoc)
        evt.Check(!reportdoc->isDesignView());
}

void AppController::onUpdateUI_StartRecord(wxUpdateUIEvent& evt)
{
    // don't allow recording if a project isn't open
    evt.Enable(g_app->isDatabaseOpen() ? true : false);
}

void AppController::onUpdateUI_StopRecord(wxUpdateUIEvent& evt)
{
    // enable if a database is open and we're recording
    evt.Enable(g_app->isDatabaseOpen() && g_macro.isRecording() ? true : false);
}

void AppController::onStatusBarItemLeftClick(IStatusBarItemPtr item)
{
    wxString name = item->getName();
    
    if (name == wxT("app_toggle_projectpanel"))
        toggleProjectPanel();
     else if (name == wxT("app_toggle_fieldspanel"))
        toggleColumnListPanel();
     else if (name == wxT("app_toggle_markspanel"))
        toggleMarkManagerPanel();
     else if (name == wxT("app_toggle_relationshipspanel"))
        toggleRelationshipPanel();
     else if (name == wxT("app_job_failed"))
        showJobManager();
}

void AppController::onStatusBarItemLeftDblClick(IStatusBarItemPtr item)
{
    wxString name = item->getName();
    
    if (name == wxT("app_job_gauge") || name == wxT("app_job_text"))
        showJobManager();
}

void AppController::onStatusBarRefresh()
{
    // NOTE: use care when adding calls to this function as it
    //       is called _EVERY_ time the statusbar refreshes itself
    
    // there are no active jobs
    if (!g_app->getJobQueue()->getJobsActive())
    {
        // restore the statusbar items to their default state
        if (!m_doc_text_item->isShown())
        {
            m_job_text_item->setProportion(0);
            m_job_separator_item->show(false);
            m_doc_text_item->show(true);
            m_frame->getStatusBar()->populate();
        }
        
        // no need to do anything
        return;
    }
    
    // a job is running and there is no text in the
    // main application statusbar area; make the job text
    // take up all of the statusbar's "stretchable" area
    if (m_doc_text_item->isShown() &&
        m_doc_text_item->getValue().IsEmpty())
    {
        m_job_text_item->setProportion(1);
        m_job_separator_item->show(false);
        m_doc_text_item->show(false);
        m_frame->getStatusBar()->populate();
        return;
    }
    
    // a job is running and there is also text in the
    // main application statusbar area; show both the job text
    // and the application text in the statusbar's "stretchable" area
    if (!m_doc_text_item->isShown() &&
        m_doc_text_item->getValue().Length() > 0)
    {
        m_job_text_item->setProportion(0);
        m_job_separator_item->show(true);
        m_doc_text_item->show(true);
        m_frame->getStatusBar()->populate();
        return;
    }
}

void AppController::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == FRAMEWORK_EVT_APPMAIN_ADD_FIND_COMBO_ITEM)
    {
        wxString str = evt.s_param;
        wxBitmap bitmap = GETBMP(gf_find_16);
        
        // if the find/filter string doesn't exist in the find combobox, add it
        if (m_project_toolbar->m_find->FindString(str) == wxNOT_FOUND &&
            !str.IsEmpty())
        {
            m_project_toolbar->m_find->Insert(str, bitmap, 0);
        }
        
        // update the standard toolbar
        m_project_toolbar->m_find->SetValue(str);
    }
     else if (evt.name == FRAMEWORK_EVT_CFW_LOCATION_CHANGED)
    {
        updateURLToolbar();
    }
     else if (evt.name == FRAMEWORK_EVT_APPMAIN_DOC_BITMAP_UPDATED)
    {
        wxBitmap bitmap = *((wxBitmap*)evt.o_param);
        int idx = m_project_toolbar->m_url->GetSelection();
        m_project_toolbar->m_url->SetItemBitmap(idx, bitmap);
    }
     else if (evt.name == FRAMEWORK_EVT_APPMAIN_PREFERENCES_SAVED)
    {
        doPreferenceRefresh();
    }
}


void AppController::onFrameClose(wxCloseEvent& evt)
{
    if (!evt.CanVeto())
    {
        m_frame->closeAll(false);
        closeProject();
        return;
    }
    

    int ID_ExitApp = 100;
    int ID_RunBackgroundJobs = 101;
    
    // check for scheduled jobs

    std::vector<JobSchedulerEntry> jobs = g_app->getJobScheduler()->getJobs();
    if (jobs.size() > 0)
    {
        bool background_jobs_active = false;
        
        std::vector<JobSchedulerEntry>::iterator job_it;
        for (job_it = jobs.begin(); job_it != jobs.end(); ++job_it)
        {
            if (job_it->active)
            {
                background_jobs_active = true;
                break;
            }
        }
        
        if (background_jobs_active)
        {
            #ifdef WIN32
                wxString appname = APPLICATION_NAME;
                wxString message = wxString::Format(_("There are currently jobs scheduled to run.  If you choose to exit %s, these jobs will not run as scheduled.  Would you like to run %s in the background or exit?"), appname.c_str(), appname.c_str());
                CustomPromptDlg dlg(m_frame->getFrameWindow(),
                                    APPLICATION_NAME,
                                    message);
                dlg.setButton1(ID_RunBackgroundJobs, _("Run Jobs in Background"));
                dlg.setButton2(ID_ExitApp, _("Exit"));
                dlg.showButtons(CustomPromptDlg::showButton1 |
                                CustomPromptDlg::showButton2);
                
                int result = dlg.ShowModal();
                if (result == ID_RunBackgroundJobs)
                {
                    g_app->showApp(false);
                    evt.Veto();
                    return;
                }
                 else if (result == wxID_CANCEL)
                {
                    evt.Veto();
                    return;
                }
                
            #else
                wxString appname = APPLICATION_NAME;
                wxString message = wxString::Format(_("There are currently jobs scheduled to run.  If you choose to exit %s, these jobs will not run as scheduled.  Are you sure you want to exit %s?"), appname.c_str(), appname.c_str());
                
                if (wxYES == appMessageBox(message,
                                                APPLICATION_NAME,
                                                wxYES_NO))
                {
                    return;
                }
                
                evt.Veto();
                return;
            #endif
        }
    }


    if (!checkForRunningJobs(true))
    {
        evt.Veto();
        return;
    }

    if (m_frame.isNull())
        return;

    // now close all
    if (!m_frame->closeAll(false))
    {
        evt.Veto();
        return;
    }

    if (!closeProject())
    {
        evt.Veto();
        return;
    }
}

void AppController::onFrameSize(wxSizeEvent& evt)
{
    wxFrame* frame_wnd = g_app->getMainWindow();
    bool maximized = frame_wnd->IsMaximized();
    
    IStatusBarPtr statusbar = g_app->getMainFrame()->getStatusBar();
    if (statusbar.isNull())
        return;
    
    if (maximized)
        statusbar->showResizeGripper(false);
         else
        statusbar->showResizeGripper(true);
}

void AppController::onFrameDestroy()
{ 
    wxFrame* frame_wnd = g_app->getMainWindow();

    if (frame_wnd->IsIconized())
    {
        frame_wnd->Iconize(false);
    }

    if (m_hidden_sites.size() > 0)
    {
        toggleAllPanels();
    }


    wxAuiManager& mgr = m_frame->getAuiManager();
    std::wstring perspective = towstr(mgr.SavePerspective());
    

    // main window dimensions

    wxPoint pos = frame_wnd->GetPosition();
    wxSize size = frame_wnd->GetSize();
    bool maximized = frame_wnd->IsMaximized();

    IAppConfigPtr config = g_app->getAppConfig();
    config->setPath(L"/WindowDimensions");
    config->write(L"XPos",                    (long)pos.x);
    config->write(L"YPos",                    (long)pos.y);
    config->write(L"Width",                   (long)size.GetWidth());
    config->write(L"Height",                  (long)size.GetHeight());
    config->write(L"Maximized",               maximized);
    config->write(L"MainPerspective",         perspective);
    config->write(L"ClearPerspectiveCounter", (long)ClearPerspectiveCounter);


    // give extensions a chance to stop
    frame_wnd->Show(false);
    g_app->getExtensionMgr()->removeUninstalledExtensions();

    // remove event handlers
    apphookUninitFrame();
    m_frame->removeWindowEventHandler(m_script_command_router);
    m_frame->removeWindowEventHandler(this);

    // free up application objects
    if (m_dbdoc)
    {
        delete m_dbdoc;
        m_dbdoc = NULL;
    }

    m_frame.clear();
    m_columnlistpanel_site.clear();
    m_markmanagerpanel_site.clear();
    
    g_app->setMainFrame(xcm::null);


    // around Summer 2007 we had a problem on wxMSW where appmain.exe would
    // not exit right away on certain machines -- maybe about 1/3 of
    // machines.  Furthermore, on these machines, it only happened
    // about 1 out of every 5 times.  The call below prevents this
    // from happening.  It could be a wxWidgets problem.  Also likely
    // is a cfw mutex could be interfering with the shutdown procedure.
    // It looks like the event loop refuses to shut down.  If we want to
    // investigate this problem more later, we can by commenting out the
    // line below.

    // So this line takes care of the problem.
    wxTheApp->ExitMainLoop();
}


void AppController::onFrameBarRightClick()
{
    wxMenu menuPopup;
    menuPopup.AppendCheckItem(ID_Frame_ToggleStandardToolbar, _("&Navigation Toolbar"));
    menuPopup.AppendCheckItem(ID_Frame_ToggleLinkToolbar, _("&Bookmarks Toolbar"));
    menuPopup.AppendCheckItem(ID_Frame_ToggleFormatToolbar, _("F&ormat Toolbar"));
    menuPopup.AppendSeparator();
    menuPopup.AppendCheckItem(ID_Frame_ToggleProjectPanel, _("&Project Panel"));
    menuPopup.AppendCheckItem(ID_Frame_ToggleConsolePanel, _("&Console Panel"));
    menuPopup.AppendCheckItem(ID_Frame_ToggleColumnListPanel, _("&Fields Panel"));
    menuPopup.AppendCheckItem(ID_Frame_ToggleMarkManagerPanel, _("&Marks Panel"));
    menuPopup.AppendCheckItem(ID_Frame_ToggleRelationshipManagerPanel, _("&Relationships Panel"));
    menuPopup.AppendSeparator();
    menuPopup.AppendCheckItem(ID_Frame_ToggleStatusBar, _("&Status Bar"));
    menuPopup.AppendSeparator();
    menuPopup.AppendCheckItem(ID_Frame_ToggleFullScreen, _("F&ull Screen"));

    wxPoint pt = ::wxGetMousePosition();
    pt = g_app->getMainWindow()->ScreenToClient(pt);

    g_app->getMainWindow()->PopupMenu(&menuPopup, pt);
}

void AppController::updateTitle()
{
    wxString title;
    
    // update the frame's titlebar
    IDocumentSitePtr doc_site = g_app->getMainFrame()->getActiveChild();
    if (doc_site.isNull())
    {
        title = m_frame_caption;
    }
     else
    {
        IDocumentPtr doc = doc_site->getDocument();
        IWebDocPtr webdoc = doc;
        if (webdoc.isOk())
        {
            // show the web document's title in the titlebar
            title = doc->getDocumentTitle();
            if (title.Length() > 0)
                title += wxT(" - ");
            title += m_frame_caption;
        }
         else
        {
            title = m_frame_caption;
        }
    }
    

    wxFrame* wnd = g_app->getMainWindow();
    if (wnd && wnd->GetTitle() != title)
    {
        wnd->SetTitle(title);
    }
}

void AppController::updateURLToolbar()
{
    if (!m_frame)
        return;
        
    wxString name;
    wxBitmap bitmap;
    
    // get the name of the child
    IDocumentSitePtr doc_site = m_frame->getActiveChild();
    if (doc_site.isOk())
    {
        bitmap = doc_site->getBitmap();
        
        IDocumentPtr doc = doc_site->getDocument();
        if (doc.isOk())
            name = doc->getDocumentLocation();
        
        // this is a temporary fix so that the spinning progress
        // icon doesn't show up as the document icon in the URL bar
        IWebDocPtr webdoc = doc;
        if (webdoc.isOk())
            bitmap = GETBMP(gf_document_16);
    }
    
    // if it doesn't exist in the url bar, add it
    if (m_project_toolbar->m_url->FindString(name) == wxNOT_FOUND &&
        name.CmpNoCase(wxT("about:blank")) != 0 && !name.IsEmpty())
    {
        m_project_toolbar->m_url->Insert(name, bitmap, 0);
    }
    
    // update the standard toolbar
    m_project_toolbar->m_url->SetValue(name);
}

void AppController::updateQuickFilterToolBarItem()
{
    wxASSERT(kl::Thread::isMain());

    if (!m_frame)
    {
        m_project_toolbar->ToggleTool(ID_Data_QuickFilter, false);
        m_project_toolbar->Refresh();
        return;
    }
    
    // get the name of the child
    IDocumentSitePtr doc_site = m_frame->getActiveChild();
    if (doc_site.isNull())
    {
        m_project_toolbar->ToggleTool(ID_Data_QuickFilter, false);
        m_project_toolbar->Refresh();
        return;
    }
    
    IDocumentPtr doc = doc_site->getDocument();
    if (doc.isNull())
    {
        m_project_toolbar->ToggleTool(ID_Data_QuickFilter, false);
        m_project_toolbar->Refresh();
        return;
    }
    
    ITableDocPtr tabledoc = doc;
    if (tabledoc.isNull())
    {
        m_project_toolbar->ToggleTool(ID_Data_QuickFilter, false);
        m_project_toolbar->Refresh();
        return;
    }
    
    wxString filter = tabledoc->getFilter();
    bool quick_filter_pending = tabledoc->isQuickFilterPending();

    if (filter.IsEmpty() && ! quick_filter_pending)
    {
        m_project_toolbar->ToggleTool(ID_Data_QuickFilter, false);
        m_project_toolbar->Refresh();
        return;
    }
    
    m_project_toolbar->ToggleTool(ID_Data_QuickFilter, true);
    m_project_toolbar->Refresh();
}

void AppController::updateViewMenu(IDocumentSitePtr doc_site)
{
    if (!m_view_menu)
        return;
        
    // remove old items from the view menu
    
    if (m_view_menu->FindItem(ID_View_SwitchToSourceView) != NULL)
        m_view_menu->Delete(ID_View_SwitchToSourceView);
    if (m_view_menu->FindItem(ID_View_SwitchToDesignView) != NULL)
        m_view_menu->Delete(ID_View_SwitchToDesignView);
    if (m_view_menu->FindItem(ID_View_SwitchToLayoutView) != NULL)
        m_view_menu->Delete(ID_View_SwitchToLayoutView);
    wxMenuItem* item = m_view_menu->FindItemByPosition(0);
    if (item->IsSeparator())
        m_view_menu->Delete(item);
    
    // we don't have an active document, bail out
    if (doc_site.isNull())
        return;
    
    // the editor doc has only one view
    IEditorDocPtr editordoc = doc_site->getDocument();
    if (editordoc.isOk())
        return;
    
    // add items to the menu based on the active view
    
    IWebDocPtr webdoc = doc_site->getDocument();
    IReportDocPtr reportdoc = doc_site->getDocument();
    ITextDocPtr textdoc = lookupOtherDocument(doc_site, "appmain.TextDoc");
    ITransformationDocPtr transdoc = lookupOtherDocument(doc_site, "appmain.TransformationDoc");
    ITableDocPtr tabledoc = lookupOtherDocument(doc_site, "appmain.TableDoc");
    IStructureDocPtr structdoc = lookupOtherDocument(doc_site, "appmain.StructureDoc");
    IQueryDocPtr querydoc = lookupOtherDocument(doc_site, "appmain.QueryDoc");
    
    if (webdoc.isOk())
    {
        m_view_menu->PrependSeparator();
        m_view_menu->InsertCheckItem(0, ID_View_SwitchToSourceView, _("Source View"));
        m_view_menu->InsertCheckItem(1, ID_View_SwitchToLayoutView, _("Web Page View"));
        return;
    }
    
    if (reportdoc.isOk())
    {
        m_view_menu->PrependSeparator();
        m_view_menu->InsertCheckItem(0, ID_View_SwitchToDesignView, _("Design View"));
        m_view_menu->InsertCheckItem(1, ID_View_SwitchToLayoutView, _("Layout View"));
        return;
    }
    
    if (textdoc.isOk() || transdoc.isOk())
    {
        m_view_menu->PrependSeparator();
        m_view_menu->InsertCheckItem(0, ID_View_SwitchToSourceView, _("Source View"));
        m_view_menu->InsertCheckItem(1, ID_View_SwitchToDesignView, _("Design View"));
        m_view_menu->InsertCheckItem(2, ID_View_SwitchToLayoutView, _("Table View"));
        return;
    }
    
    if (structdoc.isOk())
    {
        m_view_menu->PrependSeparator();
        m_view_menu->InsertCheckItem(0, ID_View_SwitchToDesignView, _("Design View"));
        m_view_menu->InsertCheckItem(1, ID_View_SwitchToLayoutView, _("Table View"));
        return;
    }
    
    if (querydoc.isOk())
    {
        m_view_menu->PrependSeparator();
        m_view_menu->InsertCheckItem(0, ID_View_SwitchToSourceView, _("SQL View"));
        m_view_menu->InsertCheckItem(1, ID_View_SwitchToDesignView, _("Design View"));
        m_view_menu->InsertCheckItem(2, ID_View_SwitchToLayoutView, _("Table View"));
        return;
    }
    
    // the tabledoc was opened by itself (no StructureDoc is available yet),
    // so populate the menu with the design and table items
    if (tabledoc.isOk())
    {
        m_view_menu->PrependSeparator();
        m_view_menu->InsertCheckItem(0, ID_View_SwitchToDesignView, _("Design View"));
        m_view_menu->InsertCheckItem(1, ID_View_SwitchToLayoutView, _("Table View"));
    }
}

void AppController::onActiveChildChanged(IDocumentSitePtr doc_site)
{
    updateTitle();
    updateURLToolbar();
    updateQuickFilterToolBarItem();
    updateViewMenu(doc_site);
}


void AppController::onFrameChildContextMenu(IDocumentSitePtr doc_site)
{
    if (doc_site.isNull())
        return;

    wxMenu menuPopup;
    menuPopup.Append(8000, _("&New Tab"));
    
    // allow the tab to be duplicated, as long as we're not on an editordoc
    IEditorDocPtr editor_doc = doc_site->getDocument();
    if (editor_doc.isNull())
    {
        menuPopup.Append(8001, _("&Duplicate Tab"));
    }
    
    menuPopup.AppendSeparator();
    menuPopup.Append(8002, _("&Refresh Tab"));
    menuPopup.Append(8003, _("Refresh &All Tabs"));
    menuPopup.AppendSeparator();
    menuPopup.Append(8004, _("&Bookmark This Location..."));
    menuPopup.AppendSeparator();
    menuPopup.Append(8005, _("&Close Tab"));
    menuPopup.Append(8006, _("Close &Other Tabs"));

    wxFrame* main_window = g_app->getMainWindow();
    int command;
    
    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = main_window->ScreenToClient(pt_mouse);
    CommandCapture* cc = new CommandCapture;
    main_window->PushEventHandler(cc);
    main_window->PopupMenu(&menuPopup, pt_mouse);
    command = cc->getLastCommandId();
    main_window->PopEventHandler(true);

    IFramePtr frame = g_app->getMainFrame();
    
    switch (command)
    {
        case (8000):
            {
                // open a new tab
                wxCommandEvent evt;
                onNewTab(evt);
            }
            break;
        case (8001):
            {
                // duplicate the tab
                IDocumentPtr doc = doc_site->getDocument();

                IWebDocPtr web_doc = doc;
                if (web_doc.isOk())
                {
                    openWeb(doc->getDocumentLocation(), NULL,
                            appOpenForceNewWindow | appOpenActivateNewWindow);
                }
                 else
                {
                    openAny(doc->getDocumentLocation());
                }
            }
            break;
        case (8002):
            {
                // refresh the tab
                IWebDocPtr web_doc = doc_site->getDocument();
                if (!web_doc.isNull())
                    web_doc->getWebControl()->Reload();
            }
            break;
        case (8003):
            {
                // refresh all tabs
                IDocumentSitePtr site;
                IDocumentSiteEnumPtr sites = frame->getDocumentSites(sitetypeNormal);
                size_t i, site_count = sites->size();

                for (i = 0; i < site_count; ++i)
                {
                    site = sites->getItem(i);
                    if (site.isNull())
                        continue;
                        
                    IWebDocPtr web_doc = site->getDocument();
                    if (!web_doc.isNull())
                        web_doc->getWebControl()->Reload();
                }                
            }
            break;
        case (8004):
            {
                // create a bookmark
                // TODO: this will bookmark the active tab; however, the other
                // tab menu items (such as close), work on the tabs that are
                // clicked, not on the active tab; need to make this consistent
                wxCommandEvent evt;
                onCreateBookmark(evt);
            }
            break;
        case (8005):
            {
                // close this tab
                g_app->getMainFrame()->closeSite(doc_site);
            }
            break;
        case (8006):
            {
                // close the other tabs
                IDocumentSitePtr site;
                IDocumentSiteEnumPtr sites = frame->getDocumentSites(sitetypeNormal);
                size_t i, site_count = sites->size();

                for (i = 0; i < site_count; ++i)
                {
                    site = sites->getItem(i);
                    if (site.isNull())
                        continue;
                        
                    if (site != doc_site)
                        frame->closeSite(site);
                }
            }
            break;
    }
}

void AppController::onBorderPanelDestructing(kcl::BorderPanel* panel)
{
    if (m_borderpanel_helper)
    {
        delete m_borderpanel_helper;
        m_borderpanel_helper = NULL;
    }
}

void AppController::onColorPanelDestructing(kcl::ColorPanel* panel)
{
    if (m_colorpanel_helper)
    {
        delete m_colorpanel_helper;
        m_colorpanel_helper = NULL;
    }
}

void AppController::onCustomColorsRequested(std::vector<wxColor>& colors)
{
    getAppPrefsCustomColors(colors);
    
    if (m_colorpanel_helper != NULL)
    {
        wxAuiToolBar* toolbar = m_colorpanel_helper->m_toolbar;
        int tool_id = m_colorpanel_helper->m_tool_id;
        
        if (toolbar != NULL && tool_id != -1)
            toolbar->SetToolSticky(tool_id, false);
    }
}

void AppController::onCustomColorsChanged(std::vector<wxColor> colors)
{
    setAppPrefsCustomColors(colors);
}

void AppController::onHelp(wxCommandEvent& evt)
{
    wxHelpControllerBase* hc = g_app->getHelpController();
    if (hc)
    {
        hc->DisplayContents();
    }
     else
    {                 
        // the help file can't be found.  As an alternitave, show
        // the online help in a web browser
        wxString uri = getAppPrefsDefaultString(wxT("general.location.help"));
        openWeb(uri);
    }
}

void AppController::onReference(wxCommandEvent& evt)
{
    // show the online help in a web browser
    wxString uri = getAppPrefsDefaultString(wxT("general.location.help"));
    openWeb(uri);
}

void AppController::onResources(wxCommandEvent& evt)
{
    // show the developer resources in a web browser
    wxString uri = getAppPrefsDefaultString(wxT("general.location.resources"));
    openWeb(uri);
}

void AppController::onSupport(wxCommandEvent& evt)
{
    // show the online help in a web browser
    wxString uri = getAppPrefsDefaultString(wxT("general.location.support"));
    openWeb(uri);
}

void AppController::onContactUs(wxCommandEvent& evt)
{
    // show the online help in a web browser
    wxString uri = getAppPrefsDefaultString(wxT("general.location.contact_us"));
    openWeb(uri);
}

void AppController::onHomeMiddleClick(wxAuiToolBarEvent& evt)
{
    // open the home page(s)
    wxString home_pages = getAppPrefsString(wxT("general.location.home"));
    if (home_pages.IsEmpty())
        home_pages = wxT("about:blank");
        
    bool first = true;
    
    wxArrayString arr;
    wxStringTokenizer tkz(home_pages, wxT("|"));
    while (tkz.HasMoreTokens())
    {
        wxString t = tkz.GetNextToken();
        if (!t.IsEmpty())
        {
            int open_mask = appOpenForceNewWindow;
            if (first)
                open_mask |= appOpenActivateNewWindow;
            
            // the active child is not a webdoc; open all homepages
            // in new tabs and select the first one we open
            openWeb(t, NULL, open_mask);
            first = false;
        }
    }
}

void AppController::onHome(wxCommandEvent& evt)
{
    // open the home page(s)
    wxString home_pages = getAppPrefsString(wxT("general.location.home"));
    if (home_pages.IsEmpty())
        home_pages = wxT("about:blank");

    IDocumentSitePtr docsite = m_frame->getActiveChild();
    IWebDocPtr webdoc;
    if (docsite.isOk())
        webdoc = docsite->getDocument();
    
    bool first = true;

    wxArrayString arr;
    wxStringTokenizer tkz(home_pages, wxT("|"));
    while (tkz.HasMoreTokens())
    {
        wxString t = tkz.GetNextToken();
        if (!t.IsEmpty())
        {
            if (first && webdoc.isOk())
            {
                // the active child is a webdoc; open the first
                // homepage in the existing child frame
                openWeb(t, NULL, appOpenDefault);
                first = false;
            }
             else
            {
                int open_mask = appOpenForceNewWindow;
                if (first)
                    open_mask |= appOpenActivateNewWindow;
                
                // the active child is not a webdoc; open all homepages
                // in new tabs and select the first one we open
                openWeb(t, NULL, open_mask);
                first = false;
            }
        }
    }
}

void AppController::onPrint(wxCommandEvent& evt)
{

}

void AppController::onLicenseManager(wxCommandEvent& evt)
{
    showLicenseManager();
}

void AppController::onCheckForUpdates(wxCommandEvent& evt)
{
    checkForUpdates();
}


void AppController::onConnectExternal(wxCommandEvent& evt)
{
    showCreateExternalConnectionWizard();
}


void AppController::onJobScheduler(wxCommandEvent& evt)
{
    showJobScheduler();
}

void AppController::onJobManager(wxCommandEvent& evt)
{
    showJobManager();
}

void AppController::onExtensionManager(wxCommandEvent& evt)
{
    showExtensionManager();
}

void AppController::onProjectProperties(wxCommandEvent& evt)
{
    showProjectProperties();
}

void AppController::onCleanProject(wxCommandEvent& evt)
{
    cleanProject();
}

void AppController::setRelationshipSync(int new_val)
{
    if (m_relationship_sync == new_val)
        return;

    m_relationship_sync = new_val;
    updateTableDocRelationshipSync(m_relationship_sync);
}

int AppController::getRelationshipSync()
{
    return m_relationship_sync;
}

bool AppController::openAny(const wxString& _location,
                            int open_mask,
                            int* site_id)
{
    if (site_id)
        *site_id = 0;
        
    // trim the location of leading and trailing spaces
    wxString location = _location;
    location.Trim(true);
    location.Trim(false);

    wxString protocol = location.BeforeFirst(wxT(':'));
    protocol.MakeLower();

    if (protocol == wxT("sdserv") || protocol == wxT("sdservs"))
        return openDataLink(location, site_id);

    // if the open as table flag is set, open the location
    // with the table browser
    if (open_mask & appOpenAsTable)
        return openTable(location, site_id);

    // if the open as text flag is set, open the location
    // with the script editor
    if (open_mask & appOpenAsText)
        return openScript(location, site_id);

    // if the open as web flag is set, open the location
    // with the web browser
    if (open_mask & appOpenAsWeb)
        return openWeb(location);


    // get the extension
    wxString ext;
    int ext_pos = location.Find(wxT('.'), true);
    if (ext_pos != -1 && ext_pos > 0)
    {
        if (location[ext_pos-1] != wxT('/'))
            ext = location.Mid(ext_pos+1);
        if (ext.Find(L'/') != -1)
            ext = L"";
    }
  
    ext.MakeUpper();

    // if we have a script/text file, open it with the script/text editor
    if (protocol.Left(4) != wxT("http") &&
        protocol.Left(5) != wxT("https") &&
        (
            ext == wxT("LOG")       ||  // log file
            ext == wxT("S")         ||  // src file
            ext == wxT("SRC")       ||  // src file
            ext == wxT("PRG")       ||  // program file
            ext == wxT("PRF")       ||  // preference file
            ext == wxT("OPT")       ||  // options file
            ext == wxT("INC")       ||  // include file
            ext == wxT("MAC")       ||  // macro file
            ext == wxT("FMT")       ||  // format file

            ext == wxT("BSH")       ||  // shell script
            ext == wxT("CSH")       ||
            ext == wxT("CMD")       ||
            ext == wxT("WS")        ||
            ext == wxT("WSC")       ||
            ext == wxT("WSF")       ||
            ext == wxT("NT")        ||
            ext == wxT("SH")        ||

            ext == wxT("ASP")       ||  // asp
            ext == wxT("ASPX")      ||

            ext == wxT("C")         ||  // c
            ext == wxT("CC")        ||
            ext == wxT("CPP")       ||
            ext == wxT("CS")        ||
            ext == wxT("CXX")       ||
            ext == wxT("H")         ||
            ext == wxT("HH")        ||
            ext == wxT("HPP")       ||
            ext == wxT("HXX")       ||
            ext == wxT("IDL")       ||

            ext == wxT("F")         ||  // fortran
            ext == wxT("FOR")       ||
            ext == wxT("F95")       ||

            ext == wxT("J")         ||  // java         
            ext == wxT("JAVA")      ||

            ext == wxT("JS")        ||  // javscript
            ext == wxT("JSON")      ||
            ext == wxT("JSP")       ||
            ext == wxT("ES")        ||

            ext == wxT("PHP")       ||  // php
            ext == wxT("PHP2")      ||
            ext == wxT("PHP3")      ||
            ext == wxT("PHP4")      ||
            ext == wxT("PHP5")      ||
            ext == wxT("PHTML")     ||
            ext == wxT("MOD")       ||

            ext == wxT("RB")        ||  // ruby
            ext == wxT("RBW")       ||

            ext == wxT("VB")        ||  // visual basic
            ext == wxT("VBP")       ||
            ext == wxT("VBS")       ||

            ext == wxT("BAS")       ||  // basic
            ext == wxT("PAS")       ||  // pascal
            ext == wxT("LUA")       ||  // lua
            ext == wxT("LSP")       ||  // lisp
            ext == wxT("TCL")       ||  // tcl
            ext == wxT("PL")        ||  // perl
            ext == wxT("PY")        ||  // python

            ext == wxT("RUL")       ||  // sql
            ext == wxT("SQL")       ||

            ext == wxT("CSS")       ||  // web
            ext == wxT("CGI")       ||
            ext == wxT("FCGI")      ||
            ext == wxT("HTML")      ||
            ext == wxT("HTA")       ||  // html application
            ext == wxT("LESS")      ||
            ext == wxT("IIM")       ||

            ext == wxT("CFG")       ||  // configuration
            ext == wxT("CONF")      ||
            ext == wxT("CONFIG")    ||
            ext == wxT("CONFIGURE") ||
            ext == wxT("INI")       ||
            ext == wxT("HTACCESS")  ||
            ext == wxT("TPL")       ||  // template file

            ext == wxT("MK")        ||  // makefile
            ext == wxT("MAKE")      ||
            ext == wxT("MSC")
        )
       )
    {
        return openScript(location, site_id);
    }
 
    // if we have an excel file (XLS extension), open a dialog asking
    // the user which tables they would like to open (read-only)
    if (ext == wxT("XLS") || ext == wxT("XLSX"))
        return openExcel(location, site_id);

    // if we have an access file (MDB extension),
    // go directly to the import tables page
    if (ext == wxT("MDB") || ext == wxT("ACCDB"))
        return openAccess(location);

    // if we have a package file (KPG extension),
    // go directly to the import tables page
    if (ext == wxT("KPG"))
        return openPackage(location);

    // somes files should automatically open up in a web browser
    // such as HTML files or image files that we otherwise would
    // never display
    if (protocol.Left(4) == wxT("http") ||
        protocol.Left(5) == wxT("https") ||
        ext == wxT("HTML") ||
        ext == wxT("HTM") ||
        ext == wxT("XML") ||
        ext == wxT("BMP") ||
        ext == wxT("PNG") ||
        ext == wxT("JPEG") ||
        ext == wxT("JPG") ||
        ext == wxT("GIF") ||
        ext == wxT("PDF"))
    {
        return openWeb(location, NULL, appOpenDefault, site_id);
    }


    // try to open all other files as a table
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
    {
        // otherwise open the location in a browser
        return openWeb(location, NULL, appOpenDefault, site_id);
    }
    
    tango::IFileInfoPtr file_info;
    file_info = db->getFileInfo(towstr(location));
    if (file_info.isOk())
    {
        int item_type = file_info->getType();
  
        if (item_type == tango::filetypeSet)
        {
            if (openTable(location, site_id))
                return true;
        }
         else if (item_type == tango::filetypeStream)
        {
            std::wstring mime_type = file_info->getMimeType();
            if (mime_type == L"application/vnd.kx.bookmark")
            {
                if (openBookmark(location, open_mask))
                    return true;
            }
             else if (mime_type.substr(0, 19) == L"application/vnd.kx.")
            {
                if (openTemplate(location, open_mask))
                    return true;
            }
             else
            {
                if (openScript(location, site_id))
                    return true;
            }
        }
         else if (item_type == tango::filetypeNode)
        {
            kl::JsonNode node;
        
            if (!file_info->isMount())
            {
                node = JsonConfig::loadFromDb(g_app->getDatabase(), towstr(location));
            }
             else
            {
                // links to node files must be dereferenced manually.  We
                // should expand the API to include a way of opening node
                // files having the database do the deferencing work
                std::wstring cstr, rpath;
                if (db->getMountPoint(towstr(location), cstr, rpath))
                {
                    if (cstr == L"")
                    {
                        node = JsonConfig::loadFromDb(g_app->getDatabase(), towstr(rpath));
                        location = towx(rpath);
                    }
                }
            }
            
            if (!node.isOk())
                return false;

            kl::JsonNode root_node = node["root"];
            if (!root_node.isOk())
                return false;

            kl::JsonNode bookmark_node = root_node["bookmark"];
            if (bookmark_node.isOk())
                return openBookmark(location, open_mask, site_id);

            kl::JsonNode kpp_script_node = root_node["kpp_script"];
            if (kpp_script_node.isOk())
                return openScript(location, site_id);

            kl::JsonNode kpp_report = root_node["kpp_report"];
            if (kpp_report.isOk())
                return openReport(location, open_mask);

            kl::JsonNode kpp_template = root_node["kpp_template"];
            if (kpp_template.isOk())
                return openTemplate(location);
        }    
    }
    
    // sanity check; open the location in a browser
    return openWeb(location, NULL, appOpenDefault, site_id);
}

bool AppController::openReport(const wxString& location,
                               int open_mask,
                               int* site_id)
{
    // if the document is open, set the focus to that document
    if (setActiveChildByLocation(location, site_id))
        return true;

    AppBusyCursor bc;

    IDocumentSitePtr site;
    ReportDoc* doc = new ReportDoc;
    site = m_frame->createSite(static_cast<IDocument*>(doc), sitetypeNormal, -1, -1, -1, -1);

    bool layout = (open_mask & appOpenInLayout) ? true : false;
    if (!doc->loadFile(location, layout))
    {
        appMessageBox(_("This report could not be opened.  It may have an invalid format or may have been created with a different version."),
                           APPLICATION_NAME,
                           wxOK | wxICON_INFORMATION | wxCENTER);
                           
        g_app->getMainFrame()->closeSite(site);
        return false;
    }

    if (site.isOk())
    {
        if (site_id)
            *site_id = site->getId();
    }

    return true;
}

bool AppController::openQuery(const wxString& location, int* site_id)
{
    QueryDoc* doc = new QueryDoc;
    if (location.Length() > 0)
    {
        if (!doc->load(location))
        {
            delete doc;
            return false;
        }
    }

    // if the document is open, set the focus to that document
    if (setActiveChildByLocation(location, site_id))
        return true;
    
    IDocumentSitePtr site;
    site = m_frame->createSite(static_cast<IDocument*>(doc),
                        sitetypeNormal,
                        -1, -1, -1, -1);
    if (site.isOk())
    {
        if (site_id)
            *site_id = site->getId();
        return true;
    }
     else
    {
        return false;
    }
}


bool AppController::openScript(const wxString& _location, int* site_id)
{
    // if the file is already open, just set that site as the
    // active site, to prevent the text file from being opened
    // multiple times

    wxString location = _location;
    
    
    if (location.Length() > 0)
    {
        // get the physical location of the script file
        // or the path of the script in the project
        wxString phys_location = getPhysPathFromMountPath(location);
        if (phys_location.Length() > 0)
        {
            location = filenameToUrl(phys_location);
        }
         else
        {
            wxString old_location = location;
            location = getObjectPathFromMountPath(location);
            
            if (location == old_location)
            {
                tango::IDatabasePtr db = g_app->getDatabase();
                
                if (db.isOk() && db->getFileExist(towstr(location)))
                {
                    // file is a database path, which takes precedence
                }
                 else
                {
                    // file path is not a mount, but it might be a physical
                    // disk path.  If so, get the URL version of it
                    if (xf_get_file_exist(towstr(location)))
                        location = filenameToUrl(location);
                }
            }
        }
    }
    
    // if the document is open, set the focus to that document
    if (setActiveChildByLocation(location, site_id))
        return true;

    // open the document
    AppBusyCursor bc;

    EditorDoc* doc = new EditorDoc;
    IDocumentSitePtr site;
    
    site = m_frame->createSite(static_cast<IDocument*>(doc),
                               sitetypeNormal | siteHidden,
                               -1, -1, -1, -1);

    if (site.isNull())
        return false;
    
    if (location.Length() > 0)
    {
        if (!doc->loadFile(location))
        {
            if (site_id)
                *site_id = 0;
                
            m_frame->closeSite(site, closeForce);
            return false;
        }
    }
    
    if (site_id)
        *site_id = site->getId();
    
    site->setVisible(true);
    
    return true;
}
        
bool AppController::openTable(const wxString& _location, int* site_id)
{
    if (site_id)
        *site_id = 0;
        
    AppBusyCursor bc;
    
    tango::IDatabasePtr database = g_app->getDatabase();
    wxASSERT_MSG(database.isOk(), wxT("Current database is NULL."));
    
    if (database.isNull())
        return false;
        
    std::wstring location = towstr(_location);
    kl::trim(location);


    tango::IFileInfoPtr fileinfo = database->getFileInfo(location);
    if (fileinfo.isNull())
        return false;

    int format = fileinfo->getFormat();

    if (format == tango::formatDelimitedText || format == tango::formatFixedLengthText)
    {
        // open text set
        createTextDoc(location, NULL, site_id);
    }
     else
    {
        // open all normal table docs
        ITableDocPtr doc = TableDocMgr::createTableDoc();
        if (!doc->open(location))
        {
            wxFAIL_MSG(wxT("ITableDoc::open() returned false"));
            return false;
        }

        if (doc->getCaption().Length() == 0)
            doc->setCaption(location, wxEmptyString);
            
        unsigned int site_type = sitetypeNormal;

        IDocumentSitePtr doc_site;
        doc_site = g_app->getMainFrame()->createSite(doc, site_type, -1, -1, -1, -1);
            
        if (site_id)
            *site_id = doc_site->getId();
    }

    return true;
}


bool AppController::openDataLink(const wxString& location, int* site_id)
{
    std::wstring url = towstr(location);

    int url_sign = url.find(L"://");
    if (url_sign == -1)
        return false;
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    // parse url
    std::wstring protocol;
    std::wstring server;
    std::wstring port;
    std::wstring path;
    std::wstring database;

    protocol = url.substr(0, url_sign);
    server = url.substr(url_sign+3);

    if (server.find('/') != server.npos)
    {
        path = kl::afterFirst(server, '/');
        server = kl::beforeFirst(server, '/');
    }

    if (server.find(':') != server.npos)
    {
        port = kl::afterFirst(server, ':');
        server = kl::beforeFirst(server, ':');
    }
    
    if (path.find('/') == path.npos)
    {
        database = path;
        path = '/';
    }
     else
    {
        database = kl::beforeFirst(path, '/');
        path = kl::afterFirst(path, '/');
    }


    if (path.length() == 0 || path[0] != '/')
        path = L"/" + path;

    if (port.length() == 0)
        port = L"4800";

    // make a connection string
    std::wstring conn_str = L"xdprovider=xdclient;";
    conn_str += L"host=" + server + L";";
    conn_str += L"port=" + port + L";";
    conn_str += L"database=default;user id=admin;password=";




    if (!db->getFileExist(L"/.mnt"))
        db->createFolder(L"/.mnt");

    std::wstring mnt_server = L"/.mnt/" + server;
    if (!db->getFileExist(mnt_server))
        db->createFolder(mnt_server);

    std::wstring mnt_database = mnt_server + L"/" + database;

    if (!db->getFileExist(mnt_database))
        db->setMountPoint(mnt_database, conn_str, L"/");



    path = mnt_database + path;



    tango::IFileInfoPtr finfo = db->getFileInfo(path);
    if (finfo.isNull())
    {
        appMessageBox(wxT("A valid data view or link was not found at the specified location."), wxT("Error"));
        return false;
    }
    
    if (finfo->getMimeType() == L"application/vnd.kx.view-link")
    {
        std::wstring json;
        if (!readStreamTextFile(db, path, json))
            return false;

        std::wstring mount_root = getMountRoot(db, path);

        kl::JsonNode root;
        root.fromString(json);

        path = root["data"]["table"];
        std::wstring filter = root["data"]["where"];
        std::wstring sort = root["data"]["order"];

        if (filter.length() == 0 && sort.length() == 0)
        {
            return openTable(path);
        }
         else
        {
            jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.query-job");

            // configure the job parameters
            kl::JsonNode params;

            std::wstring path = mount_root + L"/" + path;
            params = createSortFilterJobParams(path, towstr(filter), towstr(sort));

            // set the job parameters and start the job
            job->getJobInfo()->setTitle(towstr(_("Opening data view...")));
            job->setParameters(params.toString());

            job->sigJobFinished().connect(this, &AppController::onOpenDataViewFinished);
            g_app->getJobQueue()->addJob(job, jobStateRunning);


            if (site_id)
                *site_id = 0;
        }
    }
     else if (finfo->getType() == tango::filetypeSet)
    {
        return openTable(path, site_id);
    }

    return true;
}


void AppController::onOpenDataViewFinished(jobs::IJobPtr query_job)
{
    tango::IIteratorPtr iter = query_job->getResultObject();
    if (iter.isNull())
        return;

    std::wstring table_path = iter->getTable();
    if (table_path.empty())
        return;

    ITableDocPtr doc = TableDocMgr::createTableDoc();
    if (!doc->open(towx(table_path), iter))
    {
        wxFAIL_MSG(wxT("ITableDoc::open() returned false"));
        return;
    }

    unsigned int site_type = sitetypeNormal;

    IDocumentSitePtr doc_site;
    doc_site = g_app->getMainFrame()->createSite(doc, site_type, -1, -1, -1, -1);
}


bool AppController::openTemplate(const wxString& location,
                                 int open_mask)
{
    AppBusyCursor bc;

    tango::IDatabasePtr db = g_app->getDatabase();
    if (!db)
        return false;
    
    tango::IFileInfoPtr file_info;
    file_info = db->getFileInfo(towstr(location));
    if (!file_info.isOk())
        return false;


    // give the application hook a chance to handle this call
    bool handled = false;
    bool res = apphookOpenTemplate(location,
                                   file_info,
                                   &handled);
    if (handled)
        return res;


    // handle it ourselves
    if (file_info->getType() == tango::filetypeStream)
    {
        std::wstring mime_type = file_info->getMimeType();
        if (mime_type == L"application/vnd.kx.report")
        {
            openReport(location, open_mask);
        }
         else if (mime_type == L"application/vnd.kx.query")
        {
            openQuery(location);
        }
         else if (mime_type == L"application/vnd.kx.import")
        {
            ImportInfo info;
            showImportWizard(info, location);
        }
        
        return true;
    }
     else if (file_info->getType() == tango::filetypeNode)
    {
        kl::JsonNode node = JsonConfig::loadFromDb(g_app->getDatabase(), location);
        if (!node.isOk())
            return false;

        kl::JsonNode root_node = node["root"];
        if (!root_node.isOk())
            return false;

        kl::JsonNode kpp_template_node = root_node["kpp_template"];
        if (!kpp_template_node.isOk())
            return false;

        kl::JsonNode type_node = kpp_template_node["type"];
        if (!type_node.isOk())
            return false;
        wxString type = towx(type_node.getString());

        if (!type.CmpNoCase(wxT("import")))
        {
            ImportInfo info;
            showImportWizard(info, location);
        }
         else if (!type.CmpNoCase(wxT("query")))
        {
            openQuery(location);
        }

        return true;
    }
    
    return false;
}


bool AppController::openBookmark(const wxString& _location,
                                 int open_mask,
                                 int* site_id)
{
    // check if the location specified is a bookmark in the project;
    // if so, load the url location from the bookmark
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    tango::IFileInfoPtr file_info;
    file_info = db->getFileInfo(towstr(_location));
    if (!file_info.isOk())
        return false;

    // note: old bookmark node files are converted to the new format
    // when they are loaded, which happens when a first project is
    // first loaded, so we shouldn't have bookmarks in the old node
    // format
    if (file_info->getType() != tango::filetypeStream)
        return false;

    kl::JsonNode node = JsonConfig::loadFromDb(g_app->getDatabase(), _location);
    if (!node.isOk())
        return false;

    if (!isValidFileVersion(node, L"application/vnd.kx.bookmark", 1))
        return false;

    kl::JsonNode bookmark_node = node["bookmark"];
    if (!bookmark_node.isOk())
        return false;

    kl::JsonNode location_node = bookmark_node["location"];
    if (!location_node.isOk())
        return false;
    wxString location = towx(location_node.getString());

    bool run_target = false;
    kl::JsonNode run_target_node = bookmark_node["run_target"];
    if (!run_target_node.isOk())
        run_target = (run_target_node.getInteger() != 0 ? true : false);

    if (run_target)
    {
        if (execute(location))
            return true;
                    
        // execute failed, try to open the target
    }

    return openAny(location);
}


bool AppController::openWeb(const wxString& _location,
                            wxWebPostData* post_data,
                            int open_mask,
                            int* site_id)
{
    if (site_id)
        *site_id = 0;
        
    wxString location = _location;

    wxString last_clicked_bookmark_path;
    
    if (location.Find(wxT("://")) == wxNOT_FOUND)
    {
        tango::IDatabasePtr db = g_app->getDatabase();
        if (db.isOk())
        {


            // check if the location specified is a bookmark in the project; if so, load 
            // the url location from the bookmark

            kl::JsonNode node = JsonConfig::loadFromDb(g_app->getDatabase(), towstr(location));
            if (node.isOk())
            {
                kl::JsonNode root_node = node["root"];
                if (root_node.isOk())
                {
                    kl::JsonNode bookmark_node = root_node["bookmark"];
                    if (bookmark_node.isOk())
                    {
                        last_clicked_bookmark_path = location;

                        kl::JsonNode location_node = bookmark_node["location"];
                        if (!location_node.isOk())
                            return false;
                    
                        location = towx(location_node.getString());
                    }
                }
                else
                {
                    // TODO: when bookmark stream format becomes available, add 
                    // check for it here
                }
            }
             else
            {
                // the file could be a web file that is located
                // in a mounted (filesystem) folder; get the physical
                // location of the file and try to open it instead
                wxString phys_path = getPhysPathFromMountPath(location);
                if (phys_path.Length() > 0)
                {
                    location = filenameToUrl(phys_path);
                    open_mask |= appOpenForceNewWindow;
                    open_mask |= appOpenActivateNewWindow;
                }
            }
        }
    }


    // finally, if we can't open the url as a file in the project, try 
    // to open it as a browser document
    IWebDocPtr doc;
    
    if (!(open_mask & appOpenForceNewWindow))
    {
        IDocumentSitePtr active_site = m_frame->getActiveChild();
        if (active_site)
        {
            doc = active_site->getDocument();
            if (site_id)
                *site_id = active_site->getId();
        }
    }

    // if we don't have a webdoc, create one     
    if (doc.isNull())
    {    
        int state = sitetypeNormal;
        if (!(open_mask & appOpenActivateNewWindow))
            state |= siteNoInitialActivate;
        doc = static_cast<IWebDoc*>(new WebDoc);
        IDocumentSitePtr site;
        site = m_frame->createSite(doc, state, -1, -1, -1, -1);
        if (site.isOk() && site_id)
            *site_id = site->getId();
    }    

    doc->setLastClickedBookmarkPath(last_clicked_bookmark_path);
    doc->openURI(location, post_data);
    
    return true;
}



// -- utility functions (ripped straight out of importpages.cpp) --

static void getAllSets(tango::IDatabasePtr db_ptr,
                       const wxString& path,
                       std::vector<wxString>& retval)
{
    tango::IFileInfoEnumPtr items = db_ptr->getFolderInfo(towstr(path));
    if (items.isNull())
        return;

    size_t i, count = items->size();
    int item_type;

    for (i = 0; i < count; ++i)
    {
        tango::IFileInfoPtr info = items->getItem(i);
        item_type = info->getType();

        if (item_type == tango::filetypeFolder)
        {
            wxString folder_path = path;
            folder_path += towx(info->getName());
            folder_path += wxT("/");

            // recursively traverse this folder
            getAllSets(db_ptr, folder_path, retval);
        }
         else if (item_type == tango::filetypeSet)
        {
            wxString name = path;
            
            if (name.Length() == 0 || name.Last() != wxT('/'))
                name += wxT("/");
                
            name += towx(info->getName());

            retval.push_back(name);
        }
    }
}

static wxString getTempTablename(const wxString& filename, const wxString& tablename)
{
    wxString fn = filename.AfterLast(PATH_SEPARATOR_CHAR);
    if (fn.Find(wxT('.')) != wxNOT_FOUND)
        fn = fn.BeforeLast(wxT('.'));

    wxString tn = tablename;
    while (tn.Length() > 0 && tn.GetChar(0) == wxT('/'))
        tn = tn.Mid(1);
    
    wxString retval = wxT("/.temp/");
    retval += fn;
    retval += wxT("/");
    retval += tn;
    return retval;
}

static void onOpenExcelJobFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobStateFinished)
        return;

    // get the list of output tables;
    std::wstring output = job->getExtraValue(L"appmain.output_tables");

    std::vector<std::wstring> output_tables;
    kl::parseDelimitedList(output, output_tables, ',', true);

    int first_site_id = 0;
    int site_id = 0;
    
    std::vector<std::wstring>::iterator it, it_end;
    it_end = output_tables.end();

    for (it = output_tables.begin(); it != it_end; ++it)
    {
        g_app->getAppController()->openTable(*it, &site_id);

        // store the set id for the first table we open,
        // so we can activate that site
        if (first_site_id == 0)
            first_site_id = site_id;
    }
    
    // activate the tab which contains the first table in the file
    IFramePtr frame = g_app->getMainFrame();
    IDocumentSitePtr first_site = frame->lookupSiteById(first_site_id);
    if (first_site.isOk())
        frame->setActiveChild(first_site);
        
    g_app->getAppController()->refreshDbDoc();
}

static void makeSafeConnectionName(tango::IDatabasePtr db, wxString& name)
{
    name.Trim();
    name.Trim(false);
    
    // DOS/Windows-based filesystem mount
    if (name.Mid(1, 1) == wxT(":"))
    {
        if (name.Length() <= 3)
            name = name.Left(1); // drive letter
             else
            name = name.Mid(2, wxString::npos);
    }
    
    while (name.Mid(0, 1) == wxT("/") ||
           name.Mid(0, 1) == wxT("\\"))
    {
        name = name.Mid(1, wxString::npos);
    }
    
    name.Replace(wxT("\\\\"), wxT("_"), true);
    name.Replace(wxT("\\"), wxT("_"), true);
    name.Replace(wxT("/"), wxT("_"), true);

    // this is a coverall function from previous version which is not
    // very user-friendly (it can result in large numbers of underscore
    // characters in the path name on the tree)... this should be reworked
    name = makeValidObjectName(name);

    // determine if name already exists in destination,
    // append an '_2', '_3', etc. if it does
    bool first = true;
    int counter = 1;
    while (db->getFileExist(towstr(name)))
    {
        // if we've already added a number onto the
        // string, remove it and add a new number
        if (first)
            first = false;
              else
            name = name.BeforeLast(wxT('_'));

        counter++;
        name = wxString::Format(wxT("%s_%d"),
                                     name.c_str(),
                                     counter);
    }
}

static void onImportWizardFinished(ImportWizard* dlg)
{
    dlg->getTemplate().execute();
}

static void onCreateExternalConnectionWizardFinished(ConnectionWizard* dlg)
{
    wxString conn_str = dlg->getConnectionString();
    ConnectionInfo info = dlg->getConnectionInfo();
    wxString conn_name;
    switch (info.type)
    {
        case dbtypeSqlServer:
        case dbtypeMySql:
            conn_name = wxString::Format(_("%s on %s"),
                                         info.database.c_str(),
                                         info.server.c_str());
            break;
        
        case dbtypeOracle:
        case dbtypeDb2:
        case dbtypeOdbc:
            conn_name = wxString::Format(_("%s on %s"),
                                         info.database.c_str(),
                                         dlg->getConnectionInfo().server.c_str());
            break;

        case dbtypeFilesystem:
            conn_name = dlg->getConnectionInfo().path;
            if (conn_name.Last() == '/' || conn_name.Last() == '\\')
                conn_name.RemoveLast();
            break;

        case dbtypeClient:
            conn_name = dlg->getConnectionInfo().server;
            break;
    }

    makeSafeConnectionName(g_app->getDatabase(), conn_name);
            
    g_app->getAppController()->createMountPoint(conn_str, conn_name);
}

bool AppController::openExcel(const wxString& location, int* site_id)
{
    // convert the path of the excel file
    wxString fn;
    if (location.Left(5).CmpNoCase(wxT("file:")) == 0)
        fn = urlToFilename(location);
     else
        fn = getPhysPathFromMountPath(location);

    IConnectionPtr conn = createUnmanagedConnection();
    conn->setType(dbtypeExcel);
    conn->setPath(fn);

    // if we cannot open the connection, bail out
    if (!conn->open())
    {
        wxString error_message = _("There was an error opening the specified Microsoft Excel file.  Make sure the file is not open by another application.");
        appMessageBox(error_message,
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);

        // we return true since we've already shown an
        // error message to the user (returning false
        // would show multiple error messages)
        return true;
    }
    
    // create the dialog's title
    wxString title = fn.AfterLast(PATH_SEPARATOR_CHAR);
    wxString dlg_title = wxString::Format(_("Open '%s'"), title.c_str());
    
    // get a list of all items in the source database
    std::vector<wxString> info;
    std::vector<wxString>::iterator info_it;
    getAllSets(conn->getDatabasePtr(), wxT("/"), info);

    // sort the tablenames vector
    std::sort(info.begin(), info.end());

    // populate the array of strings
    wxArrayString as;
    for (info_it = info.begin(); info_it != info.end(); ++info_it)
        as.Add(*info_it);

    wxMultiChoiceDialog dlg(g_app->getMainWindow(),
                            _("Select the tables you would like to open from this file."),
                            dlg_title,
                            as);
    
    // select all the tables by default
    int sel = 0;
    wxArrayInt selections;
    for (info_it = info.begin(); info_it != info.end(); ++info_it)
        selections.Add(sel++);
    
    dlg.SetSelections(selections);
    if (dlg.ShowModal() == wxID_OK)
    {
        wxString job_title = wxString::Format(_("Importing from '%s'"),
                                              title.c_str());
        
        ImportJob* job = new ImportJob;
        job->setImportType(dbtypeExcel);
        job->setFilename(towstr(fn));
            
        selections = dlg.GetSelections();
        int i, count = selections.Count();
        if (count == 0)
        {
            appMessageBox(_("No tables were opened from the Microsoft Excel file because no tables in the list were selected."),
                               APPLICATION_NAME,
                               wxOK | wxICON_INFORMATION | wxCENTER);

            // we return true since we've already shown an
            // error message to the user (returning false
            // would show multiple error messages)
            return true;
        }

        std::vector<std::wstring> output_paths;
            
        for (i = 0; i < count; ++i)
        {
            int idx = selections[i];
            
            ImportJobInfo job_import_info;
            job_import_info.input_path = as[idx];
            job_import_info.output_path = getTempTablename(fn, as[idx]);
            job_import_info.append = false;
            job->addImportSet(job_import_info);

            output_paths.push_back(job_import_info.output_path);
        }

        // pass the output tables as a delimited list
        std::wstring output = kl::joinList(output_paths, L",");
        job->setExtraValue(L"appmain.output_tables", output);

        job->getJobInfo()->setTitle(towstr(job_title));
        job->sigJobFinished().connect(&onOpenExcelJobFinished);
        g_app->getJobQueue()->addJob(job, jobStateRunning);
    }

    return true;
}

bool AppController::openAccess(const wxString& location)
{
    // convert the path of the excel file
    wxString fn;
    if (location.Left(5).CmpNoCase(wxT("file:")) == 0)
        fn = urlToFilename(location);
     else
        fn = getPhysPathFromMountPath(location);

    // scoping so the smart pointer destroys itself
    {
        IConnectionPtr conn = createUnmanagedConnection();
        conn->setType(dbtypeAccess);
        conn->setPath(fn);

        // if we cannot open the connection, bail out
        if (!conn->open())
        {
            wxString error_message = _("There was an error opening the specified Microsoft Access file.  Make sure the file is not open by another application.");
            appMessageBox(error_message,
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            // we return true since we've already shown an
            // error message to the user (returning false
            // would show multiple error messages)
            return true;
        }
    }
    
    wxString title = fn.AfterLast(PATH_SEPARATOR_CHAR);
    title = wxString::Format(_("Import '%s'"), title.c_str());
    
    ImportWizard* wizard = new ImportWizard;
    wizard->setTitle(title);
    wizard->setMode(ImportWizard::ModeOpen);
    
    ImportTemplate& import_template = wizard->getTemplate();
    import_template.m_ii.type = dbtypeAccess;
    import_template.m_ii.path = fn;
    wizard->sigImportWizardFinished.connect(&onImportWizardFinished);

    IDocumentSitePtr site;
    site = g_app->getMainFrame()->createSite(wizard, sitetypeModeless,
                                             -1, -1, 540, 480);
    site->setMinSize(540, 480);
    return true;
}

bool AppController::openPackage(const wxString& location)
{
    // convert the path of the excel file
    wxString fn;
    if (location.Left(5).CmpNoCase(wxT("file:")) == 0)
        fn = urlToFilename(location);
     else
        fn = getPhysPathFromMountPath(location);

    // scoping so the package file closes
    {
        // make sure we've got a valid package file
        PkgFile pkgfile;
        if (!pkgfile.open(towstr(fn), PkgFile::modeRead))
        {
            wxString error_message = _("There was an error opening the specified package file.  Make sure the file is not open by another application.");
            appMessageBox(error_message,
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            // we return true since we've already shown an
            // error message to the user (returning false
            // would show multiple error messages)
            return true;
        }
    }
    
    wxString title = fn.AfterLast(PATH_SEPARATOR_CHAR);
    title = wxString::Format(_("Import '%s'"), title.c_str());
    
    ImportWizard* wizard = new ImportWizard;
    wizard->setTitle(title);
    wizard->setMode(ImportWizard::ModeOpen);
    
    ImportTemplate& import_template = wizard->getTemplate();
    import_template.m_ii.type = dbtypePackage;
    import_template.m_ii.path = fn;
    wizard->sigImportWizardFinished.connect(&onImportWizardFinished);

    IDocumentSitePtr site;
    site = g_app->getMainFrame()->createSite(wizard, sitetypeModeless,
                                             -1, -1, 540, 480);
    site->setMinSize(540, 480);
    return true;
}

bool AppController::setActiveChildByLocation(const wxString& location, int* site_id)
{
    IDocumentSiteEnumPtr doc_sites;
    doc_sites = g_app->getMainFrame()->getDocumentSites(sitetypeNormal);
    
    size_t i, size = doc_sites->size();
    for (i = 0; i < size; ++i)
    {
        IDocumentSitePtr site = doc_sites->getItem(i);
        IDocumentPtr doc = site->getDocument();
        wxString loc = doc->getDocumentLocation();
        if (doc.isOk() && !loc.IsEmpty() && loc.CmpNoCase(location) == 0)
        {
            g_app->getMainFrame()->setActiveChild(site);
            if (site_id)
                *site_id = site->getId();
            return true;
        }
    }

    return false;
}

bool AppController::newReport(int* site_id)
{
    // report creation info
    ReportCreateInfo data;

    // see if the currently active document has any
    // report creation info to send
    IDocumentSitePtr doc_site = g_app->getMainFrame()->getActiveChild();
    
    ReportCreateInfo tabledoc_data;
    if (setReportCreateInfo(doc_site, tabledoc_data))
    {
        // show a message asking the user if they want to
        // use the tabledoc data to create the report or
        // create a new report

        int result = appMessageBox(_("Would you like to create a report from the selected columns?"),
                                        APPLICATION_NAME,
                                        wxYES_NO | wxICON_QUESTION | wxCENTER);
        if (result == wxYES)
            data = tabledoc_data;
    }

    // create a report
    ReportDoc* doc = new ReportDoc;
    doc_site = g_app->getMainFrame()->createSite(static_cast<IDocument*>(doc),
                                             sitetypeNormal, -1, -1, -1, -1);

    if (doc_site.isNull() || !doc->create(data))
    {
        delete doc;
        return false;
    }
    
    if (site_id)
        *site_id = doc_site->getId();
    
    return true;
}

bool AppController::newQuery(int* site_id)
{
    return openQuery(L"", site_id);
}

bool AppController::newTable(int* site_id)
{
    IDocumentSitePtr doc_site;
    
    StructureDoc* doc = new StructureDoc;
    doc_site = m_frame->createSite(static_cast<IDocument*>(doc),
                        sitetypeNormal,
                        -1, -1, -1, -1);
    if (doc_site.isNull())
        return false;
        
    if (site_id)
        *site_id = doc_site->getId();
    return true;
}

bool AppController::newScript(int* site_id)
{
    return openScript(L"", site_id);
}







jobs::IJobPtr AppController::executeScript(const wxString& _location,
                                           ScriptHostParams* params,
                                           AppScriptError* error,
                                           bool async)
{
    AppBusyCursorIfMainThread bc;

    if (error)
    {
        // initialize error object
        error->line = 0;
        error->offset = 0;
        error->code = 0;
    }    
    
    ScriptHostInclude include_path;

    wxString location = _location;
    
    if (xf_get_file_exist(towstr(location)))
    {
        wxString ext = _location.AfterLast(wxT('.'));
        ext.MakeUpper();
    
        include_path.type = ScriptHostInclude::includeFile;
        wxString dir = location.BeforeLast(PATH_SEPARATOR_CHAR);
        if (dir != location && ext != L"ZIP" && ext != L"KXT")
        {
            include_path.path = dir;
        }
        
        location = filenameToUrl(location);
    }
     else
    {
        wxString phys_path = getPhysPathFromMountPath(location);
        if (phys_path.Length() > 0)
        {
            include_path.type = ScriptHostInclude::includeFile;
            wxString dir = phys_path.BeforeLast(PATH_SEPARATOR_CHAR);
            if (dir != phys_path)
            {
                include_path.path = dir;
            }
        } 
         else
        {
            wxString inc = location;
            if (inc.Find(wxT("://")) != -1)
            {
                inc = urlToFilename(location);
                include_path.type = ScriptHostInclude::includeFile;
                wxString dir = inc.BeforeLast(PATH_SEPARATOR_CHAR);
                if (dir != inc)
                {
                    include_path.path = dir;
                }
            }
             else
            {
                include_path.type = ScriptHostInclude::includeProjectFile;
                wxString dir = location.BeforeLast(wxT('/'));
                if (dir != location)
                {
                    include_path.path = dir;
                    if (include_path.path == wxT(""))
                        include_path.path = wxT("/");
                }
            }
        }
    }


    // create a script host object
    ScriptHost* script_host = new ScriptHost;
    script_host->setParams(params);
    
    // add the include path
    if (include_path.path.Length() > 0)
    {
        script_host->addIncludePath(include_path);
        script_host->setStartupPath(include_path.path);
    }
    
    
    bool compile_result;
    
    

    wxString ext = _location.AfterLast(wxT('.'));
    ext.MakeUpper();
    if (ext == L"ZIP" || ext == L"KXT")
    {
        ExtensionPkg* pkg = new ExtensionPkg;
        if (!pkg->open(_location))
        {
            // could not open package
            return xcm::null;
        }
        
        // script host will delete package
        script_host->setPackage(pkg);
        
        compile_result = script_host->compilePackage();
    }
     else
    {
        tango::IDatabasePtr db = g_app->getDatabase();
        if (db.isNull())
        {
            // no database, try an xdfs next
            db.create_instance("xdfs.Database");
            if (db.isNull())
            {
                if (error)
                {
                    error->code = kscript::errorUnknown;
                    error->message = _("Could not initialize database environment");
                }
                return xcm::null;
            }
        }
        
        tango::IStreamPtr stream = db->openStream(towstr(location));
        if (!stream)
        {
            if (error)
            {
                error->code = kscript::errorUnknown;
                error->message = _("Input file not found");
            }
            return xcm::null;
        }
        
        std::wstring value;
        
        
        
        // load script data
        
        wxMemoryBuffer buf;
        
        char* tempbuf = new char[1025];
        unsigned long read = 0;
        
        while (1)
        {
            if (!stream->read(tempbuf, 1024, &read))
                break;
            
            buf.AppendData(tempbuf, read);
            
            if (read != 1024)
                break;
        }
        
        delete[] tempbuf;
        
        
        unsigned char* ptr = (unsigned char*)buf.GetData();
        size_t buf_len = buf.GetDataLen();
        if (buf_len >= 2 && ptr[0] == 0xff && ptr[1] == 0xfe)
        {
            kl::ucsle2wstring(value, ptr+2, (buf_len-2)/2);
        }
         else if (buf_len >= 3 && ptr[0] == 0xef && ptr[1] == 0xbb && ptr[2] == 0xbf)
        {
            // utf-8
            wchar_t* tempbuf = new wchar_t[buf_len+1];
            kl::utf8_utf8tow(tempbuf, buf_len+1, (char*)ptr+3, buf_len-3);
            value = towx(tempbuf);
            delete[] tempbuf;
        }
         else
        {
            buf.AppendByte(0);
            value = wxString::From8BitData((char*)buf.GetData());
        }
        
        compile_result = script_host->compile(value.c_str());
    }



    if (!compile_result)
    {
        if (error)
        {
            error->line = script_host->getErrorLine();
            error->offset = script_host->getErrorOffset();
            error->code = script_host->getErrorCode();
            error->message = script_host->getErrorString();
            error->file = script_host->getErrorFile();
        }

        delete script_host;
        return xcm::null;
    }


    if (error)
    {
        error->line = 0;
        error->code = 0;
        error->message = wxT("");
    }



    if (!async)
    {
        script_host->run();
        delete script_host;
        return xcm::null;
    }


    ScriptJob* script_job = new ScriptJob;
    script_job->setScriptHost(script_host);
    jobs::IJobPtr job = static_cast<jobs::IJob*>(script_job);

    g_app->getScriptJobQueue()->addJob(job, jobStateRunning);
    return job;
}


jobs::IJobPtr AppController::executeCode(const wxString& value,
                                         ScriptHostParams* params,
                                         AppScriptError* error)
{
    // create a script host object
    ScriptHost* script_host = new ScriptHost;
    script_host->setParams(params);
    
    if (!script_host->compile(value))
    {
        if (error)
        {
            error->line = script_host->getErrorLine();
            error->offset = script_host->getErrorOffset();
            error->code = script_host->getErrorCode();
            error->message = script_host->getErrorString();
            error->file = script_host->getErrorFile();
        }

        delete script_host;
        return xcm::null;
    }

    if (error)
    {
        error->line = 0;
        error->code = 0;
        error->message = wxT("");
    }

    ScriptJob* script_job = new ScriptJob;
    script_job->setScriptHost(script_host);
    jobs::IJobPtr job = static_cast<jobs::IJob*>(script_job);

    g_app->getScriptJobQueue()->addJob(job, jobStateRunning);
    return job;
}





jobs::IJobPtr AppController::execute(const wxString& location)
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (!db)
        return xcm::null;

    tango::IFileInfoPtr file_info = db->getFileInfo(towstr(location));
    if (!file_info)
        return xcm::null;
        
    
    // give the application hook a chance to handle this call
    bool handled = false;
    jobs::IJobPtr res = apphookExecute(location, file_info, &handled);
    if (handled)
    {
        // hook handled the call
        return res;
    }


    if (file_info->getType() == tango::filetypeStream)
    {
        std::wstring mime_type = file_info->getMimeType();

        if (mime_type == L"application/vnd.kx.import")
        {
            ImportTemplate t;
            if (!t.load(location))
                return xcm::null;
            return t.execute();
        }
        else if (mime_type == L"application/vnd.kx.query")
        {
            QueryTemplate t;
            if (!t.load(location))
                return xcm::null;
            return t.execute();

            // TODO: QueryTemplate now returns jobs::IJobPtr; need to
            // uncomment above once this function returns the same
        }
         else
        {
            // TODO: add hook for JSON job templates


            // most likely a script
            return executeScript(location);
        }
    }
     else if (file_info->getType() == tango::filetypeNode)
    {
        kl::JsonNode node = JsonConfig::loadFromDb(g_app->getDatabase(), towstr(location));
        if (!node.isOk())
            return xcm::null;

        kl::JsonNode root_node = node["root"];
        if (!root_node.isOk())
            return xcm::null;

        kl::JsonNode kpp_template_node = root_node["kpp_template"];
        if (!kpp_template_node.isOk())
            return xcm::null;

        kl::JsonNode type_node = kpp_template_node["type"];
        if (!type_node.isOk())
            return xcm::null;

        wxString type = towx(type_node.getString());

        if (type == L"query")
        {
            QueryTemplate t;
            if (!t.load(location))
                return xcm::null;
            return t.execute();
        }
         else if (type == L"import")
        {
            ImportTemplate t;
            if (!t.load(location))
                return xcm::null;
            return t.execute();
        }
    }

    return xcm::null;
}

bool AppController::print(const wxString& location)
{
    AppBusyCursor bc;

    IDocumentSitePtr site;
    ReportDoc* doc = new ReportDoc;
    site = m_frame->createSite(static_cast<IDocument*>(doc), siteHidden, -1, -1, -1, -1);

    if (!doc->loadFile(location))
    {
        appMessageBox(_("This report could not be printed."),
                           APPLICATION_NAME,
                           wxOK | wxICON_INFORMATION | wxCENTER);
                           
        g_app->getMainFrame()->closeSite(site);
        return false;
    }

    doc->print(false);
    g_app->getMainFrame()->closeSite(site);
    return true;
}

static void addDefaultItemsToProject(const wxString& project_path)
{
    wxFrame* wnd = g_app->getMainWindow();
    if (!wnd)
        return;
    
    // don't show any of this happening
    wnd->Freeze();
    
    // the project login and password
    // probably shouldn't be hardcoded here
    if (!g_app->getAppController()->openProject(project_path,
                                                wxT("admin"),
                                                wxEmptyString))
    {
        wnd->Thaw();
        return;
    }
    
    // get the path of the startup sample file
    wxString filename = g_app->getInstallPath();
    #ifdef WIN32
    filename += wxT("\\..\\samples\\startup.kpg");
    #else
    filename += wxT("/../samples/startup.kpg");
    #endif
    
    // the following error checks are also done in
    // ImportWizard::onPathSelectionPageChanging()
    PkgFile pkgfile;
    if (!pkgfile.open(towstr(filename), PkgFile::modeRead))
    {
        // make sure we close the project
        g_app->getAppController()->closeProject();
        wnd->Thaw();
        return;
    }
    
    // create a package stream (NOTE: we always need to destroy this)
    PkgStreamEnum* stream_enum = pkgfile.getStreamEnum();
    int stream_count = stream_enum->getStreamCount();
    if (!stream_enum)
    {
        // make sure we close the project
        g_app->getAppController()->closeProject();
        wnd->Thaw();
        return;
    }
    
    // populate the item names vector from the package file stream
    std::vector<wxString> item_names;
    for (int i = 0; i < stream_count; ++i)
    {
        // don't get hidden stream names
        wxString stream_name = towx(stream_enum->getStreamName(i));
        if (*(stream_name.c_str()) == '.')
            continue;
        
        item_names.push_back(towx(stream_enum->getStreamName(i)));
    }
    
    delete stream_enum;
    
    // sort the tablenames vector
    std::sort(item_names.begin(), item_names.end());
    
    // create the import package job        
    ImportPkgJob* import_job = new ImportPkgJob;
    import_job->setPkgFilename(towstr(filename));
    
    // add the items to the import job
    std::vector<wxString>::iterator it;
    for (it = item_names.begin(); it != item_names.end(); ++it)
        import_job->addImportObject(towstr(*it), towstr(*it));
    
    // run the job
    jobs::IJobPtr job = static_cast<jobs::IJob*>(import_job);
    if (job.isOk())
    {
        job->runJob();
        job->runPostJob();
    }
    
    // make sure the project is closed again
    // (since we'll be opening it again shortly)
    g_app->getAppController()->closeProject();
    wnd->Thaw();
}

bool AppController::createDefaultProject()
{
    wxString project_path = getDefaultProjectsPath();
    if (project_path.Last() != PATH_SEPARATOR_CHAR)
        project_path += PATH_SEPARATOR_CHAR;
    project_path += wxT("Default Project");
    
    // create and open the project
    bool created = createProject(project_path, wxEmptyString, true);
    
    // add default items to the project
    if (created)
        addDefaultItemsToProject(project_path);
    
    // set a flag indicating that a default project has been created
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    prefs->setBoolean(wxT("general.default_project_created"), created);
    
    if (created && !prefs->exists(wxT("general.startup.default_action")))
    {
        // if they don't already have a default startup action,
        // add one that loads the default project we just created
            
         // lookup the connection number
        ProjectMgr projectmgr;
        int idx = projectmgr.getIdxFromLocation(project_path);
        
        // if the project isn't already in the project manager, add it
        if (idx == -1)
        {
            projectmgr.addProjectEntry(wxEmptyString,
                                    project_path,
                                    wxT("admin"),
                                    wxEmptyString,
                                    true);
                                    
            idx = projectmgr.getIdxFromLocation(project_path);
        }
        
        // if we have a valid index, set that project as our startup project
        if (idx != -1)
        {
            prefs->setLong(wxT("general.startup.default_action"),
                           getAppPrefsDefaultLong(wxT("general.startup.default_action")));
            prefs->setString(wxT("general.startup.connection"),
                             projectmgr.getProjectEntries()[idx].entry_name);
        }
    }
    
    // flush preferences
    prefs->flush();

    return (created ? true : false);
}

void AppController::createDefaultLinks()
{
    wxString bookmarks_folder = g_app->getBookmarksFolder();
    if (bookmarks_folder.IsEmpty() || bookmarks_folder.Last() != wxT('/'))
        bookmarks_folder += wxT("/");
    
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    
    Bookmark::create(bookmarks_folder + wxT("Home Page"), getAppPrefsDefaultString(wxT("general.location.home")));
    Bookmark::create(bookmarks_folder + wxT("Online Help"), getAppPrefsDefaultString(wxT("general.location.help")));
    Bookmark::create(bookmarks_folder + wxT("Developer Resources"), getAppPrefsDefaultString(wxT("general.location.resources")));
    Bookmark::create(bookmarks_folder + wxT("Support Forums"), getAppPrefsDefaultString(wxT("general.location.support")));

    DbDoc::setFileVisualLocation(bookmarks_folder + wxT("Home Page"), 0);
    DbDoc::setFileVisualLocation(bookmarks_folder + wxT("Online Help"), 1);
    DbDoc::setFileVisualLocation(bookmarks_folder + wxT("Developer Resources"), 2);
    DbDoc::setFileVisualLocation(bookmarks_folder + wxT("Support Forums"), 3);
}


bool AppController::createProject(const wxString& location,
                                  const wxString& db_name,
                                  bool open_project)
{
    // actually create the database

    xcm::ptr<tango::IDatabaseMgr> dbmgr;
#ifdef USE_XDSL
    dbmgr.create_instance("xdsl.DatabaseMgr");
#else
    dbmgr.create_instance("xdnative.DatabaseMgr");
#endif

    if (dbmgr.isNull())
    {
        appMessageBox(_("Your system is missing a software component.  To correct this problem, please reinstall the software."),
                           APPLICATION_NAME,
                           wxOK | wxICON_INFORMATION | wxCENTER);

        return false;
    }
    
    // create the folder in the filesystem
    if (!createFolderStructure(location))
        return false;

    tango::IDatabasePtr database;
    database = dbmgr->createDatabase(towstr(location), towstr(db_name));
    if (database.isNull())
        return false;
    
    // add an entry in the project manager
    ProjectMgr projmgr;
    projmgr.addProjectEntry(db_name,
                            location,
                            wxT("admin"),
                            wxEmptyString,
                            true);
    
    // open the project we just created
    if (open_project)
    {
        g_app->setDatabase(database);
        
        if (m_dbdoc)
            m_dbdoc->setDatabase(database);

        refreshDbDoc();
    }
    
    return true;
}

bool AppController::openProject(const wxString& location,
                                const wxString& uid,
                                const wxString& password)
{
    AppBusyCursor bc;

    if (g_auth->checkAuth() != paladin::errNone)
        return false;

    // create the database manager
    tango::IDatabaseMgrPtr dbmgr = tango::getDatabaseMgr();
    if (dbmgr.isNull())
    {
        appMessageBox(_("Your system is missing a software component.  To correct this problem, please reinstall the software."),
                           APPLICATION_NAME,
                           wxOK | wxICON_INFORMATION | wxCENTER);

        return false;
    }

    wxString lower_location = location;
    lower_location.MakeLower();

#ifndef USE_XDSL

    if (lower_location.Find(L"xdprovider=") == -1 && 
        lower_location.SubString(0,6) != wxT("http://") && 
        lower_location.SubString(0,7) != wxT("https://"))
    {
        if (!xf_get_directory_exist(towstr(location)))
        {
            appMessageBox(_("The specified project could not be opened.  The path does not exist or is invalid."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            return false;
        }


        wxString ofs_path = location;
        ofs_path += PATH_SEPARATOR_STR;
        ofs_path += wxT("ofs");

        if (!xf_get_directory_exist(towstr(ofs_path)))
        {
            appMessageBox(_("The specified path does not contain a valid database project."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            return false;
        }

        // check the project's database version
        wxString ver_file = location;
        if (ver_file.IsEmpty() || ver_file.Last() != PATH_SEPARATOR_CHAR)
            ver_file += PATH_SEPARATOR_CHAR;
        ver_file += wxT("ofs");
        ver_file += PATH_SEPARATOR_CHAR;
        ver_file += wxT(".system");
        ver_file += PATH_SEPARATOR_CHAR;
        ver_file += wxT("database_version.xml");


        if (!xf_get_file_exist(towstr(ver_file)))
        {
            int res;
            res = appMessageBox(_("This project is currently stored in an older format and must be \nconverted to a newer format before it can be used by this application.\nAfter the project is converted to the newer format, it can no longer\nbe used by other applications that utilize the older format.\nWould you like to upgrade this project to the newer format?"),
                                     APPLICATION_NAME,
                                     wxYES_NO | wxICON_QUESTION | wxCENTER);

            if (res != wxYES)
                return false;
        }
    }
    
#endif // #ifndef USE_XDSL


    wxString cstr;
    if (lower_location.Find(wxT("xdprovider=")) != -1)
    {
        // location string is already a connection string
        cstr = location;
        if (cstr.Length() == 0 || cstr.Last() != ';')
            cstr += wxT(";");
        
        if (lower_location.Find(wxT("user id=")) == -1)
        {
            cstr += wxT("user id=");
            cstr += uid;
            cstr += wxT(";");
            cstr += wxT("password=");
            cstr += password;
            cstr += wxT(";");
        }
    }
     else if (lower_location.SubString(0,6) == wxT("http://"))
    {
        // TODO: extract out database from path
        wxString database = wxT("default");
        wxString port = wxT("4800");

        cstr = wxT("xdprovider=xdclient;");
        cstr += wxT("host=");
        cstr += location.Mid(7);
        cstr += wxT(";");
        cstr += wxT("port=");
        cstr += port;
        cstr += wxT(";");
        cstr += wxT("database=");
        cstr += database;
        cstr += wxT(";");
        cstr += wxT("user id=");
        cstr += uid;
        cstr += wxT(";");
        cstr += wxT("password=");
        cstr += password;
        cstr += wxT(";");
    }
     else if (lower_location.SubString(0,7) == wxT("https://"))
    {
    }
     else
    {
        cstr = wxT("xdprovider=xdnative;");
        cstr += wxT("database=");
        cstr += location;
        cstr += wxT(";");
        cstr += wxT("user id=");
        cstr += uid;
        cstr += wxT(";");
        cstr += wxT("password=");
        cstr += password;
        cstr += wxT(";");
    }
    
    tango::IDatabasePtr database = dbmgr->open(towstr(cstr));
    if (database.isNull())
    {
        appMessageBox(_("The specified project could not be opened."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);

        return false;
    }
    
    // get the project's name from the registry
    ProjectMgr projmgr;
    int idx = projmgr.getIdxFromLocation(location);
    std::vector<ProjectInfo>& projects = projmgr.getProjectEntries();
    wxString entry_name = projects[idx].entry_name;
    wxString name = projects[idx].name;
    
    // set the database
    tango::IAttributesPtr attr = database->getAttributes();
    if (attr)
        attr->setStringAttribute(tango::dbattrDatabaseName, towstr(name));

    g_app->setDatabase(database);
    g_app->setDatabaseLocation(location);
    
    if (m_dbdoc)
        m_dbdoc->setDatabase(database);
    
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    bool default_links_created = prefs->getBoolean(wxT("general.default_links_created"), false);
    if (!default_links_created)
    {
        createDefaultLinks();
        prefs->setBoolean(wxT("general.default_links_created"), true);
    }
    
    // if the 'Last Open Project' preference has been set,
    // set this project as the default startup connection
    if (getAppPrefsBoolean(wxT("general.startup.open_last_project")))
        prefs->setString(wxT("general.startup.connection"), entry_name);
    
    refreshDbDoc();
    return true;
}


bool AppController::openProject(tango::IDatabasePtr database)
{
    g_app->setDatabase(database);
    
    if (m_dbdoc)
        m_dbdoc->setDatabase(database);
        
    refreshDbDoc();
    return true;
}


void AppController::onPrintConsoleText(wxCommandEvent& evt)
{
    if (m_frame.isNull())
        return;

    g_app->getAppController()->showConsolePanel();
    IDocumentSitePtr site = m_frame->lookupSite(wxT("ConsolePanel"));
    if (site.isOk())
    {
        IConsolePanelPtr console = site->getDocument();
        if (console.isOk())
            console->print(evt.GetString());
    }
}


void AppController::printConsoleText(const wxString& text)
{
    #if APP_CONSOLE==1
    printf("%s", (const char*)text.mbc_str());
    #else
    
    if (kl::Thread::isMain())
    {
        wxCommandEvent e;
        e.SetString(text);
        onPrintConsoleText(e);
    }
     else
    {
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, ID_PrintConsoleText);
        e.SetString(text);
        ::wxPostEvent(this, e);
    }
    
    #endif
}



// -- DlgCancellingJobsWait dialog --

class DlgCancellingJobsWait : public wxDialog
{

public:

    DlgCancellingJobsWait(wxWindow* parent) : 
                wxDialog(parent, -1, APPLICATION_NAME,
                         wxDefaultPosition,
                         wxSize(240, 100),
                          (wxDEFAULT_DIALOG_STYLE & ~wxCLOSE_BOX) |
                          wxCLIP_CHILDREN | wxCENTER)
    {
        // create the dialog message
        wxStaticText* label_message = new wxStaticText(this, -1, _("Cancelling all running jobs..."));
        
        // create the dialog bitmap
        wxBitmap bmp = wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MESSAGE_BOX);
        wxStaticBitmap* bitmap_information = new wxStaticBitmap(this, -1, bmp);
        
        wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
        top_sizer->Add(bitmap_information);
        top_sizer->AddSpacer(16);
        top_sizer->Add(label_message, 0, wxEXPAND);
        
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->AddSpacer(12);
        main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
        main_sizer->AddSpacer(12);
        SetSizer(main_sizer);
        Layout();
        
        // 70 = 16*2 (outer border) + 16 (inner border) + 32 (bitmap width) - 10
        resizeStaticText(label_message, GetClientSize().GetWidth()-70);
        
        CenterOnParent();
    }

    ~DlgCancellingJobsWait()
    {
    }
};


bool AppController::checkForRunningJobs(bool exit_message)
{
    int ID_CancelAndExit = 100;
    int ID_KeepJobsRunning = 101;
    int ID_ForceExit = 102;

    IJobQueuePtr job_queue = g_app->getJobQueue();
    if (job_queue->getJobsActive())
    {
        wxString appname = APPLICATION_NAME;
        wxString message;
        
        if (exit_message)
            message =  _("There are currently jobs running.  Would you like to cancel all running jobs and exit?");
             else
            message = _("There are currently jobs running.  Would you like to cancel all running jobs and continue?");

        CustomPromptDlg dlg(m_frame->getFrameWindow(),
                            APPLICATION_NAME,
                            message);
        dlg.setButton1(ID_CancelAndExit, exit_message ? _("Cancel Jobs and Exit") : _("Cancel Jobs and Continue"));
        dlg.setButton2(ID_KeepJobsRunning, exit_message ? _("Don't Exit") : _("Don't Continue"));
        dlg.setButton3(ID_ForceExit, _("Force Exit"));

        if (exit_message)
        {
            dlg.showButtons(CustomPromptDlg::showButton1 |
                            CustomPromptDlg::showButton2 |
                            CustomPromptDlg::showButton3);
        }
         else
        {
            dlg.showButtons(CustomPromptDlg::showButton1 |
                            CustomPromptDlg::showButton2);
        }

        int result = dlg.ShowModal();
        if (result == ID_CancelAndExit)
        {
            // create the "cancelling jobs..." dialog
            DlgCancellingJobsWait* wait_dlg =
                new DlgCancellingJobsWait(m_frame->getFrameWindow());
            wait_dlg->Show(true);
            
            // show a busy cursor while we're cancelling the jobs
            AppBusyCursor bc;
            
            // make sure the dialog doesn't just pop up and go away
            // right away (appears like flicker)
            ::wxYield();
            wxMilliSleep(1500);
            
            // cancel all running jobs
            jobs::IJobInfoEnumPtr jobs = job_queue->getJobInfoEnum(jobStateRunning);
            size_t i, job_count = jobs->size();
            for (i = 0; i < job_count; i++)
            {
                jobs::IJobInfoPtr job_info = jobs->getItem(i);
                int job_id = job_info->getJobId();
                
                jobs::IJobPtr job = job_queue->lookupJob(job_id);
                if (job)
                    job->cancel();
            }
            
            // wait for all the jobs to cancel before we close the application
            bool cancelling = true;
            while (cancelling)
            {
                jobs = job_queue->getJobInfoEnum(jobStateCancelling);
                if (jobs->size() == 0)
                {
                    wait_dlg->Show(false);
                    cancelling = false;
                }
            }
            
            // destroy the wait dialog and exit the program
            wait_dlg->Destroy();
            return true;
        }
         else if (result == ID_KeepJobsRunning)
        {
            return false;
        }
         else if (result == ID_ForceExit)
        {
            return true;
        }

        return false;
    }

    return true;
}

bool AppController::checkForTemporaryFiles()
{
    // check to make sure no untitled/temporary documents
    // are open
    IDocumentSiteEnumPtr docsites;
    docsites = g_app->getMainFrame()->getDocumentSites(sitetypeNormal);

    size_t i, site_count = docsites->size();
    for (i = 0; i < site_count; ++i)
    {
        IDocumentSitePtr site;
        site = docsites->getItem(i);

        ITableDocPtr table_doc = site->getDocument();

        if (table_doc.isNull())
            continue;

        if (isTemporaryTable(towstr(table_doc->getPath())))
            return true;
    }
    
    return false;
}

bool AppController::closeProject()
{
    if (m_frame)
    {
        if (!m_frame->closeAll(false))
            return false;
    }

    // hide all modeless panels that were showing

    IDocumentSitePtr doc_site;
    IDocumentSiteEnumPtr sites;
    sites = m_frame->getDocumentSites(sitetypeModeless);

    size_t i, site_count = sites->size();
    for (i = 0; i < site_count; ++i)
    {
        doc_site = sites->getItem(i);
        doc_site->setVisible(false);
        sites->setItem(i, xcm::null);
    }


    TableDocMgr::clearModelRegistry();

    tango::IDatabasePtr db = g_app->getDatabase();
    g_app->setDatabase(xcm::null);

    if (db.isOk())
    {
        db->close();
        db = xcm::null;
    }

    if (m_dbdoc)
        m_dbdoc->setDatabase(xcm::null);
    
    g_app->setDatabaseLocation(wxEmptyString);

    return true;
}

bool AppController::cleanProject()
{
    int result = appMessageBox(_("Would you like to compact this project?"),
                                    APPLICATION_NAME,
                                    wxYES_NO | wxICON_QUESTION | wxCENTER);
    if (result != wxYES)
        return false;

    // make sure there are no running jobs
    if (!checkForRunningJobs())
        return false;

    AppBusyCursor bc;
    g_app->getDatabase()->cleanup();
    
    appMessageBox(_("Project successfully compacted."),
                  APPLICATION_NAME,
                  wxOK | wxICON_INFORMATION);
    
    return true;
}

void AppController::createMountPoint(const wxString& conn_str,
                                     const wxString& mount_path,
                                     bool refresh_tree)
{
    wxString path = mount_path;
    
    if (path.IsEmpty())
        path = wxT("/mount");
        
    if (path.Mid(0, 1) != wxT("/"))
        path.Prepend(wxT("/"));

    tango::IDatabasePtr db = g_app->getDatabase();
    db->setMountPoint(towstr(path), towstr(conn_str), L"/");
    
    if (refresh_tree)
        g_app->getAppController()->refreshDbDoc();
}


static void onExportWizardFinished(ExportWizard* dlg)
{
    dlg->getTemplate().execute();
}


void AppController::showCreateExternalConnectionWizard()
{
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("CreateConnectionWizard"));
    if (site.isNull())
    {
        ConnectionWizard* wizard = new ConnectionWizard;
        wizard->sigConnectionWizardFinished.connect(&onCreateExternalConnectionWizardFinished);

        site = g_app->getMainFrame()->createSite(wizard, sitetypeModeless,
                                                 -1, -1, 540, 480);
        site->setMinSize(540,480);
        site->setName(wxT("CreateConnectionWizard"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}


void AppController::showImportWizard(const ImportInfo& info,
                                     const wxString& location)
{
    AppBusyCursor bc;
    
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("ImportWizard"));
    if (site.isNull())
    {
        ImportWizard* wizard = new ImportWizard;
        wizard->sigImportWizardFinished.connect(&onImportWizardFinished);

        if (info.base_path.Length() > 0)
            wizard->getTemplate().m_ii = info;
            
        if (!location.IsEmpty())
        {
            wizard->loadTemplate(location);
            wizard->setMode(ImportWizard::ModeOpen);
        }

        site = g_app->getMainFrame()->createSite(wizard, sitetypeModeless,
                                                 -1, -1, 540, 480);
        site->setMinSize(540,480);
        site->setName(wxT("ImportWizard"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}


void AppController::showExportWizard(const ExportInfo& info,
                                     const wxString& location)
{
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("ExportWizard"));
    if (site.isNull())
    {
        ExportWizard* wizard = new ExportWizard;
        wizard->sigExportWizardFinished.connect(&onExportWizardFinished);

        if (info.tables.size() > 0)
            wizard->getTemplate().m_ei = info;

        site = g_app->getMainFrame()->createSite(wizard, sitetypeModeless,
                                                 -1, -1, 540, 480);
        site->setMinSize(540,480);
        site->setName(wxT("ExportWizard"));
        
        if (!location.IsEmpty())
            wizard->loadTemplate(location);
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

void AppController::showLicenseManager()
{    
    // make sure there are no running jobs
    if (!checkForRunningJobs())
        return;

    if (checkForTemporaryFiles())
    {
        appMessageBox(_("Untitled documents must be either saved or closed before using the license manager."),
                      _("License Manager"),
                      wxOK | wxICON_INFORMATION);
        return;
    }

    // show the license manager
    wxString version;
    version = wxString::Format(wxT("%d.%d.%d"),
                        APP_VERSION_MAJOR,
                        APP_VERSION_MINOR,
                        APP_VERSION_SUBMINOR);

    g_app->stopLicenseTimer();

    DlgAuth dlg(g_app->getMainWindow(),
                APPLICATION_NAME,
                version,
                g_auth);
    dlg.ShowModal();

    if (g_auth->checkAuth() != paladin::errNone)
    {
        // hide all modeless panels that were showing

        IDocumentSitePtr doc_site;
        IDocumentSiteEnumPtr sites;
        sites = m_frame->getDocumentSites(sitetypeModeless);

        size_t i, site_count = sites->size();
        for (i = 0; i < site_count; ++i)
        {
            doc_site = sites->getItem(i);
            doc_site->setVisible(false);
            sites->setItem(i, xcm::null);
        }

        g_app->getMainFrame()->closeAll(true);
        g_app->getMainWindow()->Close(true);
        return;
    }

    g_app->startLicenseTimer();
}

void AppController::showFindPanel()
{
    size_t child_count = g_app->getMainFrame()->getChildCount();
    
    FindPanel* panel;

    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("FindPanel"));
    if (site.isOk())
    {
        IDocumentPtr doc = site->getDocument();
        if (doc.isOk())
            panel = (FindPanel*)(doc->getDocumentWindow());
        
        IFindPanelPtr find_panel = doc;
        if (find_panel.isOk())
            find_panel->setMode(FindPanel::ModeFind, true);
        
        if (!site->getVisible())
            site->setVisible(true);
    }
    
    if (panel)
    {
        // populate the panel's find combobox with either
        // the global find/filter combobox's value or the
        // value queried from the active document
        
        wxString val;
        
        FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_FIND_PANEL_QUERY_FIND_VALUE);
        e->l_param = (unsigned long)(&val);
        g_app->getMainFrame()->sendEvent(e);
        
        if (val.IsEmpty())
        {
            val = g_app->getAppController()->
                         getStandardToolbar()->
                         getFindCombo()->
                         GetValue();
        }
        
        panel->setFindValue(val);
    }
}

void AppController::showFindInFilesPanel()
{
    size_t child_count = g_app->getMainFrame()->getChildCount();
    
    FindPanel* panel;

    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("FindPanel"));
    if (site.isOk())
    {
        IDocumentPtr doc = site->getDocument();
        if (doc.isOk())
            panel = (FindPanel*)(doc->getDocumentWindow());
        
        IFindPanelPtr find_panel = doc;
        if (find_panel.isOk())
            find_panel->setMode(FindPanel::ModeFindInFiles, true);
        
        if (!site->getVisible())
            site->setVisible(true);
    }
    
    if (panel)
    {
        // populate the panel's find combobox with either
        // the global find/filter combobox's value or the
        // value queried from the active document
        
        wxString val;
        
        FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_FIND_PANEL_QUERY_FIND_VALUE);
        e->l_param = (unsigned long)(&val);
        g_app->getMainFrame()->sendEvent(e);
        
        if (val.IsEmpty())
        {
            val = g_app->getAppController()->
                         getStandardToolbar()->
                         getFindCombo()->
                         GetValue();
        }
        
        panel->setFindValue(val);
    }
}

void AppController::showReplacePanel()
{
    FindPanel* panel;
    
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("FindPanel"));
    if (site.isOk())
    {
        IDocumentPtr doc = site->getDocument();
        if (doc.isOk())
        {
            panel = (FindPanel*)(doc->getDocumentWindow());
            if (panel != NULL)
                panel->setMode(FindPanel::ModeFindReplace, true);
        }
        
        IFindPanelPtr find_panel = doc;
        if (find_panel.isOk())
            find_panel->setMode(FindPanel::ModeFindReplace, true);
        
        if (!site->getVisible())
            site->setVisible(true);
    }
    
    if (panel)
    {
        // populate the panel's find combobox with either
        // the global find/filter combobox's value or the
        // value queried from the active document
        
        wxString val;
        
        FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_FIND_PANEL_QUERY_FIND_VALUE);
        e->l_param = (unsigned long)(&val);
        g_app->getMainFrame()->sendEvent(e);
        
        if (val.IsEmpty())
        {
            val = g_app->getAppController()->
                         getStandardToolbar()->
                         getFindCombo()->
                         GetValue();
        }
        
        panel->setFindValue(val);
    }
}

void AppController::showColumnListPanel()
{
    IDocumentSitePtr columnlist_site;
    columnlist_site = m_frame->lookupSite(wxT("FieldsPanel"));
    if (columnlist_site.isOk())
    {
        bool visible = columnlist_site->getVisible();

        if (!visible)
            columnlist_site->setVisible(true);
    }
}

void AppController::showMarkManagerPanel()
{
    IDocumentSitePtr markmanager_site;
    markmanager_site = m_frame->lookupSite(wxT("MarksPanel"));
    if (markmanager_site.isOk())
    {
        bool visible = markmanager_site->getVisible();

        if (!visible)
            markmanager_site->setVisible(true);
    }
}

void AppController::showStartupPage()
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    
    long counter = 0;
    
    #if APP_WELCOME_PAGES == 1
    counter = prefs->getLong(wxT("general.startup.counter"), 0);
    ++counter;
    if (counter < 10)
    {
        prefs->setLong(wxT("general.startup.counter"), counter);
        prefs->flush();
    }
    #endif

    
    wxString home_pages;
    home_pages = getAppPrefsString(wxT("general.location.home"));
    
    bool activate_last = false;
    
    #if APP_WELCOME_PAGES == 1
    if (counter == 1)
    {
        // if a welcome page is defined, show it
        wxString welcome_page = APP_WEBLOCATION_WELCOME1;
        if (welcome_page.Length() > 0)
        {
            home_pages = welcome_page;
        }
    }
     else if (counter == 5)
    {
        // if a welcome page is defined, show it
        wxString welcome_page = APP_WEBLOCATION_WELCOME2;
        if (welcome_page.Length() > 0)
        {
            activate_last = true;
            if (home_pages.Length() > 0)
                home_pages += wxT("|");
            home_pages += welcome_page;
        }
    }
    #endif
    
    
    
    // if there 5 or less days left, show the 'about to expire' page
    if (g_auth->getDaysLeft() <= 5)
    {
        // if a license expiration page is defined, show it
        wxString license_expiration = APP_WEBLOCATION_LICENSEEXPIRING;
        if (license_expiration.Length() > 0)
        {
            if (home_pages.Length() > 0)
                home_pages += wxT("|");
            home_pages += license_expiration;
        }
    }
    
    
    
    
    //  open the homepage(s)
    
    wxArrayString arr;
    wxStringTokenizer tkz(home_pages, wxT("|"));
    while (tkz.HasMoreTokens())
    {
        wxString t = tkz.GetNextToken();
        if (!t.IsEmpty())
        {
            int flags = appOpenForceNewWindow;
            
            if (activate_last && !tkz.HasMoreTokens())
                flags |= appOpenActivateNewWindow;
            
            openWeb(t, NULL, flags);
        }
    }
}

bool AppController::doReadOnlyCheck()
{
    if (g_app->isDatabaseReadOnly())
    {
        int result = appMessageBox(_("This project is currently set to 'read-only' status to protect your data from unintentional changes.  Would you like\nto temporarily remove the 'read-only' status to perform this operation?"),
                                        APPLICATION_NAME,
                                        wxYES_NO | wxICON_QUESTION | wxCENTER);
        if (result != wxYES)
            return false;
    }

    return true;
}


void AppController::showOptionsDialog()
{
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("OptionsPanel"));
    if (site.isNull())
    {
        int w = 530;
        int h = 500;
        
        #ifdef __WXGTK__
        if (wxNORMAL_FONT->GetPointSize() >= 11)
        {
            w = 680;
            h = 560;
        }
        #endif
        
        OptionsPanel* panel = new OptionsPanel;
        site = g_app->getMainFrame()->createSite(panel,
                                                 sitetypeModeless,
                                                 -1, -1,
                                                 w, h);
        site->setMinSize(w,h);
        site->setName(wxT("OptionsPanel"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

void AppController::showAbout()
{
    WebDoc* doc = new WebDoc;
    m_frame->createSite(doc, sitetypeNormal, -1, -1, -1, -1);

    // show the about dialog box if we have a valid web browser;
    // if we are on a 64-bit build, the embedded browser is probably
    // not wxWebConnect, therefore display the simple dialog box
    // on 64-bit builds as well

    if (!doc->isWebBrowserOk() || sizeof(void*) == 8)
    {
        wxString bit = wxT("");
        if (sizeof(void*) == 8)
            bit = wxT(" (64-bit)");

        wxString appname = APPLICATION_NAME;
        wxString message;
        message.Printf(wxT("%s %d.%d.%d.%d%s - %s\n\nThis software contains an unmodified binary version of the open-source XULRunner engine as provided by the Mozilla Foundation.  Please read the about:license screen for more information."),
                 appname.c_str(),
                 APP_VERSION_MAJOR,
                 APP_VERSION_MINOR,
                 APP_VERSION_SUBMINOR,
                 APP_VERSION_BUILDSERIAL,
                 bit.c_str(),
                 APP_COPYRIGHT);

        appMessageBox(message,
                           _("About"),
                           wxICON_INFORMATION);
    }
     else
    {
        doc->showWebRes(WebDoc::webresAbout);
    }
}

void AppController::showCreateTable()
{
    if (!g_app->isDatabaseOpen())
    {
        ::appMessageBox(_("To create a database table, please create or open a project."),
                             APPLICATION_NAME,
                             wxOK | wxICON_EXCLAMATION | wxCENTER);

        return;
    }

    newTable();
}


void AppController::showProjectManager()
{
    DlgProjectMgr dlg(g_app->getMainWindow());
    if (dlg.ShowModal() == wxID_OK)
    {
        ProjectInfo info = dlg.getSelectedProject();
        
        if (g_app->getDatabase().isOk())
        {
            // the project that we selected is already open; do nothing
            #ifdef WIN32
                if (info.location.CmpNoCase(g_app->getDatabaseLocation()) == 0)
                    return;
            #else
                if (info.location == g_app->getDatabaseLocation())
                    return;
            #endif
            
            if (!closeProject())
                return;
        }

        openProject(info.location, info.user_id, info.passwd);
    }
}


void AppController::showRelationshipManager()
{
    IDocumentSitePtr site;

    site = m_frame->lookupSite(wxT("RelationshipsPanel"));
    if (site.isNull())
    {
        int x, y, width, height;

        IAppPreferencesPtr prefs = g_app->getAppPreferences();

        x = prefs->getLong(wxT("window_dimensions.relationship_mgr.x"), -1);
        y = prefs->getLong(wxT("window_dimensions.relationship_mgr.y"), -1);
        width = prefs->getLong(wxT("window_dimensions.relationship_mgr.width"), 700);
        height = prefs->getLong(wxT("window_dimensions.relationship_mgr.height"), 500);

        x = wxMax(x, -2);
        y = wxMax(y, -2);
        width = wxMax(width, 50);
        height = wxMax(height, 50);
        
        RelationshipPanel* panel = new RelationshipPanel;
        site = m_frame->createSite(panel,
                                   sitetypeModeless |
                                   siteMaximizeButton,
                                   x, y, width, height);
        site->setMinSize(450,300);
        site->setCaption(_("Relationships"));
        site->setName(wxT("RelationshipsPanel"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

void AppController::showProjectPanel(bool show, bool focus)
{
    // TODO: this code parallels almost exactly the code for
    // toggling the panel; the reason for this is that we have 
    // various pieces of show/toggle code for different panels, 
    // and there isn't a systematic implementation for showing 
    // and/or hiding the panels; e.g. compare this implementation 
    // with showConsolePanel() and then toggleConsolePanel(); at 
    // this point, rather than trying to implement a systematic 
    // method for showing hiding panels, its easier to implement 
    // this function for the need at hand

    IDocumentSitePtr dbdoc_site;
    if (m_dbdoc)
        dbdoc_site = m_dbdoc->getDbDocSite();

    if (!dbdoc_site)
        return;

    dbdoc_site->setVisible(show);

    // if the project becomes visible, set the focus to
    // the project, otherwise, set the focus back to the
    // active child
    if (dbdoc_site->getVisible())
    {
        IDocumentPtr dbdoc = dbdoc_site->getDocument();
        if (focus && dbdoc.isOk())
            dbdoc->setDocumentFocus();
    }
    else
    {
        IDocumentSitePtr active_child_site = g_app->getMainFrame()->getActiveChild();
        
        if (active_child_site.isOk())
        {
            IDocumentPtr active_child_doc = active_child_site->getDocument();
            if (focus && active_child_doc.isOk())
                active_child_doc->setDocumentFocus();
        }
    }
}

void AppController::showConsolePanel(bool show, bool focus)
{
#if APP_GUI==1
    if (m_frame.isNull())
        return;
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("ConsolePanel"));
    if (site.isNull() && show)
    {
        ConsolePanel* panel = new ConsolePanel;
        site = m_frame->createSite(panel,
                                   sitetypeDockable | dockBottom,
                                   0, 0, 600, 175);
        site->setCaption(_("Console"));
        site->setName(wxT("ConsolePanel"));

        IDocumentPtr doc = site->getDocument();
        if (focus && doc.isOk())
            doc->setDocumentFocus();
    }
     else
    {
        site->setVisible(show);

        IDocumentPtr doc = site->getDocument();
        if (show && focus && doc.isOk())
            doc->setDocumentFocus();
    }
#endif
}


void AppController::showJobScheduler()
{
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("JobSchedulerPanel"));
    if (site.isNull())
    {
        JobSchedulerPanel* panel = new JobSchedulerPanel;
        site = g_app->getMainFrame()->createSite(panel, sitetypeModeless,
                                                 -1, -1, 640, 420);
        site->setMinSize(640,420);
        site->setName(wxT("JobSchedulerPanel"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}


void AppController::showJobManager(bool show)
{
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("JobsPanel"));
    if (site.isNull())
    {
        JobManagerPanel* panel = new JobManagerPanel;
        site = g_app->getMainFrame()->createSite(panel, sitetypeModeless,
                                                 -1, -1, 520, 560);
        site->setMinSize(520,320);
        site->setName(wxT("JobsPanel"));
    }
     else
    {
        if (site->getVisible() != show)
            site->setVisible(show);
    }

    m_job_update_timer->hideJobFailedIcon();
}


void AppController::showExtensionManager()
{
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("ExtensionsPanel"));
    if (site.isNull())
    {
        ExtensionManagerPanel* panel = new ExtensionManagerPanel;
        site = g_app->getMainFrame()->createSite(panel, sitetypeModeless,
                                                 -1, -1, 520, 560);
        site->setMinSize(520,320);
        site->setName(wxT("ExtensionsPanel"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}


void AppController::showProjectProperties()
{
    if (!g_app->isDatabaseOpen())
    {
        appMessageBox(_("A project must first be opened to display project properties."),
                           APPLICATION_NAME,
                           wxOK | wxICON_INFORMATION);
        return;
    }

    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("ProjectPropertiesPanel"));
    if (site.isNull())
    {
        DatabaseInfoPanel* panel = new DatabaseInfoPanel;
        site = g_app->getMainFrame()->createSite(panel, sitetypeModeless,
                                                 -1, -1, 420, 200);
        site->setMinSize(420,200);
        site->setName(wxT("ProjectPropertiesPanel"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

void AppController::toggleFullScreen()
{
    // note: leave the status bar (for job status) and the toolbar visible
    wxFrame* frame = g_app->getMainFrame()->getFrameWindow();
    frame->ShowFullScreen(!frame->IsFullScreen(),
                           wxFULLSCREEN_NOMENUBAR |
                           wxFULLSCREEN_NOBORDER |
                           wxFULLSCREEN_NOCAPTION);
}

void AppController::toggleLock()
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    m_data_locked = !m_data_locked;
    prefs->setBoolean(wxT("app.data_locked"), m_data_locked);
}


void AppController::toggleAllPanels()
{
    if (m_hidden_sites.size() > 0)
    {
        IDocumentSitePtr site;

        std::vector<HiddenSiteInfo>::iterator it;
        for (it = m_hidden_sites.begin();
             it != m_hidden_sites.end(); ++it)
        {
            site = g_app->getMainFrame()->lookupSite(it->site_name);
            if (site.isOk())
            {
                g_app->getMainFrame()->dockSite(site,
                                                it->site_flags,
                                                it->x,
                                                it->y,
                                                it->w,
                                                it->h);
            }
        }

        g_app->getMainFrame()->refreshFrameLayout();

        m_hidden_sites.clear();
    }
     else
    {
        IDocumentSiteEnumPtr docsites;
        IDocumentSitePtr site;
        IDocumentPtr doc;
        wxWindow* docwnd;

        docsites = m_frame->getDocumentSites(sitetypeDockable);

        std::vector<IDocumentSitePtr> all_sites;
        bool show = true;

        size_t i, site_count = docsites->size();
        for (i = 0; i < site_count; ++i)
        {
            site = docsites->getItem(i);

            if (!site->getVisible())
                continue;

            if (!(site->getSiteType() & (dockLeft | dockBottom | dockRight)))
                continue;

            wxString site_name = site->getName();

            if (site_name.Length() == 0)
                continue;

            doc = site->getDocument();
            if (doc.isOk())
            {
                docwnd = doc->getDocumentWindow();
                if (docwnd)
                {
                    if (!docwnd->IsKindOf(CLASSINFO(wxToolBar)) &&
                        !docwnd->IsKindOf(CLASSINFO(wxAuiToolBar)))
                    {
                        HiddenSiteInfo h;
                        h.site_name = site_name;
                        h.site_flags = site->getSiteType();
                        site->getPosition(&h.x, &h.y, &h.w, &h.h);
                        m_hidden_sites.push_back(h);
                    }
                }
            }
        }

        std::vector<HiddenSiteInfo>::iterator it;

        for (it = m_hidden_sites.begin(); it != m_hidden_sites.end(); ++it)
        {
            IDocumentSitePtr site;
            site = g_app->getMainFrame()->lookupSite(it->site_name);
            if (site)
            {
                site->setVisible(false);
            }
        }
    }
}

void AppController::toggleProjectPanel()
{
    IDocumentSitePtr dbdoc_site;
    if (m_dbdoc)
        dbdoc_site = m_dbdoc->getDbDocSite();

    if (!dbdoc_site)
        return;

    bool visible = dbdoc_site->getVisible();
    dbdoc_site->setVisible(!visible);

    // TODO: may want to generalize focus setting code so
    // that other panels that are toggled behave similarly

    // if the project becomes visible, set the focus to
    // the project, otherwise, set the focus back to the
    // active child
    if (dbdoc_site->getVisible())
    {
        IDocumentPtr dbdoc = dbdoc_site->getDocument();
        if (dbdoc.isOk())
            dbdoc->setDocumentFocus();
    }
    else
    {
        IDocumentSitePtr active_child_site = g_app->getMainFrame()->getActiveChild();
        
        if (active_child_site.isOk())
        {
            IDocumentPtr active_child_doc = active_child_site->getDocument();
            if (active_child_doc.isOk())
                active_child_doc->setDocumentFocus();
        }
    }
}

void AppController::toggleConsolePanel()
{
    IDocumentSitePtr console_site;
    console_site = m_frame->lookupSite(wxT("ConsolePanel"));
    if (console_site.isOk())
    {
        bool visible = console_site->getVisible();
        console_site->setVisible(!visible);

        // TODO: may want to generalize focus setting code so
        // that other panels that are toggled behave similarly

        // if the console becomes visible, set the focus to
        // the console, otherwise, set the focus back to the
        // active child
        if (console_site->getVisible())
        {
            IDocumentPtr console_doc = console_site->getDocument();
            if (console_doc.isOk())
                console_doc->setDocumentFocus();
        }
        else
        {
            IDocumentSitePtr active_child_site = g_app->getMainFrame()->getActiveChild();
            
            if (active_child_site.isOk())
            {
                IDocumentPtr active_child_doc = active_child_site->getDocument();
                if (active_child_doc.isOk())
                    active_child_doc->setDocumentFocus();
            }
        }
    }
}

void AppController::toggleColumnListPanel()
{
    IDocumentSitePtr columnlist_site;
    columnlist_site = m_frame->lookupSite(wxT("FieldsPanel"));
    if (columnlist_site.isOk())
    {
        bool visible = columnlist_site->getVisible();
        columnlist_site->setVisible(!visible);
    }
}

void AppController::toggleMarkManagerPanel()
{
    IDocumentSitePtr markmanager_site;
    markmanager_site = m_frame->lookupSite(wxT("MarksPanel"));
    if (markmanager_site.isOk())
    {
        bool visible = markmanager_site->getVisible();
        markmanager_site->setVisible(!visible);
    }
}

void AppController::toggleRelationshipPanel()
{
    IDocumentSitePtr relmanager_site;
    relmanager_site = m_frame->lookupSite(wxT("RelationshipsPanel"));
    if (relmanager_site.isOk())
    {
        g_app->getMainFrame()->closeSite(relmanager_site);
    }
     else
    {
        showRelationshipManager();
    }
}

void AppController::toggleStatusBar()
{
    IStatusBarPtr statusbar = m_frame->getStatusBar();
    if (statusbar.isOk())
    {
        wxAuiToolBar* wnd = statusbar->getStatusBarCtrl();
        
        m_frame->showWindow(wnd, !wnd->IsShown());
        m_frame->refreshFrameLayout();
    }
}

void AppController::toggleToolbar(wxWindowID id)
{
    wxWindowID wnd_id = -1;

    switch (id)
    {
        case ID_Frame_ToggleStandardToolbar:    wnd_id = ID_Toolbar_Standard; break;
        case ID_Frame_ToggleLinkToolbar:        wnd_id = ID_Toolbar_Link; break;
        case ID_Frame_ToggleFormatToolbar:      wnd_id = ID_Toolbar_Format; break;
        default:
            return;
    }

    wxWindow* wnd = m_frame->findWindow(wnd_id);
    if (!wnd)
        return;
    
    m_frame->getFrameWindow()->Freeze();
    m_frame->showWindow(wnd, !wnd->IsShown());
    
    if (id == ID_Frame_ToggleFormatToolbar)
    {
        // let everyone know we've toggled the format toolbar on or off
        FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_FORMAT_TOOLBAR_TOGGLED);
        if (wnd->IsShown())
            e->l_param = 1;
             else
            e->l_param = 0;
        g_app->getMainFrame()->sendEvent(e);
    }
    
    m_frame->refreshFrameLayout();
    m_frame->getFrameWindow()->Thaw();
}


void AppController::refreshDbDoc()
{
#if APP_GUI==1
    // because refreshing the tree/linkbar involves gui operations,
    // we will check to make sure that we are running in the
    // main thread
    
    if (kl::Thread::isMain())
    {
        // running in the main thread, simply invoke the refresh
        wxCommandEvent e;
        e.SetId(ID_TreeRefresh);
        onTreeRefresh(e);
    }
     else
    {
        // we are not running in the main thread, so fire an event
        // to ourselves.  The event will be handled in the main thread
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, ID_TreeRefresh);
        ::wxPostEvent(this, e);
    }
#endif
}

void AppController::refreshLinkBar()
{
#if APP_GUI==1
    // because refreshing the tree/linkbar involves gui operations,
    // we will check to make sure that we are running in the
    // main thread
    
    if (kl::Thread::isMain())
    {
        // running in the main thread, simply invoke the refresh
        wxCommandEvent e;
        e.SetId(ID_LinkBarRefresh);
        onTreeRefresh(e);
    }
     else
    {
        // we are not running in the main thread, so fire an event
        // to ourselves.  The event will be handled in the main thread
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, ID_LinkBarRefresh);
        ::wxPostEvent(this, e);
    }
#endif
}

void AppController::onTreeRefresh(wxCommandEvent& evt)
{
    // this is guaranteed to be running in the main thread
    
    if (evt.GetId() == ID_TreeRefresh)
    {
        if (m_dbdoc)
            m_dbdoc->refresh();
        if (m_linkbar)
            m_linkbar->refresh();
    }
     else
    {
        if (m_linkbar)
            m_linkbar->refresh();
    }
}

