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

    std::wstring server;
    int port;
    std::wstring database;
    std::wstring username;
    std::wstring password;

};



class wxToggleButton;
class wxFileListCtrl;
namespace kcl { class Grid; }

class DlgConnection : public wxDialog
{

public:

    DlgConnection(wxWindow* parent);
    ~DlgConnection();


    void setActivePage(int page);


private:


    void populateDataSourceGrid();

    // event handlers
    
    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onToggleButton(wxCommandEvent& evt);

private:
    
    wxAuiToolBar* m_toolbar;
    wxToggleButton* m_togglebutton_folder;
    wxToggleButton* m_togglebutton_server;
    wxToggleButton* m_togglebutton_datasources;

    wxSizer* m_container_sizer;
    wxSizer* m_filepage_sizer;
    wxSizer* m_serverpage_sizer;
    wxSizer* m_datasourcepage_sizer;

    // file page controls
    wxFileListCtrl* m_file_ctrl;
    

    // data source page controls
    kcl::Grid* m_datasource_grid;


    ConnectionInfo2 m_ci;

    DECLARE_EVENT_TABLE()
};





#endif
