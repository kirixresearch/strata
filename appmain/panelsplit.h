/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-10-18
 *
 */


#ifndef __APP_PANELSPLIT_H
#define __APP_PANELSPLIT_H


struct SplitInfo;
class SplitPanel : public wxPanel,
                   public IDocument,
                   public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.SplitPanel")
    XCM_BEGIN_INTERFACE_MAP(SplitPanel)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    SplitPanel();
    ~SplitPanel();

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    // event handlers
    void onSplitTypeChanged(wxCommandEvent& event);
    void onPrefixTextChanged(wxCommandEvent& event);
    void onSourceTableTextChanged(wxCommandEvent& event);
    void onTableCountTextChanged(wxCommandEvent& event);
    void onTableSizeTextChanged(wxCommandEvent& event);
    void onRowCountTextChanged(wxCommandEvent& event);
    void onBrowse(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);

    void setTextControls(int state);
    bool validate();
    void populate();

private:

    void onSourceTableDropped(wxDragResult& drag_result, FsDataObject* data);

private:

    IFramePtr m_frame;
    IDocumentSitePtr m_doc_site;
    
    wxChoice*   m_splittype_choice;
    wxTextCtrl* m_sourcetable_textctrl;
    wxButton*   m_ok_button;
    wxButton*   m_browse_button;
    wxTextCtrl* m_prefix_textctrl;
    wxTextCtrl* m_rowcount_textctrl;
    wxTextCtrl* m_tablecount_textctrl;
    wxTextCtrl* m_tablesize_textctrl;
    wxTextCtrl* m_expression_textctrl;
    
    wxBoxSizer* m_main_sizer;
    wxStaticBoxSizer* m_input_sizer;
    wxStaticBoxSizer* m_output_sizer;
    wxBoxSizer* m_rowcount_sizer;
    wxBoxSizer* m_tablecount_sizer;
    wxBoxSizer* m_tablesize_sizer;
    wxBoxSizer* m_expression_sizer;
    wxSizerItem* m_spacer;
    
    bool m_prefix_edited;
    tango::ISetPtr m_set;

    DECLARE_EVENT_TABLE()
};

#endif


