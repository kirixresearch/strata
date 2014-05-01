/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  wxWebConnect Test Application
 * Author:   Benjamin I. Williams
 * Created:  2009-05-14
 *
 */


#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/printdlg.h>
#include "wx/artprov.h"
#include "wx/aui/aui.h"
#include "../webconnect/webcontrol.h"


// web control and url combobox ids
const int wxID_WEB = 9001;
const int wxID_URL = 9002;


class MyFrame : public wxFrame
{
    enum
    {
        ID_About = wxID_HIGHEST+1,
        
        // file
        ID_OpenHref,
        ID_OpenLocation,
        ID_Close,
        ID_SaveAs,
        ID_PageSetup,
        ID_Print,
        ID_Exit,
        
        // edit
        ID_Undo,
        ID_Redo,
        ID_Cut,
        ID_Copy,
        ID_CopyLink,
        ID_Paste,
        ID_SelectAll,
        ID_Find,
        ID_FindAgain,
        
        // view
        ID_GoBack,
        ID_GoForward,
        ID_GoHome,
        ID_Stop,
        ID_Reload,
        ID_ZoomIn,
        ID_ZoomOut,
        ID_ZoomReset,
        ID_ShowSource,
        ID_ShowLinks,
        
        // help
        ID_GoHelp,
        ID_GoForums,
        ID_GoAbout
    };

public:
    MyFrame(wxWindow* parent,
            wxWindowID id,
            const wxString& title,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_FRAME_STYLE | wxSUNKEN_BORDER);

    ~MyFrame();

private:

    void OnSize(wxSizeEvent& evt);
    void OnEraseBackground(wxEraseEvent& evt);

    void OnUrlTextEntered(wxCommandEvent& evt);
    void OnUrlItemSelected(wxCommandEvent& evt);    

    void OnOpenHref(wxCommandEvent& evt);
    void OnOpenLocation(wxCommandEvent& evt);
    void OnClose(wxCommandEvent& evt);
    void OnSaveAs(wxCommandEvent& evt);
    void OnPageSetup(wxCommandEvent& evt);
    void OnPrint(wxCommandEvent& evt);
    void OnExit(wxCommandEvent& evt);

    void OnUndo(wxCommandEvent& evt);
    void OnRedo(wxCommandEvent& evt);
    void OnCut(wxCommandEvent& evt);
    void OnCopy(wxCommandEvent& evt);
    void OnCopyLink(wxCommandEvent& evt);
    void OnPaste(wxCommandEvent& evt);
    void OnSelectAll(wxCommandEvent& evt);
    void OnFind(wxCommandEvent& evt);
    void OnFindAgain(wxCommandEvent& evt);

    void OnGoBack(wxCommandEvent& evt);
    void OnGoForward(wxCommandEvent& evt);
    void OnGoHome(wxCommandEvent& evt);
    void OnStop(wxCommandEvent& evt);
    void OnReload(wxCommandEvent& evt);
    void OnZoomIn(wxCommandEvent& evt);
    void OnZoomOut(wxCommandEvent& evt);
    void OnZoomReset(wxCommandEvent& evt);
    void OnShowSource(wxCommandEvent& evt);    
    void OnShowLinks(wxCommandEvent& evt);

    void OnGoHelp(wxCommandEvent& evt);
    void OnGoForums(wxCommandEvent& evt);
    void OnGoAbout(wxCommandEvent& evt);        

    void OnStatusText(wxWebEvent& evt);
    void OnStatusChange(wxWebEvent& evt);
    void OnStateChange(wxWebEvent& evt);
    void OnLocationChange(wxWebEvent& evt);
    void OnTitleChange(wxWebEvent& evt);
    void OnShowContextMenu(wxWebEvent& evt);
    void OnInitDownload(wxWebEvent& evt);
    void OnShouldHandleContent(wxWebEvent& evt);
    void OnDOMContentLoaded(wxWebEvent& evt);

    void OnUpdateUI(wxUpdateUIEvent& evt);

private:

    void SetUrlBarValue(const wxString& str);
    void ShowLinks();
    void GetChildLinks(wxDOMNode node, wxArrayString& arr);

private:

    // frame manager
    wxAuiManager m_mgr;

    // web control and url bar
    wxWebControl* m_browser;
    wxComboBox* m_urlbar;

    // default locations
    wxString m_uri_home;
    wxString m_uri_help;
    wxString m_uri_forums;
    wxString m_uri_about;     

    // href location
    wxString m_uri_href;

    // find text/flags
    wxString m_find_text;
    unsigned int m_find_flags;

    // dom content
    bool m_dom_contentloaded;

    DECLARE_EVENT_TABLE();
};


class MyApp : public wxApp
{
public:

