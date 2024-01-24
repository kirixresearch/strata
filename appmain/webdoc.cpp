/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; Aaron L. Williams
 * Created:  2006-09-22
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "webdoc.h"
#include "toolbars.h"
#include "tabledoc.h"
#include "textdoc.h"
#include "extensionmgr.h"
#include "feedparser.h"
#include "dlgpagesetup.h"
#include "dbdoc.h"
#include "bookmarkfs.h"
#include "linkbar.h"
#include "panelfind.h"
#include "dlgcustomprompt.h"
#include <wx/webview.h>
#include <wx/fs_inet.h>
#include <wx/stdpaths.h>
#include <wx/artprov.h>
#include <kl/regex.h>
#include <kl/md5.h>
#include <kl/thread.h>


const int wxID_WEB = 9001;
const int wxID_WEBVIEW = 9002;

const int ID_Open = 18001;
const int ID_OpenNewTab = 18002;
const int ID_RotateLeft = 18004;
const int ID_RotateRight = 18005;
const int ID_ViewPageSource = 18006;
const int ID_ViewWebPage = 18007;
const int ID_SaveLinkAs = 18008;
const int ID_SavePageAs = 18009;

const float ZOOM_CHANGE_PERC = 0.10f;
const float ZOOM_DEFAULT = 1.0f;








class OpenOrSaveDlg : public wxDialog
{
    
public:

    OpenOrSaveDlg(wxWindow* parent, const wxString& filename)
                         : wxDialog(parent,
                                    -1,
                                    _("File Download"),
                                    wxDefaultPosition,
                                    wxSize(360, 175),
                                    wxDEFAULT_DIALOG_STYLE |
                                    wxCENTER)
    {
        wxStaticText* label_message = new wxStaticText(this, -1, _("Would you like to open or save this file?"));
        
        // create filename text
        
        wxString fn = _("Filename:");
        fn += wxT(" ");
        fn += filename;
        wxStaticText* label_filename = new wxStaticText(this, -1, fn);
        
        // create a platform standards-compliant OK/Cancel sizer
        wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
        ok_cancel_sizer->SetAffirmativeButton(new wxButton(this, wxID_OPEN));
        ok_cancel_sizer->SetNegativeButton(new wxButton(this, wxID_SAVE));
        ok_cancel_sizer->SetCancelButton(new wxButton(this, wxID_CANCEL));
        ok_cancel_sizer->Realize();
        ok_cancel_sizer->AddSpacer(5);
        
        // this code is necessary to get the OK/Cancel sizer's bottom margin to 8
        wxSize min_size = ok_cancel_sizer->GetMinSize();
        min_size.SetHeight(min_size.GetHeight()+16);
        ok_cancel_sizer->SetMinSize(min_size);

        // create username/password sizer
        
        wxBitmap bmp = wxArtProvider::GetBitmap(wxART_QUESTION, wxART_MESSAGE_BOX);
        wxStaticBitmap* bitmap_question = new wxStaticBitmap(this, -1, bmp);
        
        wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
        vert_sizer->Add(label_message, 0, wxEXPAND);
        vert_sizer->AddSpacer(16);
        vert_sizer->Add(label_filename, 0, wxEXPAND);
        
        // create top sizer
        
        wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
        top_sizer->AddSpacer(7);
        top_sizer->Add(bitmap_question, 0, wxTOP, 7);
        top_sizer->AddSpacer(15);
        top_sizer->Add(vert_sizer, 1, wxEXPAND | wxTOP, 7);
        
        // create main sizer
        
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->AddSpacer(8);
        main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
        main_sizer->AddStretchSpacer();
        main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
                
        SetSizer(main_sizer);
        Layout();
    }
    
    ~OpenOrSaveDlg()
    {
    }

private:

    // event handlers
    
    void OnOpen(wxCommandEvent& evt)
    {
        EndModal(wxID_OPEN);
    }
    
    void OnSave(wxCommandEvent& evt)
    {
        EndModal(wxID_SAVE);
    }
    
