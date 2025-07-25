/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-11-29
 *
 */


#ifndef H_APP_TABLEDOC_PRIVATE_H
#define H_APP_TABLEDOC_PRIVATE_H


class AppendPanel;
class ColPropsPanel;
class ViewPanel;
class IndexPanel;
class ExprBuilderPanel;
class ReportCreateInfo;
class KeyBuilderPanel;


#include "panelcolumnlist.h"
#include "panelfind.h"
#include "xdgridmodel.h"
#include <map>
#include <deque>

// Maximum number of undo/redo operations to store
const int MAX_UNDO_REDO_STACK_SIZE = 100;

// forward declarations
namespace kscript
{
    class ExprEnv;
    class Value;
};

enum UndoActionType
{
    UndoAction_CellEdit          // Single cell edit
    // UndoAction_ColumnEdit,    // Column property change
    // UndoAction_ColumnInsert,  // Column insertion
    // UndoAction_ColumnDelete,  // Column deletion
    // UndoAction_RowInsert,     // Row insertion
    // UndoAction_RowDelete,     // Row deletion
    // UndoAction_BatchEdit      // Multiple cell edits in one operation
};

class UndoRecord
{
public:
    UndoRecord() : action_type(UndoAction_CellEdit), row(-1), model_col(-1) {}

    UndoRecord(const UndoRecord& record)
    {
        action_type = record.action_type;
        row = record.row;
        model_col = record.model_col;
        rowid = record.rowid;
        col_name = record.col_name;
        primary_key = record.primary_key;
        old_value = record.old_value;
        new_value = record.new_value;
        data_type = record.data_type;
    }
 
    UndoRecord& operator=(const UndoRecord& record)
    {
        if (this != &record)
        {
            action_type = record.action_type;
            row = record.row;
            model_col = record.model_col;
            rowid = record.rowid;
            col_name = record.col_name;
            primary_key = record.primary_key;
            old_value = record.old_value;
            new_value = record.new_value;
            data_type = record.data_type;
        }
        return *this;
    }
    
    UndoActionType action_type;  // type of change
    int row;                     // row affected
    int model_col;               // model column index affected
    xd::rowid_t rowid;           // row id of the affected row (if applicable)
    
    // For cell edits
    std::wstring col_name;       // name of the column
    std::wstring primary_key;    // name of the primary key column
    std::wstring old_value;      // previous value
    std::wstring new_value;      // new value
    int data_type;               // data type of the cell
};

enum
{
    quickFilterPending = -1,
    quickFilterNotPending = -2
};


