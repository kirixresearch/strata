/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Code Generator Client
 * Author:   Benjamin I. Williams
 * Created:  2004-02-02
 *
 */


#include <wx/wx.h>
#include <wx/calctrl.h>
#include <wx/notebook.h>
#include <kl/klib.h>
#include <xcm/xcm.h>
#include "../paladin/paladin.h"
#include "../paladin/crypt.h"
#include "../paladin/crc.h"
#include "../paladin/util.h"
#include "app.h"
#include "generatepage.h"
#include "gencode.h"


namespace paladin
{
    sitecode_t getHostId();
};


enum
{
    ID_Generate = wxID_HIGHEST+1,
    ID_Quit,
    ID_About,
    ID_SiteCodeInfo,
    ID_LicenseKeyInfo,
    ID_SiteCode,
    ID_AppTag,
    ID_FeatureID,
    ID_ActivationCode,
    ID_Calendar,
    ID_ExpOneMonth,
    ID_ExpThreeMonths,
    ID_ExpSixMonths,
    ID_ExpOneYear,
    ID_ExpEighteenMonths,
    ID_ExpTwoYears,
    ID_Perpetual,
    ID_GenerateLicenseFile
};


BEGIN_EVENT_TABLE(GenerationPage, wxPanel)
    EVT_BUTTON(wxID_CANCEL, GenerationPage::onCancel)
    EVT_BUTTON(ID_ExpOneMonth, GenerationPage::onExpirationButton)
    EVT_BUTTON(ID_ExpThreeMonths, GenerationPage::onExpirationButton)
    EVT_BUTTON(ID_ExpSixMonths, GenerationPage::onExpirationButton)
    EVT_BUTTON(ID_ExpOneYear, GenerationPage::onExpirationButton)
    EVT_BUTTON(ID_ExpEighteenMonths, GenerationPage::onExpirationButton)
    EVT_BUTTON(ID_ExpTwoYears, GenerationPage::onExpirationButton)
    EVT_BUTTON(ID_GenerateLicenseFile, GenerationPage::onGenerateLicenseFile)
    EVT_TEXT(ID_SiteCode, GenerationPage::onSiteCodeChanged)
    EVT_TEXT(ID_AppTag, GenerationPage::onAppTagChanged)
    EVT_TEXT(ID_FeatureID, GenerationPage::onFeatureIDChanged)
    EVT_CALENDAR_SEL_CHANGED(ID_Calendar, GenerationPage::onCalendar)
    EVT_CHECKBOX(ID_Perpetual, GenerationPage::onPerpetual)
END_EVENT_TABLE()


void makeTextCtrlUpperCase(wxTextCtrl* text)
{
#ifdef WIN32
    HWND h = (HWND)text->GetHWND();
    LONG l = GetWindowLongA(h, GWL_STYLE);
    l |= ES_UPPERCASE;
    SetWindowLongA(h, GWL_STYLE, l);
#endif
}


