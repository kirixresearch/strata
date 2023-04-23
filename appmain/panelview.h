/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2002-10-31
 *
 */


#ifndef H_APP_PANELVIEW_H
#define H_APP_PANELVIEW_H


#include "tabledocmodel.h"


struct ColumnViewInfo
{
    wxString name;
    int size;
    int alignment;
    wxColor fg_color;
    wxColor bg_color;
    bool separator;
};


struct ViewInfo
{
    wxString name;
    std::vector<ColumnViewInfo> cols;
};


class FieldListControl;
class ViewPanel : public wxPanel,
                  public IDocument,
                  public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.ViewPanel")
    XCM_BEGIN_INTERFACE_MAP(ViewPanel)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    ViewPanel(ITableDocPtr tabledoc);
    ~ViewPanel();
    
    void updateTableDocViewEnum(ITableDocPtr tabledoc);
    
    xcm::signal1<ViewPanel*> sigOkPressed;

private:

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

private:

    void populateViewsList();
    void populateVisibleFieldsList();
    void updateAvailableFieldsList();
    void checkOverlayText();
    
    void insertViewColumn(int row,
                          const wxString& col_name,
                          ColumnViewInfo* col_info = NULL);
    void refreshViewInfo();
    
    wxString getUniqueViewName();
    
private:

    void onOK(wxCommandEvent& evt);
    void onCancel(wxCommandEvent& evt);
    void onAddView(wxCommandEvent& evt);
    void onDeleteView(wxCommandEvent& evt);
    void onGridCursorMove(kcl::GridEvent& evt);
    void onGridPreGhostRowInsert(kcl::GridEvent& evt);
    void onGridPreInvalidAreaInsert(kcl::GridEvent& evt);

    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);
    
    // -- signal events --
    void onAvailableFieldsDblClicked(int row, const wxString& text);
    void onDeletingViews(std::vector<int> rows, bool* allow);
    void onDeletedViews(std::vector<int> rows);
    void onDeletedVisibleFields(std::vector<int> rows);

private:

    kcl::RowSelectionGrid* m_views_list;
    kcl::RowSelectionGrid* m_visible_fields;
    FieldListControl* m_available_fields;
    
    wxButton* m_ok_button;
    wxButton* m_delete_view_button;
    
    wxString m_set_path;
    ViewInfo* m_selected_view_info;
    ITableDocViewEnumPtr m_views;
    ITableDocViewPtr m_original_view;
    xd::Structure m_structure;
    IDocumentSitePtr m_doc_site;
    
    DECLARE_EVENT_TABLE()
};



#endif

