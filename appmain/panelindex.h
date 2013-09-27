/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-05-20
 *
 */


#ifndef __APP_PANELINDEX_H
#define __APP_PANELINDEX_H


#include "tabledoc.h"


struct IndexColumnInfo
{
    wxString name;
    bool ascending;
};


struct IndexInfo
{
    wxString orig_name;
    wxString name;
    wxString expr;
    std::vector<IndexColumnInfo> cols;
};


class FieldListControl;
class IndexPanel :  public wxPanel,
                    public IDocument,
                    public xcm::signal_sink
{

XCM_CLASS_NAME_NOREFCOUNT("appmain.IndexPanel")
XCM_BEGIN_INTERFACE_MAP(IndexPanel)
    XCM_INTERFACE_ENTRY(IDocument)
XCM_END_INTERFACE_MAP()

public:

    IndexPanel();
    ~IndexPanel();

    bool setPath(const std::wstring& path);

    std::vector<IndexInfo*> getAllIndexes();
    
    xcm::signal1<IndexPanel*> sigOkPressed;

private:

    // IDocument 
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr site,
                 wxWindow* doc_site,
                 wxWindow* pane_site);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

private:

    void populateIndexesList();
    void populateIndexFieldsList();
    
    void checkEnabled();
    void checkOverlayText();
    
    void insertIndexColumn(int row,
                           const wxString& col_name,
                           IndexColumnInfo* col_info = NULL);
    void refreshIndexInfo();
    
    wxString getUniqueIndexName();

private:

    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onAddIndex(wxCommandEvent& evt);
    void onDeleteIndex(wxCommandEvent& evt);
    void onGridCursorMove(kcl::GridEvent& evt);
    void onGridPreGhostRowInsert(kcl::GridEvent& evt);
    void onGridPreInvalidAreaInsert(kcl::GridEvent& evt);

    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);
    
    // signal events
    void onAvailableFieldsDblClicked(int row, const wxString& text);
    void onDeletingIndexes(std::vector<int> rows, bool* allow);
    void onDeletedIndexes(std::vector<int> rows);
    void onDeletedIndexFields(std::vector<int> rows);

private:

    kcl::GridActionValidator* m_indexes_list_validator;
    kcl::RowSelectionGrid* m_indexes_list;
    kcl::RowSelectionGrid* m_index_fields;
    FieldListControl* m_available_fields;
    
    wxButton* m_ok_button;
    wxButton* m_delete_index_button;
    
    IndexInfo* m_selected_index_info;

    std::wstring m_path;
    xd::IStructurePtr m_structure;
    IDocumentSitePtr m_doc_site;
    
    DECLARE_EVENT_TABLE()
};


#endif