    bool OnInit()
    {
        // Locate the XULRunner engine; the following call will look for 
        // a directory named "xr"
        wxString xulrunner_path = FindXulRunner(wxT("xr"));
        if (xulrunner_path.IsEmpty())
        {
            wxMessageBox(wxT("Could not find xulrunner directory"));
            return false;
        }    
    
        // Locate some common paths and initialize the control with
        // the plugin paths; add these common plugin directories to 
        // MOZ_PLUGIN_PATH
        wxString program_files_dir;
        ::wxGetEnv(wxT("ProgramFiles"), &program_files_dir);
        if (program_files_dir.Length() == 0 || program_files_dir.Last() != '\\')
            program_files_dir += wxT("\\");

        wxString dir = program_files_dir;
        dir += wxT("Mozilla Firefox\\plugins");
        wxWebControl::AddPluginPath(dir);

        // to install the flash plugin automatically, if it exists, 
        // add a path to the flash location; for example, on windows,
        // if the system directory is given by system_dir, then, we have:
        //
        // wxString dir2 = system_dir;
        // dir2 += wxT("Macromed\\Flash");
        // wxWebControl::AddPluginPath(dir2);

        // Finally, initialize the engine; Calling wxWebControl::InitEngine()
        // is very important and has to be made before using wxWebControl.  
        // It instructs wxWebConnect where the xulrunner directory is.
        wxWebControl::InitEngine(xulrunner_path);

        // Create and show the frame
        wxFrame* frame = new MyFrame(NULL,
                                     wxID_ANY,
                                     wxT("Gecko Embedding Test"),
                                     wxDefaultPosition,
                                     wxSize(1024, 768));
        SetTopWindow(frame);
        frame->Show();

        return true;
    }
    
    wxString FindXulRunner(const wxString& xulrunner_dirname)
    {
        // get the location of this executable
        wxString exe_path = wxStandardPaths::Get().GetExecutablePath();
        wxString path_separator = wxFileName::GetPathSeparator();
        exe_path = exe_path.BeforeLast(path_separator[0]);
        exe_path += path_separator;

        wxString path;

        // first, check <exe_path>/<xulrunner_path>
        path = exe_path + xulrunner_dirname;
        if (wxDir::Exists(path))
            return path;
  
        // next, check <exe_path>/../<xulrunner_path>
        path = exe_path + wxT("..") + path_separator + xulrunner_dirname;
        if (wxDir::Exists(path))
            return path;

        // finally, check <exe_path>/../../<xulrunner_path>
        path = exe_path + wxT("..") + path_separator + wxT("..") + path_separator + xulrunner_dirname;
        if (wxDir::Exists(path))
            return path;

        return wxEmptyString;
    }
};

DECLARE_APP(MyApp);
IMPLEMENT_APP(MyApp);



