/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-11-06
 *
 */


#include "appmain.h"
#include "dlgbookmarkprops.h"
#include "bookmarkfscombo.h"

enum
{
    ID_NameTextCtrl = wxID_HIGHEST+1,
    ID_BookmarkFsCtrl,
    ID_LocationTextCtrl,
    ID_TagsTextCtrl,
    ID_DescriptionTextCtrl,
    ID_RunTargetCheckbox,
};


// BookmarkPropsDialog class implementation

BEGIN_EVENT_TABLE(BookmarkPropsDialog, wxDialog)
    EVT_TEXT(ID_NameTextCtrl, BookmarkPropsDialog::onNameChanged)
    EVT_TEXT(ID_LocationTextCtrl, BookmarkPropsDialog::onLocationChanged)
    EVT_TEXT(ID_TagsTextCtrl, BookmarkPropsDialog::onTagsChanged)
    EVT_TEXT(ID_DescriptionTextCtrl, BookmarkPropsDialog::onDescriptionChanged)
    EVT_CHECKBOX(ID_RunTargetCheckbox, BookmarkPropsDialog::onRunTargetChanged)
    EVT_SIZE(BookmarkPropsDialog::onSize)
    EVT_BUTTON(wxID_OK, BookmarkPropsDialog::onOk)
END_EVENT_TABLE()


BookmarkPropsDialog::BookmarkPropsDialog(wxWindow* parent,
                                 const wxString& name,
                                 const wxString& location,
                                 const wxString& tags,
                                 const wxString& description) :
                            wxDialog(parent, 
                                     -1,
                                     _("Bookmark Properties"),
                                     wxDefaultPosition,
                                     parent->FromDIP(wxSize(460,300)),
                                     wxDEFAULT_DIALOG_STYLE |
                                     wxRESIZE_BORDER |
                                     wxCLIP_CHILDREN |
                                     wxNO_FULL_REPAINT_ON_RESIZE)
{
    m_name_textctrl = nullptr;
    m_location_textctrl = nullptr;
    m_tags_textctrl = nullptr;
    m_description_textctrl = nullptr;
    m_runtarget_checkbox = nullptr;
    
    m_name = name;
    m_location = location;
    m_tags = tags;
    m_description = description;
    m_run_target = false;
    
    m_mode = BookmarkPropsDialog::ModeCreate;
}

BookmarkPropsDialog::~BookmarkPropsDialog()
{

}