GenerationPage::GenerationPage(wxWindow* parent) : wxPanel(parent, -1)
{
    wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* button_sizer = new wxBoxSizer(wxVERTICAL);


    // -- key sizer --
    wxStaticBox* key_static_box = new wxStaticBox(this, -1, _("Site and Activation Codes"));
    wxStaticBoxSizer* key_sizer = new wxStaticBoxSizer(key_static_box, wxVERTICAL);

    wxBoxSizer* site_code_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* st1 = new wxStaticText(this, -1, _("Site Code:"), wxDefaultPosition, wxDefaultSize);
    m_site_code = new wxTextCtrl(this, ID_SiteCode);
    site_code_sizer->Add(st1, 0, wxALIGN_CENTER | wxRIGHT, 3);
    site_code_sizer->Add(m_site_code, 1, wxEXPAND, 0);
    makeTextCtrlUpperCase(m_site_code);

    wxBoxSizer* app_tag_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* st2 = new wxStaticText(this, -1, _("Application Tag:"), wxDefaultPosition, wxDefaultSize);
    m_app_tag = new wxTextCtrl(this, ID_AppTag);
    app_tag_sizer->Add(st2, 0, wxALIGN_CENTER | wxRIGHT, 3);
    app_tag_sizer->Add(m_app_tag, 1, wxEXPAND, 0);
    makeTextCtrlUpperCase(m_app_tag);

    wxBoxSizer* feature_id_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* st3 = new wxStaticText(this, -1, _("Feature ID:"), wxDefaultPosition, wxDefaultSize);
    m_feature_id = new wxTextCtrl(this, ID_FeatureID, wxT(""));
    feature_id_sizer->Add(st3, 0, wxALIGN_CENTER | wxRIGHT, 3);
    feature_id_sizer->Add(m_feature_id, 1, wxEXPAND, 0);
    makeTextCtrlUpperCase(m_feature_id);

    wxBoxSizer* activation_key_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* st4 = new wxStaticText(this, -1, _("Activation Code:"), wxDefaultPosition, wxDefaultSize);
    m_activation_code = new wxTextCtrl(this, ID_ActivationCode, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    activation_key_sizer->Add(st4, 0, wxALIGN_CENTER | wxRIGHT, 3);
    activation_key_sizer->Add(m_activation_code, 1, wxEXPAND, 0);
    makeTextCtrlUpperCase(m_activation_code);


    site_code_sizer->SetItemMinSize(st1, wxSize(100,14));
    site_code_sizer->SetItemMinSize(m_site_code, wxSize(250,22));
    app_tag_sizer->SetItemMinSize(st2, wxSize(100,14));
    app_tag_sizer->SetItemMinSize(m_app_tag, wxSize(250,22));
    feature_id_sizer->SetItemMinSize(st3, wxSize(100,14));
    feature_id_sizer->SetItemMinSize(m_feature_id, wxSize(250,22));
    activation_key_sizer->SetItemMinSize(st4, wxSize(100,14));
    activation_key_sizer->SetItemMinSize(m_activation_code, wxSize(250,22));

    key_sizer->Add(site_code_sizer, 0, wxEXPAND | wxALL, 4);
    key_sizer->Add(app_tag_sizer, 0, wxEXPAND | wxALL, 4);
    key_sizer->Add(feature_id_sizer, 0, wxEXPAND | wxALL, 4);
    key_sizer->Add(activation_key_sizer, 0, wxEXPAND | wxALL, 4);

    // -- expiration date sizer --

    wxBoxSizer* exp_button_sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBox* exp_static_box = new wxStaticBox(this, -1, _("Expiration Date"));
    wxStaticBoxSizer* exp_sizer = new wxStaticBoxSizer(exp_static_box, wxVERTICAL);
    wxBoxSizer* exp_horz_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_exp_date = new wxCalendarCtrl(this, ID_Calendar, wxDefaultDateTime);


    wxCheckBox* perpetual = new wxCheckBox(this, ID_Perpetual, _("Perpetual"));
    wxButton* exp1 = new wxButton(this, ID_ExpOneMonth, _("One Month"));
    wxButton* exp2 = new wxButton(this, ID_ExpThreeMonths, _("Three Months"));
    wxButton* exp3 = new wxButton(this, ID_ExpSixMonths, _("Six Months"));
    wxButton* exp4 = new wxButton(this, ID_ExpOneYear, _("One Year"));
    wxButton* exp5 = new wxButton(this, ID_ExpEighteenMonths, _("Eighteen Months"));
    wxButton* exp6 = new wxButton(this, ID_ExpTwoYears, _("Two Years"));

    exp_button_sizer->Add(perpetual, 0, wxEXPAND | wxBOTTOM, 2);
    exp_button_sizer->Add(exp1, 0, wxEXPAND | wxBOTTOM, 2);
    exp_button_sizer->Add(exp2, 0, wxEXPAND | wxBOTTOM, 2);
    exp_button_sizer->Add(exp3, 0, wxEXPAND | wxBOTTOM, 2);
    exp_button_sizer->Add(exp4, 0, wxEXPAND | wxBOTTOM, 2);
    exp_button_sizer->Add(exp5, 0, wxEXPAND | wxBOTTOM, 2);
    exp_button_sizer->Add(exp6, 0, wxEXPAND | wxBOTTOM, 2);

    wxString today_str;
    today_str = _("Today is: ");
    today_str += wxDateTime::Today().Format(wxT("%A, %B %d, %Y"));

    wxStaticText* st5 = new wxStaticText(this, -1, today_str, wxDefaultPosition, wxDefaultSize);

    exp_horz_sizer->Add(m_exp_date, 0, wxALIGN_CENTER | wxALL, 5);
    exp_horz_sizer->Add(exp_button_sizer, 1, wxALIGN_CENTER | wxALL, 5);

    exp_sizer->Add(st5, 0, wxEXPAND | wxALL, 5);
    exp_sizer->Add(exp_horz_sizer, 1, wxEXPAND);


    wxButton* save_license_file = new wxButton(this, ID_GenerateLicenseFile, _("Create License File..."));

    vert_sizer->Add(key_sizer, 0, wxEXPAND | wxBOTTOM, 8);
    vert_sizer->Add(exp_sizer, 1, wxEXPAND | wxBOTTOM, 5);
    vert_sizer->Add(save_license_file, 0, wxBOTTOM | wxALIGN_RIGHT);

    //m_generate_button = new wxButton(this, ID_Generate, _("Generate"));
    m_cancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
    m_cancel->Show(FALSE);

    //button_sizer->Add(m_generate_button, 0, wxEXPAND | wxBOTTOM, 5);
    button_sizer->Add(m_cancel, 0, wxEXPAND);

    main_sizer->Add(vert_sizer, 1, wxEXPAND | wxALL, 7);

    SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);

    Center();

    m_feature_id->SetValue(wxT("0"));
    recalcActivationCode();
}