BEGIN_EVENT_TABLE(MyFrame, wxFrame)

    // frame events
    EVT_SIZE(MyFrame::OnSize)
    EVT_ERASE_BACKGROUND(MyFrame::OnEraseBackground)

    // url combobox events
    EVT_TEXT_ENTER(wxID_URL, MyFrame::OnUrlTextEntered)
    EVT_COMBOBOX(wxID_URL, MyFrame::OnUrlItemSelected)

    // menu events
    EVT_MENU(ID_OpenHref, MyFrame::OnOpenHref)
    EVT_MENU(ID_OpenLocation, MyFrame::OnOpenLocation)
    EVT_MENU(ID_Close, MyFrame::OnClose)
    EVT_MENU(ID_SaveAs, MyFrame::OnSaveAs)
    EVT_MENU(ID_PageSetup, MyFrame::OnPageSetup)
    EVT_MENU(ID_Print, MyFrame::OnPrint)
    EVT_MENU(ID_Exit, MyFrame::OnExit)
    EVT_MENU(ID_Cut, MyFrame::OnCut)
    EVT_MENU(ID_Copy, MyFrame::OnCopy)
    EVT_MENU(ID_CopyLink, MyFrame::OnCopyLink)
    EVT_MENU(ID_Paste, MyFrame::OnPaste)
    EVT_MENU(ID_SelectAll, MyFrame::OnSelectAll)
    EVT_MENU(ID_Find, MyFrame::OnFind)
    EVT_MENU(ID_FindAgain, MyFrame::OnFindAgain)
    EVT_MENU(ID_GoBack, MyFrame::OnGoBack)
    EVT_MENU(ID_GoForward, MyFrame::OnGoForward)
    EVT_MENU(ID_GoHome, MyFrame::OnGoHome)
    EVT_MENU(ID_Stop, MyFrame::OnStop)
    EVT_MENU(ID_Reload, MyFrame::OnReload)
    EVT_MENU(ID_ZoomIn, MyFrame::OnZoomIn)
    EVT_MENU(ID_ZoomOut, MyFrame::OnZoomOut)
    EVT_MENU(ID_ZoomReset, MyFrame::OnZoomReset)
    EVT_MENU(ID_ShowSource, MyFrame::OnShowSource)
    EVT_MENU(ID_ShowLinks, MyFrame::OnShowLinks)
    EVT_MENU(ID_GoHelp, MyFrame::OnGoHelp)
    EVT_MENU(ID_GoForums, MyFrame::OnGoForums)
    EVT_MENU(ID_GoAbout, MyFrame::OnGoAbout)

    // update events
    EVT_UPDATE_UI(ID_Undo, MyFrame::OnUpdateUI)
    EVT_UPDATE_UI(ID_Redo, MyFrame::OnUpdateUI)
    EVT_UPDATE_UI(ID_Cut, MyFrame::OnUpdateUI)
    EVT_UPDATE_UI(ID_Copy, MyFrame::OnUpdateUI)
    EVT_UPDATE_UI(ID_Copy, MyFrame::OnUpdateUI)
    EVT_UPDATE_UI(ID_FindAgain, MyFrame::OnUpdateUI)
    EVT_UPDATE_UI(ID_ShowSource, MyFrame::OnUpdateUI)    
    EVT_UPDATE_UI(ID_ShowLinks, MyFrame::OnUpdateUI)

    // web events
    EVT_WEB_STATUSTEXT(wxID_WEB, MyFrame::OnStatusText)
    EVT_WEB_STATUSCHANGE(wxID_WEB, MyFrame::OnStatusChange)
    EVT_WEB_STATECHANGE(wxID_WEB, MyFrame::OnStateChange)
    EVT_WEB_LOCATIONCHANGE(wxID_WEB, MyFrame::OnLocationChange)
    EVT_WEB_TITLECHANGE(wxID_WEB, MyFrame::OnTitleChange)
    EVT_WEB_SHOWCONTEXTMENU(wxID_WEB, MyFrame::OnShowContextMenu)
    EVT_WEB_INITDOWNLOAD(wxID_WEB, MyFrame::OnInitDownload)
    EVT_WEB_SHOULDHANDLECONTENT(wxID_WEB, MyFrame::OnShouldHandleContent)
    EVT_WEB_DOMCONTENTLOADED(wxID_WEB, MyFrame::OnDOMContentLoaded)    

END_EVENT_TABLE()



