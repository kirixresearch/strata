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
#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/tglbtn.h>
#include <wx/statline.h>
#include <wx/listctrl.h>
#include <kl/string.h>
#include <xd/xd.h>
#include "../kcl/grid.h"
#include "../kcl/rowselectiongrid.h"
#include "../kcl/filepanel.h"
#include "../kcl/buttonbar.h"
#include "dlgconnection.h"
#include "util.h"



std::wstring Connection::getConnectionString()
{
    std::wstring cstr;
    std::wstring provider;
    std::wstring dbtype;
        
    switch (type)
    {
        case xd::dbtypeXdnative:        provider = L"xdnative"; break;
        case xd::dbtypeKpg:             provider = L"xdkpg"; break;

        #ifdef WIN32
        case xd::dbtypeAccess:          provider = L"xdodbc"; dbtype = L"access"; break;
        case xd::dbtypeSqlServer:       provider = L"xdodbc"; dbtype = L"mssql"; break;
        #else
        case xd::dbtypeAccess:          provider = L"xdaccess"; break;
        case xd::dbtypeSqlServer:       provider = L"xdsqlserver"; break;
        #endif                      
                                    
        case xd::dbtypeExcel:           provider = L"xdodbc"; dbtype = L"excel"; break;
        case xd::dbtypeSqlite:          provider = L"xdsqlite"; break;
        case xd::dbtypeMySql:           provider = L"xdmysql"; break;
                                    
        case xd::dbtypePostgres:        provider = L"xdpgsql"; break;
        case xd::dbtypeOracle:          provider = L"xdoracle"; break;
        case xd::dbtypeOdbc:            provider = L"xdodbc"; dbtype = L"dsn"; break;
        case xd::dbtypeDb2:             provider = L"xdodbc"; dbtype = L"db2"; break;
        case xd::dbtypeClient:          provider = L"xdclient"; break;
        case xd::dbtypeFilesystem:
        case xd::dbtypeUndefined:
            if (path.length() == 0)
                return L"";
            provider = L"xdfs"; break;

        default:
            return L"";
    }


    // build connection string
        

    cstr += L"xdprovider=";
    cstr += provider;
    cstr += L";";
        
    if (dbtype.length() > 0)
    {
        cstr += L"xddbtype=";
        cstr += dbtype;
        cstr += L";";
    }
        
    if (server.length() > 0)
    {
        cstr += L"host=";
        cstr += server;
        cstr += L";";
    }
        
    if (port != 0)
    {
        cstr += L"port=";
        cstr += kl::itowstring(port);
        cstr += L";";
    }
        
    cstr += L"user id=";
    cstr += username;
    cstr += L";";
        
    cstr += L"password=";
    cstr += password;
    cstr += L";";
        
    if (database.length() > 0)
    {
        cstr += L"database=";
        cstr += database;
        cstr += L";";
    }
     else
    {
        if (type == xd::dbtypeKpg || type == xd::dbtypeAccess || type == xd::dbtypeSqlite || type == xd::dbtypeExcel || type == xd::dbtypeFilesystem)
        {
            cstr += L"database=";
            cstr += path;
            cstr += L";";
        }
    }
        
    return cstr;
}

















enum
{
    ID_First = wxID_HIGHEST + 1,
    ID_ToggleButton_Folder,
    ID_ToggleButton_File,
    ID_ToggleButton_Server,
    ID_ToggleButton_DataSource,

    ID_Server_Type,
    ID_Server_Server,
    ID_Server_Database,
    ID_Server_Port,
    ID_Server_Username,
    ID_Server_Password,

    ID_DataSource_Grid,

    ID_TableList_Grid,
    ID_TableList_BasePathTextCtrl,
    ID_TableList_BasePathBrowseButton,
    ID_TableList_SelectAllButton,
    ID_TableList_SelectNoneButton,

    ID_File_Panel

};



// table selection grid column indexes

enum
{
    ONOFF_IDX = 0,
    SOURCE_TABLENAME_IDX = 1,
    DEST_TABLENAME_IDX = 2,
    APPEND_IDX = 3
};



BEGIN_EVENT_TABLE(DlgConnection, wxDialog)
    EVT_BUTTON(ID_ToggleButton_File, DlgConnection::onToggleButton)
    EVT_BUTTON(ID_ToggleButton_Server, DlgConnection::onToggleButton)
    EVT_BUTTON(ID_ToggleButton_DataSource, DlgConnection::onToggleButton)

    EVT_CHOICE(ID_Server_Type,   DlgConnection::onServerParameterChanged)
    EVT_TEXT(ID_Server_Server,   DlgConnection::onServerParameterChanged)
    EVT_TEXT(ID_Server_Database, DlgConnection::onServerParameterChanged)
    EVT_TEXT(ID_Server_Port,     DlgConnection::onServerParameterChanged)
    EVT_TEXT(ID_Server_Username, DlgConnection::onServerParameterChanged)
    EVT_TEXT(ID_Server_Password, DlgConnection::onServerParameterChanged)
    EVT_TEXT_ENTER(ID_Server_Password, DlgConnection::onServerPasswordEnterPressed)

    EVT_BUTTON(ID_TableList_SelectAllButton, DlgConnection::onTableListSelectAll)
    EVT_BUTTON(ID_TableList_SelectNoneButton, DlgConnection::onTableListSelectNone)

    EVT_BUTTON(wxID_BACKWARD, DlgConnection::onBackward)
    EVT_BUTTON(wxID_FORWARD, DlgConnection::onForward)

    EVT_BUTTON(wxID_OK, DlgConnection::onOK)
    EVT_BUTTON(wxID_CANCEL, DlgConnection::onCancel)

    EVT_FILEPANEL_ITEM_ACTIVATED(ID_File_Panel, DlgConnection::onFilePanelItemActivated)

    EVT_KCLGRID_CELL_LEFT_DCLICK(DlgConnection::onDataSourceLeftDClick)
END_EVENT_TABLE()


