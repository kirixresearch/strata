/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-03-28
 *
 */


#ifndef __APP_PANELMARKMGR_H
#define __APP_PANELMARKMGR_H


#include "tabledoc.h"

class DlgExprBuilder;




xcm_interface IMarkMgr : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IMarkMgr")

public:

    virtual void editMark(int mark) = 0;
    virtual void populate(bool refresh) = 0;
};

XCM_DECLARE_SMARTPTR(IMarkMgr)



class MarkMgrPanel  : public wxPanel,
                      public IMarkMgr,
                      public IDocument,
                      public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.MarkMgrPanel")
    XCM_BEGIN_INTERFACE_MAP(MarkMgrPanel)
        XCM_INTERFACE_ENTRY(IMarkMgr)
        XCM_INTERFACE_ENTRY(IDocument)
    XCM_END_INTERFACE_MAP()

public:

    MarkMgrPanel();
    ~MarkMgrPanel();

    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    void setDocumentFocus();

private:

    // panel methods
    void checkOverlayText();
    void populate(bool refresh = true);
    void populateRow(int row, ITableDocMarkPtr mark);
    void editMark(int mark);
    
    wxString getUniqueMarkName();

private:

    void onEditMark(wxCommandEvent& event);
    void onAddMark(wxCommandEvent& event);
    void onDeleteMark(wxCommandEvent& event);
    void onFilterRows(wxCommandEvent& event);
    void onCopyRows(wxCommandEvent& event);
    void onReplaceRows(wxCommandEvent& event);
    void onDeleteRows(wxCommandEvent& event);
    void onSize(wxSizeEvent& event);
    void onSetFocus(wxFocusEvent& event);
    void onKillFocus(wxFocusEvent& event);
    
    // RowSelectionGrid signal events
    void onDeletingRows(std::vector<int> rows, bool* allow);
    void onDeletedRows(std::vector<int> rows);
    
    // frame events
    void onActiveChildChanged(IDocumentSitePtr doc_site);
    void onFrameEvent(FrameworkEvent& evt);

    // grid events
    void onGridPreGhostRowInsert(kcl::GridEvent& evt);
    void onGridPreInvalidAreaInsert(kcl::GridEvent& evt);
    void onGridModelChange(kcl::GridEvent& evt);
    void onGridEditExpressionClick(kcl::GridEvent& evt);
    void onGridColorClick(kcl::GridEvent& evt);
    void onGridCellRightClick(kcl::GridEvent& evt);
    void onGridBeginEdit(kcl::GridEvent& evt);

    // expr builder events
    void onExprEditFinished(wxString);
    void onExprEditCancelled();

    // color panel events
    void onColorPanelDestructing(kcl::ColorPanel* panel);
    void onColorSelected(wxColour color, int button);

private:

    IFramePtr m_frame;
    IDocumentSitePtr m_doc_site;
    
    ITableDocMarkEnumPtr m_markvec;

    DlgExprBuilder* m_builder;
    kcl::RowSelectionGrid* m_grid;
    kcl::ColorPanel* m_color_panel;
    kcl::PopupContainer* m_color_panel_cont;

    bool m_ghost_inserting;
    bool m_fgcolor_editing;
    
    int m_cli_width;
    int m_cli_height;
    
    int m_color_column;
    int m_edit_mark;

    DECLARE_EVENT_TABLE()
};

#endif