MyFrame::MyFrame(wxWindow* parent,
                 wxWindowID id,
                 const wxString& title,
                 const wxPoint& pos,
                 const wxSize& size,
                 long style)
        : wxFrame(parent, id, title, pos, size, style)
{
    // set frame icon
    #ifdef __WXMSW__
    SetIcon(wxIcon(wxT("mondrian"), wxBITMAP_TYPE_ICO_RESOURCE, 16, 16));
    #endif


    // use wxAuiManager to manage this frame
    m_mgr.SetManagedWindow(this);


    // create the menu
    wxMenuBar* mb = new wxMenuBar;

    wxMenu* file_menu = new wxMenu;
    file_menu->Append(ID_OpenLocation, _("&Open Location...\tCtrl+L"));
    file_menu->Append(ID_Close, _("&Close\tCtrl+W"));
    file_menu->AppendSeparator();    
    file_menu->Append(ID_SaveAs, _("&Save As...\tCtrl+S"));
    file_menu->AppendSeparator();
    file_menu->Append(ID_PageSetup, _("Page Set&up..."));
    file_menu->Append(ID_Print, _("&Print...\tCtrl+P"));
    file_menu->AppendSeparator();
    file_menu->Append(ID_Exit, _("E&xit"));

    wxMenu* edit_menu = new wxMenu;
    edit_menu->Append(ID_Undo, _("&Undo\tCtrl+Z"));
    edit_menu->Append(ID_Redo, _("&Redo\tCtrl+Y"));
    edit_menu->AppendSeparator();    
    edit_menu->Append(ID_Cut, _("Cu&t\tCtrl+X"));
    edit_menu->Append(ID_Copy, _("&Copy\tCtrl+C"));
    edit_menu->Append(ID_Paste, _("&Paste\tCtrl+V"));
    edit_menu->Append(ID_SelectAll, _("Select &All\tCtrl+Z"));
    edit_menu->AppendSeparator();    
    edit_menu->Append(ID_Find, _("&Find...\tCtrl+F"));
    edit_menu->Append(ID_FindAgain, _("Find A&gain\tCtrl+G"));    

    wxMenu* view_menu = new wxMenu;
    view_menu->Append(ID_GoBack, _("Go &Back"));
    view_menu->Append(ID_GoForward, _("Go &Forward"));
    view_menu->Append(ID_GoHome, _("&Home"));
    view_menu->AppendSeparator();
    view_menu->Append(ID_Stop, _("&Stop\tEsc"));
    view_menu->Append(ID_Reload, _("&Reload\tCtrl+R"));
    view_menu->AppendSeparator();
    view_menu->Append(ID_ZoomIn, _("Zoom &In"));
    view_menu->Append(ID_ZoomOut, _("Zoom &Out"));
    view_menu->Append(ID_ZoomReset, _("Reset &Zoom"));
    view_menu->AppendSeparator();
    view_menu->Append(ID_ShowSource, _("Sho&w Source"));
    view_menu->Append(ID_ShowLinks, _("Show &Links"));

    wxMenu* help_menu = new wxMenu;
    help_menu->Append(ID_GoHelp, _("Help\tF1"));
    help_menu->Append(ID_GoForums, _("Forums"));
    help_menu->Append(ID_GoAbout, _("About"));

    mb->Append(file_menu, _("&File"));
    mb->Append(edit_menu, _("&Edit"));
    mb->Append(view_menu, _("&View"));
    mb->Append(help_menu, _("&Help"));
    SetMenuBar(mb);

    CreateStatusBar();
    GetStatusBar()->SetStatusText(_("Ready"));


    // create the main toolbar
    wxAuiToolBar* toolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                             wxAUI_TB_DEFAULT_STYLE);
    toolbar->SetToolBitmapSize(wxSize(24,24));
    toolbar->AddTool(ID_GoBack, wxT("Go Back"), wxArtProvider::GetBitmap(wxART_GO_BACK));
    toolbar->AddTool(ID_GoForward, wxT("Go Forward"), wxArtProvider::GetBitmap(wxART_GO_FORWARD));
    toolbar->AddTool(ID_GoHome, wxT("Go Home"), wxArtProvider::GetBitmap(wxART_GO_HOME));

    m_urlbar = new wxComboBox(toolbar, wxID_URL, wxT(""), wxPoint(0,0), wxSize(850,23));    
    toolbar->AddControl(m_urlbar, wxT("Location"));

    toolbar->Realize();


    // create the main browser control
    m_browser = new wxWebControl(this, wxID_WEB, wxPoint(0,0), wxSize(800,600));


    // add the toolbar to the manager
    m_mgr.AddPane(toolbar, wxAuiPaneInfo().
                  Name(wxT("Toolbar")).Caption(wxT("Toolbar")).
                  ToolbarPane().Top().
                  LeftDockable(false).RightDockable(false));    


    // add the browser to the manager
    m_mgr.AddPane(m_browser, wxAuiPaneInfo().
                  Name(wxT("Browser")).
                  CenterPane().Show());


    // update the wxAUI manager
    m_mgr.Update();    


    // set the default browser preferences; to learn
    // more about the Mozilla preference values, see
    // http://kb.mozillazine.org/About:config_entries

    // for the example, disable cookies and popups
    wxWebPreferences webprefs = wxWebControl::GetPreferences();
    webprefs.SetIntPref(wxT("network.cookie.cookieBehavior"), 2 /* disable all cookies */);
    webprefs.SetIntPref(wxT("privacy.popups.policy"), 2 /* reject popups */);    

    // example proxy settings
    // webprefs.SetIntPref(wxT("network.proxy.type"),           0 /* no proxy */);
    // webprefs.SetIntPref(wxT("network.proxy.type"),           1 /* manual proxy */);
    // webprefs.SetIntPref(wxT("network.proxy.type"),           4 /* auto-detect proxy */);
    // webprefs.SetStringPref( wxT("network.proxy.http"),       wxT(""));
    // webprefs.SetIntPref(    wxT("network.proxy.http_port"),  0);
    // webprefs.SetStringPref( wxT("network.proxy.ftp"),        wxT(""));
    // webprefs.SetIntPref(    wxT("network.proxy.ftp_port"),   0);
    // webprefs.SetStringPref( wxT("network.proxy.ssl"),        wxT(""));
    // webprefs.SetIntPref(    wxT("network.proxy.ssl_port"),   0);
    // webprefs.SetStringPref( wxT("network.proxy.socks"),      wxT(""));
    // webprefs.SetIntPref(    wxT("network.proxy.socks_port"), 0);

    // example cookie preferences
    // webprefs.SetIntPref(wxT("network.cookie.cookieBehavior"), 0 /* enable all cookies */);
    // webprefs.SetIntPref(wxT("network.cookie.cookieBehavior"), 2 /* disable all cookies */);
    // webprefs.SetIntPref(wxT("network.cookie.lifetime.days"),  0);
    // webprefs.SetIntPref(wxT("network.cookie.lifetimePolicy"), 2 /* accept for session only */);    
    // webprefs.SetIntPref(wxT("network.cookie.lifetimePolicy"), 3 /* keep for number of days specified in "privacy.cookie.lifetime_days" */);

    // example popup preferences
    // webprefs.SetIntPref(wxT("privacy.popups.policy"), 1 /* accept popups */);
    // webprefs.SetIntPref(wxT("privacy.popups.policy"), 2 /* reject popups */);


    // set the default home and help URIs
    m_uri_home = wxT("http://www.kirix.com/labs");
    m_uri_help = wxT("http://www.kirix.com/labs/wxwebconnect/documentation/quick-facts.html");
    m_uri_forums = wxT("http://www.kirix.com/forums/");
    m_uri_about = wxT("http://www.kirix.com/labs/wxwebconnect.html");       

    // set the DOM content loaded flag
    m_dom_contentloaded = false;

    // open the home location
    m_browser->OpenURI(m_uri_home);
}

