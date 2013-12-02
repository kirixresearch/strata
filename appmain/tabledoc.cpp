/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2001-11-15
 *
 */


#include "appmain.h"
#include "app.h"
#include "appcontroller.h"
#include "tabledoc.h"
#include "tabledoc_private.h"
#include "xdgridmodel.h"
#include "exprbuilder.h"
#include "feedparser.h"
#include "panelcolprops.h"
#include "panelmarkmgr.h"
#include "panelreplacerows.h"
#include "panelrelationship.h"
#include "panelindex.h"
#include "panelview.h"
#include "panelgroup.h"
#include "panelexprbuilderdoc.h"
#include "panelcolordoc.h"
#include "panelmerge.h"
#include "panelfind.h"
#include "reportdoc.h"
#include "dlgdatabasefile.h"
#include "dlgshareview.h"
#include "toolbars.h"
#include "textdoc.h"
#include "transformationdoc.h"
#include "structuredoc.h"
#include "querydoc.h"
#include "sqldoc.h"
#include "webdoc.h"
#include "exporttemplate.h"
#include "../kcl/griddnd.h"
#include <algorithm>
#include <set>
#include <kl/utf8.h>
#include <kl/thread.h>
#include <kl/string.h>

enum
{
    ID_DoReloadRefresh = 19419,
    ID_DoRefresh = 19420
};


BEGIN_EVENT_TABLE(TableDoc, wxWindow)
    EVT_MENU(ID_DoReloadRefresh, TableDoc::onDoReloadRefresh)
    EVT_MENU(ID_DoRefresh, TableDoc::onDoRefresh)
    EVT_MENU(ID_Data_Sort, TableDoc::onSetOrder)
    EVT_MENU(ID_Data_RemoveSort, TableDoc::onRemoveOrder)
    EVT_MENU(ID_Data_SortAscending, TableDoc::onSetOrderAscending)
    EVT_MENU(ID_Data_SortDescending, TableDoc::onSetOrderDescending)
    EVT_MENU(ID_Data_MakeDynamicFieldStatic, TableDoc::onMakeStatic)
    EVT_MENU(ID_Data_QuickFilter, TableDoc::onQuickFilter)
    EVT_MENU(ID_Data_Filter, TableDoc::onFilter)
    EVT_MENU(ID_Data_CopyRecords, TableDoc::onCopyRecords)
    EVT_MENU(ID_Data_AppendRecords, TableDoc::onAppendRecords)
    EVT_MENU(ID_Data_UpdateRecords, TableDoc::onUpdateRecords)
    EVT_MENU(ID_Data_DeleteRecords, TableDoc::onDeleteRecords)
    EVT_MENU(ID_Data_RemoveSortFilter, TableDoc::onRemoveFilter)
    EVT_MENU(ID_Data_CreateDynamicField, TableDoc::onCreateDynamicField)    
    EVT_MENU(ID_Data_ModifyDynamicField, TableDoc::onModifyDynamicField)
    EVT_MENU(ID_Data_DeleteField, TableDoc::onDeleteField)
    EVT_MENU(ID_Data_MarkRecords, TableDoc::onCreateNewMark)
    EVT_MENU(ID_Data_GroupRecords, TableDoc::onGroup)
    EVT_MENU(ID_Data_EditStructure, TableDoc::onEditStructure)
    EVT_MENU(ID_Data_EditIndexes, TableDoc::onEditIndexes)
    EVT_MENU(ID_Data_Summary, TableDoc::onSummary)
    EVT_MENU(ID_Edit_Cut, TableDoc::onCut)
    EVT_MENU(ID_Edit_Copy, TableDoc::onCopy)
    EVT_MENU(ID_Edit_Paste, TableDoc::onPaste)
    EVT_MENU(ID_Edit_SelectAll, TableDoc::onSelectAll)
    EVT_MENU(ID_Edit_Delete, TableDoc::onDelete)
    EVT_MENU(ID_Edit_GoTo, TableDoc::onGoTo)
    EVT_MENU(ID_Table_SelectRow, TableDoc::onSelectRow)
    EVT_MENU(ID_Table_SelectColumn, TableDoc::onSelectColumn)
    EVT_MENU(ID_Table_EditViews, TableDoc::onEditViews)
    EVT_MENU(ID_Table_HideColumn, TableDoc::onHideColumn)
    EVT_MENU(ID_Table_InsertColumnBreak, TableDoc::onInsertColumnSeparator)
    EVT_MENU(ID_Table_InsertGroupBreak, TableDoc::onSetGroupBreakExpr)
    EVT_MENU(ID_Table_RemoveGroupBreak, TableDoc::onRemoveGroupBreakExpr)
    EVT_MENU(ID_File_Save, TableDoc::onSave)
    EVT_MENU(ID_File_SaveAs, TableDoc::onSaveAs)
    EVT_MENU(ID_File_SaveAsExternal, TableDoc::onSaveAsExternal)
    EVT_MENU(ID_File_Reload, TableDoc::onReload)
    EVT_MENU(ID_File_Print, TableDoc::onPrint)
    EVT_MENU(ID_File_ShareView, TableDoc::onShareView)

    EVT_MENU(ID_Format_ToggleWrapText, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_ToggleMergeCells, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_ToggleFontWeight, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_ToggleFontStyle, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_ToggleFontUnderline, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignTextLeft, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignTextCenter, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignTextRight, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignTop, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignMiddle, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_AlignBottom, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_FillColor, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_TextColor, TableDoc::onFormatChanged)
    EVT_MENU(ID_Format_LineColor, TableDoc::onFormatChanged)
    
    // disable canvas object items
    EVT_UPDATE_UI_RANGE(ID_Canvas_First, ID_Canvas_Last, TableDoc::onUpdateUI_DisableAlways)

    // disable the zoom
    EVT_UPDATE_UI(ID_View_ZoomCombo, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomIn, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomOut, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToFit, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToActual, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToWidth, TableDoc::onUpdateUI_DisableAlways)

    // disable some text format items
    EVT_UPDATE_UI(ID_Format_Settings, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Format_FontFace_Combo, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Format_FontSize_Combo, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Format_ToggleFontWeight, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Format_ToggleFontStyle, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Format_ToggleFontUnderline, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Format_Border, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Format_ToggleWrapText, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Format_ToggleMergeCells, TableDoc::onUpdateUI_DisableAlways)

    // disable some table items; note: delete columns and delete rows are disabled
    // because deleting columns and rows in the tabledoc correspond to deleting
    // fields and records, which are data items, and are enabled
    EVT_UPDATE_UI(ID_Table_DeleteColumns, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Table_InsertRows, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Table_DeleteRows, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_Table_ClearCells, TableDoc::onUpdateUI_DisableAlways)

    // disable some of the file items
    EVT_UPDATE_UI(ID_File_PageSetup, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Print, TableDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Run, TableDoc::onUpdateUI_DisableAlways)

    // enable/disable data/edit/group break items based on conditions
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, TableDoc::onUpdateUI)
    EVT_UPDATE_UI_RANGE(ID_Edit_First, ID_Edit_Last, TableDoc::onUpdateUI)
    EVT_UPDATE_UI(ID_Table_InsertGroupBreak, TableDoc::onUpdateUI)
    EVT_UPDATE_UI(ID_Table_RemoveGroupBreak, TableDoc::onUpdateUI)

    EVT_SIZE(TableDoc::onSize)
    EVT_SET_FOCUS(TableDoc::onSetFocus)
    EVT_KILL_FOCUS(TableDoc::onKillFocus)
    EVT_ERASE_BACKGROUND(TableDoc::onEraseBackground)
    EVT_KCLGRID_LINK_LEFTCLICK(TableDoc::onGridLinkLeftClick)
    EVT_KCLGRID_LINK_MIDDLECLICK(TableDoc::onGridLinkMiddleClick)
    EVT_KCLGRID_SELECTION_CHANGE(TableDoc::onGridSelectionChange)
    EVT_KCLGRID_BEGIN_EDIT(TableDoc::onGridBeginEdit)
    EVT_KCLGRID_END_EDIT(TableDoc::onGridEndEdit)
    EVT_KCLGRID_CELL_RIGHT_CLICK(TableDoc::onGridCellRightClick)
    EVT_KCLGRID_COLUMN_RIGHT_CLICK(TableDoc::onGridColumnRightClick)
    EVT_KCLGRID_CURSOR_MOVE(TableDoc::onGridCursorMove)
    EVT_KCLGRID_COLUMN_RESIZE(TableDoc::onGridColumnResize)
    EVT_KCLGRID_ROW_RESIZE(TableDoc::onGridRowResize)
    EVT_KCLGRID_ROW_RIGHT_CLICK(TableDoc::onGridRowRightClick)
    EVT_KCLGRID_ROW_SASH_DCLICK(TableDoc::onGridRowSashDblClick)
    EVT_KCLGRID_COLUMN_MOVE(TableDoc::onGridColumnMove)
    EVT_KCLGRID_NEED_TOOLTIP_TEXT(TableDoc::onGridNeedTooltipText)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(TableDoc::onGridPreGhostRowInsert)
    EVT_KCLGRID_KEY_DOWN(TableDoc::onGridKeyDown)
END_EVENT_TABLE()


// helper functions

static void setColumnProps(wxColor* fill_color,
                           wxColor* text_color = NULL,
                           int alignment = -1);

static wxString buildSelectedColumnExpression(kcl::Grid* grid, bool descending = false)
{
    wxString expr;
    kcl::IModelPtr model = grid->getModel();

    int col_count = grid->getColumnCount();
    for (int i = 0; i < col_count; ++i)
    {
        if (grid->isColumnSelected(i))
        {
            // if the column is a separator, move on
            int model_idx = grid->getColumnModelIdx(i);
            if (model_idx == -1)
                continue;

            // add commas after every column added,
            // except the first
            if (!expr.IsEmpty())
                expr += wxT(",");

            // add the column
            std::wstring col_name = towstr(model->getColumnInfo(model_idx)->getName());
            expr += xd::quoteIdentifierIfNecessary(g_app->getDatabase(), col_name);

            // if the descending flag is set, add the
            // descending keyword
            if (descending)
                expr += " DESC";
        }
    }

    return expr;
}

static wxString getCursorColumnName(kcl::Grid* grid)
{
    kcl::IModelPtr model = grid->getModel();
    if (model.isNull())
        return wxT("");

    int col_idx = grid->getCursorColumn();
    int model_idx = grid->getColumnModelIdx(col_idx);

    return model->getColumnInfo(model_idx)->getName();
}



// this function is necessary to compare views
// that may have a different object ids

static bool compareViews(ITableDocView* v1, ITableDocView* v2)
{
    // check column count
    int v1_col_count = v1->getColumnCount();
    int v2_col_count = v2->getColumnCount();
    if (v1_col_count != v2_col_count)
        return false;
    
    // check row size
    int v1_row_size = v1->getRowSize();
    int v2_row_size = v2->getRowSize();
    if (v1_row_size != v2_row_size)
        return false;
    
    // check description
    wxString v1_desc = v1->getDescription();
    wxString v2_desc = v2->getDescription();
    if (v1_desc.Cmp(v2_desc) != 0)
        return false;
    
    // dig down to the columns
    int i, col_count = v1_col_count;
    for (i = 0; i < col_count; ++i)
    {
        ITableDocViewColPtr v1_col = v1->getColumn(i);
        ITableDocViewColPtr v2_col = v2->getColumn(i);
        
        wxString v1_col_name = v1_col->getName();
        wxString v2_col_name = v2_col->getName();
        if (v1_col_name.CmpNoCase(v2_col_name) != 0)
            return false;
        
        int v1_col_size = v1_col->getSize();
        int v2_col_size = v2_col->getSize();
        if (v1_col_size != v2_col_size)
            return false;
        
        wxColor v1_col_fg_color = v1_col->getForegroundColor();
        wxColor v2_col_fg_color = v2_col->getForegroundColor();
        if (v1_col_fg_color != v2_col_fg_color)
            return false;
        
        wxColor v1_col_bg_color = v1_col->getBackgroundColor();
        wxColor v2_col_bg_color = v2_col->getBackgroundColor();
        if (v1_col_bg_color != v2_col_bg_color)
            return false;
        
        int v1_col_align = v1_col->getAlignment();
        int v2_col_align = v2_col->getAlignment();
        if (v1_col_align != v2_col_align)
            return false;
        
        int v1_col_text_wrap = v1_col->getTextWrap();
        int v2_col_text_wrap = v2_col->getTextWrap();
        if (v1_col_text_wrap != v2_col_text_wrap)
            return false;
    }
    
    // views are identical
    return true;
}

static std::wstring getOrderExprFromJobParam(kl::JsonNode order_node)
{
    if (!order_node.isOk())
        return L"";

    std::wstring result;

    std::vector<kl::JsonNode> order_children_node = order_node.getChildren();
    std::vector<kl::JsonNode>::iterator it, it_end;
    it_end = order_children_node.end();

    bool first = true;
    for (it = order_children_node.begin(); it != it_end; ++it)
    {
        if (!first)
            result += L",";
        first = false;

        std::wstring expression = it->getChild(L"expression").getString();
        std::wstring direction = it->getChild(L"direction").getString();

        result += expression;
        if (kl::iequals(direction, L"DESC"))
            result += L" DESC";
    }

    return result;
}


// -- TableDoc class implementation --

TableDoc::TableDoc()
{
    m_grid = NULL;
    m_caption = wxT("");

    createModel();

    m_temporary_model = false;
    m_relationship_sync = g_app->getAppController()->getRelationshipSync();
    m_is_childset = false;
    m_enabled = true;
    m_default_view_created = false;
    m_override_beginedit = false;
    m_text_wrapping = tabledocWrapOff;
    m_stat_row_count = (xd::rowpos_t)-1;
    m_allow_delete_menuid = false;
    m_external_table = -1;
    m_doing_reload = false;
    m_quick_filter_jobid = quickFilterNotPending;
    
    m_db_type = xd::dbtypeXdnative;
}


TableDoc::~TableDoc()
{
    freeTemporaryHandles();

    m_model.clear();
    TableDocMgr::cleanupModelRegistry();
}

void TableDoc::onEraseBackground(wxEraseEvent& evt)
{
}

void TableDoc::onSize(wxSizeEvent& evt)
{
    if (m_grid)
    {
        m_grid->SetSize(evt.GetSize());
    }
}

void TableDoc::onSetFocus(wxFocusEvent& evt)
{
    setDocumentFocus();
}

void TableDoc::onKillFocus(wxFocusEvent& evt)
{
}


void TableDoc::setTemporaryModel(bool new_val)
{
    m_temporary_model = new_val;
}

void TableDoc::createModel()
{
    XdGridModel* model = new XdGridModel;
    model->sigRequestRowColors.connect(this, &TableDoc::onRequestRowColors);

    // set the boolean checkbox preference for the grid
    bool bool_checkbox = g_app->getAppPreferences()->getBoolean(wxT("grid.boolean_checkbox"), true);
    model->setBooleanCheckbox(bool_checkbox);

    m_grid_model = static_cast<IXdGridModel*>(model);
}

bool TableDoc::isExternalTable()
{
    // m_external_table is a cache variable which allows
    // subsequent calls to isExternalTable() to operate 
    // quickly
    
    if (m_external_table == -1)
    {
        std::wstring db_driver = getDbDriver();
        m_external_table = (db_driver != L"xdnative" && db_driver != L"xdclient" && db_driver != L"xdcommon") ? 1 : 0;
    }
    
    return (m_external_table == 1) ? true : false;
}

bool TableDoc::isTemporary()
{
    std::wstring table_prefix = kl::afterLast(m_path, '/');
    table_prefix = table_prefix.substr(0, 5);
    if (kl::iequals(table_prefix, L"xtmp_"))
        return true;
         else
        return false;
}


bool TableDoc::canDeleteColumns(std::vector<int>& view_cols)
{
    // allow column deletion for the native driver only
    if (!isExternalTable())
        return true;
    
    // we're checking an external set
    
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    xd::IStructurePtr structure = db->describeTable(m_path);
    if (structure.isNull())
        return false;
    
    kcl::Grid* grid = getGrid();
        
    // iterate through the view columns to determine
    // if any of them are fixed fields
    std::vector<int>::iterator it;
    for (it = view_cols.begin(); it != view_cols.end(); ++it)
    {
        // couldn't find the model column, bail out
        int model_col = grid->getColumnModelIdx(*it);
        if (model_col == -1)
            return false;
        
        // don't allow deletes on static fields in external sets
        xd::IColumnInfoPtr colinfo;
        colinfo = structure->getColumnInfoByIdx(model_col);
        if (colinfo.isNull() || !colinfo->getCalculated())
            return false;
    }
    
    return true;
}

IUIContextPtr TableDoc::getUserInterface()
{
    IUIContextPtr ui_context;

    // see if a user interface of this type already exists
    ui_context = m_frame->lookupUIContext(wxT("TableDocUI"));
    if (ui_context.isOk())
        return ui_context;
    
    // no, so create one
    ui_context = m_frame->createUIContext(wxT("TableDocUI"));
    return ui_context;
}



bool TableDoc::initDoc(IFramePtr frame,
                       IDocumentSitePtr doc_site,
                       wxWindow* docsite_wnd,
                       wxWindow* panesite_wnd)
{
    m_frame = frame;
    m_doc_site = doc_site;

    doc_site->setCaption(m_caption);
    doc_site->setBitmap(GETBMP(gf_table_16));

    // create document's window
    bool result = Create(docsite_wnd,
                         -1,
                         wxPoint(0,0),
                         docsite_wnd->GetClientSize(),
                         wxNO_FULL_REPAINT_ON_RESIZE);

    if (!result)
        return false;

    SetBackgroundColour(wxColor(255,255,255));


    int w, h;
    GetClientSize(&w, &h);

    // create the grid control
    m_grid = new kcl::Grid;
    m_grid->setBorderType(wxNO_BORDER);

    // create a model for the grid
    m_grid->setModel(m_grid_model);
    m_grid->setOptionState(kcl::Grid::optGhostRow, true);
    m_grid->setOptionState(kcl::Grid::optCursorRowMarker, true);

    // show the grid
    m_grid->Create(this,
                   -1,
                   wxPoint(0,0),
                   wxSize(w,h),
                   0,
                   kcl::Grid::stateHidden);

    // set the selection foreground color
    m_grid->setSelectForegroundColor(wxNullColour);

    // set the selection background color (same as toolbar,
    // preferences, job selection)
    wxColour highlight_colour;
    highlight_colour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    highlight_colour = kcl::stepColor(highlight_colour, 170);
    m_grid->setSelectBackgroundColor(highlight_colour);

    // turn on continuous scrolling
    m_grid->setOptionState(kcl::Grid::optContinuousScrolling, true);

    // load a font for the grid
    reloadSettings(false);

    // set view
    refreshActiveView();
    if (m_default_view_created)
    {
        m_grid->autoColumnResize(-1);
        m_model->writeObject(m_active_view);
        m_default_view_created = false;
    }

    // show the grid
    m_grid->setVisibleState(kcl::Grid::stateVisible);

    // set up drop target to receive columns from column list
    kcl::GridDataDropTarget* drop_target;
    drop_target = new kcl::GridDataDropTarget(m_grid, kcl::GridScrollHorizontal);
    drop_target->setHintOrientation(wxVERTICAL);
    drop_target->setGridDataObjectFormats(wxT("fieldspanel"));
    drop_target->sigDropped.connect(this, &TableDoc::onColumnsDropped);
    m_grid->SetDropTarget(drop_target);

    // load our ui
    m_doc_site->setUIContext(getUserInterface());

    // connect the frame event signal
    frame->sigFrameEvent().connect(this, &TableDoc::onFrameEvent);
    frame->sigActiveChildChanged().connect(this, &TableDoc::onActiveChildChanged);
    
    // connect statusbar signals
    g_app->getMainFrame()->getStatusBar()->sigItemLeftDblClick().connect(
                                this, &TableDoc::onStatusBarItemLeftDblClick);

    // create the statusbar items for this document
    IStatusBarItemPtr item;
    
    item = addStatusBarItem(wxT("tabledoc_sum"));
    item->setWidth(110);
    item->show(false);
    
    item = addStatusBarItem(wxT("tabledoc_position"));
    item->setWidth(120);
    
    item = addStatusBarItem(wxT("tabledoc_record_count"));
    item->setWidth(150);
    
    return true;
}

void TableDoc::setDocumentFocus()
{
    if (m_grid)
    {
        m_grid->SetFocus();
    }
}

wxString TableDoc::getDocumentLocation()
{
    IDocumentPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
    if (textdoc)
        return textdoc->getDocumentLocation();
    
    IDocumentPtr querydoc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
    if (querydoc)
        return querydoc->getDocumentLocation();
    
    if (m_source_url.length() > 0)
        return m_source_url;
    
    return m_path;
}

wxString TableDoc::getDocumentTitle()
{
    return m_caption;
}


// overrides the value returned by getDocumentLocation
void TableDoc::setSourceUrl(const wxString& source_url)
{
    m_source_url = source_url;
}

void TableDoc::setSourceMimeType(const wxString& source_mimetype)
{
    m_source_mimetype = source_mimetype;
}





bool TableDoc::onSiteClosing(bool force)
{
    if (isTemporary() && !force)
    {
        // note: temporarily disable warning on closing unsaved data sets; the reason
        // for this is that many more untitled sets are created casually from the
        // web (html/rss/etc) simply by clicking, and everytime these are closed,
        // this warning pops up, constantly interrupting the user; if we want to have
        // this warning back, we could add it if the user begins to do operations on
        // the file, such as creating calculated fields, or sorting/filtering, etc; this
        // would parallel the script document, which doesn't pop up a warning message
        // for new untitled files unless they begin modifying the document
        
        int result = wxNO;
        /*
        int result;
        result = appMessageBox(
                               _("Would you like to save this table?"),
                               APPLICATION_NAME,
                               wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);
        */
        
        if (result == wxYES)
        {
            DlgDatabaseFile dlg(g_app->getMainWindow(),
                                DlgDatabaseFile::modeSave);

            if (dlg.ShowModal() == wxID_OK)
            {
                if (g_app->getDatabase()->moveFile(m_path, towstr(dlg.getPath())))
                {
                    g_app->getAppController()->refreshDbDoc();

                    m_path = dlg.getPath();
                }
            }
             else
            {
                return false;
            }
        }

        if (result == wxCANCEL)
        {
            return false;
        }
    }
    
    AppBusyCursor bc;


    // let the column props (dynamic) field panel know that we are closing
    FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_ON_SITE_CLOSING, (long)(ITableDoc*)this);
    m_frame->sendEvent(e);


    // save the view
    flushActiveView();
    freeTemporaryHandles();

    // release all references to everything
    m_frame.clear();
    m_doc_site.clear();
    m_grid_model.clear();
    m_iter.clear();

    if (m_grid)
    {
        delete m_grid;
        m_grid = NULL;
    }

    if (isTemporary())
    {
        TableDocMgr::deleteModel(m_model->getId());

        xd::IDatabasePtr db = g_app->getDatabase();
        if (db.isOk())
        {
            db->deleteFile(m_path);
        }
    }

    return true;
}


void TableDoc::onSiteDeactivated()
{
    if (m_grid)
    {
        m_grid->setCursorVisible(false);
        m_grid->refresh(kcl::Grid::refreshPaint);
    }
}


void TableDoc::onSiteActivated()
{
    if (m_grid)
    {
        m_grid->setCursorVisible(true);
        updateStatusBar();
    }
    
    IQueryDocPtr querydoc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
    if (querydoc)
        m_doc_site->setBitmap(GETBMP(gf_query_16));

    // we've activated the cursor, so post an event to repaint; 
    // we could repaint here, but in cases where the grid cursor
    // moves because the table content has been clicked, the grid
    // would refresh twice, causing a flicker; to avoid this post 
    // an event to repaint and handle the repaint after the grid 
    // will have redrawn from moving the cursor
    
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, ID_DoRefresh);
    ::wxPostEvent(this, evt);
}

void TableDoc::onStatusBarItemLeftDblClick(IStatusBarItemPtr item)
{
    // only pop open the "Go To Record" dialog for the active table
    
    IDocumentSitePtr doc_site = m_frame->getActiveChild();
    if (doc_site.isOk() && doc_site == m_doc_site)
    {
        if (item->getName() == wxT("tabledoc_position") ||
            item->getName() == wxT("tabledoc_record_count"))
        {
            gotoRecord();
        }
    }
}

void TableDoc::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == FRAMEWORK_EVT_TABLEDOC_REFRESH)
    {
        // get the event set path; refresh the tabledoc if the base or
        // browse sets have the same path

        if (isSamePath(m_path, towstr(evt.s_param)))
            m_grid->refresh(kcl::Grid::refreshAll);
    }
    else if (evt.name == FRAMEWORK_EVT_TABLEDOC_DO_VIEW_RELOAD &&
        evt.l_param != (unsigned long)this)
    {
        TableDoc* td1 = (TableDoc*)evt.l_param;
        TableDoc* td2 = (TableDoc*)this;

        if (isSamePath(td1->m_path, td2->m_path))
        {
            if (evt.l_param2 != (unsigned long)m_active_view.p)
            {
                ITableDocObjectPtr v1 = ((ITableDocView*)(evt.l_param2));
                ITableDocObjectPtr v2 = m_active_view;

                if (v1.isOk() && v2.isOk())
                {
                    if (v1->getObjectId() == v2->getObjectId())
                        m_active_view = v1;
                }
            }

            refreshActiveView();
        }

    }
     else if (evt.name == wxT("tabledoc.doReloadSettings") ||
              evt.name == FRAMEWORK_EVT_APPMAIN_PREFERENCES_SAVED)
    {
        reloadSettings(true);
    }
     else if (evt.name == FRAMEWORK_EVT_TABLEDOC_STRUCTURE_MODIFIED)
    {
        if (getEnabled())
        {
            freeTemporaryHandles();
            
            if (m_iter)
            {
                m_iter->refreshStructure();
            }

            m_grid->refreshModel();
            refreshActiveView();
        }
    }
     else if (evt.name == FRAMEWORK_EVT_TREEPANEL_OFS_FILE_RENAMED)
    {
        updateCaption();
        
        IDocumentSitePtr doc_site = m_frame->getActiveChild();
        if (doc_site.isOk() && doc_site == m_doc_site)
        {
            // fire this event so that the URL combobox will be updated
            // with the new path if this is the active child
            m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));
        }
    }
     else if (evt.name == FRAMEWORK_EVT_APPMAIN_RELATIONSHIPS_UPDATED)
    {
        m_grid->refresh(kcl::Grid::refreshAll);
    }
     else if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_QUERY_AVAILABLE_VIEW)
    {
        IDocumentSitePtr active_child;
        active_child = g_app->getMainFrame()->getActiveChild();
        
        if (active_child.isNull() || m_doc_site.isNull())
            return;
            
        if (active_child->getContainerWindow() != m_doc_site->getContainerWindow())
            return;

        // site ptrs to check the active site
        IDocumentSitePtr textdoc_site;
        IDocumentSitePtr transdoc_site;
        IDocumentSitePtr structdoc_site;
        IDocumentSitePtr querydoc_site;
        IDocumentSitePtr sqldoc_site;
        IDocumentSitePtr active_site;
        active_site = g_app->getMainFrame()->getActiveChild();
        
        // populate the view switcher list with the available views
        ViewSwitcherList* list = (ViewSwitcherList*)(evt.o_param);
        
        ITextDocPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
        IDocumentPtr querydoc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
        
        
        // querydoc's have a sql view, query view and a table view
        if (querydoc.isOk())
        {
            sqldoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.SqlDoc");
            querydoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.QueryDoc");
            
            // if we can find a QueryDoc, we're part of a QueryDoc

            list->addItem(ID_View_SwitchToSourceView, _("SQL View"),
                          (sqldoc_site == active_site) ? true : false);
            
            list->addItem(ID_View_SwitchToDesignView, _("Design View"),
                          (querydoc_site == active_site) ? true : false);
        }
        
        // textdoc's have the standard three-view for text documents
        if (textdoc.isOk())
        {
            textdoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.TextDoc");
            transdoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.TransformationDoc");
            
            // if we can find a TextDoc, we're part of a TextDoc
            list->addItem(ID_View_SwitchToSourceView, _("Source View"),
                          (textdoc_site == active_site) ? true : false);
            list->addItem(ID_View_SwitchToDesignView, _("Design View"),
                          (transdoc_site == active_site) ? true : false);
        }
        
        // normal tabledoc's have the structure editor
        if (textdoc.isNull() && querydoc.isNull())
        {
            structdoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.StructureDoc");
        
            list->addItem(ID_View_SwitchToDesignView, _("Design View"),
                          (structdoc_site == active_site) ? true : false);
        }

        // this item is always in the list
        list->addItem(ID_View_SwitchToLayoutView, _("Table View"),
                      (m_doc_site == active_site) ? true : false);
    }
     else if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGED)
    {
        int id = (int)(evt.l_param);
        
        // -- make sure we are in the active container --
        IDocumentSitePtr active_site;
        active_site = g_app->getMainFrame()->getActiveChild();
        if (active_site.isNull() || m_doc_site.isNull())
            return;
        if (active_site->getContainerWindow() != m_doc_site->getContainerWindow())
            return;
        
        
        if (id == ID_View_SwitchToLayoutView)
        {
            // if we are on structure doc, we might need to prompt for saving
            IDocumentSitePtr structdoc_site;

            structdoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.StructureDoc");
            active_site = g_app->getMainFrame()->getActiveChild();

            if (active_site == structdoc_site)
            {
                IStructureDocPtr structdoc = structdoc_site->getDocument();
                if (structdoc.isOk())
                {
                    structdoc->doSave();
                    
                    // clear out smartptr because doSave() sometimes closes the
                    // document which will make a smart ptr crash on destruct
                    structdoc.p = NULL;
                }
            }
        }
        
        
        ISqlDocPtr sqldoc = lookupOtherDocument(m_doc_site, "appmain.SqlDoc");
        IQueryDocPtr querydoc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
        ITransformationDocPtr transdoc = lookupOtherDocument(m_doc_site, "appmain.TransformationDoc");
                
        switch (id)
        {
            case ID_View_SwitchToSourceView:
                if (querydoc.isOk())
                {
                    // we don't have a sql doc yet, create one
                    if (sqldoc.isNull())
                        querydoc->createSqlDoc();
                         else
                        querydoc->updateSqlDoc();
                    
                    switchToOtherDocument(m_doc_site, "appmain.SqlDoc");
                    return;
                }
                 else if (transdoc.isOk())
                {
                    switchToOtherDocument(m_doc_site, "appmain.TextDoc");
                    return;
                }
                
            case ID_View_SwitchToDesignView:
                if (querydoc.isOk())
                {
                    switchToOtherDocument(m_doc_site, "appmain.QueryDoc");
                    return;
                }
                 else if (transdoc.isOk())
                {
                    switchToOtherDocument(m_doc_site, "appmain.TransformationDoc");
                    return;
                }

                createOrShowStructureDoc();
                return;
                
            case ID_View_SwitchToLayoutView:
                if (querydoc.isOk())
                {
                    // let query doc decide what to do
                    return;
                }
                
                switchToOtherDocument(m_doc_site, "appmain.TableDoc");
                return;
                
            default:
                // something went wrong, do nothing
                return;
        }
    }
}

