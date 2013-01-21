/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2002-08-13
 *
 */


#ifndef __KCL_WIZARD_H
#define __KCL_WIZARD_H



#include <xcm/signal.h>
#include <vector>


class wxBoxSizer;


namespace kcl
{


class WizardPage;
class Wizard : public wxPanel
{
public:

    enum
    {
        ID_PrevButton = 3214,
        ID_NextButton,
        ID_CancelButton,
        ID_FinishButton,
    };

public:

    Wizard(wxWindow* parent,
           wxWindowID id = -1,
           const wxPoint& pos = wxDefaultPosition,
           const wxSize& size = wxDefaultSize,
           long style = 0);
    virtual ~Wizard();

    void addPage(WizardPage* page,
                 const wxString& caption = wxEmptyString,
                 const wxBitmap& bitmap = wxNullBitmap);

    bool startWizard(WizardPage* page);
    bool startWizard(int idx = 0);

    WizardPage* getPage(int idx);
    int getPageIdx(WizardPage* page);
    int getPageCount();

    bool goPage(int idx);
    bool goPage(kcl::WizardPage* page);
    
    bool goPrevPage(bool notify_page = true);
    bool goNextPage(bool notify_page = true);

    void setPageOrder(WizardPage* page1,
                      WizardPage* page2 = NULL,
                      WizardPage* page3 = NULL,
                      WizardPage* page4 = NULL,
                      WizardPage* page5 = NULL,
                      WizardPage* page6 = NULL,
                      WizardPage* page7 = NULL,
                      WizardPage* page8 = NULL,
                      WizardPage* page9 = NULL,
                      WizardPage* page10 = NULL);

    void setFinishButtonLabel(const wxString& label = wxEmptyString);
    
    wxSizer* getExtraSizer();

public: // signals

    xcm::signal1<Wizard*> sigCancelPressed;
    xcm::signal1<Wizard*> sigFinishPressed;

private:
    
    void updateButtons(bool do_layout = true);

    // -- event handlers --
    void onPrevPage(wxCommandEvent& evt);
    void onNextPage(wxCommandEvent& evt);
    void onFinish(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);

private:

    wxBoxSizer* m_sizer;
    wxBoxSizer* m_curpage_sizer;
    wxBoxSizer* m_button_sizer;
    wxBoxSizer* m_extra_sizer;

    std::vector<WizardPage*> m_pages;
    WizardPage* m_cur_page;
    wxButton* m_prev_button;
    wxButton* m_next_button;
    wxButton* m_cancel_button;
    wxButton* m_finish_button;

    int m_start_page_idx;

    DECLARE_EVENT_TABLE()
};


class WizardPage : public wxPanel
{

public:
    WizardPage(Wizard* parent) :
            wxPanel(parent, -1,
                    wxPoint(0,0),
                    wxSize(1,1),
                     wxTAB_TRAVERSAL |
                     wxNO_FULL_REPAINT_ON_RESIZE |
                     wxCLIP_CHILDREN)
    {
        m_wizard_ptr = parent;
        m_caption = wxT("");

        Show(false);
    }

    ~WizardPage()
    {

    }

    virtual void onPageChanged()
    {

    }

    virtual bool onPageChanging(bool forward)
    {
        return true;
    }

    void setCaption(wxString caption)
    {
        m_caption = caption;
    }

    virtual wxString getCaption()
    {
        return m_caption;
    }

    Wizard* getWizard() const
    {
        return m_wizard_ptr;
    }

private:

    Wizard* m_wizard_ptr;
    wxString m_caption;
};


}; // namespace kcl


#endif