MyFrame::~MyFrame()
{
    m_mgr.UnInit();
}

void MyFrame::OnSize(wxSizeEvent& evt)
{
}

void MyFrame::OnEraseBackground(wxEraseEvent& evt)
{
}

void MyFrame::OnUrlTextEntered(wxCommandEvent& evt)
{
    m_browser->OpenURI(evt.GetString());
}

void MyFrame::OnUrlItemSelected(wxCommandEvent& evt)
{
    m_browser->OpenURI(evt.GetString());
}

void MyFrame::OnOpenHref(wxCommandEvent& evt)
{
    m_browser->OpenURI(m_uri_href);
}

void MyFrame::OnOpenLocation(wxCommandEvent& evt)
{
    wxTextEntryDialog dlg(this,
                          wxT("Please enter the URL you wish to open:"),
                          wxT("Open URL"),
                          wxT(""));

    int res = dlg.ShowModal();
    if (res != wxID_OK)
        return;

    m_browser->OpenURI(dlg.GetValue());
}

void MyFrame::OnClose(wxCommandEvent& evt)
{
    Close();
}

void MyFrame::OnSaveAs(wxCommandEvent& evt)
{
    wxString filter;
    filter += _("All Files");
    filter += wxT(" (*.*)|*.*|");  // some html pages simply contain images
    filter += _("HTML Files");
    filter += wxT(" (*.html, *.htm)|*.html;*.htm");
    
    wxFileDialog dlg(this,
                     _("Save As"),
                     wxT(""),
                     wxT(""),
                     filter,
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() != wxID_OK)
        return;

    m_browser->SaveCurrent(dlg.GetPath());
}

void MyFrame::OnPageSetup(wxCommandEvent& evt)
{
    // TODO: need to handle page orientation; need better
    // unit handling and/or interface, so we don't always 
    // have to convert between inches and millimeters

    // get the web control page settings;
    // currently, units are in inches
    int orientation;
    double page_width = 0;
    double page_height = 0;
    double left_margin = 0;
    double right_margin = 0;
    double top_margin = 0;
    double bottom_margin = 0;

    m_browser->GetPageSettings(&orientation, &page_width, &page_height,
                               &left_margin, &right_margin, 
                               &top_margin, &bottom_margin);

    // convert the page settings to dialog data;
    // dialog data units in in millimeters
    wxPageSetupDialogData data;
    data.SetPaperSize(wxSize(page_width*25.4, page_height*25.4));
    data.SetMarginTopLeft(wxPoint(left_margin*25.4, top_margin*25.4));
    data.SetMarginBottomRight(wxPoint(right_margin*25.4, bottom_margin*25.4));

    // initialize the dialog with the page settings
    wxPageSetupDialog dlg(this, &data);
    int res = dlg.ShowModal();
    if (res != wxID_OK)
        return;

    // ok pressed; get the dialog data
    data = dlg.GetPageSetupData();

    // convert the dialog data from mm back to inches
    int paper_orientation = wxPORTRAIT;
    wxSize paper_size = data.GetPaperSize();
    wxPoint top_left_margin = data.GetMarginTopLeft();
    wxPoint bottom_right_margin = data.GetMarginBottomRight();

    page_width = paper_size.GetWidth()/25.4;
    page_height = paper_size.GetHeight()/25.4;    
    left_margin = top_left_margin.x/25.4;
    top_margin = top_left_margin.y/25.4;    
    right_margin = bottom_right_margin.x/25.4;
    bottom_margin = bottom_right_margin.y/25.4;      

    // save the page settings
    m_browser->SetPageSettings(paper_orientation, page_width, page_height,
                               left_margin, right_margin, top_margin, bottom_margin);    
}

void MyFrame::OnPrint(wxCommandEvent& evt)
{
    m_browser->Print();
}

void MyFrame::OnExit(wxCommandEvent& evt)
{
    Close();
}

void MyFrame::OnUndo(wxCommandEvent& evt)
{
    // TODO: needs to be implemented in browser control
}

void MyFrame::OnRedo(wxCommandEvent& evt)
{
    // TODO: needs to be implemented in browser control
}

void MyFrame::OnCut(wxCommandEvent& evt)
{
    m_browser->CutSelection();
}

