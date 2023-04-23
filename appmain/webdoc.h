/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; Aaron L. Williams
 * Created:  2006-09-22
 *
 */


#ifndef H_APP_WEBDOC_H
#define H_APP_WEBDOC_H


#include "panelfind.h"

class wxWebControl;
class wxWebEvent;
class wxWebPostData;
class wxWebView;
class wxWebViewEvent;


xcm_interface IWebDoc : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IWebDoc")

public:
    
    virtual void openURI(const wxString& uri, wxWebPostData* post_data = NULL) = 0;
    virtual wxWebControl* getWebControl() = 0;
    virtual bool isViewingSource() const = 0;
    virtual wxImage getFavIcon() = 0;
};

XCM_DECLARE_SMARTPTR(IWebDoc)







// this little helper class allows a document site
// to have an "animated" progress icons

class ProgressBitmapUpdater : public wxTimer
{
public:

    ProgressBitmapUpdater()
    {
        m_progress_idx = 0;
        m_bitmaps[0]  = GETBMPSMALL(gf_progress1);
        m_bitmaps[1]  = GETBMPSMALL(gf_progress2);
        m_bitmaps[2]  = GETBMPSMALL(gf_progress3);
        m_bitmaps[3]  = GETBMPSMALL(gf_progress4);
        m_bitmaps[4]  = GETBMPSMALL(gf_progress5);
        m_bitmaps[5]  = GETBMPSMALL(gf_progress6);
        m_bitmaps[6]  = GETBMPSMALL(gf_progress7);
        m_bitmaps[7]  = GETBMPSMALL(gf_progress8);
        m_bitmaps[8]  = GETBMPSMALL(gf_progress9);
        m_bitmaps[9]  = GETBMPSMALL(gf_progress10);
        m_bitmaps[10] = GETBMPSMALL(gf_progress11);
        m_bitmaps[11] = GETBMPSMALL(gf_progress12);
    }
    
    void setDocSite(IDocumentSitePtr doc_site)
    {
        m_doc_site = doc_site;
    }
    
    // this function allows us to explictly specify the bitmap to switch
    // back to after the progress bitmap updater is finished running --
    // example: text delimited downloads start in a webdoc and
    // finish in a tabledoc... this avoids bitmap flicker
    void setFinishBitmap(const wxBitmap& bmp)
    {
        m_finish_bitmap = bmp;
    }

    void start()
    {
        if (m_doc_site.isNull())
            return;
        if (IsRunning())
            return;
        m_orig_bitmap = m_doc_site->getBitmap();
        Start(80);
    }
    
    void stop()
    {
        if (m_doc_site.isNull())
            return;

        if (m_finish_bitmap.IsOk())
            m_doc_site->setBitmap(m_finish_bitmap);
             else            
            m_doc_site->setBitmap(m_orig_bitmap);
        
        Stop();
    }
    

protected:

    void Notify()
    {
        if (m_doc_site.isNull())
            return;
            
        advanceProgressBitmap();
    }
    
private:

    void advanceProgressBitmap()
    {        
        // start over with the beginning progress bitmap
        if (m_progress_idx > 11)
            m_progress_idx = 0;

        m_doc_site->setBitmap(m_bitmaps[m_progress_idx]);
        m_progress_idx++;
    }
    
private:

    IDocumentSitePtr m_doc_site;
    wxBitmap m_bitmaps[12];
    wxBitmap m_orig_bitmap;
    wxBitmap m_finish_bitmap;
    int m_progress_idx;
};












class WebDoc : public wxWindow,
               public IWebDoc,
               public IFindTarget,
               public IDocument,
               public StatusBarProviderBase,
               public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.WebDoc")
    XCM_BEGIN_INTERFACE_MAP(WebDoc)
        XCM_INTERFACE_ENTRY(IDocument)
        XCM_INTERFACE_ENTRY(IWebDoc)
        XCM_INTERFACE_ENTRY(IFindTarget)
        XCM_INTERFACE_CHAIN(StatusBarProviderBase)
    XCM_END_INTERFACE_MAP()