void TableDoc::onActiveChildChanged(IDocumentSitePtr doc_site)
{
    if (doc_site.isNull())
        return;
    
    ITableDocPtr tabledoc = doc_site->getDocument();
    if (tabledoc.isNull())
        return;
    
    // do nothing for now
}

void TableDoc::onFormatChanged(wxCommandEvent& evt)
{
    int id = evt.GetId();

    switch (id)
    {
        case ID_Format_ToggleWrapText:
        case ID_Format_ToggleMergeCells:
        case ID_Format_ToggleFontWeight:
        case ID_Format_ToggleFontStyle:
        case ID_Format_ToggleFontUnderline:
        case ID_Format_AlignTop:
        case ID_Format_AlignMiddle:
        case ID_Format_AlignBottom:
        case ID_Format_LineColor:
            break;
            
        case ID_Format_AlignTextLeft:
        case ID_Format_AlignTextCenter:
        case ID_Format_AlignTextRight:
            {
                if (m_grid->isEditing())
                    m_grid->endEdit(true);
                
                if (id == ID_Format_AlignTextLeft)
                    setColumnProps(NULL, NULL, tabledocAlignLeft);
                     else if (id == ID_Format_AlignTextCenter)
                    setColumnProps(NULL, NULL, tabledocAlignCenter);
                     else if (id == ID_Format_AlignTextRight)
                    setColumnProps(NULL, NULL, tabledocAlignRight);
                     else
                    setColumnProps(NULL, NULL, tabledocAlignDefault);
            }
            break;
            
        case ID_Format_TextColor:
            {
                if (m_grid->isEditing())
                    m_grid->endEdit(true);
                
                wxColor* color = (wxColor*)evt.GetExtraLong();
                setColumnProps(NULL, color);
            }
            break;
            
        case ID_Format_FillColor:
            {
                if (m_grid->isEditing())
                    m_grid->endEdit(true);
                
                wxColor* color = (wxColor*)evt.GetExtraLong();
                setColumnProps(color, NULL);
            }
            break;
    }
}

void TableDoc::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void TableDoc::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}


void TableDoc::onUpdateUI(wxUpdateUIEvent& evt)
{
    int id = evt.GetId();

    if (!m_enabled)
    {
        evt.Enable(false);
        return;
    }
    
    // always disable undo/redo
    if (id == ID_Edit_Undo || id == ID_Edit_Redo)
    {
        evt.Enable(false);
        return;
    }

    // disable copy link (for now)
    if (id == ID_Edit_CopyLink)
    {
        evt.Enable(false);
        return;
    }
    
    // disable replace (for now)
    if (id == ID_Edit_Replace)
    {
        evt.Enable(false);
        return;
    }            

    // disable goto when we have an unknown number of records
    if (id == ID_Edit_GoTo)
    {
        if (m_iter.isNull())
        {
            evt.Enable(false);
            return;
        }

        if ((m_iter->getIteratorFlags() & xd::ifFastRowCount) == 0)
        {
            evt.Enable(false);
            return;
        }
    }
    
    if (id == ID_Edit_FindPrev || id == ID_Edit_FindNext)
    {
        wxString find_filter_text = g_app->getAppController()->
                                           getStandardToolbar()->
                                           getFindCombo()->
                                           GetValue();
        evt.Enable((find_filter_text.Length() > 0) ? true : false);
        return;
    }
    
    // disable data-write ids for child sets
    
    if (getIsChildSet())
    {
        // TODO: these are allowed from key-board operations;
        // should be allowed from the main menu
        if (id == ID_Edit_Replace ||
            id == ID_Data_DeleteRecords ||
            id == ID_Data_UpdateRecords)
        {
            evt.Enable(false);
            return;
        }
    }
    
    if (isExternalTable())
    {
        // TODO: disable manual deletes for external databases;
        // note: deleting based on expression is allowed, as
        // it should be, because a different ID is used for
        // deleting based on an expression

        if (id == ID_Edit_Delete)
        {
            if (m_allow_delete_menuid)
                evt.Enable(true);
                 else
                evt.Enable(false);
                
            return;
        }
    }

    if (isExternalTable())
    {
        // don't allow calculated fields to be made static
        // on external databases
    
        if (id == ID_Data_MakeDynamicFieldStatic)
        {
            evt.Enable(false);
            return;
        }
    }


    switch (id)
    {
        case ID_Table_RemoveGroupBreak:
        {
            IXdGridModelPtr model = m_grid_model;
            if (model)
            {
                evt.Enable(model->getGroupBreakExpr().length() > 0 ? true : false);
            }
        }
        break;

        case ID_Data_RemoveSortFilter:
            evt.Enable(m_sort_order.length() > 0 || m_filter.length() > 0 ? true : false);
            break;

        case ID_Data_ModifyDynamicField:
        {
            if (!m_grid)
            {
                evt.Enable(false);
                return;
            }

            if (m_grid_model.isNull())
            {
                evt.Enable(false);
                return;
            }

            int col = m_grid->getColumnModelIdx(m_grid->getCursorColumn());
            int selection_count = 0;
            int i, col_count = m_grid->getColumnCount();
            for (i = 0; i < col_count; ++i)
            {
                if (m_grid->isColumnSelected(i))
                    selection_count++;
                
                if (selection_count > 1)
                    break;
            }

            // if the current column is selected or the cursor is on
            // the calculated field, enable the event
            evt.Enable((m_grid_model->getColumnCalculated(col) &&
                          selection_count <= 1) ? true : false);
            break;
        }

        case ID_Data_MakeDynamicFieldStatic:
            if (m_is_childset)
            {
                // if we are a child set in a relationship sync
                // situation, do not allow making static
                evt.Enable(false);
                return;
            }
            // fall through

        case ID_Data_DeleteField:
        {
            if (!m_grid)
            {
                evt.Enable(false);
                return;
            }

            if (m_grid_model.isNull())
            {
                evt.Enable(false);
                return;
            }


            // for now, limit deleting to calculated fields
            xd::IStructurePtr structure = m_iter->getStructure();
            xd::IColumnInfoPtr colinfo;

            int col_count = m_grid->getColumnCount();
            int i;

            for (i = 0; i < col_count; ++i)
            {
                if (m_grid->isColumnSelected(i))
                {
                    colinfo = structure->getColumnInfo(towstr(m_grid->getColumnCaption(i)));
                    if (!colinfo)
                    {
                        evt.Enable(false);
                        return;
                    }

                    if (!colinfo->getCalculated())
                    {
                        evt.Enable(false);
                        return;
                    }
                }
            }

            // -- if no fields are selected, check the cursor column --
            int col_idx = m_grid->getCursorColumn();
            colinfo = structure->getColumnInfo(towstr(m_grid->getColumnCaption(col_idx)));
            if (!colinfo)
            {
                evt.Enable(false);
                return;
            }

            if (!colinfo->getCalculated())
            {
                evt.Enable(false);
                return;
            }

            evt.Enable(true);

            break;
        }

        default:
            evt.Enable(true);
            break;
    }
}

void TableDoc::onSave(wxCommandEvent& evt)
{
    // we're in a query (even though we're viewing the table),
    // so we need to save a query and not a table
    
    IQueryDocPtr querydoc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
    if (querydoc.isOk())
    {
        querydoc->doSave(false);
        updateCaption();
        return;
    }

    // always show a "Save As" dialog for temporary tables
    if (isTemporary())
    {
        onSaveAs(evt);
        return;
    }
    
    // note: previously for external tables, we triggered a "Save As"
    // dialog on save; however, for external tables, treat them similarly 
    // to internal tables; ; i.e., save should "save" the current document, 
    // not open a "Save As" dialog since we have "Save As" and "Save As 
    // External"; and just as we do for internal tables, which we treat as 
    // "up-to-date" when in table mode, so we should do for external
    
    // at this point, the user has clicked the save button and nothing
    // happens, so at least show a busy cursor for a second to make them
    // feel good about themselves
    
    AppBusyCursor bc;
    wxMilliSleep(500);
}

void TableDoc::onSaveAsJobFinished(jobs::IJobPtr job)
{
    kl::JsonNode params;
    params.fromString(job->getParameters());

    // re-enabled the GUI
    setEnabled(true);
    
    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;
    
    g_app->getAppController()->refreshDbDoc();

    int docsite_id = params["extra_docsite_id"].getInteger();
    IDocumentSitePtr site = g_app->getMainFrame()->lookupSiteById(docsite_id);
    if (site.isNull())
        return;

    ITableDocPtr tabledoc = site->getDocument();
    if (tabledoc.isNull())
        return;
    
    // setting this to an empty string will make the caption and
    // URL bar show the db location instead of the web location
    setSourceUrl(wxEmptyString);
    setSourceMimeType(wxEmptyString);
    
    // set the base set to the result set
    open(params["output"].getString());
    
    // the copy job copies the TableDocModel for the table; when doing so,
    // all of the TableDocObjects have a new ID assigned to them, thus the
    // m_active_view member variable is no longer valid, so we need to
    // set it to the view that was created when copyModel() was called
    ITableDocViewEnumPtr views = getModel()->getViewEnum();
    ITableDocViewPtr view;
    
    size_t i, view_count = views->size();
    if (view_count == 1)
    {
        // there's only one view for the table,
        // so set that view as the active view
        setActiveView(views->getItem(0));
    }
     else
    {
        bool found = false;
        
        for (i = 0; i < view_count; ++i)
        {
            view = views->getItem(i);
            if (compareViews(view.p, m_active_view.p))
            {
                setActiveView(view);
                found = true;
                break;
            }
        }
        
        // fallback
        if (!found)
            setActiveView(views->getItem(0));
    }
    
    // get rid of other extranneous document types
    IDocumentSitePtr textdoc_site;
    IDocumentSitePtr transdoc_site;
    IDocumentSitePtr structuredoc_site;
    textdoc_site = lookupOtherDocumentSite(site, "appmain.TextDoc");
    transdoc_site = lookupOtherDocumentSite(site, "appmain.TransformationDoc");
    structuredoc_site = lookupOtherDocumentSite(site, "appmain.StructureDoc");

    // we're now an "official" TableDoc (on an internal set), so we don't
    // need these document sites anymore; also, it is important to remove
    // the StructureDoc as well, since we'll need to completely refresh it
    // to make sure it doesn't have any cruft left over from the old one
    if (textdoc_site.isOk())
        g_app->getMainFrame()->closeSite(textdoc_site);
    if (transdoc_site.isOk())
        g_app->getMainFrame()->closeSite(transdoc_site);
    if (structuredoc_site.isOk())
        g_app->getMainFrame()->closeSite(structuredoc_site);

    // update the URL bar, view menu and statusbar
    g_app->getAppController()->updateURLToolbar();
    g_app->getAppController()->updateViewMenu(site);
    updateStatusBar();
}

void TableDoc::onSaveAs(wxCommandEvent& evt)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    // show a "Save As" dialog
    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeSave);
    if (dlg.ShowModal() != wxID_OK)
        return;

    // if we're on a temporary file and saving the file locally to the
    // project, we need only give the file a name and we're done;
    // otherwise, we need to make a hard copy of the file since we're
    // either creating a local copy of a named file or are saving it
    // to a mount
    if (isTemporary())
    {
        std::wstring cstr;
        std::wstring rpath;
        std::wstring folder = towstr(dlg.getPath().BeforeLast(wxT('/')));

        xd::IFileInfoPtr finfo = db->getFileInfo(folder);
        if (finfo.isNull())
            return;

        if (!finfo->isMount())
        {
            std::wstring save_path = towstr(dlg.getPath());

            bool needs_close = false;

            if (getDbDriver() != L"xdnative")
            {
                // because pgsql has the cursor open, the db->moveFile() call below won't
                // work because the table is locked;  We need to close the table, rename it,
                // and re-open it in order for that to work
                needs_close = true;
            }


            if (needs_close)
                closeSet();

            if (!db->moveFile(m_path, save_path))
            {
                appMessageBox(_("The file could not be saved in the specified location.  The destination location may by in use."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                return;
            }


            if (needs_close)
            {
                createModel();
                m_grid->setModel(m_grid_model);

                open(save_path);
            }

            g_app->getAppController()->refreshDbDoc();
            g_app->getAppController()->updateURLToolbar();
            g_app->getAppController()->updateViewMenu(m_doc_site);
            
            setSourceUrl(wxEmptyString);
            setSourceMimeType(wxEmptyString);

            m_path = save_path;

            updateCaption();
            updateStatusBar();
            g_app->getAppController()->updateURLToolbar();        

            return;
        }
    }


    // copy the file
    if (m_iter.isNull())
        return;

    // if we're saving the file to a filesystem mount and no
    // extension is specified, then automatically add a 'csv'
    // extension; this is a usability issue since without the
    // extension, the user usually ends up adding this as the 
    // first item of business after saving

    std::wstring path = towstr(dlg.getPath());
    path = addExtensionIfExternalFsDatabase(path, L".csv");


    bool is_indeterminate = false;
    xd::IIteratorPtr source_iter = m_iter->clone();
    if (source_iter.isOk())
    {
        source_iter->goFirst();
        
        /*
        TODO: reimplement
        // determine if the copy job's progress is indeterminate or not
        xd::IFixedLengthDefinitionPtr fset = source_iter->getSet();
        xd::IDelimitedTextSetPtr dset = source_iter->getSet();
        if ((fset.isOk() && !fset->isLineDelimited()) || dset.isOk())
            is_indeterminate = true;
        */
    }
     else
    {
        // iterators without clone capabilities are almost always
        // going to be indeterminate recno jobs
        is_indeterminate = true;
    }
    
    
    // save any changes that were made to the active view
    // before beginning the save as operation
    flushActiveView();
    
    
    setEnabled(false);
    
    wxString title = wxString::Format(_("Saving '%s' as '%s'"),
                              getCaption().c_str(),
                              kl::afterLast(path, '/').c_str());


    // set up the job from the info we gathered
    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.copy-job");
    job->getJobInfo()->setTitle(towstr(title));
    kl::JsonNode params;

    params["input"].setString(m_path);
    params["output"].setString(path);
    params["where"].setString(getFilter());
    params["order"].setString(getSortOrder());
    params["extra_docsite_id"].setInteger(m_doc_site->getId());

    job->setParameters(params.toString());
    job->sigJobFinished().connect(this, &TableDoc::onSaveAsJobFinished);
    g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
}



void TableDoc::onSaveAsExternal(wxCommandEvent& evt)
{
    wxString filter;

    // NOTE: if you add or remove items from this
    // list, make sure you adjust the case statement
    // below, because it affects which file type the
    // target will be
    filter += _("Package Files");
    filter += wxT(" (*.kpg)|*.kpg|");
    filter += _("Comma-Delimited Files");
    filter += wxT(" (*.csv)|*.csv|");
    filter += _("Comma-Delimited Interchange Files");
    filter += wxT(" (*.icsv)|*.icsv|");
    filter += _("Tab-Delimited Files");
    filter += wxT(" (*.tsv)|*.tsv|");
    filter += _("Fixed-Length Files");
    filter += wxT(" (*.txt)|*.txt|");
    filter += _("Microsoft FoxPro/Xbase Files");
    filter += wxT(" (*.dbf)|*.dbf|");
    filter += _("Microsoft Access Files");
    filter += wxT(" (*.mdb, *.accdb)|*.mdb;*.accdb|");
    filter += _("Microsoft Excel Files");
    filter += wxT(" (*.xls, *.xlsx)|*.xls;*.xlsx|");
    filter += _("Structure Files");
    filter += wxT(" (*.txt)|*.txt|");
    filter.RemoveLast(); // get rid of the last pipe sign

    wxString filename = getFilenameFromPath(m_path, false);
    
    if (xd::isTemporaryPath(m_path))
        filename = _("Untitled");
    
    wxFileDialog dlg(g_app->getMainWindow(),
                     _("Save As"),
                     wxT(""),
                     filename,
                     filter,
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() != wxID_OK)
        return;

    int export_type = 0;
    int dbtype = 0;
    wxString delimiters = wxT("");

    enum
    {
        exportPackage,
        exportDelimitedText,
        exportFixedLength,
        exportXbase,
        exportAccess,
        exportExcel,
        exportStructure
    };

    switch (dlg.GetFilterIndex())
    {
        case 0: 
            export_type = exportPackage;
            dbtype = dbtypePackage;
            break;        
        case 1:
            export_type = exportDelimitedText;
            dbtype = dbtypeDelimitedText;
            delimiters = L",";
            break;
        case 2:
            export_type = exportDelimitedText;
            dbtype = dbtypeDelimitedText;
            delimiters = L",";
            break;
        case 3:
            export_type = exportDelimitedText;
            dbtype = dbtypeDelimitedText;
            delimiters = L"\t";
            break;
        case 4:
            export_type = exportFixedLength;
            dbtype = dbtypeFixedLengthText;
            break;
        case 5:
            export_type = exportXbase;
            dbtype = dbtypeXbase;
            break;
        case 6:
            export_type = exportAccess;
            dbtype = dbtypeAccess;
            break;
        case 7:
            export_type = exportExcel;
            dbtype = dbtypeExcel;
            break;
        case 8:
            export_type = exportStructure;
            // no databse type
            break;
        default:
            wxFAIL_MSG(wxT("invalid filter index"));
            return;
    }
    
    
    
    
    if (export_type == exportStructure)
    {
        saveAsStructure(dlg.GetPath());
        return;
    }
     else
    {
        wxString stream_name = m_caption;
        if (stream_name == _("(Untitled)"))
            stream_name = _("Untitled");
            
        wxString title = wxString::Format(_("Saving '%s' as '%s'"),
                      stream_name.c_str(),
                      dlg.GetPath().AfterLast(PATH_SEPARATOR_CHAR).c_str());

        ExportTemplate templ;
        templ.m_ei.type = dbtype;

        ExportTableSelection tbl;
        tbl.input_tablename = this->m_path;
        tbl.append = false;

        if (dbtype == dbtypeDelimitedText)
        {
            templ.m_ei.delimiters = towstr(delimiters);
            templ.m_ei.text_qualifier = L"\"";
            templ.m_ei.first_row_header = true;
        }
        
        if (dbtype == dbtypeXbase || dbtype == dbtypeDelimitedText || dbtype == dbtypeFixedLengthText)
        {
            tbl.output_tablename = towstr(dlg.GetPath());
        }
         else
        {
            if (dbtype == dbtypeAccess || dbtype == dbtypeExcel || dbtype == dbtypePackage || dbtypeSqlite)
                templ.m_ei.path = towstr(dlg.GetPath());

            tbl.output_tablename = kl::beforeLast( kl::afterLast(this->m_path, '/'), '.');
            kl::replaceStr(tbl.output_tablename, L".", L"_");
            kl::replaceStr(tbl.output_tablename, L" ", L"_");
            kl::replaceStr(tbl.output_tablename, L"-", L"_");
            if (tbl.output_tablename.empty())
                tbl.output_tablename = L"exptable";
        }
       
        templ.m_ei.tables.push_back(tbl);

        templ.execute();
    }
}




     
void TableDoc::onReloadDownloadFinished(jobs::IJobInfoPtr job_info)
{
    // this is in the main thread, but truly, only because
    // of the way xulrunner thread marshals the call;
    // we want to handle this later, so fire another signal
    
    if (job_info->getState() != jobs::jobStateFinished)
        return;

    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, ID_DoReloadRefresh);
    ::wxPostEvent(this, evt);
}

void TableDoc::onDoReloadRefresh(wxCommandEvent& evt)
{ 
    wxASSERT_MSG(kl::Thread::isMain(), wxT("Callback must happen in the main thread"));
    
    m_doing_reload = false;
    
    if (m_source_mimetype == wxT("text/csv"))
    {    
        ITextDocPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
        if (!textdoc)
            return;
        
        ITransformationDocPtr transdoc = lookupOtherDocument(m_doc_site, "appmain.TransformationDoc");
        if (!transdoc)
            return;
            
        
        // close out grid (same as closeSet)
        freeTemporaryHandles();
        m_grid->setModel(xcm::null);
        m_grid_model.clear();
        m_iter.clear();
        
        // close out text doc
        textdoc->close();
        transdoc->close();
        
        
        // try to delete the filename
        bool error = false;
        wxString filename = urlToFilename(textdoc->getPath());
        if (::wxRemoveFile(filename))
        {
            if (!::wxRenameFile(m_reload_filename, filename))
                error = true;
        }
         else
        {
            error = true;
        }
        
        filename = filenameToUrl(filename);
        
        // open new file
        textdoc->open(filename);
        transdoc->initFromSet(textdoc->getPath());
        
       
        // reload grid
        createModel();
        m_grid->setModel(m_grid_model);
        open(m_path);
        setEnabled(true);
        
        if (error)
        {
            appMessageBox(_("The file could not be reloaded, because the file is locked or marked as read only"));
        }
    }
     else if (m_source_mimetype == wxT("application/rss+xml"))
    {
        xd::IDatabasePtr db = g_app->getDatabase();
        if (db.isNull())
            return;

        xd::IFileInfoPtr old_file_info = db->getFileInfo(m_path);
        std::wstring old_set_id = old_file_info.isOk() ? old_file_info->getObjectId() : L"";
        

        FeedParser parser;
        if (!parser.loadAndParse(m_reload_filename))
        {
            xf_remove(m_reload_filename);
            // can't parse feed file
            return;
        }
        
        xf_remove(m_reload_filename);
        
        std::wstring output_path = xd::getTemporaryPath();
        if (!parser.convertToTable(output_path))
            return;

        xd::IFileInfoPtr new_file_info = db->getFileInfo(output_path);
        std::wstring new_set_id = new_file_info.isOk() ? new_file_info->getObjectId() : L"";

        if (old_set_id.length() > 0 && new_set_id.length() > 0)
        {
            TableDocMgr::copyModel(old_set_id, new_set_id);
            TableDocMgr::deleteModel(old_set_id);
        }

        open(output_path);
    }
}

void TableDoc::onDoRefresh(wxCommandEvent& evt)
{
    // m_grid may not exist when onDoRefresh was called.  This scenario happens
    // when you 1) open a table; 2) open a new query; 3) add the same table to the
    // query; 4) optionally, add some fields to the query; 5) close the package;
    // This check will clear out the grid
    
    if (!m_grid)
        return;
        
    
    // the refresh requires a model, which may not exist when we go to refresh;
    // e.g. during a modify structure
    
    kcl::IModelPtr model = m_grid->getModel();
    if (model.isNull())
        return;

    m_grid->refresh(kcl::Grid::refreshAll);
}

void TableDoc::onReload(wxCommandEvent& evt)
{
    AppBusyCursor bc;
    kl::Thread::sleep(100);
    
    // make sure a reload isn't already in progress
    if (m_doing_reload)
        return;
       
    if (m_source_mimetype == wxT("text/csv"))
    {
        // these lines check to make sure that we have a textdoc setup
        ITextDocPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
        if (!textdoc)
            return;
        
        ITransformationDocPtr transdoc = lookupOtherDocument(m_doc_site, "appmain.TransformationDoc");
        if (!transdoc)
            return;
        
        wxString filename = urlToFilename(textdoc->getPath());
        filename += wxT(".");
        filename += wxString::Format(wxT("%08x%08x"), (long)time(NULL), (long)clock());
        m_reload_filename = filename;

        textdoc.clear();
        transdoc.clear();


        jobs::IJobInfoPtr job_info = jobs::createJobInfoObject();
        job_info->sigStateChanged().connect(this, &TableDoc::onReloadDownloadFinished);
        
        
        m_doing_reload = true;
        WebDoc::downloadFile(m_source_url, filename, job_info);
    }
     else if (m_source_mimetype == wxT("application/rss+xml"))
    {
        jobs::IJobInfoPtr job_info = jobs::createJobInfoObject();
        if (job_info.isNull())
            return;
        job_info->sigStateChanged().connect(this, &TableDoc::onReloadDownloadFinished);
            
        m_reload_filename = xf_get_temp_filename(L"feed", L"xml");

        m_doing_reload = true;
        WebDoc::downloadFile(m_source_url, m_reload_filename, job_info);
    }
     else
    {
        // check if it's a query doc
        IQueryDocPtr query_doc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
        if (query_doc.isOk())
        {
            if (!query_doc->isRunning())
                query_doc->execute();
        }
         else
        {
            jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.query-job");


            // configure the job parameters
            kl::JsonNode params;
            params = createSortFilterJobParams(m_path, m_filter, m_sort_order);


            // set the job parameters and start the job
            wxString title = wxString::Format(_("Filtering '%s'"),
                                              getCaption().c_str());

            job->getJobInfo()->setTitle(towstr(title));
            job->setParameters(params.toString());

            job->sigJobFinished().connect(this, &TableDoc::onFilterJobFinished);
            g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
        }
    }
}

void TableDoc::onPrint(wxCommandEvent& evt)
{
    print();
}




void TableDoc::onShareUrlRequested(wxString& url)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
    {
        url = wxT("Invalid database");
        return;
    }


    std::wstring path = m_path;

    if (g_app->getDbDriver() != L"xdclient")
    {
        xd::IDatabasePtr mount_db = db->getMountDatabase(path);
        if (mount_db.isNull())
        {
            url = wxT("Table is not stored in a centrally accessible location");
            return;
        }

        std::wstring mount_path = getMountRoot(db, path);
        if (mount_path.length() == 0)
            return;

        db = mount_db;

        path = path.substr(mount_path.length());
    }


    std::wstring dburl;


    xd::IAttributesPtr attr = db->getAttributes();
    dburl = attr->getStringAttribute(xd::dbattrDatabaseUrl);

    if (dburl.length() == 0)
    {
        url = wxT("Table is not stored in a centrally accessible location");
        return;
    }

    wxBusyCursor bc;


    // create view information in json format

    kl::JsonNode root;
    root["data"]["table"] = path;
    root["data"]["where"] = getFilter();
    root["data"]["order"] = getSortOrder();

    root["display"]["group_break"] = getGroupBreak();

    std::wstring json = root.toString();



    // store it on the server
    
    db->createFolder(L"/.views");

    // FIXME: need better random number generator here
    #ifdef __WXWINCE__
    unsigned int c = ((unsigned int)GetTickCount()) & 0xffffff;
    #else
    unsigned int c = ((unsigned int)clock()) & 0xffffff;
    #endif
    wchar_t rand[255];

    static unsigned char counter = 0;
    counter++;
    swprintf(rand, 255, L"%06x%02x", c, counter);

    std::wstring view_path = L"/.views/";
    view_path += rand;

    writeStreamTextFile(db, view_path, json, L"application/vnd.kx.view-link");

    url = (dburl + view_path);
}