void MyFrame::OnCopy(wxCommandEvent& evt)
{
    m_browser->CopySelection();
}

void MyFrame::OnCopyLink(wxCommandEvent& evt)
{
    m_browser->CopyLinkLocation();
}

void MyFrame::OnPaste(wxCommandEvent& evt)
{
    m_browser->Paste();
}

void MyFrame::OnSelectAll(wxCommandEvent& evt)
{
    m_browser->SelectAll();
}

void MyFrame::OnFind(wxCommandEvent& evt)
{
    wxTextEntryDialog dlg(this,
                          wxT("Please enter the text you wish to find:"),
                          wxT("Find Text"),
                          wxT(""));

    int res = dlg.ShowModal();
    if (res != wxID_OK)
        return;

    // set the find text
    m_find_text = dlg.GetValue();

    // set the flags to use when finding the text
    m_find_flags = 0;
    m_find_flags |= wxWEB_FIND_WRAP;
    m_find_flags |= wxWEB_FIND_SEARCH_FRAMES;
    
    // some additional flags:
    // wxWEB_FIND_BACKWARDS
    // wxWEB_FIND_MATCH_CASE
    // wxWEB_FIND_ENTIRE_WORD

    m_browser->Find(m_find_text, m_find_flags);
}

void MyFrame::OnFindAgain(wxCommandEvent& evt)
{
    m_browser->Find(m_find_text, m_find_flags);
}

void MyFrame::OnGoBack(wxCommandEvent& evt)
{
    m_browser->GoBack();
}

void MyFrame::OnGoForward(wxCommandEvent& evt)
{
    m_browser->GoForward();
}

void MyFrame::OnGoHome(wxCommandEvent& evt)
{
    m_browser->OpenURI(m_uri_home);
}

void MyFrame::OnStop(wxCommandEvent& evt)
{
    m_browser->Stop();
}

void MyFrame::OnReload(wxCommandEvent& evt)
{
    m_browser->Reload();
}

void MyFrame::OnZoomIn(wxCommandEvent& evt)
{
    float zoom;
    m_browser->GetTextZoom(&zoom);
    m_browser->SetTextZoom(zoom + 0.10);
}

void MyFrame::OnZoomOut(wxCommandEvent& evt)
{
    float zoom;
    m_browser->GetTextZoom(&zoom);
    m_browser->SetTextZoom(zoom - 0.10);
}

void MyFrame::OnZoomReset(wxCommandEvent& evt)
{
    m_browser->SetTextZoom(1);
}

void MyFrame::OnShowSource(wxCommandEvent& evt)
{
    m_browser->ViewSource();
}

void MyFrame::OnShowLinks(wxCommandEvent& evt)
{
    ShowLinks();
}

void MyFrame::OnGoHelp(wxCommandEvent& evt)
{
    m_browser->OpenURI(m_uri_help);
}

void MyFrame::OnGoForums(wxCommandEvent& evt)
{
    m_browser->OpenURI(m_uri_forums);
}

void MyFrame::OnGoAbout(wxCommandEvent& evt)
{
    wxArrayString arr;

    arr.Add(wxT("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\""));
    arr.Add(wxT("\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"));
    arr.Add(wxT("<?xml version=\"1.0\" encoding=\"utf-8\"?>"));
    arr.Add(wxT("<html>"));
    arr.Add(wxT("<head>"));
    arr.Add(wxT("	<title id=\"title\">About the wxWebConnect Test Application</title>"));
    arr.Add(wxT("	<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso8859-1\" />"));
    arr.Add(wxT("	<style type=\"text/css\"><!--"));
    arr.Add(wxT("		body         { background: #eee; }"));
    arr.Add(wxT("		div#main     { width: 540px; padding: 40px 40px 20px 40px; margin: 50px auto; background: #fff; border: 1px solid #bbb; -moz-border-radius: 10px; }"));
    arr.Add(wxT("       h3, p        { padding: 0; margin: 0; font-family: Arial, Helvetica, sans-serif; clear: left }"));
    arr.Add(wxT("       h5, p        { padding: 0; margin: 0; font-family: Arial, Helvetica, sans-serif; clear: left }"));
    arr.Add(wxT("		p            { font-size: 11px; padding: 0; margin: 20px 0; line-height: 16px; }"));
    arr.Add(wxT("		p.gray       { color: #999; padding-top: 20px; margin-bottom: 0; clear: both;  }"));
    arr.Add(wxT("		p.copyright  { float: left; text-align: left; clear: left                      }"));
    arr.Add(wxT("	//--></style>"));
    arr.Add(wxT("</head>"));
    arr.Add(wxT("<body>"));
    arr.Add(wxT("	<div id=\"main\">"));
    arr.Add(wxT("		<h3>wxWebConnect Test Application<br/></h3>"));
    arr.Add(wxT("		<h5>Version 1.0<br/></h5>"));
    arr.Add(wxT("		<p class=\"copyright\">Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.<br/>"));
    arr.Add(wxT("		                       Licence:     wxWindows Library Licence, Version 3.1</p>"));
    arr.Add(wxT("		<p class=\"gray\">This software contains an unmodified binary version of the open-source XULRunner"));
    arr.Add(wxT("                         engine as provided by the Mozilla Foundation. Please read the"));
    arr.Add(wxT("                         <a href=\"http://www.mozilla.org/MPL/\">license terms and notices"));
    arr.Add(wxT("                         </a> for the XULRunner engine. This software uses XULRunner version 1.8.1.3.</p>"));
    arr.Add(wxT("	</div>"));
    arr.Add(wxT("</body>"));
    arr.Add(wxT("</html>"));

    // create a temporary file listing the links
    wxTextFile file;    
    wxString temp_file = wxFileName::CreateTempFileName(wxT("web"));
    file.Create(temp_file);

    int i, count = arr.Count();
    for (i = 0; i < count; ++i)
    {
        wxString html = arr.Item(i);
        file.AddLine(html);
    }

    file.Write();

    // open the temporary file with the links in
    // the browser
    m_browser->OpenURI(temp_file);
}