    void OnCancel(wxCommandEvent& evt)
    {
        EndModal(wxID_CANCEL);
    }
    
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(OpenOrSaveDlg, wxDialog)
    EVT_BUTTON(wxID_OPEN, OpenOrSaveDlg::OnOpen)
    EVT_BUTTON(wxID_SAVE, OpenOrSaveDlg::OnSave)
    EVT_BUTTON(wxID_CANCEL, OpenOrSaveDlg::OnCancel)
END_EVENT_TABLE()





BEGIN_EVENT_TABLE(WebDoc, wxWindow)
    EVT_MENU(ID_Edit_Cut, WebDoc::onCutSelection)
    EVT_MENU(ID_Edit_Copy, WebDoc::onCopySelection)
    EVT_MENU(ID_Edit_CopyLink, WebDoc::onCopyLinkLocation)
    EVT_MENU(ID_Edit_Paste, WebDoc::onPaste)
    EVT_MENU(ID_Edit_SelectAll, WebDoc::onSelectAll)
    EVT_MENU(ID_Edit_Delete, WebDoc::onDelete)
    EVT_MENU(ID_File_Back, WebDoc::onGoBackward)
    EVT_MENU(ID_File_Forward, WebDoc::onGoForward)
    EVT_MENU(ID_File_Reload, WebDoc::onReload)
    EVT_MENU(ID_File_Stop, WebDoc::onStop)
    EVT_MENU(ID_File_SaveAsExternal, WebDoc::onSaveAsExternal)
    EVT_MENU(ID_File_Print, WebDoc::onPrint)
    EVT_MENU(ID_File_PageSetup, WebDoc::onPageSetup)
    EVT_MENU(ID_View_ZoomIn, WebDoc::onZoomIn)
    EVT_MENU(ID_View_ZoomOut, WebDoc::onZoomOut)
    EVT_MENU(ID_View_ZoomToActual, WebDoc::onZoomToActual)
    EVT_MENU(ID_View_ZoomToWidth, WebDoc::onZoomToActual)
    EVT_MENU(ID_View_ZoomToFit, WebDoc::onZoomToActual)
    EVT_SIZE(WebDoc::onSize)

    // for wxWebView alternative component
    EVT_WEBVIEW_LOADED(wxID_WEBVIEW, WebDoc::onWebViewDocumentLoaded)
    EVT_WEBVIEW_TITLE_CHANGED(wxID_WEBVIEW, WebDoc::onWebViewTitleChanged)
    EVT_WEBVIEW_NAVIGATING(wxID_WEBVIEW, WebDoc::onWebViewNavigating)

    // disable the data items
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, WebDoc::onUpdateUI_DisableAlways)

    // disable canvas object items
    EVT_UPDATE_UI_RANGE(ID_Canvas_First, ID_Canvas_Last, WebDoc::onUpdateUI_DisableAlways)
    
    // disable format items
    EVT_UPDATE_UI_RANGE(ID_Format_First, ID_Format_Last, WebDoc::onUpdateUI_DisableAlways)

    // disable table items
    EVT_UPDATE_UI_RANGE(ID_Table_First, ID_Table_Last, WebDoc::onUpdateUI_DisableAlways)

    // disable the scale-related zoom
    EVT_UPDATE_UI(ID_View_ZoomCombo, WebDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToFit, WebDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToWidth, WebDoc::onUpdateUI_DisableAlways) 

    // disable some of the file items
    EVT_UPDATE_UI(ID_File_Save, WebDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_SaveAs, WebDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Run, WebDoc::onUpdateUI_DisableAlways)

    // enable/disable some of the file items based on conditions
    EVT_UPDATE_UI(ID_File_Reload, WebDoc::onUpdateUI)
    EVT_UPDATE_UI(ID_File_Stop, WebDoc::onUpdateUI)
    
    // enable/disable these item based on if we're already viewing the source
    EVT_UPDATE_UI(ID_ViewPageSource, WebDoc::onUpdateUI)

    // enable/disable the edit menu based on conditions
    EVT_UPDATE_UI_RANGE(ID_Edit_First, ID_Edit_Last, WebDoc::onUpdateUI)

END_EVENT_TABLE()





WebDoc::WebDoc(bool use_wxwebview)
{
    m_webcontrol = NULL;
    m_webview = NULL;
    m_web = NULL;
    m_use_wxwebview = true;// use_wxwebview;
    m_site_id = 0;
}

WebDoc::~WebDoc()
{
}

bool WebDoc::downloadFile(const wxString& url,
                          const wxString& filename,
                          jobs::IJobInfoPtr job_info)
{
    return false;

    //DownloadProgressListener* listener = new DownloadProgressListener(job_info);
    //return wxWebControl::SaveRequest(url, filename, NULL, listener);
}