public:
    
    enum WebResId
    {
        webresNone =           0,
        webresAbout =          1,
        webresLicense =        2,
        webresDownloading =    3,
        webresServerNotFound = 4
    };

public:

    WebDoc(bool use_wxwebview = false);
    virtual ~WebDoc();

    bool isWebBrowserOk() const;
    
    void openURI(const wxString& uri, wxWebPostData* post_data = NULL);
    wxWebControl* getWebControl();
    bool isViewingSource() const;
    wxImage getFavIcon();
    
    void showWebRes(int webres_id);
    
    static bool downloadFile(const wxString& url,
                             const wxString& filename,
                             jobs::IJobInfoPtr job_info);
    
private:

    // IDocument
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    
    wxString getDocumentTitle();
    wxString getDocumentLocation();
    wxWindow* getDocumentWindow();
    void setDocumentFocus();
    bool onSiteClosing(bool force);
    
    // IFindTarget
    bool findNextMatch(
                    const wxString& text,
                    bool forward,
                    bool match_case,
                    bool whole);
    bool findReplaceWith(
                    const wxString& find_val,
                    const wxString& replace_val,
                    bool forward,
                    bool match_case,
                    bool whole);
    bool findReplaceAll(
                    const wxString& find_val,
                    const wxString& replace_val,
                    bool match_case,
                    bool whole);
    bool findIsReplaceAllowed();
    
    // other methods
    void switchToWeb();
    void switchToSource();
    void savePageAsExternal();
    
private:
 
    // event handlers
    
    void onPrint(wxCommandEvent& evt);
    void onPageSetup(wxCommandEvent& ev);
    void onSaveAsExternal(wxCommandEvent& evt);
    void onCutSelection(wxCommandEvent& evt);
    void onCopySelection(wxCommandEvent& evt);
    void onCopyLinkLocation(wxCommandEvent& evt);
    void onPaste(wxCommandEvent& evt);
    void onDelete(wxCommandEvent& evt);
    void onSelectAll(wxCommandEvent& evt);
    void onGoBackward(wxCommandEvent& evt);
    void onGoForward(wxCommandEvent& evt);
    void onReload(wxCommandEvent& evt);
    void onStop(wxCommandEvent& evt);
    void onZoomIn(wxCommandEvent& evt);
    void onZoomOut(wxCommandEvent& evt);
    void onZoomToActual(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);
    void onStatusChange(wxWebEvent& evt);
    void onStateChange(wxWebEvent& evt);
    void onLocationChange(wxWebEvent& evt);
    void onTitleChange(wxWebEvent& evt);
    void onShowContextMenu(wxWebEvent& evt);
    void onLeftUp(wxWebEvent& evt);
    void onMiddleUp(wxWebEvent& evt);
    void onElementClick(wxWebEvent& evt);
    void onCreateBrowserRequest(wxWebEvent& evt);
    void onInitDownload(wxWebEvent& evt);
    void onOpenURI(wxWebEvent& evt);
    void onShouldHandleContent(wxWebEvent& evt);
    void onFavIconAvailable(wxWebEvent& evt);
    void onDOMContentLoaded(wxWebEvent& evt);
    void onFsDataDropped(wxWebEvent& evt);
    void onUpdateUI_EnableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI(wxUpdateUIEvent& evt);

    void onWebViewDocumentLoaded(wxWebViewEvent& evt);
    void onWebViewTitleChanged(wxWebViewEvent& evt);
    void onWebViewNavigating(wxWebViewEvent& evt);

    void onFrameEvent(FrameworkEvent& evt);

private:

    IDocumentSitePtr m_doc_site;
    wxWindow* m_web;
    wxWebControl* m_webcontrol;
    wxWebView* m_webview;
    int m_site_id;
    IFramePtr m_frame;
    wxString m_url;
    wxString m_title;
    ProgressBitmapUpdater m_bitmap_updater;
    wxImage m_favicon;
    bool m_use_wxwebview;

private:

    DECLARE_EVENT_TABLE()
};


#endif

