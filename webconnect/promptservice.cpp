/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  wxWebConnect Embedded Web Browser Control Library
 * Author:   Benjamin I. Williams
 * Created:  2006-10-07
 *
 */



#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/printdlg.h>
#include <string>
#include "webcontrol.h"
#include "nsinclude.h"
#include "promptservice.h"



///////////////////////////////////////////////////////////////////////////////
//  utilities
///////////////////////////////////////////////////////////////////////////////


wxWebControl* GetWebControlFromBrowserChrome(nsIWebBrowserChrome* chrome);

wxWebControl* GetWebControlFromDOMWindow(nsIDOMWindow* window)
{
    ns_smartptr<nsIWindowWatcher> window_watcher = nsGetWindowWatcherService();
    ns_smartptr<nsIWebBrowserChrome> chrome;
    
    if (window == NULL || window_watcher.empty())
    {
        // we don't have either a dom window pointer or
        // access to the window watcher service.  return error
        return NULL;
    }
    
    window_watcher->GetChromeForWindow(window, &chrome.p);
    
    return GetWebControlFromBrowserChrome(chrome);
}

wxWindow* GetTopFrameFromDOMWindow(nsIDOMWindow* window)
{
    wxWindow* win = GetWebControlFromDOMWindow(window);
    if (!win)
        return NULL;
        
    // now that we have a window, go up the window
    // hierarchy to find a frame
    
    wxWindow* w = win;
    while (1)
    {
        if (w->IsKindOf(CLASSINFO(wxFrame)))
            return w;
        
        wxWindow* old_win = w;
        w = w->GetParent();
        if (!w)
            return old_win;
    }
    
    return win;
}




///////////////////////////////////////////////////////////////////////////////
//  various dialogs
///////////////////////////////////////////////////////////////////////////////


class PromptDlgPassword : public wxDialog
{

    enum
    {
        ID_UsernameTextCtrl = wxID_HIGHEST+1,
        ID_PasswordTextCtrl
    };
    
public:

    PromptDlgPassword(wxWindow* parent)
                         : wxDialog(parent,
                                    -1,
                                    _("Authentication Required"),
                                    wxDefaultPosition,
                                    wxSize(400, 200),
                                    wxDEFAULT_DIALOG_STYLE |
                                    wxCENTER)
    {
        // create the username sizer
        
        wxStaticText* label_username = new wxStaticText(this,
                                                        -1,
                                                        _("User Name:"),
                                                        wxDefaultPosition,
                                                        wxDefaultSize);
        m_username_ctrl = new wxTextCtrl(this, ID_UsernameTextCtrl, m_username);
        
        wxBoxSizer* username_sizer = new wxBoxSizer(wxHORIZONTAL);
        username_sizer->Add(label_username, 0, wxALIGN_CENTER);
        username_sizer->Add(m_username_ctrl, 1, wxEXPAND);
        
        
        // create the password sizer
        
        wxStaticText* label_password = new wxStaticText(this,
                                                        -1,
                                                        _("Password:"),
                                                        wxDefaultPosition,
                                                        wxDefaultSize);
        m_password_ctrl = new wxTextCtrl(this,
                                         ID_PasswordTextCtrl,
                                         wxEmptyString,
                                         wxDefaultPosition,
                                         wxDefaultSize,
                                         wxTE_PASSWORD);
        
        wxBoxSizer* password_sizer = new wxBoxSizer(wxHORIZONTAL);
        password_sizer->Add(label_password, 0, wxALIGN_CENTER);
        password_sizer->Add(m_password_ctrl, 1, wxEXPAND);


        // create a platform standards-compliant OK/Cancel sizer
        
        wxButton* ok_button = new wxButton(this, wxID_OK);
        wxButton* cancel_button = new wxButton(this, wxID_CANCEL);
        
        wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
        ok_cancel_sizer->AddButton(ok_button);
        ok_cancel_sizer->AddButton(cancel_button);
        ok_cancel_sizer->Realize();
        ok_cancel_sizer->AddSpacer(5);
        
        ok_button->SetDefault();
        
        // this code is necessary to get the sizer's bottom margin to 8
        wxSize min_size = ok_cancel_sizer->GetMinSize();
        min_size.SetHeight(min_size.GetHeight()+16);
        ok_cancel_sizer->SetMinSize(min_size);
        
        
        // code to allow us to line up the static text elements
        wxSize s1 = label_username->GetSize();
        wxSize s2 = label_password->GetSize();
        wxSize max_size = wxSize(wxMax(s1.x, s2.x), wxMax(s1.y, s2.y));
        max_size.x += 10;
        username_sizer->SetItemMinSize(label_username, max_size);
        password_sizer->SetItemMinSize(label_password, max_size);


        // create username/password sizer
        
        wxBitmap bmp = wxArtProvider::GetBitmap(wxART_QUESTION, wxART_MESSAGE_BOX);
        wxStaticBitmap* bitmap_question = new wxStaticBitmap(this, -1, bmp);
        m_message_ctrl = new wxStaticText(this, -1, m_message);
        
        wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
        vert_sizer->Add(m_message_ctrl, 0, wxEXPAND);
        vert_sizer->AddSpacer(16);
        vert_sizer->Add(username_sizer, 0, wxEXPAND);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(password_sizer, 0, wxEXPAND);
        
        // create top sizer
        
        wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
        top_sizer->AddSpacer(7);
        top_sizer->Add(bitmap_question, 0, wxTOP, 7);
        top_sizer->AddSpacer(15);
        top_sizer->Add(vert_sizer, 1, wxEXPAND | wxTOP, 7);

        // create main sizer
        
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->AddSpacer(8);
        main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
        main_sizer->AddStretchSpacer();
        main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);

        SetSizer(main_sizer);
        Layout();
    }
    
    ~PromptDlgPassword()
    {
        // clear out password in memory
        m_password = wxT("            ");
    }

    wxString GetUserName() const
    {
        return m_username;
    }
    
    wxString GetPassword() const
    {
        return m_password;
    }
    
    void SetMessage(const wxString& message)
    {
        m_message = message;
        m_message_ctrl->SetLabel(m_message);
        wxSizer* sizer = m_message_ctrl->GetContainingSizer();
        m_message_ctrl->Wrap(sizer->GetSize().GetWidth());
        Layout();
    }
    
    void SetUserName(const wxString& username)
    {
        m_username = username;
    }
    
    