bool WebDoc::initDoc(IFramePtr frame,
                     IDocumentSitePtr doc_site,
                     wxWindow* docsite_wnd,
                     wxWindow* panesite_wnd)
{
    // create document window
    bool result = Create(docsite_wnd,
                         doc_site,
                         wxPoint(0,0),
                         docsite_wnd->GetClientSize(),
                         wxNO_BORDER |
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxCLIP_CHILDREN);

    if (!result)
    {
        return false;
    }


    // force cfw ChildFrame and auimdi windows to not want tabs
    docsite_wnd->SetWindowStyle(docsite_wnd->GetWindowStyle() & ~wxTAB_TRAVERSAL);
    docsite_wnd->GetParent()->SetWindowStyle(docsite_wnd->GetParent()->GetWindowStyle() & ~wxTAB_TRAVERSAL);


    m_frame = frame;
    m_site_id = doc_site->getId();

    frame->sigFrameEvent().connect(this, &WebDoc::onFrameEvent);

    // set the caption
    doc_site->setCaption(_("(Untitled)"));
    doc_site->setBitmap(GETBMPSMALL(gf_document));
    m_doc_site = doc_site;
    
    m_bitmap_updater.setDocSite(m_doc_site);
    
    m_webview = wxWebView::New(this, wxID_WEBVIEW, wxWebViewDefaultURLStr, wxPoint(0,0), docsite_wnd->GetClientSize(), wxWebViewBackendDefault, wxBORDER_NONE);
    m_web = m_webview;

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_web, 1, wxEXPAND);
    SetSizer(main_sizer);
    Layout();
    
    return true;
}

wxString WebDoc::getDocumentTitle()
{
    return m_title;
}

wxString WebDoc::getDocumentLocation()
{
    return m_url;
}

wxWindow* WebDoc::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void WebDoc::setDocumentFocus()
{
    m_web->SetFocus();
}

static void clearStatusBarTextItem()
{
    // clear out statusbar text
    IStatusBarPtr statusbar = g_app->getMainFrame()->getStatusBar();
    IStatusBarItemPtr item = statusbar->getItem(wxT("app_statusbar_text"));
    if (item.isNull())
        return;

    // doing the following will ensure that the item's dirty flag
    // is set -- if we don't do this, there is a chance that the
    // item will not be refreshed because it is not dirty
    wxString val = item->getValue();
    item->setValue(val + wxT("dirty"));
    item->setValue(wxEmptyString);

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}

bool WebDoc::onSiteClosing(bool force)
{
    clearStatusBarTextItem();
    return true;
}

bool WebDoc::isWebBrowserOk() const
{
    return m_webview ? true : false;
}

bool WebDoc::findNextMatch(const wxString& text,
                           bool forward,
                           bool match_case,
                           bool whole)
{
    if (m_webview)
    {
        return false;
    }
     else
    {
        return false;
    }
}

bool WebDoc::findReplaceWith(const wxString& find_val,
                             const wxString& replace_val,
                             bool forward,
                             bool match_case,
                             bool whole)
{
    if (!findIsReplaceAllowed())
        return false;
    
    return false;
}

bool WebDoc::findReplaceAll(const wxString& find_val,
                            const wxString& replace_val,
                            bool match_case,
                            bool whole)
{
    if (!findIsReplaceAllowed())
        return false;
    
    return false;
}

bool WebDoc::findIsReplaceAllowed()
{
    return false;
}



void WebDoc::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_QUERY_AVAILABLE_VIEW)
    {
        // make sure we are the same site
        if (g_app->getMainFrame()->getActiveChild() != m_doc_site)
            return;

        evt.name = wxT("");
        
        // populate the view switcher list with the available views
        ViewSwitcherList* list = (ViewSwitcherList*)(evt.o_param);
        
        bool is_source = isViewingSource();

        list->addItem(ID_View_SwitchToSourceView, _("Source View"),
                      is_source ? true : false);
        
        list->addItem(ID_View_SwitchToLayoutView, _("Web Page View"),
                      is_source ? false : true);
    }
     else if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGED)
    {
        int id = (int)(evt.l_param);
        
        // make sure we are the same site
        if (g_app->getMainFrame()->getActiveChild() != m_doc_site)
            return;
        
        // make the event only fire once
        evt.name = wxT("");
        
        switch (id)
        {
            case ID_View_SwitchToLayoutView:
                switchToWeb();
                break;
                
            case ID_View_SwitchToSourceView:
                switchToSource();
                break;
        }
    }
}


bool WebDoc::isViewingSource() const
{
    return (m_url.Left(12) == wxT("view-source:") ? true : false);
}

