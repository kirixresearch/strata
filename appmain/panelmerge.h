/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2002-10-18
 *
 */


#ifndef __APP_PANELMERGE_H
#define __APP_PANELMERGE_H


class MergePanel :  public wxPanel,
                    public cfw::IDocument,
                    public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.MergePanel")
    XCM_BEGIN_INTERFACE_MAP(MergePanel)
        XCM_INTERFACE_ENTRY(cfw::IDocument)
    XCM_END_INTERFACE_MAP()

public:

    MergePanel();
    ~MergePanel();

    // -- IDocument --
    bool initDoc(cfw::IFramePtr frame,
                 cfw::IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

    void setAppend(const wxString& append_path);
    
    // event handlers
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onBrowse(wxCommandEvent& event);
    void onSize(wxSizeEvent& event);

private:
    
    // -- signal events --
    void onDeletedRows(std::vector<int> rows);

    // event handlers
    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);
    void onOutputPathDropped(wxDragResult& drag_result, cfw::FsDataObject* data);
    void checkOverlayText();

private:

    cfw::IFramePtr m_frame;
    cfw::IDocumentSitePtr m_doc_site;
    tango::ISetPtr m_set;
    
    wxTextCtrl* m_output_table;
    kcl::RowSelectionGrid* m_grid;
    wxButton* m_ok_button;
    
    bool m_append;
    wxString m_append_path;
    wxString m_output_path;

    DECLARE_EVENT_TABLE()
};

#endif


