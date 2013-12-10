/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2013-12-09
 *
 */


#include "appmain.h"
#include "dlgconnection.h"
#include <wx/artprov.h>
#include <wx/tglbtn.h>
#include <wx/filectrl.h>


enum
{
    ID_First = wxID_HIGHEST + 1,
    ID_ToggleButton_Folder,
    ID_ToggleButton_Server,
    ID_ToggleButton_DataSource
};




BEGIN_EVENT_TABLE(DlgConnection, wxDialog)
    //EVT_BUTTON(wxID_OK, DlgConnection::onOK)
    //EVT_BUTTON(wxID_CANCEL, DlgConnection::onCancel)
    EVT_TOGGLEBUTTON(ID_ToggleButton_Folder, DlgConnection::onToggleButton)
    EVT_TOGGLEBUTTON(ID_ToggleButton_Server, DlgConnection::onToggleButton)
    EVT_TOGGLEBUTTON(ID_ToggleButton_DataSource, DlgConnection::onToggleButton)
END_EVENT_TABLE()


static wxBitmap addMarginToBitmap(const wxBitmap& bmp, int left, int top, int right, int bottom)
{
    wxImage img = bmp.ConvertToImage();
    wxSize new_size = img.GetSize();
    new_size.IncBy(left+right, top+bottom);
    wxImage newimg = img.Size(new_size, wxPoint(left, top));
    return wxBitmap(newimg);
}

DlgConnection::DlgConnection(wxWindow* parent) : wxDialog(parent,
                                                          -1,
                                                          _("Create Connection"),
                                                          wxDefaultPosition,
                                                          wxSize(540, 480),
                                                          wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

    // toggle button sizer

    wxBoxSizer* togglebutton_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBitmap bmp;

    m_togglebutton_folder = new wxToggleButton(this, ID_ToggleButton_Folder, _("Folder"));
    bmp = addMarginToBitmap(GETBMP(gf_folder_open_32), 0,8,0,0);
    m_togglebutton_folder->SetBitmap(bmp, wxTOP);

    m_togglebutton_server = new wxToggleButton(this, ID_ToggleButton_Server, _("Database"));
    bmp = addMarginToBitmap(GETBMP(gf_db_db_32), 0,8,0,0);
    m_togglebutton_server->SetBitmap(bmp, wxTOP);

    m_togglebutton_datasources = new wxToggleButton(this, ID_ToggleButton_DataSource, _("Data Sources"));
    bmp = addMarginToBitmap(GETBMP(gf_db_od_32), 0,8,0,0);
    m_togglebutton_datasources->SetBitmap(bmp, wxTOP);

    togglebutton_sizer->Add(m_togglebutton_folder, 0, wxEXPAND);
    togglebutton_sizer->Add(m_togglebutton_server, 0, wxEXPAND);
    togglebutton_sizer->Add(m_togglebutton_datasources, 0, wxEXPAND);




    // file page

    m_filepage_sizer = new wxBoxSizer(wxVERTICAL);
    m_file_ctrl = new wxFileListCtrl(this, -1, "", false);
    m_file_ctrl->GoToHomeDir();
    m_filepage_sizer->Add(m_file_ctrl, 1, wxEXPAND);



    // server page
    m_serverpage_sizer = new wxBoxSizer(wxVERTICAL);





    // data source page
    m_datasourcepage_sizer = new wxBoxSizer(wxVERTICAL);






    m_container_sizer = new wxBoxSizer(wxVERTICAL);
    m_container_sizer->Add(m_filepage_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    m_container_sizer->Add(m_serverpage_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    m_container_sizer->Add(m_datasourcepage_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);



    wxButton* ok_button = new wxButton(this, wxID_OK);
    wxButton* cancel_button = new wxButton(this, wxID_CANCEL);
        
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(ok_button);
    ok_cancel_sizer->AddButton(cancel_button);
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    ok_button->SetDefault();




    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(5);
    sizer->Add(togglebutton_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    sizer->AddSpacer(10);
    sizer->Add(m_container_sizer, 1, wxEXPAND);
    sizer->AddSpacer(10);
    sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    sizer->AddSpacer(10);
    SetSizer(sizer);


    Center();
}

DlgConnection::~DlgConnection()
{
}

void DlgConnection::onOK(wxCommandEvent& evt)
{
}
    
void DlgConnection::onCancel(wxCommandEvent& evt)
{

}

void DlgConnection::onToggleButton(wxCommandEvent& evt)
{
    setActivePage(evt.GetId());
}


void DlgConnection::setActivePage(int page)
{

    m_togglebutton_folder->SetValue(page == m_togglebutton_folder->GetId());
    m_togglebutton_server->SetValue(page == m_togglebutton_server->GetId());
    m_togglebutton_datasources->SetValue(page == m_togglebutton_datasources->GetId());
    
    if (page == ID_ToggleButton_Folder)
    {
        m_container_sizer->Show(m_filepage_sizer);
        m_container_sizer->Hide(m_serverpage_sizer);
        m_container_sizer->Hide(m_datasourcepage_sizer);
    }
     else if (page == ID_ToggleButton_Server)
    {
        m_container_sizer->Hide(m_filepage_sizer);
        m_container_sizer->Show(m_serverpage_sizer);
        m_container_sizer->Hide(m_datasourcepage_sizer);
    }
     else if (page == ID_ToggleButton_DataSource)
    {
        m_container_sizer->Hide(m_filepage_sizer);
        m_container_sizer->Hide(m_serverpage_sizer);
        m_container_sizer->Show(m_datasourcepage_sizer);
    }
}
