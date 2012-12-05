/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Code Generator Client
 * Author:   Benjamin I. Williams
 * Created:  2003-04-27
 *
 */


#ifndef __PALGEN_GENERATEPAGE_H
#define __PALGEN_GENERATEPAGE_H


class wxTextCtrl;
class wxCalendarCtrl;
class wxCalendarEvent;


inline std::wstring towstr(const wxString& s)
{
#ifdef _UNICODE
    return (const wchar_t*)s.wc_str();
#else
    std::wstring result;
    int i, len;
    len = s.Length();
    result.resize(len);
    for (i = 0; i < len; ++i)
        result[i] = (unsigned char)s[i];
    return result;
#endif
}


class GenerationPage : public wxPanel
{

public:

    GenerationPage(wxWindow* parent);
    ~GenerationPage();

private:

    void onCancel(wxCommandEvent& event);
    void onExpirationButton(wxCommandEvent& event);
    void onSiteCodeChanged(wxCommandEvent& event);
    void onAppTagChanged(wxCommandEvent& event);
    void onFeatureIDChanged(wxCommandEvent& event);
    void onCalendar(wxCalendarEvent& event);

    void recalcActivationCode();
    void onGenerateLicenseFile(wxCommandEvent& event);
    void onPerpetual(wxCommandEvent& event);

private:

    wxTextCtrl* m_site_code;
    wxTextCtrl* m_activation_code;
    wxTextCtrl* m_feature_id;
    wxTextCtrl* m_app_tag;
    wxCalendarCtrl* m_exp_date;

    wxButton* m_generate_button;
    wxButton* m_cancel;

    DECLARE_EVENT_TABLE()
};





#endif