void TableDoc::onShareView(wxCommandEvent& evt)
{
    DlgShareView dlg(g_app->getMainWindow());
    dlg.sigRequestShareUrl.connect(this, &TableDoc::onShareUrlRequested);
    dlg.ShowModal();
}




void TableDoc::updateStatusSelectionSum()
{
    // in a parent-child relationship, this function get's called 
    // on the child when the parent is being navigated, so don't
    // update the status bar if we're the child set unless we have
    // the focus
    if (getIsChildSet() && !windowOrChildHasFocus(m_doc_site->getContainerWindow()))
        return;


    double sum = 0.0;
    int scale = 0;
    bool show = false;

    int i, sel_count = m_grid->getSelectionCount();

    kcl::IModelPtr grid_model = m_grid_model;

    int modelcol_count = grid_model->getColumnCount();
    int* modelcol_types = new int[modelcol_count];
    int* modelcol_scales = new int[modelcol_count];

    for (i = 0; i < modelcol_count; ++i)
    {
        kcl::IModelColumnPtr modelcol = grid_model->getColumnInfo(i);
        modelcol_types[i] = modelcol->getType();
        modelcol_scales[i] = modelcol->getScale();
    }


    // get the row count while outside the loop, this will be used
    // below;  note we can use the status bar's cached row count
    // value here to save some time, as fetching the row count from
    // the database can take relatively long (even if the data set has
    // xd::sfFastRowCount)
    xd::rowpos_t row_count;
    
    if (m_stat_row_count != (xd::rowpos_t)-1)
        row_count = m_stat_row_count;
     else
        row_count = m_grid->getRowCount();


    for (i = 0; i < sel_count; ++i)
    {
        kcl::SelectionRect sel;
        m_grid->getSelection(i, &sel);

        if (sel.m_start_col == sel.m_end_col &&
            m_grid->isColumnSelected(sel.m_start_col))
        {
            // whole column selections should disable the status bar sum
            show = false;
            break;
        }
        
        if (sel.m_end_row - sel.m_start_row > 50000)
        {
            show = false;
            break;
        }

        int row, col;

        for (row = sel.m_start_row; row <= sel.m_end_row; ++row)
        {
            if (row >= row_count)
                break;

            for (col = sel.m_start_col; col <= sel.m_end_col; ++col)
            {
                int model_col = m_grid->getColumnModelIdx(col);

                if (model_col < 0)
                    continue;

                if (scale < modelcol_scales[model_col])
                    scale = modelcol_scales[model_col];

                if (modelcol_types[model_col] == kcl::Grid::typeDouble)
                {
                    show = true;
                    sum += grid_model->getCellDouble(row, model_col);
                }
                 else if (modelcol_types[model_col] == kcl::Grid::typeInteger)
                {
                    show = true;
                    sum += grid_model->getCellInteger(row, model_col);
                }
            }
        }
    }

    delete[] modelcol_types;
    delete[] modelcol_scales;

    wxString text = wxEmptyString;
    if (show)
    {
        text = wxString::Format(_("Sum: %s"), kl::formattedNumber(sum, scale).c_str());
    } 
     else
    {
        text = wxEmptyString;
    }

    // get the tabledoc's sum statusbar cell
    IStatusBarItemPtr item;
    item = m_frame->getStatusBar()->getItem(wxT("tabledoc_sum"));
    if (item.isNull())
        return;

    // hide/show the item based on if there is a sum to show
    if ((show && !item->isShown()) || (!show && item->isShown()))
    {
        item->show(show);
        
        // populate the statusbar
        g_app->getMainFrame()->getStatusBar()->populate();
    }
    
    // update the item's string value
    item->setValue(text);

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}


void TableDoc::updateStatusBar()
{
    // update status bar, including row count
    updateStatusBar(true);
}

void TableDoc::updateStatusBar(bool row_count_update)
{
    // in a parent-child relationship, this function get's called 
    // on the child when the parent is being navigated, so don't
    // update the status bar if we're the child set unless we have
    // the focus
    if (getIsChildSet() && !windowOrChildHasFocus(m_doc_site->getContainerWindow()))
        return;


    wxString position_text, reccount_text;

    wxString currow_text;
    if (m_filter.length() + m_sort_order.length() > 0)
        currow_text = wxT("~");
    currow_text += kl::formattedNumber(m_grid->getCursorRow()+1);

    if (row_count_update)
    {
        if (m_iter->getIteratorFlags() & xd::ifFastRowCount)
        {
            m_stat_row_count = m_iter->getRowCount();
        }
         else
        {
            m_stat_row_count = (xd::rowpos_t)-1;
        }
    }
        
    if (m_stat_row_count != (xd::rowpos_t)-1)
    {
        position_text = wxString::Format(_("Position: %s"), currow_text.c_str());
        reccount_text = wxString::Format(_("Record Count: %s"),
            kl::formattedNumber((long long)m_stat_row_count).c_str());
    }
        else
    {
        position_text = wxString::Format(_("Position: %s"), currow_text.c_str());
        reccount_text = wxEmptyString;
    }

    IStatusBarItemPtr item;
    IStatusBarPtr statusbar = g_app->getMainFrame()->getStatusBar();
    if (statusbar.isNull())
        return;
        
    item = statusbar->getItem(wxT("tabledoc_position"));
    if (item.isNull())
        return;

    // update the item's value
    item->setValue(position_text);

    item = statusbar->getItem(wxT("tabledoc_record_count"));
    if (item.isNull())
        return;

    // update the item's value
    item->setValue(reccount_text);

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
}



void TableDoc::onColumnsDropped(kcl::GridDataDropTarget* drop_target)
{
    if (g_app->getMainFrame()->getActiveChild() != m_doc_site)
        return;

    int idx = drop_target->getDropColumn();
    if (idx < 0)
        return;
        
    kcl::GridDraggedCells cells = drop_target->getDraggedCells();
    kcl::GridDraggedCells::reverse_iterator it;
    for (it = cells.rbegin(); it != cells.rend(); ++it)
    {
        if ((*it)->m_col == 0)
            insertColumnInternal(idx, (*it)->m_strvalue, false, cells.size() < 10 ? true : false);
    }

    if (cells.size() >= 10 && m_grid)
    {
        m_grid->refresh(kcl::Grid::refreshAll);
    }

    if (m_model.isOk() && m_active_view.isOk())
    {
        m_model->writeObject(m_active_view);
    }
}


std::wstring TableDoc::getPath()
{
    return m_path;
}

std::wstring TableDoc::getBrowsePath()
{
    if (m_browse_path.length() > 0)
        return m_browse_path;
    return m_path;
}



bool TableDoc::open(const std::wstring& _path,
                    xd::IIteratorPtr optional_iterator,
                    xd::IFileInfoPtr optional_fileinfo)
{
    std::wstring path = _path;

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    xd::IFileInfoPtr file_info = optional_fileinfo;
    if (file_info.isNull())
    {
        file_info = db->getFileInfo(path);
        if (file_info.isNull() || file_info->getType() != xd::filetypeTable)
            return false;
    }

    // make sure we know the database type
    m_mount_db = db->getMountDatabase(path);
    if (m_mount_db)
    {
        m_db_type = m_mount_db->getDatabaseType();
    }
     else
    {
        m_db_type = db->getDatabaseType();
    }
    
    m_filter = L"";
    m_sort_order = L"";
    m_caption_suffix = L"";

    m_path = _path;

    // if the set/table displayed has a url associated with it, display it

    if (m_path.find(L"://") != m_path.npos)
    {
        // path is itself a url, display that in the url bar
        setSourceUrl(m_path);
    }
     else if (file_info->getUrl().length() > 0)
    {
        setSourceUrl(file_info->getUrl());
    }
     else
    {

        xd::IAttributesPtr attr = db->getAttributes();
        std::wstring url = attr->getStringAttribute(xd::dbattrDatabaseUrl);
        if (url.length() > 0)
        {
            // project is a remote project
            url += m_path;
            setSourceUrl(url);
        }
         else
        {
            std::wstring mount_root = getMountRoot(db, path);

            std::wstring url;
            xd::IDatabasePtr mount_db = db->getMountDatabase(mount_root);
            if (mount_db.isOk())
            {
                attr = mount_db->getAttributes();
                if (attr)
                    url = attr->getStringAttribute(xd::dbattrDatabaseUrl);
            }

            if (url.length() > 0)
            {
                if (path == mount_root)
                {
                    std::wstring cstr, rpath;
                    if (db->getMountPoint(path, cstr, rpath))
                    {
                        setSourceUrl(url + rpath);
                    }
                }
                 else
                {
                    std::wstring tpart = path;
                    tpart.erase(0, mount_root.length());
                    setSourceUrl(url + tpart);
                }
            }
        }
    }


    // update caption
    updateCaption();

    // load a model for this table
    if (m_temporary_model)
        m_model = TableDocMgr::loadModel(wxT(""));
         else
        m_model = TableDocMgr::loadModel(file_info->getObjectId());


    xd::IIteratorPtr browse_iter;

    if (optional_iterator.isOk())
    {
        // use the iterator passed to us
        browse_iter = optional_iterator;
    }
     else
    {
        browse_iter = db->query(path, L"", L"", L"", NULL);
        if (browse_iter.isNull())
            return false; // something's wrong
    }

    setIterator(browse_iter);

    return true;
}

bool TableDoc::setBrowseSet(const std::wstring& path, xd::IIteratorPtr iter)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    m_browse_path = path;

    if (iter.isOk())
    {
        setIterator(iter);
    }
     else
    {
        // create a default iterator
        xd::IIteratorPtr iter = db->query(m_browse_path, L"", L"", L"", NULL);
        if (iter.isNull())
            return false;

        setIterator(iter);
    }

    updateChildWindows();

    return true;
}


void TableDoc::closeSet()
{
    freeTemporaryHandles();
    m_grid->setModel(xcm::null);

    m_grid_model.clear();
    m_iter.clear();
}


void TableDoc::setEnabled(bool new_val)
{
    if (m_enabled == new_val)
        return;

    m_enabled = new_val;

    m_grid->setVisibleState(new_val ?
                            kcl::Grid::stateVisible :
                            kcl::Grid::stateDisabled);

    m_grid->refresh(kcl::Grid::refreshAll);

    m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_ENABLED_STATE_CHANGED));
    wxTheApp->ProcessIdle();
}

bool TableDoc::getEnabled()
{
    return m_enabled;
}





void TableDoc::reloadSettings(bool redraw)
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();

    // font info
    int font_ptsize;
    int font_family;
    int font_style;
    int font_weight;
    bool font_underline;
    wxString font_facename;
    int font_encoding;

    if (prefs->exists(wxT("grid.boolean_checkbox")))
    {
        kcl::IModelPtr model = m_grid->getModel();
        IXdGridModelPtr tg_model = model;

        if (tg_model)
        {
            bool bool_checkbox = getAppPrefsBoolean(wxT("grid.boolean_checkbox"));
            tg_model->setBooleanCheckbox(bool_checkbox);
        }
    }

    if (prefs->exists(wxT("grid.horzlines_visible")))
    {
        bool vis = getAppPrefsBoolean(wxT("grid.horzlines_visible"));
        m_grid->setOptionState(kcl::Grid::optHorzGridLines, vis);
    }

    if (prefs->exists(wxT("grid.vertlines_visible")))
    {
        bool vis = getAppPrefsBoolean(wxT("grid.vertlines_visible"));
        m_grid->setOptionState(kcl::Grid::optVertGridLines, vis);
    }

    if (prefs->exists(wxT("grid.greenbar_visible")) &&
        prefs->exists(wxT("grid.greenbar_interval")))
    {
        bool vis = getAppPrefsBoolean(wxT("grid.greenbar_visible"));

        if (vis)
            m_grid->setGreenBarInterval(getAppPrefsLong(wxT("grid.greenbar_interval")));
             else
            m_grid->setGreenBarInterval(0);
    }

    if (prefs->exists(wxT("grid.font.face_name")))
    {
        font_ptsize = getAppPrefsLong(wxT("grid.font.point_size"));
        font_family = getAppPrefsLong(wxT("grid.font.family"));
        font_style = getAppPrefsLong(wxT("grid.font.style"));
        font_weight = getAppPrefsLong(wxT("grid.font.weight"));
        font_underline = getAppPrefsBoolean(wxT("grid.font.underline"));
        font_facename = getAppPrefsString(wxT("grid.font.face_name"));
        font_encoding = getAppPrefsLong(wxT("grid.font.encoding"));

        m_font = wxFont(font_ptsize, font_family, font_style, font_weight,
                        font_underline, font_facename, (wxFontEncoding)font_encoding);
    }
     else
    {
        m_font = *wxNORMAL_FONT;
    }

    m_grid->SetFont(m_font);

    wxColor color;

    if (prefs->exists(wxT("grid.background_color")))
    {
        color = getAppPrefsColor(wxT("grid.background_color"));
        m_grid->setDefaultBackgroundColor(color);
    }

    if (prefs->exists(wxT("grid.foreground_color")))
    {
        color = getAppPrefsColor(wxT("grid.foreground_color"));
        m_grid->setDefaultForegroundColor(color);
    }

    if (prefs->exists(wxT("grid.line_color")))
    {
        color = getAppPrefsColor(wxT("grid.line_color"));
        m_grid->setDefaultLineColor(color);
    }

    if (prefs->exists(wxT("grid.greenbar_color")))
    {
        color = getAppPrefsColor(wxT("grid.greenbar_color"));
        m_grid->setGreenBarColor(color);
    }

    color = getAppPrefsColor(wxT("grid.invalid_area_color"));
    m_grid->setDefaultInvalidAreaColor(color);

    bool text_wrap = false;
    if (prefs->exists(wxT("grid.text_wrap")))
        text_wrap = getAppPrefsBoolean(wxT("grid.text_wrap"));

    setTextWrapping(text_wrap ? tabledocWrapOn : tabledocWrapOff);

    if (redraw)
        m_grid->refresh(kcl::Grid::refreshAll);
}

void TableDoc::connectAlterTableJob(jobs::IJobPtr job)
{
    job->sigJobFinished().connect(this, &TableDoc::onAlterTableJobFinished);
}

void TableDoc::onColumnNameChanged(const std::wstring& old_name,
                                   const std::wstring& new_name)
{
    if (0 == wcscasecmp(old_name.c_str(), new_name.c_str()))
        return;

    // update view structures

    ITableDocViewEnumPtr viewvec = m_model->getViewEnum();
    ITableDocViewPtr view;
    int idx;
    size_t i, view_count = viewvec->size();


    // put our active view in this vector, instead
    // of the copy from the disk

    ITableDocObjectPtr t2 = m_active_view;
    for (i = 0; i < view_count; ++i)
    {
        ITableDocObjectPtr t1 = viewvec->getItem(i);
        if (t1->getObjectId() == t2->getObjectId())
        {
            viewvec->setItem(i, m_active_view);
            break;
        }
    }


    for (i = 0; i < view_count; ++i)
    {
        view = viewvec->getItem(i);
        
        bool updated = false;
        ITableDocViewColPtr col;
        while (1)
        {
            idx = view->getColumnIdx(old_name);
            if (idx == -1)
            {
                break;
            }

            col = view->getColumn(idx);
            col->setName(new_name);
            updated = true;
        }

        if (updated)
        {
            m_model->writeObject(view);
        }
    }


    // update grid's view

    int new_idx = m_grid->getColumnModelIdxByName(new_name);
    if (new_idx == -1)
        return;

    while (1)
    {
        idx = m_grid->getColumnViewIdxByName(old_name);
        if (idx == -1)
            break;

        m_grid->setColumn(idx, new_idx);
    }
}


void TableDoc::setCaption(const wxString& caption,
                          const wxString& caption_suffix)
{
    m_caption = caption;
    m_caption_suffix = caption_suffix;
    updateCaption();
}

wxString TableDoc::getCaption()
{
    return m_caption;
}



void TableDoc::updateCaption()
{
    // we're in a query (even though we're viewing the table),
    // so we need to show the querydoc's caption
    IDocumentPtr querydoc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
    if (querydoc)
    {
        wxString loc = querydoc->getDocumentLocation();
        
        if (loc.IsEmpty())
        {
            m_caption = _("(Untitled)");
        }
         else
        {
            m_caption = loc.AfterLast(wxT('/'));
        }
           
        m_doc_site->setCaption(m_caption);
        return;
    }

    // get caption

    if (isTemporary())
    {
        m_caption = _("(Untitled)");
    }
     else
    {
        m_caption = kl::afterLast(m_path, '/');
    }

    if (m_doc_site)
    {
        wxString s = m_caption;
        s += m_caption_suffix;

        m_doc_site->setCaption(s);
    }
}


wxString TableDoc::makeCaption(const wxString& title)
{
    // make panel caption
    wxString caption = title;

    if (!xd::isTemporaryPath(m_path))
    {
        wxString name = kl::afterLast(m_path, '/');

        caption += wxT(" - [");
        caption += name;
        caption += wxT("]");
    }

    return caption;
}


void TableDoc::setFont(const wxFont& font)
{
    m_font = font;
    if (m_grid)
    {
        m_grid->SetFont(m_font);
        m_grid->refresh(kcl::Grid::refreshAll);
    }
}




void TableDoc::setTextWrapping(int new_value)
{
    m_text_wrapping = new_value;
    
/*

    int col_count = m_grid->getColumnCount();
    int i;

    for (i = 0; i < col_count; ++i)
    {
        m_grid->setColumnTextWrapping(i, (new_value == tabledocWrapOn) ? kcl::Grid::wrapOn : kcl::Grid::wrapOff);
    }

    if (col_count > 0)
    {
        m_grid->refresh(kcl::Grid::refreshAll);
    }
*/
}


wxWindow* TableDoc::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void TableDoc::setIterator(xd::IIteratorPtr iter, bool go_first)
{
    // save the cursor column and column selections so
    // we can restore them after we set the iterator
    int cursor_column = 0;
    std::vector<kcl::SelectionRect> selection_rects;

    if (m_grid)
    {
        cursor_column = m_grid->getCursorColumn();    
        m_grid->getAllSelections(selection_rects);
    }

    freeTemporaryHandles();

    // cause external table variable to refresh
    m_external_table = -1;

    m_iter = iter;
    if (m_iter->getIteratorFlags() & xd::ifForwardOnly)
    {
        // if the iterator is forward-only, try to turn on
        // xd's backward scroll row cache
        m_iter->setIteratorFlags(xd::ifReverseRowCache, xd::ifReverseRowCache);
    }
    
    if (go_first)
        m_iter->goFirst();
    m_grid_model->setIterator(m_iter);

    if (!go_first)
    {
        // convince the grid and its row model to jump to
        // where the iterator is currently positioned
        kcl::IModelPtr model = m_grid_model;
        long long row_count = (long long)model->getRowCount();
        double pos = m_iter->getPos();
        int approx_pos = (int)(row_count*pos);
        m_grid_model->setCurrentRow(approx_pos);
        m_grid->setRowOffset(approx_pos);
        m_grid->updateData();
        m_grid->moveCursor(approx_pos, 0);
    }


    ITableDocViewEnumPtr viewvec = m_model->getViewEnum();
    m_default_view_created = false;

    if (viewvec->size() == 0)
    {
        // we have no view, so create a default one (which
        // will be flushed later on

        ITableDocViewPtr defview = m_model->createViewObject();
        xd::IStructurePtr s = m_iter->getStructure();
        initializeDefaultView(defview, s);
        defview->setDescription(towstr(_("Default View")));
        setActiveView(defview);
        m_default_view_created = true;
    }

    if (m_active_view.isNull())
        setActiveView(viewvec->getItem(0));


    if (m_grid)
    {
        // insertion of new rows is not allowed when
        // an order or a filter is set

        if (m_sort_order.empty() && m_filter.empty())
        {
            m_grid->setOptionState(kcl::Grid::optGhostRow, true);
        }
         else
        {
            m_grid->setOptionState(kcl::Grid::optGhostRow, false);
        }

        // reset the grid
        if (go_first)
            m_grid->reset();

        // refresh the active view, but don't repaint
        refreshActiveView(false);
        
        if (m_default_view_created)
        {
            m_grid->autoColumnResize(-1);
            m_model->writeObject(m_active_view);
            m_default_view_created = false;
        }

        // restore the cursor column and any column selection
        int cursor_row = m_grid->getCursorRow();
        m_grid->moveCursor(cursor_row, cursor_column, false);

        std::vector<kcl::SelectionRect>::iterator it, it_end;
        it_end = selection_rects.end();
        
        for (it = selection_rects.begin(); it != it_end; ++it)
        {
            if (it->m_end_row == -1)
                m_grid->addSelection(&(*it));
        }
        
        m_grid->refresh(kcl::Grid::refreshAll);
    }
}

kcl::Grid* TableDoc::getGrid()
{
    return m_grid;
}

xd::IIteratorPtr TableDoc::getIterator()
{
    return m_iter;
}


ITableDocModelPtr TableDoc::getModel()
{
    return m_model;
}

void TableDoc::insertColumnSeparator(int insert_pos)
{
    if (m_grid->isEditing())
    {
        m_grid->endEdit(true);
    }

    ITableDocViewColPtr col = m_active_view->createColumn(insert_pos);
    col->setName(wxT(""));
    col->setSize(10);

    int idx = m_grid->insertColumnSeparator(insert_pos);
    m_grid->setColumnSize(idx, 10);

    m_grid->refresh(kcl::Grid::refreshAll);

    m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_VIEW_MODIFIED, 0));

    // update other windows that are showing the same view
    FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_DO_VIEW_RELOAD,
                                           (unsigned long)this);
    e->l_param2 = (unsigned long)m_active_view.p;
    m_frame->postEvent(e);
}


void TableDoc::insertColumn(int insert_pos, const wxString& expr)
{
    insertColumnInternal(insert_pos, expr, true, true);
}

void TableDoc::insertColumnInternal(int insert_pos,
                                    const wxString& expr,
                                    bool save_view,
                                    bool refresh)
{
    wxString viewcol_name = expr;

    if (m_grid->isEditing())
    {
        m_grid->endEdit(false);
    }

    int colidx = m_grid->getColumnModelIdxByName(expr);

    if (colidx == -1)
    {
        // column was not found, try to refresh the model
        m_grid->refreshModel();
        colidx = m_grid->getColumnModelIdxByName(expr);
        if (colidx == -1)
        {
            insertChildColumn(insert_pos, expr);
            return;
        }
    }

    ITableDocViewColPtr col = m_active_view->createColumn(insert_pos);
    col->setName(towstr(viewcol_name));
    col->setSize(80);

    if (save_view)
    {
        m_model->writeObject(m_active_view);
    }

    int idx = m_grid->insertColumn(insert_pos, colidx);
    m_grid->setColumnSize(idx, 80);

    if (refresh)
        m_grid->refresh(kcl::Grid::refreshAll);

    m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_VIEW_MODIFIED, 0));

    // update other windows that are showing the same view
    FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_DO_VIEW_RELOAD,
                                          (unsigned long)this);
    e->l_param2 = (unsigned long)m_active_view.p;
    m_frame->postEvent(e);
}


void TableDoc::insertChildColumn(int insert_pos, const wxString& text)
{
    // try to find an existing calculated field which contains
    // the calculated field expression
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    xd::IRelationSchemaPtr rels = db;
    if (rels.isNull())
        return;

    xd::IStructurePtr s = db->describeTable(m_path);
    if (s.isNull())
        return;

    int i, col_count = s->getColumnCount();

    xd::IColumnInfoPtr colinfo;
    for (i = 0; i < col_count; ++i)
    {
        colinfo = s->getColumnInfoByIdx(i);
        if (!colinfo->getCalculated())
            continue;

        if (text.CmpNoCase(colinfo->getExpression()) == 0)
        {
            // we found an exact match
            insertColumn(insert_pos, colinfo->getName());
            return;
        }
    }

    // a suitable calculated field did not exist for the expression,
    // so we have to create it

    if (text.Find(wxT('.')) == -1)
    {
        return;
    }

    wxString rel_tag = text.BeforeFirst(wxT('.'));
    wxString col_name = text.AfterFirst(wxT('.'));


    // now try to find the set that has that column
    xd::IStructurePtr right_structure;

    xd::IRelationEnumPtr rel_enum = rels->getRelationEnum(m_path);
    xd::IRelationPtr rel;
    int rel_count = (int)rel_enum->size();

    for (i = 0; i < rel_count; ++i)
    {
        rel = rel_enum->getItem(i);

        if (rel.isNull())
            continue;

        if (0 != rel_tag.CmpNoCase(rel->getTag()))
            continue;

        right_structure = db->describeTable(rel->getRightTable());
        if (right_structure.isNull())
            continue;

        colinfo = right_structure->getColumnInfo(towstr(col_name));
        if (colinfo.isNull())
        {
            // we couldn't find the appropriate column
            return;
        }
    }

    wxString column_name = makeProperIfNecessary(col_name);
    wxString column_expr;

    column_expr = wxT("FIRST(");
    column_expr += rel_tag;
    column_expr += wxT(".");
    column_expr += col_name;
    column_expr += wxT(")");

    wxString temp;
    i = 0;
    while (1)
    {
        if (i > 0)
            temp = wxString::Format(wxT("%s%d"), column_name.c_str(), i+1);
             else
            temp = column_name.c_str();

        if (!s->getColumnExist(towstr(temp)))
        {
            column_name = temp;
            break;
        }

        i++;
    }


    if (createDynamicField(column_name,
                           colinfo->getType(),
                           colinfo->getWidth(),
                           colinfo->getScale(),
                           column_expr,
                           true))
    {
        insertColumn(insert_pos, column_name);

        // let other windows know that the structure was modified
        FrameworkEvent* evt = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_STRUCTURE_MODIFIED);
        evt->s_param = m_path;
        m_frame->postEvent(evt);
    }
}

void TableDoc::hideColumn(int idx)
{
    if (m_grid->isEditing())
        m_grid->endEdit(false);

    m_grid->hideColumn(idx);
    m_active_view->deleteColumn(idx);
    m_grid->refresh(kcl::Grid::refreshAll);
    m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_VIEW_MODIFIED, 0));

    // update other windows that are showing the same view
    FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_DO_VIEW_RELOAD,
                                           (unsigned long)this);
    e->l_param2 = (unsigned long)m_active_view.p;
    m_frame->postEvent(e);
}

void TableDoc::onFilterJobFinished(jobs::IJobPtr job)
{
    if (job.isOk())
    {
        // if the job that finished is the quick filter job,
        // reset the filter pending state
        int id = job->getJobId();
        if (id == m_quick_filter_jobid)
            m_quick_filter_jobid = quickFilterNotPending;
    }

    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
    {
        // if the job is cancelled or failed, update the filter toolbar item
        // and we're done
        if (job->getJobInfo()->getState() == jobs::jobStateCancelled ||
            job->getJobInfo()->getState() == jobs::jobStateFailed)
        {
            g_app->getAppController()->updateQuickFilterToolBarItem();    
        }

        return;
    }

    xd::IIteratorPtr iter = job->getResultObject();
    if (iter.isOk())
    {
        kl::JsonNode params_node;
        params_node.fromString(job->getParameters());
        m_filter = params_node["where"].getString();

        setBrowseSet(iter->getTable(), iter);
    }

    updateStatusBar();
    g_app->getAppController()->updateQuickFilterToolBarItem();

    wxString suffix;
    if (m_filter.length() > 0)
    {
        suffix = wxT(" [");
        suffix += _("Filtered");
        suffix += wxT("]");
    }
    
    setCaption(wxEmptyString, suffix);
}