private:

    // event handlers
    
    void OnOK(wxCommandEvent& evt)
    {
        m_username = m_username_ctrl->GetValue();
        m_password = m_password_ctrl->GetValue();
        
        EndModal(wxID_OK);
    }
    
    void OnCancel(wxCommandEvent& evt)
    {
        m_username = wxT("");
        m_password = wxT("");
        
        EndModal(wxID_CANCEL);
    }

private:
    
    wxString m_message;
    wxString m_username;
    wxString m_password;
    
    wxStaticText* m_message_ctrl;
    wxTextCtrl* m_username_ctrl;
    wxTextCtrl* m_password_ctrl;
    
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(PromptDlgPassword, wxDialog)
    EVT_BUTTON(wxID_OK, PromptDlgPassword::OnOK)
    EVT_BUTTON(wxID_CANCEL, PromptDlgPassword::OnCancel)
END_EVENT_TABLE()






/*
class PromptDlgBadCert : public wxDialog
{

    enum
    {
        ID_UsernameTextCtrl = wxID_HIGHEST+1,
        ID_PasswordTextCtrl
    };
    
public:

    PromptDlgBadCert(wxWindow* parent)
                         : wxDialog(parent,
                                    -1,
                                    _("Authentication Required"),
                                    wxDefaultPosition,
                                    wxSize(400, 200),
                                    wxDEFAULT_DIALOG_STYLE |
                                    wxCENTER)
    {
        // create a platform standards-compliant OK/Cancel sizer
        
        wxButton* yes_button = new wxButton(this, wxID_YES);
        wxButton* no_button = new wxButton(this, wxID_NO);
        
        wxStdDialogButtonSizer* yes_no_sizer = new wxStdDialogButtonSizer;
        yes_no_sizer->AddButton(yes_button);
        yes_no_sizer->AddButton(no_button);
        yes_no_sizer->Realize();
        yes_no_sizer->AddSpacer(5);
        
        no_button->SetDefault();
        
        // this code is necessary to get the sizer's bottom margin to 8
        wxSize min_size = yes_no_sizer->GetMinSize();
        min_size.SetHeight(min_size.GetHeight()+16);
        yes_no_sizer->SetMinSize(min_size);
        
        
        // create username/password sizer
        
        wxBitmap bmp = wxArtProvider::GetBitmap(wxART_QUESTION, wxART_MESSAGE_BOX);
        wxStaticBitmap* bitmap_question = new wxStaticBitmap(this, -1, bmp);
        m_message_ctrl = new wxStaticText(this, -1, m_message);
        
        wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
        vert_sizer->Add(m_message_ctrl, 0, wxEXPAND);
        vert_sizer->AddSpacer(16);

        // create top sizer
        
        wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
        top_sizer->AddSpacer(7);
        top_sizer->Add(bitmap_question, 0, wxTOP, 7);
        top_sizer->AddSpacer(15);
        top_sizer->Add(vert_sizer, 1, wxEXPAND | wxTOP, 7);

        // create main sizer
        
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->AddSpacer(8);
        main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
        main_sizer->AddStretchSpacer();
        main_sizer->Add(yes_no_sizer, 0, wxEXPAND);

        SetSizer(main_sizer);
        Layout();
    }
    
    ~PromptDlgBadCert()
    {
    }

    void SetMessage(const wxString& message)
    {
        m_message = message;
        m_message_ctrl->SetLabel(m_message);
        wxSizer* sizer = m_message_ctrl->GetContainingSizer();
        m_message_ctrl->Wrap(sizer->GetSize().GetWidth());
        Layout();
    }

private:

    // event handlers
    
    void OnYes(wxCommandEvent& evt)
    {
        EndModal(wxID_YES);
    }
    
    void OnNo(wxCommandEvent& evt)
    {
        EndModal(wxID_NO);
    }

private:
    
    wxString m_message;
    
    wxStaticText* m_message_ctrl;
    wxCheckBox* m_username_ctrl;
    
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(PromptDlgBadCert, wxDialog)
    EVT_BUTTON(wxID_YES, PromptDlgBadCert::OnYes)
    EVT_BUTTON(wxID_NO, PromptDlgBadCert::OnNo)
END_EVENT_TABLE()
*/












///////////////////////////////////////////////////////////////////////////////
//  PromptService class implementation
///////////////////////////////////////////////////////////////////////////////


class PromptService : public nsIPrompt,
                      public nsIPromptService2,
                      public nsIBadCertListener,
                      public nsIBadCertListener2,
                      public nsIAuthPrompt2
{
public:

    PromptService();
    virtual ~PromptService();

    void onBadCertificate(const wxString& message, nsIDOMWindow* dom_window);
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROMPTSERVICE
    NS_DECL_NSIPROMPTSERVICE2
    NS_DECL_NSIBADCERTLISTENER
    NS_DECL_NSIBADCERTLISTENER2
    NS_DECL_NSIAUTHPROMPT2
    
    
    NS_IMETHOD Alert(const char16_t* dialog_title, const char16_t *text)
    {
        return this->Alert(m_parent, dialog_title, text);
    }

    NS_IMETHOD AlertCheck(const char16_t* dialog_title, const char16_t* text, const char16_t* check_msg, bool* check_state)
    {
        return this->AlertCheck(m_parent, dialog_title, text, check_msg, check_state);
    }

    NS_IMETHOD Confirm(const char16_t* dialog_title, const char16_t* text, bool* _retval)
    {
        return this->Confirm(m_parent, dialog_title, text, _retval);
    }

    NS_IMETHOD ConfirmCheck(const char16_t* dialog_title, const char16_t* text, const char16_t* check_msg, bool* check_state, bool* _retval)
    {
        return this->ConfirmCheck(m_parent, dialog_title, text, check_msg, check_state, _retval);
    }

    NS_IMETHOD ConfirmEx(const char16_t* dialog_title, const char16_t* text, PRUint32 button_flags, const char16_t* button0_title, const char16_t* button1_title, const char16_t* button2_title, const char16_t* check_msg, bool* check_state, PRInt32* _retval)
    {
        return this->ConfirmEx(m_parent, dialog_title, text, button_flags, button0_title, button1_title, button2_title, check_msg, check_state, _retval);
    }

    NS_IMETHOD Prompt(const char16_t* dialog_title, const char16_t* text, char16_t** value, const char16_t* check_msg, bool* check_state, bool* _retval)
    {
        return this->Prompt(m_parent, dialog_title, text, value, check_msg, check_state, _retval);
    }

    NS_IMETHOD PromptPassword(const char16_t* dialog_title, const char16_t* text, char16_t** password, const char16_t* check_msg, bool* check_state, bool* _retval)
    {
        return this->PromptPassword(m_parent, dialog_title, text, password, check_msg, check_state, _retval);
    }

    NS_IMETHOD PromptUsernameAndPassword(const char16_t* dialog_title, const char16_t* text, char16_t** username, char16_t** password, const char16_t* check_msg, bool* check_state, bool *_retval)
    {
        return this->PromptUsernameAndPassword(m_parent, dialog_title, text, username, password, check_msg, check_state, _retval);
    }

    NS_IMETHOD Select(const char16_t* dialog_title, const char16_t* text, PRUint32 count, const char16_t** select_list, PRInt32 *out_selection, bool *_retval)
    {
        return this->Select(m_parent, dialog_title, text, count, select_list, out_selection, _retval);
    }
    
    nsIDOMWindow* m_parent;
};