void MyFrame::OnUpdateUI(wxUpdateUIEvent& evt)
{
    int id = evt.GetId();
    switch (id)
    {
        case ID_Undo:
            evt.Enable(false);  // TODO: undo needs to be implemented
            break;
            
        case ID_Redo:
            evt.Enable(false);  // TODO: undo needs to be implemented
            break;

        case ID_Cut:
            evt.Enable(m_browser->CanCutSelection());
            break;
            
        case ID_Copy:
            evt.Enable(m_browser->CanCopySelection());
            break;

        case ID_CopyLink:
            evt.Enable(m_browser->CanCopyLinkLocation());
            break;
            
        case ID_Paste:
            evt.Enable(m_browser->CanPaste());
            break;

        case ID_FindAgain:
            evt.Enable(m_find_text.Length() > 0 ? true : false);
            break;

        case ID_ShowSource:
        case ID_ShowLinks:
            evt.Enable(m_dom_contentloaded);
            break;
    }
}

void MyFrame::OnStatusText(wxWebEvent& evt)
{
    // wxEVT_WEB_STATUSTEXT is received when somebody hovers
    // the mouse over a link and the status text should
    // be updated
    
    wxString status_text = evt.GetString();
    if (status_text.Length() == 0)
        status_text = _("Ready");

    wxStatusBar* status_bar = GetStatusBar();
    status_bar->SetStatusText(status_text);
}

void MyFrame::OnStatusChange(wxWebEvent& evt)
{
    // wxEVT_WEB_STATUSCHANGE is received when the status text
    // changes when a web page is loading
    
    wxStatusBar* status_bar = GetStatusBar();
    status_bar->SetStatusText(evt.GetString());

    // note: the status bar text is reset when
    // all the content is finished loading, in
    // OnDOMContentLoaded()
}

void MyFrame::OnStateChange(wxWebEvent& evt)
{
    // clear the status bar hear since OnStatusChange() doesn't 
    // contain an empty string and we don't want "stuck" text in 
    // the statusbar
    int state = evt.GetState();    
    wxString status_text = _("Ready");
    wxStatusBar* status_bar = GetStatusBar();

    if ((state & wxWEB_STATE_STOP) && (state & wxWEB_STATE_IS_REQUEST))
        status_bar->SetStatusText(status_text);

    if ((state & wxWEB_STATE_STOP) && (state & wxWEB_STATE_IS_REQUEST))
        status_bar->SetStatusText(status_text);
}

void MyFrame::OnLocationChange(wxWebEvent& evt)
{
    // set the url bar
    SetUrlBarValue(evt.GetString());

    // set the DOM content loaded flag to false
    // until the DOM is safely loaded
    m_dom_contentloaded = false;
}

void MyFrame::OnTitleChange(wxWebEvent& evt)
{
    SetTitle(evt.GetString());
}

void MyFrame::OnShowContextMenu(wxWebEvent& evt)
{
    wxMenu menuPopup;

    wxString href = evt.GetHref();
    if (!href.IsEmpty())
    {
        menuPopup.Append(ID_OpenHref, _("&Open"));
        menuPopup.AppendSeparator();
        m_uri_href = href;
    }
    else
    {
        menuPopup.Append(ID_GoBack, _("&Back"));
        menuPopup.Append(ID_GoForward, _("&Forward"));
        menuPopup.Append(ID_Reload, _("&Reload"));
        menuPopup.Append(ID_Stop, _("&Stop"));
        menuPopup.AppendSeparator();
    }

    menuPopup.Append(ID_Cut, _("Cu&t"));
    menuPopup.Append(ID_Copy, _("&Copy"));
    menuPopup.Append(ID_CopyLink, _("Copy &Link"));
    menuPopup.Append(ID_Paste, _("&Paste"));
    menuPopup.Append(ID_SelectAll, _("Select &All"));

    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = m_browser->ScreenToClient(pt_mouse);
    PopupMenu(&menuPopup, pt_mouse);
}