void TableDoc::onSortJobFinished(jobs::IJobPtr query_job)
{
    if (query_job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;

    bool remove_group_break = true;

    kl::JsonNode params_node;
    params_node.fromString(query_job->getParameters());

    kl::JsonNode order_node = params_node["order"];
    if (order_node.isOk())
    {
        m_sort_order = getOrderExprFromJobParam(order_node);

        // if we have a group break and the group break isn't a subset
        // of the new sort order, remove the group break
        std::vector< std::pair<std::wstring, bool> > sort_fields;
        std::vector<std::wstring> group_fields;

        sort_fields = sortExprToVector(m_sort_order);
        kl::parseDelimitedList(m_group_break, group_fields, ',', true);

        if (group_fields.size() <= sort_fields.size())
        {
            remove_group_break = false;
        
            std::vector<std::wstring>::iterator it, it_end;
            it_end = group_fields.end();
        
            int i = 0;
            for (it = group_fields.begin(); it != it_end; ++it)
            {
                if (*it != sort_fields[i].first)
                {
                    remove_group_break = true;
                    break;
                }

                ++i;
            }
        }
    }

    if (remove_group_break)
        setGroupBreak(wxT(""));

    // set the browse set and update the status bar
    xd::IIteratorPtr iter = query_job->getResultObject();
    if (iter.isOk())
        setBrowseSet(iter->getTable(), iter);

    updateStatusBar();
}

void TableDoc::onDeleteJobFinished(jobs::IJobPtr delete_job)
{
    m_grid->reset();
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

static void extractAlterJobInfo(kl::JsonNode params,
                                wxString& input,
                                std::vector<std::pair<std::wstring, std::wstring> >& to_rename,
                                std::vector<std::pair<std::wstring, int> >& to_insert,
                                std::vector<std::wstring>& to_delete)
{
    if (params.childExists("input"))
        input = params["input"].getString();

    if (!params.childExists("actions"))
        return;

    kl::JsonNode node = params["actions"];
    std::vector<kl::JsonNode> actions = node.getChildren();

    std::vector<kl::JsonNode>::iterator it, it_end;
    it_end = actions.end();

    for (it = actions.begin(); it != actions.end(); ++it)
    {
        if (!it->childExists("action"))
            continue;

        std::wstring action = it->getChild("action");

        if (action == L"modify")
        {
            std::pair<wxString,wxString> p;

            if (it->childExists("column"))
                p.first = it->getChild("column").getString();

            if (it->childExists("params"))
            {
                if (it->getChild("params").childExists("name"))
                {
                    p.second = it->getChild("params").getChild("name").getString();
                    to_rename.push_back(p);
                }
            }
        }

        if (action == L"add")
        {
            std::pair<wxString,int> p;

            if (it->childExists("column"))
                p.first = it->getChild("column").getString();

            if (it->childExists("params"))
            {
                if (it->getChild("params").childExists("position"))
                {
                    p.second = it->getChild("params").getChild("position").getInteger();
                    to_insert.push_back(p);
                }
            }
        }

        if (action == L"drop")
        {
            std::wstring column;
            if (it->childExists("column"))
                to_delete.push_back(it->getChild("column"));
        }
    }
}

void TableDoc::onAlterTableJobFinished(jobs::IJobPtr job)
{
    // unlock this window
    m_enabled = true;
    m_grid->setVisibleState(kcl::Grid::stateVisible);
    m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_ENABLED_STATE_CHANGED));

    createModel();
    m_grid->setModel(m_grid_model);


    // get information about what happened to each of the columns in the job
    std::vector<std::pair<std::wstring, std::wstring> > to_rename;
    std::vector<std::pair<std::wstring, int> > to_insert;
    std::vector<std::wstring> to_delete;
    wxString input_path;

    kl::JsonNode params;
    params.fromString(job->getParameters());
    extractAlterJobInfo(params, input_path, to_rename, to_insert, to_delete);


    // rename the columns; note: for the rename to work, we have to do this
    // after we have a grid model, but before we open the table since the
    // rename operation needs a model. However, opening a set goes "too far"
    // and will try to refresh the view with the old names, causing them
    // to be removed if they aren't in sync with the model

    if (job->getJobInfo()->getState() == jobs::jobStateFinished)
    {
        std::vector<std::pair<std::wstring, std::wstring> >::iterator rename_iter;
        for (rename_iter = to_rename.begin();
             rename_iter != to_rename.end(); ++rename_iter)
        {
            onColumnNameChanged(rename_iter->first, rename_iter->second);
        }
    }

    open(towstr(input_path));

    // remove the "Filtered" suffix
    setCaption(wxEmptyString, wxEmptyString);
    
    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;

    // do inserts for each new field
    std::vector<std::pair<std::wstring, int> >::iterator insert_iter;
    for (insert_iter = to_insert.begin();
         insert_iter != to_insert.end(); ++insert_iter)
    {
        insertColumnInternal(insert_iter->second,
                             insert_iter->first, false, true);
    }

    // do deletes in the view
    std::vector<std::wstring>::iterator delete_iter;
    for (delete_iter = to_delete.begin();
         delete_iter != to_delete.end(); ++delete_iter)
    {
        // delete each column in the view with this name
        int view_idx = 0;
        while (view_idx != -1)
        {
            view_idx = m_active_view->getColumnIdx(*delete_iter);
            if (view_idx != -1)
                m_active_view->deleteColumn(view_idx);
        }
    }

    // write out all of changes
    m_model->writeObject(m_active_view);

    m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_VIEW_MODIFIED, 0));

    // update other windows that are showing the same view
    FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_DO_VIEW_RELOAD,
                                   (unsigned long)this);
    e->l_param2 = (unsigned long)m_active_view.p;
    m_frame->postEvent(e);
    
    updateStatusBar();
    g_app->getAppController()->updateQuickFilterToolBarItem();
    
    // let other windows know that the structure was modified
    FrameworkEvent* evt = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_STRUCTURE_MODIFIED);
    evt->s_param = m_path;
    m_frame->postEvent(evt);
}



// -- Cell Context Menu --

enum
{
    ID_ExprMenuItem_Equal = 0,
    ID_ExprMenuItem_NotEqual,
    ID_ExprMenuItem_GreaterThan,
    ID_ExprMenuItem_LessThan,
    ID_ExprMenuItem_GreaterThanEqual,
    ID_ExprMenuItem_LessThanEqual,
    ID_ExprMenuItem_Contains
};


static bool getMenuItemExpr(const wxString& field,
                            int type,
                            int oper,
                            const wxString& value,
                            wxString& result)
{
    wxString lhs = xd::quoteIdentifierIfNecessary(g_app->getDatabase(), towstr(field));
    wxString rhs = value;

    if (type == xd::typeBoolean)
    {
        if (oper == ID_ExprMenuItem_Equal)
        {
            result = wxString::Format(wxT("%s = %s"), lhs.c_str(), value.c_str());
            return true;
        }

        if (oper == ID_ExprMenuItem_NotEqual)
        {
            result = wxString::Format(wxT("%s != %s"), lhs.c_str(), value.c_str());
            return true;
        }
        
        // don't do anything else for boolean type
        return false;
    }

    // handle null dates with special expression
    if ((type == xd::typeDate || type == xd::typeDateTime) &&
         value == wxT("null"))
    {
        if (oper == ID_ExprMenuItem_Equal)
        {
            result = wxString::Format(wxT("Empty(%s)"), lhs.c_str());
            return true;
        }

        if (oper == ID_ExprMenuItem_NotEqual)
        {
            result = wxString::Format(wxT("!Empty(%s)"), lhs.c_str());
            return true;
        }
        
        // don't do anything else for date and datetime types with
        // null values
        return false;
    }

    // quote character field values; handle special case for LIKE 
    // with character field
    if (type == xd::typeCharacter || type == xd::typeWideCharacter)
    {
        // double single quotes
        rhs = doubleQuote(rhs, L'\'');
        
        if (oper != ID_ExprMenuItem_Contains)
        {
            // add quotes and fall through
            rhs = wxT("'") + rhs + wxT("'");
        }
        else
        {
            // build string of form "<lhs> LIKE '%<rhs>%'"
            result = wxT("");
            result += wxString::Format(wxT("%s LIKE "), lhs.c_str());
            result += wxT("'%");
            result += wxString::Format(wxT("%s"), rhs.c_str());
            result += wxT("%'");

            // special case; we're done
            return true;
        }
    }

    switch (oper)
    {
        case ID_ExprMenuItem_Equal:
            result = wxString::Format(wxT("%s = %s"), lhs.c_str(), rhs.c_str());
            return true;

        case ID_ExprMenuItem_NotEqual:
            result = wxString::Format(wxT("%s != %s"), lhs.c_str(), rhs.c_str());
            return true;

        case ID_ExprMenuItem_GreaterThan:
            result = wxString::Format(wxT("%s > %s"), lhs.c_str(), rhs.c_str());
            return true;

        case ID_ExprMenuItem_LessThan:
            result = wxString::Format(wxT("%s < %s"), lhs.c_str(), rhs.c_str());
            return true;

        case ID_ExprMenuItem_GreaterThanEqual:
            result = wxString::Format(wxT("%s >= %s"), lhs.c_str(), rhs.c_str());
            return true;

        case ID_ExprMenuItem_LessThanEqual:
            result = wxString::Format(wxT("%s <= %s"), lhs.c_str(), rhs.c_str());
            return true;
    }

    // no expression for this item
    result = wxT("");
    return false;
}


static wxMenu* createExprMenu(const wxString& field,
                              int field_type,
                              const wxString& value,
                              int base_id)
{
    wxMenu* menu = new wxMenu;

    // iterate through the expression types
    int max_item = 7;
    for (int i = 0; i < max_item; ++i)
    {
        wxString expression;
        if (getMenuItemExpr(field, field_type, i, value, expression))
            menu->Append(base_id+i, expression);
    }

    return menu;
}

static wxMenu* createViewsMenu(ITableDocViewEnumPtr views,
                               ITableDocViewPtr active_view,
                               int base_id)
{
    wxMenu* menu = new wxMenu;
    
    ITableDocObjectPtr v1, v2 = active_view;
    
    int i, count = views->size();
    for (i = 0; i < count; ++i)
    {
        ITableDocViewPtr view = views->getItem(i);
        v1 = view;
        
        menu->AppendCheckItem(base_id+i, view->getDescription());
        
        // check the active view in the menu
        if (v1->getObjectId() == v2->getObjectId())
            menu->Check(base_id+i, true);
    }
    
    menu->AppendSeparator();
    menu->Append(base_id+i, _("&Edit..."));
    return menu;
}

static wxMenu* createIndexesMenu(xd::IIndexInfoEnumPtr indexes,
                                 const wxString sort_order_expr,
                                 int base_id)
{
    wxMenu* menu = new wxMenu;
    menu->AppendCheckItem(base_id, _("Default Order"));
    
    // check the current sort order in the menu
    if (sort_order_expr.IsEmpty())
        menu->Check(base_id, true);
    
    base_id++;
    
    int i, count = indexes->size();
    for (i = 0; i < count; ++i)
    {
        xd::IIndexInfoPtr index = indexes->getItem(i);
        wxString index_tag = index->getTag();
        wxString index_expr = index->getExpression();
        
        menu->AppendCheckItem(base_id, index_tag);
        
        // check the current sort order in the menu
        if (index_expr.Length() > 0 &&
            index_expr.CmpNoCase(sort_order_expr) == 0)
        {
            menu->Check(base_id, true);
        }
        
        base_id++;
    }
    
    menu->AppendSeparator();
    menu->Append(base_id++, _("&Edit..."));
    return menu;
}

void TableDoc::onGridLinkLeftClick(kcl::GridEvent& evt)
{
    if (::wxGetKeyState(WXK_CONTROL))
    {
        // if the control key is down, use similar behavior to onGridLinkMiddleClick,
        // this allows links to be opened in a background tab and is a necessity for
        // anyone wanting to open a link from a table in the background on a laptop
        // without an external mouse    
        g_app->getAppController()->openWeb(evt.GetString(), NULL, appOpenForceNewWindow, NULL);
    }
    else
    {
        g_app->getAppController()->openWeb(evt.GetString());
    }
}

void TableDoc::onGridLinkMiddleClick(kcl::GridEvent& evt)
{
    g_app->getAppController()->openWeb(evt.GetString(), NULL, appOpenForceNewWindow, NULL);
}

void TableDoc::onGridCellRightClick(kcl::GridEvent& event)
{
    // make sure we are the active window.  If the user 
    m_frame->setActiveChild(m_doc_site);
    
    // get the row and column of the cell that was right-clicked
    int row = event.GetRow();
    int col = event.GetColumn();
    
    // figure out the grid model column index
    int model_col = m_grid->getColumnModelIdx(col);
    if (model_col == -1)
        return;

    // move the cursor there
    m_grid->moveCursor(row, col);

    // get column info from the grid
    kcl::IModelPtr model = m_grid->getModel();
    IXdGridModelPtr xd_grid_model = model;
    if (!xd_grid_model)
        return;
        
    kcl::IModelColumnPtr grid_colinfo = model->getColumnInfo(model_col);
    if (grid_colinfo.isNull())
        return;

    if (!model->isRowValid(row))
        return;
        
    wxString colname = grid_colinfo->getName();

    //  get column type from the iterator
    xd::objhandle_t colhandle = m_iter->getHandle(towstr(colname));
    if (!colhandle)
        return;

    xd::IColumnInfoPtr colinfo = m_iter->getInfo(colhandle);
    if (colinfo.isNull())
        return;

    int coltype = colinfo->getType();
    
    m_iter->releaseHandle(colhandle);


    wxString value;

    switch (coltype)
    {
        case xd::typeWideCharacter:
        case xd::typeCharacter:
        {
            // we'll handle quoting in the menu expression function
            value = xd_grid_model->getCellString(row, model_col);
            break;
        }

        case xd::typeDouble:
        case xd::typeNumeric:
        {
            double d = xd_grid_model->getCellDouble(row, model_col);
            value = wxString::Format(wxT("%.*f"), colinfo->getScale(), d);
            value.Replace(wxT(","), wxT("."));
        }
        break;

        case xd::typeInteger:
        {
            int i = xd_grid_model->getCellInteger(row, model_col);
            value = wxString::Format(wxT("%d"), i);
        }
        break;

        case xd::typeDate:
        {   
            xd::datetime_t d = xd_grid_model->getCellDateTime(row, model_col);
            if (d == 0)
                value = wxT("null");
            else
            {
                xd::DateTime dt = d;
                        
                switch (m_db_type)
                {
                    default:
                    case xd::dbtypeXdnative:
                        value = wxString::Format(wxT("DATE('%04d-%02d-%02d')"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay());
                        break;
                    case xd::dbtypeSqlServer:
                        value = wxString::Format(wxT("{d '%04d-%02d-%02d' }"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay());
                        break;
                    case xd::dbtypeOracle:
                        value = wxString::Format(wxT("TO_DATE('%04d-%02d-%02d', 'YYYY-MM-DD')"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay());
                        break;
                }
            }

            break;
        }

        case xd::typeDateTime:
        {
            xd::datetime_t d = xd_grid_model->getCellDateTime(row, model_col);
            if (d == 0)
                value = wxT("null");
            else
            {
                xd::DateTime dt = d;
                 
                switch (m_db_type)
                {
                    default:
                    case xd::dbtypeXdnative:
                        value = wxString::Format(wxT("DATE('%04d-%02d-%02d %02d:%02d:%02d')"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay(),
                                                 dt.getHour(), dt.getMinute(), dt.getSecond());
                        break;
                    case xd::dbtypeSqlServer:
                        value = wxString::Format(wxT("{ts '%04d-%02d-%02d %02d:%02d:%02d' }"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay(),
                                                 dt.getHour(), dt.getMinute(), dt.getSecond());
                        break;
                    case xd::dbtypeOracle:
                        value = wxString::Format(wxT("TO_DATE('%04d-%02d-%02d %02d:%02d:%02d', 'YYYY-MM-DD HH24:MI:SS')"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay(),
                                                 dt.getHour(), dt.getMinute(), dt.getSecond());
                        break;
                }
            }

            break;
        }

        case xd::typeBoolean:
        {
            bool bool_value = xd_grid_model->getCellBoolean(row, model_col);        
            switch (m_db_type)
            {            
                default:
                case xd::dbtypeXdnative:
                    value = bool_value ? wxT("true") : wxT("false");
                    break;

                case xd::dbtypeSqlServer:
                    value = bool_value ? wxT("1") : wxT("0");
                    break;
            }
        }
    }


    // -- create value-side string --
    wxMenu menuPopup;
    menuPopup.Append(ID_Edit_Cut, _("Cu&t"));
    menuPopup.Append(ID_Edit_Copy, _("&Copy"));
    menuPopup.Append(ID_Edit_Paste, _("&Paste"));
    menuPopup.AppendSeparator();
    menuPopup.Append(27100, _("&Filter Records"), createExprMenu(colname, coltype, value, 27100));
    menuPopup.Append(ID_Data_RemoveSortFilter, _("&Remove Sort/Filter"));
    menuPopup.AppendSeparator();
    menuPopup.Append(27500, _("&Mark Records"), createExprMenu(colname, coltype, value, 27500));
    menuPopup.AppendSeparator();
    menuPopup.Append(27200, _("C&opy Records"), createExprMenu(colname, coltype, value, 27200));
    menuPopup.Append(27400, _("&Delete Records"), createExprMenu(colname, coltype, value, 27400));
    menuPopup.Append(27300, _("&Update Records"), createExprMenu(colname, coltype, value, 27300));


    if (getIsChildSet())
    {
        menuPopup.Enable(27400, false);
        menuPopup.Enable(27300, false);
    }



    m_grid->refresh(kcl::Grid::refreshPaint);

    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);
    CommandCapture* cc = new CommandCapture;
    PushEventHandler(cc);
    PopupMenu(&menuPopup, pt_mouse);
    int command = cc->getLastCommandId();
    PopEventHandler(true);

    if (command >= 27100 && command <= 27199)
    {
        // -- filter --
        wxString expr;
        getMenuItemExpr(colname, coltype, command-27100, value, expr);
        
        
        // --------------------
        g_macro << "";
        g_macro << "// set filter";
        g_macro << wxString::Format(wxT("auto.activeDocument.setFilter(\"%s\");"),
                                    wxcstr(jsEscapeString(expr, '"')));
        g_macro << "auto.waitForRunningJobs();";
        // --------------------

        
        setFilter(towstr(expr));
    }
     else if (command >= 27200 && command <= 27299)
    {
        // copy rows
        wxString expr;
        getMenuItemExpr(colname, coltype, command-27200, value, expr);
        copyRecords(towstr(expr));
    }
     else if (command >= 27300 && command <= 27399)
    {
        // replace rows
        wxString expr;
        getMenuItemExpr(colname, coltype, command-27300, value, expr);
        
        wxString cursor_column = getCursorColumnName(m_grid);
        showReplacePanel(expr, cursor_column);
    }
     else if (command >= 27400 && command <= 27499)
    {
        // delete rows
        if (!g_app->getAppController()->doReadOnlyCheck())
            return;

        wxString message = wxString::Format(_("Performing this operation will permanently delete data.  Are you sure\nyou want to delete the records that meet the selected condition?"));

        int res = wxMessageBox(message,
                               APPLICATION_NAME,
                               wxYES_NO | wxICON_EXCLAMATION | wxCENTER);

        if (res == wxYES)
        {
            wxString expr;
            getMenuItemExpr(colname, coltype, command-27400, value, expr);
            deleteRecords(towstr(expr));
        }
    }
     else if (command >= 27500 && command <= 27599)
    {
        wxString expr;
        getMenuItemExpr(colname, coltype, command-27500, value, expr);
        createNewMark(expr.c_str());
    }
     else
    {
        if (command != 0)
        {
            wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, command);
            ::wxPostEvent(g_app->getMainWindow()->GetEventHandler(), event);
        }
    }
}




// -- Grid event handlers --

static bool isDeleteAllowed(TableDoc* tabledoc, bool deleting_rows)
{
    // allow deletes on internal tables
    if (!tabledoc->isExternalTable())
        return true;
    
    // we're working on an external set, continue our checks
    
    // don't allow row deleting on external sets
    if (deleting_rows)
        return false;
    
    // check to see if column deletion is allowed on the selected columns
    
    ITableDocViewPtr view = tabledoc->getActiveView();
    if (view.isNull())
        return false;
    
    kcl::Grid* grid = tabledoc->getGrid();
    std::vector<int> selected_view_cols;
    
    int col, col_count = view->getColumnCount();
    for (col = 0; col < col_count; ++col)
    {
        if (grid->isColumnSelected(col))
            selected_view_cols.push_back(col);
    }
    
    return tabledoc->canDeleteColumns(selected_view_cols);
}

void TableDoc::onGridColumnRightClick(kcl::GridEvent& evt)
{
    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);

    m_frame->setActiveChild(m_doc_site);

    m_grid->moveCursor(m_grid->getCursorRow(), evt.GetColumn());


    // remove all non-column selections
    int i;
    int sel_count = m_grid->getSelectionCount();
    kcl::SelectionRect r;
    int row_count = m_grid->getKnownRowCount();
    for (i = 0; i < sel_count; ++i)
    {
        m_grid->getSelection(i, &r);
        if (r.m_start_row != 0 || r.m_end_row != row_count-1)
        {
            m_grid->removeSelection(i);
            i = -1;
            sel_count = m_grid->getSelectionCount();
        }
    }

    if (!m_grid->isColumnSelected(evt.GetColumn()))
    {
        m_grid->clearSelection();
        m_grid->setColumnSelected(evt.GetColumn(), true);
    }

    m_grid->refresh(kcl::Grid::refreshPaint);


    // flag to determine if the "Delete Field" menu item is enabled
    m_allow_delete_menuid = isDeleteAllowed(this, false /* we're not deleting rows */);
    

    // find out if the column properties panel is presently shown

    bool column_props_panel_shown = false;
    IDocumentSitePtr site = m_frame->lookupSite(wxT("CalculatedFieldPropertiesPanel"));
    if (site.isOk())
        column_props_panel_shown = site->getVisible();

    // find out if any calculated fields are selected

    int calculated_field_count = 0;
    int fixed_field_count = 0;
    int selected_count = 0;

    kcl::IModelPtr model = m_grid->getModel();
    IXdGridModelPtr tmodel = model;
    
    int col_count = m_grid->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
        {
            // increment the selected count
            selected_count++;

            // this code fragement makes sure that we are not doing context
            // column operations on calculated fields that are currently
            // being modified; when checking, make sure we have a valid
            // model column index (i.e., that we're not on a column separator)

            int model_idx = m_grid->getColumnModelIdx(i);

            if (tmodel->getColumnCalculated(model_idx))
                calculated_field_count++;
                 else
                fixed_field_count++;


            if (m_grid->getColumnModelIdx(i) != -1  && /* check if on a real field, not a col separator */
                tmodel->getColumnCalculated(model_idx) &&
                column_props_panel_shown)
            {
                // if column properties panel is showing, don't allow any edit at the moment
                return;
            }
        }
    }
    
    xd::IDatabasePtr db = g_app->getDatabase();
    xd::IIndexInfoEnumPtr index_enum = db->getIndexEnum(m_path);

    wxMenu menuPopup;

    menuPopup.Append(ID_Data_SortAscending, _("Sort &Ascending"));
    menuPopup.Append(ID_Data_SortDescending, _("&Sort Descending"));
    menuPopup.AppendSeparator();
    menuPopup.Append(27700, _("Sort &Orders"), createIndexesMenu(index_enum, getSortOrder(), 27700));
    menuPopup.Append(ID_Data_RemoveSortFilter, _("&Remove Sort/Filter"));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Table_InsertGroupBreak, _("Insert &Group Break"));
    menuPopup.Append(ID_Table_RemoveGroupBreak, _("Re&move Group Break"));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Data_CreateDynamicField, _("Insert &Calculated Field..."));
    if (calculated_field_count > 0 && selected_count == 1)
        menuPopup.Append(ID_Data_ModifyDynamicField, _("&Edit Calculated Field..."));
    if (fixed_field_count == 0)
    {
        menuPopup.Append(ID_Data_MakeDynamicFieldStatic, _("Convert to Fixed &Field"));
        menuPopup.Append(ID_Edit_Delete, _("&Delete Calculated Field"));
    }
    menuPopup.AppendSeparator();
    menuPopup.Append(27600, _("Column &Views"),
                     createViewsMenu(getModel()->getViewEnum(), m_active_view, 27600));
    menuPopup.Append(ID_Table_EditViews, _("&Insert Column..."));
    menuPopup.Append(ID_Table_HideColumn, _("&Hide Column"));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Data_Summary, _("Summari&ze"));


    CommandCapture* cc = new CommandCapture;
    PushEventHandler(cc);
    PopupMenu(&menuPopup, pt_mouse);
    int command = cc->getLastCommandId();
    PopEventHandler(true);
    
    if (command >= 27600 && command <= 27699)
    {
        int i = command-27600;
        
        ITableDocViewEnumPtr views = m_model->getViewEnum();
        if (i >= (int)views->size())
        {
            // user clicked the "Edit..." menu item
            showViewPanel();
        }
         else
        {
            // user clicked on one of the views;
            // set that view to the active view
            ITableDocViewPtr view = views->getItem(i);
            setActiveView(view);
        }
    }
     else if (command >= 27700 && command <= 27799)
    {
        int i = command-27700;
        
        // default sort order
        if (i == 0)
        {
            setSortOrder(L"");
            return;
        }


        xd::IIndexInfoEnumPtr indexes;
        indexes = db->getIndexEnum(m_path);
        
        if (i >= (int)(indexes->size()+1))
        {
            // user clicked the "Edit..." menu item
            showIndexPanel();
        }
         else
        {
            // user clicked on one of the indexes;
            // set that index as the sort order
            xd::IIndexInfoPtr index = indexes->getItem(i-1);
            setSortOrder(index->getExpression());
        }
    }
     else if (command != 0)
    {
        wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, command);
        ::wxPostEvent(g_app->getMainWindow()->GetEventHandler(), evt);
    }
}

void TableDoc::onGridCursorMove(kcl::GridEvent& evt)
{
    if (evt.GetColumn() != evt.GetDestinationColumn())
    {
    }

    if (evt.GetRow() != evt.GetDestinationRow())
    {
        updateStatusBar(false);
        updateChildWindows();
    }
}



static void setColumnProps(wxColor* fill_color,
                           wxColor* text_color,
                           int alignment)
{
    ITableDocPtr table_doc;

    IDocumentSitePtr doc_site = g_app->getMainFrame()->getActiveChild();
    if (doc_site.isNull())
        return;

    table_doc = doc_site->getDocument();
    if (table_doc.isNull())
        return;

    kcl::Grid* grid;

    grid = table_doc->getGrid();
    if (!grid)
        return;

    std::vector<int> action_cols;

    int sel_count = grid->getSelectionCount();
    if (sel_count > 0)
    {
        int col_count = grid->getColumnCount();
        for (int i = 0; i < col_count; ++i)
        {
            if (grid->isColumnSelected(i))
            {
                action_cols.push_back(i);
            }
        }
    }
     else
    {
        action_cols.push_back(grid->getCursorColumn());
    }
    
    ITableDocViewPtr active_view = table_doc->getActiveView();

    std::vector<int>::iterator it;
    wxColor fg, bg;
    for (it = action_cols.begin(); it != action_cols.end(); ++it)
    {
        grid->getColumnColors(*it, fg, bg);

        if (text_color != NULL)
        {
            fg = *text_color;
        }
        
        if (fill_color != NULL)
        {
            bg = *fill_color;
        }

        grid->setColumnColors(*it, fg, bg);

        if (alignment != -1)
        {
            int grid_align;
            switch (alignment)
            {
                default:
                case tabledocAlignDefault:
                    grid_align = kcl::Grid::alignDefault;
                    break;
                case tabledocAlignLeft:
                    grid_align = kcl::Grid::alignLeft;
                    break;
                case tabledocAlignCenter:
                    grid_align = kcl::Grid::alignCenter;
                    break;
                case tabledocAlignRight:
                    grid_align = kcl::Grid::alignRight;
            }

            grid->setColumnAlignment(*it, grid_align);
        }

        if (active_view.isOk())
        {
            ITableDocViewColPtr col_info;
            col_info = active_view->getColumn(*it);
            
            if (col_info.isNull())
            {
                continue;
            }

            if (text_color != NULL)
            {
                fg = *text_color;
                col_info->setForegroundColor(fg);
            }
            
            if (fill_color != NULL)
            {
                bg = *fill_color;
                col_info->setBackgroundColor(bg);
            }

            if (alignment != -1)
            {
                col_info->setAlignment(alignment);
            }
        }
    }

    grid->refresh(kcl::Grid::refreshAll);
}

