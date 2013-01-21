/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-08-12
 *
 */


#ifndef __APP_PANELREPLACEROWS_H
#define __APP_PANELREPLACEROWS_H


class ExprBuilderPanel;
class ReplaceRowsPanel :  public wxPanel,
                          public IDocument,
                          public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ReplaceRowsPanel")
    XCM_BEGIN_INTERFACE_MAP(ReplaceRowsPanel)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    ReplaceRowsPanel();
    ~ReplaceRowsPanel();
    void setParams(tango::ISetPtr set, const wxString& expr, const wxString& field = wxT(""));

private:

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    void populate();
    bool validate(bool* value = NULL);
    bool isValidValue();
    void checkEnableRun();

    void onFieldChoiceChanged(wxCommandEvent& evt);
    void onReplaceTextChanged(wxCommandEvent& evt);
    void onCancelPressed(ExprBuilderPanel* panel);
    void onOKPressed(ExprBuilderPanel* panel);

private:

    tango::ISetPtr m_set;
    tango::IIteratorPtr m_iter;
    IDocumentSitePtr m_doc_site;
    
    wxChoice* m_field_choice;
    wxTextCtrl* m_replace_text;
    kcl::ValidControl* m_valid_control;
    ExprBuilderPanel* m_expr_panel;

    DECLARE_EVENT_TABLE()
};


#endif


