/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-11-06
 *
 */


#include "appmain.h"
#include "dlglinkprops.h"
#include "dbcombo.h"


enum
{
    ID_NameTextCtrl = wxID_HIGHEST+1,
    ID_LocationTextCtrl,
    ID_TagsTextCtrl,
    ID_DescriptionTextCtrl,
    ID_RunTargetCheckbox
};


// -- LinkPropsDialog class implementation --

BEGIN_EVENT_TABLE(LinkPropsDialog, wxDialog)
    EVT_TEXT(ID_NameTextCtrl, LinkPropsDialog::onNameChanged)
    EVT_TEXT(ID_LocationTextCtrl, LinkPropsDialog::onLocationChanged)
    EVT_TEXT(ID_TagsTextCtrl, LinkPropsDialog::onTagsChanged)
    EVT_TEXT(ID_DescriptionTextCtrl, LinkPropsDialog::onDescriptionChanged)
    EVT_CHECKBOX(ID_RunTargetCheckbox, LinkPropsDialog::onRunTargetChanged)
    EVT_SIZE(LinkPropsDialog::onSize)
END_EVENT_TABLE()


LinkPropsDialog::LinkPropsDialog(wxWindow* parent,
                                 const wxString& name,
                                 const wxString& location,
                                 const wxString& tags,
                                 const wxString& description) :
                            wxDialog(parent, 
                                     -1,
                                     _("Link Properties"),
                                     wxDefaultPosition,
                                     wxSize(360,280),
                                     wxDEFAULT_DIALOG_STYLE |
                                     wxRESIZE_BORDER |
                                     wxCLIP_CHILDREN |
                                     wxNO_FULL_REPAINT_ON_RESIZE)
{
    m_db_comboctrl = NULL;
    m_name_textctrl = NULL;
    m_location_textctrl = NULL;
    m_tags_textctrl = NULL;
    m_description_textctrl = NULL;
    m_runtarget_checkbox = NULL;
    
    m_name = name;
    m_location = location;
    m_tags = tags;
    m_description = description;
    m_run_target = false;
    
    m_mode = LinkPropsDialog::ModeCreate;
}

LinkPropsDialog::~LinkPropsDialog()
{

}

