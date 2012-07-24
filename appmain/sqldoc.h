/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2008-02-28
 *
 */


#ifndef __APP_SQLDOC_H
#define __APP_SQLDOC_H


xcm_interface ISqlDoc : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.ISqlDoc")
    
    virtual void setText(const wxString& text) = 0;
};

XCM_DECLARE_SMARTPTR(ISqlDoc)


class SqlTextCtrl;
class SqlDoc : public wxWindow,
               public cfw::IDocument,
               public ISqlDoc,
               public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.SqlDoc")
    XCM_BEGIN_INTERFACE_MAP(SqlDoc)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
        XCM_INTERFACE_ENTRY(ISqlDoc)
    XCM_END_INTERFACE_MAP()

public:

    SqlDoc();
    virtual ~SqlDoc();

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    wxString getDocumentTitle();
    wxString getDocumentLocation();
    void setDocumentFocus();
    bool onSiteClosing(bool force);
    void onSiteActivated();
    
    // -- ISqlDoc --
    void setText(const wxString& text);

private:

    // -- frame event handlers --
    void onFrameEvent(cfw::Event& evt);
    
    // -- wx event handlers --
    void onSize(wxSizeEvent& evt);
    void onCopy(wxCommandEvent& evt);
    void onSelectAll(wxCommandEvent& evt);
    void onSelectNone(wxCommandEvent& evt);
    void onUpdateUI_EnableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI(wxUpdateUIEvent& evt);

private:

    SqlTextCtrl* m_textctrl;
    wxString m_sql_text;
    
    cfw::IFramePtr m_frame;
    cfw::IDocumentSitePtr m_doc_site;

    DECLARE_EVENT_TABLE()
};


#endif  // __APP_SQLDOC_


