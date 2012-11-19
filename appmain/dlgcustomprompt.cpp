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
    ID_Choice2Button,
    ID_Choice3Button,
};

CustomPromptDlg::CustomPromptDlg(wxWindow* parent,
                  const wxString& caption,
                  const wxString& message,
                  const wxSize& size)
                     : wxDialog(parent,
                                -1,
                                caption,
                                wxDefaultPosition,
                                (size == wxDefaultSize) ? wxSize(380, 200)
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
}

CustomPromptDlg::~CustomPromptDlg()
{

}

int CustomPromptDlg::ShowModal()
{
    // create the message
    wxStaticText* label_message = new wxStaticText(this, -1, m_message);
    
    // create a platform standards-compliant OK/Cancel sizer
    wxButton* button1 = NULL;
    wxButton* button2 = NULL;
    wxButton* button3 = NULL;
    wxButton* cancel_button = NULL;
    

    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);

    button_sizer->AddStretchSpacer();
    if (m_button_flags & showButton1)
    {
        button1 = new wxButton(this, ID_Choice1Button, m_button1_label);
        button_sizer->Add(button1, 0);
    }
    if (m_button_flags & showButton2)
    {
        button2 = new wxButton(this, ID_Choice2Button, m_button2_label);
        button_sizer->AddSpacer(5);
        button_sizer->Add(button2, 0);
    }
    if (m_button_flags & showButton3)
    {
        button3 = new wxButton(this, ID_Choice3Button, m_button3_label);
        button_sizer->AddSpacer(5);
        button_sizer->Add(button3, 0);
    }
    if (m_button_flags & showCancel)
    {
        cancel_button = new wxButton(this, wxID_CANCEL);
        button_sizer->AddSpacer(5);
        button_sizer->Add(cancel_button, 0);
    }
    button_sizer->AddSpacer(8); // per convention, buttons should be right-justified



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
    main_sizer->AddSpacer(5);
    main_sizer->Add(button_sizer, 0, wxEXPAND, 16);
    main_sizer->AddSpacer(8);
            
    SetSizer(main_sizer);
    Layout();
    
    // re-calculate the size of the dialog so it doesn't look stupid
    
    int w = GetClientSize().GetWidth();
    
    // 80 = 16*3 (left, right and inner padding) + 32 (bitmap width)
    if (button_sizer->GetMinSize().GetWidth()+80 > w)
        w = button_sizer->GetMinSize().GetWidth()+80;
    
    // 75 = 16*3 (left, right and inner padding) + 32 (bitmap width) - 5 (labels look better just a little closer to the dialog window border)
    resizeStaticText(label_message, w-75);
    

    int h = 0;
    h += 12;     // top spacer
    h += top_sizer->GetMinSize().GetHeight();
    h += 12;    // spacer between text and ok/cancel sizer
    h += button_sizer->GetMinSize().GetHeight();
    h += 16;

    SetClientSize(w,h);

    CenterOnParent();
    
    // set the default button for the dialog
    if (cancel_button)
        cancel_button->SetDefault();
         else if (button1)
        button1->SetDefault();
         else if (button2)
        button2->SetDefault();
         else if (button3)
        button3->SetDefault();

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

void CustomPromptDlg::setButton3(int id, const wxString& label)
{
    m_button3_id = id;
    m_button3_label = label;
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
    EVT_BUTTON(ID_Choice3Button, CustomPromptDlg::onChoice3Clicked)
    EVT_BUTTON(wxID_CANCEL, CustomPromptDlg::onCancel)
END_EVENT_TABLE()