void MyFrame::OnInitDownload(wxWebEvent& evt)
{
    // TODO: add download status indicator and option
    // to open content in the browser

    // note: this handler gets called when content is
    // available to download; the content can be handled
    // as follows:
    //   evt.SetDownloadAction(wxWEB_DOWNLOAD_OPEN);   // open content
    //   evt.SetDownloadAction(wxWEB_DOWNLOAD_SAVEAS); // save content
    //   evt.SetDownloadAction(wxWEB_DOWNLOAD_CANCEL); // cancel the operation

    // here, we'll allow the user to download it or cancel
    // the operation

    // get the filename
    wxString filename = evt.GetFilename();
    wxMessageDialog dlg(this,
                        wxString::Format(wxT("Would you like to download %s?"), filename),
                        wxT("Download File"),
                        wxYES_NO);

    int result = dlg.ShowModal();

    switch (result)
    {
        case wxID_YES:
            {
                wxString filter;
                filter += _("All Files");
                filter += wxT(" (*.*)|*.*");

                wxFileDialog dlg(this,
                         _("Save As"),
                         wxT(""),
                         filename,
                         filter,
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

                if (dlg.ShowModal() != wxID_OK)
                {
                    evt.SetDownloadAction(wxWEB_DOWNLOAD_CANCEL);
                    return;
                }

                wxWebProgressBase* listener = new wxWebProgressBase;
                evt.SetDownloadAction(wxWEB_DOWNLOAD_SAVEAS);
                evt.SetDownloadTarget(dlg.GetPath());
                evt.SetDownloadListener(listener);
            }
            break;

        case wxID_NO:
            evt.SetDownloadAction(wxWEB_DOWNLOAD_CANCEL);
            break;
    }
}

void MyFrame::OnShouldHandleContent(wxWebEvent& evt)
{
    // note: this handler gets called when a content
    // type needs to be handled and allows us to determine
    // whether or not we want to handle the content type
    // or let the browser handle it

    // for example, by default, we'll let the browser handle
    // all content types, except XML

    // get the content type
    wxString input_content_type = evt.GetContentType();
    if (input_content_type == wxT("application/xml"))
    {
        // we have an XML content; don't let the browser
        // handle this, since we'll do it ourselves
        evt.SetShouldHandle(false);
        
        // TODO: custom XML handling
        return;
    }

    evt.Skip();
}

void MyFrame::OnDOMContentLoaded(wxWebEvent& evt)
{
    m_dom_contentloaded = true;
}

void MyFrame::SetUrlBarValue(const wxString& str)
{
    // don't show 'about::blank'
    if (str.CmpNoCase(wxT("about:blank")) == 0)
        return;

    // set the value
    m_urlbar->SetValue(str);
    
    // save the value in the combo list values
    m_urlbar->AppendString(str);
}

void MyFrame::ShowLinks()
{
    // make sure the DOM content is loaded
    if (!m_dom_contentloaded)
        return;

    // get the DOM Document
    wxArrayString arr;
    wxDOMDocument doc = m_browser->GetDOMDocument();

    // get the child links
    GetChildLinks(doc, arr);

    // create a temporary file listing the links
    wxTextFile file;    
    wxString temp_file = wxFileName::CreateTempFileName(wxT("web"));
    file.Create(temp_file);

    int i, count = arr.Count();
    for (i = 0; i < count; ++i)
    {
        wxString href = arr.Item(i);
        file.AddLine(href);
    }

    file.Write();

    // open the temporary file with the links in
    // the browser
    m_browser->OpenURI(temp_file);
}

void MyFrame::GetChildLinks(wxDOMNode node, wxArrayString& arr)
{
    if (!node.IsOk())
        return;

    wxDOMNodeList nodelist = node.GetChildNodes();

    int i, count = nodelist.GetLength();
    for (i = 0; i < count; ++i)
    {
        wxDOMNode node = nodelist.Item(i);
        wxDOMHTMLAnchorElement anchor = node;

        if (anchor.IsOk())
        {
            // get the anchor and create a list of urls similar to
            // <a href="http://www.kirix.com">http://www.kirix.com</a><br>
            // so we can see both the href as well as click on it

            wxString href = anchor.GetHref();
            wxString link = wxT("<a href=\"") + href + wxT("\">") + href + wxT("</a><br>");
            arr.Add(link);
        }

        GetChildLinks(node, arr);
    }
}

