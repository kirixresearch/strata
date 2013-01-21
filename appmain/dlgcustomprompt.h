/*!
 *
 * Copyright (c) 2012-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2012-09-05
 *
 */


#ifndef __APP_DLGCUSTOMPROMPT_H
#define __APP_DLGCUSTOMPROMPT_H



class CustomPromptDlg : public wxDialog
{
public:
    
    enum Bitmaps
    {
        bitmapError = 0,
        bitmapQuestion,
        bitmapWarning,
        bitmapInformation
    };
    
    enum ButtonFlags
    {
        showButton1 = 0x01,
        showButton2 = 0x02,
        showButton3 = 0x04,
        showCancel  = 0x08
    };
    
    CustomPromptDlg(wxWindow* parent, 
                    const wxString& caption,
                    const wxString& message,
                    const wxSize& size = wxDefaultSize);
    ~CustomPromptDlg();
    
    int ShowModal();
    
    // for bitmap ids, reference the list in the ArtProvider class
    void setBitmap(int bitmap_id);
    void setButton1(int id, const wxString& label);
    void setButton2(int id, const wxString& label);
    void setButton3(int id, const wxString& label);
    void showButtons(int button_flags);

private:

    // event handlers
    void onChoice1Clicked(wxCommandEvent& evt);
    void onChoice2Clicked(wxCommandEvent& evt);
    void onChoice3Clicked(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);

private:

    int m_bitmap_id;
    int m_button_flags;
    int m_button1_id;
    int m_button2_id;
    int m_button3_id;
    wxString m_button1_label;
    wxString m_button2_label;
    wxString m_button3_label;
    wxString m_message;
    
    DECLARE_EVENT_TABLE()
};



#endif

