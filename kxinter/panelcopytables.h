/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   David Z. Williams
 * Created:  2003-08-30
 *
 */


#ifndef __KXINTER_PANELCOPYTABLES_H
#define __KXINTER_PANELCOPYTABLES_H


typedef std::pair<wxString,wxString> StringPair;


class CopyTablesPanel : public wxWindow,
                        public cfw::IDocument,
                        public xcm::signal_sink
{

XCM_CLASS_NAME_NOREFCOUNT("kxinter.CopyTablesPanel")
XCM_BEGIN_INTERFACE_MAP(CopyTablesPanel)
    XCM_INTERFACE_ENTRY(cfw::IDocument)
XCM_END_INTERFACE_MAP()


public:
    CopyTablesPanel();
    virtual ~CopyTablesPanel();

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame, cfw::IDocumentSitePtr doc_site, wxWindow* docsite_wnd, wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    // -- ExportWizard methods --
    void onWizardCancelled(kcl::Wizard* wizard);
    void onWizardFinished(kcl::Wizard* wizard);

    void onSize(wxSizeEvent& event);
    void onSourceDatabaseChanged(wxCommandEvent& event);
    void onAddOne(wxCommandEvent& event);
    void onAddAll(wxCommandEvent& event);
    void onRemoveOne(wxCommandEvent& event);
    void onRemoveAll(wxCommandEvent& event);
    void onOk(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void populate();

    DECLARE_EVENT_TABLE()

private:
    cfw::IDocumentSitePtr m_doc_site;

    kcl::Grid* m_source_grid;
    kcl::Grid* m_target_grid;

    wxComboBox* m_source_combo;
    wxComboBox* m_target_combo;

    std::vector<wxString> m_source_tables;
    std::vector<StringPair> m_to_convert;
    bool m_append;
};


#endif