void TableDoc::resetChildWindows()
{
    xd::IDatabasePtr db = g_app->getDatabase();

    xd::IRelationSchemaPtr rels = db;
    if (rels.isNull())
        return;

    xd::IRelationEnumPtr rel_enum;
    rel_enum = rels->getRelationEnum(m_path);

    if (rel_enum->size() == 0)
        return;

    wxString site_name;
    IDocumentSitePtr site;
    ITableDocPtr table_doc;
    xd::IRelationPtr rel;
    
    size_t i,rel_count = rel_enum->size();

    for (i = 0; i < rel_count; ++i)
    {
        rel = rel_enum->getItem(i);

        site_name = m_path;
        site_name += ";";
        site_name += rel->getTag();
        site_name.MakeLower();

        site = g_app->getMainFrame()->lookupSite(site_name);
        
        if (site.isNull())
        {
            continue;
        }

        table_doc = site->getDocument();
        if (table_doc.isOk())
        {
            // reset child window with original path
            table_doc->open(table_doc->getPath());
            table_doc->setCaption(wxT(""), wxT(""));
            site->setName(wxT(""));
            table_doc->setIsChildSet(false);
        }

    }
}


static wxString generateContextSyncMarkExpression(
                                const wxString& left_expr,
                                const wxString& right_expr,
                                xd::IIteratorPtr& left_iter,
                                xd::IIteratorPtr& right_iter)
{
    std::vector<wxString> left_parts;
    std::vector<wxString> right_parts;
    
    wxStringTokenizer t1(left_expr, wxT(","));
    while (t1.HasMoreTokens())
    {
        wxString s = t1.GetNextToken();
        s.Trim();
        s.Trim(FALSE);
        left_parts.push_back(s);
    }

    wxStringTokenizer t2(right_expr, wxT(","));
    while (t2.HasMoreTokens())
    {
        wxString s = t2.GetNextToken();
        s.Trim();
        s.Trim(FALSE);
        right_parts.push_back(s);
    }
    
    
    wxString res;
    
    size_t idx = 0;
    std::vector<wxString>::iterator it;
    for (it = right_parts.begin(); it != right_parts.end(); ++it)
    {
        wxString part, value;
        
        xd::objhandle_t lh = left_iter->getHandle(towstr(left_parts[idx]));
        idx++;
        if (!lh)
            return "";
        
        xd::objhandle_t rh = right_iter->getHandle(towstr(*it));
        if (!rh)
            return "";
        
        part = xd::quoteIdentifierIfNecessary(g_app->getDatabase(), towstr(*it));
        part += "=";
        
        xd::IColumnInfoPtr info = right_iter->getInfo(rh);
        if (info.isNull())
            return "";
            
        switch (info->getType())
        {
            case xd::typeCharacter:
            case xd::typeWideCharacter:
                part += "'";
                part += doubleQuote(left_iter->getWideString(lh), '\'');
                part += "'";
                break;
            
            case xd::typeInteger:
                part += wxString::Format("%d", left_iter->getInteger(lh));
                break;
                
            case xd::typeNumeric:
            case xd::typeDouble:
                value = wxString::Format("%.*f", info->getScale(), left_iter->getDouble(lh));
                value.Replace(",", ".");
                part += value;
                break;
                
            case xd::typeDate:
            {
                xd::datetime_t dtt = left_iter->getDateTime(lh);
                xd::DateTime dt;
                dt.setDateTime(dtt);

                part += wxString::Format(wxT("{d '%04d-%02d-%02d'}"),
                                       dt.getYear(), dt.getMonth(), dt.getDay());
                break;
            }
            
            case xd::typeDateTime:
            {
                xd::datetime_t dtt = left_iter->getDateTime(lh);
                xd::DateTime dt;
                dt.setDateTime(dtt);

                part += wxString::Format(wxT("{ts '%04d-%02d-%02d %02d:%02d:%02d' }"),
                             dt.getYear(), dt.getMonth(), dt.getDay(),
                             dt.getHour(), dt.getMinute(), dt.getSecond());
                break;
            }
        }
        
        if (res.Length() > 0)
            res += wxT(" AND ");
        res += part;
    }
        
    return res;
}

void TableDoc::updateChildWindows()
{
    if (m_relationship_sync == tabledocRelationshipSyncNone)
        return;

    xd::IRelationSchemaPtr rels = g_app->getDatabase();
    if (rels.isNull())
        return;

    xd::IRelationEnumPtr rel_enum;
    rel_enum = rels->getRelationEnum(m_path);

    if (rel_enum->size() == 0)
        return;

    // this will ensure that our iterator is positioned
    // on the correct row
    if (m_grid)
    {
        IXdGridModelPtr grid_model = m_grid->getModel();
        if (grid_model)
        {
            grid_model->getRowId(m_grid->getCursorRow());
        }
    }


    wxString site_name;
    IDocumentSitePtr site;
    ITableDocPtr table_doc;
    xd::IRelationPtr rel;
    size_t i, rel_count = rel_enum->size();


    IDocumentSiteEnumPtr doc_sites;
    doc_sites = g_app->getMainFrame()->getDocumentSites(sitetypeNormal);

    for (i = 0; i < rel_count; ++i)
    {
        rel = rel_enum->getItem(i);

        site_name = m_path;
        site_name += ";";
        site_name += rel->getTag();
        site_name.MakeLower();


        site = g_app->getMainFrame()->lookupSite(site_name);
        
        if (site.isNull())
        {
            // we could not find a site with our link label, so we
            // must try to find a suitable site (one with the
            // same set as our "right set"

            bool found = false;
                        
            size_t i, site_count = doc_sites->size();
            for (i = 0; i < site_count; ++i)
            {
                site = doc_sites->getItem(i);
                table_doc = site->getDocument();

                if (table_doc.p == static_cast<ITableDoc*>(this))
                {
                    // don't use ourself
                    continue;
                }

                if (table_doc)
                {
                    std::wstring path = table_doc->getPath();
                    if (isSamePath(path, rel->getRightTable()))
                    {
                        found = true;
                        site->setName(site_name);
                        break;
                    }
                }
            }

            if (!found)
            {
                continue;
            }
        }


        table_doc = site->getDocument();
        if (table_doc.isOk())
        {
            xd::IIteratorRelationPtr iter_r = m_iter;
            if (iter_r.isNull())
                return;
                    
            if (m_relationship_sync == tabledocRelationshipSyncFilter)
            {       
                xd::IIteratorPtr right_iter = iter_r->getFilteredChildIterator(rel);
                if (right_iter)
                {
                    table_doc->setIterator(right_iter);

                    wxString suffix = " ";
                    suffix += _("[Matching Records]");

                    table_doc->setCaption("", suffix);

                    table_doc->updateChildWindows();
                    table_doc->setIsChildSet(true);
                }
            }
             else if (m_relationship_sync == tabledocRelationshipSyncSeek)
            {
                xd::IIteratorPtr right_iter = iter_r->getChildIterator(rel);
                if (right_iter)
                {
                    wxString expr = generateContextSyncMarkExpression(
                                        rel->getLeftExpression(),
                                        rel->getRightExpression(),
                                        m_iter,
                                        right_iter);
                    
                    table_doc->setRelationshipSyncMarkExpr(expr);
                    table_doc->setIterator(right_iter, false);
                    
                    wxString suffix;
                    suffix = wxT(" ");
                    suffix += _("[Matching Records And Context]");
                    table_doc->setCaption("", suffix);

                    table_doc->updateChildWindows();
                    table_doc->setIsChildSet(true);
                }
                 else
                {
                    table_doc->setRelationshipSyncMarkExpr(wxT(""));
                    table_doc->getGrid()->refresh(kcl::Grid::refreshAll);
                }
            }
        }
    }
}


void TableDoc::onGridColumnResize(kcl::GridEvent& evt)
{
    ITableDocViewColPtr colp = m_active_view->getColumn(evt.GetColumn());
    colp->setSize(evt.GetInt());

    if (evt.GetUserEvent())
    {
        FrameworkEvent* e = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_DO_VIEW_RELOAD,
                                       (unsigned long)this);
        e->l_param2 = (unsigned long)m_active_view.p;
        m_frame->postEvent(e);
        g_app->processIdle();
    }
}

void TableDoc::onGridRowResize(kcl::GridEvent& evt)
{
    if (evt.GetUserEvent())
    {
        if (m_active_view)
        {
            m_active_view->setRowSize(evt.GetInt());
        }
    }
}

void TableDoc::onGridRowRightClick(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    
    if (::wxGetKeyState(WXK_SHIFT))
    {
        m_grid->clearSelection();

        kcl::SelectionRect s;
        s.m_start_col = 0;
        s.m_end_col = -1;
        s.m_start_row = std::min(row, m_grid->getCursorRow());
        s.m_end_row = std::max(row, m_grid->getCursorRow());
        m_grid->addSelection(&s);
        m_grid->refresh(kcl::Grid::refreshPaint);
    }
     else if (::wxGetKeyState(WXK_CONTROL))
    {
        if (!m_grid->isRowSelected(row))
            m_grid->setRowSelected(row, true);
            
        m_grid->moveCursor(row, 0);
        m_grid->refresh(kcl::Grid::refreshPaint);
    }
     else
    {
        if (!m_grid->isRowSelected(row))
        {
            m_grid->clearSelection();
            m_grid->setRowSelected(row, true);
            m_grid->moveCursor(row, 0);
            m_grid->refresh(kcl::Grid::refreshPaint);
        }
    }
        
    
    // flag to determine if the "Delete Record" menu item is enabled
    m_allow_delete_menuid = isDeleteAllowed(this, true /* we would be deleting rows */);

    wxMenu menuPopup;
    menuPopup.Append(ID_Edit_Copy, _("&Copy"));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Edit_Delete, _("&Delete Record"));

    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);
    PopupMenu(&menuPopup, pt_mouse);
}

void TableDoc::onGridRowSashDblClick(kcl::GridEvent& evt)
{
    if (m_active_view)
    {
        IAppPreferencesPtr prefs = g_app->getAppPreferences();
        int row_height = getAppPrefsLong(wxT("grid.row_height"));
        
        m_active_view->setRowSize(row_height);
        
        refreshActiveView();
    }
}


void TableDoc::onGridColumnMove(kcl::GridEvent& evt)
{
    FrameworkEvent* e;

    m_active_view->moveColumn(evt.GetColumn(), evt.GetDestinationColumn());

    e = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_VIEW_MODIFIED, 0);
    e->s_param = wxT("colmove");
    m_frame->postEvent(e);

    e = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_DO_VIEW_RELOAD, (unsigned long)this);
    e->l_param2 = (unsigned long)m_active_view.p;
    m_frame->postEvent(e);
}


void TableDoc::onGridNeedTooltipText(kcl::GridEvent& event)
{
    int row = event.GetRow();
    int col = event.GetColumn();

    if (!m_grid)
        return;
        
    if (!m_iter)
        return;
        
    kcl::IModelPtr model = m_grid->getModel();
    if (!model)
        return;
        
    int model_idx = m_grid->getColumnModelIdx(col);

        
    kcl::IModelColumnPtr grid_colinfo = model->getColumnInfo(model_idx);
    if (grid_colinfo.isNull())
        return;

    wxString colname = grid_colinfo->getName();

    if (row == -1)
    {
        xd::objhandle_t colhandle = m_iter->getHandle(towstr(colname));
        if (!colhandle)
            return;

        xd::IColumnInfoPtr colinfo = m_iter->getInfo(colhandle);
        if (colinfo.isNull())
            return;

        event.SetString(wxString::Format(_("Name: %s, Type: %s, Width: %d, Decimals: %d"),
                            makeProperIfNecessary(colinfo->getName()).c_str(),
                            getDbColumnTypeText(colinfo->getType()).c_str(),
                            colinfo->getWidth(),
                            colinfo->getScale()));
    }
     else
    {
        xd::objhandle_t colhandle = m_iter->getHandle(towstr(colname));
        if (!colhandle)
            return;

        xd::IColumnInfoPtr colinfo = m_iter->getInfo(colhandle);
        if (colinfo.isNull())
            return;

        int coltype = colinfo->getType();

        if (coltype == xd::typeDate ||
            coltype == xd::typeDateTime)
        {
            const wxChar* date_rep = wxT("%#c");

            if (coltype == xd::typeDate)
            {
                date_rep = wxT("%#x");
            }

            // -- move model --
            if (!model->isRowValid(row))
            {
                m_iter->releaseHandle(colhandle);
                return;
            }

            xd::datetime_t dt = m_iter->getDateTime(colhandle);
            if (dt != 0)
            {
                xd::DateTime d(dt);

                if (d.getYear() < 1900 || d.getYear() > 3000 ||
                    d.getHour() < 0 || d.getHour() > 23)
                {
                    event.SetString(wxT(""));
                    m_iter->releaseHandle(colhandle);
                    return;
                }

                struct tm tm;
                tm.tm_isdst = -1;
                tm.tm_hour = d.getHour();
                tm.tm_min = d.getMinute();
                tm.tm_sec = d.getSecond();
                tm.tm_mday = d.getDay();
                tm.tm_mon = d.getMonth()-1;
                tm.tm_year = d.getYear()-1900;

                int a = (14-d.getMonth())/12;
                int y = d.getYear() - a;
                int m = d.getMonth() + (12*a) - 2;
                tm.tm_wday = ((d.getDay() + y + (y/4) - (y/100) + (y/400) + ((31*m)/12)) % 7);

                wxChar buf[255];
                wxStrftime(buf, 254, date_rep, &tm);

                event.SetString(buf);
            }

            m_iter->releaseHandle(colhandle);
        }
    }
}


void TableDoc::onGridBeginEdit(kcl::GridEvent& evt)
{
    if (!m_override_beginedit)
    {
        if (g_app->isDatabaseReadOnly())
        {
            evt.Veto();
            return;
        }
    }

    // disallow the editing of calculated fields
    int model_idx = m_grid->getColumnModelIdx(evt.GetColumn());
    if (model_idx != -1)
    {
        IXdGridModelPtr model = m_grid->getModel();
        if (model.isNull())
            return;

        if (model->getColumnCalculated(model_idx))
        {
            evt.Veto();
            return;
        }
    }
}



std::wstring TableDoc::getWhereExpressionForRow(int row)
{
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return L"";

    kcl::IModelPtr model = m_grid->getModel();
    IXdGridModelPtr xd_grid_model = model;
    if (xd_grid_model.isNull())
        return L"";

    if (!model->isRowValid(row))
        return L"";

    std::wstring primary_key;

    xd::IFileInfoPtr info = db->getFileInfo(m_path);
    if (info.isOk())
    {
        primary_key = info->getPrimaryKey();
    }

    std::wstring db_driver = getDbDriver();

    // can't update -- table doesn't have a primary key or a rowid
    if (primary_key.empty() && db_driver != L"xdnative" && db_driver != L"xdclient" && db_driver != L"xdcommon")
        return L"";

    xd::rowid_t rowid = xd_grid_model->getRowId(row);
    if ((db_driver == L"xdnative" || db_driver == L"xdclient" || db_driver == L"xdcommon") && rowid == 0)
        return L"";


    // first, we need to build a where clause
    std::wstring where_str;
    
    if (primary_key.length() == 0)
    {
        // if we don't have a primary key, use the row id
        where_str += L"rowid=";
        where_str += kl::stdswprintf(L"'%08X%08X'",
                                (unsigned int)(rowid >> 32),
                                (unsigned int)(rowid & 0xffffffff));
    }
     else
    {
        // split out primary key field names into an array
        std::vector<std::wstring> prikeys;
        kl::parseDelimitedList(primary_key, prikeys, ',');

        std::vector<std::wstring>::iterator it;
        for (it = prikeys.begin(); it != prikeys.end(); ++it)
        {
            kl::trim(*it);

            xd::objhandle_t handle = getTemporaryHandle(*it);
            if (!handle)
                return L"";
            
            int type = m_iter->getType(handle);
            
            std::wstring piece;
            piece = L"(";
            piece += xd::quoteIdentifierIfNecessary(db, *it);
            piece += L"=";
            
            switch (type)
            {
                case xd::typeCharacter:
                case xd::typeWideCharacter:
                    piece += L"'";
                    piece += m_iter->getWideString(handle);
                    piece += L"'";
                    break;
                    
                case xd::typeInteger:
                {
                    int i = m_iter->getInteger(handle);
                    wxString str = wxString::Format(wxT("%d"), i);
                    piece += towstr(str);
                }
                break;
                
                case xd::typeNumeric:
                case xd::typeDouble:
                {
                    xd::IColumnInfoPtr info = m_iter->getInfo(handle);
                    if (info.isNull())
                        return L"";
                    double d = m_iter->getDouble(handle);
                    int scale = info->getScale();
                    wxString str = wxString::Format(wxT("%.*f"), scale, d);
                    piece += towstr(str);
                }
                break;
                
                case xd::typeDate:
                {
                    xd::datetime_t dtt = m_iter->getDateTime(handle);
                    xd::DateTime dt;
                    dt.setDateTime(dtt);
                    wxString str;
                    
                    if (getDbDriver() == L"xdoracle")
                    {
                        str = wxString::Format(wxT("TO_DATE('%04d-%02d-%02d','YYYY-MM-DD')"),
                                               dt.getYear(), dt.getMonth(), dt.getDay());
                    } 
                     else
                    {
                        str = wxString::Format(wxT("{d '%04d-%02d-%02d'}"),
                                               dt.getYear(), dt.getMonth(), dt.getDay());
                    }
                    
                    piece += towstr(str);
                }
                break;
            }
            
            piece += L")";
            
            if (where_str.length() > 0)
                where_str += L" AND ";
            where_str += piece;            
        }
    }

    return where_str;
}


static std::wstring formatSqlPath(const std::wstring& path)
{
    std::wstring result = path;
    kl::trim(result);
    if (result.length() > 0 && kl::stringFrequency(path, '/') == 1 && result[0] == '/')
        return result.substr(1);
         else
        return result;
}


void TableDoc::onGridEndEdit(kcl::GridEvent& evt)
{
    // check if the edit was cancelled
    if (evt.GetEditCancelled())
        return;

    // update the database with the new cell value
    int model_col = m_grid->getColumnModelIdx(evt.GetColumn());
    if (model_col == -1)
        return;

    xd::IDatabasePtr db = g_app->getDatabase();
    kcl::IModelPtr model = m_grid->getModel();

    IXdGridModelPtr xd_grid_model = model;
    if (xd_grid_model.isNull())
        return;

    kcl::IModelColumnPtr model_colinfo = model->getColumnInfo(model_col);
    if (model_colinfo.isNull())
        return;
        
    std::wstring primary_key;

    xd::IFileInfoPtr info = db->getFileInfo(m_path);
    if (info.isOk())
    {
        primary_key = info->getPrimaryKey();
    }

    std::wstring db_driver = getDbDriver();

    // can't update -- table doesn't have a primary key or a rowid
    if (primary_key.empty() && db_driver != L"xdnative" && db_driver != L"xdclient")
    {
        deferredAppMessageBox(_("The database table does not have a primary key specified, which is required for editing data values."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_INFORMATION | wxCENTER);
        return;
    }

    wxString col_name = model_colinfo->getName();

    xd::rowid_t rowid = xd_grid_model->getRowId(m_grid->getCursorRow());
    if ((db_driver == L"xdnative" || db_driver == wxT("xdclient")) && rowid == 0)
        return;


    // first, we need to build a where clause
    std::wstring where_str;
    
    if (primary_key.length() == 0)
    {
        // if we don't have a primary key, use the row id
        where_str += wxT(" WHERE rowid=");
        where_str += wxString::Format(wxT("'%08X%08X'"),
                                (unsigned int)(rowid >> 32),
                                (unsigned int)(rowid & 0xffffffff));
    }
     else
    {
        // split out primary key field names into an array
        std::vector<wxString> prikeys;
        
        wxStringTokenizer t(primary_key, wxT(","));
        while (t.HasMoreTokens())
        {
            wxString s = t.GetNextToken();
            s.Trim();
            s.Trim(false);
            prikeys.push_back(xd::dequoteIdentifier(db, towstr(s)));
        }
        
        std::vector<wxString>::iterator it;
        for (it = prikeys.begin(); it != prikeys.end(); ++it)
        {
            xd::objhandle_t handle = getTemporaryHandle(*it);
            if (!handle)
                return;
            
            int type = m_iter->getType(handle);
            
            std::wstring piece;
            piece = L"(";
            piece += xd::quoteIdentifierIfNecessary(db, towstr(*it));
            piece += L"=";
            
            switch (type)
            {
                case xd::typeCharacter:
                case xd::typeWideCharacter:
                    piece += L"'";
                    piece += m_iter->getWideString(handle);
                    piece += L"'";
                    break;
                    
                case xd::typeInteger:
                {
                    int i = m_iter->getInteger(handle);
                    wxString str = wxString::Format(wxT("%d"), i);
                    piece += towstr(str);
                }
                break;
                
                case xd::typeNumeric:
                case xd::typeDouble:
                {
                    xd::IColumnInfoPtr info = m_iter->getInfo(handle);
                    if (info.isNull())
                        return;
                    double d = m_iter->getDouble(handle);
                    int scale = info->getScale();
                    wxString str = wxString::Format(wxT("%.*f"), scale, d);
                    piece += towstr(str);
                }
                break;
                
                case xd::typeDate:
                {
                    xd::datetime_t dtt = m_iter->getDateTime(handle);
                    xd::DateTime dt;
                    dt.setDateTime(dtt);
                    wxString str;
                    
                    if (getDbDriver() == L"xdoracle")
                    {
                        str = wxString::Format(wxT("TO_DATE('%04d-%02d-%02d','YYYY-MM-DD')"),
                                               dt.getYear(), dt.getMonth(), dt.getDay());
                    } 
                     else
                    {
                        str = wxString::Format(wxT("{d '%04d-%02d-%02d'}"),
                                               dt.getYear(), dt.getMonth(), dt.getDay());
                    }
                    
                    piece += towstr(str);
                }
                break;
            }
            
            piece += L")";
            
            if (where_str.length() > 0)
                where_str += L" AND ";
            where_str += piece;            
        }
        
        where_str = L" WHERE " + where_str;
    }


    xd::IStructurePtr structure = db->describeTable(m_path);
    if (structure.isNull())
        return;

    xd::IColumnInfoPtr col_info = structure->getColumnInfo(towstr(col_name));
    if (col_info.isNull())
        return;


    wxString quoted_col_name = xd::quoteIdentifierIfNecessary(db, towstr(col_name));


    // update_info is used by ICacheRowUpdate below, however only
    // iterators that employ a row cache need this
    xd::ColumnUpdateInfo update_info;
    update_info.null = false;

    wxString str;
    switch (col_info->getType())
    {
        case xd::typeCharacter:
        case xd::typeWideCharacter:
        {
            // fill out update_info for ICacheRowUpdate below
            update_info.wstr_val = towstr(evt.GetString());
            update_info.str_val = kl::tostring(update_info.wstr_val);

            // double the quotes, if present
            wxString in = evt.GetString();
            wxString out;

            in.Trim(TRUE);  // trim right

            const wxChar* v = (const wxChar*)in;

            while (*v)
            {
                if (*v == wxT('\''))
                    out += wxT("''");
                     else
                    out += *v;

                v++;
            }

            str = wxString::Format(wxT("%s='%s'"), quoted_col_name.c_str(),
                                                   out.c_str());
            break;
        }

        case xd::typeInteger:
            // fill out update_info for ICacheRowUpdate below
            update_info.int_val = evt.GetInt();

            str = wxString::Format(wxT("%s=%d"), quoted_col_name.c_str(),
                                                 evt.GetInt());
            break;
            
        case xd::typeDouble:
        case xd::typeNumeric:
        {
            // fill out update_info for ICacheRowUpdate below
            update_info.dbl_val = evt.GetDouble();

            wxString num = wxString::Format(wxT("%.*f"), col_info->getScale(),
                                                         evt.GetDouble());
            num.Replace(wxT(","), wxT("."));

            str = quoted_col_name;
            str += wxT("=");
            str += num;
        }
        break;
        
        case xd::typeBoolean:
            // fill out update_info for ICacheRowUpdate below
            update_info.bool_val = evt.GetBoolean();

            if (getDbDriver() == L"xdnative")
            {
                str = wxString::Format(wxT("%s=%s"), quoted_col_name.c_str(),
                                                     evt.GetBoolean() ? wxT("true") : wxT("false"));
            }
             else
            {
                str = wxString::Format(wxT("%s=%s"), quoted_col_name.c_str(),
                                                     evt.GetBoolean() ? wxT("1") : wxT("0"));
            }
            
            break;
            
        case xd::typeDateTime:
        case xd::typeDate:
        {
            int y, m, d, hh, mm, ss;
            bool valid = Locale::parseDateTime(evt.GetString(),
                                                    &y, &m, &d,
                                                    &hh, &mm, &ss);

            if (!valid)
            {
                if (evt.GetUserEvent())
                {
                    appMessageBox(_("The date entered was not formatted correctly."),
                                       APPLICATION_NAME,
                                       wxOK | wxICON_EXCLAMATION | wxCENTER);

                    m_grid->SetFocus();
                }

                evt.Veto();
                return;
            }

            if (hh == -1 || mm == -1)
            {
                update_info.date_val = xd::DateTime(y, m, d);
                
                
                if (getDbDriver() == L"xdoracle")
                {
                    str = wxString::Format(wxT("%s=TO_DATE('%04d-%02d-%02d','YYYY-MM-DD')"),
                                           quoted_col_name.c_str(),
                                           y, m, d);
                } 
                 else
                {
                    str = wxString::Format(wxT("%s={d '%04d-%02d-%02d'}"),
                                           quoted_col_name.c_str(),
                                           y, m, d);
                }
            }
             else
            {
                update_info.date_val = xd::DateTime(y, m, d, hh, mm, ss);
                
                if (getDbDriver() == L"xdoracle")
                {
                    str = wxString::Format(wxT("%s=TO_DATE('%04d-%02d-%02d %02d:%02d:%02d', 'YYYY-MM-DD HH24:MI:SS')"),
                                           quoted_col_name.c_str(),
                                           y, m, d, hh, mm, ss);
                } 
                 else
                {
                    str = wxString::Format(wxT("%s={ts '%04d-%02d-%02d %02d:%02d:%02d'}"),
                                           quoted_col_name.c_str(),
                                           y, m, d, hh, mm, ss);
                }
            }
        }

        break;
    }

    str.Trim();
    if (str.Length() == 0)
        return;


    std::wstring cmd = L"UPDATE ";
    cmd += xd::quoteIdentifierIfNecessary(db, formatSqlPath(m_path));
    cmd += L" SET ";
    cmd += towstr(str);
    cmd += where_str;
            
    xcm::IObjectPtr result_obj;
    if (g_app->getDatabase()->execute(cmd,
                                      xd::sqlPassThrough,
                                      result_obj,
                                      NULL))
    {
        // some iterators are forward-only and cache the data, so that
        // the grid view can be scrolled bidirectionally.  If this is the
        // case, update the iterators cache.
        xd::ICacheRowUpdatePtr update = m_iter;
        xd::objhandle_t handle = getTemporaryHandle(col_name);
        if (update.isOk() && handle)
        {
            update_info.handle = handle;
            update->updateCacheRow(m_iter->getRowId(), &update_info, 1);
        }
    }
    
    if (m_mount_db)
    {
        xcm::IObjectPtr result;
        m_mount_db->execute(L"COMMIT", 0, result, NULL);
    }
    
    
    // this next line forces our model to get recalculate
    // where it is.  This must be done because the key
    // value may have been edited and the iterator repositioned
    if (getDbDriver() == L"xdnative")
        model->reset();

    m_grid->refresh(kcl::Grid::refreshAll);
}


void TableDoc::onGridPreGhostRowInsert(kcl::GridEvent& evt)
{
    if (!g_app->getAppController()->doReadOnlyCheck())
    {
        evt.Veto();
        return;
    }

    xd::IRowInserterPtr inserter = g_app->getDatabase()->bulkInsert(m_path);
    if (inserter.isNull())
        return;

    inserter->startInsert(L"*");
    inserter->insertRow();
    inserter->finishInsert();
    updateStatusBar();
}

void TableDoc::onGridKeyDown(kcl::GridEvent& evt)
{
    // note: handle the grid key event; if the key is a delete
    // key, try to delete any selected rows or columns; if the
    // event isn't handled, skip the event so the grid has a
    // chance to handle it

    // if the keycode is a delete key, delete any selected row
    // or column
    bool handled = false;
    int keycode = evt.GetKeyEvent().GetKeyCode();
    if (keycode == WXK_DELETE ||
        keycode == WXK_NUMPAD_DELETE)
    {
        handled = deleteSelectedRowsColumns();
    }

    // if the shift or control characters are down, highlight
    // the row or column, respectively
    if (keycode == WXK_SPACE)
    {
        if (evt.GetKeyEvent().ShiftDown())
        {
            // when shift and space are pressed, select the row
            m_grid->clearSelection();
            m_grid->setRowSelected(m_grid->getCursorRow(), true);
            m_grid->refresh(kcl::Grid::refreshAll);
            handled = true;
        }

        if (evt.GetKeyEvent().ControlDown())
        {
            // when ctrl and space are pressed, select the column
            m_grid->clearSelection();
            m_grid->setColumnSelected(m_grid->getCursorColumn(), true);
            m_grid->refresh(kcl::Grid::refreshAll);
            handled = true;            
        }
    }

    // if the event wasn't handled, skip the event so the grid
    // has a chance to handle it
    if (!handled)
    {
        evt.Skip();
    }
}


void TableDoc::onHideColumn(wxCommandEvent& evt)
{
    std::vector<int> to_hide;
    
    int col_count = m_grid->getColumnCount();
    for (int i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
            to_hide.push_back(i);
    }

    m_grid->clearSelection();

    if (to_hide.size() == 0)
        to_hide.push_back(m_grid->getCursorColumn());

    std::vector<int>::iterator it;
    int hidden_count = 0;
    for (it = to_hide.begin(); it != to_hide.end(); ++it)
    {
        hideColumn(*it - hidden_count);
        hidden_count++;
    }
}


bool TableDoc::createDynamicField(const wxString& col_name,
                                  int type,
                                  int width,
                                  int scale,
                                  const wxString& expr,
                                  bool on_set)
{
    freeTemporaryHandles();
        
    xd::IStructurePtr structure = m_iter->getStructure();

    xd::IColumnInfoPtr col;
    col = structure->createColumn();

    col->setName(towstr(col_name));
    col->setType(type);
    col->setWidth(width);
    col->setScale(scale);
    col->setExpression(towstr(expr));

    if (m_iter->modifyStructure(structure, NULL))
    {
        m_grid->refreshModel();
    }
     else
    {
        return false;
    }


    if (on_set)
    {
        xd::IDatabasePtr db = g_app->getDatabase();

        xd::IStructurePtr structure = db->describeTable(m_path);
        if (structure.isNull())
            return false;

        xd::IColumnInfoPtr col = structure->createColumn();

        col->setName(towstr(col_name));
        col->setType(type);
        col->setWidth(width);
        col->setScale(scale);
        col->setExpression(towstr(expr));

        if (!db->modifyStructure(m_path, structure, NULL))
        {
            return false;
        }
    }

    return true;
}


void TableDoc::onCreateDynamicFieldOk(ColPropsPanel* panel)
{
    if (m_model.isOk() && m_active_view.isOk())
    {
        m_model->writeObject(m_active_view);
    }
}


void TableDoc::onEditDynamicFieldOk(ColPropsPanel* panel)
{
    // if there were relationships synced, we need to 
    // update the display because the calculated field we
    // just changed may play a role

    int synctype = g_app->getAppController()->getRelationshipSync();
    if (synctype != tabledocRelationshipSyncNone)
        g_app->getAppController()->updateTableDocRelationshipSync(synctype);
}


void TableDoc::onCreateDynamicFieldCancelled(ColPropsPanel* panel)
{
    freeTemporaryHandles();
    
    // we are deleting just calculated fields
    wxString modify_struct = panel->getModifyField();
    xd::IStructurePtr structure = m_iter->getStructure();
    structure->deleteColumn(towstr(modify_struct));
    if (m_iter->modifyStructure(structure, NULL))
    {
        m_grid->refreshModel();

        int idx;
        while (1)
        {
            idx = m_grid->getColumnViewIdxByName(modify_struct);
            if (idx == -1)
                break;

            hideColumn(idx);
        }

        m_grid->clearSelection();
        m_grid->refresh(kcl::Grid::refreshAll);

        // let other windows know that the structure was modified
        FrameworkEvent* evt = new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_STRUCTURE_MODIFIED);
        evt->s_param = m_path;
        m_frame->postEvent(evt);
    }
}