NS_IMPL_ADDREF(PromptService)
NS_IMPL_RELEASE(PromptService)

NS_INTERFACE_MAP_BEGIN(PromptService)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPromptService)
    NS_INTERFACE_MAP_ENTRY(nsIPrompt)
    NS_INTERFACE_MAP_ENTRY(nsIPromptService)
    NS_INTERFACE_MAP_ENTRY(nsIPromptService2)
    NS_INTERFACE_MAP_ENTRY(nsIBadCertListener)
    NS_INTERFACE_MAP_ENTRY(nsIBadCertListener2)
    NS_INTERFACE_MAP_ENTRY(nsIAuthPrompt2)
NS_INTERFACE_MAP_END


PromptService::PromptService()
{
    m_parent = NULL;
}

PromptService::~PromptService()
{
}

NS_IMETHODIMP PromptService::Alert(nsIDOMWindow* parent,
                                   const char16_t* ns_dialog_title,
                                   const char16_t* ns_text)
{
    wxString title = ns2wx(ns_dialog_title);
    wxString text = ns2wx(ns_text);


    if (text.Find(wxT("ssl_error_")) != -1 ||
        text.Find(wxT("sec_error_")) != -1)
    {
        onBadCertificate(text, parent);
        return NS_OK;
    }


    wxMessageBox(text,
                 title,
                 wxOK,
                 GetTopFrameFromDOMWindow(parent));
                 
    return NS_OK;
}

NS_IMETHODIMP PromptService::AlertCheck(
                                    nsIDOMWindow* parent,
                                    const char16_t* ns_dialog_title,
                                    const char16_t* ns_text,
                                    const char16_t* ns_check_msg,
                                    bool* checkValue)
{
    wxString dialog_title = ns2wx(ns_dialog_title);
    wxString text = ns2wx(ns_text);
    wxString check_msg = ns2wx(ns_check_msg);
    
    return NS_OK;
}

NS_IMETHODIMP PromptService::Confirm(
                                    nsIDOMWindow* parent,
                                    const char16_t* ns_dialog_title,
                                    const char16_t* ns_text,
                                    bool* retval)
{
    wxString dialog_title = ns2wx(ns_dialog_title);
    wxString text = ns2wx(ns_text);
    
    int res = wxMessageBox(text,
             dialog_title,
             wxYES_NO,
             GetTopFrameFromDOMWindow(parent));
    
    if (!retval)
        return NS_ERROR_NULL_POINTER;
    
    if (res == wxYES)
        *retval = PR_TRUE;
         else
        *retval = PR_FALSE;
           
    return NS_OK;
}

NS_IMETHODIMP PromptService::ConfirmCheck(
                                    nsIDOMWindow* parent,
                                    const char16_t* dialog_title,
                                    const char16_t* text,
                                    const char16_t* check_msg,
                                    bool* check_value,
                                    bool* retval)
{
    return NS_OK;
}

NS_IMETHODIMP PromptService::ConfirmEx(
                                    nsIDOMWindow* parent,
                                    const char16_t* dialog_title,
                                    const char16_t* text,
                                    PRUint32 buttonFlags,
                                    const char16_t* button0_title,
                                    const char16_t* button1_title,
                                    const char16_t* button2_title,
                                    const char16_t* check_msg,
                                    bool* check_value,
                                    PRInt32* button_pressed)
{
    return NS_OK;
}

NS_IMETHODIMP PromptService::Prompt(
                                    nsIDOMWindow* parent,
                                    const char16_t* _dialog_title,
                                    const char16_t* _text,
                                    char16_t** _value,
                                    const char16_t* check_msg,
                                    bool* check_value,
                                    bool* retval)
{
    // check message and check value aren't implemented yet
    
    wxString dialog_title = ns2wx(_dialog_title);
    wxString text = ns2wx(_text);
    wxString value = ns2wx(*_value);
    
    wxTextEntryDialog dlg(GetTopFrameFromDOMWindow(parent),
                          text,
                          dialog_title,
                          value,
                          wxOK | wxCANCEL | wxCENTER);

    int res = dlg.ShowModal();
    if (res == wxID_OK)
    {
        *_value = wxToUnichar(dlg.GetValue());
        *retval = PR_TRUE;
    }
     else
    {
        *retval = PR_FALSE;
    }

    return NS_OK;
}

