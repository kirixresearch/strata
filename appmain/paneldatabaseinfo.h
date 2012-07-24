/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-03-25
 *
 */
 

#ifndef __APP_PANELDATABASEINFO_H
#define __APP_PANELDATABASEINFO_H


class DatabaseInfoPanel : public wxPanel,
                          public cfw::IDocument,
                          public xcm::signal_sink
{
XCM_CLASS_NAME_NOREFCOUNT("appmain.DatabaseInfoPanel")
XCM_BEGIN_INTERFACE_MAP(DatabaseInfoPanel)
    XCM_INTERFACE_ENTRY(cfw::IDocument)
XCM_END_INTERFACE_MAP()

public:

    DatabaseInfoPanel();
    ~DatabaseInfoPanel();

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

private:

    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);

private:

    cfw::IDocumentSitePtr m_doc_site;

    wxTextCtrl* m_project_name_text;
    wxString m_project_name;

    DECLARE_EVENT_TABLE()
};



#endif