GenerationPage::~GenerationPage()
{
}


void GenerationPage::onCancel(wxCommandEvent& event)
{
    Destroy();
}


void GenerationPage::onPerpetual(wxCommandEvent& event)
{
    bool checked = (event.GetInt() == wxCHK_CHECKED);

    if (checked)
    {
        m_exp_date->Enable(false);
    }
     else
    {
        m_exp_date->Enable(true);
    }

    recalcActivationCode();
}

void GenerationPage::onExpirationButton(wxCommandEvent& event)
{
    wxDateTime new_date = wxDateTime::Today();

    switch (event.GetId())
    {
        case ID_ExpOneMonth:
            new_date += wxDateSpan::Months(1);
            break;

        case ID_ExpThreeMonths:
            new_date += wxDateSpan::Months(3);
            break;

        case ID_ExpSixMonths:
            new_date += wxDateSpan::Months(6);
            break;

        case ID_ExpOneYear:
            new_date += wxDateSpan::Months(12);
            break;

        case ID_ExpEighteenMonths:
            new_date += wxDateSpan::Months(18);
            break;

        case ID_ExpTwoYears:
            new_date += wxDateSpan::Months(24);
            break;
    }

    m_exp_date->SetDate(new_date);

    m_cancel->SetDefault();

    m_site_code->SetInsertionPointEnd();
    m_site_code->SetFocus();

    recalcActivationCode();
}


void GenerationPage::onSiteCodeChanged(wxCommandEvent& event)
{
    if (validateSiteCode(event.GetString()))
    {
        m_site_code->SetForegroundColour(*wxBLUE);
        m_site_code->Refresh();
    }
     else
    {
        m_site_code->SetForegroundColour(*wxBLACK);
        m_site_code->Refresh();
    }
    
    recalcActivationCode();
}

void GenerationPage::onAppTagChanged(wxCommandEvent& event)
{
    recalcActivationCode();
}

void GenerationPage::onFeatureIDChanged(wxCommandEvent& event)
{
    recalcActivationCode();
}


void GenerationPage::onCalendar(wxCalendarEvent& event)
{
    recalcActivationCode();
}




void GenerationPage::recalcActivationCode()
{
    wxDateTime exp_date = m_exp_date->GetDate();

    int y = exp_date.GetYear();
    int m = exp_date.GetMonth() + 1;
    int d = exp_date.GetDay();

    if (!m_exp_date->IsEnabled())
    {
        // -- perpetual license --
        y = 0;
        m = 0;
        d = 0;
    }

    wxDateTime yesterday = wxDateTime::Now();
    yesterday -= wxTimeSpan::Day();

    if (exp_date < yesterday)
    {
        m_activation_code->SetValue(_("[Expiration in the past]"));
        return;
    }

    wxString actcode_str = calcActivationCode(m_app_tag->GetValue(),
                                              m_site_code->GetValue(),
                                              wxAtoi(m_feature_id->GetValue()),
                                              y,
                                              m,
                                              d);

    m_activation_code->SetValue(actcode_str);
}



void GenerationPage::onGenerateLicenseFile(wxCommandEvent& event)
{
    wxFileDialog dlg(this, _("Create License File"),
                     wxT(""), wxT(""), _("License Files (*.kxl)|*.kxl"), wxFD_SAVE);

    if (dlg.ShowModal() == wxID_OK)
    {
        kl::xmlnode node;

        node.setNodeName(L"license_info");

        kl::xmlproperty& prop = node.appendProperty();
        prop.name = L"version";
        prop.value = L"1.0";

        node.addChild(L"app_tag", towstr(m_app_tag->GetValue()));
        node.addChild(L"act_code", towstr(m_activation_code->GetValue()));

        char buf[2048];
        std::string xml = kl::tostring(node.getXML(kl::xmlnode::formattingCrLf | kl::xmlnode::formattingSpaces));
        strcpy(buf+2, xml.c_str());
        buf[0] = 0;
        buf[1] = 0x1a;

        FILE* f = fopen(dlg.GetPath().mbc_str(), "wb");
        if (!f)
        {
            wxMessageBox(wxT("Error while writing file."));
            return;
        }

        fwrite(buf, 1, strlen(buf+2)+2, f);
        fclose(f);
    }
}