int LinkPropsDialog::ShowModal()
{
    // if we haven't specified a min size, do it now
    if (!GetMinSize().IsFullySpecified())
        SetMinSize(GetSize());

    // create the message sizer
    wxStaticText* label_message = new wxStaticText(this, -1, m_message);
    
    wxBoxSizer* message_sizer = new wxBoxSizer(wxHORIZONTAL);
    message_sizer->Add(label_message, 1, wxALIGN_CENTER);

    // create the separator
    wxStaticLine* separator = new wxStaticLine(this, -1);

    // create the db combo sizer
    wxStaticText* label_create = new wxStaticText(this,
                                                  -1,
                                                  _("Create in:"),
                                                  wxDefaultPosition,
                                                  wxDefaultSize);
    m_db_comboctrl = NULL;
    m_db_comboctrl = new DbComboCtrl(this);
    m_db_comboctrl->setShowFullPath(true, true);

    if (m_start_folder.Length() > 0)
    {
        wxString label = m_start_folder.AfterLast(wxT('/'));
        m_db_comboctrl->setPopupRootFolder(m_start_folder, label);
    }
     else
    {
        m_db_comboctrl->setPopupRootFolder(g_app->getBookmarkFolder(),
                                           _("Bookmarks Toolbar"));
    }
    
    wxBoxSizer* create_sizer = new wxBoxSizer(wxHORIZONTAL);
    create_sizer->Add(label_create, 0, wxALIGN_CENTER);
    create_sizer->Add(m_db_comboctrl, 1, wxALIGN_CENTER);

    // create the name sizer
    wxStaticText* label_name = new wxStaticText(this,
                                                -1,
                                                _("Name:"),
                                                wxDefaultPosition,
                                                wxDefaultSize);
    m_name_textctrl = NULL;
    m_name_textctrl = new wxTextCtrl(this,
                                     ID_NameTextCtrl,
                                     m_name);
    
    wxBoxSizer* name_sizer = new wxBoxSizer(wxHORIZONTAL);
    name_sizer->Add(label_name, 0, wxALIGN_CENTER);
    name_sizer->Add(m_name_textctrl, 1, wxALIGN_CENTER);

    if (m_mode == LinkPropsDialog::ModeRename)
        label_name->Show(false);
    
    // create the name sizer
    wxStaticText* label_location = new wxStaticText(this,
                                                    -1,
                                                    _("Location:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_location_textctrl = NULL;
    m_location_textctrl = new wxTextCtrl(this,
                                         ID_LocationTextCtrl,
                                         m_location);
    
    wxBoxSizer* location_sizer = new wxBoxSizer(wxHORIZONTAL);
    location_sizer->Add(label_location, 0, wxALIGN_CENTER);
    location_sizer->Add(m_location_textctrl, 1, wxALIGN_CENTER);

    // create the tags sizer
    wxStaticText* label_tags = new wxStaticText(this,
                                                -1,
                                                _("Tags:"),
                                                wxDefaultPosition,
                                                wxDefaultSize);
    m_tags_textctrl = NULL;
    m_tags_textctrl = new wxTextCtrl(this,
                                     ID_TagsTextCtrl,
                                     m_tags,
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     wxTE_MULTILINE | wxTE_BESTWRAP);
    m_tags_textctrl->SetMaxLength(512);
    
    wxBoxSizer* tags_sizer = new wxBoxSizer(wxHORIZONTAL);
    tags_sizer->Add(label_tags);
    tags_sizer->Add(m_tags_textctrl, 1, wxEXPAND);

    // create the description sizer
    wxStaticText* label_description = new wxStaticText(this,
                                                    -1,
                                                    _("Description:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_description_textctrl = NULL;
    m_description_textctrl = new wxTextCtrl(this,
                                            ID_DescriptionTextCtrl,
                                            m_description,
                                            wxDefaultPosition,
                                            wxDefaultSize,
                                            wxTE_MULTILINE | wxTE_BESTWRAP);
    m_description_textctrl->SetMaxLength(4096);

    
    wxBoxSizer* description_sizer = new wxBoxSizer(wxHORIZONTAL);
    description_sizer->Add(label_description);
    description_sizer->Add(m_description_textctrl, 1, wxEXPAND);
    
    
    m_runtarget_checkbox = NULL;
    m_runtarget_checkbox = new wxCheckBox(this,
                                            ID_RunTargetCheckbox,
                                            _("Run Target"),
                                            wxDefaultPosition);
    m_runtarget_checkbox->SetValue(m_run_target);

    
    wxBoxSizer* flags_sizer = new wxBoxSizer(wxHORIZONTAL);
    flags_sizer->AddSpacer(1);
    flags_sizer->Add(m_runtarget_checkbox);


    if (m_mode == LinkPropsDialog::ModeCreate ||
        m_mode == LinkPropsDialog::ModeEdit)
    {
        // measure the label widths
        wxSize label_size = cfw::getMaxTextSize(label_name,
                                                label_location,
                                                label_tags,
                                                label_description);
        label_size.x += 10;
        
        name_sizer->SetItemMinSize(label_name, label_size);
        location_sizer->SetItemMinSize(label_location, label_size);
        tags_sizer->SetItemMinSize(label_tags, label_size);
        description_sizer->SetItemMinSize(label_description, label_size);
        flags_sizer->SetItemMinSize((size_t)0, label_size);
    }
     else if (m_mode == LinkPropsDialog::ModeEditNoDesc)
    {
        // measure the label widths
        wxSize label_size = cfw::getMaxTextSize(label_name,
                                                label_location,
                                                label_tags);
        label_size.x += 10;
        
        name_sizer->SetItemMinSize(label_name, label_size);
        location_sizer->SetItemMinSize(label_location, label_size);
        tags_sizer->SetItemMinSize(label_tags, label_size);
        flags_sizer->SetItemMinSize((size_t)0, label_size);
    }
     else if (m_mode == LinkPropsDialog::ModeCreateSmall)
    {
        // measure the label widths
        wxSize label_size = cfw::getMaxTextSize(label_create,
                                                label_name);
        label_size.x += 10;
        
        create_sizer->SetItemMinSize(label_create, label_size);
        name_sizer->SetItemMinSize(label_name, label_size);
        flags_sizer->SetItemMinSize((size_t)0, label_size);
    }
    
    
    
    
    // create a platform standards-compliant OK/Cancel sizer
    wxButton* ok_button = new wxButton(this, wxID_OK);
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(ok_button);
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    ok_button->SetDefault();
    
    // this code is necessary to get the OK/Cancel sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(message_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    main_sizer->Add(create_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    main_sizer->Add(name_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    main_sizer->Add(separator, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    main_sizer->Add(location_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    main_sizer->Add(tags_sizer, 2, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    main_sizer->Add(description_sizer, 3, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    main_sizer->Add(flags_sizer, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    wxSizerItem* spacer = main_sizer->AddStretchSpacer();
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    
    if (m_mode == LinkPropsDialog::ModeCreate ||
        m_mode == LinkPropsDialog::ModeEdit)
    {
        message_sizer->Show(false);
        create_sizer->Show(false);
        tags_sizer->Show(false);
        separator->Show(false);
        spacer->Show(false);
    }
     else if (m_mode == LinkPropsDialog::ModeEditNoDesc)
    {
        message_sizer->Show(false);
        create_sizer->Show(false);
        tags_sizer->Show(false);
        description_sizer->Show(false);
        separator->Show(false);
        flags_sizer->Show(false);
    }
     else if (m_mode == LinkPropsDialog::ModeCreateSmall)
    {
        message_sizer->Show(false);
        separator->Show(false);
        location_sizer->Show(false);
        tags_sizer->Show(false);
        description_sizer->Show(false);
    }
     else if (m_mode == LinkPropsDialog::ModeRename)
    {
        create_sizer->Show(false);
        separator->Show(false);
        location_sizer->Show(false);
        tags_sizer->Show(false);
        description_sizer->Show(false);
    }
    
    SetSizer(main_sizer);
    Layout();
    
    m_name_textctrl->SetFocus();
    m_name_textctrl->SetSelection(0,-1);
    
    return wxDialog::ShowModal();
}

wxString LinkPropsDialog::getPath()
{
    if (!m_db_comboctrl)
        return wxEmptyString;
        
    wxString name = m_name;
    trimUnwantedUrlChars(name);
    
    wxString retval = m_db_comboctrl->getPath();
    retval += wxT("/");
    retval += name;
    return retval;
}

wxString LinkPropsDialog::getName()
{
    wxString name = m_name;
    trimUnwantedUrlChars(name);
    return name;
}

void LinkPropsDialog::onNameChanged(wxCommandEvent& evt)
{
    if (m_name_textctrl == NULL)
        return;
    
    m_name = evt.GetString();
}

void LinkPropsDialog::onLocationChanged(wxCommandEvent& evt)
{
    if (m_location_textctrl == NULL)
        return;
    
    m_location = evt.GetString();

}

void LinkPropsDialog::onTagsChanged(wxCommandEvent& evt)
{
    if (m_tags_textctrl == NULL)
        return;
    
    m_tags = evt.GetString();

}

void LinkPropsDialog::onDescriptionChanged(wxCommandEvent& evt)
{
    if (m_description_textctrl == NULL)
        return;
    
    m_description = evt.GetString();
}


void LinkPropsDialog::onRunTargetChanged(wxCommandEvent& evt)
{
    if (m_runtarget_checkbox == NULL)
        return;
        
    m_run_target = m_runtarget_checkbox->GetValue();
}



void LinkPropsDialog::onSize(wxSizeEvent& evt)
{
    Layout();
}