void TableDoc::showCreateDynamicField()
{
    // see if any other calculated field frames are visible
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("CalculatedFieldPropertiesPanel"));
    if (site.isOk())
    {
        if (!site->getVisible())
            site->setVisible(true);
        return;
    }

    m_grid->clearSelection();

    xd::IStructurePtr structure = g_app->getDatabase()->describeTable(m_path);
    if (structure.isNull())
        return;

    int i = 0;
    wxString column_name;
    do
    {
        i++;
        column_name = wxString::Format(wxT("Field%d"), i);
    } while (structure->getColumnExist(towstr(column_name)));


    if (createDynamicField(column_name,
                           xd::typeCharacter,
                           30, 0, wxT("\"\""), false))
    {
        // insert column at cursor position
        insertColumnInternal(m_grid->getCursorColumn(), column_name, false, true);

        ColPropsPanel* panel = new ColPropsPanel;

        panel->sigOkPressed.connect(this,
                                &TableDoc::onCreateDynamicFieldOk);

        panel->sigCancelPressed.connect(this,
                                &TableDoc::onCreateDynamicFieldCancelled);

        panel->setModifyField(column_name);

        site = m_frame->createSite(panel, sitetypeModeless,
                                   -1, -1, 560, 310);
        site->setMinSize(560,310);
        site->setName(wxT("CalculatedFieldPropertiesPanel"));
    }
}

void TableDoc::showEditDynamicField(const wxString& column_name)
{
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("CalculatedFieldPropertiesPanel"));
    if (site.isOk())
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
     else
    {
        ColPropsPanel* panel = new ColPropsPanel;
        panel->setModifyField(column_name);

        panel->sigOkPressed.connect(this,
                        &TableDoc::onEditDynamicFieldOk);

        site = m_frame->createSite(panel,
                                   sitetypeModeless,
                                   -1, -1, 560, 310);
        site->setMinSize(560,310);
        site->setName(wxT("CalculatedFieldPropertiesPanel"));
    }
}


void TableDoc::onCreateDynamicField(wxCommandEvent& evt)
{
    showCreateDynamicField();
}

void TableDoc::onModifyDynamicField(wxCommandEvent& evt)
{
    kcl::IModelPtr model = m_grid->getModel();
    kcl::IModelColumnPtr model_colinfo;
    wxString column_name;
    int model_idx = m_grid->getColumnModelIdx(m_grid->getCursorColumn());
    if (model_idx == -1)
        return;

    model_colinfo = model->getColumnInfo(model_idx);
    column_name = model_colinfo->getName();

    showEditDynamicField(column_name);
}

void TableDoc::onDeleteField(wxCommandEvent& evt)
{
    deleteSelectedColumns();
}

void TableDoc::onCreateNewMark(wxCommandEvent& evt)
{
    createNewMark(wxT(""));

    // find the marks panel and tell it to repopulate its list
    IDocumentSitePtr markmanager_site;
    markmanager_site = g_app->getAppController()->getMarkManagerPanelSite();
    if (markmanager_site.isNull())
        return;

    IMarkMgrPtr mark_panel;
    mark_panel = markmanager_site->getDocument();
    if (mark_panel.isNull())
        return;

    mark_panel->populate(true);
    mark_panel->editMark(m_model->getMarkEnum()->size()-1);
    g_app->getAppController()->showMarkManagerPanel();
}

void TableDoc::onMakeStatic(wxCommandEvent& evt)
{
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    // get the current set path
    wxString object_path = m_path;

    // find out which columns are selected
    kcl::IModelPtr model = m_grid->getModel();
    IXdGridModelPtr tmodel = model;

    std::set<wxString> cols;
    
    int col_count = m_grid->getColumnCount();
    for (int i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
        {
            int model_idx = m_grid->getColumnModelIdx(i);
            if (model_idx == -1)
                continue;

            kcl::IModelColumnPtr col = model->getColumnInfo(model_idx);
            cols.insert(col->getName());
        }
    }

    m_grid->clearSelection();

    if (cols.size() == 0)
    {
        // no columns were selected, so use the current column
        kcl::IModelPtr model = m_grid->getModel();
        kcl::IModelColumnPtr model_colinfo;
        int model_idx = m_grid->getColumnModelIdx(m_grid->getCursorColumn());
        if (model_idx == -1)
        {
            return;
        }
        model_colinfo = model->getColumnInfo(model_idx);
        cols.insert(model_colinfo->getName());
    }




    // make sure that the columns are all dynamic

    xd::IStructurePtr structure = g_app->getDatabase()->describeTable(m_path);
    xd::IColumnInfoPtr colinfo;
           
    std::set<wxString>::iterator it;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        colinfo = structure->getColumnInfo(towstr(*it));

        if (colinfo.isNull())
        {
            appMessageBox(_("One or more of the selected calculated fields does not exist."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        if (!colinfo->getCalculated())
        {
            appMessageBox(_("This operation may only be performed on calculated fields."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            m_grid->clearSelection();
            m_grid->refresh(kcl::Grid::refreshAll);
            return;
        }
    }


    setEnabled(false);
    closeSet();


    // set up the job from the info we gathered
    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.alter-job");

    kl::JsonNode params;
    params["input"].setString(towstr(object_path));
    params["actions"].setArray();

    for (it = cols.begin(); it != cols.end(); ++it)
    {
        kl::JsonNode node = params["actions"].appendElement();
        node["action"].setString(L"modify");
        node["column"].setString(towstr(*it));
        node["params"].setObject();

        kl::JsonNode modify_param = node["params"];
        modify_param["expression"].setNull();
    }


    // create the job
    wxString title = wxString::Format(_("Modifying Structure of '%s'"),
                                        getCaption().c_str());

    job->getJobInfo()->setTitle(towstr(title));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(this, &TableDoc::onAlterTableJobFinished);
    g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
}


void TableDoc::onResizeColumn(wxCommandEvent& evt)
{
    int i, col_count = m_grid->getColumnCount();
    int selected_count = 0;

    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
        {
            m_grid->autoColumnResize(i);
            selected_count++;
        }
    }

    if (selected_count == 0)
    {
        m_grid->autoColumnResize(m_grid->getCursorColumn());
    }
}


void TableDoc::onResizeAllColumns(wxCommandEvent& evt)
{
    m_grid->autoColumnResize(-1);
}


// IColumnListTarget::getColumnListItems

void TableDoc::getColumnListItems(std::vector<ColumnListItem>& list)
{
    list.clear();
    
    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    xd::IStructurePtr structure = m_iter->getStructure();
    if (structure.isNull())
        return;
    
    int i, col_count = structure->getColumnCount();
    list.reserve(col_count);
    
    xd::IColumnInfoPtr colinfo;

    for (i = 0; i < col_count; i++)
    {
        colinfo = structure->getColumnInfoByIdx(i);
        
        bool in_view = false;
        int model_idx = m_grid->getColumnModelIdxByName(colinfo->getName());
        if (model_idx >= 0 && m_grid->getColumnViewIdx(model_idx) != -1)
        {
            in_view = true;
        }
        
        ColumnListItem item;

        item.active = true;
        item.text = makeProperIfNecessary(colinfo->getName());

        if (colinfo->getCalculated())
        {
            item.bitmap = in_view ? GETBMP(gf_lightning_16) : GETDISBMP(gf_lightning_16);
        }
         else
        {
            item.bitmap = in_view ? GETBMP(gf_field_16) : GETDISBMP(gf_field_16);
        }

        list.push_back(item);
    }
    

    // add fields from child file(s)

    xd::IRelationSchemaPtr rels = db;
    if (rels.isOk())
    {
        xd::IRelationEnumPtr rel_enum = rels->getRelationEnum(m_path);
        xd::IRelationPtr rel;
        size_t r, rel_count = rel_enum->size();
        
        wxString s;

        for (r = 0; r < rel_count; ++r)
        {
            rel = rel_enum->getItem(r);

            if (rel.isNull())
                continue;

            std::wstring right_path = rel->getRightTable();

            xd::IStructurePtr right_structure = db->describeTable(right_path);
            if (right_structure.isNull())
                continue;

            int i, col_count = right_structure->getColumnCount();
 
            for (i = 0; i < col_count; ++i)
            {
                colinfo = right_structure->getColumnInfoByIdx(i);

                s = wxString::Format(wxT("%s.%s"),
                            makeProperIfNecessary(rel->getTag()).c_str(),
                            makeProperIfNecessary(colinfo->getName()).c_str());
                
                ColumnListItem item;
                item.text = s;
                item.bitmap = GETBMP(gf_related_field_16);
                item.active = true;
                list.push_back(item);
            }
        }
    }

}

// IColumnListTarget::onColumnListDblClicked
void TableDoc::onColumnListDblClicked(const std::vector<wxString>& items)
{
    std::vector<wxString>::const_iterator it, it_end = items.end();

    int counter = 0;
    for (it = items.begin(); it != it_end; ++it)
    {
        insertColumn(m_grid->getCursorColumn()+counter, *it);
        counter++;
    }
}

void TableDoc::deleteSelectedRows()
{
    // check for external data; if we're on external data, we're done
    if (isExternalTable())
    {
        // show warning
        wxString message = wxString::Format(_("Unable to delete records on external files."));

        int res = wxMessageBox(message,
                               APPLICATION_NAME,
                               wxOK |
                               wxICON_EXCLAMATION |
                               wxCENTER);
        return;
    }

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    // check for read-only flag; if it's set, we're done
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    // check for the model
    IXdGridModelPtr model;

    model = m_grid->getModel();
    if (model.isNull())
        return;

    std::vector<std::wstring> deletecmds;

    // get the selection count and make sure we only have rows
    kcl::SelectionRect sel;
    size_t i, selection_count = m_grid->getSelectionCount();

    for (i = 0; i < selection_count; ++i)
    {
        // if we don't have a row, we're done
        m_grid->getSelection(i, &sel);
        if (sel.m_start_col != 0 || sel.m_end_col != m_grid->getColumnCount()-1)
            return;

        int row;
        for (row = sel.m_start_row; row <= sel.m_end_row; ++row)
        {
            deletecmds.push_back(L"DELETE FROM %tbl% WHERE " + getWhereExpressionForRow(row));
        }
    }


    // ok, now actually do the deletion
    AppBusyCursor bc;

    // show warning
    wxString message = wxString::Format(_("Performing this operation will permanently delete data.  Are you sure\nyou want to delete %d record(s)?"),
                                        deletecmds.size());

    int res = wxMessageBox(message,
                           APPLICATION_NAME,
                           wxYES_NO |
                           wxICON_EXCLAMATION |
                           wxCENTER);
    if (res != wxYES)
        return;

    // delete the records from the table
    std::wstring sql;

    std::vector<std::wstring>::iterator it;
    for (it = deletecmds.begin(); it != deletecmds.end(); ++it)
    {
        sql = *it;
        kl::replaceStr(sql, L"%tbl%", m_path);
        xcm::IObjectPtr resobj;
        db->execute(sql, 0, resobj, NULL);
    }


    // delete the records from the browse set,
    // if it is different from the base set

    if (m_filter.length() > 0 && m_browse_path != m_path)
    {
        for (it = deletecmds.begin(); it != deletecmds.end(); ++it)
        {
            sql = *it;
            kl::replaceStr(sql, L"%tbl%", m_browse_path);
            xcm::IObjectPtr resobj;
            db->execute(sql, 0, resobj, NULL);
        }
    }

    // reset the model; this forces the model to reload,
    // while keeping our position
    m_grid->getModel()->reset();
    
    // get the cursor position, known row count and
    // actual row count
    int old_cursor_pos = m_grid->getCursorRow();
    int known_row_count = m_grid->getKnownRowCount();
    int reported_row_count = m_grid->getRowCount();
    
    // we need to move the cursor if it's on the last row; to do that, we need
    // to know the last row, so we either have to rely on the actual row count
    // or the known row count; however, these may differ depending on whether
    // or not the table is in a relationship or not; further, the known row
    // count is only accurate after refreshing to grid; so if the actual row
    // count is less than the known row count, rely on the actual row count;
    // otherwise refresh the grid and rely on the known row count
    
    int actual_row_count = reported_row_count;
    if (known_row_count <= reported_row_count || reported_row_count < 0)
    {
        m_grid->refresh(kcl::Grid::refreshAll);
        actual_row_count = m_grid->getKnownRowCount();
    }

    // if the cursor row is past the last row of the known
    // number of rows in the grid, set it on the last row
    if (old_cursor_pos >= actual_row_count)
        m_grid->moveCursor(actual_row_count - 1, m_grid->getCursorColumn(), false);

    // clear the selections, and if there are rows left over, 
    // select the cursor row
    m_grid->clearSelection();
    if (actual_row_count > 0)
        m_grid->setRowSelected(m_grid->getCursorRow(), true);

    // refresh the grid and status bar
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void TableDoc::deleteSelectedColumns()
{
    // read only check
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    wxString object_path = m_path;
    xd::IStructurePtr structure = m_iter->getStructure();
    xd::IColumnInfoPtr colinfo;

    std::set<wxString> cols;
    std::set<wxString>::iterator it;

    int col_count = m_grid->getColumnCount();
    int total_phys_fields_to_delete = 0;
    int total_phys_fields = 0;
    int i;

    for (i = 0; i < col_count; ++i)
    {
        colinfo = structure->getColumnInfo(towstr(m_grid->getColumnCaption(i)));
        if (colinfo.isNull())
            continue;

        if (m_grid->isColumnSelected(i))
        {
            cols.insert(colinfo->getName());

            if (!colinfo->getCalculated())
                total_phys_fields_to_delete++;
        }

        if (!colinfo->getCalculated())
            total_phys_fields++;
    }

    // if we're trying to delete all fields, don't do anything
    if (total_phys_fields_to_delete == (size_t)total_phys_fields)
        return;

    // show warning
    wxString message = _("Performing this operation will permanently delete data.  Are you sure\nyou want to delete the following field(s):");
    message += wxT("\n");

    for (it = cols.begin(); it != cols.end(); ++it)
    {
        message += wxT("\n\t");
        message += makeProperIfNecessary(*it);
    }
    
    int res = wxMessageBox(message,
                           APPLICATION_NAME,
                           wxYES_NO |
                           wxICON_EXCLAMATION |
                           wxCENTER);

    if (res != wxYES)
        return;


    // refresh the grid; if we're deleting physical columns, grey out
    // the interface
    m_grid->clearSelection();
    m_grid->refresh(kcl::Grid::refreshAll);

    if (total_phys_fields_to_delete > 0)
    {
        setEnabled(false);
        closeSet();
    }

    // set up the job from the info we gathered
    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.alter-job");

    kl::JsonNode params;
    params["input"].setString(towstr(object_path));
    params["actions"].setArray();

    for (it = cols.begin(); it != cols.end(); ++it)
    {
        kl::JsonNode node = params["actions"].appendElement();
        node["action"].setString(L"drop");
        node["column"].setString(towstr(*it));
    }


    // create the job
    wxString title = wxString::Format(_("Modifying Structure of '%s'"),
                                        getCaption().c_str());

    job->getJobInfo()->setTitle(towstr(title));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(this, &TableDoc::onAlterTableJobFinished);
    g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
}

bool TableDoc::deleteSelectedRowsColumns()
{
    // NOTE: this function deletes the selected rows or columns and
    //       returns true if a row or column is deleted; false otherwise

    // get the selection count
    kcl::SelectionRect sel;
    size_t i, selection_count = m_grid->getSelectionCount();
    size_t col_count = m_grid->getColumnCount();
    
    // if we don't have a selection, we're done
    if (selection_count == 0)
        return false;

    bool delete_rows = true;
    bool delete_columns = true;

    // find out if we're deleting rows
    
    for (i = 0; i < selection_count; ++i)
    {
        m_grid->getSelection(i, &sel);
        if (sel.m_start_col != 0 || sel.m_end_col != m_grid->getColumnCount()-1)
        {
            delete_rows = false;
        }
    }

    // find out if we're deleting columns
    
    int selected_cols = 0;
    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
            selected_cols++;
    }

    if (selected_cols == 0)
        delete_columns = false;

    // if we have both rows and column selections, we're done
    if (delete_rows && delete_columns)
        return false;

    // if we have column selections, do the column deletion
    if (delete_columns)
    {    
        deleteSelectedColumns();
        return true;
    }
        
    // if we have row selections, do the row deletion
    if (delete_rows)
    {
        deleteSelectedRows();
        return true;
    }
    
    // return false; we didn't delete anything
    return false;
}

void TableDoc::deleteAllRelations()
{
    //getBaseSet()->deleteAllRelations();
    //TODO: make sure the above line gets reimplemented
}

void TableDoc::onRemoveAllRelationships(wxCommandEvent& evt)
{
    deleteAllRelations();
}

int TableDoc::getRelationshipSync()
{
    return m_relationship_sync;
}

void TableDoc::setRelationshipSync(int state)
{
    m_relationship_sync = state;

    setRelationshipSyncMarkExpr(wxT(""));

    if (m_relationship_sync != tabledocRelationshipSyncNone)
        updateChildWindows();
         else
        resetChildWindows();
}


void TableDoc::setRelationshipSyncMarkExpr(const wxString& expr)
{
    m_relsync_mark_expr = expr;
}

void TableDoc::setIsChildSet(bool new_val)
{
    m_is_childset = new_val;
}

bool TableDoc::getIsChildSet()
{
    return m_is_childset;
}

std::wstring TableDoc::getDbDriver()
{
    xcm::IObject* p;
    std::wstring class_name;
    xcm::class_info* class_info;

    if (m_iter.isOk())
    {
        p = m_iter.p;
        class_info = xcm::get_class_info(p);
        class_name = towstr(class_info->get_name());
        class_name = kl::beforeFirst(class_name, '.');
        if (class_name != L"xdcommon")
            return class_name; // if it's xdcommon, check the database ptr instead
    }
    

    
    p = g_app->getDatabase().p;
    if (!p)
        return L"";

    class_info = xcm::get_class_info(p);
    class_name = towstr(class_info->get_name());
    return kl::beforeFirst(class_name, '.');
}


void TableDoc::onCut(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }
    
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    if (m_grid->getSelectionCount() != 0)
    {
        appMessageBox(_("Only a single cell may be cut."),
                           APPLICATION_NAME,
                           wxICON_INFORMATION | wxOK);
        return;
    }

    int model_col = m_grid->getColumnModelIdx(m_grid->getCursorColumn());
    if (model_col == -1)
        return;

    wxString s = m_grid->getCellString(m_grid->getCursorRow(), model_col);

    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(s));
        wxTheClipboard->Close();
    }

    m_override_beginedit = true;

    if (!m_grid->isEditing())
    {
        m_grid->beginEdit(wxEmptyString);
    }

    m_override_beginedit = false;
}



void TableDoc::onPaste(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }
    
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    int width = 512;

    int cursor_col = m_grid->getCursorColumn();
    int model_idx = m_grid->getColumnModelIdx(cursor_col);
    if (model_idx != -1)
    {
        kcl::IModelPtr model = m_grid->getModel();
        kcl::IModelColumnPtr model_col;
        model_col = model->getColumnInfo(model_idx);
        width = model_col->getWidth();
    }


    // get the paste text from the clipboard
    wxString text;

    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->IsSupported(wxDF_TEXT))
        {
            wxTextDataObject data;
            wxTheClipboard->GetData(data);
            text = data.GetText();

            m_override_beginedit = true;

            if (!m_grid->isEditing())
            {
                m_grid->beginEdit(text.Left(width).c_str());
            }

            m_override_beginedit = false;
        }
    }

    wxTheClipboard->Close();
}

void TableDoc::onCopy(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }

    AppBusyCursor c;
    m_grid->copySelection();
}



void TableDoc::onGridSelectionChange(kcl::GridEvent& evt)
{
    // flag to determine if the "Delete Field" menu item is enabled
    m_allow_delete_menuid = isDeleteAllowed(this, true /* user might want to delete rows */);

    updateStatusSelectionSum();
}



static void onSummaryJobFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;

    kl::JsonNode params;
    params.fromString(job->getParameters());

    std::wstring output_path = params["output"];

    if (!g_app->getAppController()->openTable(output_path))
    {
        appMessageBox(_("An output set could not be created."),
                        APPLICATION_NAME,
                         wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }

    
}


void TableDoc::onSummary(wxCommandEvent& evt)
{
    std::vector<std::wstring> summary_columns;

    // find out which columns are selected
    int i, col_count = m_grid->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
            summary_columns.push_back(towstr(m_grid->getColumnCaption(i)));
    }

    // if there was no selection, summarize all columns
    if (summary_columns.size() == 0)
    {
        xd::IStructurePtr structure = g_app->getDatabase()->describeTable(m_path);
        if (structure.isNull())
            return;

        size_t i, col_count = structure->getColumnCount();
        for (i = 0; i < col_count; ++i)
            summary_columns.push_back(structure->getColumnName(i));
    }


    // set up the job from the info we gathered
    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.summarize-job");

    kl::JsonNode params;
    params["input"].setString(m_path);
    params["output"].setString(xd::getTemporaryPath());
    params["columns"].setArray();
    params["where"].setString(getFilter());

    std::vector<std::wstring>::iterator it, it_end;
    it_end = summary_columns.end();

    for (it = summary_columns.begin(); it != it_end; ++it)
    {
        kl::JsonNode column_node = params["columns"].appendElement();
        column_node.setString(*it);
    }

    wxString title = wxString::Format(_("Summarizing '%s'"),
                                      getCaption().c_str());

    job->getJobInfo()->setTitle(towstr(title));
    job->setParameters(params.toString());

    job->sigJobFinished().connect(&onSummaryJobFinished);
    g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
}



// Find Next Row Implementation


class DlgSearchWait : public wxDialog
{
private:
    bool m_cancelled;

public:

    DlgSearchWait(wxWindow* parent) : wxDialog(parent, -1, _("Find"),
                 wxDefaultPosition,
                 wxSize(300, 120),
                  wxDEFAULT_DIALOG_STYLE |
                  wxNO_FULL_REPAINT_ON_RESIZE |
                  wxCLIP_CHILDREN |
                  wxCENTER)
    {
        m_cancelled = false;

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        wxStaticBitmap* bitmap = new wxStaticBitmap(this, -1, GETBMP(gf_find_16));
        wxStaticText* text = new wxStaticText(this, -1, _("Please wait while the data is being searched..."));
        resizeStaticText(text);

        wxButton* cancel_button;
        cancel_button = new wxButton(this, wxID_CANCEL, _("Cancel"));

        wxBoxSizer* text_sizer = new wxBoxSizer(wxHORIZONTAL);
        text_sizer->Add(bitmap, 0, wxALIGN_CENTER | wxRIGHT, 7);
        text_sizer->Add(text, 0, wxEXPAND);

        wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
        button_sizer->Add(1,1, 1, wxEXPAND);
        button_sizer->Add(cancel_button, 0);

        sizer->Add(7,7, 0, wxEXPAND);
        sizer->Add(text_sizer, 0, wxEXPAND | wxALL, 7);
        sizer->Add(1,1, 1, wxEXPAND);
        sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 7);

        CenterOnParent();
        
        SetSizer(sizer);
        Layout();
    }

    ~DlgSearchWait()
    {
    }

    void onClose(wxCloseEvent& event)
    {
        m_cancelled = true;
    }

    void onCancel(wxCommandEvent& event)
    {
        m_cancelled = true;
    }

    bool getCancelled()
    {
        return m_cancelled;
    }

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(DlgSearchWait, wxDialog)
    EVT_CLOSE(DlgSearchWait::onClose)
    EVT_BUTTON(wxID_CANCEL, DlgSearchWait::onCancel)
END_EVENT_TABLE()