void WebDoc::switchToWeb()
{
    IDocumentSitePtr other_site;
    other_site = lookupOtherDocumentSite(
                            m_doc_site,
                            "appmain.WebDoc");
                            
    if (other_site.isNull() || other_site == m_doc_site)
    {
        wxString regular_url = m_url;
        if (isViewingSource())
            regular_url.Remove(0, 12);

        WebDoc* doc = new WebDoc;
        g_app->getMainFrame()->createSite(m_doc_site->getContainerWindow(),
                                          doc,
                                          true);
        doc->openURI(regular_url);
    }
     else
    {
        switchToOtherDocument(m_doc_site, "appmain.WebDoc");
    }
}

void WebDoc::switchToSource()
{
/*
    // if we're already viewing the source, do nothing
    if (isViewingSource())
        return;
        
    wxString source_url = wxT("view-source:") + m_url;
    
    IDocumentSitePtr other_site;
    other_site = lookupOtherDocumentSite(
                            m_doc_site,
                            "appmain.WebDoc");
                            
    if (other_site.isNull() || other_site == m_doc_site)
    {
        WebDoc* doc = new WebDoc;

        g_app->getMainFrame()->createSite(m_doc_site->getContainerWindow(),
                                          doc,
                                          true);
                                          
        //doc->openURI(source_url);
        doc->m_webcontrol->ViewSource(m_webcontrol);
    }
     else
    {
        if (other_site.isOk())
        {
            IWebDocPtr web_doc = other_site->getDocument();
            if (web_doc)
            {
                //web_doc->openURI(source_url);
                web_doc->getWebControl()->ViewSource(m_webcontrol);
                
            }
            
            switchToOtherDocument(m_doc_site, "appmain.WebDoc");
        }
    }

    */
}

void WebDoc::savePageAsExternal()
{
    if (!m_webcontrol)
        return;

    wxString filter;

    // NOTE: if you add or remove items from this
    // list, make sure you adjust the case statement
    // below, because it affects which file type the
    // target will be
    filter += _("All Files");
    filter += wxT(" (*.*)|*.*|");  // some html pages simply contain images
    filter += _("HTML Files");
    filter += wxT(" (*.html, *.htm)|*.html;*.htm|");
    filter.RemoveLast(); // get rid of the last pipe sign

    wxString filename = getFilenameFromPath(m_url, true);   // include extension
    wxFileDialog dlg(g_app->getMainWindow(),
                     _("Save As"),
                     wxT(""),
                     filename,
                     filter,
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() != wxID_OK)
        return;

    //m_webcontrol->SaveCurrent(dlg.GetPath());
}

// web doc api

void WebDoc::openURI(const wxString& uri, wxWebPostData* post_data)
{
    if (m_webview)
    {
        m_url = uri;

        if (m_frame)
        {
            IDocumentSitePtr doc_site = m_frame->getActiveChild();
            if (doc_site.isOk() && doc_site == m_doc_site)
            {
                m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));
            }
        }

        m_webview->LoadURL(uri);
        return;
    }
}

wxWebControl* WebDoc::getWebControl()
{
    return m_webcontrol;
}


// web doc events

void WebDoc::onSaveAsExternal(wxCommandEvent& evt)
{
    savePageAsExternal();
}

void WebDoc::onPrint(wxCommandEvent& evt)
{
     m_webview->Print();
}

void WebDoc::onPageSetup(wxCommandEvent& evt)
{
    // get the current page settings
    double page_width = 0;
    double page_height = 0;
    double left_margin = 0;
    double right_margin = 0;
    double top_margin = 0;
    double bottom_margin = 0;
    int orientation = wxPORTRAIT;

   // m_webcontrol->GetPageSettings(&orientation, &page_width, &page_height,
   //                               &left_margin, &right_margin, &top_margin, &bottom_margin);

    PageSetupDialog dlg(this);
    dlg.setPageDimensions(page_width, page_height);
    dlg.setOrientation(orientation);
    dlg.setMargins(left_margin,
                   right_margin,
                   top_margin,
                   bottom_margin);

    // set the new page settings
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.getPageDimensions(&page_width, &page_height);
        orientation = dlg.getOrientation();
        dlg.getMargins(&left_margin,
                       &right_margin,
                       &top_margin,
                       &bottom_margin);

   //     m_webcontrol->SetPageSettings(orientation,
   //                                   page_width, page_height,
   //                                   left_margin, right_margin, top_margin, bottom_margin);
    }

    setDocumentFocus();
}

void WebDoc::onCutSelection(wxCommandEvent& evt)
{
    if (m_webview)
    {
        m_webview->Cut();
    }
}

