/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2012-08-17
 *
 */


#include "appmain.h"
#include "dlgshareview.h"

enum
{
    ID_Share = wxID_HIGHEST + 1,
    ID_Url_Text,
    ID_Copy_Url
};


BEGIN_EVENT_TABLE(DlgShareView, wxDialog)
    EVT_BUTTON(wxID_OK, DlgShareView::onOK)
    EVT_BUTTON(ID_Share, DlgShareView::onShare)
    EVT_BUTTON(ID_Copy_Url, DlgShareView::onCopy)
END_EVENT_TABLE()


DlgShareView::DlgShareView(wxWindow* parent)
                    : wxDialog(parent, -1, _("Share"),
                         wxDefaultPosition,
                         wxSize(500,280),
                         wxDEFAULT_DIALOG_STYLE |
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxCLIP_CHILDREN |
                         wxCENTER |
                         wxRESIZE_BORDER)
{
    SetMinSize(wxSize(500,280));
    SetMaxSize(wxSize(640,480));
    
    // create vertical sizer
    
    wxString appname = APPLICATION_NAME;
    wxString email = APP_CONTACT_SALESEMAIL;
    wxString str;

    str = wxString::Format(_("To share your current view, click the button to the Share button.  Paste the generated link in the location bar to load the view again."));

    wxStaticText* message = new wxStaticText(this, -1, str);
    


    m_share_button = new kcl::Button(this, ID_Share, wxDefaultPosition, wxSize(100,75),
                        _("Share"), GETBMP(gf_globe_32), GETBMP(gf_globe_32));

    m_share_button->setBitmapLocation(kcl::Button::bitmapTop);
    m_share_button->setTextVisible(true);
    m_share_button->setAlignment(kcl::Button::alignCenter);
    m_share_button->setMode(kcl::Button::modeCompatible);
    m_share_button->setXOffset(12);



    // create top sizer
    
    wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
    top_sizer->AddSpacer(5);
    top_sizer->Add(m_share_button, 0, wxTOP, 5);
    top_sizer->AddSpacer(13);
    top_sizer->Add(message, 1, wxEXPAND | wxTOP, 7);
    

    // create url box
    
    m_url_text = new wxTextCtrl(this, ID_Url_Text, wxEmptyString, wxDefaultPosition, wxSize(100, 40), wxTE_CENTRE);
    m_url_text->SetFont(wxFont(14, wxSWISS, wxNORMAL, wxFONTWEIGHT_BOLD, false));


    wxBoxSizer* url_sizer = new wxBoxSizer(wxVERTICAL);
    url_sizer->Add(m_url_text, 0, wxEXPAND, 0);

    
    // create a platform standards-compliant OK/Cancel sizer
    
    m_copy_button = new wxButton(this, ID_Copy_Url, _("Copy"));
    m_copy_button->Disable();

    m_ok_button = new wxButton(this, wxID_OK, _("Close"));

    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->Add(m_copy_button, 0, wxEXPAND, 0);
    button_sizer->AddStretchSpacer(1);
    button_sizer->Add(m_ok_button, 0, wxEXPAND, 0);
    

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddStretchSpacer(2);
    main_sizer->Add(url_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddStretchSpacer(1);
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddSpacer(8);
    SetSizer(main_sizer);
    Layout();
    
    cfw::resizeStaticText(message, message->GetClientSize().GetWidth());
    Center();
    
    // set the ok button as the default
    m_ok_button->SetDefault();
    m_ok_button->SetFocus();
}

DlgShareView::~DlgShareView()
{

}


void DlgShareView::onShare(wxCommandEvent& evt)
{
    wxString url;
    sigRequestShareUrl(url);

    m_url_text->SetValue(url);
    m_share_button->Disable();
    m_copy_button->Enable();
    m_ok_button->SetFocus();
}

void DlgShareView::onCopy(wxCommandEvent& evt)
{
    m_url_text->SelectAll();
    m_url_text->Copy();
    m_copy_button->Disable();
    m_ok_button->SetFocus();
}

void DlgShareView::onOK(wxCommandEvent& evt)
{
    EndModal(wxID_OK);
}