wxString TableDoc::getFindExprFromValue(const wxString& _search,
                                        bool match_case,
                                        bool whole_cell)
{
    wxString search = _search.c_str();


    if (m_iter.isNull() || m_grid == NULL)
    {
        return wxEmptyString;
    }

    kcl::IModelPtr model = m_grid->getModel();
    if (model.isNull())
    {
        return wxEmptyString;
    }


    // if the search criterion is an expression, just return it

    xd::objhandle_t handle = m_iter->getHandle(towstr(search));
    if (handle != 0 && m_iter->getType(handle) == xd::typeBoolean)
    {
        return search;
    }


    // otherwise, create an expression which will search for
    // the search string with the appropriate options.  We
    // will first check for any selected columns that will
    // limit the scope of our search

    xd::IStructurePtr iter_struct = m_iter->getStructure();


    std::vector<xd::IColumnInfoPtr> search_cols;

    xd::IColumnInfoPtr colinfo;
    int col_count = m_grid->getColumnCount();
    int i;

    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
        {
            int model_idx = m_grid->getColumnModelIdx(i);
            if (model_idx == -1)
                continue;

            wxString col_name = model->getColumnInfo(model_idx)->getName();

            colinfo = iter_struct->getColumnInfo(towstr(col_name));
            if (colinfo.isNull())
                continue;

            search_cols.push_back(colinfo);
        }
    }

    if (search_cols.size() == 0)
    {
        // no columns selected, therefore search all columns

        col_count = iter_struct->getColumnCount();
        for (i = 0; i < col_count; ++i)
        {
            colinfo = iter_struct->getColumnInfoByIdx(i);
            search_cols.push_back(colinfo);
        }
    }



    bool is_date = false;
    int y, m, d, hh, mm, ss;
    is_date = Locale::parseDateTime(search,
                                         &y, &m, &d,
                                         &hh, &mm, &ss);

    if (hh == -1)
    {
        hh = 0;
        mm = 0;
        ss = 0;
    }

    // build the search expression

    wxString expr;

    std::vector<xd::IColumnInfoPtr>::iterator it;
    for (it = search_cols.begin(); it != search_cols.end(); ++it)
    {
        wxString piece, left, right;
        wxString colname = xd::quoteIdentifierIfNecessary(g_app->getDatabase(), (*it)->getName());        
        
        switch ((*it)->getType())
        {
            default:
            case xd::typeBoolean:
                continue;

            case xd::typeCharacter:
            case xd::typeWideCharacter:
            {
                if (match_case)
                    left = colname;
                      else
                    {
                        wxString temps;
                        temps.Printf(wxT("UPPER(%s)"), colname.c_str());
                        left = temps;
                    }
            }
            break;

            case xd::typeNumeric:
            case xd::typeDouble:
            case xd::typeInteger:
                left = colname;
                break;

            // for dates, if the user is searching for a date with a
            // string we can parse as a date, put the date into a common 
            // format, allowing the user to use different date formats
            // when searching; if we can't parse it as a date, search
            // on the actual text they entered
            case xd::typeDate:
            {
                if (!is_date)
                    left = colname;
                      else
                    {
                        left = colname;
                        right.Printf(wxT("%04d-%02d-%02d"),
                                     y, m, d, hh, mm, ss);
                    }
            }
            break;
    
            case xd::typeDateTime:
            {
                if (!is_date)
                    left = colname;
                      else
                    {
                        left = colname;
                        right.Printf(wxT("%04d-%02d-%02d %02d:%02d:%02d"),
                                     y, m, d, hh, mm, ss);
                    }
            }
            break;
        }

        if (right.IsEmpty())
            right = search;

        if (!match_case)
            right = right.Upper();

        if (whole_cell)
        {
            right = wxT("\'") + doubleQuote(right, L'\'') + wxT("\'");
            piece.Printf(wxT("%s = %s"), left.c_str(), right.c_str());
        }
         else
        {
            right = wxT("\'%") + doubleQuote(right, L'\'') + wxT("%\'");
            piece.Printf(wxT("%s LIKE %s"), left.c_str(), right.c_str());
        }

        if (!expr.IsEmpty())
            expr += wxT(") OR (");

        expr += piece;
    }

    // if we have more than one column, add the opening
    // and closing paranthesis we need to complete the
    // ones we added with the first and last OR statement
    if (search_cols.size() > 1)
        expr = wxT("(") + expr + wxT(")");
    
    return expr;
}

void TableDoc::gotoRecord()
{
    if ((m_iter->getIteratorFlags() & xd::ifFastRowCount) == 0)
        return;

    if (m_grid->getRowCount() > 0)
    {
        long long row_count = (long long)m_stat_row_count;
        
        wxString message = wxString::Format(_("Record number (1 - %s):"),
                kl::formattedNumber(row_count).c_str());
        wxTextEntryDialog dlg(this, message, _("Go To Record"),
                kl::formattedNumber(m_grid->getCursorRow()+1));
        dlg.SetSize(270,143);
        
        if (dlg.ShowModal() == wxID_OK)
        {
            long long goto_row = wxAtof(dlg.GetValue());
            if (goto_row < 1)
                goto_row = 1;
             else if (goto_row > row_count)
                goto_row = row_count;
            
            goto_row -= 1; // rows are 0-based
                                
            m_grid->moveCursor(goto_row,
                               m_grid->getCursorColumn(),
                               false);

            m_grid->scrollVertToCursor();
            m_grid->refresh(kcl::Grid::refreshAll);
        }
    }
}

bool TableDoc::findNextCell(const wxString& search,
                            int start_column,
                            bool forward,
                            bool match_case,
                            bool whole_cell)
{
    int start_col;
    int end_col;
    int col_count = m_grid->getColumnCount();

    if (forward)
    {
        if (start_column == -1)
            start_col = -1;
             else
            start_col = start_column;
        end_col = col_count - 1;
    }
     else
    {
        if (start_column == -1)
            start_col = col_count+1;
             else
            start_col = start_column;
        end_col = 0;
    }


    int col = start_col;
    int row = m_grid->getCursorRow();
    bool found = false;
    

    if (row < 0 || row >= m_grid->getRowCount())
        return false;


    wxString search_upper = search;
    search_upper.MakeUpper();


    bool is_date = false;
    int y, m, d, hh, mm, ss;
    is_date = Locale::parseDateTime(search,
                                         &y, &m, &d,
                                         &hh, &mm, &ss);
    if (hh == -1)
    {
        hh = 0;
        mm = 0;
        ss = 0;
    }



    kcl::IModelPtr model = m_grid->getModel();
    if (model.isNull())
        return false;


    bool columns_selected = false;


    for (col = 0; col < col_count; ++col)
    {
        if (m_grid->isColumnSelected(col))
        {
            columns_selected = true;
            break;
        }
    }

    col = start_col;

    while (1)
    {
        wxString value;

        if (forward)
        {
            ++col;
            if (col > end_col)
                return false;
        }
         else
        {
            --col;
            if (col < end_col)
                return false;
        }

        int model_col = m_grid->getColumnModelIdx(col);
        if (model_col == -1)
            continue;

        if (columns_selected && !m_grid->isColumnSelected(col))
            continue;

        value = m_grid->getCellString(row, model_col);


        if (is_date)
        {
            // see if the cell is a date, too
            kcl::IModelColumnPtr col;
            col = model->getColumnInfo(model_col);
            if (col.isOk())
            {
                if (col->getType() == kcl::Grid::typeDate ||
                    col->getType() == kcl::Grid::typeDateTime)
                {
                    int cy, cm, cd, chh, cmm, css;
                    bool b = Locale::parseDateTime(value,
                                                        &cy, &cm, &cd,
                                                        &chh, &cmm, &css);
                    if (chh == -1)
                    {
                        chh = 0;
                        cmm = 0;
                        css = 0;
                    }

                    if (b)
                    {
                        if (cy == y && cm == m && cd == d &&
                            chh == hh && cmm == mm && css == ss)
                        {
                            found = true;
                            break;
                        }
                    }
                }
            }
        }



        if (whole_cell)
        {
            if (match_case)
            {
                if (search == value)
                {
                    found = true;
                }
            }
             else
            {
                if (0 == search.CmpNoCase(value))
                {
                    found = true;
                }
            }
        }
         else
        {
            if (match_case)
            {
                if (-1 != value.Find(search))
                {
                    found = true;
                }
            }
             else
            {
                value.MakeUpper();
                if (-1 != value.Find(search_upper))
                {
                    found = true;
                }
            }
        }

        if (found)
            break;
    }


    if (found)
    {
        m_grid->moveCursor(row, col, false);
        m_grid->scrollHorzToCursor();
        m_grid->refresh(kcl::Grid::refreshAll);
        return true;
    }


    return false;
}

void TableDoc::getReportCreateInfo(ReportCreateInfo& data)
{
    // set the font facename and size being used by the grid
    data.path = m_path;
    data.font_facename = m_font.GetFaceName();
    data.font_size = m_font.GetPointSize();

    // set the data fields
    int i, col_count = m_grid->getColumnCount();
    kcl::IModelPtr model = m_grid_model;

    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
        {
            int model_idx = m_grid->getColumnModelIdx(i);
            if (model_idx == -1)
                continue;

            ReportCreateField field;
            field.field_name = model->getColumnInfo(model_idx)->getName();
            field.caption = makeProperIfNecessary(field.field_name);
            field.column_width = m_grid->getColumnSize(i)*(kcanvas::CANVAS_MODEL_DPI/kcanvas::CANVAS_SCREEN_DPI);
            data.content_fields.push_back(field);
        }
    }
    
    // set the sort and filter
    data.sort_expr = getSortOrder();
    data.filter_expr = getFilter();
}

bool TableDoc::print(const wxString& caption)
{
    return false;
}

bool TableDoc::saveAsPdf(const wxString& path)
{
    return false;
}

static std::wstring xdTypeToOutputType(int type)
{
    switch (type)
    {
        default:
        case xd::typeUndefined:     return L"undefined";
        case xd::typeInvalid:       return L"invalid";
        case xd::typeCharacter:     return L"character";
        case xd::typeWideCharacter: return L"widecharacter";
        case xd::typeNumeric:       return L"numeric";
        case xd::typeDouble:        return L"double";
        case xd::typeInteger:       return L"integer";
        case xd::typeDate:          return L"date";
        case xd::typeDateTime:      return L"datetime";
        case xd::typeBoolean:       return L"boolean";
        case xd::typeBinary:        return L"binary";
    }
}

bool TableDoc::saveAsStructure(const wxString& path)
{
    xd::IStructurePtr structure = g_app->getDatabase()->describeTable(m_path);
    if (structure.isNull())
        return false;

    // build up a string that we'll save
    std::wstring result_text = L"";

    int col_count = structure->getColumnCount();
    xd::IColumnInfoPtr colinfo;


    // TODO: we could use the kl::JsonNode library here instead of hand-generating the JSON
    
    result_text += L"{\n";
    result_text += L"    \"fields\":";
    result_text += L"\n    [\n";

    bool first = true;
    int i;
    for (i = 0; i < col_count; ++i)
    {
        colinfo = structure->getColumnInfoByIdx(i);    
    
        if (!first)
            result_text += L",\n";
        first = false;
 
        // initial tab space (use spaces instead of tab)
        std::wstring name = colinfo->getName();
        std::wstring type = xdTypeToOutputType(colinfo->getType());

        wchar_t buf[30];
        swprintf(buf, 30, L"%d", colinfo->getWidth());
        std::wstring width(buf);
        swprintf(buf, 30, L"%d", colinfo->getScale());
        std::wstring scale(buf);

        result_text += L"        ";
        result_text += L"{ ";
        result_text += (L"\"name\": \"" + name + L"\"");
        result_text += (L", \"type\": \"" + type + L"\"");
        result_text += (L", \"width\": " + width);
        result_text += (L", \"scale\": " + scale);
        
        if (colinfo->getCalculated())
            result_text += (L", \"formula\": \"" + kl::escape_string(colinfo->getExpression()) + L"\"");
            true;
        
        result_text += L" }";
    }
    
    result_text += L"\n    ]";
    result_text += L"\n}\n";


    // save the text
    std::wstring val = result_text;
    bool uses_unicode = false;

    const wchar_t* p = val.c_str();
    while (*p)
    {
        if (*p > 127)
        {
            uses_unicode = true;
            break;
        }    
        ++p;
    }
    
    size_t buf_len = (val.size() * 4) + 3;
    unsigned char* buf = new unsigned char[buf_len];
    
    if (uses_unicode)
    {
        // convert to utf8
        kl::utf8_wtoutf8((char*)buf+3, buf_len-3, val.c_str(), val.length(), &buf_len);
        
        if (buf_len > 0 && (buf+3)[buf_len-1] == 0)
            buf_len--;
            
        // add space for the byte order mark
        buf_len += 3;
        
        // add byte order mark
        buf[0] = 0xef;
        buf[1] = 0xbb;
        buf[2] = 0xbf;
    }
    /* else ( if we want to save in ucs2 le )
    {
        // little endian byte order mark
        buf[0] = 0xff;
        buf[1] = 0xfe;
        kl::wstring2ucsle(buf+2, val, val.length());
        buf_len = (val.length() * 2) + 2;
    }*/
     else
    {
        // just save as 7-bit ascii because we don't use
        // any characters > char code 127
        std::string s = kl::tostring(val);
        buf_len = val.length();
        memcpy(buf, s.c_str(), buf_len);
    }

    // file is not in project, try disk filesystem
    xf_file_t f = xf_open(towstr(path), xfCreate, xfWrite, xfShareNone);
    if (!f)
    {
        delete[] buf;
        return false;
    }

    xf_write(f, buf, 1, buf_len);
    xf_close(f);


    g_app->getAppController()->openScript(path);

    return false;
}

