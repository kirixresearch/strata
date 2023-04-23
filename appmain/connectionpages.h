/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-02-27
 *
 */


#ifndef H_APP_CONNECTIONPAGES_H
#define H_APP_CONNECTIONPAGES_H


class ConnectionInfo;


class PathSelectionPage : public kcl::WizardPage
{    
public:

    enum ShowOptions
    {
        showPath = 0x01,
        showFilter = 0x02,
        showVersion = 0x04,
        showCompression = 0x08,
        showFileType = 0x10
    };
    
    // this enum order needs to match the order the items
    // are appended to the file type choice in the constructor
    enum FileTypeChoices
    {
        filetypeTextDelimited = 0,
        filetypeFixedLength,
        filetypeXbase,
        filetypeAccess,
        filetypeExcel,
        filetypePackage
    };

    PathSelectionPage(kcl::Wizard* parent, ConnectionInfo* ci);
    void setMessage(const wxString& message = wxEmptyString);       // wxEmptyString reverts to default
    void setPathLabel(const wxString& label = wxEmptyString);       // wxEmptyString reverts to default
    void setFileTypeLabel(const wxString& label = wxEmptyString);   // wxEmptyString reverts to default
    void setBrowse(bool browse_for_files = true,
                   bool filedialog_open = true,
                   bool filedialog_select_multiple = true);
    void setFileDialogFilter(const wxString& filter);
    
    wxString getPath();
    void setPath(const wxString& path);
    
    int getFileType();
    
    void showElements(unsigned int options);
    void loadPageData();
    void savePageData();

public: // signals

    xcm::signal0 sigPageChanged;
    xcm::signal2<bool  /* forward */,
                 bool* /* allow */> sigPageChanging;
    xcm::signal1<int   /* file type */> sigFileTypeChanged;

private:

    void resizeLabelWidths();
    
    void onPageChanged();
    bool onPageChanging(bool forward);

    // event handlers
    void onBrowse(wxCommandEvent& evt);
    void onPathTextEnter(wxCommandEvent& evt);
    void onPathTextChanged(wxCommandEvent& evt);
    void onFileTypeSelectionChanged(wxCommandEvent& evt);
    void onFilterSelectionChanged(wxCommandEvent& evt);
    void onVersionSelectionChanged(wxCommandEvent& evt);
    void onCompressionChecked(wxCommandEvent& evt);

private:

    ConnectionInfo* m_ci;

    wxBoxSizer* m_main_sizer;
    wxBoxSizer* m_path_sizer;
    wxBoxSizer* m_filetype_sizer;
    wxBoxSizer* m_filter_sizer;
    wxBoxSizer* m_version_sizer;
    wxBoxSizer* m_compression_sizer;

    wxStaticText* m_message;
    wxStaticText* m_path_label;
    wxStaticText* m_filetype_label;
    wxStaticText* m_filter_label;
    wxStaticText* m_version_label;
    wxTextCtrl* m_path_textctrl;
    wxChoice* m_filetype_choice;
    wxChoice* m_filter_choice;
    wxChoice* m_version_choice;
    wxCheckBox* m_compress_checkbox;

    ShowOptions m_show_options;
    wxString m_default_message;
    wxString m_default_path_label;
    wxString m_default_filetype_label;
    wxString m_filedialog_filter;
    bool m_filedialog_select_multiple;
    bool m_filedialog_open;
    bool m_browse_for_files;

    DECLARE_EVENT_TABLE()
};




class DataSourceSelectionPage : public kcl::WizardPage
{
public:

    DataSourceSelectionPage(kcl::Wizard* parent, ConnectionInfo* ci);
    void setMessage(const wxString& message);
    void savePageData();

private:

    void onPageChanged();
    bool onPageChanging(bool forward);

    void populate();

public: // signals

    xcm::signal0 sigPageChanged;
    xcm::signal2<bool     /* forward */,
                 bool*    /* allow */> sigPageChanging;

private:

    // event handlers
    void onGridCellLeftDClick(kcl::GridEvent& evt);

private:

    ConnectionInfo* m_ci;
    
    wxString m_default_message;
    wxStaticText* m_message;
    kcl::Grid* m_grid;

    DECLARE_EVENT_TABLE()
};




class ServerPropertiesPage : public kcl::WizardPage
{
public:
    
    enum ShowOptions
    {
        showServer       = 0x01,
        showDatabase     = 0x02,
        showUsername     = 0x04,
        showPassword     = 0x08,
        showPort         = 0x10,
        showSavePassword = 0x20,
        
        showAll = showServer |
                  showDatabase |
                  showUsername |
                  showPassword |
                  showPort |
                  showSavePassword
    };

    ServerPropertiesPage(kcl::Wizard* parent, ConnectionInfo* ci);
    void setMessage(const wxString& message = wxEmptyString);
    void showElements(unsigned int options);
    void loadPageData();
    void savePageData();

private:

    void handleFocus();

    void onPageChanged();
    bool onPageChanging(bool forward);
    
    // event handlers
    void onServerTextChanged(wxCommandEvent& evt);
    void onDatabaseTextChanged(wxCommandEvent& evt);
    void onPortTextChanged(wxCommandEvent& evt);
    void onUsernameTextChanged(wxCommandEvent& evt);
    void onPasswordTextChanged(wxCommandEvent& evt);
    void onSavePasswordChecked(wxCommandEvent& evt);

public: // signals

    xcm::signal0 sigPageChanged;
    xcm::signal2<bool     /* forward */,
                 bool*    /* allow */> sigPageChanging;

private:

    ConnectionInfo* m_ci;
    
    wxBoxSizer* m_main_sizer;
    wxBoxSizer* m_server_sizer;
    wxBoxSizer* m_database_sizer;
    wxBoxSizer* m_username_sizer;
    wxBoxSizer* m_password_sizer;
    wxBoxSizer* m_port_sizer;
    wxBoxSizer* m_savepassword_sizer;
    
    wxStaticText* m_message;
    wxTextCtrl* m_server_textctrl;
    wxTextCtrl* m_port_textctrl;
    wxTextCtrl* m_database_textctrl;
    wxTextCtrl* m_username_textctrl;
    wxTextCtrl* m_password_textctrl;
    wxCheckBox* m_savepassword_checkbox;

    ShowOptions m_show_options;
    wxString m_default_message;

    DECLARE_EVENT_TABLE()
};




class SavePage : public kcl::WizardPage
{
public:

    SavePage(kcl::Wizard* parent, ConnectionInfo* ci);
    void setMessage(const wxString& message);
    void setSaveButtonLabel(const wxString& label = wxEmptyString);

private:

    void onPageChanged();
    bool onPageChanging(bool forward);

    // event handlers
    void onSave(wxCommandEvent& evt);

public: // signals

    xcm::signal1<wxString /* ofs_path */> sigOnSave;

private:

    ConnectionInfo* m_ci;
    
    wxBoxSizer* m_main_sizer;
    wxBoxSizer* m_savebutton_sizer;
    
    wxStaticText* m_message;
    wxButton* m_save_button;

    DECLARE_EVENT_TABLE()
};



#endif  // __APP_CONNECTIONPAGES_H
