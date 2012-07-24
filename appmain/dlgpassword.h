/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-09-29
 *
 */


#ifndef __APP_DLGPASSWORD_H
#define __APP_DLGPASSWORD_H


class DlgPassword : public wxWindow,
                       public cfw::IDocument
{

XCM_CLASS_NAME_NOREFCOUNT("appmain.DlgPassword")
XCM_BEGIN_INTERFACE_MAP(DlgPassword)
    XCM_INTERFACE_ENTRY(cfw::IDocument)
XCM_END_INTERFACE_MAP()

public:

    DlgPassword();
    ~DlgPassword();

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    wxString getUsername();
    wxString getPassword();
    
    void setMessage(const wxString& message);
    void setUsername(const wxString& username);
    
private:

    // -- event handlers --
    
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);

private:

    cfw::IDocumentSitePtr m_doc_site;
    
    wxString m_message;
    wxString m_username;
    
    wxTextCtrl* m_username_ctrl;
    wxTextCtrl* m_password_ctrl;
    
    DECLARE_EVENT_TABLE()
};

#endif

