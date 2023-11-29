/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-02-26
 *
 */


#ifndef H_APP_PANELOPTIONS_H
#define H_APP_PANELOPTIONS_H


struct PrefInfo
{
    // general preferences
    wxString general_startup_location;
    wxString general_startup_connection;
    bool     general_startup_open_last_project;
    long     general_startup_action;
    long     general_toolbar_style;
    bool     general_check_for_updates;
    
    // internet preferences
    wxString internet_proxy_http;
    wxString internet_proxy_ftp;
    wxString internet_proxy_ssl;
    wxString internet_proxy_socks;
    long internet_proxy_http_port;
    long internet_proxy_ftp_port;
    long internet_proxy_ssl_port;
    long internet_proxy_socks_port;
    long internet_proxy_type;
    wxString default_web_browser;

    // internet preferences
    bool privacy_block_popups;
    bool privacy_accept_cookies;
    long privacy_cookie_lifetime_days;
    long privacy_cookie_lifetime_policy;

    // grid preferences
    bool     grid_horzlines_visible;
    bool     grid_vertlines_visible;
    bool     grid_greenbar_visible;
    bool     grid_text_wrap;
    bool     grid_boolean_checkbox;
    long     grid_row_height;
    long     grid_greenbar_interval;
    long     grid_font_ptsize;
    long     grid_font_family;
    long     grid_font_style;
    long     grid_font_weight;
    long     grid_font_encoding;
    bool     grid_font_underline;
    wxString grid_font_facename;
    wxColor  grid_background_color;
    wxColor  grid_foreground_color;
    wxColor  grid_line_color;
    wxColor  grid_invalid_area_color;
    wxColor  grid_greenbar_color;
    
    // report preferences (these values are in hundreths of an inch)
    long     report_page_width;
    long     report_page_height;
    long     report_margin_left;
    long     report_margin_right;
    long     report_margin_top;
    long     report_margin_bottom;
    long     report_font_ptsize;
    wxString report_font_style;
    wxString report_font_weight;
    wxString report_font_underscore;
    wxString report_font_facename;
    
    // script preferences
    bool script_show_syntax_highlight;
    bool script_show_line_numbers;
    bool script_show_whitespace;
    bool script_insert_spaces;
    long script_spaces_size;
    long script_tab_size;
};




class GeneralOptionsPage;
class InternetOptionsPage;
class PrivacyOptionsPage;
class DatasheetOptionsPage;
class ReportOptionsPage;
class ScriptOptionsPage;


class OptionsPanel : public wxPanel,
                     public IDocument
{

XCM_CLASS_NAME_NOREFCOUNT("appmain.OptionsPanel")
XCM_BEGIN_INTERFACE_MAP(OptionsPanel)
    XCM_INTERFACE_ENTRY(IDocument)
XCM_END_INTERFACE_MAP()

public:

    OptionsPanel();
    ~OptionsPanel();

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

private:

    void loadPrefs(PrefInfo* info);
    void savePrefs(PrefInfo* info);
    
    // event handlers
    
    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onApply(wxCommandEvent& evt);
    void onButtonBarClicked(wxCommandEvent& evt);
    void onRestoreDefaultsClicked(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);

public:

    void onComboTabPressed(wxKeyEvent& evt);

private:

    IDocumentSitePtr m_doc_site;
    PrefInfo* m_pi;

    // pages to show/hide in the sizer
    GeneralOptionsPage* m_general_page;
    InternetOptionsPage* m_internet_page;
    PrivacyOptionsPage* m_privacy_page;
    DatasheetOptionsPage* m_datasheet_page;
    ReportOptionsPage* m_report_page;
    ScriptOptionsPage* m_script_page;
    
    wxBoxSizer* m_sizer;
    
    DECLARE_EVENT_TABLE()
};


#endif  // __APP_PANELOPTIONS_H


