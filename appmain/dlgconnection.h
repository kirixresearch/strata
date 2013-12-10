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




class ConnectionInfo2
{
public:

    ConnectionInfo2()
    {
        port = 0;
    }

    std::wstring server;
    int port;
    std::wstring database;
    std::wstring username;
    std::wstring password;

    std::wstring base_path;
};



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


private:


    void populateDataSourceGrid();
    void showButtons(int mask);

    // event handlers
    
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
    wxTextCtrl* m_server_server;

    // data source page controls
    kcl::Grid* m_datasource_grid;

    // table list page controls
    kcl::RowSelectionGrid* m_tablelist_grid;
    wxTextCtrl* m_tablelist_basepath;

    ConnectionInfo2 m_ci;

    DECLARE_EVENT_TABLE()
};





#endif
