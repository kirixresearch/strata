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
    ID_ToggleButton_DataSource,

    ID_Server_Type,
    ID_Server_Server,
    ID_Server_Database,
    ID_Server_Port,
    ID_Server_Username,
    ID_Server_Password,

    ID_DataSource_Grid

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




    // -- file page ----------------------------------------------------------

    m_filepage_sizer = new wxBoxSizer(wxVERTICAL);
    m_file_ctrl = new wxFileListCtrl(this, -1, "", false);
    m_file_ctrl->GoToHomeDir();
    m_filepage_sizer->Add(m_file_ctrl, 1, wxEXPAND);






    // -- server page --------------------------------------------------------
    
    m_serverpage_sizer = new wxBoxSizer(wxVERTICAL);

    // create message
    wxStaticText* server_message_label = new wxStaticText(this, -1, _("Please enter the connection settings for the database to which you would like to connect."));
    resizeStaticText(server_message_label);

    // create the server sizer
    wxStaticText* server_label = new wxStaticText(this, -1, _("Server:"));
    wxTextCtrl* server_textctrl = new wxTextCtrl(this, ID_Server_Server, 
                                                 m_ci.server,
                                                 wxDefaultPosition,
                                                 wxSize(200,21));
    
    wxSizer* server_sizer = new wxBoxSizer(wxHORIZONTAL);
    server_sizer->Add(50,23);
    server_sizer->Add(server_label, 0, wxALIGN_CENTER);
    server_sizer->Add(server_textctrl, 1, wxALIGN_CENTER);
    server_sizer->Add(50,23);
    
    // create the database sizer
    wxStaticText* database_label = new wxStaticText(this,  -1,  _("Database:"));
    wxTextCtrl* database_textctrl = new wxTextCtrl(this, 
                                         ID_Server_Database,
                                         m_ci.database,
                                         wxDefaultPosition,
                                         wxSize(200,21));
    
    wxSizer* database_sizer = new wxBoxSizer(wxHORIZONTAL);
    database_sizer->Add(50,23);
    database_sizer->Add(database_label, 0, wxALIGN_CENTER);
    database_sizer->Add(database_textctrl, 1, wxALIGN_CENTER);
    database_sizer->Add(50,23);

    // create the port number sizer
    wxStaticText* port_label = new wxStaticText(this, -1,  _("Port Number:"));
    wxTextCtrl* port_textctrl = new wxTextCtrl(this, 
                                     ID_Server_Port, 
                                     wxString::Format(wxT("%d"), m_ci.port),
                                     wxDefaultPosition,
                                     wxSize(200,21));
    
    wxSizer* port_sizer = new wxBoxSizer(wxHORIZONTAL);
    port_sizer->Add(50,23);
    port_sizer->Add(port_label, 0, wxALIGN_CENTER);
    port_sizer->Add(port_textctrl, 1, wxALIGN_CENTER);
    port_sizer->Add(50,23);

    // create the username sizer
    wxStaticText* username_label = new wxStaticText(this, -1, _("User Name:"));
    wxTextCtrl* username_textctrl = new wxTextCtrl(this, 
                                         ID_Server_Username,
                                         m_ci.username,
                                         wxDefaultPosition,
                                         wxSize(200,21));
    
    wxSizer* username_sizer = new wxBoxSizer(wxHORIZONTAL);
    username_sizer->Add(50,23);
    username_sizer->Add(username_label, 0, wxALIGN_CENTER);
    username_sizer->Add(username_textctrl, 1, wxALIGN_CENTER);
    username_sizer->Add(50,23);

    // create the password sizer
    wxStaticText* password_label = new wxStaticText(this,
                                                    -1,
                                                    _("Password:"));

    wxTextCtrl* password_textctrl = new wxTextCtrl(this,
                                         ID_Server_Password,
                                         m_ci.password,
                                         wxDefaultPosition,
                                         wxSize(200,21),
                                         wxTE_PASSWORD);
    
    wxSizer* password_sizer = new wxBoxSizer(wxHORIZONTAL);
    password_sizer->Add(50,23);
    password_sizer->Add(password_label, 0, wxALIGN_CENTER);
    password_sizer->Add(password_textctrl, 1, wxALIGN_CENTER);
    password_sizer->Add(50,23);

    // measure the label widths
    wxSize label_size = getMaxTextSize(server_label,
                                       database_label,
                                       username_label,
                                       password_label,
                                       port_label);
    label_size.x += 10;
    

    server_sizer->SetItemMinSize(server_label, label_size);
    database_sizer->SetItemMinSize(database_label, label_size);
    username_sizer->SetItemMinSize(username_label, label_size);
    password_sizer->SetItemMinSize(password_label, label_size);
    port_sizer->SetItemMinSize(port_label, label_size);

    // create main sizer
    m_serverpage_sizer->AddSpacer(20);
    m_serverpage_sizer->Add(server_message_label, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_serverpage_sizer->AddSpacer(4);
    m_serverpage_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                            0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_serverpage_sizer->AddSpacer(2);
    m_serverpage_sizer->Add(server_sizer, 0, wxEXPAND | wxTOP, 10);
    m_serverpage_sizer->Add(database_sizer, 0, wxEXPAND | wxTOP, 10);
    m_serverpage_sizer->Add(port_sizer, 0, wxEXPAND | wxTOP, 10);
    m_serverpage_sizer->Add(username_sizer, 0, wxEXPAND | wxTOP, 10);
    m_serverpage_sizer->Add(password_sizer, 0, wxEXPAND | wxTOP, 10);




    // -- data source page ---------------------------------------------------

    m_datasourcepage_sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* datasource_message_label = new wxStaticText(this, -1, _("Please select the ODBC data source to which you would like to connect."));
    resizeStaticText(datasource_message_label);

    // create grid
    m_datasource_grid = new kcl::Grid;
    m_datasource_grid->setCursorType(kcl::Grid::cursorRowHighlight);
    m_datasource_grid->setBorderType(kcl::DEFAULT_BORDER);
    m_datasource_grid->setOptionState(kcl::Grid::optHorzGridLines |
                           kcl::Grid::optVertGridLines |
                           kcl::Grid::optColumnMove |
                           kcl::Grid::optColumnResize |
                           kcl::Grid::optSelect |
                           kcl::Grid::optActivateHyperlinks, false);
    m_datasource_grid->createModelColumn(-1, _("Data Source"), kcl::Grid::typeCharacter, 256, 0);
    m_datasource_grid->createModelColumn(-1, _("Description"), kcl::Grid::typeCharacter, 1024, 0);
    m_datasource_grid->setRowLabelSize(0);
    m_datasource_grid->createDefaultView();
    m_datasource_grid->setColumnProportionalSize(0, 1);
    m_datasource_grid->setColumnProportionalSize(1, 2);
    m_datasource_grid->Create(this, ID_DataSource_Grid);

    kcl::CellProperties cp;
    cp.mask = kcl::CellProperties::cpmaskEditable;
    cp.editable = false;
    m_datasource_grid->setModelColumnProperties(0, &cp);
    
    populateDataSourceGrid();
    
    // create main sizer
    m_datasourcepage_sizer = new wxBoxSizer(wxVERTICAL);
    m_datasourcepage_sizer->AddSpacer(20);
    m_datasourcepage_sizer->Add(datasource_message_label, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_datasourcepage_sizer->AddSpacer(4);
    m_datasourcepage_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                    0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_datasourcepage_sizer->AddSpacer(12);
    m_datasourcepage_sizer->Add(m_datasource_grid, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_datasourcepage_sizer->AddSpacer(20);




    m_container_sizer = new wxBoxSizer(wxVERTICAL);
    m_container_sizer->Add(m_filepage_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    m_container_sizer->Add(m_serverpage_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    m_container_sizer->Add(m_datasourcepage_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    setActivePage(ID_ToggleButton_Folder);



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
        m_container_sizer->GetItem(m_filepage_sizer)->SetProportion(1);
        m_container_sizer->Hide(m_serverpage_sizer);
        m_container_sizer->GetItem(m_serverpage_sizer)->SetProportion(0);
        m_container_sizer->Hide(m_datasourcepage_sizer);
        m_container_sizer->GetItem(m_datasourcepage_sizer)->SetProportion(0);
    }
     else if (page == ID_ToggleButton_Server)
    {
        m_container_sizer->Hide(m_filepage_sizer);
        m_container_sizer->GetItem(m_filepage_sizer)->SetProportion(0);
        m_container_sizer->Show(m_serverpage_sizer);
        m_container_sizer->GetItem(m_serverpage_sizer)->SetProportion(1);
        m_container_sizer->Hide(m_datasourcepage_sizer);
        m_container_sizer->GetItem(m_datasourcepage_sizer)->SetProportion(0);
    }
     else if (page == ID_ToggleButton_DataSource)
    {
        m_container_sizer->Hide(m_filepage_sizer);
        m_container_sizer->GetItem(m_filepage_sizer)->SetProportion(0);
        m_container_sizer->Hide(m_serverpage_sizer);
        m_container_sizer->GetItem(m_serverpage_sizer)->SetProportion(0);
        m_container_sizer->Show(m_datasourcepage_sizer);
        m_container_sizer->GetItem(m_datasourcepage_sizer)->SetProportion(1);
    }

    Layout();
}


void DlgConnection::populateDataSourceGrid()
{
    xcm::ptr<xd::IDatabaseMgr> dbmgr;
    dbmgr.create_instance("xdodbc.DatabaseMgr");

    if (dbmgr.isNull())
    {
        wxString appname = APPLICATION_NAME;
        wxString message = wxString::Format(_("This software is missing a required component (xdodbc) and must be reinstalled"));
        m_datasource_grid->setOverlayText(message);
        m_datasource_grid->SetFocus();
        return;
    }

    xd::IDatabaseEntryEnumPtr odbc_databases;
    odbc_databases = dbmgr->getDatabaseList(L"", 0, L"", L"");

    m_datasource_grid->deleteAllRows();

    int i, count = odbc_databases->size();

    for (i = 0; i < count; ++i)
    {
        m_datasource_grid->insertRow(-1);

        xd::IDatabaseEntryPtr item = odbc_databases->getItem(i);
        m_datasource_grid->setCellString(i, 0, item->getName());
        m_datasource_grid->setCellString(i, 1, item->getDescription());
    }

    // if we already have a server name, move the cursor
    // to the corresponding grid row

    int row_count = m_datasource_grid->getRowCount();
    std::wstring cell_str;

    for (i = 0; i < row_count; ++i)
    {
        cell_str = towstr(m_datasource_grid->getCellString(i, 0));

        if (kl::iequals(m_ci.server, cell_str))
        {
            m_datasource_grid->moveCursor(i, 0);
            break;
        }
    }

    if (row_count == 0)
        m_datasource_grid->setOverlayText(_("Please contact your database administrator\nto set up an ODBC database connection"));
         else
        m_datasource_grid->setOverlayText(wxEmptyString);
    
    m_datasource_grid->refresh(kcl::Grid::refreshAll);
}
