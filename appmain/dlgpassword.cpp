/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-09-29
 *
 */


#include "appmain.h"
#include "dlgpassword.h"


enum
{
    ID_UsernameTextCtrl = wxID_HIGHEST + 1,
    ID_PasswordTextCtrl
};


BEGIN_EVENT_TABLE(DlgPassword, wxWindow)
    EVT_BUTTON(wxID_OK, DlgPassword::onOK)
    EVT_BUTTON(wxID_CANCEL, DlgPassword::onCancel)
END_EVENT_TABLE()


DlgPassword::DlgPassword()
{
    m_message = _("Enter username and password for this web site:");
    m_username = wxEmptyString;
}

DlgPassword::~DlgPassword()
{

}

bool DlgPassword::initDoc(IFramePtr frame,
                          IDocumentSitePtr doc_site,
                          wxWindow* docsite_wnd,
                          wxWindow* panesite_wnd)
{
    // -- create document's window --
    bool result = Create(docsite_wnd,
                         -1,
                         wxDefaultPosition,
                         wxSize(320, 124),
                         wxNO_FULL_REPAINT_ON_RESIZE);
    if (!result)
    {
        return false;
    }

    doc_site->setCaption(_("Login"));
    m_doc_site = doc_site;


    wxStaticText* label_message = new wxStaticText(this, -1, m_message);
    resizeStaticText(label_message);
    
    
    // -- create the username sizer --
    
    wxStaticText* label_username = new wxStaticText(this,
                                                    -1,
                                                    _("Username:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize,
                                                    wxALIGN_RIGHT);
    m_username_ctrl = new wxTextCtrl(this, ID_UsernameTextCtrl, m_username);
    
    wxBoxSizer* username_sizer = new wxBoxSizer(wxHORIZONTAL);
    username_sizer->Add(label_username, 0, wxALIGN_CENTER);
    username_sizer->AddSpacer(5);
    username_sizer->Add(m_username_ctrl, 1, wxEXPAND);
    
    
    // -- create the password sizer --
    
    wxStaticText* label_password = new wxStaticText(this,
                                                    -1,
                                                    _("Password:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize,
                                                    wxALIGN_RIGHT);
    m_password_ctrl = new wxTextCtrl(this, ID_PasswordTextCtrl);
    
    wxBoxSizer* password_sizer = new wxBoxSizer(wxHORIZONTAL);
    password_sizer->Add(label_password, 0, wxALIGN_CENTER);
    password_sizer->AddSpacer(5);
    password_sizer->Add(m_password_ctrl, 1, wxEXPAND);


    // -- create a platform standards-compliant OK/Cancel sizer --
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_OK));
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    
    // -- this code is necessary to get the sizer's bottom margin to 8 --
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
    
    
    // -- code to allow us to line up the static text elements --
    wxSize max_size = getMaxTextSize(label_username,
                                          label_password);
    max_size.x += 20;
    username_sizer->SetItemMinSize(label_username, max_size);
    password_sizer->SetItemMinSize(label_password, max_size);


    // create main sizer
    
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(label_message, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(16);
    main_sizer->Add(username_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(8);
    main_sizer->Add(password_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);

    SetSizer(main_sizer);
    Layout();
    
    doc_site->getContainerWindow()->SetClientSize(main_sizer->GetSize());
    return true;
}

wxWindow* DlgPassword::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void DlgPassword::setDocumentFocus()
{

}

wxString DlgPassword::getUsername()
{
    return m_username_ctrl->GetValue();
}

wxString DlgPassword::getPassword()
{
    return m_password_ctrl->GetValue();
}

void DlgPassword::setMessage(const wxString& message)
{
    m_message = message;
}

void DlgPassword::setUsername(const wxString& username)
{
    m_username = username;
}

void DlgPassword::onOK(wxCommandEvent& event)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void DlgPassword::onCancel(wxCommandEvent& event)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}



