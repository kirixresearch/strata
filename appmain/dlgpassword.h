/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-09-29
 *
 */


#ifndef __APP_DLGPASSWORD_H
#define __APP_DLGPASSWORD_H





class DlgPassword : public wxDialog
{

    enum
    {
        ID_UsernameTextCtrl = wxID_HIGHEST+1,
        ID_PasswordTextCtrl,
        ID_ServiceTextCtrl
    };
    
public:

    DlgPassword(wxWindow* parent);
    ~DlgPassword();

    wxString getUserName() const { return m_username; }
    wxString getPassword() const { return m_password; }
    wxString getService() const { return m_service; }
    
    void setMessage(const wxString& message);
    void setUserName(const wxString& username);
    void setService(const wxString& service);
    
private:

    // event handlers
    
    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);

private:
    
    wxString m_message;
    wxString m_username;
    wxString m_password;
    wxString m_service;
    
    wxStaticText* m_message_ctrl;
    wxTextCtrl* m_username_ctrl;
    wxTextCtrl* m_password_ctrl;
    wxTextCtrl* m_service_ctrl;
    
    DECLARE_EVENT_TABLE()
};






#endif