void WebDoc::onCopySelection(wxCommandEvent& evt)
{
    if (m_webview)
    {
        m_webview->Copy();
    }
}

void WebDoc::onCopyLinkLocation(wxCommandEvent& evt)
{
}

void WebDoc::onPaste(wxCommandEvent& evt)
{
    if (m_webview)
    {
        m_webview->Paste();
    }
}

void WebDoc::onDelete(wxCommandEvent& evt)
{
    if (m_webview)
    {
        m_webview->DeleteSelection();
    }
}

void WebDoc::onSelectAll(wxCommandEvent& evt)
{
    if (m_webview)
    {
        m_webview->SelectAll();
    }
}

void WebDoc::onGoBackward(wxCommandEvent& evt)
{
    if (m_webview)
    {
        m_webview->GoBack();
    }
}

void WebDoc::onGoForward(wxCommandEvent& evt)
{
    if (m_webview)
    {
        m_webview->GoForward();
    }
}

void WebDoc::onReload(wxCommandEvent& evt)
{
    if (m_webview)
    {
        m_webview->Reload();
    }
}

void WebDoc::onStop(wxCommandEvent& evt)
{
    if (m_webview)
    {
        m_webview->Stop();
    }
}

void WebDoc::onZoomIn(wxCommandEvent& evt)
{
    if (m_webview)
    {
        switch (m_webview->GetZoom())
        {
            case wxWEBVIEW_ZOOM_TINY:      m_webview->SetZoom(wxWEBVIEW_ZOOM_SMALL);   break;
            case wxWEBVIEW_ZOOM_SMALL:     m_webview->SetZoom(wxWEBVIEW_ZOOM_MEDIUM);  break;
            default:
            case wxWEBVIEW_ZOOM_MEDIUM:    m_webview->SetZoom(wxWEBVIEW_ZOOM_LARGE);   break;
            case wxWEBVIEW_ZOOM_LARGE:	   m_webview->SetZoom(wxWEBVIEW_ZOOM_LARGEST); break;
            case wxWEBVIEW_ZOOM_LARGEST:   break;
        }
    }
}

void WebDoc::onZoomOut(wxCommandEvent& evt)
{
    if (m_webview)
    {
        switch (m_webview->GetZoom())
        {
            case wxWEBVIEW_ZOOM_TINY:      break;
            case wxWEBVIEW_ZOOM_SMALL:     m_webview->SetZoom(wxWEBVIEW_ZOOM_TINY);  break;
            default:
            case wxWEBVIEW_ZOOM_MEDIUM:    m_webview->SetZoom(wxWEBVIEW_ZOOM_SMALL);  break;
            case wxWEBVIEW_ZOOM_LARGE:	   m_webview->SetZoom(wxWEBVIEW_ZOOM_MEDIUM); break;
            case wxWEBVIEW_ZOOM_LARGEST:   m_webview->SetZoom(wxWEBVIEW_ZOOM_LARGE);  break;
        }
    }
}

void WebDoc::onZoomToActual(wxCommandEvent& evt)
{
    if (m_webview)
    {
        m_webview->SetZoom(wxWEBVIEW_ZOOM_MEDIUM);
    }
}

void WebDoc::onSize(wxSizeEvent& evt)
{
    Layout();
}


class WebDocLoadDeferred : public wxEvtHandler
{
public:
    
    WebDocLoadDeferred(WebDoc* doc, const wxString& url)
    {
        m_webdoc = doc;
        m_url = url;
        
        wxCommandEvent cmd(wxEVT_COMMAND_MENU_SELECTED, 10000);
        AddPendingEvent(cmd);
    }
    
    bool ProcessEvent(wxEvent& evt)
    {
        m_webdoc->openURI(m_url);
        wxPendingDelete.Append(this);
        return true;
    }
    
    WebDoc* m_webdoc;
    wxString m_url;
};



void WebDoc::showWebRes(int webres_id)
{
    wxString path = g_app->getInstallPath();
    path = filenameToUrl(path);
    if (path.IsEmpty() || path[path.Length()-1] != wxT('/'))
        path += wxT('/');
    path += wxT("webres.jar!/");
    
    switch (webres_id)
    {
        default:
        case webresAbout:          path += wxT("about.html");            break;
        case webresLicense:        path += wxT("license.html");          break;
        case webresDownloading:    path += wxT("download.html");         break;
        case webresServerNotFound: path += wxT("server_not_found.html"); break;
    }
    
    path.Prepend(wxT("jar:"));
    //openURI(path);
    
    WebDocLoadDeferred* l = new WebDocLoadDeferred(this, path);
}

