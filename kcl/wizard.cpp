/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2002-08-13
 *
 */


#include <wx/wx.h>
#include <wx/statline.h>
#include "wizard.h"
#include "button.h"


namespace kcl
{


BEGIN_EVENT_TABLE(Wizard, wxPanel)
    EVT_BUTTON(ID_PrevButton, Wizard::onPrevPage)
    EVT_BUTTON(ID_NextButton, Wizard::onNextPage)
    EVT_BUTTON(ID_FinishButton, Wizard::onFinish)
    EVT_BUTTON(ID_CancelButton, Wizard::onCancel)
    EVT_SIZE(Wizard::onSize)
END_EVENT_TABLE()


Wizard::Wizard(wxWindow* parent,
               wxWindowID id,
               const wxPoint& pos,
               const wxSize& size,
               long style) : wxPanel(parent, id, pos, size, style |
                                       wxTAB_TRAVERSAL |
                                       wxNO_FULL_REPAINT_ON_RESIZE |
                                       wxCLIP_CHILDREN)
{
    // initialize member variables
    m_cur_page = NULL;

    // create controls
    m_prev_button = new wxButton(this, ID_PrevButton, _("Previous"));
    m_next_button = new wxButton(this, ID_NextButton, _("Next"));
    m_finish_button = new wxButton(this, ID_FinishButton, _("Finish"));
    m_cancel_button = new wxButton(this, ID_CancelButton, _("Cancel"));

    // NOTE: we do not show the finish button here as the programmer must
    //       explitly show the button based on the page the user is on
    m_prev_button->Show(true);
    m_next_button->Show(true);
    m_finish_button->Show(false);
    m_cancel_button->Show(true);

    // create extra sizer
    m_extra_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_extra_sizer->AddSpacer(1);

    // create button sizer
    m_button_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_button_sizer->Add(m_extra_sizer, 1, wxEXPAND);
    m_button_sizer->Add(m_prev_button, 0, wxEXPAND);
    m_button_sizer->Add(m_next_button, 0, wxEXPAND | wxRIGHT, 8);
    m_button_sizer->Add(m_finish_button, 0, wxEXPAND | wxRIGHT, 8);
    m_button_sizer->Add(m_cancel_button, 0, wxEXPAND | wxRIGHT, 8);

    // create current page sizer
    m_curpage_sizer = new wxBoxSizer(wxVERTICAL);
    
    // create main sizer
    m_sizer = new wxBoxSizer(wxVERTICAL);
    m_sizer->Add(m_curpage_sizer, 1, wxEXPAND);
    m_sizer->Add(new wxStaticLine(this, -1), 0, wxEXPAND);
    m_sizer->AddSpacer(8);
    m_sizer->Add(m_button_sizer, 0, wxEXPAND);
    m_sizer->AddSpacer(8);

    SetSizer(m_sizer);
    Layout();
}

Wizard::~Wizard()
{

}

bool Wizard::startWizard(WizardPage* page)
{
    if (page == NULL)
        return false;

    size_t idx = 0, count = m_pages.size();
    while (idx < count)
    {
        if (page == getPage(idx))
        {
            goPage(idx);
            return true;
        }
        
        idx++;
    }
    
    return false;
}

bool Wizard::startWizard(int idx)
{
    if (idx < 0 || (size_t)idx >= m_pages.size())
        return false;

    if (m_pages[idx] == NULL)
        return false;

    goPage(idx);
    return true;
}

WizardPage* Wizard::getPage(int idx)
{
    if (idx < 0 || (size_t)idx >= m_pages.size())
        return NULL;

    return m_pages[idx];
}

int Wizard::getPageIdx(WizardPage* page)
{
    int i, count = (int)m_pages.size();
    for (i = 0; i < count; ++i)
    {
        if (page == m_pages[i])
            return i;
    }
    
    return -1;
}

int Wizard::getPageCount()
{
    int count = 0;
    std::vector<WizardPage*>::iterator it;
    for (it = m_pages.begin(); it != m_pages.end(); ++it)
    {
        if (*it == NULL)
            break;
            
        count++;
    }
    
    return count;
}

void Wizard::updateButtons(bool do_layout)
{
    if (!m_cur_page)
        return;

    WizardPage* prev_page = getPage(getPageIdx(m_cur_page)-1);
    if (prev_page != NULL)
    {
        if (!m_prev_button->IsShown())
            m_prev_button->Show(true);
    }
     else
    {
        if (m_prev_button->IsShown())
            m_prev_button->Show(false);
    }

    WizardPage* next_page = getPage(getPageIdx(m_cur_page)+1);
    if (next_page != NULL)
    {
        if (!m_next_button->IsShown())
            m_next_button->Show(true);
            
        if (m_finish_button->IsShown())
            m_finish_button->Show(false);
    }
     else
    {
        if (m_next_button->IsShown())
            m_next_button->Show(false);
            
        if (!m_finish_button->IsShown())
            m_finish_button->Show(true);
    }
    
    if (do_layout)
        m_button_sizer->Layout();
}

void Wizard::addPage(WizardPage* page,
                     const wxString& caption,
                     const wxBitmap& bitmap)
{
    page->setCaption(caption);
    m_pages.push_back(page);
}

bool Wizard::goPage(kcl::WizardPage* page)
{
    return goPage(getPageIdx(page));
}

bool Wizard::goPage(int idx)
{
    if (idx < 0 || (size_t)idx >= m_pages.size())
        return false;

    if (m_cur_page == m_pages[idx])
        return true;

    Freeze();
    
    if (m_cur_page != NULL)
    {
        m_curpage_sizer->Detach(m_cur_page);
        m_cur_page->Show(false);
    }
    
    m_cur_page = m_pages[idx];
    m_cur_page->Show(true);

    m_curpage_sizer->Add(m_cur_page, 1, wxEXPAND);

    updateButtons(false);
    Layout();

    m_cur_page->onPageChanged();
    
    Thaw();
    return true;
}

bool Wizard::goPrevPage(bool notify_page)
{
    if (notify_page)
    {
        if (!m_cur_page->onPageChanging(false))
        {
            return false;
        }
    }

    int prev_idx = getPageIdx(m_cur_page) - 1;
    
    if (getPage(prev_idx) != NULL)
    {
            goPage(prev_idx);
    }

    return true;
}

bool Wizard::goNextPage(bool notify_page)
{
    if (notify_page)
    {
        if (!m_cur_page->onPageChanging(true))
        {
            return false;
        }
    }

    int next_idx = getPageIdx(m_cur_page) + 1;
    
    if (getPage(next_idx) != NULL)
    {
        goPage(next_idx);
    }
     else
    {
        wxCommandEvent unused;
        onFinish(unused);
    }

    return true;
}

void Wizard::setPageOrder(WizardPage* page1, WizardPage* page2,
                          WizardPage* page3, WizardPage* page4,
                          WizardPage* page5, WizardPage* page6,
                          WizardPage* page7, WizardPage* page8,
                          WizardPage* page9, WizardPage* page10)
{
    m_pages.clear();
    m_pages.push_back(page1);
    m_pages.push_back(page2);
    m_pages.push_back(page3);
    m_pages.push_back(page4);
    m_pages.push_back(page5);
    m_pages.push_back(page6);
    m_pages.push_back(page7);
    m_pages.push_back(page8);
    m_pages.push_back(page9);
    m_pages.push_back(page10);
    
    updateButtons();
}

void Wizard::setFinishButtonLabel(const wxString& label)
{
    if (label.IsEmpty())
    {
        m_finish_button->SetLabel(_("Finish"));
        return;
    }
    
    m_finish_button->SetLabel(label);
}

wxSizer* Wizard::getExtraSizer()
{
    return (wxSizer*)m_extra_sizer;
}

// event handlers

void Wizard::onPrevPage(wxCommandEvent& evt)
{
    goPrevPage();
}

void Wizard::onNextPage(wxCommandEvent& evt)
{
    goNextPage();
}

void Wizard::onFinish(wxCommandEvent& evt)
{
    // looks like we are done with the wizard; get permission
    // to close it out from the current page, and then fire signal

    if (!m_cur_page->onPageChanging(true))
        return;

    sigFinishPressed(this);
}

void Wizard::onCancel(wxCommandEvent& evt)
{
    sigCancelPressed(this);
}

void Wizard::onSize(wxSizeEvent& evt)
{
    Layout();
}

}; // namespace kcl


