/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2013-12-09
 *
 */



#ifndef __APP_DLGCONNECTION_H
#define __APP_DLGCONNECTION_H




class ConnectionTable
{
public:

    bool append;
    std::wstring input_tablename;
    std::wstring output_tablename;
};



class Connection
{
public:

    Connection()
    {
        port = 3306;
        type = xd::dbtypeMySql;
    }

    std::wstring getConnectionString();

public:

    std::wstring path;

    int type;
    std::wstring server;
    int port;
    std::wstring database;
    std::wstring username;
    std::wstring password;

    std::wstring base_path;

    std::vector<ConnectionTable> tables;
};


class wxAuiToolBar;
class wxToggleButton;
class wxFileListCtrl;
namespace kcl { class Grid; }
namespace kcl { class RowSelectionGrid; }




class DlgConnection : public wxDialog
{

    enum
    {
        pageFile = 1,
        pageFolder,
        pageServer,
        pageDataSource,
        pageTextFormat,
        pageTableList
    };

public:

    DlgConnection(wxWindow* parent);
    ~DlgConnection();

    void setActivePage(int page);
    Connection& getConnectionInfo() { return m_ci; }

public: // signals
    xcm::signal1<DlgConnection*> sigFinished;

private:


    void populateDataSourceGrid();
    void populateTableListGrid(xd::IDatabasePtr);
    void showButtons(int mask);

    // event handlers
    
    void onServerParameterChanged(wxCommandEvent& evt);
    void onServerPasswordEnterPressed(wxCommandEvent& evt);

    void onTableListSelectAll(wxCommandEvent& evt);
    void onTableListSelectNone(wxCommandEvent& evt);

    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onBackward(wxCommandEvent& evt);
    void onForward(wxCommandEvent& evt);
    void onToggleButton(wxCommandEvent& evt);

private:
    
    wxAuiToolBar* m_toolbar;
    wxToggleButton* m_togglebutton_folder;
    wxToggleButton* m_togglebutton_server;
    wxToggleButton* m_togglebutton_datasources;

    int m_last_page;
    int m_current_page;

    wxSizer* m_container_sizer;
    wxSizer* m_filepage_sizer;
    wxSizer* m_serverpage_sizer;
    wxSizer* m_datasourcepage_sizer;
    wxSizer* m_tablelistpage_sizer;

    wxSizer* m_button_sizer;

    wxButton* m_ok_button;
    wxButton* m_cancel_button;
    wxButton* m_forward_button;
    wxButton* m_backward_button;

    // file page controls
    wxFileListCtrl* m_file_ctrl;
    
    // server page controls
    wxChoice* m_server_type;
    wxTextCtrl* m_server_server;
    wxTextCtrl* m_server_port;

    // data source page controls
    kcl::Grid* m_datasource_grid;

    // table list page controls
    kcl::RowSelectionGrid* m_tablelist_grid;
    wxTextCtrl* m_tablelist_basepath;

    Connection m_ci;

    std::vector<int> m_server_types;  // database types (xd::dbtype*) that have the same indexes as the combo box

    DECLARE_EVENT_TABLE()
};







#endif