wxImage WebDoc::getFavIcon()
{
    return m_favicon;
}



static wxString extensionFromMimeType(const wxString& _mime_type)
{
    wxString mime_type = _mime_type;
    mime_type.MakeLower();
    
    if (mime_type == wxT("application/excel") ||
        mime_type == wxT("application/vnd.ms-excel") ||
        mime_type == wxT("application/vnd.msexcel") ||
        mime_type == wxT("application/x-excel") ||
        mime_type == wxT("application/x-msexcel"))
    {
        return wxT("xls");
    }
    
    if (mime_type == wxT("application/vnd.interchange-csv"))
    {
        return wxT("icsv");
    }
    
    if (mime_type == wxT("text/comma-separated-values") ||
        mime_type == wxT("text/csv") ||
        mime_type == wxT("application/csv"))
    {
        return wxT("csv");
    }
    
    if (mime_type == wxT("text/tab-separated-values") ||
        mime_type == wxT("text/tsv") ||
        mime_type == wxT("application/tsv"))
    {
        return wxT("tsv");
    }
    
    if (mime_type == wxT("application/dbase") ||
        mime_type == wxT("application/x-dbase") ||
        mime_type == wxT("application/dbf") ||
        mime_type == wxT("application/x-dbf") ||
        mime_type == wxT("zz-application/zz-winassoc-dbf"))
    {
        return wxT("dbf");
    }
    
    return wxEmptyString;
}

void WebDoc::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void WebDoc::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}

void WebDoc::onUpdateUI(wxUpdateUIEvent& evt)
{
    return;

    int id = evt.GetId();
    switch (id)
    {
        case ID_Edit_Undo:
            evt.Enable(false);
            break;
            
        case ID_Edit_Redo:
            evt.Enable(false);
            break;

        case ID_Edit_Delete:
        case ID_Edit_Cut:
            if (m_webview)
                evt.Enable(m_webview->CanCut());
            else
                evt.Enable(false);
            break;
            
        case ID_Edit_Copy:
            if (m_webview)
                evt.Enable(m_webview->CanCopy());
            else
                evt.Enable(false);
            break;
            
        case ID_Edit_CopyLink:
            evt.Enable(false);
            break;
            
        case ID_Edit_Paste:
            if (m_webview)
                evt.Enable(m_webview->CanPaste());
            else
                evt.Enable(false);
            break;
            
        case ID_Edit_SelectAll:
            evt.Enable(m_webview ? true : false);
            break;
            
        case ID_Edit_SelectNone:
            evt.Enable(m_webview ? true : false);
            break;
            
        case ID_Edit_Find:
            evt.Enable(m_webview ? true : false);
            break;
            
        case ID_Edit_FindPrev:
        case ID_Edit_FindNext:
        {
            wxString find_filter_text = g_app->getAppController()->
                                               getStandardToolbar()->
                                               getFindCombo()->
                                               GetValue();
            evt.Enable((find_filter_text.Length() > 0) ? true : false);
            break;
        }
        
        case ID_Edit_Replace:
            evt.Enable(false);
            break;

        case ID_Edit_GoTo:
            evt.Enable(false);
            break;

        case ID_File_Reload:
            evt.Enable(m_webview ? true : false);
            break;

        case ID_ViewPageSource:
            evt.Enable(isViewingSource() ? false : true);
            break;
            
        // TODO: enable only if site loading
        case ID_File_Stop:
            evt.Enable(m_webview ? true : false);
            break;
      
        default:
            evt.Enable(false);
    }
}



void WebDoc::onWebViewTitleChanged(wxWebViewEvent& evt)
{
    wxString title = evt.GetString();

    if (title.IsEmpty())
        title = _("(Untitled)");
    
    IDocumentSitePtr doc_site = m_frame->lookupSiteById(m_site_id);
    if (doc_site.isOk())
        doc_site->setCaption(title);
    
    m_title = title;
    
    // make sure the application's titlebar is updated
    g_app->getAppController()->updateTitle();
}




void WebDoc::onWebViewDocumentLoaded(wxWebViewEvent& evt)
{
    m_url = m_webview->GetCurrentURL();

    if (m_frame)
    {
        IDocumentSitePtr doc_site = m_frame->getActiveChild();
        if (doc_site.isOk() && doc_site == m_doc_site)
        {
            m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));
        }
    }
}


void WebDoc::onWebViewNavigating(wxWebViewEvent& evt)
{

}