static wxBitmap addMarginToBitmap(const wxBitmap& bmp, int left, int top, int right, int bottom)
{
    wxImage img = bmp.ConvertToImage();
    wxSize new_size = img.GetSize();
    new_size.IncBy(left+right, top+bottom);
    wxImage newimg = img.Size(new_size, wxPoint(left, top));
    return wxBitmap(newimg);
}

DlgConnection::DlgConnection(wxWindow* parent, wxWindowID id, const wxString& title, int options)
                                               : wxDialog(parent,
                                                          id,
                                                          title.empty() ? _("Create Connection") : title,
                                                          wxDefaultPosition,
                                                          wxSize(680, 520),
                                                          wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    m_last_page = 0;
    m_current_page = 0;
    m_options = options;
    m_need_text_format = false;

    // toggle button sizer


    kcl::ButtonBar* button_bar = new kcl::ButtonBar(this, wxID_ANY);
    button_bar->addItem(ID_ToggleButton_File, GETBMP(gf_folder_open_32), (m_options & optionFolder) ? _("Folder") : _("File"));
    button_bar->addItem(ID_ToggleButton_Server, GETBMP(gf_db_db_32), _("Database"));
    button_bar->addItem(ID_ToggleButton_DataSource, GETBMP(gf_db_od_32), _("Data Sources"));

    wxBoxSizer* togglebutton_sizer = new wxBoxSizer(wxHORIZONTAL);
    togglebutton_sizer->Add(button_bar, 1, wxEXPAND);

    // -- file page ----------------------------------------------------------

    m_filepage_sizer = new wxBoxSizer(wxVERTICAL);
    m_file_panel = new kcl::FilePanel(this, ID_File_Panel);

    if (m_options & optionFolder)
        m_file_panel->setFolderOnly(true);


    // populate the file dialog filter
    wxString filter;
    filter += _("All Files");
    filter += wxT(" (*.*)|*.*|");
    filter += _("Package Files");
    filter += wxT(" (*.kpg)|*.kpg|");
    filter += _("Microsoft Access Files");
    filter += wxT(" (*.mdb, *.accdb)|*.mdb;*.accdb|");
    filter += _("Microsoft Excel Files");
    filter += wxT(" (*.xls, *.xlsx)|*.xls;*.xlsx|");
    filter += _("Microsoft FoxPro/Xbase Files");
    filter += wxT(" (*.dbf)|*.dbf|");
    filter += _("Comma-Delimited Files");
    filter += wxT(" (*.csv)|*.csv|");
    filter += _("Tab-Delimited Files");
    filter += wxT(" (*.tsv)|*.tsv|");
    filter += _("Text Files");
    filter += wxT(" (*.txt)|*.txt|");

    m_file_panel->setFilterString(filter);
    m_filepage_sizer->Add(m_file_panel, 1, wxEXPAND);






    // -- server page --------------------------------------------------------
    
    m_serverpage_sizer = new wxBoxSizer(wxVERTICAL);

    // create message
    wxStaticText* server_message_label = new wxStaticText(this, -1, _("Please enter the connection settings for the database to which you would like to connect."));
    resizeStaticText(server_message_label);

    // create the server type
    wxStaticText* servertype_label = new wxStaticText(this, -1, _("Type:"));


    m_server_type = new wxChoice(this,
                                 ID_Server_Type,
                                 wxDefaultPosition,
                                 wxSize(200,21),
                                 0,
                                 NULL);
    
    m_server_type->Append(_("MySQL"),      (void*)xd::dbtypeMySql);     
    m_server_type->Append(_("SQL Server"), (void*)xd::dbtypeSqlServer);
    m_server_type->Append(_("Oracle"),     (void*)xd::dbtypeOracle);    
    m_server_type->Append(_("PostgreSQL"), (void*)xd::dbtypePostgres);
    m_server_type->Append(_("DB2"),        (void*)xd::dbtypeDb2);       
    
    // set combo setting from m_ci
    for (size_t idx = 0; idx < m_server_type->GetCount(); ++idx)
    {
        if (m_server_type->GetClientData(idx) == (void*)m_ci.type)
            m_server_type->SetSelection(idx);
    }

    
    wxSizer* servertype_sizer = new wxBoxSizer(wxHORIZONTAL);
    servertype_sizer->Add(80,23);
    servertype_sizer->Add(servertype_label, 0, wxALIGN_CENTER);
    servertype_sizer->Add(m_server_type, 1, wxALIGN_CENTER);
    servertype_sizer->Add(80,23);
    
    // create the server sizer
    wxStaticText* server_label = new wxStaticText(this, -1, _("Server:"));
    m_server_server = new wxTextCtrl(this,
                                       ID_Server_Server, 
                                       m_ci.server,
                                       wxDefaultPosition,
                                       wxSize(200,21));
    
    wxSizer* server_sizer = new wxBoxSizer(wxHORIZONTAL);
    server_sizer->Add(80,23);
    server_sizer->Add(server_label, 0, wxALIGN_CENTER);
    server_sizer->Add(m_server_server, 1, wxALIGN_CENTER);
    server_sizer->Add(80,23);
    
    // create the database sizer
    wxStaticText* database_label = new wxStaticText(this,  -1,  _("Database:"));
    wxTextCtrl* database_textctrl = new wxTextCtrl(this, 
                                         ID_Server_Database,
                                         m_ci.database,
                                         wxDefaultPosition,
                                         wxSize(200,21));
    
    wxSizer* database_sizer = new wxBoxSizer(wxHORIZONTAL);
    database_sizer->Add(80,23);
    database_sizer->Add(database_label, 0, wxALIGN_CENTER);
    database_sizer->Add(database_textctrl, 1, wxALIGN_CENTER);
    database_sizer->Add(80,23);

    // create the port number sizer
    wxStaticText* port_label = new wxStaticText(this, -1,  _("Port Number:"));
    m_server_port = new wxTextCtrl(this, 
                                   ID_Server_Port, 
                                   wxString::Format("%d", m_ci.port),
                                   wxDefaultPosition,
                                   wxSize(200,21));
    
    wxSizer* port_sizer = new wxBoxSizer(wxHORIZONTAL);
    port_sizer->Add(80,23);
    port_sizer->Add(port_label, 0, wxALIGN_CENTER);
    port_sizer->Add(m_server_port, 1, wxALIGN_CENTER);
    port_sizer->Add(80,23);

    // create the username sizer
    wxStaticText* username_label = new wxStaticText(this, -1, _("User Name:"));
    wxTextCtrl* username_textctrl = new wxTextCtrl(this, 
                                         ID_Server_Username,
                                         m_ci.username,
                                         wxDefaultPosition,
                                         wxSize(200,21));
    
    wxSizer* username_sizer = new wxBoxSizer(wxHORIZONTAL);
    username_sizer->Add(80,23);
    username_sizer->Add(username_label, 0, wxALIGN_CENTER);
    username_sizer->Add(username_textctrl, 1, wxALIGN_CENTER);
    username_sizer->Add(80,23);

    // create the password sizer
    wxStaticText* password_label = new wxStaticText(this,
                                                    -1,
                                                    _("Password:"));

    wxTextCtrl* password_textctrl = new wxTextCtrl(this,
                                         ID_Server_Password,
                                         m_ci.password,
                                         wxDefaultPosition,
                                         wxSize(200,21),
                                         wxTE_PASSWORD | wxTE_PROCESS_ENTER);
    
    wxSizer* password_sizer = new wxBoxSizer(wxHORIZONTAL);
    password_sizer->Add(80,23);
    password_sizer->Add(password_label, 0, wxALIGN_CENTER);
    password_sizer->Add(password_textctrl, 1, wxALIGN_CENTER);
    password_sizer->Add(80,23);

    // measure the label widths
    wxSize label_size = getMaxTextSize(servertype_label,
                                       server_label,
                                       database_label,
                                       username_label,
                                       password_label,
                                       port_label);
    label_size.x += 10;
    

    servertype_sizer->SetItemMinSize(servertype_label, label_size);
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
    m_serverpage_sizer->AddSpacer(8);
    m_serverpage_sizer->Add(servertype_sizer, 0, wxEXPAND | wxTOP, 10);
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




    // -- table list page ----------------------------------------------------

    m_tablelistpage_sizer = new wxBoxSizer(wxVERTICAL);





    wxStaticText* tablelistpage_message_label = new wxStaticText(this, -1, _("Please select the tables from the list that you would like to import."));
    resizeStaticText(tablelistpage_message_label);


    // create import location sizer
    
    wxStaticText* label_basepath = new wxStaticText(this,
                                                    -1,
                                                    _("Import to:"),
                                                    wxDefaultPosition,
                                                    wxDefaultSize);
    m_tablelist_basepath = new wxTextCtrl(this,
                                         ID_TableList_BasePathTextCtrl,
                                         wxEmptyString,
                                         wxDefaultPosition,
                                         wxSize(200,21));
    m_tablelist_basepath->SetValue(m_ci.base_path);
    
    wxButton* browse_button = new wxButton(this, ID_TableList_BasePathBrowseButton, _("Browse..."));
    
    wxBoxSizer* dest_folder_sizer = new wxBoxSizer(wxHORIZONTAL);
    dest_folder_sizer->Add(label_basepath, 0, wxALIGN_CENTER);
    dest_folder_sizer->AddSpacer(5);
    dest_folder_sizer->Add(m_tablelist_basepath, 1, wxALIGN_CENTER);
    dest_folder_sizer->AddSpacer(5);
    dest_folder_sizer->Add(browse_button);
    

    // create table list grid
    m_tablelist_grid = new kcl::RowSelectionGrid(this,
                                       ID_TableList_Grid,
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       kcl::DEFAULT_BORDER,
                                       false, false);
    m_tablelist_grid->setGreenBarInterval(0);
    m_tablelist_grid->setAcceptsFocus(true);

    m_tablelist_grid->createModelColumn(ONOFF_IDX, wxEmptyString, kcl::Grid::typeBoolean, 1, 0);
    m_tablelist_grid->createModelColumn(SOURCE_TABLENAME_IDX, _("Source Name"), kcl::Grid::typeCharacter, 512, 0);
    m_tablelist_grid->createModelColumn(DEST_TABLENAME_IDX, _("Destination Name"), kcl::Grid::typeCharacter, 512, 0);
    m_tablelist_grid->createModelColumn(APPEND_IDX, _("Append"), kcl::Grid::typeBoolean, 1, 0);

    // set cell properties for the grid
    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskAlignment;
    cellprops.alignment = kcl::Grid::alignRight;
    m_tablelist_grid->setModelColumnProperties(ONOFF_IDX, &cellprops);
    
    cellprops.mask = kcl::CellProperties::cpmaskAlignment;
    cellprops.alignment = kcl::Grid::alignCenter;
    m_tablelist_grid->setModelColumnProperties(APPEND_IDX, &cellprops);
    
    cellprops.mask = kcl::CellProperties::cpmaskEditable;
    cellprops.editable = false;
    m_tablelist_grid->setModelColumnProperties(SOURCE_TABLENAME_IDX, &cellprops);
    
    cellprops.mask = kcl::CellProperties::cpmaskEditable;
    cellprops.editable = true;
    m_tablelist_grid->setModelColumnProperties(DEST_TABLENAME_IDX, &cellprops);
    

    m_tablelist_grid->setRowLabelSize(0);
    m_tablelist_grid->createDefaultView();
    m_tablelist_grid->setColumnSize(ONOFF_IDX, 23);
    m_tablelist_grid->setColumnProportionalSize(SOURCE_TABLENAME_IDX, 1); // was 120 px
    m_tablelist_grid->setColumnProportionalSize(DEST_TABLENAME_IDX, 1);
    m_tablelist_grid->setColumnSize(APPEND_IDX, 50);


    // create button sizer
    
    wxButton* selectall_button = new wxButton(this, ID_TableList_SelectAllButton, _("Select All"));
    wxButton* selectnone_button = new wxButton(this, ID_TableList_SelectNoneButton, _("Select None"));

    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->Add(selectall_button);
    button_sizer->AddSpacer(5);
    button_sizer->Add(selectnone_button);

    
    // create main sizer
    m_tablelistpage_sizer->AddSpacer(20);
    m_tablelistpage_sizer->Add(tablelistpage_message_label, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_tablelistpage_sizer->AddSpacer(4);
    m_tablelistpage_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                               0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_tablelistpage_sizer->AddSpacer(12);
    m_tablelistpage_sizer->Add(dest_folder_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_tablelistpage_sizer->AddSpacer(8);
    m_tablelistpage_sizer->Add(m_tablelist_grid, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_tablelistpage_sizer->AddSpacer(8);
    m_tablelistpage_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_tablelistpage_sizer->AddSpacer(12);




    // create text format sizer

    wxStaticBox* delimiter_staticbox = new wxStaticBox(this,
                                                       -1,
                                                       _("Field Delimiters"));

    wxStaticBox* textqualifier_staticbox = new wxStaticBox(this,
                                                       -1,
                                                       _("Text Qualifier"));
    
    wxStaticText* text = new wxStaticText(this,
                                          -1,
                                          _("Modify the settings below to determine how the text-delimited files should be read."));
    resizeStaticText(text);


    // create main delimiters sizer
    
    m_comma_radio = new wxRadioButton(this,
                                      -1,
                                      _("Comma"),
                                      wxDefaultPosition,
                                      wxDefaultSize,
                                      wxRB_GROUP);

    m_tab_radio = new wxRadioButton(this, -1, _("Tab "));
    m_semicolon_radio = new wxRadioButton(this, -1, _("Semicolon"));
    m_pipe_radio = new wxRadioButton(this, -1, _("Pipe"));
    m_space_radio = new wxRadioButton(this, -1, _("Space"));
    m_nodelimiters_radio = new wxRadioButton(this, -1, _("None"));
    m_otherdelimiters_radio = new wxRadioButton(this, -1, _("Other:"));
    m_otherdelimiters_text = new wxTextCtrl(this,
                                            -1,
                                            wxEmptyString,
                                            wxDefaultPosition,
                                            wxSize(50, 21));
    m_otherdelimiters_text->SetMaxLength(5);

    wxBoxSizer* other_delimiters_sizer = new wxBoxSizer(wxHORIZONTAL);
    other_delimiters_sizer->Add(m_otherdelimiters_radio, 0, wxALIGN_CENTER);
    other_delimiters_sizer->AddSpacer(5);
    other_delimiters_sizer->Add(m_otherdelimiters_text, 0, wxEXPAND);

    wxStaticBoxSizer* left_options_sizer = new wxStaticBoxSizer(delimiter_staticbox, wxVERTICAL);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_comma_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_tab_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_semicolon_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_pipe_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_space_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);
    left_options_sizer->Add(m_nodelimiters_radio, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(6);
    left_options_sizer->Add(other_delimiters_sizer, 0, wxEXPAND | wxLEFT, 10);
    left_options_sizer->AddSpacer(10);


    // create main text qualifier sizer
    
    m_doublequote_radio = new wxRadioButton(this,
                                            -1,
                                            _("Quotation Marks (\")"),
                                            wxDefaultPosition,
                                            wxDefaultSize,
                                            wxRB_GROUP);

    m_singlequote_radio = new wxRadioButton(this, -1, _("Single Quote (')"));
    m_notextqualifier_radio = new wxRadioButton(this, -1, _("None"));
    m_othertextqualifier_radio = new wxRadioButton(this, -1, _("Other:"));
    m_othertextqualifier_text = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxSize(50, 21));
    m_othertextqualifier_text->SetMaxLength(1);

    wxBoxSizer* other_textqualifier_sizer = new wxBoxSizer(wxHORIZONTAL);
    other_textqualifier_sizer->Add(m_othertextqualifier_radio, 0, wxALIGN_CENTER);
    other_textqualifier_sizer->AddSpacer(5);
    other_textqualifier_sizer->Add(m_othertextqualifier_text, 0, wxEXPAND);

    wxStaticBoxSizer* right_options_sizer = new wxStaticBoxSizer(textqualifier_staticbox, wxVERTICAL);
    right_options_sizer->AddSpacer(10);
    right_options_sizer->Add(m_doublequote_radio, 0, wxEXPAND | wxLEFT, 10);
    right_options_sizer->AddSpacer(10);
    right_options_sizer->Add(m_singlequote_radio, 0, wxEXPAND | wxLEFT, 10);
    right_options_sizer->AddSpacer(10);
    right_options_sizer->Add(m_notextqualifier_radio, 0, wxEXPAND | wxLEFT, 10);
    right_options_sizer->AddSpacer(6);
    right_options_sizer->Add(other_textqualifier_sizer, 0, wxEXPAND | wxLEFT, 10);
    right_options_sizer->AddSpacer(10);


    // create horizontal settings sizer
    wxBoxSizer* horz_sizer1 = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer1->Add(left_options_sizer, 1, wxEXPAND);
    horz_sizer1->AddSpacer(20);
    horz_sizer1->Add(right_options_sizer, 1, wxEXPAND);

    // create first row header sizer
    m_firstrowheader_checkbox = new wxCheckBox(this, -1, _("First row contains field names"));
    m_firstrowheader_checkbox->SetValue(true);

    wxBoxSizer* horz_sizer2 = new wxBoxSizer(wxHORIZONTAL);
    horz_sizer2->Add(12,1);
    horz_sizer2->Add(m_firstrowheader_checkbox, 1, wxALIGN_CENTER);

    // create main sizer
    m_textformatpage_sizer = new wxBoxSizer(wxVERTICAL);
    m_textformatpage_sizer->AddSpacer(20);
    m_textformatpage_sizer->Add(text, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_textformatpage_sizer->AddSpacer(4);
    m_textformatpage_sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxSize(1,1)),
                    0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    m_textformatpage_sizer->AddSpacer(12);
    m_textformatpage_sizer->Add(horz_sizer1, 0, wxEXPAND | wxLEFT | wxRIGHT, 40);
    m_textformatpage_sizer->AddSpacer(10);
    m_textformatpage_sizer->Add(horz_sizer2, 0, wxEXPAND | wxLEFT | wxRIGHT, 40);
    m_textformatpage_sizer->AddStretchSpacer(1);



    m_container_sizer = new wxBoxSizer(wxVERTICAL);
    m_container_sizer->Add(m_filepage_sizer, 1, wxEXPAND);
    m_container_sizer->Add(m_serverpage_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    m_container_sizer->Add(m_datasourcepage_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    m_container_sizer->Add(m_tablelistpage_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    m_container_sizer->Add(m_textformatpage_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);



    m_binarycopy_checkbox = new wxCheckBox(this, -1, _("Import as binary objects"));
    m_binarycopy_checkbox->SetValue(false);


    m_forward_button = new wxButton(this, wxID_FORWARD, _("Next"));
    m_backward_button = new wxButton(this, wxID_BACKWARD, _("Previous"));
    m_ok_button = new wxButton(this, wxID_OK);
    m_cancel_button = new wxButton(this, wxID_CANCEL);
        
    m_button_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_button_sizer->AddSpacer(8);
    m_button_sizer->Add(m_binarycopy_checkbox);
    m_button_sizer->AddStretchSpacer(1);
    m_button_sizer->Add(m_backward_button, 0, wxEXPAND | wxRIGHT, 5);
    m_button_sizer->Add(m_forward_button, 0, wxEXPAND | wxRIGHT, 5);
    m_button_sizer->Add(m_ok_button, 0, wxEXPAND | wxRIGHT, 5);
    m_button_sizer->Add(m_cancel_button, 0, wxEXPAND | wxRIGHT, 5);



    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(togglebutton_sizer, 0, wxEXPAND, 5);
    sizer->AddSpacer(10);
    sizer->Add(m_container_sizer, 1, wxEXPAND);
    sizer->AddSpacer(10);
    sizer->Add(m_button_sizer, 0, wxEXPAND);
    sizer->AddSpacer(10);
    SetSizer(sizer);


    button_bar->selectItem(ID_ToggleButton_File);
    setActivePage(pageFile);


    Center();
}

DlgConnection::~DlgConnection()
{
}


wxString DlgConnection::getFilePanelDirectory()
{
    return m_file_panel->getDirectory();
}

void DlgConnection::setFilePanelDirectory(const wxString& path)
{
    m_file_panel->setDirectory(path);
}



void DlgConnection::onServerParameterChanged(wxCommandEvent& evt)
{
    switch (evt.GetId())
    {
        case ID_Server_Type:
            m_ci.type = (long)m_server_type->GetClientData(m_server_type->GetSelection());
            switch (m_ci.type)
            {
                case xd::dbtypeMySql:      m_ci.port = 3306; break;
                case xd::dbtypeSqlServer:  m_ci.port = 1433; break;
                case xd::dbtypeOracle:     m_ci.port = 1521; break;
                case xd::dbtypePostgres:   m_ci.port = 5432; break;
                case xd::dbtypeDb2:        m_ci.port = 50000; break;
            }

            m_server_port->SetValue(wxString::Format("%d", m_ci.port));
            break;

        case ID_Server_Server:    m_ci.server = evt.GetString().ToStdWstring(); break;
        case ID_Server_Database:  m_ci.database = evt.GetString().ToStdWstring(); break;
        case ID_Server_Username:  m_ci.username = evt.GetString().ToStdWstring(); break;
        case ID_Server_Password:  m_ci.password = evt.GetString().ToStdWstring(); break;
        case ID_Server_Port:      m_ci.port = wxAtoi(evt.GetString()); break;
    }
}

void DlgConnection::onServerPasswordEnterPressed(wxCommandEvent& evt)
{
    if (m_forward_button->IsShown())
    {
        wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, m_forward_button->GetId());
        evt.SetEventObject(this);
        GetEventHandler()->ProcessEvent(evt);
    }
     else if (m_ok_button->IsShown())
    {
        wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, m_ok_button->GetId());
        evt.SetEventObject(this);
        GetEventHandler()->ProcessEvent(evt);
    }
}


void DlgConnection::onTableListSelectAll(wxCommandEvent& evt)
{
    if (m_tablelist_grid->isEditing())
        m_tablelist_grid->endEdit(true);

    int row, count = m_tablelist_grid->getRowCount();
    for (row = 0; row < count; ++row)
        m_tablelist_grid->setCellBoolean(row, ONOFF_IDX, true);

    m_tablelist_grid->refreshColumn(kcl::Grid::refreshAll, ONOFF_IDX);
}

void DlgConnection::onTableListSelectNone(wxCommandEvent& evt)
{
    if (m_tablelist_grid->isEditing())
        m_tablelist_grid->endEdit(true);

    int row, count = m_tablelist_grid->getRowCount();
    for (row = 0; row < count; ++row)
        m_tablelist_grid->setCellBoolean(row, ONOFF_IDX, false);

    m_tablelist_grid->refreshColumn(kcl::Grid::refreshAll, ONOFF_IDX);
}


void DlgConnection::onOK(wxCommandEvent& evt)
{
    if (m_current_page == pageFile)
    {
        m_ci.type = xd::dbtypeFilesystem;
        m_ci.port = 0;
        m_ci.path = m_file_panel->getPath().ToStdWstring();
    }

    if (m_current_page == pageDataSource)
    {
        int row = m_datasource_grid->getCursorRow();
        if (row >= 0 && row < m_datasource_grid->getRowCount())
        {
            m_ci.type = xd::dbtypeOdbc;
            m_ci.database = m_datasource_grid->getCellString(row, 0);
        }
         else
        {
            return;
        }
    }


    saveDialogData();

    sigFinished(this);

    if (IsModal())
    {
        EndModal(wxID_OK);
    }
     else
    {
        Destroy();
    }
}
    
void DlgConnection::onCancel(wxCommandEvent& evt)
{
    if (IsModal())
    {
        EndModal(wxID_CANCEL);
    }
     else
    {
        Destroy();
    }
}

void DlgConnection::onToggleButton(wxCommandEvent& evt)
{
    switch (evt.GetId())
    {
        case ID_ToggleButton_Folder:     setActivePage(pageFolder); break;
        case ID_ToggleButton_File:       setActivePage(pageFile); break;
        case ID_ToggleButton_Server:     setActivePage(pageServer); break;
        case ID_ToggleButton_DataSource: setActivePage(pageDataSource); break;
    }
}

void DlgConnection::onBackward(wxCommandEvent& evt)
{
    setActivePage(m_last_page);
}


void DlgConnection::onForward(wxCommandEvent& evt)
{
    bool connect_to_database = false;

    if (m_current_page == pageServer)
    {
        connect_to_database = true;
    }

    if (m_current_page == pageFile)
    {
        m_need_text_format = false;

        std::vector<wxString> files;
        m_file_panel->getPaths(files);


        if (files.size() == 0)
        {
            // the path was empty
            appMessageBox(_("A valid location needs to be specified to continue."),
                          _("Invalid Location"),
                          wxOK | wxICON_EXCLAMATION | wxCENTER,
                          g_app->getMainWindow());
            return;
        }

        bool binary_import = m_binarycopy_checkbox->GetValue();


        if (!binary_import && files.size() == 1 && kl::icontains(files[0].ToStdWstring(), L".mdb"))
        {
            connect_to_database = true;
            m_ci.type = xd::dbtypeAccess;
            m_ci.port = 0;
            m_ci.path = files[0];
        }
         else if (!binary_import && files.size() == 1 && kl::icontains(files[0].ToStdWstring(), L".kpg"))
        {
            connect_to_database = true;
            m_ci.type = xd::dbtypeKpg;
            m_ci.port = 0;
            m_ci.path = files[0];
        }
         else
        {
            m_ci.path = L"";

            // binary import only goes to the table list page,
            // since the objects will be copied byte for byte,
            // not imported as a table

            if (!binary_import)
            {
                std::vector<wxString>::iterator it;
                for (it = files.begin(); it != files.end(); ++it)
                {
                    std::wstring wstr = it->ToStdWstring();
                    if (kl::icontains(wstr, L".icsv"))
                        continue; // known file type
                    if (kl::icontains(wstr, L".dbf"))
                        continue; // known file type
                    if (kl::icontains(wstr, L".xlsx"))
                        continue; // known file type
                    
                    m_need_text_format = true;
                    break;
                }
            }

            m_ci.tables.clear();
            m_ci.port = 0;
            m_ci.type = xd::dbtypeFilesystem;
            populateTableListGrid(files);

            setActivePage(pageTableList);
            return;
        }
    }

    if (m_current_page == pageTableList)
    {
        if (m_need_text_format)
            setActivePage(pageTextFormat);
    }


    if (m_current_page == pageDataSource)
    {
        int row = m_datasource_grid->getCursorRow();
        if (row >= 0 && row < m_datasource_grid->getRowCount())
        {
            m_ci.type = xd::dbtypeOdbc;
            m_ci.database = m_datasource_grid->getCellString(row, 0);


            xd::IDatabaseMgrPtr dbmgr = xd::getDatabaseMgr();
            if (dbmgr.isNull())
                return;

            xd::IDatabasePtr db = dbmgr->open(m_ci.getConnectionString());
            if (db.isNull())
            {
                wxString msg = _("There was an error connecting to the specified database.");
                msg += wxT("  ");
                msg += dbmgr->getErrorString();
            
                ::wxMessageBox(msg, _("Connection failed"),  wxOK | wxICON_EXCLAMATION | wxCENTER, wxTheApp->GetTopWindow());
                return;
            }

            populateTableListGrid(db);
            
            setActivePage(pageTableList);
        }
         else
        {
            return;
        }
    }
}



void DlgConnection::setActivePage(int page)
{
    // binary copy checkbox is only shown on the first page
    m_button_sizer->Show(m_binarycopy_checkbox, page == pageFile);


    m_last_page = m_current_page;
    m_current_page = page;

    if (page == pageFile)
    {
        m_container_sizer->Show(m_filepage_sizer);
        m_container_sizer->Hide(m_serverpage_sizer);
        m_container_sizer->Hide(m_datasourcepage_sizer);
        m_container_sizer->Hide(m_tablelistpage_sizer);
        m_container_sizer->Hide(m_textformatpage_sizer);

        if (m_options & optionFolder)
            showButtons(wxOK | wxCANCEL);
             else
            showButtons(wxFORWARD | wxCANCEL);
    }
     else if (page == pageServer)
    {
        m_container_sizer->Hide(m_filepage_sizer);
        m_container_sizer->Show(m_serverpage_sizer);
        m_container_sizer->Hide(m_datasourcepage_sizer);
        m_container_sizer->Hide(m_tablelistpage_sizer);
        m_container_sizer->Hide(m_textformatpage_sizer);

        if (m_options & optionFolder)
            showButtons(wxOK | wxCANCEL);
             else
            showButtons(wxFORWARD | wxCANCEL);

        m_server_server->SetFocus();
    }
     else if (page == pageDataSource)
    {
        m_container_sizer->Hide(m_filepage_sizer);
        m_container_sizer->Hide(m_serverpage_sizer);
        m_container_sizer->Show(m_datasourcepage_sizer);
        m_container_sizer->Hide(m_tablelistpage_sizer);
        m_container_sizer->Hide(m_textformatpage_sizer);

        if (m_options & optionFolder)
            showButtons(wxOK | wxCANCEL);
             else
            showButtons(wxFORWARD | wxCANCEL);
    }
     else if (page == pageTableList)
    {
        m_last_page = pageFile;
        m_container_sizer->Hide(m_filepage_sizer);
        m_container_sizer->Hide(m_serverpage_sizer);
        m_container_sizer->Hide(m_datasourcepage_sizer);
        m_container_sizer->Show(m_tablelistpage_sizer);
        m_container_sizer->Hide(m_textformatpage_sizer);
        showButtons(wxBACKWARD | (m_need_text_format ? wxFORWARD : wxOK) | wxCANCEL);
    }
     else if (page == pageTextFormat)
    {
        m_container_sizer->Hide(m_filepage_sizer);
        m_container_sizer->Hide(m_serverpage_sizer);
        m_container_sizer->Hide(m_datasourcepage_sizer);
        m_container_sizer->Hide(m_tablelistpage_sizer);
        m_container_sizer->Show(m_textformatpage_sizer);
        showButtons(wxBACKWARD | wxOK | wxCANCEL);
    }

    Layout();
}


void DlgConnection::showButtons(int mask)
{
    m_button_sizer->Show(m_ok_button, (mask & wxOK) ? true:false);
    m_button_sizer->Show(m_cancel_button, (mask & wxCANCEL) ? true:false);
    m_button_sizer->Show(m_backward_button, (mask & wxBACKWARD) ? true:false);
    m_button_sizer->Show(m_forward_button, (mask & wxFORWARD) ? true:false);
}


void DlgConnection::saveDialogData()
{
    // table selection page

    m_ci.tables.clear();

    wxString base_path = m_tablelist_basepath->GetValue();
    if (base_path == "/")
        base_path = "";
    if (base_path.Length() > 0 && base_path[0] != '/')
        base_path.Prepend("/");
    if (base_path.Length() > 0 && base_path.Last() != '/')
        base_path += "/";

    int row, row_count = m_tablelist_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        if (m_tablelist_grid->getCellBoolean(row, ONOFF_IDX))
        {
            ConnectionTable t;
            t.input_tablename = m_tablelist_grid->getCellString(row, SOURCE_TABLENAME_IDX);
            t.output_tablename = base_path + m_tablelist_grid->getCellString(row, DEST_TABLENAME_IDX);
            t.append = m_tablelist_grid->getCellBoolean(row, APPEND_IDX);
            m_ci.tables.push_back(t);
        }
    }

    m_ci.binary_copy = m_binarycopy_checkbox->GetValue();


    // text settings page
    if (m_need_text_format)
    {
        if (m_comma_radio->GetValue())
            m_ci.delimiters = L",";
        else if (m_tab_radio->GetValue())
            m_ci.delimiters = L"\t";
        else if (m_semicolon_radio->GetValue())
            m_ci.delimiters = L";";
        else if (m_pipe_radio->GetValue())
            m_ci.delimiters = L"|";
        else if (m_space_radio->GetValue())
            m_ci.delimiters = L" ";
        else if (m_nodelimiters_radio->GetValue())
            m_ci.delimiters = L"";
        else
            m_ci.delimiters = m_otherdelimiters_text->GetValue();

        if (m_doublequote_radio->GetValue())
            m_ci.text_qualifier = L"\"";
        else if (m_singlequote_radio->GetValue())
            m_ci.text_qualifier = L"'";
        else if (m_notextqualifier_radio->GetValue())
            m_ci.text_qualifier = L"";
        else if (m_othertextqualifier_radio->GetValue())
            m_ci.text_qualifier = m_othertextqualifier_text->GetValue();

        m_ci.first_row_header = m_firstrowheader_checkbox->GetValue();
    }
     else
    {
        m_ci.delimiters = L"";
        m_ci.text_qualifier = L"";
        m_ci.date_format_str = L"";
        m_ci.first_row_header = false;
    }
}

void DlgConnection::onFilePanelItemActivated(kcl::FilePanelEvent& evt)
{
    wxCommandEvent e;
    if (m_options & optionFolder)
        onOK(evt);
         else
        onForward(e);
}

void DlgConnection::onDataSourceLeftDClick(kcl::GridEvent& evt)
{
    if (evt.GetId() == ID_DataSource_Grid)
    {
        wxCommandEvent e;
        if (m_options & optionFolder)
            onOK(evt);
             else
            onForward(e);
    }
     else
    {
        evt.Skip();
    }
}

void DlgConnection::populateDataSourceGrid()
{
    xcm::ptr<xd::IDatabaseMgr> dbmgr;
    dbmgr.create_instance("xdodbc.DatabaseMgr");

    if (dbmgr.isNull())
    {
        wxString message = wxString::Format(_("This software is missing a required component (xdodbc) and must be reinstalled"));
        m_datasource_grid->setOverlayText(message);
        m_datasource_grid->SetFocus();
        return;
    }


    // populate the grid with the odbc data sources
    {
        xd::DatabaseEntryEnum odbc_databases;
        odbc_databases = dbmgr->getDatabaseList(L"", 0, L"", L"");

        m_datasource_grid->deleteAllRows();

        size_t i, count = odbc_databases.size();

        for (i = 0; i < count; ++i)
        {
            m_datasource_grid->insertRow(-1);

            m_datasource_grid->setCellString(i, 0, odbc_databases[i].name);
            m_datasource_grid->setCellString(i, 1, odbc_databases[i].description);
        }
    }


    // if we already have a server name, move the cursor
    // to the corresponding grid row

    int i, row_count = m_datasource_grid->getRowCount();
    std::wstring cell_str;

    for (i = 0; i < row_count; ++i)
    {
        cell_str = m_datasource_grid->getCellString(i, 0).ToStdWstring();

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



static void getTablesRecursive(xd::IDatabasePtr& db, const std::wstring& path, std::vector<std::wstring>& retval)
{
    xd::IFileInfoEnumPtr items = db->getFolderInfo(path);

    if (items.isNull())
        return;

    size_t i, count = items->size();
    int item_type;

    for (i = 0; i < count; ++i)
    {
        xd::IFileInfoPtr info = items->getItem(i);
        item_type = info->getType();

        // skip 'hidden' objects
        if (info->getName().substr(0,1) == '.')
            continue;

        if (item_type == xd::filetypeFolder)
        {
            std::wstring folder_path = path;
            folder_path += info->getName();
            folder_path += L"/";

            // recursively traverse this folder
            getTablesRecursive(db, folder_path, retval);
        }
         else if (item_type == xd::filetypeTable)
        {
            std::wstring name = path;
            
            if (name.length() == 0 || name[name.length()-1] != '/')
                name += L"/";
                
            name += info->getName();

            retval.push_back(name);
        }
    }
}



static ConnectionTable* lookupTable(Connection& c, const std::wstring& tbl)
{
    std::vector<ConnectionTable>::iterator it;
    for (it = c.tables.begin(); it != c.tables.end(); ++it)
    {
        if (kl::iequals(tbl, it->input_tablename))
            return &(*it);
        if (kl::iequals(tbl, L"/" + it->input_tablename))
            return &(*it);
    }

    return NULL;
}




void DlgConnection::populateTableListGrid(std::vector<wxString>& tables)
{
    std::vector<wxString>::iterator it;

    // delete all existing rows from grid
    m_tablelist_grid->deleteAllRows();

    // sort the table names vector
    std::sort(tables.begin(), tables.end());


    // populate the grid from the info vector
    int row = 0;
    for (it = tables.begin(); it != tables.end(); ++it)
    {
        bool selected = true;
        std::wstring src_tablename = towstr(*it);
        std::wstring out_tablename = kl::afterLast(src_tablename, PATH_SEPARATOR_CHAR);
        bool append = false;


        xd::IDatabasePtr& db = g_app->getDatabase();

        if (db->getDatabaseType() != xd::dbtypeFilesystem)
        {
            if (out_tablename.find('.') != out_tablename.npos)
                out_tablename = kl::beforeLast(out_tablename, '.');
            out_tablename = makeValidObjectName(out_tablename, db).ToStdWstring();
        }


        // correlate with the template
        if (m_ci.tables.size() > 0)
        {
            // tables already exist in the template.  "selected" is
            // by default off.  If the line exists in the template, use
            // the info stored there.
            selected = false;

            ConnectionTable* tbl = lookupTable(m_ci, src_tablename);
            if (tbl)
            {
                out_tablename = tbl->output_tablename;
                append = tbl->append;
                selected = true;
            }
        }
        
        m_tablelist_grid->insertRow(-1);
        m_tablelist_grid->setCellBoolean(row, ONOFF_IDX, selected);
        m_tablelist_grid->setCellString(row, SOURCE_TABLENAME_IDX, src_tablename);
        m_tablelist_grid->setCellString(row, DEST_TABLENAME_IDX, out_tablename);
        m_tablelist_grid->setCellBitmap(row, DEST_TABLENAME_IDX, GETBMP(xpm_blank_16));
        m_tablelist_grid->setCellBoolean(row, APPEND_IDX, append);
        row++;
    }

    m_tablelist_grid->refresh(kcl::Grid::refreshAll);
}

void DlgConnection::populateTableListGrid(xd::IDatabasePtr db)
{
    // delete all existing rows from grid
    m_tablelist_grid->deleteAllRows();

    if (db.isNull())
        return;

    std::vector<std::wstring> tables;
    std::vector<std::wstring>::iterator it;

    getTablesRecursive(db, L"", tables);

    // format oracle tables the way oracle users
    // would expect them: "owner.table_name"

    if (m_ci.type == xd::dbtypeOracle || m_ci.type == xd::dbtypeDb2)
    {
        for (it = tables.begin(); it != tables.end(); ++it)
        {
            std::wstring temps = kl::afterFirst(*it, '/');
            *it = kl::replaceStr(temps, L"/", L".");
        }
    }


    // sort the table names vector
    std::sort(tables.begin(), tables.end());


    // populate the grid from the info vector
    int row = 0;
    for (it = tables.begin(); it != tables.end(); ++it)
    {
        bool selected = true;
        std::wstring src_tablename = *it;
        std::wstring out_tablename;
        bool append = false;

        // remove slash from beginning
        if (kl::stringFrequency(src_tablename, '/') == 1 && src_tablename[0] == '/')
            src_tablename = src_tablename.substr(1);

        if (m_ci.type == xd::dbtypeOracle || m_ci.type == xd::dbtypeDb2)
            out_tablename = kl::afterLast(*it, '.');
             else
            out_tablename = kl::afterLast(*it, '/');

        xd::IDatabasePtr& db = g_app->getDatabase();

        if (db->getDatabaseType() != xd::dbtypeFilesystem)
        {
            if (out_tablename.find('.') != out_tablename.npos)
                out_tablename = kl::beforeLast(out_tablename, '.');
            out_tablename = makeValidObjectName(out_tablename, db).ToStdWstring();
        }

        // correlate with the template
        if (m_ci.tables.size() > 0)
        {
            // tables already exist in the template.  "selected" is
            // by default off.  If the line exists in the template, use
            // the info stored there.
            selected = false;

            ConnectionTable* tbl = lookupTable(m_ci, src_tablename);
            if (tbl)
            {
                out_tablename = tbl->output_tablename;
                append = tbl->append;
                selected = true;
            }
        }
        
        m_tablelist_grid->insertRow(-1);
        m_tablelist_grid->setCellBoolean(row, ONOFF_IDX, selected);
        m_tablelist_grid->setCellString(row, SOURCE_TABLENAME_IDX, src_tablename);
        m_tablelist_grid->setCellString(row, DEST_TABLENAME_IDX, out_tablename);
        m_tablelist_grid->setCellBitmap(row, DEST_TABLENAME_IDX, GETBMP(xpm_blank_16));
        m_tablelist_grid->setCellBoolean(row, APPEND_IDX, append);
        row++;
    }

    m_tablelist_grid->refresh(kcl::Grid::refreshAll);
}

