/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2006-09-29
 *
 */


#ifndef __APP_STRUCTUREDOC_H
#define __APP_STRUCTUREDOC_H


#include "panelcolumnlist.h"


// -- forward declarations --
class StructureField
{
public:

    StructureField()
    {
        name = wxEmptyString;
        type = -1;
        width = -1;
        scale = -1;
        dynamic = false;
        expr = wxEmptyString;
        pos = -1;
        original_dynamic = false;
        original = true;
    }

    StructureField(const StructureField& c)
    {
        name = c.name;
        type = c.type;
        width = c.width;
        scale = c.scale;
        dynamic = c.dynamic;
        expr = c.expr;
        pos = c.pos;
        original_dynamic = c.original_dynamic;
        original = c.original;
    }

    StructureField& operator=(const StructureField& c)
    {
        name = c.name;
        type = c.type;
        width = c.width;
        scale = c.scale;
        dynamic = c.dynamic;
        expr = c.expr;
        pos = c.pos;
        original_dynamic = c.original_dynamic;
        original = c.original;
        return *this;
    }

public:

    wxString name;
    int type;
    int width;
    int scale;
    wxString expr;
    
    int pos;
    bool dynamic;
    bool original_dynamic;  // true if the field was a dynamic field in the original structure
    bool original;          // true if the field existed in the original structure
};




xcm_interface IStructureDoc : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IStructureDoc")

public:

    virtual void setModifySet(tango::ISetPtr set) = 0;
    virtual tango::ISetPtr getModifySet() = 0;
    virtual bool doSave() = 0;
};

XCM_DECLARE_SMARTPTR(IStructureDoc)



// -- StructureDoc class declaration --

class StructureDoc : public wxWindow,
                     public IStructureDoc,
                     public IColumnListTarget,
                     public IDocument,
                     public StatusBarProviderBase,
                     public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.StructureDoc")
    XCM_BEGIN_INTERFACE_MAP(StructureDoc)
        XCM_INTERFACE_ENTRY(IStructureDoc)
        XCM_INTERFACE_ENTRY(IColumnListTarget)
        XCM_INTERFACE_ENTRY(IDocument)
        XCM_INTERFACE_CHAIN(StatusBarProviderBase)
    XCM_END_INTERFACE_MAP()

public:

    enum ErrorCheckFlags
    {
        CheckDefault         = 0x00,   // will not mark rows
        CheckMarkRows        = 0x01,   // marks rows with errors
        CheckEmptyFieldnames = 0x02    // flag empty fieldnames as invalid
    };
    
public:
    
    StructureDoc();
    virtual ~StructureDoc();

    // -- IStructureDoc --
    void setModifySet(tango::ISetPtr modify_set);
    tango::ISetPtr getModifySet();
    bool doSave();

private:

    // -- IDocument --
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow() { return static_cast<wxWindow*>(this); }
    wxString getDocumentLocation();
    void setDocumentFocus();
    bool onSiteClosing(bool force);
    void onSiteActivated();
    
    // -- IColumnListTarget --
    void getColumnListItems(std::vector<ColumnListItem>& items);
    void onColumnListDblClicked(const std::vector<wxString>& items);

    void insertRow(int row, bool dynamic = false);
    void insertSelectedRows(bool dynamic = false);
    
    void updateNumberColumn();
    void updateExpressionIcon(int row, bool editing, int validation_res);
    void updateRowWidthAndScale(int row);
    void updateRowCellProps(int row);
    void updateCaption();
    void updateStatusBar();
    void checkOverlayText();
    
    // error checking functions
    bool doErrorCheck();
    int validateStructure();
    int validateExpression(const wxString& expr, int type = tango::typeUndefined);
    int checkInvalidExpressions(int check_flags);    
    int checkInvalidFieldnames(int check_flags);
    int checkDuplicateFieldnames(int check_flags);
    void markProblemField(int row, bool scroll_to);
    void markProblemFormula(int row, bool scroll_to);     
    void clearProblemRows();

    // structural
    bool createTable();
    tango::IStructurePtr createStructureFromGrid();
    void populateGridFromSet(tango::ISetPtr set);
    
    bool isChanged();
    void setChanged(bool changed);
    
private:

    void onAlterTableJobFinished(jobs::IJobPtr job);
    
    // -- frame event handlers --
    void onFrameEvent(FrameworkEvent& evt);
    
    // -- signal events --
    void onInsertingRows(std::vector<int> rows);
    void onDeletedRows(std::vector<int> rows);
    
    // -- events --
    void onSize(wxSizeEvent& evt);
    void onEraseBackground(wxEraseEvent& evt);

    void onSave(wxCommandEvent& evt);
    void onCopy(wxCommandEvent& evt);
    void onInsertField(wxCommandEvent& evt);
    void onInsertDynamicField(wxCommandEvent& evt);
    void onConvertDynamicToFixed(wxCommandEvent& evt);
    void onDeleteField(wxCommandEvent& evt);
    void onSelectAll(wxCommandEvent& evt);
    
    void onGridNeedTooltipText(kcl::GridEvent& evt);
    void onGridPreGhostRowInsert(kcl::GridEvent& evt);
    void onGridPreInvalidAreaInsert(kcl::GridEvent& evt);
    void onGridBeginEdit(kcl::GridEvent& evt);
    void onGridEndEdit(kcl::GridEvent& evt);
    void onGridEditChange(kcl::GridEvent& evt);
    void onGridCellRightClick(kcl::GridEvent& evt);
    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);

    void onUpdateUI_EnableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI(wxUpdateUIEvent& evt);

private:

    tango::ISetPtr m_modify_set;
    tango::IStructurePtr m_expr_edit_structure;
    
    IFramePtr m_frame;
    IDocumentSitePtr m_doc_site;
    IDocumentSitePtr m_target_site;

    wxString m_path;
    bool m_changed;
    bool m_modify;
    bool m_readonly;
    int m_last_selected_fieldtype;

    kcl::RowSelectionGrid* m_grid;

    DECLARE_EVENT_TABLE()
};



#endif