bool TableDoc::findNextMatch(const wxString& _expr,
                             bool forward,
                             bool match_case,
                             bool whole_cell)
{
    // make sure the eof is known
    IXdGridModelPtr xd_grid_model = m_grid_model;
    if (!xd_grid_model->isEofKnown())
    {
        xd_grid_model->discoverEof();
        m_grid->refresh(kcl::Grid::refreshAll);
    }



    wxString expr = getFindExprFromValue(_expr, match_case, whole_cell);

    // first check the current row
    if (findNextCell(_expr, m_grid->getCursorColumn(),
                     forward, match_case, whole_cell))
    {
        return true;
    }

    // if no more hits were contained in the current row, then
    // look for the next row with hits

    if (m_iter.isNull() || expr.IsEmpty())
        return false;

    // move model to row
    int start_row = m_grid->getCursorRow();
    int cursor_offset = start_row - m_grid->getRowOffset();
    if (cursor_offset < 0 || cursor_offset >= m_grid->getVisibleRowCount())
        cursor_offset = 0;


    kcl::IModelPtr model = m_grid->getModel();
    model->isRowValid(start_row);

    xd::IIteratorPtr sp_iter = m_iter->clone();
    if (sp_iter.isNull())
    {
        return false;
    }

    xd::IIterator* iter = sp_iter.p;

    if (iter->eof())
    {
        return false;
    }

    xd::objhandle_t handle = iter->getHandle(towstr(expr));

    if (handle == 0 || iter->getType(handle) != xd::typeBoolean)
    {
        return false;
    }


    bool found = false;
    bool cancelled = false;
    
    int skip_count = 0;
    int counter = 0;

    AppBusyCursor bc;
    DlgSearchWait* search_dlg = NULL;

    while (1)
    {
        if (forward)
        {
            iter->skip(1);
            skip_count++;
            if (iter->eof())
            {
                break;
            }
        }
         else
        {
            iter->skip(-1);
            skip_count--;
            if (iter->bof())
            {
                break;
            }
        }

        if (iter->getBoolean(handle))
        {
            found = true;
            break;
        }

        counter++;
        if (counter % 100 == 0)
        {
            ::wxSafeYield(search_dlg);
            if (search_dlg)
            {
                if (search_dlg->getCancelled())
                {
                    cancelled = true;
                    break;
                }
            }
        }

        if (counter == 5000)
        {
            search_dlg = new DlgSearchWait(g_app->getMainWindow());
            search_dlg->Show();
        }
    }

    iter->releaseHandle(handle);

    if (search_dlg)
    {
        search_dlg->Show(false);
        search_dlg->Destroy();
    }
    
    if (cancelled)
    {
        return true;
    }

    // move grid
    if (found)
    {
        int new_offset = start_row+skip_count-cursor_offset;
        if (new_offset < 0)
            new_offset = 0;

        m_grid->moveCursor(start_row+skip_count,
                           m_grid->getCursorColumn(),
                           false);

        m_grid->scrollVertToCursor();

        if (!findNextCell(_expr, -1, forward, match_case, whole_cell))
        {
            m_grid->refresh(kcl::Grid::refreshAll);
        }
    }

    if (!found)
    {
        if (!forward)
        {
            appMessageBox(_("The beginning of the data has been reached."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
        }
         else
        {
            appMessageBox(_("The end of the data has been reached."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
        }
    
    }


    return found;
}

bool TableDoc::findReplaceWith(const wxString& find_val,
                               const wxString& replace_val,
                               bool forward,
                               bool match_case,
                               bool whole)
{
    if (!findIsReplaceAllowed())
        return false;
    
    // replace is disabled for now
    return false;
}

bool TableDoc::findReplaceAll(const wxString& find_val,
                              const wxString& replace_val,
                              bool match_case,
                              bool whole)
{
    if (!findIsReplaceAllowed())
        return false;
    
    // replace is disabled for now
    return false;
}

bool TableDoc::findIsReplaceAllowed()
{
    // replace is disabled for now
    return false;
}


// -- Set/Remove Order Implementation --

void TableDoc::onSetOrderExprEditFinished(KeyBuilderPanel* builder)
{
    wxString expr = builder->getExpression();

    // close panel site
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("SortPanel"));
    if (site.isOk())
    {
        g_app->getMainFrame()->closeSite(site);
    }


    setSortOrder(towstr(expr));
}


void TableDoc::onSetOrder(wxCommandEvent& evt)
{
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("SortPanel"));
    if (site.isNull())
    {
        xd::IStructurePtr structure = g_app->getDatabase()->describeTable(m_path);
        if (structure.isNull())
            return;

        KeyBuilderDocPanel* panel = new KeyBuilderDocPanel;
        panel->setOKText(_("Run"));
        panel->setOverlayText(_("Select fields from the list on the left and\ndrag them here to define the table sort order"));
        site = m_frame->createSite(panel, sitetypeModeless,
                                   -1, -1, 600, 360);
        site->setMinSize(520,300);
        site->setName(wxT("SortPanel"));
        panel->setStructure(structure);

        site->setCaption(_("Sort"));

        // set the fields in the sort expression builder based on the selected
        // fields; note: we used to set the order based on the current sort
        // order, similar to filter; but then in this panel and the grouping
        // panel, we added the ability to pre-populate the form based on the
        // selected fields (consistent also with copy fields and summarize,
        // which operate on the selected columns) however, this raised the
        // issue in that if fields were selected, the sort panel would show
        // those fields, otherwise, it would show the fields in the current
        // sort order; in use, this dual behavior seemed confusing, so we're
        // populating the list based on the selected columns, since the panel
        // is more about setting a new order than a providing a status report
        // of the current order

        wxString expr = buildSelectedColumnExpression(m_grid);
        panel->setExpression(expr);

        panel->sigOkPressed.connect(this, &TableDoc::onSetOrderExprEditFinished);
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}


void TableDoc::onRemoveOrder(wxCommandEvent& evt)
{
    setSortOrder(L"");
}



void TableDoc::onSetOrderAscending(wxCommandEvent& evt)
{
    wxString col_caption;
    //xd::objhandle_t data_handle;

    // create order string
    wxString expr = buildSelectedColumnExpression(m_grid);

    // if there was no selection, use the cursor column

    if (expr.Length() == 0)
    {
        kcl::IModelPtr model = m_grid->getModel();
        kcl::IModelColumnPtr model_colinfo;
        wxString col_name;
        xd::IStructurePtr structure = m_iter->getStructure();
        int model_idx = m_grid->getColumnModelIdx(m_grid->getCursorColumn());
        if (model_idx == -1)
        {
            return;
        }
        model_colinfo = model->getColumnInfo(model_idx);
        col_name = model_colinfo->getName();

        expr = xd::quoteIdentifierIfNecessary(g_app->getDatabase(), towstr(col_name));
    }


    // --------------------
    g_macro << "";
    g_macro << "// set sort order (ascending)";
    g_macro << wxString::Format(wxT("auto.activeDocument.setSortOrder(\"%s\");"),
                                wxcstr(jsEscapeString(expr, '"')));
    g_macro << "auto.waitForRunningJobs();";
    // --------------------


    setSortOrder(towstr(expr));
}

void TableDoc::onSetOrderDescending(wxCommandEvent& evt)
{
    wxString col_caption;

    // create order string
    wxString expr = buildSelectedColumnExpression(m_grid, true);

    // if there was no selection, use the cursor column

    if (expr.length() == 0)
    {
        kcl::IModelPtr model = m_grid->getModel();
        kcl::IModelColumnPtr model_colinfo;
        wxString col_name;
        xd::IStructurePtr structure = m_iter->getStructure();
        int model_idx = m_grid->getColumnModelIdx(m_grid->getCursorColumn());
        if (model_idx == -1)
            return;
            
        model_colinfo = model->getColumnInfo(model_idx);
        col_name = model_colinfo->getName();

        expr = xd::quoteIdentifierIfNecessary(g_app->getDatabase(), towstr(col_name));
        expr += " DESC";
    }


    // --------------------
    g_macro << "";
    g_macro << "// set sort order (descending)";
    g_macro << wxString::Format(wxT("auto.activeDocument.setSortOrder(\"%s\");"),
                                wxcstr(jsEscapeString(expr, '"')));
    // --------------------

    setSortOrder(towstr(expr));
}


void TableDoc::onSetGroupBreakExpr(wxCommandEvent& evt)
{
    // create break expression string
    wxString expr = buildSelectedColumnExpression(m_grid);

    // if there was no selection, use the cursor column
    if (expr.IsEmpty())
    {
        kcl::IModelPtr model = m_grid->getModel();
        kcl::IModelColumnPtr model_colinfo;
        wxString col_name;
        xd::IStructurePtr structure = m_iter->getStructure();
        int model_idx = m_grid->getColumnModelIdx(m_grid->getCursorColumn());
        if (model_idx == -1)
            return;
        
        model_colinfo = model->getColumnInfo(model_idx);
        expr = model_colinfo->getName();
    }

    // --------------------
    g_macro << "";
    g_macro << "// set group break";
    g_macro << wxString::Format(wxT("auto.activeDocument.setGroupBreak(\"%s\");"),
                                wxcstr(jsEscapeString(expr, '"')));
    // --------------------

    setGroupBreak(towstr(expr));
}


void TableDoc::onRemoveGroupBreakExpr(wxCommandEvent& evt)
{
    // --------------------
    g_macro << "";
    g_macro << "// remove group break";
    g_macro << "auto.activeDocument.setGroupBreak(\"\");";
    // --------------------

    setGroupBreak(wxT(""));
}

void TableDoc::onInsertColumnSeparator(wxCommandEvent& evt)
{
    insertColumnSeparator(m_grid->getCursorColumn());
}

static void onCopyRecordsJobFinished(jobs::IJobPtr job)
{
    if (job->getJobInfo()->getState() != jobs::jobStateFinished)
        return;

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    kl::JsonNode params;
    params.fromString(job->getParameters());

    std::wstring input = params["input"].getString();
    std::wstring output = params["output"].getString();

    // copy tabledoc model
    std::wstring input_id, output_id;
    xd::IFileInfoPtr finfo;

    finfo = db->getFileInfo(job->getExtraValue(L"source_tabledoc_model_path"));
    if (finfo) input_id = finfo->getObjectId();

    finfo = db->getFileInfo(output);
    if (finfo) output_id = finfo->getObjectId();

    if (input_id.length() > 0 && output_id.length() > 0)
        TableDocMgr::copyModel(input_id, output_id);

    // open the result set in a new child window
    g_app->getAppController()->openTable(output);

}

void TableDoc::onCopyRecordsOk(ExprBuilderPanel* expr_panel)
{
    // close panel site
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("CopyPanel"));
    if (site.isOk())
    {
        g_app->getMainFrame()->closeSite(site);
    }

    copyRecords(towstr(expr_panel->getExpression()));
}


void TableDoc::copyRecords(const std::wstring& condition)
{
    if (m_iter.isNull())
        return;

    // flush active view to model, because copy records attempts
    // to copy the tabledoc model objects to the destination table
    // when the copy job finishes
    flushActiveView();


    xd::IIteratorPtr iter = m_iter->clone();
    if (iter.isOk())
    {
        iter->goFirst();

        // iterator supports clone; use that as the basis for the copy job
        // create a query job

        wxString title = wxString::Format(_("Copying Records from '%s'"),
                                          getCaption().c_str());

        // set up the job from the info we gathered
        jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.copy-job");
        job->getJobInfo()->setTitle(towstr(title));

        kl::JsonNode params;

        params["input_iterator"].setString(kl::stdswprintf(L"%p", (const void*)iter.p));
        params["output"].setString(xd::getTemporaryPath());
        params["where"].setString(condition);

        job->setParameters(params.toString());
        job->setRefObject(iter); // job will hold on to this object for object lifetime reasons
        job->setExtraValue(L"source_tabledoc_model_path", getPath());
        job->sigJobFinished().connect(&onCopyRecordsJobFinished);

        g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
    }
     else
    {
        std::wstring final_condition = getFilter();
        if (final_condition.length() == 0)
        {
            final_condition = condition;
        }
         else
        {
            if (condition.length() > 0)
            {
                final_condition = L"(" + final_condition;
                final_condition += L") AND (";
                final_condition += condition;
                final_condition += L")";
            }
        }


        // create a query job
        wxString title = wxString::Format(_("Copying Records from '%s'"),
                                          getCaption().c_str());

        // set up the job from the info we gathered
        jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.copy-job");
        job->getJobInfo()->setTitle(towstr(title));

        kl::JsonNode params;

        params["input"].setString(m_path);
        params["output"].setString(xd::getTemporaryPath());
        params["where"].setString(final_condition);
        params["order"].setString(getSortOrder());

        job->setParameters(params.toString());
        job->setExtraValue(L"source_tabledoc_model_path", getPath());
        job->sigJobFinished().connect(&onCopyRecordsJobFinished);
        g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
    }
}

void TableDoc::onCopyRecords(wxCommandEvent& evt)
{
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("CopyPanel"));
    if (site.isNull())
    {
        {
            AppBusyCursor bc;

            xd::IStructurePtr structure = g_app->getDatabase()->describeTable(m_path);
            if (structure.isNull())
                return;

            ExprBuilderDocPanel* panel = new ExprBuilderDocPanel;
            panel->setValidationEnabled(m_db_type == xd::dbtypeXdnative ? true : false);
            panel->setOKText(_("Run"));
            site = m_frame->createSite(panel,
                                       sitetypeModeless |
                                       siteHidden,
                                       -1, -1, 560, 310);
            site->setMinSize(560,310);
            site->setCaption(makeCaption(_("Copy")));
            site->setName(wxT("CopyPanel"));

            panel->sigOkPressed.connect(this, &TableDoc::onCopyRecordsOk);
            panel->setTypeOnly(xd::typeBoolean);
            panel->setOKText(_("Run"));
            panel->setEmptyOk(true);
            panel->setStructure(structure);
        }

        site->setVisible(true);
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}



// -- Append Rows Implementation --

void TableDoc::onAppendRecords(wxCommandEvent& evt)
{
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("AppendPanel"));
    if (site.isNull())
    {
        MergePanel* panel = new MergePanel;
        panel->setAppend(m_path);
        site = g_app->getMainFrame()->createSite(panel, 
                                                 sitetypeModeless,
                                                 -1, -1, 460, 420);
        site->setMinSize(460,420);
        site->setName(wxT("AppendPanel"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}



// -- Filter Implementation --

void TableDoc::onFilterOk(ExprBuilderPanel* expr_panel)
{
    std::wstring expr = towstr(expr_panel->getExpression());
    kl::trim(expr);

    if (expr.empty())
    {
        // if the expression is empty, remove the filter
        removeFilter();
        return;
    }

    // close panel site
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("FilterPanel"));
    if (site.isOk())
    {
        g_app->getMainFrame()->closeSite(site);
    }


    // --------------------
    g_macro << "";
    g_macro << "// set filter";
    g_macro << wxString::Format(wxT("auto.activeDocument.setFilter(\"%s\");"),
                                wxcstr(jsEscapeString(expr, '"')));
    // --------------------


    setFilter(expr);
}


void TableDoc::onQuickFilter(wxCommandEvent& evt)
{
    if (m_filter.length() > 0)
    {
        removeFilter();
        return;
    }
    
    wxString val = g_app->getAppController()->getStandardToolbar()->
                   getFindCombo()->GetValue();
    
    // there's no value in the find/filter combobox, so toggle
    // the quick filter toolbar item on and we're done
    if (val.IsEmpty())
        return;

    // if we currently have a quick filter job that we
    // just initiated, cancel the job if the quick filter
    // is clicked
    if (m_quick_filter_jobid > 0)
    {
        jobs::IJobPtr job = g_app->getJobQueue()->lookupJob(m_quick_filter_jobid);
        if (job.isOk())
        {
            job->cancel();
            return;
        }
    }

    // initialize the quick filter job id variable to -1, which
    // we'll look for to identify a quick filter job
    m_quick_filter_jobid = quickFilterPending;


    // --------------------
    g_macro << "";
    g_macro << "// set filter";
    g_macro << wxString::Format(wxT("auto.activeDocument.setQuickFilter(\"%s\");"),
                                wxcstr(jsEscapeString(val, '"')));
    // --------------------


    // set the quick filter
    setQuickFilter(towstr(val));

    // running a quick filter needs to add the value
    // to the find/filter combo control dropdown
    FrameworkEvent* cfw_evt = new FrameworkEvent(FRAMEWORK_EVT_APPMAIN_ADD_FIND_COMBO_ITEM);
    cfw_evt->s_param = val;
    g_app->getMainFrame()->postEvent(cfw_evt);
}


void TableDoc::onFilter(wxCommandEvent& evt)
{
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("FilterPanel"));
    if (site.isNull())
    {
        {
            AppBusyCursor bc;

            xd::IStructurePtr structure = g_app->getDatabase()->describeTable(m_path);
            if (structure.isNull())
                return;

            ExprBuilderDocPanel* panel = new ExprBuilderDocPanel;
            panel->setOKText(_("Run"));
            panel->setValidationEnabled(m_db_type == xd::dbtypeXdnative ? true : false);
            site = m_frame->createSite(panel,
                                       sitetypeModeless |
                                       siteHidden,
                                       -1, -1, 560, 310);
            site->setMinSize(560,310);
            site->setCaption(makeCaption(_("Filter")));
            site->setName(wxT("FilterPanel"));

            panel->setStructure(structure);
            panel->setExpression(m_filter);
            panel->setOKText(_("Run"));
            panel->sigOkPressed.connect(this, &TableDoc::onFilterOk);
            panel->setTypeOnly(xd::typeBoolean);
            panel->setEmptyOk(true);
        }

        site->setVisible(true);
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

void TableDoc::onRemoveFilter(wxCommandEvent& evt)
{
    removeFilter();
}


// Delete Records Implementation

void TableDoc::onDeleteRecordsOk(ExprBuilderPanel* expr_panel)
{
    wxString expr = expr_panel->getExpression();
    expr.Trim(TRUE);
    if (expr.IsEmpty())
    {
        return;
    }

    // close panel site
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("DeletePanel"));
    if (site.isOk())
    {
        g_app->getMainFrame()->closeSite(site);
    }

    deleteRecords(towstr(expr));
}



void TableDoc::deleteRecords(const std::wstring& condition)
{


    std::wstring main_delete_sql;
    std::wstring browse_delete_sql;

    if (m_filter.length() > 0)
    {
        main_delete_sql = L"DELETE FROM ";
        main_delete_sql += formatSqlPath(getPath());
        main_delete_sql += L" WHERE ";
        main_delete_sql += L"(";
        main_delete_sql += m_filter;
        main_delete_sql += L") AND (";
        main_delete_sql += condition;
        main_delete_sql += L");";

        if (getDbDriver() == L"xdnative")
        {
            std::wstring browse_path = getBrowsePath();
            if (browse_path.length() > 0)
            {
                browse_delete_sql += L"DELETE FROM ";
                browse_delete_sql += formatSqlPath(browse_path);
                browse_delete_sql += L" WHERE ";
                browse_delete_sql += condition;
            }
        }
    }
     else
    {
        main_delete_sql += L" DELETE FROM ";
        main_delete_sql += formatSqlPath(getPath());
        main_delete_sql += L" WHERE ";
        main_delete_sql += condition;
    }


    std::wstring title = towstr(wxString::Format(_("Deleting records in '%s'"), getCaption().c_str()));

    if (browse_delete_sql.length() == 0)
    {
        jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.execute-job");
        job->getJobInfo()->setTitle(title);

        kl::JsonNode params;
        params["command"].setString(main_delete_sql);
        job->setParameters(params.toString());

        job->sigJobFinished().connect(this, &TableDoc::onDeleteJobFinished);
        g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
    }
     else
    {
        std::vector<jobs::IJobPtr> jobs;

        jobs::IJobPtr job1 = appCreateJob(L"application/vnd.kx.execute-job");
        kl::JsonNode params1;
        params1["command"].setString(main_delete_sql);
        job1->setParameters(params1.toString());
        jobs.push_back(job1);

        jobs::IJobPtr job2 = appCreateJob(L"application/vnd.kx.execute-job");
        kl::JsonNode params2;
        params2["command"].setString(browse_delete_sql);
        job2->setParameters(params2.toString());
        jobs.push_back(job2);


        jobs::IJobPtr aggregate_job = jobs::createAggregateJob(jobs);
        aggregate_job->getJobInfo()->setTitle(title);
        aggregate_job->setDatabase(g_app->getDatabase());
        aggregate_job->sigJobFinished().connect(this, &TableDoc::onDeleteJobFinished);
        g_app->getJobQueue()->addJob(aggregate_job, jobs::jobStateRunning);
    }
}


void TableDoc::onDeleteRecords(wxCommandEvent& evt)
{
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("DeletePanel"));
    if (site.isNull())
    {
        {
            AppBusyCursor bc;

            xd::IStructurePtr structure = g_app->getDatabase()->describeTable(m_path);
            if (structure.isNull())
                return;

            ExprBuilderDocPanel* panel = new ExprBuilderDocPanel;
            panel->setOKText(_("Run"));
            panel->setValidationEnabled(m_db_type == xd::dbtypeXdnative ? true : false);
            site = m_frame->createSite(panel,
                                       sitetypeModeless |
                                       siteHidden,
                                       -1, -1, 560, 310);
            site->setMinSize(560,310);
            site->setCaption(makeCaption(_("Delete")));
            site->setName(wxT("DeletePanel"));

            panel->setStructure(structure);
            panel->sigOkPressed.connect(this, &TableDoc::onDeleteRecordsOk);
            panel->setOKText(_("Run"));
            panel->setTypeOnly(xd::typeBoolean);
            panel->setEmptyOk(false);
        }

        site->setVisible(true);
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}



void TableDoc::showReplacePanel(const wxString& def_condition, const wxString& def_field)
{
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("UpdatePanel"));
    if (site.isNull())
    {
        {
            AppBusyCursor bc;

            ReplaceRowsPanel* panel = new ReplaceRowsPanel;
            panel->setParameters(getBrowsePath(), def_condition, def_field);

            site = m_frame->createSite(panel,
                                       sitetypeModeless |
                                       siteHidden,
                                       -1, -1, 560, 340);
            site->setMinSize(560,340);
            site->setName(wxT("UpdatePanel"));
        }

        site->setVisible(true);
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}



void TableDoc::createOrShowStructureDoc()
{
    // try to do a lookup of an existing StructureDoc
    IStructureDocPtr structuredoc;
    structuredoc = lookupOtherDocument(m_doc_site, "appmain.StructureDoc");
    if (structuredoc.isOk())
    {
        // structuredoc contains the same set
        // as the tabledoc, we're good to go
        if (isSamePath(m_path, structuredoc->getPath()))
        {
            switchToOtherDocument(m_doc_site, "appmain.StructureDoc");
            return;
        }
         else
        {
            // NOTE: This should NEVER happen since we remove the existing
            //       StructureDoc site when we do a "Save As..."
            
            // structuredoc contains a different set that the tabledoc,
            // remove the existing structuredoc and fall through to the
            // code below to create a new one
            IDocumentSitePtr site;
            site = lookupOtherDocumentSite(m_doc_site, "appmain.StructureDoc");
            if (site.isOk())
                g_app->getMainFrame()->closeSite(site);
        }
    }

  
    {
        AppBusyCursor bc;

        StructureDoc* doc = new StructureDoc;
        doc->setModifySet(m_path);
        
        wxWindow* container = m_doc_site->getContainerWindow();

        IDocumentSitePtr site;
        site = g_app->getMainFrame()->createSite(container,
                                    static_cast<IDocument*>(doc),
                                    true);
        site->setVisible(true);
    }
}

void TableDoc::onUpdateRecords(wxCommandEvent& evt)
{
    wxString cursor_column = getCursorColumnName(m_grid);
    showReplacePanel(wxT(""), cursor_column);
}




void TableDoc::onViewEditFinished(ViewPanel* panel)
{
    panel->updateTableDocViewEnum(this);
}

void TableDoc::showViewPanel()
{
    // flush the active view, if any changes were made
    flushActiveView();
    
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("ColumnViewsPanel"));

    if (site.isNull())
    {
        ViewPanel* panel = new ViewPanel(this);
        panel->sigOkPressed.connect(this, &TableDoc::onViewEditFinished);

        site = g_app->getMainFrame()->createSite(panel,
                                                 sitetypeModeless,
                                                 -1, -1, 600, 400);
        site->setMinSize(600,400);
        site->setName(wxT("ColumnViewsPanel"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

static bool isKeyExpressionSame(const std::wstring& expr1, const std::wstring& expr2)
{
    std::vector<std::wstring> parts1;
    std::vector<std::wstring> parts2;

    kl::parseDelimitedList(expr1, parts1, ',', true);
    kl::parseDelimitedList(expr2, parts2, ',', true);

    if (parts1.size() != parts2.size())
        return false;

    size_t i, cnt = parts1.size();

    for (i = 0; i < cnt; ++i)
    {
        xd::dequoteIdentifier(parts1[i], '[', ']');
        xd::dequoteIdentifier(parts1[i], '"', '"');
        xd::dequoteIdentifier(parts2[i], '[', ']');
        xd::dequoteIdentifier(parts2[i], '"', '"');

        if (0 != wcscasecmp(parts1[i].c_str(), parts2[i].c_str()))
            return false;
    }

    return true;
}

void TableDoc::onIndexEditFinished(IndexPanel* panel)
{
    xd::IDatabasePtr db = g_app->getDatabase();

    // get the original indexes that exist on this set
    xd::IIndexInfoEnumPtr orig_indexes = db->getIndexEnum(m_path);
    bool found;
    
    // get all of the indexes that were created/updated in the IndexPanel
    std::vector<IndexInfo*> vec = panel->getAllIndexes();
    std::vector<IndexInfo*>::iterator it;
    IndexInfo* info;
    
    // first, delete any of the original indexes that were
    // not found in the list of indexes in the IndexPanel
    int i, count = (int)orig_indexes->size();
    for (i = count-1; i >= 0; --i)
    {
        xd::IIndexInfoPtr index = orig_indexes->getItem(i);
        wxString index_tag = index->getTag();
        found = false;
        
        // try to find the original index
        info = NULL;
        for (it = vec.begin(); it != vec.end(); ++it)
        {
            info = (*it);
            if (info->orig_name.CmpNoCase(index_tag) == 0)
            {
                found = true;
                break;
            }
        }
        
        // delete the index if it's not found
        if (!found)
        {
            db->deleteIndex(m_path, index->getTag());
        }
    }
    
    
    // we may have deleted some of the original indexes;
    // get the list of indexes again
    orig_indexes = db->getIndexEnum(m_path);
    count = (int)orig_indexes->size();
    
    // rename indexes
    for (i = count-1; i >= 0; --i)
    {
        xd::IIndexInfoPtr index = orig_indexes->getItem(i);
        wxString index_tag = index->getTag();
        
        // find the original index and rename if
        // its name was changed in the IndexPanel
        info = NULL;
        for (it = vec.begin(); it != vec.end(); ++it)
        {
            info = (*it);
            
            // don't worry about new indexes here
            if (info->orig_name.IsEmpty())
                continue;
            
            if (info->orig_name.CmpNoCase(index_tag) == 0)
            {
                // the index was renamed in the IndexPanel
                if (info->name.CmpNoCase(info->orig_name) != 0)
                {
                    // have the set rename this index
                    db->renameIndex(m_path,
                                    towstr(index_tag),
                                    towstr(info->name));
                    
                    // we also need to update the IndexInfo structure
                    info->orig_name = info->name;
                    break;
                }
            }
        }
    }
    
    
    // now that we've deleted and renamed indexes, we need to set
    // up an index job to either reindex some of the original indexes
    // (if their expression has changed) or create new indexes (if
    // their tag name can't be found in the original list of indexes)

    // create an index job
    jobs::IJobPtr job = appCreateJob(L"application/vnd.kx.index-job");

    kl::JsonNode params;
    kl::JsonNode indexes = params["indexes"];
    
    
    // we may have deleted some of the original indexes;
    // get the list of indexes again
    orig_indexes = db->getIndexEnum(m_path);
    count = (int)orig_indexes->size();
    bool index_deleted = false;
    
    // now, create new indexes or update existing indexes based
    // on the list of indexes in the IndexPanel
    for (it = vec.begin(); it != vec.end(); ++it)
    {
        info = (*it);
        
        if (info->orig_name.IsEmpty())
        {
            // this is a new index; add it to the IndexJob
            kl::JsonNode index_item = indexes.appendElement();
            index_item["input"].setString(m_path);
            index_item["name"].setString(towstr(info->name));
            index_item["expression"].setString(towstr(info->expr));

            continue;
        }
        
        xd::IIndexInfoPtr index;
        wxString index_tag, index_expr;
        
        // we may have deleted some of the original indexes;
        // get the list of indexes again
        if (index_deleted)
        {
            orig_indexes = db->getIndexEnum(m_path);
            count = (int)orig_indexes->size();
            index_deleted = false;
        }
        
        // try to find the original index
        for (i = 0; i < count; ++i)
        {
            index = orig_indexes->getItem(i);
            index_tag = index->getTag();
            
            if (info->orig_name.CmpNoCase(index_tag) == 0)
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            // we couldn't find the orginal index; create a new index
            kl::JsonNode index_item = indexes.appendElement();
            index_item["input"].setString(m_path);
            index_item["name"].setString(towstr(info->name));
            index_item["expression"].setString(towstr(info->expr));

            continue;
        }
        
        // we found the index; there are a couple of options...
        
        // 1) the expression hasn't changed; do nothing
        if (isKeyExpressionSame(index->getExpression(), towstr(info->expr)))
            continue;

        // 2) the expression has changed; delete the old index
        //    and create a new one
        kl::JsonNode index_item = indexes.appendElement();
        index_item["input"].setString(m_path);
        index_item["name"].setString(towstr(info->name));
        index_item["expression"].setString(towstr(info->expr));        
        
        if (index.isOk())
        {
            db->deleteIndex(m_path, index->getTag());
            index_deleted = true;
        }
    }
    
    // the index job has no parameters; we're done
    if (indexes.getChildCount() == 0)
        return;

    job->getJobInfo()->setTitle(towstr(_("Creating Index")));
    job->setParameters(params.toString());

    g_app->getJobQueue()->addJob(job, jobs::jobStateRunning);
}

void TableDoc::showIndexPanel()
{
    IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("IndexPanel"));
    
    if (site.isNull())
    {
        IndexPanel* panel = new IndexPanel;
        if (!panel->setPath(m_path))
        {
            delete panel;
            return;
        }

        panel->sigOkPressed.connect(this, &TableDoc::onIndexEditFinished);
        
        site = g_app->getMainFrame()->createSite(panel,
                                                 sitetypeModeless,
                                                 -1, -1, 640, 400);
        site->setMinSize(640,400);
        site->setName(wxT("IndexPanel"));
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}

void TableDoc::onEditViews(wxCommandEvent& evt)
{
    showViewPanel();
}

void TableDoc::onEditStructure(wxCommandEvent& evt)
{
    createOrShowStructureDoc();
}

void TableDoc::onEditIndexes(wxCommandEvent& evt)
{
    showIndexPanel();
}

void TableDoc::onSelectColumn(wxCommandEvent& evt)
{
    // flag to determine if the "Delete Field" menu item is enabled
    m_allow_delete_menuid = isDeleteAllowed(this, false);

    m_grid->clearSelection();
    m_grid->setColumnSelected(m_grid->getCursorColumn(), true);
    m_grid->refresh(kcl::Grid::refreshAll);
}

void TableDoc::onSelectRow(wxCommandEvent& evt)
{
    // flag to determine if the "Delete Field" menu item is enabled
    m_allow_delete_menuid = isDeleteAllowed(this, true);

    m_grid->clearSelection();
    m_grid->setRowSelected(m_grid->getCursorRow(), true);
    m_grid->refresh(kcl::Grid::refreshAll);
}

void TableDoc::onSelectAll(wxCommandEvent& evt)
{
    m_grid->selectAll();
}

void TableDoc::onDelete(wxCommandEvent& evt)
{
    deleteSelectedRowsColumns();
}

void TableDoc::onGoTo(wxCommandEvent& evt)
{
    gotoRecord();
}

static void setGroupFields(kcl::Grid* grid, GroupPanel* panel)
{
    // if columns are selected, add them to the group list
    int col_count = grid->getColumnCount();
    for (int i = 0; i < col_count; ++i)
    {
        if (grid->isColumnSelected(i))
        {
            // if we have a column (i.e., not a column break where the
            // caption length is zero), add it to the group fields
            wxString caption = grid->getColumnCaption(i);
            if (panel && caption.Length() > 0)
                panel->addGroupField(caption);
        }
    }
}

void TableDoc::onGroup(wxCommandEvent& evt)
{
    IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("GroupPanel"));
    if (site.isNull())
    {
        GroupPanel* panel = new GroupPanel;
        panel->setParameters(m_path, getFilter());

        site = g_app->getMainFrame()->createSite(panel,
                                                 sitetypeModeless,
                                                 -1, -1, 600, 360);

        site->setMinSize(600,360);
        site->setName(wxT("GroupPanel"));
        setGroupFields(m_grid, panel);
    }
     else
    {
        if (!site->getVisible())
        {
            site->setVisible(true);
        }
    }
}


void TableDoc::onSetBreakExpr(wxCommandEvent& evt)
{
    IXdGridModelPtr model = m_grid->getModel();
    if (model.isNull())
        return;

    xd::IStructurePtr structure = g_app->getDatabase()->describeTable(m_path);
    if (structure.isNull())
        return;

    DlgKeyBuilder dlg(this);

    dlg.setStructure(structure);
    dlg.setExpression(model->getGroupBreakExpr().c_str());
    dlg.setEmptyOk(true);

    if (dlg.ShowModal() == wxID_OK)
    {
        model->setGroupBreakExpr(dlg.getExpression());
        m_grid->refresh(kcl::Grid::refreshAll);
    }
}


void TableDoc::onRequestRowColors(wxColor& fgcolor, wxColor& bgcolor)
{
    if (m_iter.isNull())
    {
        fgcolor = wxNullColour;
        bgcolor = wxNullColour;
        return;
    }
    
    wxColor fg, bg;

    ITableDocMarkPtr mark;
    ITableDocMarkEnumPtr markvec = m_model->getMarkEnum();
    size_t i, mark_count = markvec->size();
    int hit_count = 0;

    xd::objhandle_t handle;

    for (i = 0; i < mark_count; ++i)
    {
        mark = markvec->getItem(i);

        if (mark->getMarkActive())
        {
            handle = getTemporaryHandle(mark->getExpression());

            if (handle &&
                m_iter->getType(handle) == xd::typeBoolean &&
                m_iter->getBoolean(handle))
            {
                hit_count++;
                fg = mark->getForegroundColor();
                bg = mark->getBackgroundColor();
            }
        }
    }

    if (hit_count == 0)
    {
        fgcolor = wxNullColour;
        bgcolor = wxNullColour;
        
        if (m_relsync_mark_expr.length() > 0)
        {
            // mark for relationship syncing with context rows
            handle = getTemporaryHandle(m_relsync_mark_expr);

            if (handle &&
                m_iter->getType(handle) == xd::typeBoolean &&
                m_iter->getBoolean(handle))
            {
                fgcolor = *wxBLACK;
                bgcolor = wxColor(255,255,178); // matches default mark
            }
        }
    }
     else if (hit_count == 1)
    {
        fgcolor = fg;
        bgcolor = bg;
    }
     else if (hit_count > 1)
    {
        // multiple hits, set to blue on grey (for now)
        fgcolor.Set(0, 0, 255);
        bgcolor.Set(219, 219, 219);
    }
}

void TableDoc::initializeDefaultView(ITableDocViewPtr view,
                                     xd::IStructurePtr v_struct)
{
    view->deleteAllColumns();

    if (v_struct)
    {
        xd::IColumnInfoPtr colinfo;
        ITableDocViewColPtr viewcol;

        int col_count;
        col_count = v_struct->getColumnCount();
        for (int i = 0; i < col_count; i++)
        {
            colinfo = v_struct->getColumnInfoByIdx(i);
            
            viewcol = view->createColumn(-1);
            viewcol->setName(colinfo->getName());
            viewcol->setSize(80);
        }
    }
}

void TableDoc::flushActiveView()
{
    if (m_active_view.isOk() && m_model.isOk())
        m_model->writeObject(m_active_view);
}

void TableDoc::setActiveView(ITableDocViewPtr active_view)
{
    if (m_grid && m_grid->isEditing())
        m_grid->endEdit(true);

    // find out if any other window is displaying this view object.
    // If so, use their local object instead

    IDocumentSiteEnumPtr docsites;
    docsites = g_app->getMainFrame()->getDocumentSites(sitetypeNormal);

    int site_count = docsites->size();
    for (int i = 0; i < site_count; ++i)
    {
        IDocumentSitePtr site;
        ITableDocPtr table_doc;

        site = docsites->getItem(i);
        table_doc = site->getDocument();

        if (table_doc.p == (ITableDoc*)this)
            continue;

        if (table_doc.isOk())
        {
            if (!isSamePath(m_path, table_doc->getPath()))
                continue;

            ITableDocObjectPtr v1 = table_doc->getActiveView();
            ITableDocObjectPtr v2 = active_view;
            if (v1.isOk() && v2.isOk())
            {
                if (v1->getObjectId() == v2->getObjectId())
                {
                    active_view = v1;
                    break;
                }
            }
        }
    }
    docsites.clear();


    // write out the active view (if dirty)
    flushActiveView();
    m_active_view = active_view;

    // make sure the grid display is not off the end of the view
    if (m_grid)
    {
        int i;
        int col_count = m_active_view->getColumnCount();
        int tot_width = 0;

        for (i = 0; i < col_count; ++i)
        {
            ITableDocViewColPtr col = m_active_view->getColumn(i);
            tot_width += col->getSize();
        }

        if (m_grid->getHorizontalOffset() > tot_width)
            m_grid->setHorizontalOffset(0);
    }

    refreshActiveView();

    // fire a signal that indicates that the view has been set
    if (m_frame)
        m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_TABLEDOC_VIEW_CHANGED));
}

ITableDocViewPtr TableDoc::getActiveView()
{
    return m_active_view;
}

void TableDoc::refreshActiveView(bool repaint)
{
    bool multiple_tabledocs_open = false;

    bool bad_column_check_performed = false;

    ITableDocPtr table_doc;
    IDocumentSitePtr site;
    IDocumentSiteEnumPtr sites;

    sites = g_app->getMainFrame()->getDocumentSites(sitetypeNormal);
    int site_count = sites->size();
    for (int i = 0; i < site_count; ++i)
    {
        site = sites->getItem(i);
        if (site.isNull())
            continue;
        table_doc = site->getDocument();
        if (table_doc.isNull())
            continue;
        if (table_doc.p == static_cast<ITableDoc*>(this))
            continue;

        if (isSamePath(m_path, table_doc->getPath()))
        {
            multiple_tabledocs_open = true;
            break;
        }
    }

    refresh_grid:
    if (m_grid)
    {
        m_grid->hideAllColumns();
        
        if (m_active_view.isNull())
            return;
         
        
        int row_height = m_active_view->getRowSize();
        if (row_height == -1)
        { 
            IAppPreferencesPtr prefs = g_app->getAppPreferences();
            row_height = getAppPrefsLong(wxT("grid.row_height"));
        }
        m_grid->setRowHeight(row_height);
        
        
           
        bool model_refreshed = false;

        ITableDocViewColPtr viewcol;

        int bad_columns = 0;

        int col_count = m_active_view->getColumnCount();
        int model_idx;
        for (int col = 0; col < col_count; col++)
        {
            viewcol = m_active_view->getColumn(col);
            
            wxString col_name = viewcol->getName();
            if (col_name.IsEmpty())
            {
                // there is no column name, which means that this
                // entry is a column separator
                int colpos = m_grid->insertColumnSeparator(-1);
                m_grid->setColumnSize(colpos, viewcol->getSize());
                m_grid->setColumnColors(colpos, viewcol->getForegroundColor(),
                                                viewcol->getBackgroundColor());
                continue;
            }


            model_idx = m_grid->getColumnModelIdxByName(col_name);

            if (model_idx == -1 && !model_refreshed)
            {
                // we couldn't find the column, try refreshing the model
                m_grid->refreshModel();

                model_idx = m_grid->getColumnModelIdxByName(col_name);
                model_refreshed = true;
            }

            if (model_idx >= 0)
            {
                wxString caption = makeProperIfNecessary(col_name);

                int colpos = m_grid->insertColumn(-1, model_idx);
                m_grid->setColumnCaption(colpos, caption);
                m_grid->setColumnSize(colpos, viewcol->getSize());
                m_grid->setColumnColors(colpos,
                                        viewcol->getForegroundColor(),
                                        viewcol->getBackgroundColor());

                int grid_align;
                switch (viewcol->getAlignment())
                {
                    default:
                    case tabledocAlignDefault:
                        grid_align = kcl::Grid::alignDefault;
                        break;
                    case tabledocAlignLeft:
                        grid_align = kcl::Grid::alignLeft;
                        break;
                    case tabledocAlignCenter:
                        grid_align = kcl::Grid::alignCenter;
                        break;
                    case tabledocAlignRight:
                        grid_align = kcl::Grid::alignRight;
                }

                m_grid->setColumnAlignment(colpos, grid_align);
                
                
                int text_wrap = viewcol->getTextWrap();
                if (text_wrap == tabledocWrapDefault)
                    text_wrap = m_text_wrapping;
                    
                if (text_wrap == tabledocWrapOn)
                    m_grid->setColumnTextWrapping(colpos, kcl::Grid::wrapOn);
                     else
                    m_grid->setColumnTextWrapping(colpos, kcl::Grid::wrapOff);
            }
             else
            {
                // view column doesn't exist, so delete it from the view
                if (multiple_tabledocs_open)
                {
                    int idx = m_grid->insertColumnSeparator(-1);
                    m_grid->setColumnSize(idx, viewcol->getSize());
                }
                 else
                {
                    m_active_view->deleteColumn(col);
                    col--;
                    col_count--;
                    bad_columns++;
                }
            }
        }


        if (!bad_column_check_performed && m_iter.isOk())
        {
            if (col_count > 0 && ((bad_columns >= col_count) || (col_count > 2 && bad_columns >= col_count/2)))
            {
                xd::IStructurePtr s = m_iter->getStructure();
                initializeDefaultView(m_active_view, s);
                m_grid->setHorizontalOffset(0);
                m_model->writeObject(m_active_view);
                goto refresh_grid;
            }
            bad_column_check_performed = true;
        }
        
        if (!repaint)
            m_grid->refresh(kcl::Grid::refreshData);
              else
            m_grid->refresh(kcl::Grid::refreshAll);
    }
}

bool TableDoc::isQuickFilterPending()
{
    return (m_quick_filter_jobid > 0 ? true : false);
}


xd::objhandle_t TableDoc::getTemporaryHandle(const wxString& expr)
{
    if (m_iter.isNull())
        return 0;

    std::map<wxString, xd::objhandle_t>::iterator it;
    
    it = m_handle_map.find(expr);
    if (it != m_handle_map.end())
        return it->second;
        

    xd::objhandle_t handle = m_iter->getHandle(towstr(expr));
    if (handle)
    {
        m_handle_map[expr] = handle;
        return handle;
    }
    
    return 0;
}


void TableDoc::freeTemporaryHandles()
{
    if (!m_iter)
        return;
        
    std::map<wxString, xd::objhandle_t>::iterator it;
    for (it = m_handle_map.begin(); it != m_handle_map.end(); ++it)
    {
        if (it->second != 0)
        {
            m_iter->releaseHandle(it->second);
        }
    }

    m_handle_map.clear();
}