class TableDoc : public wxWindow,
                 public IDocument,
                 public ITableDoc,
                 public IColumnListTarget,
                 public IFindTarget,
                 public IDocumentScriptBinding,
                 public StatusBarProviderBase,
                 public xcm::signal_sink
{
    XCM_CLASS_NAME_NOREFCOUNT("appmain.TableDoc")
    XCM_BEGIN_INTERFACE_MAP(TableDoc)
        XCM_INTERFACE_ENTRY(IDocument)
        XCM_INTERFACE_ENTRY(ITableDoc)
        XCM_INTERFACE_ENTRY(IColumnListTarget)
        XCM_INTERFACE_ENTRY(IFindTarget)
        XCM_INTERFACE_ENTRY(IDocumentScriptBinding)
        XCM_INTERFACE_CHAIN(StatusBarProviderBase)
    XCM_END_INTERFACE_MAP()

public:

    TableDoc();
    virtual ~TableDoc();

    // IDocument
    bool initDoc(IFramePtr frame,
                 IDocumentSitePtr doc_site,
                 wxWindow* docsite_wnd,
                 wxWindow* panesite_wnd);

    wxString getDocumentLocation();
    wxString getDocumentTitle();
    wxWindow* getDocumentWindow();
    void setDocumentFocus();
    bool onSiteClosing(bool force);
    void onSiteDeactivated();
    void onSiteActivated();

    // ITableDoc
    
    bool open(const std::wstring& path,
              xd::IIteratorPtr optional_iterator = xcm::null,
              xd::IFileInfoPtr optional_fileinfo = xcm::null);

    bool setBrowseSet(const std::wstring& path,
                      xd::IIteratorPtr optional_iterator = xcm::null);

    std::wstring getPath();
    std::wstring getBrowsePath();

    void closeSet();
    void setEnabled(bool new_val);
    bool getEnabled();
    
    void setTemporaryModel(bool new_val);
    
    void setCaption(const wxString& caption, const wxString& caption_suffix);
    wxString getCaption();
    void setSourceUrl(const wxString& source_url);
    void setSourceMimeType(const wxString& source_mimetype);

    ITableDocModelPtr getModel();

    kcl::Grid* getGrid();
    xd::IIteratorPtr getIterator();
    void setIterator(xd::IIteratorPtr iter, bool go_first = true);

    void connectAlterTableJob(jobs::IJobPtr job);

    void onColumnNameChanged(const std::wstring& old_name, const std::wstring& new_name);

    void insertColumn(int insert_pos, const wxString& col_name);
    void insertColumnInternal(int insert_pos, const wxString& col_name, bool save, bool refresh);
    void insertColumnSeparator(int insert_pos);
    void insertChildColumn(int insert_pos, const wxString& expr);
    void hideColumn(int idx);
    void copyRecords(const std::wstring& condition);
    void deleteRecords(const std::wstring& condition);
    void setFont(const wxFont& font);
    void reloadSettings(bool redraw);
    void showReplacePanel(const wxString& def_condition, const wxString& def_field = wxT(""));
    void showViewPanel();
    void showIndexPanel();
    void showCreateDynamicField();
    void showEditDynamicField(const wxString& field);
    void showFilterPanel(const wxString& condition);
    void createOrShowStructureDoc();
    bool createDynamicField(const wxString& col_name,
                            int type,
                            int width,
                            int scale,
                            const wxString& expr,
                            bool on_set);
    wxString getFindExprFromValue(const wxString& value,
                                  bool match_case,
                                  bool whole_cell);

    void flushActiveView();
    void setActiveView(ITableDocViewPtr view);
    ITableDocViewPtr getActiveView();
    void refreshActiveView(bool repaint = true);

    bool isQuickFilterPending();

    void setRelationshipSyncMarkExpr(const wxString& expr);
    void setIsChildSet(bool new_val);
    bool getIsChildSet();

    std::wstring getDbDriver();


    // exposed API

    void setFilter(const std::wstring& condition);
    void setQuickFilter(const std::wstring& val);
    std::wstring getFilter();
    void removeFilter();

    void setSortOrder(const std::wstring& new_value);
    std::wstring getSortOrder();

    void setFilterAndSort(const std::wstring& condition, const std::wstring& sort_order);

    void setGroupBreak(const std::wstring& expr);
    std::wstring getGroupBreak();

    void createNewMark(const wxString& expr);


    // IColumnListTarget

    void getColumnListItems(std::vector<ColumnListItem>& list);
    void onColumnListDblClicked(const std::vector<wxString>& items);
    
    // IFindTarget
    
    bool findNextMatch(
                    const wxString& text,
                    bool forward,
                    bool match_case,
                    bool whole);
    bool findReplaceWith(
                    const wxString& find_val,
                    const wxString& replace_val,
                    bool forward,
                    bool match_case,
                    bool whole);
    bool findReplaceAll(
                    const wxString& find_val,
                    const wxString& replace_val,
                    bool match_case,
                    bool whole);
    bool findIsReplaceAllowed();

    // IDocumentScriptBinding
    
    bool getScriptMember(const std::wstring& member,
                         kscript::Value* retval);
                         
    // public, but not on interface

    bool isExternalTable();
    bool isTemporary();
    bool canDeleteColumns(std::vector<int>& view_cols);

private:

    // scripting methods
    
    static void scriptfuncOpen(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void scriptfuncSetFilter(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void scriptfuncSetQuickFilter(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void scriptfuncGetFilter(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void scriptfuncSetSortOrder(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void scriptfuncGetSortOrder(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void scriptfuncSetGroupBreak(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    static void scriptfuncGetGroupBreak(kscript::ExprEnv* env, void* param, kscript::Value* retval);
  
private:

    // internal methods
    
    IUIContextPtr getUserInterface();
    void createModel();
    void updateCaption();
    wxString makeCaption(const wxString& title);
    void initializeDefaultView(ITableDocViewPtr view, const xd::Structure& v_struct);

    // Undo/Redo operation methods
    void pushUndoOperation(const UndoRecord& record);
    bool getUndoOperation(UndoRecord& record);
    bool getRedoOperation(UndoRecord& record);
    void clearUndoStack();
    void clearRedoStack();
    void clearUndoRedoStacks();

    xd::objhandle_t getTemporaryHandle(const wxString& expr);
    void freeTemporaryHandles();

    std::wstring getWhereExpressionForRow(int row);

    xd::Structure getStructureWithRelatedFields();

    void updateStatusSelectionSum();
    void updateStatusBar(bool row_count_update);
    void updateStatusBar();
    void resetChildWindows();
    void updateChildWindows();
    void updateChildWindowsLazy();
    void deleteSelectedRows();
    void deleteSelectedColumns();
    bool deleteSelectedRowsColumns();
    void deleteAllRelations();
    int getRelationshipSync();
    void setRelationshipSync(int state);
    void setTextWrapping(int new_value);
    void gotoRecord();
    bool findNextCell(const wxString& expr,
                      int start_column,
                      bool forward,
                      bool match_case,
                      bool whole_cell);

    void doCellEdit(xd::IDatabasePtr db,
         const wxString& col_name,
         const wxString& primary_key,
         xd::rowid_t rowid,
         const wxString& new_value);

    void getReportCreateInfo(ReportCreateInfo& data);
    bool print(const wxString& caption = wxT(""));
    bool saveAsPdf(const wxString& path);
    bool saveAsStructure(const wxString& path);

    void onSaveAsJobFinished(jobs::IJobPtr saveas_job);
    void onFilterJobFinished(jobs::IJobPtr query_job);
    void onSortJobFinished(jobs::IJobPtr query_job);
    void onFilterSortJobFinished(jobs::IJobPtr query_job);
    void onDeleteJobFinished(jobs::IJobPtr delete_job);
    void onAlterTableJobFinished(jobs::IJobPtr job);
    void onEditDynamicFieldOk(ColPropsPanel* panel);
    void onCreateDynamicFieldOk(ColPropsPanel* panel);
    void onCreateDynamicFieldCancelled(ColPropsPanel* panel);
    void onCopyRecordsOk(ExprBuilderPanel* panel);
    void onFilterOk(ExprBuilderPanel* expr_panel);
    void onDeleteRecordsOk(ExprBuilderPanel* expr_panel);
    void onIndexEditFinished(IndexPanel* panel);
    void onViewEditFinished(ViewPanel* panel);
    void onSetOrderExprEditFinished(KeyBuilderPanel* builder);

    // grid event handlers
    void onGridLinkLeftClick(kcl::GridEvent& evt);
    void onGridLinkMiddleClick(kcl::GridEvent& evt);
    void onGridColumnRightClick(kcl::GridEvent& evt);
    void onGridCellRightClick(kcl::GridEvent& evt);
    void onGridCursorMove(kcl::GridEvent& evt);
    void onGridColumnResize(kcl::GridEvent& evt);
    void onGridColumnMove(kcl::GridEvent& evt);
    void onGridNeedTooltipText(kcl::GridEvent& evt);
    void onGridSelectionChange(kcl::GridEvent& evt);
    void onGridBeginEdit(kcl::GridEvent& evt);
    void onGridEndEdit(kcl::GridEvent& evt);
    void onGridRowResize(kcl::GridEvent& evt);
    void onGridRowRightClick(kcl::GridEvent& evt);
    void onGridRowSashDblClick(kcl::GridEvent& evt);
    void onGridPreGhostRowInsert(kcl::GridEvent& evt);
    void onGridKeyDown(kcl::GridEvent& evt);
    void onColumnsDropped(kcl::GridDataDropTarget* drop);
    
    void onShareUrlRequested(wxString& url);

    // model event handlers
    void onRequestRowColors(wxColor& fgcolor, wxColor& bgcolor);
    
    // frame event handlers
    void onFrameEvent(FrameworkEvent& evt);
    void onActiveChildChanged(IDocumentSitePtr doc_site);
    
    // statusbar event handlers
    void onStatusBarItemLeftDblClick(IStatusBarItemPtr item);
    
    // reload handlers
    void onReloadDownloadFinished(jobs::IJobInfoPtr job_info);
    void onDoReloadRefresh(wxCommandEvent& evt);
    
    // repaint handler
    void onDoRefresh(wxCommandEvent& evt);
    
    // command handlers
    void onSize(wxSizeEvent& evt);
    void onSetFocus(wxFocusEvent& evt);
    void onKillFocus(wxFocusEvent& evt);
    void onEraseBackground(wxEraseEvent& evt);
    
    // Undo/Redo handlers
    void onUndo(wxCommandEvent& evt);
    void onRedo(wxCommandEvent& evt);
    
    void onSetOrder(wxCommandEvent& evt);
    void onRemoveOrder(wxCommandEvent& evt);
    void onSetOrderAscending(wxCommandEvent& evt);
    void onSetOrderDescending(wxCommandEvent& evt);
    void onSetGroupBreakExpr(wxCommandEvent& evt);
    void onRemoveGroupBreakExpr(wxCommandEvent& evt);
    void onHideColumn(wxCommandEvent& evt);
    void onCreateDynamicField(wxCommandEvent& evt);
    void onModifyDynamicField(wxCommandEvent& evt);
    void onDeleteField(wxCommandEvent& evt);
    void onWatch(wxCommandEvent& evt);
    void onRemoveAllRelationships(wxCommandEvent& evt);
    void onSummary(wxCommandEvent& evt);
    void onCopy(wxCommandEvent& evt);
    void onPaste(wxCommandEvent& evt);
    void onCut(wxCommandEvent& evt);
    void onQuickFilter(wxCommandEvent& evt);
    void onFilter(wxCommandEvent& evt);
    void onRemoveFilter(wxCommandEvent& evt);
    void onCopyRecords(wxCommandEvent& evt);
    void onAppendRecords(wxCommandEvent& evt);
    void onDeleteRecords(wxCommandEvent& evt);
    void onUpdateRecords(wxCommandEvent& evt);
    void onSelectColumn(wxCommandEvent& evt);
    void onSelectRow(wxCommandEvent& evt);
    void onSelectAll(wxCommandEvent& evt);
    void onDelete(wxCommandEvent& evt);
    void onGoTo(wxCommandEvent& evt);
    void onEditViews(wxCommandEvent& evt);
    void onEditStructure(wxCommandEvent& evt);
    void onEditIndexes(wxCommandEvent& evt);
    void onResizeColumn(wxCommandEvent& evt);
    void onResizeAllColumns(wxCommandEvent& evt);
    void onGroup(wxCommandEvent& evt);
    void onSetBreakExpr(wxCommandEvent& evt);
    void onSave(wxCommandEvent& evt);
    void onSaveAs(wxCommandEvent& evt);
    void onSaveAsExternal(wxCommandEvent& evt);
    void onReload(wxCommandEvent& evt);
    void onPrint(wxCommandEvent& evt);
    void onMakeStatic(wxCommandEvent& evt);
    void onCreateNewMark(wxCommandEvent& evt);
    void onInsertColumnSeparator(wxCommandEvent& evt);
    void onFormatChanged(wxCommandEvent& evt);
    void onShareView(wxCommandEvent& evt);

    void onUpdateUI_EnableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI_DisableAlways(wxUpdateUIEvent& evt);
    void onUpdateUI(wxUpdateUIEvent& evt);

private:

    xd::IIteratorPtr m_iter;
    ITableDocViewPtr m_active_view;

    // Undo/Redo management
    std::deque<UndoRecord> m_undo_stack;  // Stack of undoable actions
    std::deque<UndoRecord> m_redo_stack;  // Stack of redoable actions

    IFramePtr m_frame;                    // ptr to the application frame
    IDocumentSitePtr m_doc_site;          // ptr to our document site
    IXdGridModelPtr m_grid_model;         // grid's model
    ITableDocModelPtr m_model;            // our model (stores marks, views, etc)
    std::wstring m_last_model_id;
    kcl::Grid* m_grid;                    // grid control

    std::map<wxString, xd::objhandle_t> m_handle_map; // handle map for marks

    std::wstring m_caption;               // window's caption/title
    std::wstring m_caption_suffix;        // window's caption suffix
    std::wstring m_path;                  // path (if any) of the current window
    std::wstring m_browse_path;           // path of any filtered or subset
    std::wstring m_filter;                // current filter, or empty if none
    std::wstring m_sort_order;            // current sort order, or empty if none
    std::wstring m_group_break;           // current group break, or empty if none
    std::wstring m_site_name;             // name of the cfw document site
    std::wstring m_source_url;            // url that generated this tabledoc (rss, csv, etc)
    std::wstring m_source_mimetype;       // optional mimetype that describes resource type of m_source_url
    std::wstring m_reload_filename;       // used during reload/refresh operations
    std::wstring m_relsync_mark_expr;     // mark expression for context relationship syncing
    xd::rowpos_t m_stat_row_count; // last row count used by the status bar
    
    int m_text_wrapping;              // Grid::wrapDefault, Grid::wrapOn, or Grid::wrapOff

    bool m_temporary_model;           // true if the tabledoc should use a temporary model (e.g. for query results)
    int m_relationship_sync;          // true if the tabledoc should update child windows when cursor is moved
    bool m_tabledoc_enabled;          // true if the tabledoc is enabled
    bool m_default_view_created;
    bool m_override_beginedit;
    bool m_is_childset;               // true if the tabledoc is currently a synced child set
    bool m_doing_reload;              // true if the document is currently being reloaded
    
    int m_quick_filter_jobid;         // job id of a pending quick filter job; otherwise quickFilterPending/quickFilterNotPending
    
    bool m_allow_delete_menuid;       // this flag is set to enable/disable the ID_Edit_Delete menu ID
    int m_external_table;             // -1 = uninitialized; 0 = no; 1 = yes
    int m_db_type;                    // database type (xd::dbtype enum)
    xd::IDatabasePtr m_mount_db;   // native db for the table (null for native)

    DECLARE_EVENT_TABLE()
};



#endif

