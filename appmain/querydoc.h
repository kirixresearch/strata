/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams; Aaron L. Williams
 * Created:  2006-10-06
 *
 */


#ifndef __APP_QUERYDOC_H
#define __APP_QUERYDOC_H


#include "relationdnd.h"
#include "panelcolumnlist.h"
#include "querytemplate.h"


// -- forward declarations --
class RelationDiagram;
class RelationBox;
class RelationLine;
class RelationLineDataObject;




xcm_interface IQueryDoc : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IQueryDoc")

public:

    virtual bool createSqlDoc() = 0;
    virtual bool updateSqlDoc() = 0;
    virtual bool execute() = 0;
    virtual bool isRunning() = 0;
    virtual bool doSave(bool force) = 0;
};

XCM_DECLARE_SMARTPTR(IQueryDoc)



// -- QueryDoc class declaration --

class QueryDoc : public wxWindow,
                 public IQueryDoc,
                 public IColumnListTarget,
                 public IDocument,
                 public StatusBarProviderBase,
                 public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.QueryDoc")
    XCM_BEGIN_INTERFACE_MAP(QueryDoc)
        XCM_INTERFACE_ENTRY(IQueryDoc)
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

    static bool newFile(const wxString& path);
    
public:

    QueryDoc();
    ~QueryDoc();

    // -- IQueryDoc --
    bool createSqlDoc();
    bool updateSqlDoc();
    bool execute();
    bool isRunning();
    bool doSave(bool force = false);
    
    bool load(const wxString& path);

private:

    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);
    wxWindow* getDocumentWindow();
    wxString getDocumentLocation();
    void setDocumentFocus();
    bool onSiteClosing(bool force);
    void onSiteActivated();

    void getColumnListItems(std::vector<ColumnListItem>& items);
    void onColumnListDblClicked(const std::vector<wxString>& items);
    
    void updateCaption();
    void updateStatusBar();
    void updateColumnList();
    void updateJoinStructure(const wxString& left_path);
    bool validateGroupQuery();
    void populateDiagramFromTemplate();
    void populateGridFromTemplate();
    void populateTemplateFromInterface();
    void checkOverlayText();
    void insertRow(int row, const wxString& input_field);
    void insertSelectedRows();
    void doSizing(const wxSize& clisize, int sash_height);
    
    // error checking functions
    void clearProblemRows();
    void markProblemRow(int row, bool scroll_to);
    int  checkDuplicateFieldnames(int check_flags);
    int  checkInvalidFieldnames(int check_flags);
    int  validateStructure();
    bool doErrorCheck();
    
    bool isChanged();
    bool isChangedExecute();
    
    void setChanged(bool changed);
    void setChangedExecute(bool changed);

private:
    
    // signal events
    void onInsertingRows(std::vector<int> rows);
    void onDeletedRows(std::vector<int> rows);
    
    // grid events
    void onGridDataDropped(kcl::GridDataDropTarget* drop_target);
    void onGridPreGhostRowInsert(kcl::GridEvent& evt);
    void onGridPreInvalidAreaInsert(kcl::GridEvent& evt);
    void onGridCellRightClick(kcl::GridEvent& evt);
    void onGridEditChange(kcl::GridEvent& evt);
    void onGridEndEdit(kcl::GridEvent& evt);

    // diagram events
    void onDiagramSetAdded(wxString path, bool* allow);
    void onDiagramSetRemoved(wxString path);
    void onDiagramLineActivated(RelationLine* line);
    void onDiagramLineRightClicked(RelationLine* line, int* command);
    void onDiagramLineAdded(RelationLine* line, bool* allowed);
    void onDiagramLineDeleted(RelationLine* line);
    void onDiagramFieldActivated(wxString path, wxString field);
    void onDiagramBoxSizedMoved(wxString path, wxRect rect);

    // frame event handlers
    void onFrameEvent(FrameworkEvent& evt);

    // other events
    void onCopy(wxCommandEvent& evt);
    void onInsertField(wxCommandEvent& evt);
    void onDeleteField(wxCommandEvent& evt);
    void onSelectAll(wxCommandEvent& evt);
    void onAddTable(wxCommandEvent& evt);
    void onSelectDistinctChecked(wxCommandEvent& evt);
    void onOutputPathTextChanged(wxCommandEvent& evt);
    void onBrowse(wxCommandEvent& evt);
    void onTreeDataDropped(FsDataObject* data);
    void onSashDragged(wxSashEvent& evt);
    void onExecute(wxCommandEvent& evt);
    void onSave(wxCommandEvent& evt);
    void onSaveAs(wxCommandEvent& evt);
    void onSaveAsExternal(wxCommandEvent& evt);
    void onSize(wxSizeEvent& evt);
    void onUpdateUI_EnableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI(wxUpdateUIEvent& evt);

private:

    IFramePtr m_frame;
    IDocumentSitePtr m_doc_site;
    
    QueryTemplate m_info;
    wxString m_path;
    bool m_changed;
    bool m_changed_execute;
    
    wxSizer* m_settings_sizer;
    wxTextCtrl* m_outputpath_textctrl;
    wxButton* m_browse_button;
    wxButton* m_add_button;
    wxCheckBox* m_selectdistinct_checkbox;
    
    wxSashWindow* m_sash;
    RelationDiagram* m_diagram;
    kcl::RowSelectionGrid* m_grid;

    IJobInfoPtr m_job_info;
    
    DECLARE_EVENT_TABLE()
};




#endif

