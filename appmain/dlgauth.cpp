/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-04-28
 *
 */


#include "appmain.h"
#include "../paladin/paladin.h"
#include "dlgauth.h"
#include "inetauth.h"
#include <wx/artprov.h>

#ifdef WIN32
#include <mapi.h>
#endif

extern paladin::Authentication* g_auth;


// -- utility functions --


static wxString getActivationStatusText(paladin::Authentication* auth)
{
    int res = auth->checkAuth();
    wxString serial = getAppPrefsString(wxT("license_mgr.login"));
    
    // the user has move the system clock back; authorization failed
    if (res == paladin::errClockModified || res == paladin::errClockUnsynced)
        return _("Expired");
    
    // autorization failed and there was a serial
    // number associated with the application
    if (res == paladin::errAuthFailed && serial.Length() > 0)
        return _("Expired");
    
    // format the "days left" string
    wxString days_left_str;
    int days_left = auth->getDaysLeft();
    if (days_left == 1)
        days_left_str += wxString::Format(_("(%d day left)"), days_left);
         else
        days_left_str += wxString::Format(_("(%d days left)"), days_left);
    
    // authorization succeeded
    if (res == paladin::errNone)
    {
        if (auth->isPerpetual())
            return _("Activated");
        
        if (days_left >= 0)
        {
            wxDateTime dt = wxDateTime::Today();
            dt += wxDateSpan::Days(days_left);
            
            wxString retval = _("Expires");
            retval += wxT(" ");
            retval += dt.Format(wxT("%B %d, %Y"));
            retval += wxT(" ");
            retval += days_left_str;
            return retval;
        }
        
        return _("Expired");
    }
    
    // authorization failed and there was no serial
    // number associated with the application
    wxString retval = _("Not Activated");
    retval += wxT(" ");
    retval += days_left_str;
    return retval;
}




class DlgAuthHelp : public wxDialog
{
public:

    DlgAuthHelp(wxWindow* parent)
                    : wxDialog(parent, -1, _("Software Activation Help"),
                         wxDefaultPosition,
                         wxDefaultSize,
                         wxDEFAULT_DIALOG_STYLE |
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxCLIP_CHILDREN |
                         wxCENTER |
                         wxRESIZE_BORDER)
    {
        wxString appname = APPLICATION_NAME;
        wxString support_telno = APP_CONTACT_SUPPORTTELNO;
        wxString support_email = APP_CONTACT_SUPPORTEMAIL;
        wxString account_url = APP_WEBLOCATION_ACCOUNT;
        
        wxString str1 =  wxString::Format(_("Thank you for installing %s.  Prior to working with the software, it must first be activated."),
                                         appname.c_str());
        wxString str2 = _("Activate Automatically");
        wxString str3 = _("Activating automatically over the Internet is the easiest way to get your software up and running.  Simply enter the serial code (provided in your email confirmation) and then click the OK button.  Your software will be activated over the Internet and you'll be able to begin using the software immediately.");
        wxString str4 = _("Activate Manually");
        wxString str5 = wxString::Format(_("Sometimes it is difficult to activate automatically, which is often caused by restrictions set up by corporate firewalls.  If you find yourself in this situation, you can still activate %s using a manually-generated activation code."),
                                         appname.c_str());
        wxString str6 = wxString::Format(_("To obtain an Activation Code manually, login to your online account (%s) and follow the instructions provided.  Once you have your activation code, enter it and click the OK button to begin using the software."),
                                         account_url.c_str());
        wxString str7 = _("Further Help");
        wxString str8 = wxString::Format(_("If you have any questions or need any help activating %s, please contact our support team at %s or %s."),
                                         appname.c_str(),
                                         support_telno.c_str(),
                                         support_email.c_str());

        wxStaticText* label1 = new wxStaticText(this, -1, str1);
        wxStaticText* label2 = new wxStaticText(this, -1, str2);
        wxStaticText* label3 = new wxStaticText(this, -1, str3);
        wxStaticText* label4 = new wxStaticText(this, -1, str4);
        wxStaticText* label5 = new wxStaticText(this, -1, str5);
        wxStaticText* label6 = new wxStaticText(this, -1, str6);
        wxStaticText* label7 = new wxStaticText(this, -1, str7);
        wxStaticText* label8 = new wxStaticText(this, -1, str8);

        makeFontBold(label2);
        makeFontBold(label4);
        makeFontBold(label7);
        limitFontSize(label2, 12);
        limitFontSize(label4, 12);
        limitFontSize(label5, 12);

        // create vertical sizer
        
        wxBitmap help_bmp = wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MESSAGE_BOX);
        wxStaticBitmap* bmp = new wxStaticBitmap(this, -1, help_bmp);

        wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
        vert_sizer->Add(label1, 0, wxEXPAND);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(label2, 0, wxEXPAND | wxTOP | wxRIGHT, 12);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(label3, 0, wxEXPAND | wxRIGHT, 12);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(label4, 0, wxEXPAND | wxTOP | wxRIGHT, 12);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(label5, 0, wxEXPAND | wxRIGHT, 12);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(label6, 0, wxEXPAND | wxRIGHT, 12);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(label7, 0, wxEXPAND | wxTOP | wxRIGHT, 12);
        vert_sizer->AddSpacer(8);
        vert_sizer->Add(label8, 0, wxEXPAND | wxRIGHT, 12);
        
        
        // create top sizer
        
        wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
        top_sizer->AddSpacer(7);
        top_sizer->Add(bmp, 0, wxTOP, 7);
        top_sizer->AddSpacer(15);
        top_sizer->Add(vert_sizer, 1, wxEXPAND | wxTOP, 7);
        
        
        // -- create a platform standards-compliant OK/Cancel sizer --
        
        wxButton* ok_button = new wxButton(this, wxID_OK);
        wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
        ok_cancel_sizer->AddButton(ok_button);
        ok_cancel_sizer->Realize();
        ok_cancel_sizer->AddSpacer(5);
        
        // -- this code is necessary to get the sizer's bottom margin to 8 --
        wxSize min_size = ok_cancel_sizer->GetMinSize();
        min_size.SetHeight(min_size.GetHeight()+16);
        ok_cancel_sizer->SetMinSize(min_size);
        
        
        // create main sizer
        wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
        main_sizer->AddSpacer(8);
        main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
        main_sizer->AddStretchSpacer();
        main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
        SetSizer(main_sizer);
        Layout();
        
        resizeStaticText(label1, label1->GetClientSize().GetWidth());
        resizeStaticText(label2, label2->GetClientSize().GetWidth());
        resizeStaticText(label3, label3->GetClientSize().GetWidth());
        resizeStaticText(label4, label4->GetClientSize().GetWidth());
        resizeStaticText(label5, label5->GetClientSize().GetWidth());
        resizeStaticText(label6, label6->GetClientSize().GetWidth());
        resizeStaticText(label7, label7->GetClientSize().GetWidth());
        resizeStaticText(label8, label8->GetClientSize().GetWidth());
        Center();
        
        // set the ok button as the default
        ok_button->SetDefault();

        SetClientSize(main_sizer->GetMinSize().Scale(1.2f, 1.0));
        SetMinSize(GetSize());
        CenterOnScreen();
    }
};




enum
{
    ID_Internet_Radio = wxID_HIGHEST + 1,
    ID_Manual_Radio,
    ID_ActivationCode
};


BEGIN_EVENT_TABLE(DlgAuth, wxDialog)
    EVT_BUTTON(wxID_OK, DlgAuth::onOK)
    EVT_BUTTON(wxID_CANCEL, DlgAuth::onCancel)
    EVT_BUTTON(wxID_HELP, DlgAuth::onHelp)
    EVT_RADIOBUTTON(ID_Internet_Radio, DlgAuth::onRadio)
    EVT_RADIOBUTTON(ID_Manual_Radio, DlgAuth::onRadio)
    EVT_TEXT(ID_ActivationCode, DlgAuth::onActivationCodeChanged)
