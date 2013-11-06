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
#include <wx/artprov.h>



BEGIN_EVENT_TABLE(DlgPassword, wxDialog)
    EVT_BUTTON(wxID_OK, DlgPassword::onOK)
    EVT_BUTTON(wxID_CANCEL, DlgPassword::onCancel)
END_EVENT_TABLE()


DlgPassword::DlgPassword(wxWindow* parent) : wxDialog(parent,
                                                      -1,
                                                      _("Sign In"),
                                                      wxDefaultPosition,
                                                      wxSize(400, 230),
                                                      wxDEFAULT_DIALOG_STYLE |
                                                      wxCENTER)
{
    // create the username sizer
        
    wxStaticText* label_username = new wxStaticText(this,
                                                    -1,
                                                    _("User Name:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_username_ctrl = new wxTextCtrl(this, ID_UsernameTextCtrl, m_username);
        
    wxBoxSizer* username_sizer = new wxBoxSizer(wxHORIZONTAL);
    username_sizer->Add(label_username, 0, wxALIGN_CENTER);
    username_sizer->Add(m_username_ctrl, 1, wxEXPAND);
        
        
    // create the password sizer
        
    wxStaticText* label_password = new wxStaticText(this,
                                                    -1,
                                                    _("Password:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_password_ctrl = new wxTextCtrl(this,
                                     ID_PasswordTextCtrl,
                                     wxEmptyString,
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     wxTE_PASSWORD);
        
    wxBoxSizer* password_sizer = new wxBoxSizer(wxHORIZONTAL);
    password_sizer->Add(label_password, 0, wxALIGN_CENTER);
    password_sizer->Add(m_password_ctrl, 1, wxEXPAND);


    // create the service sizer
        
    wxStaticText* label_service = new wxStaticText(this,
                                                    -1,
                                                    _("Service:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_service_ctrl = new wxTextCtrl(this,
                                    ID_ServiceTextCtrl,
                                    wxEmptyString,
                                    wxDefaultPosition,
                                    wxDefaultSize);
        
    wxBoxSizer* service_sizer = new wxBoxSizer(wxHORIZONTAL);
    service_sizer->Add(label_service, 0, wxALIGN_CENTER);
    service_sizer->Add(m_service_ctrl, 1, wxEXPAND);



    // create a platform standards-compliant OK/Cancel sizer
        
    wxButton* ok_button = new wxButton(this, wxID_OK);
    wxButton* cancel_button = new wxButton(this, wxID_CANCEL);
        
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(ok_button);
    ok_cancel_sizer->AddButton(cancel_button);
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
        
    ok_button->SetDefault();
        
    // this code is necessary to get the sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
        
        
    // code to allow us to line up the static text elements
    wxSize s1 = label_username->GetSize();
    wxSize s2 = label_password->GetSize();
    wxSize s3 = label_service->GetSize();
    wxSize max_size = wxSize(wxMax(wxMax(s1.x, s2.x), s3.x), wxMax(wxMax(s1.y, s2.y), s3.y));
    max_size.x += 10;
    username_sizer->SetItemMinSize(label_username, max_size);
    password_sizer->SetItemMinSize(label_password, max_size);
    service_sizer->SetItemMinSize(label_service, max_size);


    // create username/password sizer
        
    wxBitmap bmp = wxArtProvider::GetBitmap(wxART_QUESTION, wxART_MESSAGE_BOX);
    wxStaticBitmap* bitmap_question = new wxStaticBitmap(this, -1, bmp);
    m_message_ctrl = new wxStaticText(this, -1, m_message);
        
    wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
    vert_sizer->Add(m_message_ctrl, 0, wxEXPAND);
    vert_sizer->AddSpacer(16);
    vert_sizer->Add(username_sizer, 0, wxEXPAND);
    vert_sizer->AddSpacer(8);
    vert_sizer->Add(password_sizer, 0, wxEXPAND);
    vert_sizer->AddSpacer(8);
    vert_sizer->Add(service_sizer, 0, wxEXPAND);
        
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

    Center();
}

DlgPassword::~DlgPassword()
{
    // clear out password in memory
    m_password = "                                      ";
}


void DlgPassword::setMessage(const wxString& message)
{
    m_message = message;
    m_message_ctrl->SetLabel(m_message);
    wxSizer* sizer = m_message_ctrl->GetContainingSizer();
    m_message_ctrl->Wrap(sizer->GetSize().GetWidth());
    Layout();
}
    
void DlgPassword::setUserName(const wxString& username)
{
    m_username = username;
    m_username_ctrl->SetValue(username);
}


void DlgPassword::setService(const wxString& service)
{
    m_service = service;
    m_service_ctrl->SetValue(service);
}


void DlgPassword::onOK(wxCommandEvent& evt)
{
    m_username = m_username_ctrl->GetValue();
    m_password = m_password_ctrl->GetValue();
    m_service = m_service_ctrl->GetValue();
        
    EndModal(wxID_OK);
}
    
void DlgPassword::onCancel(wxCommandEvent& evt)
{
    m_username = "";
    m_password = "";
    m_service = "";

    EndModal(wxID_CANCEL);
}

