/*!
 *
 * Copyright (c) 2012, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2012-09-05
 *
 */


#include "appmain.h"
#include "dlgcustomprompt.h"
#include <wx/artprov.h>

// displays a dialog with a custom question or
// information and custom labels on the buttons (instead of the
// usual OK/Cancel or Yes/No -- e.g. Append/Overwrite/Cancel)

enum
{
    ID_Choice1Button = wxID_HIGHEST + 1,
    ID_Choice2Button
};

CustomPromptDlg::CustomPromptDlg(wxWindow* parent,
                  const wxString& caption,
                  const wxString& message,
                  const wxSize& size)
                     : wxDialog(parent,
                                -1,
                                caption,
                                wxDefaultPosition,
                                (size == wxDefaultSize) ? wxSize(380, 160)
                                                        : size,
                                wxDEFAULT_DIALOG_STYLE |
                                wxCENTER)
{
    m_button_flags = showButton1 | showButton2 | showCancel;
    
    m_message   = message;
    m_bitmap_id = bitmapQuestion;
    
    m_button1_id    = wxYES;
    m_button1_label = _("Yes");
    
    m_button2_id    = wxNO;
    m_button2_label = _("No");

    m_button3_id    = wxCANCEL;
    m_button3_label = _("Cancel");
}

CustomPromptDlg::~CustomPromptDlg()
{

}

int CustomPromptDlg::ShowModal()
{
    // create the message
    wxStaticText* label_message = new wxStaticText(this, -1, m_message);
    
    // create a platform standards-compliant OK/Cancel sizer
    wxButton* yes_button = NULL;
    wxButton* no_button = NULL;
    wxButton* cancel_button = NULL;
    
    wxStdDialogButtonSizer* ok_cancel_sizer = new wxStdDialogButtonSizer;
    if (m_button_flags & showCancel)
    {
        cancel_button = new wxButton(this, wxID_CANCEL);
        ok_cancel_sizer->SetCancelButton(cancel_button);
    }
    if (m_button_flags & showButton2)
    {
        no_button = new wxButton(this, ID_Choice2Button, m_button2_label);
        ok_cancel_sizer->SetNegativeButton(no_button);
    }
    if (m_button_flags & showButton1)
    {
        yes_button = new wxButton(this, ID_Choice1Button, m_button1_label);
        ok_cancel_sizer->SetAffirmativeButton(yes_button);
    }
    ok_cancel_sizer->Realize();
    ok_cancel_sizer->AddSpacer(5);
    
    // this code is necessary to get the OK/Cancel sizer's bottom margin to 8
    wxSize min_size = ok_cancel_sizer->GetMinSize();
    min_size.SetHeight(min_size.GetHeight()+16);
    ok_cancel_sizer->SetMinSize(min_size);

    // create the dialog bitmap
    wxBitmap bmp;
    switch (m_bitmap_id)
    {
        default:
        case bitmapQuestion:    bmp = wxArtProvider::GetBitmap(wxART_QUESTION, wxART_MESSAGE_BOX);    break;
        case bitmapError:       bmp = wxArtProvider::GetBitmap(wxART_ERROR, wxART_MESSAGE_BOX);       break;
        case bitmapWarning:     bmp = wxArtProvider::GetBitmap(wxART_WARNING, wxART_MESSAGE_BOX);     break;
        case bitmapInformation: bmp = wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MESSAGE_BOX); break;
    }
    wxStaticBitmap* bitmap_question = new wxStaticBitmap(this, -1, bmp);
    
    
    // create top sizer
    
    wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
    top_sizer->Add(bitmap_question);
    top_sizer->AddSpacer(16);
    top_sizer->Add(label_message, 0, wxEXPAND);
    
    // create main sizer
    
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->AddSpacer(12);
    main_sizer->Add(top_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
    main_sizer->AddStretchSpacer();
    main_sizer->Add(ok_cancel_sizer, 0, wxEXPAND);
            
    SetSizer(main_sizer);
    Layout();

    // recalculate the size of the dialog so it doesn't look stupid
    
    int w = GetClientSize().GetWidth();
    
    // 80 = 16*3 (left, right and inner padding) + 32 (bitmap width)
    if (ok_cancel_sizer->GetMinSize().GetWidth()+80 > w)
        w = ok_cancel_sizer->GetMinSize().GetWidth()+80;
    
    // 75 = 16*3 (left, right and inner padding) + 32 (bitmap width) - 5 (labels look better just a little closer to the dialog window border)
    cfw::resizeStaticText(label_message, w-75);
    
    int h = 0;
    h += 12;     // top spacer
    h += top_sizer->GetMinSize().GetHeight();
    h += 12;    // spacer between text and ok/cancel sizer
    h += ok_cancel_sizer->GetMinSize().GetHeight();
    
    SetClientSize(w,h);
    CenterOnParent();
    
    // set the default button for the dialog
    if (cancel_button)
        cancel_button->SetDefault();
         else if (no_button)
        no_button->SetDefault();
         else if (yes_button)
        yes_button->SetDefault();

    return wxDialog::ShowModal();
}

void CustomPromptDlg::setBitmap(int bitmap_id)
{
    m_bitmap_id = bitmap_id;
}

void CustomPromptDlg::setButton1(int id, const wxString& label)
{
    m_button1_id = id;
    m_button1_label = label;
}

void CustomPromptDlg::setButton2(int id, const wxString& label)
{
    m_button2_id = id;
    m_button2_label = label;
}

void CustomPromptDlg::showButtons(int button_flags)
{
    m_button_flags = button_flags;
}

void CustomPromptDlg::onChoice1Clicked(wxCommandEvent& evt)
{
    EndModal(m_button1_id);
}

void CustomPromptDlg::onChoice2Clicked(wxCommandEvent& evt)
{
    EndModal(m_button2_id);
}

void CustomPromptDlg::onChoice3Clicked(wxCommandEvent& evt)
{
    EndModal(m_button3_id);
}

void CustomPromptDlg::onCancel(wxCommandEvent& evt)
{
    EndModal(wxID_CANCEL);
}


BEGIN_EVENT_TABLE(CustomPromptDlg, wxDialog)
    EVT_BUTTON(ID_Choice1Button, CustomPromptDlg::onChoice1Clicked)
    EVT_BUTTON(ID_Choice2Button, CustomPromptDlg::onChoice2Clicked)
    EVT_BUTTON(wxID_CANCEL, CustomPromptDlg::onCancel)
END_EVENT_TABLE()

