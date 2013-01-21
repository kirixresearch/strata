/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; Aaron L. Williams
 * Created:  2003-03-03
 *
 */


#ifndef __APP_MODULEREMOVEDUPREC_H
#define __APP_MODULEREMOVEDUPREC_H


struct DupRecInfo
{
    wxString m_input_path;
    wxString m_output_path;
    tango::ISetPtr m_input_set;
    bool m_input_changed;
};


class RemoveDupRecWizard : public wxWindow,
                           public IDocument,
                           public xcm::signal_sink                              
{

XCM_CLASS_NAME_NOREFCOUNT("appmain.RemoveDupRecWizard")
XCM_BEGIN_INTERFACE_MAP(RemoveDupRecWizard)
    XCM_INTERFACE_ENTRY(IDocument)
XCM_END_INTERFACE_MAP()



public:
    RemoveDupRecWizard();
    virtual ~RemoveDupRecWizard();

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    // -- wizards methods --
    void onWizardCancelled(kcl::Wizard* wizard);
    void onWizardFinished(kcl::Wizard* wizard);

    void onSize(wxSizeEvent& event);

private:

    IDocumentSitePtr m_doc_site;
    DupRecInfo* m_info;

public:

    wxBoxSizer* m_sizer;
    kcl::Wizard* m_wizard;

    DECLARE_EVENT_TABLE()
};


#endif

