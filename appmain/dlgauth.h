/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-04-28
 *
 */


#ifndef H_APP_DLGAUTH_H
#define H_APP_DLGAUTH_H


namespace paladin
{
    class Authentication;
};



class DlgAuth : public wxDialog
{
public:

    DlgAuth(wxWindow* parent,
            const wxString& product_name,
            const wxString& product_version,
            paladin::Authentication* global_auth);
    ~DlgAuth();

private:

    wxString getSiteCodeString();
    void updateValidControl();
    bool validateManualCode(const wxString& code,
                            bool save = false);
    
private:

    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onHelp(wxCommandEvent& evt);
    void onRadio(wxCommandEvent& evt);
    void onActivationCodeChanged(wxCommandEvent& evt);
    
private:
    
    wxRadioButton* m_internet_radio;
    wxRadioButton* m_manual_radio;
    wxTextCtrl* m_serial_textctrl;
    wxTextCtrl* m_sitecode_textctrl;
    wxTextCtrl* m_authcode_textctrl;
    kcl::ValidControl* m_activation_valid;
    
    paladin::Authentication* m_paladin_auth; // local auth object for quick testing
    paladin::Authentication* m_global_auth;  // app's g_auth object
    wxString m_product_name;
    wxString m_product_version;
    
    std::string m_company_regkey;
    std::string m_app_tag;
    std::string m_eval_tag;
    
#ifdef _MSC_VER
    __int64 m_site_code;
#else
    long long m_site_code;
#endif
    
    DECLARE_EVENT_TABLE()
};




#endif

