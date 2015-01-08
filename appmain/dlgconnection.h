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
        first_row_header = true;
        binary_copy = true;
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

    bool binary_copy;

    // text format settings
    std::wstring delimiters;
    std::wstring text_qualifier;
    std::wstring date_format_str;
    bool first_row_header;
};


class wxAuiToolBar;
class wxToggleButton;
class wxFileListCtrl;
namespace kcl { class Grid; }
namespace kcl { class GridEvent; }
namespace kcl { class RowSelectionGrid; }
namespace kcl { class FilePanel; }
namespace kcl { class FilePanelEvent; }


class DlgConnection : public wxDialog
{

public:

    enum
    {
        optionFolder = 0x01,
    };

private:

    enum
    {
        tabFile = 1,
        tabFolder,
        tabServer,
        tabDataSource
    };

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

    DlgConnection(wxWindow* parent, wxWindowID id, const wxString& title, int options = 0);
    ~DlgConnection();

    void setActivePage(int page);
    Connection& getConnectionInfo() { return m_ci; }

    wxString getFilePanelDirectory();
    void setFilePanelDirectory(const wxString& path);

public: // signals
    xcm::signal1<DlgConnection*> sigFinished;

private:

    void populateDataSourceGrid();
    void populateTableListGrid(xd::IDatabasePtr);
    void populateTableListGrid(std::vector<wxString>& paths);
    void showButtons(int mask);
    void saveDialogData();

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
    void onFilePanelItemActivated(kcl::FilePanelEvent& evt);
    void onDataSourceLeftDClick(kcl::GridEvent& evt);

private:
    
    wxAuiToolBar* m_toolbar;
    wxToggleButton* m_togglebutton_file;
    wxToggleButton* m_togglebutton_server;
    wxToggleButton* m_togglebutton_datasources;

    int m_last_page;
    int m_current_tab;
    int m_current_page;
    int m_options;
    bool m_need_text_format;

    wxSizer* m_container_sizer;
    wxSizer* m_filepage_sizer;
    wxSizer* m_serverpage_sizer;
    wxSizer* m_datasourcepage_sizer;
    wxSizer* m_tablelistpage_sizer;
    wxSizer* m_textformatpage_sizer;

    wxSizer* m_button_sizer;

    wxButton* m_ok_button;
    wxButton* m_cancel_button;
    wxButton* m_forward_button;
    wxButton* m_backward_button;

    // file page controls
    kcl::FilePanel* m_file_panel;
    
    // server page controls
    wxChoice* m_server_type;
    wxTextCtrl* m_server_server;
    wxTextCtrl* m_server_port;
    wxTextCtrl* m_server_database;
    wxTextCtrl* m_server_username;
    wxTextCtrl* m_server_password;

    // data source page controls
    kcl::Grid* m_datasource_grid;

    // table list page controls
    kcl::RowSelectionGrid* m_tablelist_grid;
    wxTextCtrl* m_tablelist_basepath;

    // text format page controls
    wxRadioButton* m_comma_radio;
    wxRadioButton* m_tab_radio;
    wxRadioButton* m_semicolon_radio;
    wxRadioButton* m_pipe_radio;
    wxRadioButton* m_space_radio;
    wxRadioButton* m_nodelimiters_radio;
    wxRadioButton* m_otherdelimiters_radio;
    wxTextCtrl*    m_otherdelimiters_text;
    wxRadioButton* m_doublequote_radio;
    wxRadioButton* m_singlequote_radio;
    wxRadioButton* m_notextqualifier_radio;
    wxRadioButton* m_othertextqualifier_radio;
    wxTextCtrl*    m_othertextqualifier_text;
    wxCheckBox*    m_firstrowheader_checkbox;
    wxCheckBox*    m_binarycopy_checkbox;


    Connection m_ci;

    std::vector<int> m_server_types;  // database types (xd::dbtype*) that have the same indexes as the combo box

    DECLARE_EVENT_TABLE()
};







#endif