NS_IMETHODIMP PromptService::PromptUsernameAndPassword(
                                    nsIDOMWindow* parent,
                                    const char16_t* dialog_title,
                                    const char16_t* text,
                                    char16_t** username,
                                    char16_t** password,
                                    const char16_t* check_msg,
                                    bool* check_value,
                                    bool* retval)
{
    // this version is used by xulrunner 1.8.x -- see below for the newer version
    wxWindow* wxparent = GetTopFrameFromDOMWindow(parent);
    
    PromptDlgPassword dlg(wxparent);
    dlg.SetMessage(ns2wx(text));
    
    int res = dlg.ShowModal();
    if (res == wxID_OK)
    {
        *username = wxToUnichar(dlg.GetUserName());
        *password = wxToUnichar(dlg.GetPassword());
        *retval = PR_TRUE;
    }
     else
    {
        *retval = PR_FALSE;
    }
    
    return NS_OK;
}

NS_IMETHODIMP PromptService::PromptPassword(
                                    nsIDOMWindow* parent,
                                    const char16_t* dialog_title,
                                    const char16_t* text,
                                    char16_t** password,
                                    const char16_t* check_msg,
                                    bool* check_value,
                                    bool* retval)
{
    return NS_OK;
}

NS_IMETHODIMP PromptService::Select(nsIDOMWindow* parent,
                                    const char16_t* dialog_title,
                                    const char16_t* text,
                                    PRUint32 count,
                                    const char16_t** select_list,
                                    PRInt32* out_selection,
                                    bool* retval)
{
    *retval = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP PromptService::ConfirmUnknownIssuer(
                                    nsIInterfaceRequestor* socketInfo,
                                    nsIX509Cert* cert,
                                    PRInt16* certAddType,
                                    bool* retval)
{
    if (wxWebControl::GetIgnoreCertErrors())
    {
        *retval = PR_TRUE;
        return NS_OK;
    }

    int res = wxMessageBox(
        _("The requested web page is certified by an unknown authority.  Would you like to continue?"),
        _("Website Certified by an Unknown Authority"),
        wxICON_QUESTION | wxCENTER | wxYES_NO,
        NULL);
        
    if (res != wxYES)
    {
        *retval = PR_FALSE;
        return NS_OK;
    }
        
    *certAddType = nsIBadCertListener::ADD_TRUSTED_FOR_SESSION;
    *retval = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP PromptService::PromptAuth(nsIDOMWindow* parent,
                                        nsIChannel* channel,
                                        PRUint32 level,
                                        nsIAuthInformation* auth_info,
                                        const char16_t* checkbox_label,
                                        bool* check_value,
                                        bool* retval)
{
    // this version used by newer >= 1.9.1 versions of xulrunner
    wxWindow* wxparent = GetTopFrameFromDOMWindow(parent);
    
    PromptDlgPassword dlg(wxparent);
    dlg.SetMessage(_("Please enter a username and password:"));
    
    int res = dlg.ShowModal();
    if (res == wxID_OK)
    {
        nsEmbedString ns_username;
        nsEmbedString ns_password;

        wx2ns(dlg.GetUserName(), ns_username);
        wx2ns(dlg.GetPassword(), ns_password);
        
        auth_info->SetUsername(ns_username);
        auth_info->SetPassword(ns_password);
        
        *retval = PR_TRUE;
    }
     else
    {
        *retval = PR_FALSE;
    }
    
    return NS_OK;
}

NS_IMETHODIMP PromptService::AsyncPromptAuth(nsIDOMWindow* parent,
                                        nsIChannel* channel,
                                        nsIAuthPromptCallback* callbck,
                                        nsISupports* contxt,
                                        PRUint32 level,
                                        nsIAuthInformation* auth_info,
                                        const char16_t* checkbox_label,
                                        bool* check_value,
                                        nsICancelable** retval)
{
    return NS_OK;
}



NS_IMETHODIMP PromptService::ConfirmMismatchDomain(
                                    nsIInterfaceRequestor* socketInfo,
                                    const nsACString& targetURL,
                                    nsIX509Cert *cert,
                                    bool* retval)
{
    if (wxWebControl::GetIgnoreCertErrors())
    {
        *retval = PR_TRUE;
        return NS_OK;
    }

    int res = wxMessageBox(
        _("The requested web page uses a certificate which does not match the domain.  Would you like to continue?"),
        _("Domain Mismatch"),
        wxICON_QUESTION | wxCENTER | wxYES_NO,
        NULL);
        
    if (res != wxYES)
    {
        *retval = PR_FALSE;
        return NS_OK;
    }
    
    *retval = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP PromptService::ConfirmCertExpired(
                                    nsIInterfaceRequestor* socketInfo,
                                    nsIX509Cert* cert,
                                    bool* retval)
{
    if (wxWebControl::GetIgnoreCertErrors())
    {
        *retval = PR_TRUE;
        return NS_OK;
    }

    int res = wxMessageBox(
        _("The requested web page uses a certificate which has expired.  Would you like to continue?"),
        _("Certificate Expired"),
        wxICON_QUESTION | wxCENTER | wxYES_NO,
        NULL);
    
    if (res != wxYES)
    {
        *retval = PR_FALSE;
        return NS_OK;
    }
    
    *retval = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP PromptService::NotifyCrlNextupdate(
                                    nsIInterfaceRequestor* socketInfo,
                                    const nsACString& targetURL,
                                    nsIX509Cert* cert)
{
    return NS_OK;
}

NS_IMETHODIMP PromptService::NotifyCertProblem(
                                    nsIInterfaceRequestor *socket_info,
                                    nsISSLStatus *status,
                                    const nsACString& target_site,
                                    bool *retval)
{
    if (wxWebControl::GetIgnoreCertErrors())
    {
        *retval = PR_TRUE;
        return NS_OK;
    }

    *retval = PR_TRUE;
    return NS_OK;
}

void PromptService::onBadCertificate(const wxString& message, nsIDOMWindow* dom_window)
{
    wxString msg_text = message;
    msg_text += wxT("\n");
    msg_text += _("Would you like to accept this certificate and continue?");
    
    int res = wxMessageBox(msg_text,
             _("Secure Connection Warning"),
             wxYES_NO,
             GetTopFrameFromDOMWindow(dom_window));

    if (res != wxYES)
        return;


    wxWebControl* ctrl = GetWebControlFromDOMWindow(dom_window);
    if (!ctrl)
        return;

    ns_smartptr<nsIURIFixup> uri_fixup = nsGetService("@mozilla.org/docshell/urifixup;1");
    if (!uri_fixup)
        return;
    
    wxString load_uri = ctrl->GetCurrentLoadURI();
    
    ns_smartptr<nsIURI> uri;
    nsEmbedCString load_uri_text;
    wx2ns(load_uri, load_uri_text);
    uri_fixup->CreateFixupURI(load_uri_text, 0, NULL, &uri.p);

    if (!uri)
        return;
    
    nsEmbedCString ns_host;
    PRInt32 ns_port = 0;
    uri->GetAsciiHost(ns_host);
    uri->GetPort(&ns_port);
    if (ns_port <= 0)
        ns_port = 443;
    
    ns_smartptr<nsIX509CertDB> cert_db = nsGetService("@mozilla.org/security/x509certdb;1");
    if (cert_db.empty())
        return;

    ns_smartptr<nsIRecentBadCerts> recent_bad_certs;

    cert_db->GetRecentBadCerts(false, &recent_bad_certs.p);
    if (recent_bad_certs.empty())
        return;


    wxString wx_host_port = ns2wx(ns_host);
    wx_host_port += wxString::Format(wxT(":%d"), ns_port);
        
    nsEmbedString ns_host_port;
    wx2ns(wx_host_port, ns_host_port);

    ns_smartptr<nsISSLStatus> status;
    recent_bad_certs->GetRecentBadCert(ns_host_port, &status.p);

    if (status.empty())
        return;

    ns_smartptr<nsIX509Cert> cert;
    status->GetServerCert(&cert.p);
    if (cert.empty())
        return;


    ns_smartptr<nsICertOverrideService> cert_override = nsGetService("@mozilla.org/security/certoverride;1");
    if (cert_override)
    {
        bool is_untrusted, is_domain_mismatch, is_not_valid_at_this_time;
                    
        status->GetIsUntrusted(&is_untrusted);
        status->GetIsDomainMismatch(&is_domain_mismatch);
        status->GetIsNotValidAtThisTime(&is_not_valid_at_this_time);
                
        unsigned int flags = 0;
        if (is_untrusted)
            flags |= nsICertOverrideService::ERROR_UNTRUSTED;
        if (is_domain_mismatch)
            flags |= nsICertOverrideService::ERROR_MISMATCH;
        if (is_not_valid_at_this_time)
            flags |= nsICertOverrideService::ERROR_TIME;

        cert_override->RememberValidityOverride(ns_host, ns_port, cert, flags, false /* remember it permanently */);
                    
        ctrl->OpenURI(load_uri);
    }
}



// nsIAuthPrompt2 implementation

NS_IMETHODIMP PromptService::PromptAuth(nsIChannel* channel,
                                        PRUint32 level,
                                        nsIAuthInformation* auth_info,
                                        bool* _retval)
{
    return this->PromptAuth(m_parent, channel, level, auth_info, NULL, NULL, _retval);
}

NS_IMETHODIMP PromptService::AsyncPromptAuth(
                                        nsIChannel* channel,
                                        nsIAuthPromptCallback* callback,
                                        nsISupports* context,
                                        PRUint32 level,
                                        nsIAuthInformation* auth_info,
                                        nsICancelable** _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}












///////////////////////////////////////////////////////////////////////////////
//  PromptFactory class implementation
///////////////////////////////////////////////////////////////////////////////


class PromptFactory : public nsIFactory,
                      public nsIPromptFactory
{
public:
    
    PromptFactory()
    {
        NS_INIT_ISUPPORTS()
    }
    
    virtual ~PromptFactory()
    {
    }
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIFACTORY
    NS_DECL_NSIPROMPTFACTORY
};

NS_IMPL_ADDREF(PromptFactory)
NS_IMPL_RELEASE(PromptFactory)

NS_INTERFACE_MAP_BEGIN(PromptFactory)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIFactory)
    NS_INTERFACE_MAP_ENTRY(nsIFactory)
    NS_INTERFACE_MAP_ENTRY(nsIPromptFactory)
NS_INTERFACE_MAP_END



void CreatePromptServiceFactory(nsIFactory** result)
{
    PromptFactory* obj = new PromptFactory;
    obj->AddRef();
    *result = static_cast<nsIFactory*>(obj);
}

NS_IMETHODIMP PromptFactory::CreateInstance(nsISupports* outer,
                                            const nsIID& iid,
                                            void** result)
{
    nsresult res;
    
    if (!result)
        return NS_ERROR_NULL_POINTER;
        
    if (outer)
        return NS_ERROR_NO_AGGREGATION;
    
    PromptFactory* obj = new PromptFactory;
    if (!obj)
        return NS_ERROR_OUT_OF_MEMORY;
        
    obj->AddRef();
    res = obj->QueryInterface(iid, result);
    obj->Release();
    
    return res;
}

NS_IMETHODIMP PromptFactory::LockFactory(bool lock)
{
    return NS_OK;
}

NS_IMETHODIMP PromptFactory::GetPrompt(nsIDOMWindow *parent,
                                       const nsIID& iid,
                                       void** result)
{
    nsresult res;
        
    PromptService* obj = new PromptService;
    obj->m_parent = parent;
    
    obj->AddRef();
    res = obj->QueryInterface(iid, result);
    obj->Release();
    
    return res;
}




///////////////////////////////////////////////////////////////////////////////
//  PrintintPromptService class implementation
///////////////////////////////////////////////////////////////////////////////



class PrintingPromptService : public nsIPrintingPromptService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPRINTINGPROMPTSERVICE

    PrintingPromptService() { }
    virtual ~PrintingPromptService() { }

};

NS_IMPL_ISUPPORTS1(PrintingPromptService, nsIPrintingPromptService)


NS_IMETHODIMP PrintingPromptService::ShowPrintDialog(nsIDOMWindow *parent,
                                                     nsIWebBrowserPrint* webBrowserPrint,
                                                     nsIPrintSettings* settings)
{
    static wxPrintDialogData wxdatain;

    wxdatain.EnableSelection(true);
    wxdatain.EnablePageNumbers(true);
    
    wxPrintDialog dlg(GetTopFrameFromDOMWindow(parent), &wxdatain);
    if (dlg.ShowModal() == wxID_CANCEL)
        return NS_ERROR_ABORT;
        
    wxPrintDialogData& wxdata = dlg.GetPrintDialogData();
    wxPrintData& wxpdata = wxdata.GetPrintData();
    
    // save the info for next time
    wxdatain = wxdata;
    wxdatain = wxpdata;
    
    settings->SetNumCopies(wxdata.GetNoCopies());
    settings->SetPrintToFile(wxdata.GetPrintToFile() ? PR_TRUE : PR_FALSE);
    settings->SetPrinterName(wxToUnichar(wxpdata.GetPrinterName()));

    if (wxpdata.GetOrientation() == wxPORTRAIT)
        settings->SetOrientation(nsIPrintSettings::kPortraitOrientation);
    else if (wxpdata.GetOrientation() == wxLANDSCAPE)
        settings->SetOrientation(nsIPrintSettings::kLandscapeOrientation);

    if (wxdata.GetAllPages())
    {
        settings->SetPrintRange(nsIPrintSettings::kRangeAllPages);
    }
     else if (wxdata.GetSelection())
    {
        settings->SetPrintRange(nsIPrintSettings::kRangeSelection);
    }
     else
    {
        settings->SetPrintRange(nsIPrintSettings::kRangeSpecifiedPageRange);
        settings->SetStartPageRange(wxdata.GetFromPage());
        settings->SetEndPageRange(wxdata.GetToPage());
    }
    
    
    switch (wxpdata.GetPaperId())
    {
        case wxPAPER_LETTER: settings->SetPaperName(wxToUnichar(wxT("Letter"))); break;
        case wxPAPER_LEGAL: settings->SetPaperName(wxToUnichar(wxT("Legal"))); break;
        case wxPAPER_EXECUTIVE: settings->SetPaperName(wxToUnichar(wxT("Executive"))); break;
        case wxPAPER_A3: settings->SetPaperName(wxToUnichar(wxT("A3"))); break;
        case wxPAPER_A4: settings->SetPaperName(wxToUnichar(wxT("A4"))); break;
        case wxPAPER_A5: settings->SetPaperName(wxToUnichar(wxT("A5"))); break;
        default:
            // don't set paper type
            break;
    }
    
    
    return NS_OK;
}

NS_IMETHODIMP PrintingPromptService::ShowProgress(nsIDOMWindow* parent,
                                                  nsIWebBrowserPrint* webBrowserPrint,
                                                  nsIPrintSettings* printSettings,
                                                  nsIObserver* openDialogObserver,
                                                  bool isForPrinting,
                                                  nsIWebProgressListener** webProgressListener,
                                                  nsIPrintProgressParams** printProgressParams,
                                                  bool* notifyOnOpen)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP PrintingPromptService::ShowPageSetup(nsIDOMWindow* parent,
                                                   nsIPrintSettings* printSettings,
                                                   nsIObserver* aObs)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP PrintingPromptService::ShowPrinterProperties(nsIDOMWindow* parent,
                                                           const char16_t* printerName,
                                                           nsIPrintSettings* printSettings)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}




///////////////////////////////////////////////////////////////////////////////
//  PrintingPromptFactory class implementation
///////////////////////////////////////////////////////////////////////////////


class PrintingPromptFactory : public nsIFactory
{
public:

    NS_DECL_ISUPPORTS
    
    PrintingPromptFactory()
    {
        NS_INIT_ISUPPORTS();
    }
    
    NS_IMETHOD CreateInstance(nsISupports* outer,
                              const nsIID& iid,
                              void** result)
    {
        nsresult res;
        
        if (!result)
            return NS_ERROR_NULL_POINTER;
            
        if (outer)
            return NS_ERROR_NO_AGGREGATION;
        

        PrintingPromptService* obj = new PrintingPromptService();
        if (!obj)
            return NS_ERROR_OUT_OF_MEMORY;
        obj->AddRef();
        res = obj->QueryInterface(iid, result);
        obj->Release();

        return res;
    }
    
    NS_IMETHOD LockFactory(bool lock)
    {
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS1(PrintingPromptFactory, nsIFactory);


void CreatePrintingPromptFactory(nsIFactory** result)
{
    PrintingPromptFactory* obj = new PrintingPromptFactory;
    obj->AddRef();
    *result = obj;
}






///////////////////////////////////////////////////////////////////////////////
//  TransferService class implementation
///////////////////////////////////////////////////////////////////////////////


class TransferService : public nsITransfer
{
public:

    NS_DECL_ISUPPORTS

    TransferService()
    {
    }

    NS_IMETHODIMP Init(nsIURI* source,
                       nsIURI* target,
                       const nsAString& display_name,
                       nsIMIMEInfo* mime_info,
                       PRTime start_time,
                       nsILocalFile* temp_file,
                       nsICancelable* cancelable)
    {
        return NS_OK;
    }

    NS_IMETHOD OnStateChange(nsIWebProgress* web_progress,
                             nsIRequest* request,
                             PRUint32 state_flags,
                             nsresult status)
    {
        return NS_OK;
    }

    NS_IMETHOD OnProgressChange(nsIWebProgress* web_progress,
                                nsIRequest* request,
                                PRInt32 cur_self_progress,
                                PRInt32 max_self_progress,
                                PRInt32 cur_total_progress,
                                PRInt32 max_total_progress)
    {
        return OnProgressChange64(web_progress,
                                  request,
                                  cur_self_progress,
                                  max_self_progress,
                                  cur_total_progress,
                                  max_total_progress);
    }
    
    NS_IMETHOD OnProgressChange64(
                                 nsIWebProgress* web_progress,
                                 nsIRequest* request,
                                 PRInt64 cur_self_progress,
                                 PRInt64 max_self_progress,
                                 PRInt64 cur_total_progress,
                                 PRInt64 max_total_progress)
    {
       return NS_OK;
    }
    
    NS_IMETHOD OnLocationChange(
                             nsIWebProgress* web_progress,
                             nsIRequest* request,
                             nsIURI* location,
                             unsigned int flags)
    {
       return NS_OK;
    }

    NS_IMETHOD OnStatusChange(
                             nsIWebProgress* web_progress,
                             nsIRequest* request,
                             nsresult status,
                             const char16_t* message)
    {
        return NS_OK;
    }


    NS_IMETHOD OnSecurityChange(
                             nsIWebProgress* web_progress,
                             nsIRequest* request,
                             PRUint32 state)
    {
       return NS_OK;
    }
    
    NS_IMETHOD OnRefreshAttempted(
                        nsIWebProgress* web_progress,
                        nsIURI* refresh_uri,
                        PRInt32 millis,
                        bool same_uri,
                        bool *retval)
    {
        return NS_OK;
    }
};

NS_IMPL_ADDREF(TransferService)
NS_IMPL_RELEASE(TransferService)

NS_INTERFACE_MAP_BEGIN(TransferService)
    NS_INTERFACE_MAP_ENTRY(nsISupports)
    NS_INTERFACE_MAP_ENTRY(nsITransfer)
    NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener2)
    NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
NS_INTERFACE_MAP_END



///////////////////////////////////////////////////////////////////////////////
//  TransferFactory class implementation
///////////////////////////////////////////////////////////////////////////////


class TransferFactory : public nsIFactory
{
public:

    NS_DECL_ISUPPORTS
    
    TransferFactory()
    {
        NS_INIT_ISUPPORTS();
    }
    
    NS_IMETHOD CreateInstance(nsISupports* outer,
                              const nsIID& iid,
                              void** result)
    {
        nsresult res;
        
        if (!result)
            return NS_ERROR_NULL_POINTER;
            
        if (outer)
            return NS_ERROR_NO_AGGREGATION;
        

        TransferService* obj = new TransferService();
        if (!obj)
            return NS_ERROR_OUT_OF_MEMORY;
        obj->AddRef();
        res = obj->QueryInterface(iid, result);
        obj->Release();

        return res;
    }
    
    NS_IMETHOD LockFactory(bool lock)
    {
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS1(TransferFactory, nsIFactory);


void CreateTransferFactory(nsIFactory** result)
{
    TransferFactory* obj = new TransferFactory;
    obj->AddRef();
    *result = obj;
}




///////////////////////////////////////////////////////////////////////////////
//  UnknownContentTypeHandler class implementation
///////////////////////////////////////////////////////////////////////////////


class UnknownContentTypeHandler : public nsIHelperAppLauncherDialog
{
public:

    NS_DECL_ISUPPORTS

    NS_IMETHOD Show(nsIHelperAppLauncher* launcher,
                    nsISupports* _context,
                    PRUint32 reason)
    {     
        ns_smartptr<nsISupports> context = _context;
        ns_smartptr<nsIDOMWindow> parent = nsRequestInterface(context);
        wxWebControl* ctrl = GetWebControlFromDOMWindow(parent);
        if (!ctrl)
        {
            // nobody to handle event, default action
            // is to save file to disk
            
            // BIW 7 May 2007 - this was causing save as dialogs to appear
            // during page loads.. we'll do nothing here instead
            return NS_OK;
        }
        
        
        wxString url;
        ns_smartptr<nsIURI> uri;
        launcher->GetSource(&uri.p);
        if (uri)
        {
            nsEmbedCString ns_spec;
            if (NS_SUCCEEDED(uri->GetSpec(ns_spec)))
                url = ns2wx(ns_spec);
        }
        
        
        
        nsEmbedString ns_filename;
        launcher->GetSuggestedFileName(ns_filename);
        wxString filename = ns2wx(ns_filename);
        
        
        // fetch mime type
        nsEmbedCString ns_mimetype;
        ns_smartptr<nsISupports> mime_info_supports;
        launcher->GetMIMEInfo((nsIMIMEInfo**)&mime_info_supports.p);
        wxString mime_type;
        

        ns_smartptr<nsIMIMEInfo> mime_info = mime_info_supports;
        if (mime_info)
        {
            mime_info->GetMIMEType(ns_mimetype);
            mime_type = ns2wx(ns_mimetype);
        }        
        
        
        // setup event object
        wxWebEvent evt(wxEVT_WEB_INITDOWNLOAD, ctrl->GetId());
        evt.SetEventObject(ctrl);
        evt.SetFilename(filename);
        evt.SetContentType(mime_type);
        evt.SetHref(url);
        bool handled = ctrl->GetEventHandler()->ProcessEvent(evt);

        if (handled)
        {
        
            switch (evt.m_download_action)
            {
                case wxWEB_DOWNLOAD_SAVE:
                    launcher->SaveToDisk(nsnull, PR_FALSE);
                    break;
                case wxWEB_DOWNLOAD_SAVEAS:
                    wxASSERT_MSG(evt.m_download_action_path.Length() > 0, wxT("You must specify a filename in the event object"));
                    if (evt.m_download_action_path.IsEmpty())
                    {
                        // no filename specified
                        launcher->Cancel(0x804b0002 /* = NS_BINDING_ABORTED */ );
                        return NS_OK;
                    }
                     else
                    {
                        std::string fname = (const char*)evt.m_download_action_path.mbc_str();
                        
                        nsILocalFile* filep = NULL;
                        NS_NewNativeLocalFile(nsDependentCString(fname.c_str()), PR_TRUE, &filep);

                        launcher->SaveToDisk(filep, PR_FALSE);

                        if (filep)
                            filep->Release();
                    }
                    break;
                case wxWEB_DOWNLOAD_OPEN:
                    launcher->LaunchWithApplication(NULL, PR_FALSE);
                    break;
                case wxWEB_DOWNLOAD_CANCEL:
                    launcher->Cancel(0x804b0002 /* = NS_BINDING_ABORTED */ );
                    break;
            }
            
            
            if (evt.m_download_listener)
            {
                evt.m_download_listener->Init(url, evt.m_download_action_path);
                nsIWebProgressListener* progress = CreateProgressListenerAdaptor(evt.m_download_listener);
                
                launcher->SetWebProgressListener((nsIWebProgressListener2*)progress);
                
                progress->Release();
            }
        
        }
         else
        {
            launcher->SaveToDisk(nsnull, PR_FALSE);
            return NS_OK;
/*
            OpenOrSaveDlg dlg(GetTopFrameFromDOMWindow(parent), filename);
            int result = dlg.ShowModal();
            
            switch (result)
            {
                case wxID_OPEN:
                    break;
                case wxID_SAVE:
                    launcher->SaveToDisk(nsnull, PR_FALSE);
                    break;
                case wxID_CANCEL:
                    launcher->Cancel(0x804b0002 ); // = NS_BINDING_ABORTED
                    return NS_OK;
            }
            */
        }
            
        return NS_OK;
    }
    
    NS_IMETHOD PromptForSaveToFile(nsIHelperAppLauncher* launcher,
                                   nsISupports* window_context,
                                   const char16_t* default_file,
                                   const char16_t* suggested_file_extension,
                                   bool force_prompt,
                                   nsILocalFile** new_file)
    {
        ns_smartptr<nsISupports> context = window_context;
        ns_smartptr<nsIDOMWindow> parent = nsRequestInterface(context);
        
        wxString default_filename = ns2wx(default_file);
        
        wxString filter;
        filter += _("All Files");
        filter += wxT(" (*.*)|*.*|");
        filter.RemoveLast(); // get rid of the last pipe sign

        wxFileDialog dlg(GetTopFrameFromDOMWindow(parent),
                         _("Save As"),
                         wxT(""),
                         default_filename,
                         filter,
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        
        if (dlg.ShowModal() != wxID_OK)
        {
            return NS_ERROR_FAILURE;
        }
        
        
        std::string fname = (const char*)dlg.GetPath().mbc_str();

        NS_NewNativeLocalFile(nsDependentCString(fname.c_str()), PR_TRUE, new_file);
        return NS_OK;
    }
};


NS_IMPL_ADDREF(UnknownContentTypeHandler)
NS_IMPL_RELEASE(UnknownContentTypeHandler)

NS_INTERFACE_MAP_BEGIN(UnknownContentTypeHandler)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIHelperAppLauncherDialog)
    NS_INTERFACE_MAP_ENTRY(nsIHelperAppLauncherDialog)
NS_INTERFACE_MAP_END









///////////////////////////////////////////////////////////////////////////////
//  UnknownContentTypeHandlerFactory class implementation
///////////////////////////////////////////////////////////////////////////////


class UnknownContentTypeHandlerFactory : public nsIFactory
{
public:
    NS_DECL_ISUPPORTS
    
    UnknownContentTypeHandlerFactory()
    {
        NS_INIT_ISUPPORTS();
    }
    
    NS_IMETHOD CreateInstance(nsISupports* outer,
                              const nsIID& iid,
                              void** result)
    {
        nsresult res;
        
        if (!result)
            return NS_ERROR_NULL_POINTER;
            
        if (outer)
            return NS_ERROR_NO_AGGREGATION;
        
        UnknownContentTypeHandler* obj = new UnknownContentTypeHandler;
        if (!obj)
            return NS_ERROR_OUT_OF_MEMORY;
            
        obj->AddRef();
        res = obj->QueryInterface(iid, result);
        obj->Release();
        
        return res;
    }
    
    NS_IMETHOD LockFactory(bool lock)
    {
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS1(UnknownContentTypeHandlerFactory, nsIFactory);


void CreateUnknownContentTypeHandlerFactory(nsIFactory** result)
{
    UnknownContentTypeHandlerFactory* obj = new UnknownContentTypeHandlerFactory;
    obj->AddRef();
    *result = obj;
}















class CertOverrideService : public nsICertOverrideService
{
public:

    NS_DECL_ISUPPORTS


    CertOverrideService()
    {
    }

    virtual ~CertOverrideService()
    {
    }

    NS_IMETHOD RememberValidityOverride(const nsACString& host_name,
                                        PRInt32 port, nsIX509Cert* cert,
                                        PRUint32 override_bits,
                                        bool temporary)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD HasMatchingOverride(const nsACString& host_name,
                                   PRInt32 port,
                                   nsIX509Cert* cert,
                                   PRUint32* override_bits,
                                   bool* is_temporary,
                                   bool *_retval)
    {
        *override_bits = 3;
        *_retval = PR_TRUE;
        return NS_OK;
    }

    NS_IMETHOD GetValidityOverride(const nsACString& host_name,
                                   PRInt32 port,
                                   nsACString& hash_alg,
                                   nsACString& fingerprint,
                                   PRUint32* override_bits,
                                   bool* is_temporary,
                                   bool* _retval)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD ClearValidityOverride(const nsACString& host_name,
                                     PRInt32 port)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD GetAllOverrideHostsWithPorts(PRUint32* count,
                                            char16_t*** hosts_with_ports_array)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD IsCertUsedForOverrides(nsIX509Cert *cert,
                                      bool check_temporaries,
                                      bool check_permanents,
                                      PRUint32 *_retval)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }
};


NS_IMPL_ISUPPORTS1(CertOverrideService, nsICertOverrideService);





///////////////////////////////////////////////////////////////////////////////
//  CertOverrideFactory class implementation
///////////////////////////////////////////////////////////////////////////////


class CertOverrideFactory : public nsIFactory
{
public:
    NS_DECL_ISUPPORTS
    
    CertOverrideFactory()
    {
        NS_INIT_ISUPPORTS();
    }
    
    NS_IMETHOD CreateInstance(nsISupports* outer,
                              const nsIID& iid,
                              void** result)
    {
        nsresult res;
        
        if (!result)
            return NS_ERROR_NULL_POINTER;
            
        if (outer)
            return NS_ERROR_NO_AGGREGATION;
        
        CertOverrideService* obj = new CertOverrideService;
        if (!obj)
            return NS_ERROR_OUT_OF_MEMORY;
            
        obj->AddRef();
        res = obj->QueryInterface(iid, result);
        obj->Release();
        
        return res;
    }
    
    NS_IMETHOD LockFactory(bool lock)
    {
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS1(CertOverrideFactory, nsIFactory);


void CreateCertOverrideFactory(nsIFactory** result)
{
    CertOverrideFactory* obj = new CertOverrideFactory;
    obj->AddRef();
    *result = obj;
}