int BookmarkPropsDialog::ShowModal()
{
    // if we haven't specified a min size, do it now
    if (!GetMinSize().IsFullySpecified())
    {
        SetMinSize(GetSize());
    }

    // create the message sizer
    wxStaticText* label_message = new wxStaticText(this, -1, m_message);
    
    wxBoxSizer* message_sizer = new wxBoxSizer(wxHORIZONTAL);
    message_sizer->Add(label_message, 1, wxALIGN_CENTER);

    // create the separator
    wxStaticLine* separator = new wxStaticLine(this, -1);

    // create the name sizer
    wxStaticText* label_name = new wxStaticText(this,
                                                -1,
                                                _("Name:"),
                                                wxDefaultPosition,
                                                wxDefaultSize);
    m_name_textctrl = nullptr;
    m_name_textctrl = new wxTextCtrl(this,
                                     ID_NameTextCtrl,
                                     m_name);
    
    wxBoxSizer* name_sizer = new wxBoxSizer(wxHORIZONTAL);
    name_sizer->Add(label_name, 0, wxALIGN_CENTER);
    name_sizer->Add(m_name_textctrl, 1, wxALIGN_CENTER);

    if (m_mode == BookmarkPropsDialog::ModeRename)
        label_name->Show(false);
    
    // create the bookmark location sizer
    wxStaticText* label_bookmark_location = new wxStaticText(this,
                                                    -1,
                                                    _("Folder:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_bookmarkfs_combo = nullptr;
    m_bookmarkfs_combo = new BookmarkFsComboCtrl(this, ID_BookmarkFsCtrl, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxCB_READONLY);
    m_bookmarkfs_combo->setBookmarkFs(g_app->getBookmarkFs());

    
    wxBoxSizer* bookmarkfs_sizer = new wxBoxSizer(wxHORIZONTAL);
    bookmarkfs_sizer->Add(label_bookmark_location, 0, wxALIGN_CENTER);
    bookmarkfs_sizer->Add(m_bookmarkfs_combo, 1, wxALIGN_CENTER);


    // create the url/path sizer
    wxStaticText* label_location = new wxStaticText(this,
                                                    -1,
                                                    _("Target URL or Location:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_location_textctrl = nullptr;
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
    m_tags_textctrl = nullptr;
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
    m_description_textctrl = nullptr;
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
    
    
    m_runtarget_checkbox = nullptr;
    m_runtarget_checkbox = new wxCheckBox(this,
                                            ID_RunTargetCheckbox,
                                            _("Run Target"),
                                            wxDefaultPosition);
    m_runtarget_checkbox->SetValue(m_run_target);


    
    wxBoxSizer* flags_sizer = new wxBoxSizer(wxHORIZONTAL);
    flags_sizer->AddSpacer(1);
    flags_sizer->Add(m_runtarget_checkbox);


    if (m_mode == BookmarkPropsDialog::ModeCreate ||
        m_mode == BookmarkPropsDialog::ModeEdit)
    {
        // measure the label widths
        wxSize label_size = getMaxTextSize(label_name,
                                           label_bookmark_location,
                                           label_location,
                                           label_tags,
                                           label_description);
        label_size.x += 10;
        
        name_sizer->SetItemMinSize(label_name, label_size);
        bookmarkfs_sizer->SetItemMinSize(label_bookmark_location, label_size);
        location_sizer->SetItemMinSize(label_location, label_size);
        tags_sizer->SetItemMinSize(label_tags, label_size);
        description_sizer->SetItemMinSize(label_description, label_size);
        flags_sizer->SetItemMinSize((size_t)0, label_size);
    }
     else if (m_mode == BookmarkPropsDialog::ModeEditNoDesc)
    {
        // measure the label widths
        wxSize label_size = getMaxTextSize(label_name,
                                           label_location,
                                           label_tags);
        label_size.x += 10;
        
        name_sizer->SetItemMinSize(label_name, label_size);
        location_sizer->SetItemMinSize(label_location, label_size);
        tags_sizer->SetItemMinSize(label_tags, label_size);
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
    main_sizer->Add(message_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
    main_sizer->Add(name_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
    main_sizer->Add(separator, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
    main_sizer->Add(bookmarkfs_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
    main_sizer->Add(location_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
    main_sizer->Add(tags_sizer, 2, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
    main_sizer->Add(description_sizer, 3, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
    main_sizer->Add(flags_sizer, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
    wxSizerItem* spacer = main_sizer->AddStretchSpacer();
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    
    if (m_mode == BookmarkPropsDialog::ModeCreate ||
        m_mode == BookmarkPropsDialog::ModeEdit)
    {
        message_sizer->Show(false);
        tags_sizer->Show(false);
        separator->Show(false);
        spacer->Show(false);

        if (m_mode == BookmarkPropsDialog::ModeEdit)
        {
            bookmarkfs_sizer->Show(false);
        }
    }
     else if (m_mode == BookmarkPropsDialog::ModeEditNoDesc)
    {
        message_sizer->Show(false);
        tags_sizer->Show(false);
        bookmarkfs_sizer->Show(false);
        description_sizer->Show(false);
        separator->Show(false);
        flags_sizer->Show(false);
    }
     else if (m_mode == BookmarkPropsDialog::ModeRename || m_mode == BookmarkPropsDialog::ModeCreateFolder)
    {
        separator->Show(false);
        location_sizer->Show(false);
        bookmarkfs_sizer->Show(false);
        tags_sizer->Show(false);
        description_sizer->Show(false);
        flags_sizer->Show(false);
    }

    
    SetSizer(main_sizer);
    Layout();
    
    m_name_textctrl->SetFocus();
    m_name_textctrl->SetSelection(0,-1);
    
    updateRunTargetStatus();

    return wxDialog::ShowModal();
}

wxString BookmarkPropsDialog::getPath()
{
    wxString name = m_name;
    trimUnwantedUrlChars(name);
    
    wxString retval;
    retval += m_bookmarkfs_combo->getPath();
    if (retval.Last() != wxT('/'))
        retval += wxT('/');
    retval += name;
    return retval;
}

wxString BookmarkPropsDialog::getName()
{
    wxString name = m_name;
    trimUnwantedUrlChars(name);
    return name;
}

void BookmarkPropsDialog::onNameChanged(wxCommandEvent& evt)
{
    if (m_name_textctrl == nullptr)
        return;
    
    m_name = evt.GetString();
}

void BookmarkPropsDialog::onLocationChanged(wxCommandEvent& evt)
{
    if (m_location_textctrl == nullptr)
        return;
    
    m_location = evt.GetString();

    updateRunTargetStatus();
}

void BookmarkPropsDialog::updateRunTargetStatus()
{
    if (m_location.Contains("://"))
    {
        m_runtarget_checkbox->SetValue(false);
        m_runtarget_checkbox->Enable(false);
    }
    else
    {
        m_runtarget_checkbox->Enable(true);
    }
}

void BookmarkPropsDialog::onTagsChanged(wxCommandEvent& evt)
{
    if (m_tags_textctrl == nullptr)
        return;
    
    m_tags = evt.GetString();

}

void BookmarkPropsDialog::onDescriptionChanged(wxCommandEvent& evt)
{
    if (m_description_textctrl == nullptr)
        return;
    
    m_description = evt.GetString();
}


void BookmarkPropsDialog::onRunTargetChanged(wxCommandEvent& evt)
{
    if (m_runtarget_checkbox == nullptr)
        return;
        
    m_run_target = m_runtarget_checkbox->GetValue();
}

void BookmarkPropsDialog::onOk(wxCommandEvent& evt)
{
    if (m_callback)
    {
        if (!m_callback())
            return;
    }

    EndModal(wxID_OK);
}

void BookmarkPropsDialog::onSize(wxSizeEvent& evt)
{
    Layout();
}




