/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2012-08-17
 *
 */


#ifndef __APP_DLGSHAREVIEW_H
#define __APP_DLGSHAREVIEW_H



class DlgShareView : public wxDialog
{
public:

    DlgShareView(wxWindow* parent);
    ~DlgShareView();
    
    xcm::signal1<wxString&> sigRequestShareUrl;

private:

    void onShare(wxCommandEvent& evt);
    void onCopy(wxCommandEvent& evt);
    void onOK(wxCommandEvent& evt);

private:

    wxTextCtrl* m_url_text;
    kcl::Button* m_share_button;
    wxButton* m_copy_button;
    wxButton* m_ok_button;


    DECLARE_EVENT_TABLE()
};




#endif