END_EVENT_TABLE()


DlgAuth::DlgAuth(wxWindow* parent,
                 const wxString& product_name,
                 const wxString& product_version,
                 paladin::Authentication* global_auth)
                    : wxDialog(parent, -1, _("Software Activation"),
                         wxDefaultPosition,
                         wxDefaultSize,
                         wxDEFAULT_DIALOG_STYLE |
                         wxNO_FULL_REPAINT_ON_RESIZE |
                         wxCLIP_CHILDREN |
                         wxCENTER |
                         wxRESIZE_BORDER)
{
    //SetMinSize(wxSize(480,360));
    //SetMaxSize(wxSize(640,480));
    
    #ifdef __WXGTK__
    SetSize(wxSize(560,400));
    SetMinSize(wxSize(560,400));
    #endif
    
    // -- create paladin authentication object --
    
    m_site_code = global_auth->getSiteCode();
    m_company_regkey = global_auth->getCompanyRegKey();
    m_app_tag = global_auth->getAppTag();
    m_eval_tag = global_auth->getEvalTag();
    m_global_auth = global_auth;
    
    m_product_name = product_name;
    m_product_version = product_version;
    m_paladin_auth = paladin::createAuthObject(m_company_regkey,
                                               m_app_tag,
                                               m_eval_tag,
                                               paladin::modeLocal);
    
    m_manual_radio = new wxRadioButton(this, ID_Manual_Radio, _("Activate manually"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_internet_radio = new wxRadioButton(this, ID_Internet_Radio, _("Activate automatically"));


    // -- create authentication code sizer --
    
    wxStaticText* label_authcode = new wxStaticText(this, -1, _("Activation Code:"));
    m_authcode_textctrl = new wxTextCtrl(this, ID_ActivationCode);
    
    m_activation_valid = new kcl::ValidControl(this, -1,
                                               wxDefaultPosition,
                                               wxSize(16,16));
    m_activation_valid->setValidBitmap(GETBMP(gf_checkmark_16));
    m_activation_valid->setInvalidBitmap(GETBMP(gf_x_16));
    m_activation_valid->setBorderVisible(false);
    m_activation_valid->setLabelVisible(false);
    
    wxBoxSizer* authcode_sizer = new wxBoxSizer(wxHORIZONTAL);
    authcode_sizer->Add(label_authcode, 0, wxALIGN_CENTER | wxLEFT, 30);
    authcode_sizer->Add(m_authcode_textctrl, 1, wxALIGN_CENTER);
    authcode_sizer->AddSpacer(5);
    authcode_sizer->Add(m_activation_valid, 0, wxALIGN_CENTER);
    
    
    // -- create site code sizer --
    
    wxStaticText* label_sitecode = new wxStaticText(this, -1, _("Site Code:"));
    m_sitecode_textctrl = new wxTextCtrl(this,
                                         -1,
                                         getSiteCodeString(),
                                         wxDefaultPosition,
                                         wxDefaultSize,
                                         wxTE_READONLY);
    
    wxBoxSizer* sitecode_sizer = new wxBoxSizer(wxHORIZONTAL);
    sitecode_sizer->Add(label_sitecode, 0, wxALIGN_CENTER | wxLEFT, 30);
    sitecode_sizer->Add(m_sitecode_textctrl, 1, wxALIGN_CENTER);
    sitecode_sizer->AddSpacer(5);
    sitecode_sizer->Add(m_activation_valid->GetSize().GetWidth(), 1);
    
    
    // -- create serial code sizer --
    
    wxStaticText* label_serial = new wxStaticText(this, -1, _("Serial Number:"));
    m_serial_textctrl = new wxTextCtrl(this, -1);

    wxBoxSizer* serial_sizer = new wxBoxSizer(wxHORIZONTAL);
    serial_sizer->Add(label_serial, 0, wxALIGN_CENTER | wxLEFT, 30);
    serial_sizer->Add(m_serial_textctrl, 1, wxALIGN_CENTER);
    serial_sizer->AddSpacer(5);
    serial_sizer->Add(m_activation_valid->GetSize().GetWidth(), 1);
    
    
    wxSize min_size_sitecode = m_serial_textctrl->GetTextExtent("XXXX XXXX XXXX XXXX").Scale(1.2f, 1.5f);
    
    // measure the label widths
    wxSize label_size = getMaxTextSize(label_serial,
                                            label_sitecode,
                                            label_authcode);
    label_size.x += 10;
    
    serial_sizer->SetItemMinSize(label_serial, label_size);
    sitecode_sizer->SetItemMinSize(label_sitecode, label_size);
    authcode_sizer->SetItemMinSize(label_authcode, label_size);

    serial_sizer->SetItemMinSize(m_serial_textctrl, min_size_sitecode);
    authcode_sizer->SetItemMinSize(m_authcode_textctrl, min_size_sitecode);

    // -- create status label --
    
    wxString activation_str;
    activation_str += _("Status");
    activation_str += wxT(":\t");
    activation_str += getActivationStatusText(m_global_auth);
    
    wxStaticText* status_label = new wxStaticText(this, -1, activation_str);
    makeFontBold(status_label);
    
    
    // create vertical sizer
    
    wxString appname = APPLICATION_NAME;
    wxString email = APP_CONTACT_SALESEMAIL;
    wxString str;

    if (APP_TRIAL_LICENSE_DAYS > 0)
    {
        str = wxString::Format(_("To continue using %s beyond the first %d day(s), a serial number is required.  You may obtain this number by purchasing %s; please contact %s for more details.  For help activating this software, please click the Help button below."),
                               appname.c_str(), APP_TRIAL_LICENSE_DAYS, appname.c_str(), email.c_str());
    }
     else
    {
         str = wxString::Format(_("A serial number is required to activate the software; for further help, please contact us at %s"), email.c_str());
    }


    wxStaticText* message = new wxStaticText(this, -1, str);
    
    // go figure -- we were having problems with wxStaticBitmap
    // showing a gray border around some bitmaps... switching
    // to kcl::ValidControl fixes the problem
    kcl::ValidControl* bmp = new kcl::ValidControl(this, -1,
                                               wxDefaultPosition,
                                               wxSize(34,34));

    bmp->setValidBitmap(GETBMP(gf_globe_32));
    bmp->setInvalidBitmap(GETBMP(gf_globe_32));
    bmp->setBorderVisible(false);
    bmp->setLabelVisible(false);
    bmp->setValid(true);
    
    wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
    vert_sizer->Add(status_label, 0, wxEXPAND | wxRIGHT, 20);
    vert_sizer->AddSpacer(15);
    vert_sizer->Add(message, 0, wxEXPAND | wxRIGHT, 20);
    vert_sizer->AddSpacer(15);
    vert_sizer->Add(m_internet_radio);
    vert_sizer->AddSpacer(8);
    vert_sizer->Add(serial_sizer, 0, wxEXPAND | wxRIGHT, 60);
    vert_sizer->AddSpacer(20);
    vert_sizer->Add(m_manual_radio);
    vert_sizer->AddSpacer(8);
    vert_sizer->Add(sitecode_sizer, 0, wxEXPAND | wxRIGHT, 60);
    vert_sizer->AddSpacer(8);
    vert_sizer->Add(authcode_sizer, 0, wxEXPAND | wxRIGHT, 60);
    vert_sizer->AddSpacer(8);
    
    
    // create top sizer
    
    wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
    top_sizer->AddSpacer(5);
    top_sizer->Add(bmp, 0, wxTOP, 5);
    top_sizer->AddSpacer(13);
    top_sizer->Add(vert_sizer, 1, wxEXPAND | wxTOP, 7);
    
    
    // -- create a platform standards-compliant OK/Cancel sizer --
    
    wxButton* ok_button = new wxButton(this, wxID_OK);
    wxButton* cancel_button = new wxButton(this, wxID_CANCEL);
    
    wxStaticText* serial_label = new wxStaticText(this, -1, wxEmptyString);

    wxString serial = getAppPrefsString(wxT("license_mgr.login"));
    serial.Trim(false);
    serial.Trim();
    
    if (serial.Length() == 15 &&
        serial.GetChar(2) == wxT('-') &&
        serial.GetChar(7) == wxT('-') &&
        serial.GetChar(12) == wxT('-'))
    {
        serial.Prepend(wxT(": "));
        serial.Prepend(_("Serial Number"));
        serial_label->SetLabel(serial);
    }
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    ok_cancel_sizer->AddButton(ok_button);
    ok_cancel_sizer->AddButton(cancel_button);
    //ok_cancel_sizer->AddButton(new wxButton(this, wxID_HELP));
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->Prepend(serial_label, 0, wxALIGN_CENTER | wxLEFT, 15);
    ok_cancel_sizer->AddSpacer(5);
    
    // -- this code is necessary to get the sizer's bottom margin to 8 --
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);
    
    
    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(8);
    main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    main_sizer->AddStretchSpacer();
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
    SetSizer(main_sizer);
    Layout();
    
    resizeStaticText(message, message->GetClientSize().GetWidth());
    Center();
    
    // start the dialog out on the "Activate automatically" radio button
    wxCommandEvent unused;
    unused.SetId(ID_Internet_Radio);
    onRadio(unused);
    
    // set the ok button as the default
    ok_button->SetDefault();

    SetClientSize(main_sizer->GetMinSize());
    SetMinSize(GetSize());
}

DlgAuth::~DlgAuth()
{

}

static void licenseServerError(wxWindow* parent)
{
    wxString compname = APP_COMPANY_NAME;
    wxString message = wxString::Format(_("The license server could not be contacted.  Your computer may not have the necessary Internet access or may be behind a firewall or proxy server.  Please try again and, if activation still does not work, contact %s for further information."),
                                        compname.c_str());

    appMessageBox(message,
                       APPLICATION_NAME,
                       wxOK | wxICON_EXCLAMATION | wxCENTER,
                       parent);
}

static void licenseClockUnsyncedError(wxWindow* parent)
{
    appMessageBox(_("Your computer's clock is set to the wrong date or time.  Please set your computer's clock to the correct date and time and try again."),
                       APPLICATION_NAME,
                       wxOK | wxICON_EXCLAMATION | wxCENTER,
                       parent);
}

static void licenseLoginInvalid(wxWindow* parent)
{
    wxString appname = APPLICATION_NAME;
    wxString compname = APP_COMPANY_NAME;
    wxString message = wxString::Format(_("%s could not be activated with the serial number provided.  Please check to make sure the serial number matches the one you were given and try again.  If activation still does not work, contact %s for further information."),
                                        appname.c_str(),
                                        compname.c_str());
    appMessageBox(message,
                       APPLICATION_NAME,
                       wxOK | wxICON_EXCLAMATION | wxCENTER,
                       parent);
}

static void licenseInvalidError(wxWindow* parent)
{
    wxString compname = APP_COMPANY_NAME;
    wxString message = wxString::Format(_("The serial number provided has either expired or is invalid.  Please check to make sure the serial number matches the one you were given and try again.  If activation still does not work, contact %s for further information."),
                                        compname.c_str());

    appMessageBox(message,
                       APPLICATION_NAME,
                       wxOK | wxICON_EXCLAMATION | wxCENTER,
                       parent);
}

static void licenseAppTagMismatchError(wxWindow* parent)
{
    wxString compname = APP_COMPANY_NAME;
    wxString message = wxString::Format(_("The serial number entered is not associated with this product.  Please contact %s or your system administrator for more information."),
                                        compname.c_str());

    appMessageBox(message,
                       APPLICATION_NAME,
                       wxOK | wxICON_EXCLAMATION | wxCENTER,
                       parent);

}

static void licenseNotAvailError(wxWindow* parent)
{
    wxString compname = APP_COMPANY_NAME;
    wxString message = wxString::Format(_("The maximum number of installations for this license has been reached.  Please contact %s for more information."),
                                        compname.c_str());

    appMessageBox(message,
                       APPLICATION_NAME,
                       wxOK | wxICON_EXCLAMATION | wxCENTER,
                       parent);
}

wxString DlgAuth::getSiteCodeString()
{
    char buf[255];
    paladin::getStringFromCode(m_site_code, buf);
    wxString str = buf;
    return str;
}

void DlgAuth::updateValidControl()
{
    if (m_internet_radio->GetValue())
    {
        m_activation_valid->setInvalidBitmap(GETBMP(gf_blank_16));
        m_activation_valid->setValid(false);
        return;
    }
    
    wxString activation_code = m_authcode_textctrl->GetValue();
    activation_code = removeChar(activation_code, ' ');
    activation_code.Replace(wxT("--"), wxT("!"));   // don't allow multiple dashes next to each other
    activation_code = removeChar(activation_code, '-');
    activation_code.MakeUpper();
    
    if (activation_code.Length() < 16)
    {
        m_activation_valid->setInvalidBitmap(GETBMP(gf_blank_16));
        m_activation_valid->setValid(false);
    }
     else if (activation_code.Length() == 16)
    {
        m_activation_valid->setInvalidBitmap(GETBMP(gf_x_16));

        if (validateManualCode(activation_code))
            m_activation_valid->setValid(true);
             else
             m_activation_valid->setValid(false);
    }
     else
    {
        m_activation_valid->setInvalidBitmap(GETBMP(gf_x_16));
        m_activation_valid->setValid(false);
    }
}

void DlgAuth::onRadio(wxCommandEvent& evt)
{
    int id = evt.GetId();
    bool internet = (id == ID_Internet_Radio) ? true : false;
    bool manual = (id == ID_Manual_Radio) ? true : false;
    
    m_internet_radio->SetValue(internet);
    m_serial_textctrl->Enable(internet);
    if (internet)
    {
        m_serial_textctrl->SetFocus();
        m_serial_textctrl->SetInsertionPointEnd();
    }
    
    m_manual_radio->SetValue(manual);
    m_sitecode_textctrl->Enable(manual);
    m_authcode_textctrl->Enable(manual);
    if (manual)
    {
        m_authcode_textctrl->SetFocus();
        m_authcode_textctrl->SetInsertionPointEnd();
    }
    
    // make sure we hide the valid control
    // if we're doing automatic activation
    updateValidControl();
}

void DlgAuth::onHelp(wxCommandEvent& evt)
{
    DlgAuthHelp dlg(this);
    dlg.ShowModal();
}

void DlgAuth::onOK(wxCommandEvent& evt)
{
    if (m_manual_radio->GetValue())
    {
        // manual code method chosen, validation must work in order to continue
        
        wxString activation_code = m_authcode_textctrl->GetValue();
        activation_code = removeChar(activation_code, ' ');
        activation_code.Replace(wxT("--"), wxT("!"));   // don't allow multiple dashes next to each other
        activation_code = removeChar(activation_code, '-');
        activation_code.MakeUpper();
        
        if (!validateManualCode(activation_code, true))
        {
            wxString appname = APPLICATION_NAME;
            wxString compname = APP_COMPANY_NAME;
            wxString message = wxString::Format(_("%s could not be activated with the activation code provided.  Please check to make sure the activation code matches the one you were given and try again.  If activation still does not work, contact %s for further information."),
                                                appname.c_str(), compname.c_str());
            appMessageBox(message,
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER,
                               this);
            
            m_authcode_textctrl->SetFocus();
            m_authcode_textctrl->SetSelection(-1,-1);
            m_authcode_textctrl->SetInsertionPointEnd();
            return;
        }
    }
     else
    {
        // automatic/internet authentication
        wxLogNull log;

        wxString license_login = m_serial_textctrl->GetValue();
        if (license_login.IsEmpty())
        {
            appMessageBox(_("Please enter your serial number to continue."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER,
                               this);
            
            m_serial_textctrl->SetFocus();
            m_serial_textctrl->SetSelection(-1,-1);
            m_serial_textctrl->SetInsertionPointEnd();
            return;
        }


        int ret = 0;

        {
            AppBusyCursor bc;

            ret = InetAuth::authorize(m_global_auth,
                                      m_product_version,
                                      license_login,
                                      wxT("password"));
        }


        if (ret == InetAuth::errorNetworkError)
        {
            licenseServerError(this);
            m_serial_textctrl->SetFocus();
            m_serial_textctrl->SetSelection(-1,-1);
            m_serial_textctrl->SetInsertionPointEnd();
            return;
        }

        if (ret == InetAuth::errorClockUnsynced)
        {
            licenseClockUnsyncedError(this);
            m_serial_textctrl->SetFocus();
            m_serial_textctrl->SetSelection(-1,-1);
            m_serial_textctrl->SetInsertionPointEnd();
            return;
        }
        
        if (ret == InetAuth::errorLoginInvalid)
        {
            licenseLoginInvalid(this);
            m_serial_textctrl->SetFocus();
            m_serial_textctrl->SetSelection(-1,-1);
            m_serial_textctrl->SetInsertionPointEnd();
            return;
        }

        if (ret == InetAuth::errorProductMismatch)
        {
            licenseAppTagMismatchError(this);
            m_serial_textctrl->SetFocus();
            m_serial_textctrl->SetSelection(-1,-1);
            m_serial_textctrl->SetInsertionPointEnd();
            return;
        }

        if (ret == InetAuth::errorLicenseNotAvail)
        {
            licenseNotAvailError(this);
            m_serial_textctrl->SetFocus();
            m_serial_textctrl->SetSelection(-1,-1);
            m_serial_textctrl->SetInsertionPointEnd();
            return;
        }

        if (ret != InetAuth::errorSuccess)
        {
            licenseInvalidError(this);
            m_serial_textctrl->SetFocus();
            m_serial_textctrl->SetSelection(-1,-1);
            m_serial_textctrl->SetInsertionPointEnd();
            return;
        }

        IAppPreferencesPtr prefs = g_app->getAppPreferences();
        if (prefs)
        {
            // -- save the license login --
            prefs->setString(wxT("license_mgr.login"), license_login);
            prefs->flush();
        }
    }
    
    Show(false);
    appMessageBox(_("Your license is now activated.  Thank you."),
                       APPLICATION_NAME,
                       wxOK | wxICON_INFORMATION | wxCENTER);
    EndModal(wxID_OK);
}

void DlgAuth::onCancel(wxCommandEvent& evt)
{
    EndModal(wxID_CANCEL);
}


void DlgAuth::onActivationCodeChanged(wxCommandEvent& evt)
{
    updateValidControl();
}


bool DlgAuth::validateManualCode(const wxString& code, bool save)
{
    // -- this will make if difficult to machine-crack --
    ::wxMilliSleep(40);
    
    if (save)
    {
        // we want to save the authentication information -- so use
        // the global paladin authentication object from the application
        paladin::actcode_t actcode = paladin::getCodeFromString(code.mbc_str());
        m_global_auth->setActivationCode(actcode);
        
        bool valid = (m_global_auth->checkAuth() == paladin::errNone ? true : false);
        return valid;
    }

    // we don't care about saving the authentication information -- we're
    // just checking so use the temporary paladin authentication object
    paladin::actcode_t actcode = paladin::getCodeFromString(code.mbc_str());
    m_paladin_auth->setActivationCode(actcode);
    
    bool valid = (m_paladin_auth->checkAuth() == paladin::errNone ? true : false);
    return valid;
}

