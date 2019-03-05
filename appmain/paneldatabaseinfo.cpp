/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-03-25
 *
 */


#include "appmain.h"
#include "paneldatabaseinfo.h"
#include "appcontroller.h"


BEGIN_EVENT_TABLE(DatabaseInfoPanel, wxPanel)
    EVT_BUTTON(wxID_OK, DatabaseInfoPanel::onOK)
    EVT_BUTTON(wxID_CANCEL, DatabaseInfoPanel::onCancel)
END_EVENT_TABLE()


DatabaseInfoPanel::DatabaseInfoPanel()
{
}

DatabaseInfoPanel::~DatabaseInfoPanel()
{
}

// IDocument
bool DatabaseInfoPanel::initDoc(IFramePtr frame,
                                IDocumentSitePtr site,
                                wxWindow* doc_site,
                                wxWindow* pane_site)
{
    if (!Create(doc_site,
                -1,
                wxPoint(0,0),
                doc_site->GetClientSize(),
                wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN))
    {
        return false;
    }

    site->setCaption(_("Project Properties"));
    m_doc_site = site;

    // create project name sizer
    
    wxStaticText* project_name_label = new wxStaticText(this,
                                             -1,
                                             _("Name:"));
    m_project_name_text = new wxTextCtrl(this,
                                         -1,
                                         wxEmptyString,
                                         wxDefaultPosition,
                                         wxSize(100, 21));
    m_project_name = g_app->getProjectName();
    
    if (m_project_name.Length() == 0)
        m_project_name = _("Project");
        
    m_project_name_text->SetValue(m_project_name);
    
    wxBoxSizer* project_name_sizer = new wxBoxSizer(wxHORIZONTAL);
    project_name_sizer->Add(project_name_label, 0, wxALIGN_CENTER);
    project_name_sizer->Add(m_project_name_text, 1, wxALIGN_CENTER);


    // create project location sizer
    
    wxStaticText* project_location_label = new wxStaticText(
                                                this,
                                                -1,
                                                _("Location:"));
    wxString project_loc_str = g_app->getDatabaseLocation();


    std::wstring local_location = towstr(project_loc_str);
    if (kl::icontains(local_location, L"xdprovider="))
    {
        xd::ConnectionString cstr(local_location);
        std::wstring provider = cstr.getLowerValue(L"xdprovider");

        if (provider == L"xdnative" || provider == L"xdfs")
        {
            project_loc_str = cstr.getLowerValue(L"database");
        }
    }




    wxStaticText* location = new wxStaticText(this, -1, project_loc_str);
    
    wxBoxSizer* project_location_sizer = new wxBoxSizer(wxHORIZONTAL);
    project_location_sizer->Add(project_location_label, 0, wxALIGN_CENTER);
    project_location_sizer->Add(location, 1, wxALIGN_CENTER);


    // create used space sizer
    
    wxStaticText* project_size_label = new wxStaticText(
                                                this,
                                                -1,
                                                _("Size:"));
    // calculate the project size
    wxString size_str;
    double size = getProjectSize(project_loc_str);
    double mb_size = size/1048576.0;
    double gb_size = size/1073741824.0;
    if (gb_size >= 1.0)
        size_str = wxString::Format(wxT("%.2f GB"), gb_size);
         else
        size_str = wxString::Format(wxT("%.2f MB"), mb_size);
        
    wxStaticText* project_size_str = new wxStaticText(
                                                this,
                                                -1,
                                                size_str,
                                                wxDefaultPosition,
                                                wxDefaultSize,
                                                wxALIGN_RIGHT |
                                                wxST_NO_AUTORESIZE);

    wxBoxSizer* project_size_sizer = new wxBoxSizer(wxHORIZONTAL);
    project_size_sizer->Add(project_size_label, 0, wxALIGN_CENTER);
    project_size_sizer->Add(project_size_str, 0, wxALIGN_CENTER);


    // resize label widths so they line up
    wxSize min = getMaxTextSize(project_name_label,
                                     project_location_label,
                                     project_size_label);
    min.x += 10;
    
    project_name_sizer->SetItemMinSize(project_name_label, min);
    project_location_sizer->SetItemMinSize(project_location_label, min);
    project_size_sizer->SetItemMinSize(project_size_label, min);


    // create a platform standards-compliant OK/Cancel sizer
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_OK));
    ok_cancel_sizer->AddButton(new wxButton(this, wxID_CANCEL));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    

    // this code is necessary to get the sizer's bottom margin to 8

    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);


    // make sure the sizers are the same height
    project_name_sizer->SetMinSize(-1, 20);
    project_location_sizer->SetMinSize(-1, 20);
    project_size_sizer->SetMinSize(-1, 20);


    // create main sizer
    
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(12);
    main_sizer->Add(project_name_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
    main_sizer->AddSpacer(8);
    main_sizer->Add(project_location_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
    main_sizer->AddSpacer(8);
    main_sizer->Add(project_size_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
    main_sizer->AddStretchSpacer(1);
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    
    SetSizer(main_sizer);
    Layout();

    return true;
}

wxWindow* DatabaseInfoPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void DatabaseInfoPanel::setDocumentFocus()
{
}

void DatabaseInfoPanel::onOK(wxCommandEvent& evt)
{
    wxString new_project_name = m_project_name_text->GetValue();

    if (new_project_name != m_project_name)
        g_app->setProjectName(new_project_name);

    g_app->getAppController()->refreshDbDoc();
    g_app->getMainFrame()->closeSite(m_doc_site);
}

void DatabaseInfoPanel::onCancel(wxCommandEvent& evt)
{
    g_app->getMainFrame()->closeSite(m_doc_site);
}




