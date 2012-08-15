/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
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
#include "tangogridmodel.h"
#include "jobcopy.h"
#include "jobexport.h"
#include "jobexportpkg.h"
#include "jobquery.h"
#include "jobappend.h"
#include "jobgroup.h"
#include "jobmodifystruct.h"
#include "jobindex.h"
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
#include "toolbars.h"
#include "textdoc.h"
#include "transformationdoc.h"
#include "structuredoc.h"
#include "querydoc.h"
#include "sqldoc.h"
#include "webdoc.h"
#include "../kcl/griddnd.h"
#include <algorithm>
#include <set>


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
    EVT_MENU(ID_Data_MakeDynamicFieldStatic, TableDoc::onMakePermanent)
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


// -- helper functions --

static void setColumnProps(wxColor* fill_color,
                           wxColor* text_color = NULL,
                           int alignment = -1);

static bool colInVector(int col, std::vector<int> cols)
{
    std::vector<int>::iterator it;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        if (col == *it)
            return true;
    }
    
    return false;
}

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
            expr += quoteIdentifier(g_app->getDatabase(), model->getColumnInfo(model_idx)->getName());

            // if the descending flag is set, add the
            // descending keyword
            if (descending)
                expr += wxT(" DESC");
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




// -- TableDoc class implementation --

TableDoc::TableDoc()
{
    m_grid = NULL;
    m_caption = wxT("");

    createModel();

    m_temporary = true;
    m_temporary_model = false;
    m_relationship_sync = g_app->getAppController()->getRelationshipSync();
    m_is_childset = false;
    m_enabled = true;
    m_default_view_created = false;
    m_override_beginedit = false;
    m_text_wrapping = tabledocWrapOff;
    m_stat_row_count = (tango::rowpos_t)-1;
    m_allow_delete_menuid = false;
    m_external_table = -1;
    m_doing_reload = false;
    m_quick_filter_jobid = quickFilterNotPending;
    
    m_db_type = tango::dbtypeXdnative;
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
    TangoGridModel* model = new TangoGridModel;
    model->sigRequestRowColors.connect(this, &TableDoc::onRequestRowColors);

    // set the boolean checkbox preference for the grid
    bool bool_checkbox = g_app->getAppPreferences()->getBoolean(wxT("grid.boolean_checkbox"), true);
    model->setBooleanCheckbox(bool_checkbox);

    m_grid_model = static_cast<ITangoGridModel*>(model);
}

bool TableDoc::isExternalTable()
{
    // m_external_table is a cache variable which allows
    // subsequent calls to isExternalTable() to operate 
    // quickly
    
    if (m_external_table == -1)
    {
        m_external_table = (getDbDriver() != wxT("xdnative") && getDbDriver() != wxT("xdclient")) ? 1 : 0;
    }
    
    return (m_external_table == 1) ? true : false;
}

bool TableDoc::canDeleteColumns(std::vector<int>& view_cols)
{
    // allow column deletion for the native driver
    if (!isExternalTable())
        return true;
    
    // we're checking an external set
    
    tango::ISetPtr set = getBaseSet();
    if (set.isNull())
        return false;
    
    tango::IStructurePtr structure = set->getStructure();
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
        tango::IColumnInfoPtr colinfo;
        colinfo = structure->getColumnInfoByIdx(model_col);
        if (colinfo.isNull() || !colinfo->getCalculated())
            return false;
    }
    
    return true;
}

cfw::IUIContextPtr TableDoc::getUserInterface()
{
    cfw::IUIContextPtr ui_context;

    // see if a user interface of this type already exists
    ui_context = m_frame->lookupUIContext(wxT("TableDocUI"));
    if (ui_context.isOk())
        return ui_context;
    
    // no, so create one
    ui_context = m_frame->createUIContext(wxT("TableDocUI"));
    return ui_context;
}



bool TableDoc::initDoc(cfw::IFramePtr frame,
                       cfw::IDocumentSitePtr doc_site,
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

    // properties
    
    initProperties(frame);
    
    defineProperty(wxT("tabledoc.fieldinfo.name"),
                   cfw::proptypeString,
                   cfw::propName(_("Field Info"), _("Name")));

    wxArrayString type_strings;
    type_strings.Add(_("Undefined"));
    type_strings.Add(_("Invalid"));
    type_strings.Add(_("Character"));
    type_strings.Add(_("Wide Character"));
    type_strings.Add(_("Binary"));
    type_strings.Add(_("Numeric"));
    type_strings.Add(_("Double"));
    type_strings.Add(_("Integer"));
    type_strings.Add(_("Date"));
    type_strings.Add(_("DateTime"));
    type_strings.Add(_("Boolean"));
    
    wxArrayInt type_ints;
    type_ints.Add(tango::typeUndefined);
    type_ints.Add(tango::typeInvalid);
    type_ints.Add(tango::typeCharacter);
    type_ints.Add(tango::typeWideCharacter);
    type_ints.Add(tango::typeBinary);
    type_ints.Add(tango::typeNumeric);
    type_ints.Add(tango::typeDouble);
    type_ints.Add(tango::typeInteger);
    type_ints.Add(tango::typeDate);
    type_ints.Add(tango::typeDateTime);
    type_ints.Add(tango::typeBoolean);
        
    defineProperty(wxT("tabledoc.fieldinfo.type"),
                   cfw::proptypeInteger,
                   cfw::propName(_("Field Info"), _("Type")),
                   &type_strings,
                   &type_ints);

    defineProperty(wxT("tabledoc.fieldinfo.width"),
                   cfw::proptypeInteger,
                   cfw::propName(_("Field Info"), _("Width")));
                   
    defineProperty(wxT("tabledoc.fieldinfo.scale"),
                   cfw::proptypeInteger,
                   cfw::propName(_("Field Info"), _("Decimal Places")));
                   
    defineProperty(wxT("tabledoc.column_fgcolor"),
                   cfw::proptypeColor,
                   cfw::propName(_("Column Foreground")));

    defineProperty(wxT("tabledoc.column_bgcolor"),
                   cfw::proptypeColor,
                   cfw::propName(_("Column Background")));
                   
    defineProperty(wxT("tabledoc.column_pixwidth"),
                   cfw::proptypeInteger,
                   cfw::propName(_("Column Width (pixels)")));

    wxArrayString alignment_strings;
    alignment_strings.Add(_("Default"));
    alignment_strings.Add(_("Left"));
    alignment_strings.Add(_("Center"));
    alignment_strings.Add(_("Right"));
    
    defineProperty(wxT("tabledoc.column_alignment"),
                   cfw::proptypeInteger,
                   cfw::propName(_("Column Alignment")),
                   &alignment_strings);

    // create the statusbar items for this document
    cfw::IStatusBarItemPtr item;
    
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
    cfw::IDocumentPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
    if (textdoc)
        return textdoc->getDocumentLocation();
    
    cfw::IDocumentPtr querydoc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
    if (querydoc)
        return querydoc->getDocumentLocation();
    
    if (m_source_url.Length() > 0)
        return m_source_url;
    
    return m_dbpath;
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
    if (m_temporary && !force)
    {
        // note: temporarily disable warning on closing unsaved data sets; the reason
        // for this is that many more untitled sets are created casually from the
        // web (html/rss/etc) simply by clicking, and everytime these are closed,
        // this warning pops up, constantly interrupting the user; if we want to have
        // this warning back, we could add it if the user begins to do operations on
        // the file, such as creating dynamic fields, or sorting/filtering, etc; this
        // would parallel the script document, which doesn't pop up a warning message
        // for new untitled files unless they begin modifying the document
        
        int result = wxNO;
        /*
        int result;
        result = cfw::appMessageBox(
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
                g_app->getDatabase()->storeObject(m_set, towstr(dlg.getPath()));
                g_app->getAppController()->refreshDbDoc();
                m_temporary = false;
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

    // if we're viewing a text file, save the view information in the OFS
    // so we can resume in this view the next time we open up this text file
    cfw::IDocumentPtr doc;
    doc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
    ITextDocPtr textdoc = doc;
    if (textdoc)
        textdoc->saveViewInfo(L"table");



    // let the column props (dynamic) field panel know that we are closing
    cfw::Event* e = new cfw::Event(wxT("tabledoc.onSiteClosing"), (long)(ITableDoc*)this);
    m_frame->sendEvent(e);


    // save the view
    flushActiveView();

    // release all references to everything
    m_frame.clear();
    m_doc_site.clear();
    m_grid_model.clear();
    m_set.clear();
    m_browse_set.clear();
    m_iter.clear();

    if (m_grid)
    {
        delete m_grid;
        m_grid = NULL;
    }

    if (m_temporary)
    {
        TableDocMgr::deleteModel(m_model->getId());
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

void TableDoc::onStatusBarItemLeftDblClick(cfw::IStatusBarItemPtr item)
{
    // only pop open the "Go To Record" dialog for the active table
    
    cfw::IDocumentSitePtr doc_site = m_frame->getActiveChild();
    if (doc_site.isOk() && doc_site == m_doc_site)
    {
        if (item->getName() == wxT("tabledoc_position") ||
            item->getName() == wxT("tabledoc_record_count"))
        {
            gotoRecord();
        }
    }
}

void TableDoc::onFrameEvent(cfw::Event& evt)
{
    if (evt.name == wxT("tabledoc.doViewReload") &&
        evt.l_param != (unsigned long)this)
    {
        TableDoc* td1 = (TableDoc*)evt.l_param;
        TableDoc* td2 = (TableDoc*)this;

        if (td1->m_set == td2->m_set)
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
              evt.name == wxT("appmain.preferencesSaved"))
    {
        reloadSettings(true);
    }
     else if (evt.name == wxT("tabledoc.structureModified"))
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
     else if (evt.name == wxT("treepanel.ofsFileRenamed"))
    {
        updateCaption();
        
        cfw::IDocumentSitePtr doc_site = m_frame->getActiveChild();
        if (doc_site.isOk() && doc_site == m_doc_site)
        {
            // fire this event so that the URL combobox will be updated
            // with the new path if this is the active child
            m_frame->postEvent(new cfw::Event(wxT("cfw.locationChanged")));
        }
    }
     else if (evt.name == wxT("appmain.relationshipsUpdated"))
    {
        m_grid->refresh(kcl::Grid::refreshAll);
    }
     else if (evt.name == wxT("appmain.view_switcher.query_available_views"))
    {
        cfw::IDocumentSitePtr active_child;
        active_child = g_app->getMainFrame()->getActiveChild();
        
        if (active_child.isNull() || m_doc_site.isNull())
            return;
            
        if (active_child->getContainerWindow() != m_doc_site->getContainerWindow())
            return;

        // site ptrs to check the active site
        cfw::IDocumentSitePtr textdoc_site;
        cfw::IDocumentSitePtr transdoc_site;
        cfw::IDocumentSitePtr structdoc_site;
        cfw::IDocumentSitePtr querydoc_site;
        cfw::IDocumentSitePtr sqldoc_site;
        cfw::IDocumentSitePtr active_site;
        active_site = g_app->getMainFrame()->getActiveChild();
        
        // populate the view switcher list with the available views
        ViewSwitcherList* list = (ViewSwitcherList*)(evt.o_param);
        
        ITextDocPtr textdoc = lookupOtherDocument(m_doc_site, "appmain.TextDoc");
        cfw::IDocumentPtr querydoc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
        
        
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
     else if (evt.name == wxT("appmain.view_switcher.active_view_changed"))
    {
        int id = (int)(evt.l_param);
        
        // -- make sure we are in the active container --
        cfw::IDocumentSitePtr active_site;
        active_site = g_app->getMainFrame()->getActiveChild();
        if (active_site.isNull() || m_doc_site.isNull())
            return;
        if (active_site->getContainerWindow() != m_doc_site->getContainerWindow())
            return;
        
        
        if (id == ID_View_SwitchToLayoutView)
        {
            // if we are on structure doc, we might need to prompt for saving
            cfw::IDocumentSitePtr structdoc_site;

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

void TableDoc::onPropertyChanged(const wxString& prop_name)
{
    cfw::PropertyValue data;
    if (!getProperty(prop_name, data))
        return;

    wxColor data_color = data.m_color;
    
    if (prop_name == wxT("tabledoc.column_fgcolor"))
    {
        setColumnProps(NULL, &data_color);
    }
     else if (prop_name == wxT("tabledoc.column_bgcolor"))
    {
        setColumnProps(&data_color);
    }
     else if (prop_name == wxT("tabledoc.column_pixwidth"))
    {
        int col = m_grid->getCursorColumn();
        if (col >= 0 || col < m_grid->getColumnCount())
        {
            m_grid->setColumnSize(col, data.m_int);
            m_grid->refresh(kcl::Grid::refreshAll);
        }
    }
     else if (prop_name == wxT("tabledoc.column_alignment"))
    {
        switch (data.m_int)
        {
            case 0:
                setColumnProps(NULL, NULL, kcl::Grid::alignDefault);
                break;
            case 1:
                setColumnProps(NULL, NULL, kcl::Grid::alignLeft);
                break;
            case 2:
                setColumnProps(NULL, NULL, kcl::Grid::alignCenter);
                break;
            case 3:
                setColumnProps(NULL, NULL, kcl::Grid::alignRight);
                break;
        }
    }
}

void TableDoc::onActiveChildChanged(cfw::IDocumentSitePtr doc_site)
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
        if (m_set->getRowCount() <= 0 || m_grid->getRowCount() <= 0)
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
        // don't allow dynamic fields to be made static
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
            ITangoGridModelPtr model = m_grid_model;
            if (model)
            {
                evt.Enable(model->getGroupBreakExpr().length() > 0 ? true : false);
            }
        }
        break;

        case ID_Data_RemoveSortFilter:
            evt.Enable(m_sort_order.Length() > 0 || m_filter.Length() > 0 ? true : false);
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
                // situation, do not allow make permanent
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


            // for now, limit deleting to dynamic fields
            tango::IStructurePtr structure = m_iter->getStructure();
            tango::IColumnInfoPtr colinfo;

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
    if (m_temporary)
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

void TableDoc::onSaveAsJobFinished(cfw::IJobPtr job)
{
    // re-enabled the GUI
    setEnabled(true);
    
    if (job->getJobInfo()->getState() != cfw::jobStateFinished)
        return;
    
    g_app->getAppController()->refreshDbDoc();
    
    ICopyJobPtr copy_job = job;
    size_t copy_count = copy_job->getInstructionCount();
    if (copy_count != 1)
        return;
    
    cfw::IDocumentSitePtr site = g_app->getMainFrame()->lookupSiteById(job->getExtraLong());
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
    setBaseSet(copy_job->getResultSet(0), xcm::null);
    
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
    cfw::IDocumentSitePtr textdoc_site;
    cfw::IDocumentSitePtr transdoc_site;
    cfw::IDocumentSitePtr structuredoc_site;
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
    // show a "Save As" dialog
    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeSave);
    if (dlg.ShowModal() != wxID_OK)
        return;

    // if we're on a temporary file and saving the file locally to the
    // project, we need only give the file a name and we're done;
    // otherwise, we need to make a hard copy of the file since we're
    // either creating a local copy of a named file or are saving it
    // to a mount
    if (m_temporary)
    {
        std::wstring cstr;
        std::wstring rpath;
        std::wstring folder = towstr(dlg.getPath().BeforeLast(wxT('/')));

        if (!g_app->getDatabase()->getMountPoint(folder, cstr, rpath))
        {
            tango::ISetPtr save_set = m_set;

            if (!g_app->getDatabase()->storeObject(save_set, towstr(dlg.getPath())))
            {
                cfw::appMessageBox(_("The file could not be saved in the specified location.  The destination location may by in use."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);
                return;
            }

            g_app->getAppController()->refreshDbDoc();
            g_app->getAppController()->updateURLToolbar();
            g_app->getAppController()->updateViewMenu(m_doc_site);

            m_temporary = false;
            
            setSourceUrl(wxEmptyString);
            setSourceMimeType(wxEmptyString);

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
    wxString path = dlg.getPath();
    path = addExtensionIfExternalFsDatabase(path, wxT(".csv"));


    bool is_indeterminate = false;
    tango::IIteratorPtr source_iter = m_iter->clone();
    if (source_iter.isOk())
    {
        source_iter->goFirst();
        
        // determine if the copy job's progress is indeterminate or not
        tango::IFixedLengthDefinitionPtr fset = source_iter->getSet();
        tango::IDelimitedTextSetPtr dset = source_iter->getSet();
        if ((fset.isOk() && !fset->isLineDelimited()) || dset.isOk())
            is_indeterminate = true;
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
                              path.AfterLast('/').c_str());
    
    CopyJob* copy_job = new CopyJob;
    copy_job->getJobInfo()->setTitle(title);
    copy_job->setExtraLong(m_doc_site->getId());

    if (is_indeterminate)
    {
        cfw::IJobInfoPtr job_info = copy_job->getJobInfo();
        job_info->setMaxCount(0.0);
    }


    if (source_iter)
    {
        copy_job->addCopyInstruction(source_iter,
                                     L"",
                                     L"",
                                     g_app->getDatabase(),
                                     path);
    }
     else
    {
            // this type of copy must requery, because no clone() is available
        copy_job->addCopyInstruction(g_app->getDatabase(),
                                     getBaseSet(),
                                     L"",
                                     getFilter(),
                                     getSortOrder(),
                                     g_app->getDatabase(),
                                     path);
    }
    
    copy_job->sigJobFinished().connect(this, &TableDoc::onSaveAsJobFinished);
    g_app->getJobQueue()->addJob(copy_job, cfw::jobStateRunning);
}



void TableDoc::onSaveAsExternal(wxCommandEvent& evt)
{
    wxString filter;

    // NOTE: if you add or remove items from this
    // list, make sure you adjust the case statement
    // below, because it affects which file type the
    // target will be
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
    filter += wxT(" (*.mdb)|*.mdb|");
    filter += _("Microsoft Excel Files");
    filter += wxT(" (*.xls)|*.xls|");
    filter += _("Package Files");
    filter += wxT(" (*.kpg)|*.kpg|");
    filter.RemoveLast(); // get rid of the last pipe sign

    wxString filename = getFilenameFromPath(m_dbpath, false);
    
    if (m_set.isOk() && m_set->isTemporary())
        filename = _("Untitled");
    
    wxFileDialog dlg(g_app->getMainWindow(),
                     _("Save As"),
                     wxT(""),
                     filename,
                     filter,
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() != wxID_OK)
        return;

    int type = 0;
    
    wxString delimiters = wxT("");
    switch (dlg.GetFilterIndex())
    {
        case 0: type = dbtypeDelimitedText;
            delimiters = L",";
            break;
        case 1: type = dbtypeDelimitedText;
            delimiters = L",";
            break;
        case 2: type = dbtypeDelimitedText;
            delimiters = L"\t";
            break;
        case 3: type = dbtypeFixedLengthText; break;
        case 4: type = dbtypeXbase; break;
        case 5: type = dbtypeAccess; break;
        case 6: type = dbtypeExcel; break;
        case 7: type = dbtypePackage; break;
        default:
            wxFAIL_MSG(wxT("invalid filter index"));
            return;
    }
    
    
    
    // -- create an export job --
    
    if (type == dbtypePackage)
    {
        wxString stream_name = m_caption;
        if (stream_name == _("(Untitled)"))
            stream_name = _("Untitled");
            
        wxString title = wxString::Format(_("Saving '%s' as '%s'"),
                      stream_name.c_str(),
                      dlg.GetPath().AfterLast(PATH_SEPARATOR_CHAR).c_str());
        
        ExportPkgJob* job = new ExportPkgJob;
        job->getJobInfo()->setTitle(title);
        job->setPkgFilename(dlg.GetPath(), ExportPkgJob::modeOverwrite);

        job->addExportObject(stream_name,
                             this->m_dbpath,
                             true /* compress */);

        g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
    }
     else
    {
        wxString title = wxString::Format(_("Saving '%s' as '%s'"),
                      getCaption().c_str(),
                      dlg.GetPath().AfterLast(PATH_SEPARATOR_CHAR).c_str());

        ExportJob* job = new ExportJob;
        job->getJobInfo()->setTitle(title);
        job->setExportType(type);

        ExportJobInfo job_export_info;
        job_export_info.input_path = this->m_dbpath;
        job_export_info.output_path = dlg.GetPath();
        job_export_info.append = false;

        if (type == dbtypeDelimitedText)
        {
            job->setDelimiters(delimiters);
            job->setTextQualifier(L"\"");
            job->setFirstRowHeader(true);
        }
        
        if (type == dbtypeAccess || type == dbtypeExcel)
        {
            job->setFilename(dlg.GetPath());
            job_export_info.output_path = this->m_dbpath.AfterLast(wxT('/')).BeforeLast(wxT('.'));
            job_export_info.output_path.Replace(wxT("."), wxT("_"));
            job_export_info.output_path.Replace(wxT(" "), wxT("_"));
            job_export_info.output_path.Replace(wxT("-"), wxT("_"));
            if (job_export_info.output_path.IsEmpty())
                job_export_info.output_path = wxT("exptable");
        }
       
        job->addExportSet(job_export_info);

        g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
    }
}




     
void TableDoc::onReloadDownloadFinished(cfw::IJobInfoPtr job_info)
{
    // this is in the main thread, but truly, only because
    // of the way xulrunner thread marshals the call;
    // we want to handle this later, so fire another signal
    
    if (job_info->getState() != cfw::jobStateFinished)
        return;

    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, ID_DoReloadRefresh);
    ::wxPostEvent(this, evt);
}

void TableDoc::onDoReloadRefresh(wxCommandEvent& evt)
{ 
    wxASSERT_MSG(wxThread::IsMain(), wxT("Callback must happen in the main thread"));
    
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
        m_set.clear();
        m_browse_set.clear();
        m_iter.clear();
        
        // close out text doc
        textdoc->close();
        transdoc->close();
        
        
        // try to delete the filename
        bool error = false;
        wxString filename = urlToFilename(textdoc->getFilename());
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
        transdoc->initFromSet(textdoc->getTextSet());
        
       
        // reload grid
        createModel();
        m_grid->setModel(m_grid_model);
        setBaseSet(textdoc->getTextSet(), xcm::null);
        setEnabled(true);
        
        if (error)
        {
            cfw::appMessageBox(_("The file could not be reloaded, because the file is locked or marked as read only"));
        }
    }
     else if (m_source_mimetype == wxT("application/rss+xml"))
    {
        FeedParser parser;
        if (!parser.loadAndParse(m_reload_filename))
        {
            xf_remove(towstr(m_reload_filename));
            // can't parse feed file
            return;
        }
        
        xf_remove(towstr(m_reload_filename));
        
        tango::ISetPtr old_set = getBaseSet();
        wxString old_set_id = towx(old_set->getSetId());
        
        tango::ISetPtr new_set = parser.convertToSet();
        if (new_set.isNull())
            return;
        
        TableDocMgr::copyModel(old_set, new_set);
        
        setBaseSet(new_set, xcm::null);
        
        TableDocMgr::deleteModel(old_set_id);
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
    wxThread::Sleep(100);
    
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
        
        wxString filename = urlToFilename(textdoc->getFilename());
        filename += wxT(".");
        filename += wxString::Format(wxT("%08x%08x"), (long)time(NULL), (long)clock());
        m_reload_filename = filename;

        textdoc.clear();
        transdoc.clear();


        cfw::IJobInfoPtr job_info;
        job_info.create_instance("cfw.JobInfo");
        if (job_info.isNull())
            return;
            

        job_info->sigStateChanged().connect(this, &TableDoc::onReloadDownloadFinished);
        
        
        m_doing_reload = true;
        WebDoc::downloadFile(m_source_url, filename, job_info);
    }
     else if (m_source_mimetype == wxT("application/rss+xml"))
    {
        cfw::IJobInfoPtr job_info;
        job_info.create_instance("cfw.JobInfo");
        if (job_info.isNull())
            return;
        job_info->sigStateChanged().connect(this, &TableDoc::onReloadDownloadFinished);
            
        m_reload_filename = towx(xf_get_temp_filename(L"feed", L"xml"));

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
            // refresh normal table with query
            
            wxString title = wxString::Format(_("Filtering '%s'"),
                                              getCaption().c_str());

            SortFilterJob* query_job = new SortFilterJob;
            query_job->getJobInfo()->setTitle(title);
            query_job->setInstructions(m_set, m_filter, m_sort_order);

            query_job->sigJobFinished().connect(this, &TableDoc::onSortFilterJobFinished);

            g_app->getJobQueue()->addJob(query_job, cfw::jobStateRunning);
        }
    }
}

void TableDoc::onPrint(wxCommandEvent& evt)
{
    print();
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
    // tango::sfFastRowCount)
    tango::rowpos_t row_count;
    
    if (m_stat_row_count != (tango::rowpos_t)-1)
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
        text = wxString::Format(_("Sum: %s"), cfw::dbl2fstr(sum, scale).c_str());
    } 
     else
    {
        text = wxEmptyString;
    }

    // get the tabledoc's sum statusbar cell
    cfw::IStatusBarItemPtr item;
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
    if (m_filter.Length() + m_sort_order.Length() > 0)
        currow_text = wxT("~");
    currow_text += cfw::dbl2fstr(m_grid->getCursorRow()+1);

    if (m_browse_set.isOk())
    {
        if (row_count_update)
        {
            if (m_browse_set->getSetFlags() & tango::sfFastRowCount)
            {
                m_stat_row_count = m_browse_set->getRowCount();
            }
             else
            {
                m_stat_row_count = (tango::rowpos_t)-1;
            }
        }
        
        if (m_stat_row_count != (tango::rowpos_t)-1)
        {
            position_text = wxString::Format(_("Position: %s"), currow_text.c_str());
            reccount_text = wxString::Format(_("Record Count: %s"),
                cfw::dbl2fstr((tango::tango_int64_t)m_stat_row_count).c_str());
        }
         else
        {
            position_text = wxString::Format(_("Position: %s"), currow_text.c_str());
            reccount_text = wxEmptyString;
        }
    }

    cfw::IStatusBarItemPtr item;
    cfw::IStatusBarPtr statusbar = g_app->getMainFrame()->getStatusBar();
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
            insertColumn(idx, (*it)->m_strvalue);
    }
}



bool TableDoc::setBaseSet(tango::IDatabasePtr db,
                          const wxString& table,
                          tango::ISetPtr optional_set,
                          tango::IIteratorPtr optional_iterator)
{
    tango::ISetPtr set = optional_set;
    if (set.isNull())
        set = db->openSet(towstr(table));
    
    m_db_type = db->getDatabaseType();
    
    m_mount_db = db->getMountDatabase(towstr(table));
    if (m_mount_db)
    {
        m_db_type = m_mount_db->getDatabaseType();
    }
    
    return setBaseSet(set, optional_iterator);
}


bool TableDoc::setBaseSet(tango::ISetPtr set, tango::IIteratorPtr iter)
{
    m_filter = wxT("");
    m_sort_order = wxT("");
    m_caption_suffix = wxT("");

    // -- save set pointer --
    m_set = set;
    m_browse_set = set;
    
    if (m_set.isNull())
        return false;

    if (m_set.isOk())
        m_temporary = m_set->isTemporary();
         else
        m_temporary = false;
    

    // if the set/table displayed has a url associated with it,
    // display it


    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isOk() && m_set.isOk())
    {
        tango::IAttributesPtr attr = db->getAttributes();
        wxString url = towx(attr->getStringAttribute(tango::dbattrDatabaseUrl));
        if (url.Length() > 0)
        {
            url += towx(m_set->getObjectPath());
            setSourceUrl(url);
        }
    }


    // update caption
    updateCaption();


    // load a model for this set
    if (m_temporary_model)
        m_model = TableDocMgr::loadModel(wxT(""));
         else
        m_model = TableDocMgr::loadModel(towx(m_set->getSetId()));



    tango::IIteratorPtr browse_iter;

    if (iter.isOk())
    {
        browse_iter = iter;
    }
     else
    {
        // create a default iterator
        if (set.isOk())
        {
            browse_iter = set->createIterator(L"", L"", NULL);
        }
    }

    if (browse_iter.isNull())
    {
        // something's wrong
        return false;
    }

    // do a structure update
    doStructureUpdate();

    setIterator(browse_iter);

    return true;
}

bool TableDoc::setBrowseSet(tango::ISetPtr set, tango::IIteratorPtr iter)
{
    if (set.isNull())
        return false;

    m_browse_set = set;

    if (iter.isOk())
    {
        setIterator(iter);
    }
     else
    {
        // create a default iterator
        tango::IIteratorPtr iter = set->createIterator(L"", L"", NULL);
        setIterator(iter);
    }

    updateChildWindows();

    return true;
}


void TableDoc::closeSet()
{
    setEnabled(false);

    freeTemporaryHandles();
    m_grid->setModel(xcm::null);

    m_grid_model.clear();
    m_set.clear();
    m_browse_set.clear();
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

    m_frame->postEvent(new cfw::Event(wxT("tabledoc.enabledStateChanged")));
    wxTheApp->ProcessIdle();
}

bool TableDoc::getEnabled()
{
    return m_enabled;
}





void TableDoc::reloadSettings(bool redraw)
{
    cfw::IAppPreferencesPtr prefs = g_app->getAppPreferences();

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
        ITangoGridModelPtr tg_model = model;

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




void TableDoc::connectModifyStructJob(ModifyStructJob* modify_job)
{
    modify_job->sigJobFinished().connect(this, &TableDoc::onModifyStructJobFinished);
}


void TableDoc::onColumnNameChanged(const wxString& old_name,
                                   const wxString& new_name)
{
    if (old_name.CmpNoCase(new_name) == 0)
        return;

    int idx;

    // update view structures

    ITableDocViewEnumPtr viewvec = m_model->getViewEnum();
    ITableDocViewPtr view;
    int view_count = viewvec->size();
    int i;


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
    {
        return;
    }

    while (1)
    {
        idx = m_grid->getColumnViewIdxByName(old_name);
        if (idx == -1)
        {
            break;
        }

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
    cfw::IDocumentPtr querydoc = lookupOtherDocument(m_doc_site, "appmain.QueryDoc");
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
    if (m_set.isOk())
    {
        wxString temps = towx(m_set->getObjectPath());
        
        m_dbpath = temps;
        
        if (m_set->isTemporary())
        {
            m_caption = _("(Untitled)");
        }
         else
        {
            if (m_source_url.Length() > 0)
                m_caption = m_source_url.AfterLast(wxT('/'));
             else
                m_caption = temps.AfterLast(wxT('/'));
        }
    }

    if (m_doc_site)
    {
        wxString s;
        s = m_caption;
        s += m_caption_suffix;

        m_doc_site->setCaption(s);
    }
}


wxString TableDoc::makeCaption(const wxString& title)
{
    // make panel caption
    wxString caption = title;

    if (m_set.isOk())
    {
        if (!m_set->isTemporary())
        {
            wxString name = towx(m_set->getObjectPath());
            name.AfterLast(wxT('/'));
            
            caption += wxT(" - [");
            caption += name;
            caption += wxT("]");
        }
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

void TableDoc::setIterator(tango::IIteratorPtr iter, bool go_first)
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
    if (m_iter->getIteratorFlags() & tango::ifForwardOnly)
    {
        // if the iterator is forward-only, try to turn on
        // tango's backward scroll row cache
        m_iter->setIteratorFlags(tango::ifReverseRowCache, tango::ifReverseRowCache);
    }
    
    if (go_first)
        m_iter->goFirst();
    m_grid_model->setIterator(m_iter);

    if (!go_first)
    {
        // convince the grid and its row model to jump to
        // where the iterator is currently positioned
        kcl::IModelPtr model = m_grid_model;
        tango::tango_int64_t row_count = (tango::tango_int64_t)model->getRowCount();
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
        tango::IStructurePtr s = m_iter->getStructure();
        initializeDefaultView(defview, s);
        defview->setDescription(wxString(_("Default View")));
        setActiveView(defview);
        m_default_view_created = true;
    }

    if (m_active_view.isNull())
        setActiveView(viewvec->getItem(0));


    if (m_grid)
    {
        // insertion of new rows is not allowed when
        // an order or a filter is set

        if (m_sort_order.IsEmpty() &&
            m_filter.IsEmpty() &&
            m_set == m_browse_set)
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

tango::IIteratorPtr TableDoc::getIterator()
{
    return m_iter;
}

tango::ISetPtr TableDoc::getBaseSet()
{
    return m_set;
}

ITableDocModelPtr TableDoc::getModel()
{
    return m_model;
}

tango::ISetPtr TableDoc::getBrowseSet()
{
    return m_browse_set;
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

    m_frame->postEvent(new cfw::Event(wxT("tabledoc.viewModified"), 0));

    // update other windows that are showing the same view
    cfw::Event* e = new cfw::Event(wxT("tabledoc.doViewReload"),
                                   (unsigned long)this);
    e->l_param2 = (unsigned long)m_active_view.p;
    m_frame->postEvent(e);
}


void TableDoc::insertColumn(int insert_pos, const wxString& expr)
{
    insertColumnInternal(insert_pos, expr, true);
}

void TableDoc::insertColumnInternal(int insert_pos,
                                    const wxString& expr,
                                    bool save_view)
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
    col->setName(viewcol_name);
    col->setSize(80);

    if (save_view)
    {
        m_model->writeObject(m_active_view);
    }

    int idx = m_grid->insertColumn(insert_pos, colidx);
    m_grid->setColumnSize(idx, 80);

    m_grid->refresh(kcl::Grid::refreshAll);

    m_frame->postEvent(new cfw::Event(wxT("tabledoc.viewModified"), 0));

    // update other windows that are showing the same view
    cfw::Event* e = new cfw::Event(wxT("tabledoc.doViewReload"),
                                  (unsigned long)this);
    e->l_param2 = (unsigned long)m_active_view.p;
    m_frame->postEvent(e);
}


void TableDoc::insertChildColumn(int insert_pos, const wxString& text)
{
    // try to find an existing dynamic field which contains
    // the dynamic field expression

    tango::ISetPtr set = getBaseSet();
    if (set.isNull())
        return;

    tango::IStructurePtr s = set->getStructure();

    int col_count = s->getColumnCount();
    tango::IColumnInfoPtr colinfo;

    int i;

    for (i = 0; i < col_count; ++i)
    {
        colinfo = s->getColumnInfoByIdx(i);
        if (!colinfo->getCalculated())
            continue;

        if (text.CmpNoCase(towx(colinfo->getExpression())) == 0)
        {
            // we found an exact match
            insertColumn(insert_pos, towx(colinfo->getName()));
            return;
        }
    }

    // a suitable dynamic field did not exist for the expression,
    // so we have to create it

    if (text.Find(wxT('.')) == -1)
    {
        return;
    }

    wxString rel_tag = text.BeforeFirst(wxT('.'));
    wxString col_name = text.AfterFirst(wxT('.'));


    // now try to find the set that has that column

    tango::ISetPtr right_set;
    tango::IStructurePtr right_structure;

    tango::IRelationEnumPtr rel_enum = set->getRelationEnum();
    tango::IRelationPtr rel;
    int rel_count = rel_enum->size();

    for (i = 0; i < rel_count; ++i)
    {
        rel = rel_enum->getItem(i);

        if (rel.isNull())
            continue;

        if (0 != rel_tag.CmpNoCase(towx(rel->getTag())))
            continue;

        right_set = rel->getRightSetPtr();
        if (right_set.isNull())
            continue;

        right_structure = right_set->getStructure();

        colinfo = right_structure->getColumnInfo(towstr(col_name));
        if (colinfo.isNull())
        {
            // we couldn't find the appropriate column
            return;
        }
    }

    wxString column_name = cfw::makeProperIfNecessary(col_name);
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
        cfw::Event* evt = new cfw::Event(wxT("tabledoc.structureModified"));
        evt->s_param = m_dbpath;
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
    m_frame->postEvent(new cfw::Event(wxT("tabledoc.viewModified"), 0));

    // update other windows that are showing the same view
    cfw::Event* e = new cfw::Event(wxT("tabledoc.doViewReload"),
                                   (unsigned long)this);
    e->l_param2 = (unsigned long)m_active_view.p;
    m_frame->postEvent(e);
}


void TableDoc::onSortFilterJobFinished(cfw::IJobPtr job)
{
    if (job.isOk())
    {
        // if the job that finished is the quick filter job,
        // reset the filter pending state
        int id = job->getJobId();
        if (id == m_quick_filter_jobid)
            m_quick_filter_jobid = quickFilterNotPending;
    }

    if (job->getJobInfo()->getState() != cfw::jobStateFinished)
    {
        // if the job is cancelled or failed, update the filter toolbar item
        // and we're done
        if (job->getJobInfo()->getState() == cfw::jobStateCancelled ||
            job->getJobInfo()->getState() == cfw::jobStateFailed)
        {
            g_app->getAppController()->updateQuickFilterToolBarItem();    
        }

        return;
    }

    ISortFilterJobPtr query_job = job;

    m_filter = query_job->getCondition();
    m_sort_order = query_job->getOrder();
    setBrowseSet(query_job->getResultSet(), query_job->getResultIterator());

    updateStatusBar();
    g_app->getAppController()->updateQuickFilterToolBarItem();

    wxString suffix;
    if (m_filter.Length() > 0)
    {
        suffix = wxT(" [");
        suffix += _("Filtered");
        suffix += wxT("]");
    }
    
    setCaption(wxEmptyString, suffix);
}

void TableDoc::onSetOrderFinished(cfw::IJobPtr job)
{
    if (job->getJobInfo()->getState() != cfw::jobStateFinished)
        return;

    ISortFilterJobPtr query_job = job;


    // set the sort order
    m_sort_order = query_job->getOrder();
    

    // if we have a group break and the group break isn't a subset
    // of the new sort order, remove the group break; TODO: decide
    // if this behavior is predictable, or if we should always
    // remove the group sort when setting an order of any kind
    std::vector< std::pair<wxString, bool> > sort_fields;
    std::vector<wxString> group_fields;

    sort_fields = sortExprToVector(m_sort_order);
    group_fields = stringToVector(m_group_break);

    bool remove_group_break = true;
    if (group_fields.size() <= sort_fields.size())
    {
        remove_group_break = false;
        
        std::vector<wxString>::iterator it, it_end;
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

    if (remove_group_break)
        setGroupBreak(wxT(""));    


    // set the browse set and update the status bar
    setBrowseSet(query_job->getResultSet(), query_job->getResultIterator());
    
    updateStatusBar();
}

void TableDoc::onDeleteJobFinished(cfw::IJobPtr delete_job)
{
    m_grid->reset();
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void TableDoc::onIndexJobFinished(cfw::IJobPtr job)
{
    // set as the active order
    AppBusyCursor bc;

    IIndexJobPtr index_job = job;
    if (index_job.isNull())
        return;

    std::vector<IndexJobInstruction> instructions;
    instructions = index_job->getInstructions();

    if (instructions.size() != 1)
        return;

    tango::ISetPtr set = m_set;
    tango::IIteratorPtr iter;

    iter = set->createIterator(L"*",
                               towstr(instructions[0].expr),
                               NULL);

    if (iter.isNull())
        return;

    m_filter = wxT("");
    m_sort_order = instructions[0].expr;

    setBrowseSet(set, iter);

    updateStatusBar();
}

void TableDoc::onReplaceJobFinished(cfw::IJobPtr replace_job)
{
    m_grid->refresh(kcl::Grid::refreshAll);
}

void TableDoc::onAppendRecordsFinished(cfw::IJobPtr append_job)
{
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void TableDoc::onModifyStructJobFinished(cfw::IJobPtr job)
{
    IModifyStructJobPtr modify_job = job;

    // doStructureUpdate() will update the views and such
    // when setBaseSet is called

    if (job->getJobInfo()->getState() == cfw::jobStateFinished)
        m_structure_job = modify_job;
         else
        m_structure_job.clear();

    // -- unlock this window --
    m_enabled = true;
    m_grid->setVisibleState(kcl::Grid::stateVisible);
    m_frame->postEvent(new cfw::Event(wxT("tabledoc.enabledStateChanged")));

    createModel();
    m_grid->setModel(m_grid_model);
    setBaseSet(modify_job->getActionSet(), xcm::null);
    
    // remove the "Filtered" suffix
    setCaption(wxEmptyString, wxEmptyString);
    
    if (job->getJobInfo()->getState() != cfw::jobStateFinished)
        return;

    // do inserts for each new field
    std::vector<std::pair<wxString,int> > to_insert;
    std::vector<std::pair<wxString,int> >::iterator insert_iter;
    
    modify_job->getToInsert(to_insert);
    for (insert_iter = to_insert.begin();
         insert_iter != to_insert.end(); ++insert_iter)
    {
        insertColumnInternal(insert_iter->second,
                             insert_iter->first, false);
    }
    
    // do deletes in the view
    std::vector<wxString> to_delete;
    std::vector<wxString>::iterator delete_iter;
    
    modify_job->getToDelete(to_delete);
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

    m_frame->postEvent(new cfw::Event(wxT("tabledoc.viewModified"), 0));

    // update other windows that are showing the same view
    cfw::Event* e = new cfw::Event(wxT("tabledoc.doViewReload"),
                                   (unsigned long)this);
    e->l_param2 = (unsigned long)m_active_view.p;
    m_frame->postEvent(e);
    
    updateStatusBar();
    g_app->getAppController()->updateQuickFilterToolBarItem();
    
    // let other windows know that the structure was modified
    cfw::Event* evt = new cfw::Event(wxT("tabledoc.structureModified"));
    evt->s_param = m_dbpath;
    m_frame->postEvent(evt);
}



void TableDoc::doStructureUpdate()
{
    if (m_structure_job.isNull())
        return;

    std::vector<std::pair<wxString, wxString> > to_rename;
    std::vector<std::pair<wxString, wxString> >::iterator rename_iter;

    m_structure_job->getToRename(to_rename);

    for (rename_iter = to_rename.begin();
         rename_iter != to_rename.end(); ++rename_iter)
    {
        onColumnNameChanged(rename_iter->first, rename_iter->second);
    }


    m_structure_job.clear();
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
    wxString lhs = quoteIdentifier(g_app->getDatabase(), field);
    wxString rhs = value;

    if (type == tango::typeBoolean)
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
    if ((type == tango::typeDate || type == tango::typeDateTime) &&
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
    if (type == tango::typeCharacter || type == tango::typeWideCharacter)
    {
        // double single quotes
        rhs = cfw::doubleQuote(rhs, L'\'');
        
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

static wxMenu* createIndexesMenu(tango::IIndexInfoEnumPtr indexes,
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
        tango::IIndexInfoPtr index = indexes->getItem(i);
        wxString index_tag = towx(index->getTag());
        wxString index_expr = towx(index->getExpression());
        
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
    ITangoGridModelPtr tango_grid_model = model;
    if (!tango_grid_model)
        return;
        
    kcl::IModelColumnPtr grid_colinfo = model->getColumnInfo(model_col);
    if (grid_colinfo.isNull())
        return;

    if (!model->isRowValid(row))
        return;
        
    wxString colname = grid_colinfo->getName();

    //  get column type from the iterator
    tango::objhandle_t colhandle = m_iter->getHandle(towstr(colname));
    if (!colhandle)
        return;

    tango::IColumnInfoPtr colinfo = m_iter->getInfo(colhandle);
    if (colinfo.isNull())
        return;

    int coltype = colinfo->getType();
    
    m_iter->releaseHandle(colhandle);


    wxString value;

    switch (coltype)
    {
        case tango::typeWideCharacter:
        case tango::typeCharacter:
        {
            // we'll handle quoting in the menu expression function
            value = tango_grid_model->getCellString(row, model_col);
            break;
        }

        case tango::typeDouble:
        case tango::typeNumeric:
        {
            double d = tango_grid_model->getCellDouble(row, model_col);
            value = wxString::Format(wxT("%.*f"), colinfo->getScale(), d);
            value.Replace(wxT(","), wxT("."));
        }
        break;

        case tango::typeInteger:
        {
            int i = tango_grid_model->getCellInteger(row, model_col);
            value = wxString::Format(wxT("%d"), i);
        }
        break;

        case tango::typeDate:
        {   
            tango::datetime_t d = tango_grid_model->getCellDateTime(row, model_col);
            if (d == 0)
                value = wxT("null");
            else
            {
                tango::DateTime dt = d;
                        
                switch (m_db_type)
                {
                    default:
                    case tango::dbtypeXdnative:
                        value = wxString::Format(wxT("DATE('%04d-%02d-%02d')"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay());
                        break;
                    case tango::dbtypeSqlServer:
                        value = wxString::Format(wxT("{d '%04d-%02d-%02d' }"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay());
                        break;
                    case tango::dbtypeOracle:
                        value = wxString::Format(wxT("TO_DATE('%04d-%02d-%02d', 'YYYY-MM-DD')"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay());
                        break;
                }
            }

            break;
        }

        case tango::typeDateTime:
        {
            tango::datetime_t d = tango_grid_model->getCellDateTime(row, model_col);
            if (d == 0)
                value = wxT("null");
            else
            {
                tango::DateTime dt = d;
                 
                switch (m_db_type)
                {
                    default:
                    case tango::dbtypeXdnative:
                        value = wxString::Format(wxT("DATE('%04d-%02d-%02d %02d:%02d:%02d')"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay(),
                                                 dt.getHour(), dt.getMinute(), dt.getSecond());
                        break;
                    case tango::dbtypeSqlServer:
                        value = wxString::Format(wxT("{ts '%04d-%02d-%02d %02d:%02d:%02d' }"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay(),
                                                 dt.getHour(), dt.getMinute(), dt.getSecond());
                        break;
                    case tango::dbtypeOracle:
                        value = wxString::Format(wxT("TO_DATE('%04d-%02d-%02d %02d:%02d:%02d', 'YYYY-MM-DD HH24:MI:SS')"),
                                                 dt.getYear(), dt.getMonth(), dt.getDay(),
                                                 dt.getHour(), dt.getMinute(), dt.getSecond());
                        break;
                }
            }

            break;
        }

        case tango::typeBoolean:
        {
            bool bool_value = tango_grid_model->getCellBoolean(row, model_col);        
            switch (m_db_type)
            {            
                default:
                case tango::dbtypeXdnative:
                    value = bool_value ? wxT("true") : wxT("false");
                    break;

                case tango::dbtypeSqlServer:
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
    cfw::CommandCapture* cc = new cfw::CommandCapture;
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

        
        setFilter(expr);
    }
     else if (command >= 27200 && command <= 27299)
    {
        // copy rows
        wxString expr;
        getMenuItemExpr(colname, coltype, command-27200, value, expr);
        copyRecords(expr);
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
            deleteRecords(expr);
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
    

    tango::IStructurePtr iter_structure = m_iter->getStructure();
    tango::IStructurePtr set_structure = getBaseSet()->getStructure();



    // find out if any dynamic fields are selected
    bool dynamic_selected = false;
    bool permanent_selected = false;
    int selected_count = 0;

    kcl::IModelPtr model = m_grid->getModel();
    ITangoGridModelPtr tmodel = model;
    
    int col_count = m_grid->getColumnCount();
    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
        {
            // increment the selected count
            selected_count++;

            // this code fragement makes sure that we are not doing
            // context column operations on dynamic fields that are
            // currently being modified; when checking, make sure we
            // have a valid model column index (i.e., that we're not 
            // on a column separator)
            int model_idx = m_grid->getColumnModelIdx(i);
            if (model_idx != -1)
            {
                kcl::IModelColumnPtr colinfo = model->getColumnInfo(model_idx);
                wxString col_name = colinfo->getName();
                tango::IColumnInfoPtr iter_col_info = iter_structure->getColumnInfo(towstr(col_name));
                if (!iter_col_info)
                    return;
                tango::IColumnInfoPtr set_col_info = set_structure->getColumnInfo(towstr(col_name));
                if (!set_col_info)
                    return;
                if (iter_col_info->getCalculated() != set_col_info->getCalculated())
                    return;
                if (iter_col_info->getCalculated())
                {
                    if (iter_col_info->getType() != set_col_info->getType())
                        return;
                    if (iter_col_info->getWidth() != set_col_info->getWidth())
                        return;
                    if (iter_col_info->getScale() != set_col_info->getScale())
                        return;
                    if (iter_col_info->getExpression() != set_col_info->getExpression())
                        return;
                }
            }

            if (tmodel->getColumnCalculated(model_idx))
                dynamic_selected = true;
                 else
                permanent_selected = true;
        }
    }
    
    wxMenu menuPopup;

    menuPopup.Append(ID_Data_SortAscending, _("Sort &Ascending"));
    menuPopup.Append(ID_Data_SortDescending, _("&Sort Descending"));
    menuPopup.AppendSeparator();
    menuPopup.Append(27700, _("Sort &Orders"),
                     createIndexesMenu(getBaseSet()->getIndexEnum(), getSortOrder(), 27700));
    menuPopup.Append(ID_Data_RemoveSortFilter, _("&Remove Sort/Filter"));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Table_InsertGroupBreak, _("Insert &Group Break"));
    menuPopup.Append(ID_Table_RemoveGroupBreak, _("Re&move Group Break"));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Data_CreateDynamicField, _("Insert &Calculated Field..."));
    if (dynamic_selected && selected_count == 1)
        menuPopup.Append(ID_Data_ModifyDynamicField, _("&Edit Calculated Field..."));
    if (!permanent_selected)
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


    cfw::CommandCapture* cc = new cfw::CommandCapture;
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
            setSortOrder(wxEmptyString);
            return;
        }
        
        tango::IIndexInfoEnumPtr indexes = getBaseSet()->getIndexEnum();
        if (i >= (int)(indexes->size()+1))
        {
            // user clicked the "Edit..." menu item
            showIndexPanel();
        }
         else
        {
            // user clicked on one of the indexes;
            // set that index as the sort order
            tango::IIndexInfoPtr index = indexes->getItem(i-1);
            setSortOrder(towx(index->getExpression()));
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
        m_frame->postEvent(new cfw::Event(wxT("tabledoc.cursorColumnChanged")));
    }

    if (evt.GetRow() != evt.GetDestinationRow())
    {
        updateStatusBar(false);
        updateChildWindows();

        m_frame->postEvent(new cfw::Event(wxT("tabledoc.cursorRowChanged")));
    }
}



static void setColumnProps(wxColor* fill_color,
                           wxColor* text_color,
                           int alignment)
{
    ITableDocPtr table_doc;

    cfw::IDocumentSitePtr doc_site = g_app->getMainFrame()->getActiveChild();
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
    if (m_browse_set->getRelationCount() == 0)
        return;

    wxString site_name;
    cfw::IDocumentSitePtr site;
    ITableDocPtr table_doc;
    tango::IRelationPtr rel;
    tango::IRelationEnumPtr rel_enum;
    rel_enum = m_browse_set->getRelationEnum();
    int rel_count = rel_enum->size();

    for (int i = 0; i < rel_count; ++i)
    {
        rel = rel_enum->getItem(i);
        site_name = wxString::Format(wxT("%s-%s"),
                       towx(m_browse_set->getSetId()).c_str(),
                       towx(rel->getTag()).c_str());

        site = g_app->getMainFrame()->lookupSite(site_name);
        
        if (site.isNull())
        {
            continue;
        }

        table_doc = site->getDocument();
        if (table_doc.isOk())
        {
            table_doc->setBrowseSet(table_doc->getBaseSet(), xcm::null);
            table_doc->setCaption(wxT(""), wxT(""));
            site->setName(wxT(""));
            table_doc->setIsChildSet(false);
        }

    }
}


static wxString generateContextSyncMarkExpression(
                                const wxString& left_expr,
                                const wxString& right_expr,
                                tango::IIteratorPtr& left_iter,
                                tango::IIteratorPtr& right_iter)
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
        
        tango::objhandle_t lh = left_iter->getHandle(towstr(left_parts[idx]));
        idx++;
        if (!lh)
            return wxT("");
        
        tango::objhandle_t rh = right_iter->getHandle(towstr(*it));
        if (!rh)
            return wxT("");
        
        part = quoteIdentifier(g_app->getDatabase(), *it);
        part += wxT("=");
        
        tango::IColumnInfoPtr info = right_iter->getInfo(rh);
        if (info.isNull())
            return wxT("");
            
        switch (info->getType())
        {
            case tango::typeCharacter:
            case tango::typeWideCharacter:
                part += wxT("'");
                part += cfw::doubleQuote(towx(left_iter->getWideString(lh)), '\'');
                part += wxT("'");
                break;
            
            case tango::typeInteger:
                part += wxString::Format(wxT("%d"), left_iter->getInteger(lh));
                break;
                
            case tango::typeNumeric:
            case tango::typeDouble:
                value = wxString::Format(wxT("%.*f"), info->getScale(), left_iter->getDouble(lh));
                value.Replace(wxT(","), wxT("."));
                part += value;
                break;
                
            case tango::typeDate:
            {
                tango::datetime_t dtt = left_iter->getDateTime(lh);
                tango::DateTime dt;
                dt.setDateTime(dtt);

                part += wxString::Format(wxT("{d '%04d-%02d-%02d'}"),
                                       dt.getYear(), dt.getMonth(), dt.getDay());
                break;
            }
            
            case tango::typeDateTime:
            {
                tango::datetime_t dtt = left_iter->getDateTime(lh);
                tango::DateTime dt;
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

    if (m_browse_set->getRelationCount() == 0)
        return;

    // this will ensure that our iterator is positioned
    // on the correct row
    if (m_grid)
    {
        ITangoGridModelPtr grid_model = m_grid->getModel();
        if (grid_model)
        {
            grid_model->getRowId(m_grid->getCursorRow());
        }
    }


    wxString site_name;
    cfw::IDocumentSitePtr site;
    ITableDocPtr table_doc;
    tango::IRelationPtr rel;
    tango::IRelationEnumPtr rel_enum;
    rel_enum = m_browse_set->getRelationEnum();
    int rel_count = rel_enum->size();


    cfw::IDocumentSiteEnumPtr doc_sites;
    doc_sites = g_app->getMainFrame()->getDocumentSites(cfw::sitetypeNormal);

    for (int i = 0; i < rel_count; ++i)
    {
        rel = rel_enum->getItem(i);
        site_name = wxString::Format(wxT("%s-%s"),
                       towx(m_browse_set->getSetId()).c_str(),
                       towx(rel->getTag()).c_str());

        site = g_app->getMainFrame()->lookupSite(site_name);
        
        if (site.isNull())
        {
            // we could not find a site with our link label, so we
            // must try to find a suitable site (one with the
            // same set as our "right set"

            bool found = false;
                        
            int site_count = doc_sites->size();
            for (int i = 0; i < site_count; ++i)
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
                    tango::ISetPtr base_set = table_doc->getBaseSet();
                    if (base_set == rel->getRightSetPtr())
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
            tango::IIteratorRelationPtr iter_r = m_iter;
            if (iter_r.isNull())
                return;
                    
            if (m_relationship_sync == tabledocRelationshipSyncFilter)
            {       
                tango::ISetPtr child_set = iter_r->getChildSet(rel->getTag());
                if (child_set)
                {
                    table_doc->setBrowseSet(child_set, xcm::null);

                    wxString suffix;
                    suffix = wxT(" ");
                    suffix += _("[Matching Records]");

                    table_doc->setCaption(wxT(""), suffix);

                    table_doc->updateChildWindows();
                    table_doc->setIsChildSet(true);
                }
            }
             else if (m_relationship_sync == tabledocRelationshipSyncSeek)
            {
                tango::IIteratorPtr right_iter = iter_r->getChildIterator(rel->getTag());
                if (right_iter)
                {
                    wxString expr = generateContextSyncMarkExpression(
                                        towx(rel->getLeftExpression()),
                                        towx(rel->getRightExpression()),
                                        m_iter,
                                        right_iter);
                    
                    table_doc->setRelationshipSyncMarkExpr(expr);
                    table_doc->setIterator(right_iter, false);
                    
                    wxString suffix;
                    suffix = wxT(" ");
                    suffix += _("[Matching Records And Context]");
                    table_doc->setCaption(wxT(""), suffix);

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
        cfw::Event* e = new cfw::Event(wxT("tabledoc.doViewReload"),
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
        cfw::IAppPreferencesPtr prefs = g_app->getAppPreferences();
        int row_height = getAppPrefsLong(wxT("grid.row_height"));
        
        m_active_view->setRowSize(row_height);
        
        refreshActiveView();
    }
}


void TableDoc::onGridColumnMove(kcl::GridEvent& evt)
{
    cfw::Event* e;

    m_active_view->moveColumn(evt.GetColumn(), evt.GetDestinationColumn());

    e = new cfw::Event(wxT("tabledoc.viewModified"), 0);
    e->s_param = wxT("colmove");
    m_frame->postEvent(e);

    e = new cfw::Event(wxT("tabledoc.doViewReload"), (unsigned long)this);
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
        tango::objhandle_t colhandle = m_iter->getHandle(towstr(colname));
        if (!colhandle)
            return;

        tango::IColumnInfoPtr colinfo = m_iter->getInfo(colhandle);
        if (colinfo.isNull())
            return;

        event.SetString(wxString::Format(_("Name: %s, Type: %s, Width: %d, Decimals: %d"),
                            cfw::makeProperIfNecessary(towx(colinfo->getName())).c_str(),
                            getTypeText(colinfo->getType()).c_str(),
                            colinfo->getWidth(),
                            colinfo->getScale()));
    }
     else
    {
        tango::objhandle_t colhandle = m_iter->getHandle(towstr(colname));
        if (!colhandle)
            return;

        tango::IColumnInfoPtr colinfo = m_iter->getInfo(colhandle);
        if (colinfo.isNull())
            return;

        int coltype = colinfo->getType();

        if (coltype == tango::typeDate ||
            coltype == tango::typeDateTime)
        {
            const wxChar* date_rep = wxT("%#c");

            if (coltype == tango::typeDate)
            {
                date_rep = wxT("%#x");
            }

            // -- move model --
            if (!model->isRowValid(row))
            {
                m_iter->releaseHandle(colhandle);
                return;
            }

            tango::datetime_t dt = m_iter->getDateTime(colhandle);
            if (dt != 0)
            {
                tango::DateTime d(dt);

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
        ITangoGridModelPtr model = m_grid->getModel();
        if (model.isNull())
            return;

        if (model->getColumnCalculated(model_idx))
        {
            evt.Veto();
            return;
        }
    }
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

    kcl::IModelPtr model = m_grid->getModel();

    ITangoGridModelPtr tango_grid_model = model;
    if (tango_grid_model.isNull())
        return;

    kcl::IModelColumnPtr model_colinfo = model->getColumnInfo(model_col);
    if (model_colinfo.isNull())
        return;
        
    std::wstring primary_key;
    tango::IFileInfoPtr info = g_app->getDatabase()->getFileInfo(getBaseSet()->getObjectPath());
    if (info.isOk())
    {
        primary_key = info->getPrimaryKey();
    }

    // can't update -- table doesn't have a primary key or a rowid
    if (primary_key.empty() && getDbDriver() != wxT("xdnative") && getDbDriver() != wxT("xdclient"))
    {
        cfw::deferredAppMessageBox(_("The database table does not have a primary key specified, which is required for editing data values."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_INFORMATION | wxCENTER);
        return;
    }

    wxString col_name = model_colinfo->getName();

    tango::rowid_t rowid = tango_grid_model->getRowId(m_grid->getCursorRow());
    if ((getDbDriver() == wxT("xdnative") || getDbDriver() == wxT("xdclient")) && rowid == 0)
        return;

    // quote identifier to use
    wxString left_quote_identifier = wxT("[");
    wxString right_quote_identifier = wxT("]");

    
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
        
        wxStringTokenizer t(towx(primary_key), wxT(","));
        while (t.HasMoreTokens())
        {
            wxString s = t.GetNextToken();
            s.Trim();
            s.Trim(FALSE);
            prikeys.push_back(dequoteIdentifier(s));
        }
        
        std::vector<wxString>::iterator it;
        for (it = prikeys.begin(); it != prikeys.end(); ++it)
        {
            tango::objhandle_t handle = getTemporaryHandle(*it);
            if (!handle)
                return;
            
            int type = m_iter->getType(handle);
            
            std::wstring piece;
            piece = L"(";
            piece += left_quote_identifier;
            piece += towstr(*it);
            piece += right_quote_identifier;
            piece += L"=";
            
            switch (type)
            {
                case tango::typeCharacter:
                case tango::typeWideCharacter:
                    piece += L"'";
                    piece += m_iter->getWideString(handle);
                    piece += L"'";
                    break;
                    
                case tango::typeInteger:
                {
                    int i = m_iter->getInteger(handle);
                    wxString str = wxString::Format(wxT("%d"), i);
                    piece += towstr(str);
                }
                break;
                
                case tango::typeNumeric:
                case tango::typeDouble:
                {
                    tango::IColumnInfoPtr info = m_iter->getInfo(handle);
                    if (info.isNull())
                        return;
                    double d = m_iter->getDouble(handle);
                    int scale = info->getScale();
                    wxString str = wxString::Format(wxT("%.*f"), scale, d);
                    piece += towstr(str);
                }
                break;
                
                case tango::typeDate:
                {
                    tango::datetime_t dtt = m_iter->getDateTime(handle);
                    tango::DateTime dt;
                    dt.setDateTime(dtt);
                    wxString str;
                    
                    if (getDbDriver() == wxT("xdoracle"))
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


    tango::IStructurePtr structure = getBaseSet()->getStructure();
    if (structure.isNull())
        return;

    tango::IColumnInfoPtr col_info = structure->getColumnInfo(towstr(col_name));
    if (col_info.isNull())
        return;


    wxString quoted_col_name = col_name;
    
    if (::wxStrpbrk(quoted_col_name, wxT(" \t./\\[]{}-!@#$%^&*()")))
        quoted_col_name = left_quote_identifier + col_name + right_quote_identifier;


    // update_info is used by ICacheRowUpdate below, however only
    // iterators that employ a row cache need this
    tango::ColumnUpdateInfo update_info;
    update_info.null = false;

    wxString str;
    switch (col_info->getType())
    {
        case tango::typeCharacter:
        case tango::typeWideCharacter:
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

        case tango::typeInteger:
            // fill out update_info for ICacheRowUpdate below
            update_info.int_val = evt.GetInt();

            str = wxString::Format(wxT("%s=%d"), quoted_col_name.c_str(),
                                                 evt.GetInt());
            break;
            
        case tango::typeDouble:
        case tango::typeNumeric:
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
        
        case tango::typeBoolean:
            // fill out update_info for ICacheRowUpdate below
            update_info.bool_val = evt.GetBoolean();

            if (getDbDriver() == wxT("xdnative"))
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
            
        case tango::typeDateTime:
        case tango::typeDate:
        {
            int y, m, d, hh, mm, ss;
            bool valid = cfw::Locale::parseDateTime(evt.GetString(),
                                                    &y, &m, &d,
                                                    &hh, &mm, &ss);

            if (!valid)
            {
                if (evt.GetUserEvent())
                {
                    cfw::appMessageBox(_("The date entered was not formatted correctly."),
                                       APPLICATION_NAME,
                                       wxOK | wxICON_EXCLAMATION | wxCENTER);

                    m_grid->SetFocus();
                }

                evt.Veto();
                return;
            }

            if (hh == -1 || mm == -1)
            {
                update_info.date_val = tango::DateTime(y, m, d);
                
                
                if (getDbDriver() == wxT("xdoracle"))
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
                update_info.date_val = tango::DateTime(y, m, d, hh, mm, ss);
                
                if (getDbDriver() == wxT("xdoracle"))
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


    wxString cmd = wxT("UPDATE ");
    cmd += left_quote_identifier;
    cmd += towx(getBaseSet()->getObjectPath());
    cmd += right_quote_identifier;
    cmd += wxT(" SET ");
    cmd += str;
    cmd += where_str;
            
    xcm::IObjectPtr result_obj;
    if (g_app->getDatabase()->execute(towstr(cmd),
                                      tango::sqlPassThrough,
                                      result_obj,
                                      NULL))
    {
        // some iterators are forward-only and cache the data, so that
        // the grid view can be scrolled bidirectionally.  If this is the
        // case, update the iterators cache.
        tango::ICacheRowUpdatePtr update = m_iter;
        tango::objhandle_t handle = getTemporaryHandle(col_name);
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
    if (getDbDriver() == wxT("xdnative"))
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

    tango::ISetPtr set = getBaseSet();
    tango::IRowInserterPtr inserter = set->getRowInserter();
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
        
    tango::IStructurePtr structure = m_iter->getStructure();

    tango::IColumnInfoPtr col;
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
        tango::IColumnInfoPtr col;
        tango::ISetPtr set = getBaseSet();

        structure = set->getStructure();
        col = structure->createColumn();

        col->setName(towstr(col_name));
        col->setType(type);
        col->setWidth(width);
        col->setScale(scale);
        col->setExpression(towstr(expr));

        if (!set->modifyStructure(structure, NULL))
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
    // update the display because the dynamic field we
    // just changed may play a role

    int synctype = g_app->getAppController()->getRelationshipSync();
    if (synctype != tabledocRelationshipSyncNone)
        g_app->getAppController()->updateTableDocRelationshipSync(synctype);
}


void TableDoc::onCreateDynamicFieldCancelled(ColPropsPanel* panel)
{
    freeTemporaryHandles();
    
    // we are deleting just dynamic fields
    wxString modify_struct = panel->getModifyField();
    tango::IStructurePtr structure = m_iter->getStructure();
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
        cfw::Event* evt = new cfw::Event(wxT("tabledoc.structureModified"));
        evt->s_param = m_dbpath;
        m_frame->postEvent(evt);
    }
}


void TableDoc::showCreateDynamicField()
{
    // see if any other dynamic field frames are visible
    cfw::IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("CalculatedFieldPropertiesPanel"));
    if (site.isOk())
    {
        if (!site->getVisible())
            site->setVisible(true);
        return;
    }

    m_grid->clearSelection();

    tango::IStructurePtr structure = getBaseSet()->getStructure();
    
    int i = 0;
    wxString column_name;
    do
    {
        i++;
        column_name = wxString::Format(wxT("Field%d"), i);
    } while (structure->getColumnExist(towstr(column_name)));


    if (createDynamicField(column_name,
                           tango::typeCharacter,
                           30, 0, wxT("\"\""), false))
    {
        // insert column at cursor position
        insertColumnInternal(m_grid->getCursorColumn(), column_name, false);

        ColPropsPanel* panel = new ColPropsPanel;

        panel->sigOkPressed.connect(this,
                                &TableDoc::onCreateDynamicFieldOk);

        panel->sigCancelPressed.connect(this,
                                &TableDoc::onCreateDynamicFieldCancelled);

        panel->setModifyField(column_name);

        site = m_frame->createSite(panel, cfw::sitetypeModeless,
                                   -1, -1, 560, 310);
        site->setMinSize(560,310);
        site->setName(wxT("CalculatedFieldPropertiesPanel"));
    }
}

void TableDoc::showEditDynamicField(const wxString& column_name)
{
    cfw::IDocumentSitePtr site;
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
                                   cfw::sitetypeModeless,
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

wxString TableDoc::findDynamicFieldByExpr(const wxString& expr)
{
    tango::IStructurePtr structure = m_iter->getStructure();

    int col_count = structure->getColumnCount();
    int i;

    for (i = 0; i < col_count; ++i)
    {
        tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);

        if (expr == towx(colinfo->getExpression()))
            return towx(colinfo->getName());
    }
    
    return wxEmptyString;
}

void TableDoc::onCreateNewMark(wxCommandEvent& evt)
{
    createNewMark(wxT(""));

    // find the marks panel and tell it to repopulate its list
    cfw::IDocumentSitePtr markmanager_site;
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

void TableDoc::onMakePermanent(wxCommandEvent& evt)
{
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    // find out which columns are selected

    kcl::IModelPtr model = m_grid->getModel();
    ITangoGridModelPtr tmodel = model;

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
    
    tango::ISetPtr action_set = m_set;

    tango::IStructurePtr structure = action_set->getStructure();
    tango::IColumnInfoPtr colinfo;
           
    std::set<wxString>::iterator it;
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        colinfo = structure->getColumnInfo(towstr(*it));

        if (colinfo.isNull())
        {
            cfw::appMessageBox(_("One or more of the selected calculated fields does not exist."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        if (!colinfo->getCalculated())
        {
            cfw::appMessageBox(_("This operation may only be performed on calculated fields."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            m_grid->clearSelection();
            m_grid->refresh(kcl::Grid::refreshAll);
            return;
        }
    }

    closeSet();

    // create a job and send it to the queue

    wxString title = wxString::Format(_("Modifying Structure of '%s'"),
                                      getCaption().c_str());

    ModifyStructJob* job = new ModifyStructJob;
    job->getJobInfo()->setTitle(title);
    
    for (it = cols.begin(); it != cols.end(); ++it)
    {
        colinfo = structure->modifyColumn(towstr(*it));
        colinfo->setCalculated(false);
    }

    job->setInstructions(action_set, structure);

    connectModifyStructJob(job);

    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
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
    
    tango::ISetPtr set;
    tango::IStructurePtr structure;
    
    if (getDbDriver() == wxT("xdnative"))
    {
        set = getBrowseSet();
        if (set.isOk())
            structure = set->getStructure();
    }
     else
    {
        // external databases get their structure from iterators;
        // eventually we'll move over to this for xdnative iterators, too
        
        if (m_iter)
        {
            structure = m_iter->getStructure();
        }
    }
    
    if (structure.isNull())
        return;
    
    int i, col_count = structure->getColumnCount();
    list.reserve(col_count);
    
    for (i = 0; i < col_count; i++)
    {
        tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);
        
        bool in_view = false;
        int model_idx = m_grid->getColumnModelIdxByName(towx(colinfo->getName()));
        if (model_idx >= 0 && m_grid->getColumnViewIdx(model_idx) != -1)
        {
            in_view = true;
        }
        
        ColumnListItem item;
        item.text = cfw::makeProperIfNecessary(towx(colinfo->getName()));
        if (colinfo->getCalculated())
        {
            item.bitmap = in_view ? GETBMP(gf_lightning_16) : GETDISBMP(gf_lightning_16);
        }
         else
        {
            item.bitmap = in_view ? GETBMP(gf_field_16) : GETDISBMP(gf_field_16);
        }
        item.active = true;
        list.push_back(item);
    }
    
    
    if (set.isOk())
    {
    
        // -- add fields from child file(s) --

        tango::ISetPtr right_set;
        tango::IStructurePtr right_structure;

        tango::IRelationEnumPtr rel_enum = set->getRelationEnum();
        tango::IRelationPtr rel;
        size_t r, rel_count = rel_enum->size();
        
        wxString s;

        for (r = 0; r < rel_count; ++r)
        {
            rel = rel_enum->getItem(r);

            if (rel.isNull())
                continue;

            right_set = rel->getRightSetPtr();
            if (right_set.isNull())
                continue;

            right_structure = right_set->getStructure();

            int i, col_count;
            col_count = right_structure->getColumnCount();

            for (i = 0; i < col_count; ++i)
            {
                tango::IColumnInfoPtr colinfo;
                colinfo = right_structure->getColumnInfoByIdx(i);

                s = wxString::Format(wxT("%s.%s"),
                           cfw::makeProperIfNecessary(towx(rel->getTag())).c_str(),
                           cfw::makeProperIfNecessary(towx(colinfo->getName())).c_str());
                
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


    // check for read-only flag; if it's set, we're done
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;


    // check for the model
    std::set<tango::rowid_t> rows;
    ITangoGridModelPtr model;

    model = m_grid->getModel();
    if (model.isNull())
        return;

    // get the selection count and make sure we only have rows
    kcl::SelectionRect sel;
    unsigned int selection_count = m_grid->getSelectionCount();

    int row;
    for (unsigned int i = 0; i < selection_count; ++i)
    {
        // if we don't have a row, we're done
        m_grid->getSelection(i, &sel);
        if (sel.m_start_col != 0 || sel.m_end_col != m_grid->getColumnCount()-1)
            return;

        for (row = sel.m_start_row; row <= sel.m_end_row; ++row)
        {
            rows.insert(model->getRowId(row));
        }
    }


    // ok, now actually do the deletion
    AppBusyCursor bc;

    tango::ISetPtr base_set = getBaseSet();
    tango::ISetPtr browse_set = getBrowseSet();
    
    tango::IRowDeleterPtr base_set_deleter;
    tango::IRowDeleterPtr browse_set_deleter;
    
    if (base_set.isOk())
    {
        base_set_deleter = base_set->getRowDeleter();
        if (base_set_deleter.isNull())
            return;
    }
    
    if (browse_set.isOk() && base_set != browse_set)
    {
        browse_set_deleter = browse_set->getRowDeleter();
        if (browse_set_deleter.isNull())
            return;
    }


    // show warning
    wxString message = wxString::Format(_("Performing this operation will permanently delete data.  Are you sure\nyou want to delete %d record(s)?"),
                                        rows.size());

    int res = wxMessageBox(message,
                           APPLICATION_NAME,
                           wxYES_NO |
                           wxICON_EXCLAMATION |
                           wxCENTER);
    if (res != wxYES)
        return;

    // delete the records from the base set
    if (base_set_deleter)
    {
        base_set_deleter->startDelete();

        std::set<tango::rowid_t>::iterator it;
        for (it = rows.begin(); it != rows.end(); ++it)
            base_set_deleter->deleteRow(*it);

        base_set_deleter->finishDelete();
        base_set_deleter.clear();
    }


    // delete the records from the browse set,
    // if it is different from the base set
    if (browse_set_deleter)
    {
        browse_set_deleter->startDelete();

        std::set<tango::rowid_t>::iterator it;
        for (it = rows.begin(); it != rows.end(); ++it)
        {
            browse_set_deleter->deleteRow(*it);
        }

        browse_set_deleter->finishDelete();
        browse_set_deleter.clear();
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
    tango::IStructurePtr structure = m_iter->getSet()->getStructure();
    tango::IColumnInfoPtr colinfo;

    std::set<wxString> permcols;
    std::set<wxString> cols;
    std::set<wxString>::iterator it;

    int col_count = m_grid->getColumnCount();
    int i;

    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
        {
            colinfo = structure->getColumnInfo(towstr(m_grid->getColumnCaption(i)));
            if (!colinfo)
                return;

            cols.insert(towx(colinfo->getName()));

            if (!colinfo->getCalculated())
                permcols.insert(towx(colinfo->getName()));
        }
    }

    // make sure the user is not deleting _every_ field in the file
    int total_phys_fields = 0;
    col_count = structure->getColumnCount();

    for (i = 0; i < col_count; ++i)
    {
        colinfo = structure->getColumnInfoByIdx(i);
        if (!colinfo->getCalculated())
        {
            total_phys_fields++;
        }
    }

    // if we're trying to delete all fields, don't do anything
    if (permcols.size() >= (size_t)total_phys_fields)
        return;

    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    // show warning
    wxString message = _("Performing this operation will permanently delete data.  Are you sure\nyou want to delete the following field(s):");
    message += wxT("\n");

    for (it = cols.begin(); it != cols.end(); ++it)
    {
        message += wxT("\n\t");
        message += cfw::makeProperIfNecessary(*it);
    }
    
    int res = wxMessageBox(message,
                           APPLICATION_NAME,
                           wxYES_NO |
                           wxICON_EXCLAMATION |
                           wxCENTER);

    if (res != wxYES)
        return;

    // ok, now actually do the deletion
    m_grid->clearSelection();

    for (it = cols.begin(); it != cols.end(); ++it)
    {
        structure->deleteColumn(towstr(*it));
    }

    if (permcols.size() > 0)
    {
        m_grid->clearSelection();
        m_grid->refresh(kcl::Grid::refreshAll);


        // submit a non-blocking job to the queue, because physical
        // structure changes can take more time

        wxString title = wxString::Format(_("Modifying Structure of '%s'"),
                                          getCaption().c_str());

        ModifyStructJob* job = new ModifyStructJob;
        job->getJobInfo()->setTitle(title);
        job->setInstructions(m_set, structure);
        connectModifyStructJob(job);
        closeSet();

        g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);

        return;
    }

    freeTemporaryHandles();


    // we are deleting just dynamic fields
    if (m_iter->getSet()->modifyStructure(structure, NULL))
    {
        m_iter->refreshStructure();
        m_grid->refreshModel();

        // remove all columns from view which are not contained in the model
        int idx;
        for (it = cols.begin(); it != cols.end(); ++it)
        {
            while (1)
            {
                idx = m_grid->getColumnViewIdxByName(*it);
                if (idx == -1)
                {
                    break;
                }

                hideColumn(idx);
            }
        }

        // let other windows know that the structure was modified
        cfw::Event* evt = new cfw::Event(wxT("tabledoc.structureModified"));
        evt->s_param = m_dbpath;
        m_frame->postEvent(evt);
    }
     else
    {
        // show error message
    }

    m_grid->clearSelection();
    m_grid->refresh(kcl::Grid::refreshAll);
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
    tango::ISetPtr set = getBaseSet();
    if (set.isNull())
        return;

    set->deleteAllRelations();
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

wxString TableDoc::getDbDriver()
{
    tango::ISetPtr browse_set = getBrowseSet();
    if (browse_set.isNull())
        return wxT("");
        
    xcm::class_info* class_info = xcm::get_class_info(browse_set.p);
    
    return towx(class_info->get_name()).BeforeFirst('.');
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
        cfw::appMessageBox(_("Only a single cell may be cut."),
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




// -- Field Summary Implementation --


static void onSummaryJobFinished(cfw::IJobPtr job)
{
    // pivot output table

    IGroupJobPtr group_job = job;
    if (!group_job)
    {
        cfw::appMessageBox(_("An output set could not be created."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return;
    }

    
    tango::ISetPtr results = group_job->getResultSet();
    if (!results)
        return;

    int max_scale = job->getExtraLong();
    
    tango::IStructurePtr output_structure = g_app->getDatabase()->createStructure();
    tango::IColumnInfoPtr colinfo;

    const wchar_t* fields[] = { L"Field",
                                L"Minimum",
                                L"Maximum",
                                L"Sum_Amount",
                                L"Average",
                                L"Min_Length",
                                L"Max_Length",
                                L"Empty_Count",
                                L"Total_Count",
                                0 };

    tango::objhandle_t field_handles[255];

    int i;
    
    i = 0;
    while (fields[i])
    {
        colinfo = output_structure->createColumn();
        colinfo->setName(fields[i]);
        colinfo->setType(tango::typeWideCharacter);
        colinfo->setWidth(255);
        colinfo->setScale(0);

        if (0 == wcscasecmp(fields[i], L"Sum_Amount") ||
            0 == wcscasecmp(fields[i], L"Average"))
        {
            colinfo->setType(tango::typeNumeric);
            colinfo->setWidth(15);
            colinfo->setScale(max_scale);
        }
        
        if (0 == wcscasecmp(fields[i], L"Min_Length") ||
            0 == wcscasecmp(fields[i], L"Max_Length") ||
            0 == wcscasecmp(fields[i], L"Total_Count") ||
            0 == wcscasecmp(fields[i], L"Empty_Count"))
        {
            colinfo->setType(tango::typeNumeric);
            colinfo->setWidth(12);
        }

        ++i;
    }


    tango::ISetPtr output = g_app->getDatabase()->createSet(L"",
                                                     output_structure,
                                                     NULL);
    if (!output)
        return;

    tango::IRowInserterPtr output_inserter = output->getRowInserter();
    if (!output_inserter)
        return;

    i = 0;
    while (fields[i])
    {
        field_handles[i] = output_inserter->getHandle(fields[i]);
        i++;
    }


    tango::IStructurePtr result_structure = results->getStructure();
    tango::IIteratorPtr result_iter = results->createIterator(L"", L"", NULL);
    
    if (result_iter.isNull())
        return;

    result_iter->goFirst();
    if (result_iter->eof())
        return;

    wxString last_field_name = wxT("");
    wxString field_name;
    wxString col_name;
    
    output_inserter->startInsert(L"*");


    tango::objhandle_t total_count_handle = result_iter->getHandle(L"total_count");
    double total_count = result_iter->getDouble(total_count_handle);
    result_iter->releaseHandle(total_count_handle);


    int result_field_count = result_structure->getColumnCount();
    for (i = 0; i < result_field_count; ++i)
    {
        colinfo = result_structure->getColumnInfoByIdx(i);

        col_name = towx(colinfo->getName());
        col_name.MakeUpper();
        
        int idx = col_name.Find(wxT("_0RESULT0_"));
        if (idx == -1)
        {
            continue;
        }

        wxString type = col_name.Mid(idx+10);
        type.MakeUpper();

        field_name = col_name.Left(idx);


        tango::objhandle_t h = 0;

        if (type == wxT("MIN"))
            h = field_handles[1];
         else if (type == wxT("MAX"))
            h = field_handles[2];
         else if (type == wxT("SUM"))
            h = field_handles[3];
         else if (type == wxT("AVG"))
            h = field_handles[4];
         else if (type == wxT("MINLENGTH"))
            h = field_handles[5];
         else if (type == wxT("MAXLENGTH"))
            h = field_handles[6];
         else if (type == wxT("EMPTY"))
            h = field_handles[7];

        if (!h)
        {
            continue;
        }


        if (last_field_name != field_name)
        {
            if (i > 0)
            {
                output_inserter->insertRow();
            }
            
            last_field_name = field_name;

            output_inserter->putWideString(field_handles[0],
                                         towstr(cfw::makeProperIfNecessary(field_name)));

            // total count
            output_inserter->putDouble(field_handles[8], total_count);
        }


        tango::objhandle_t result_col_handle = result_iter->getHandle(towstr(col_name));
        tango::IColumnInfoPtr result_col_info = result_iter->getInfo(result_col_handle);
    
        // empty count    
        if (h == field_handles[7])
        {
            output_inserter->putDouble(h, result_iter->getDouble(result_col_handle));
            continue;
        }

        switch (result_col_info->getType())
        {
            case tango::typeWideCharacter:
            case tango::typeCharacter:
                output_inserter->putWideString(h, result_iter->getWideString(result_col_handle));
                break;

            case tango::typeDate:
            {
                tango::DateTime dt = result_iter->getDateTime(result_col_handle);
                wxString s;

                if (!dt.isNull())
                {
                    s = cfw::Locale::formatDate(dt.getYear(), dt.getMonth(), dt.getDay());
                }

                output_inserter->putWideString(h, towstr(s));
            }
            break;

            case tango::typeDateTime:
            {
                tango::DateTime dt = result_iter->getDateTime(result_col_handle);
                wxString s;

                if (!dt.isNull())
                {
                    s = cfw::Locale::formatDate(dt.getYear(), dt.getMonth(), dt.getDay(), dt.getHour(), dt.getMinute(), dt.getSecond());
                }

                output_inserter->putWideString(h, towstr(s));
            }
            break;


            case tango::typeDouble:
            case tango::typeNumeric:
            case tango::typeInteger:
            {
                if (type == wxT("SUM") || type == wxT("AVG"))
                {
                    output_inserter->putDouble(h, result_iter->getDouble(result_col_handle));
                }
                 else if (type == wxT("MINLENGTH") || type == wxT("MAXLENGTH"))
                {
                    output_inserter->putInteger(h, result_iter->getInteger(result_col_handle));
                }
                 else
                {
                    wchar_t buf[255];
                    swprintf(buf, 255, L"%.*f", result_col_info->getScale(), result_iter->getDouble(result_col_handle));
                    output_inserter->putWideString(h, buf);
                }
            }
            break;

            case tango::typeBoolean:
            {
                output_inserter->putWideString(h, result_iter->getBoolean(result_col_handle) ? L"TRUE" : L"FALSE");
            }
            break;
        }
    }
    
    if (i > 0)
    {
        output_inserter->insertRow();
    }

    output_inserter->finishInsert();

    TableDoc* doc = new TableDoc;
    doc->setBaseSet(output, xcm::null);
    g_app->getMainFrame()->createSite(doc, cfw::sitetypeNormal,
                                      -1, -1, -1, -1);
}


void TableDoc::onSummary(wxCommandEvent& evt)
{
    tango::IStructurePtr structure = m_browse_set->getStructure();

    std::vector<wxString> summary_columns;
    std::vector<wxString> output_columns;
    std::vector<wxString>::iterator it;

    wxString outcol;

    // find out which columns are selected

    int i, col_count = m_grid->getColumnCount();

    for (i = 0; i < col_count; ++i)
    {
        if (m_grid->isColumnSelected(i))
        {
            summary_columns.push_back(m_grid->getColumnCaption(i));
        }
    }

    // if there was no selection, summarize all columns

    if (summary_columns.size() == 0)
    {
        int col_count = structure->getColumnCount();
        int i;

        for (i = 0; i < col_count; ++i)
        {
            summary_columns.push_back(towx(structure->getColumnName(i)));
        }
    }


    // create a Grouping job and submit it to the job queue

    wxString title = wxString::Format(_("Summarizing '%s'"),
                                      getCaption().c_str());

    GroupJob* job = new GroupJob;
    job->getJobInfo()->setTitle(title);

    wxString group_funcs;


    // record the max scale used by all fields -- this will be
    // used in the sum and average columns in the output file
    int max_scale = 0;
    
    // create the group functions string
    tango::IColumnInfoPtr colinfo;
    bool last;
    for (it = summary_columns.begin(); it != summary_columns.end(); ++it)
    {
        last = false;
        if ((it+1) == summary_columns.end())
            last = true;

        colinfo = structure->getColumnInfo(towstr(*it));

        if (colinfo.isNull())
            continue;

        if (colinfo->getType() == tango::typeCharacter ||
            colinfo->getType() == tango::typeWideCharacter)
        {
            outcol.Printf(wxT("%s_0result0_minlength=min(length(%s)),"), it->c_str(), it->c_str());
            group_funcs += outcol;

            outcol.Printf(wxT("%s_0result0_maxlength=max(length(%s)),"), it->c_str(), it->c_str());
            group_funcs += outcol;        
        }

        if (colinfo->getType() == tango::typeInteger ||
            colinfo->getType() == tango::typeDouble ||
            colinfo->getType() == tango::typeNumeric)
        {
            int scale = colinfo->getScale();
            if (scale > max_scale)
                max_scale = scale;
            
            outcol.Printf(wxT("%s_0result0_sum=sum(%s),"), it->c_str(), it->c_str());
            group_funcs += outcol;

            outcol.Printf(wxT("%s_0result0_avg=avg(%s),"), it->c_str(), it->c_str());
            group_funcs += outcol;
        }

        outcol.Printf(wxT("%s_0result0_min=min(%s),"), it->c_str(), it->c_str());
        group_funcs += outcol;

        outcol.Printf(wxT("%s_0result0_max=max(%s),"), it->c_str(), it->c_str());
        group_funcs += outcol;

        outcol.Printf(wxT("%s_0result0_empty=count(empty(%s)),"), it->c_str(), it->c_str());
        group_funcs += outcol;
    }


    group_funcs += wxT("total_count=count()");

    job->setInstructions(getBrowseSet(),
                         wxT(""),
                         group_funcs,
                         wxT(""),
                         wxT(""));

    job->setExtraLong(max_scale);
    job->sigJobFinished().connect(&onSummaryJobFinished);

    // -- add and start job --
    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
}



// -- Find Next Row Implementation --


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
        cfw::resizeStaticText(text);

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

    tango::objhandle_t handle = m_iter->getHandle(towstr(search));
    if (handle != 0 && m_iter->getType(handle) == tango::typeBoolean)
    {
        return search;
    }


    // otherwise, create an expression which will search for
    // the search string with the appropriate options.  We
    // will first check for any selected columns that will
    // limit the scope of our search

    tango::IStructurePtr iter_struct = m_iter->getStructure();


    std::vector<tango::IColumnInfoPtr> search_cols;

    tango::IColumnInfoPtr colinfo;
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
    is_date = cfw::Locale::parseDateTime(search,
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

    std::vector<tango::IColumnInfoPtr>::iterator it;
    for (it = search_cols.begin(); it != search_cols.end(); ++it)
    {
        wxString piece, left, right;
        wxString colname = quoteIdentifier(g_app->getDatabase(), towx((*it)->getName()));        
        
        switch ((*it)->getType())
        {
            default:
            case tango::typeBoolean:
                continue;

            case tango::typeCharacter:
            case tango::typeWideCharacter:
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

            case tango::typeNumeric:
            case tango::typeDouble:
            case tango::typeInteger:
                left = colname;
                break;

            // for dates, if the user is searching for a date with a
            // string we can parse as a date, put the date into a common 
            // format, allowing the user to use different date formats
            // when searching; if we can't parse it as a date, search
            // on the actual text they entered
            case tango::typeDate:
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
    
            case tango::typeDateTime:
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
            right = wxT("\'") + cfw::doubleQuote(right, L'\'') + wxT("\'");
            piece.Printf(wxT("%s = %s"), left.c_str(), right.c_str());
        }
         else
        {
            right = wxT("\'%") + cfw::doubleQuote(right, L'\'') + wxT("%\'");
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
    if (m_set->getRowCount() > 0 && m_grid->getRowCount() > 0)
    {
        tango::tango_int64_t row_count = (tango::tango_int64_t)m_stat_row_count;
        
        wxString message = wxString::Format(_("Record number (1 - %s):"),
                cfw::dbl2fstr(row_count).c_str());
        wxTextEntryDialog dlg(this, message, _("Go To Record"),
                cfw::dbl2fstr(m_grid->getCursorRow()+1));
        dlg.SetSize(260,141);
        
        if (dlg.ShowModal() == wxID_OK)
        {
            tango::tango_int64_t goto_row = wxAtof(dlg.GetValue());
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
    is_date = cfw::Locale::parseDateTime(search,
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
                    bool b = cfw::Locale::parseDateTime(value,
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
    // set the source
    tango::ISetPtr set = getBaseSet();
    if (set.isOk())
    {
        data.path = towx(set->getObjectPath());
    }

    // set the font facename and size being used by the grid
    data.font_facename = m_font.GetFaceName();
    data.font_size = m_font.GetPointSize();

    // set the data fields
    int i;
    int col_count = m_grid->getColumnCount();
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
            field.caption = cfw::makeProperIfNecessary(field.field_name);
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

bool TableDoc::findNextMatch(const wxString& _expr,
                             bool forward,
                             bool match_case,
                             bool whole_cell)
{
    // make sure the eof is known
    ITangoGridModelPtr tango_grid_model = m_grid_model;
    if (!tango_grid_model->isEofKnown())
    {
        tango_grid_model->discoverEof();
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

    tango::IIteratorPtr sp_iter = m_iter->clone();
    if (sp_iter.isNull())
    {
        return false;
    }

    tango::IIterator* iter = sp_iter.p;

    if (iter->eof())
    {
        return false;
    }

    tango::objhandle_t handle = iter->getHandle(towstr(expr));

    if (handle == 0 || iter->getType(handle) != tango::typeBoolean)
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
            cfw::appMessageBox(_("The beginning of the data has been reached."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
        }
         else
        {
            cfw::appMessageBox(_("The end of the data has been reached."),
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

static void onSetOrderExprEditFinished(KeyBuilderPanel* builder)
{
    tango::IIteratorPtr iter = builder->getIterator();
    wxString expr = builder->getExpression();


    // close panel site

    cfw::IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("SortPanel"));
    if (site.isOk())
    {
        g_app->getMainFrame()->closeSite(site);
    }


    // set the order on the appropriate table doc

    cfw::IDocumentSiteEnumPtr docsites;
    ITableDocPtr table_doc;

    docsites = g_app->getMainFrame()->getDocumentSites(cfw::sitetypeNormal);

    int site_count = docsites->size();
    for (int i = 0; i < site_count; ++i)
    {
        site = docsites->getItem(i);
        table_doc = site->getDocument();
        if (table_doc.isOk())
        {
            if (table_doc->getIterator() == iter)
            {
                table_doc->setSortOrder(expr);
            }
        }
    }
}


void TableDoc::onSetOrder(wxCommandEvent& evt)
{
    cfw::IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("SortPanel"));
    if (site.isNull())
    {
        KeyBuilderDocPanel* panel = new KeyBuilderDocPanel;
        panel->setOKText(_("Run"));
        panel->setOverlayText(_("Select fields from the list on the left and\ndrag them here to define the table sort order"));
        site = m_frame->createSite(panel, cfw::sitetypeModeless,
                                   -1, -1, 600, 360);
        site->setMinSize(520,300);
        site->setName(wxT("SortPanel"));
        panel->setIterator(m_iter);

        // set title
        tango::ISetPtr browse_set = getBrowseSet();
        if (!browse_set)
            return;

        wxString caption = _("Sort");
        if (!browse_set->isTemporary())
        {
            caption += wxT(" - [");
            caption += towx(browse_set->getObjectPath());
            caption += wxT("]");
        }

        site->setCaption(caption);

        // set the fields in the sort expression builder based on the 
        // selected fields; note: we used to set the order based on the 
        // current sort order, similar to filter; but then in this panel 
        // and the grouping panel, we added the ability to pre-populate 
        // the form based on the selected fields (consistent also with
        // copy fields and summarize, which operate on the selected columns)
        // however, this raised the issue in that if fields were selected,
        // the sort panel would show those fields, otherwise, it would show
        // the fields in the current sort order; in use, this dual behavior
        // seemed confusing, so we're populating the list based on the
        // selected columns, since the panel is more about setting a new 
        // order than a providing a status report of the current order
        wxString expr = buildSelectedColumnExpression(m_grid);
        panel->setExpression(expr);

        panel->sigOkPressed.connect(&onSetOrderExprEditFinished);
        panel->setIterator(m_iter);
    }
     else
    {
        if (!site->getVisible())
            site->setVisible(true);
    }
}


void TableDoc::onRemoveOrder(wxCommandEvent& evt)
{
    setSortOrder(wxT(""));
}



void TableDoc::onSetOrderAscending(wxCommandEvent& evt)
{
    wxString col_caption;
    //tango::objhandle_t data_handle;

    // create order string
    wxString expr = buildSelectedColumnExpression(m_grid);

    // if there was no selection, use the cursor column

    if (expr.Length() == 0)
    {
        kcl::IModelPtr model = m_grid->getModel();
        kcl::IModelColumnPtr model_colinfo;
        wxString col_name;
        tango::IStructurePtr structure = m_iter->getStructure();
        int model_idx = m_grid->getColumnModelIdx(m_grid->getCursorColumn());
        if (model_idx == -1)
        {
            return;
        }
        model_colinfo = model->getColumnInfo(model_idx);
        col_name = model_colinfo->getName();

        expr = quoteIdentifier(g_app->getDatabase(), col_name);
    }


    // --------------------
    g_macro << "";
    g_macro << "// set sort order (ascending)";
    g_macro << wxString::Format(wxT("auto.activeDocument.setSortOrder(\"%s\");"),
                                wxcstr(jsEscapeString(expr, '"')));
    g_macro << "auto.waitForRunningJobs();";
    // --------------------


    setSortOrder(expr);
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
        tango::IStructurePtr structure = m_iter->getStructure();
        int model_idx = m_grid->getColumnModelIdx(m_grid->getCursorColumn());
        if (model_idx == -1)
            return;
            
        model_colinfo = model->getColumnInfo(model_idx);
        col_name = model_colinfo->getName();

        expr = quoteIdentifier(g_app->getDatabase(), col_name);
        expr += wxT(" DESC");
    }


    // --------------------
    g_macro << "";
    g_macro << "// set sort order (descending)";
    g_macro << wxString::Format(wxT("auto.activeDocument.setSortOrder(\"%s\");"),
                                wxcstr(jsEscapeString(expr, '"')));
    // --------------------

    setSortOrder(expr);
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
        tango::IStructurePtr structure = m_iter->getStructure();
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

    setGroupBreak(expr);
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

void onCopyRecordsJobFinished(cfw::IJobPtr job)
{
    if (job->getJobInfo()->getState() != cfw::jobStateFinished)
        return;

    // open the result set in a new child window

    ICopyJobPtr copy_job = job;

    size_t copy_count = copy_job->getInstructionCount();
    size_t i;

    for (i = 0; i < copy_count; ++i)
    {
        TableDoc* doc = new TableDoc;
        doc->setBaseSet(copy_job->getResultSet(i), xcm::null);

        g_app->getMainFrame()->createSite(doc, cfw::sitetypeNormal,
                                          -1, -1, -1, -1);
    }
    
    g_app->getAppController()->refreshDbDoc();
}

void TableDoc::onCopyRecordsOk(ExprBuilderPanel* expr_panel)
{
    // close panel site
    cfw::IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("CopyPanel"));
    if (site.isOk())
    {
        g_app->getMainFrame()->closeSite(site);
    }

    copyRecords(expr_panel->getExpression());
}




void TableDoc::copyRecords(const wxString& condition)
{
    flushActiveView();

    // try to clone the iterator which is being browsed.
    // this may or may not work based no the database type.
    // If it doesn't work, we'll have to requery.  For xdnative
    // and xdfs databases, this will always work
    tango::IIteratorPtr source_iter = m_iter->clone();
    if (source_iter.isOk())
    {
        source_iter->goFirst();
    }

    // find out which columns we should copy
    wxString columns = buildSelectedColumnExpression(m_grid);

    // create a query job
    wxString title = wxString::Format(_("Copying Records from '%s'"),
                                      getCaption().c_str());

    CopyJob* copy_job = new CopyJob;
    copy_job->getJobInfo()->setTitle(title);
    
    if (source_iter)
    {
        copy_job->addCopyInstruction(source_iter,
                                     condition,
                                     columns,
                                     g_app->getDatabase(),
                                     wxEmptyString);
    }
     else
    {
        wxString final_condition = getFilter();
        if (final_condition.Length() == 0)
        {
            final_condition = condition;
        }
         else
        {
            final_condition.Prepend(wxT("("));
            final_condition += wxT(") AND (");
            final_condition += condition;
            final_condition += wxT(")");
        }
    
    
        // this type of copy must requery, because no clone() is available
        copy_job->addCopyInstruction(g_app->getDatabase(),
                                     getBaseSet(),
                                     columns,
                                     final_condition,
                                     getSortOrder(),
                                     g_app->getDatabase(),
                                     wxEmptyString);
    }
    
    copy_job->sigJobFinished().connect(&onCopyRecordsJobFinished);

    g_app->getJobQueue()->addJob(copy_job, cfw::jobStateRunning);
}



void TableDoc::onCopyRecords(wxCommandEvent& evt)
{
    cfw::IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("CopyPanel"));
    if (site.isNull())
    {
        {
            AppBusyCursor bc;

            ExprBuilderDocPanel* panel = new ExprBuilderDocPanel;
            panel->setValidationEnabled(m_db_type == tango::dbtypeXdnative ? true : false);
            panel->setOKText(_("Run"));
            site = m_frame->createSite(panel,
                                       cfw::sitetypeModeless |
                                       cfw::siteHidden,
                                       -1, -1, 560, 310);
            site->setMinSize(560,310);
            site->setCaption(makeCaption(_("Copy")));
            site->setName(wxT("CopyPanel"));

            panel->sigOkPressed.connect(this, &TableDoc::onCopyRecordsOk);
            panel->setTypeOnly(tango::typeBoolean);
            panel->setOKText(_("Run"));
            panel->setEmptyOk(true);
            panel->setIterator(m_iter);
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

    cfw::IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("AppendPanel"));
    if (site.isNull())
    {
        MergePanel* panel = new MergePanel;
        panel->setAppend(towx(getBaseSet()->getObjectPath()));
        site = g_app->getMainFrame()->createSite(panel, 
                                                 cfw::sitetypeModeless,
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
    wxString expr = expr_panel->getExpression();
    expr.Trim(true);
    if (expr.IsEmpty())
    {
        // if the expression is empty, remove the filter
        removeFilter();
        return;
    }

    // close panel site
    cfw::IDocumentSitePtr site;
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
    if (m_filter.Length() > 0)
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
        cfw::IJobPtr job = g_app->getJobQueue()->lookupJob(m_quick_filter_jobid);
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
    setQuickFilter(val);

    // running a quick filter needs to add the value
    // to the find/filter combo control dropdown
    cfw::Event* cfw_evt = new cfw::Event(wxT("appmain.addFindComboItem"));
    cfw_evt->s_param = val;
    g_app->getMainFrame()->postEvent(cfw_evt);
}


void TableDoc::onFilter(wxCommandEvent& evt)
{
    cfw::IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("FilterPanel"));
    if (site.isNull())
    {
        {
            AppBusyCursor bc;

            ExprBuilderDocPanel* panel = new ExprBuilderDocPanel;
            panel->setOKText(_("Run"));
            panel->setValidationEnabled(m_db_type == tango::dbtypeXdnative ? true : false);
            site = m_frame->createSite(panel,
                                       cfw::sitetypeModeless |
                                       cfw::siteHidden,
                                       -1, -1, 560, 310);
            site->setMinSize(560,310);
            site->setCaption(makeCaption(_("Filter")));
            site->setName(wxT("FilterPanel"));

            panel->setIterator(m_iter);
            panel->setExpression(m_filter);
            panel->setOKText(_("Run"));
            panel->sigOkPressed.connect(this, &TableDoc::onFilterOk);
            panel->setTypeOnly(tango::typeBoolean);
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


// -- Delete Rows Implementation --


void TableDoc::onDeleteRecordsOk(ExprBuilderPanel* expr_panel)
{
    wxString expr = expr_panel->getExpression();
    expr.Trim(TRUE);
    if (expr.IsEmpty())
    {
        return;
    }

    // close panel site
    cfw::IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("DeletePanel"));
    if (site.isOk())
    {
        g_app->getMainFrame()->closeSite(site);
    }

    deleteRecords(expr);
}


void TableDoc::deleteRecords(const wxString& condition)
{
    wxString title = wxString::Format(_("Deleting records in '%s'"),
                                      getCaption().c_str());

    QueryJob* delete_job = new QueryJob;
    delete_job->getJobInfo()->setTitle(title);
    
    wxString cmd;
        
    tango::IDatabasePtr db = g_app->getDatabase();

    tango::IFileInfoPtr file_info = db->getFileInfo(getBaseSet()->getObjectPath());
    
    if (m_filter.Length() > 0)
    {
        cmd = wxT("DELETE FROM ");
        cmd += towx(getBaseSet()->getObjectPath());
        cmd += wxT(" WHERE ");
        cmd += wxT("(");
        cmd += m_filter;
        cmd += wxT(") and (");
        cmd += condition;
        cmd += wxT(");");
        
        cmd += wxT("DELETE FROM ");
        cmd += towx(getBrowseSet()->getObjectPath());
        cmd += wxT(" WHERE ");
        cmd += condition;
    }
     else
    {
        cmd += wxT("DELETE FROM ");
        cmd += towx(getBaseSet()->getObjectPath());
        cmd += wxT(" WHERE ");
        cmd += condition;
    }
    
    delete_job->setQuery(cmd, tango::sqlPassThrough);
    delete_job->sigJobFinished().connect(this, &TableDoc::onDeleteJobFinished);

    g_app->getJobQueue()->addJob(delete_job, cfw::jobStateRunning);
}


void TableDoc::onDeleteRecords(wxCommandEvent& evt)
{
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;

    cfw::IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("DeletePanel"));
    if (site.isNull())
    {
        {
            AppBusyCursor bc;

            ExprBuilderDocPanel* panel = new ExprBuilderDocPanel;
            panel->setOKText(_("Run"));
            panel->setValidationEnabled(m_db_type == tango::dbtypeXdnative ? true : false);
            site = m_frame->createSite(panel,
                                       cfw::sitetypeModeless |
                                       cfw::siteHidden,
                                       -1, -1, 560, 310);
            site->setMinSize(560,310);
            site->setCaption(makeCaption(_("Delete")));
            site->setName(wxT("DeletePanel"));

            panel->setIterator(m_iter);
            panel->sigOkPressed.connect(this, &TableDoc::onDeleteRecordsOk);
            panel->setOKText(_("Run"));
            panel->setTypeOnly(tango::typeBoolean);
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



// -- Replace Rows Implementation --

void TableDoc::showReplacePanel(const wxString& def_condition, const wxString& def_field)
{
    if (!g_app->getAppController()->doReadOnlyCheck())
    {
        return;
    }

    cfw::IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("UpdatePanel"));
    if (site.isNull())
    {
        {
            AppBusyCursor bc;

            ReplaceRowsPanel* panel = new ReplaceRowsPanel;

            site = m_frame->createSite(panel,
                                       cfw::sitetypeModeless |
                                       cfw::siteHidden,
                                       -1, -1, 560, 340);
            site->setMinSize(560,340);
            site->setName(wxT("UpdatePanel"));
            panel->setParams(getBrowseSet(), def_condition, def_field);
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
        if (structuredoc->getModifySet() == m_set)
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
            cfw::IDocumentSitePtr site;
            site = lookupOtherDocumentSite(m_doc_site, "appmain.StructureDoc");
            if (site.isOk())
                g_app->getMainFrame()->closeSite(site);
        }
    }

  
    {
        AppBusyCursor bc;

        StructureDoc* doc = new StructureDoc;
        doc->setModifySet(m_set);
        
        wxWindow* container = m_doc_site->getContainerWindow();

        cfw::IDocumentSitePtr site;
        site = g_app->getMainFrame()->createSite(container,
                                    static_cast<cfw::IDocument*>(doc),
                                    true);
        site->setVisible(true);
    }
}

void TableDoc::onUpdateRecords(wxCommandEvent& evt)
{
    wxString cursor_column = getCursorColumnName(m_grid);
    showReplacePanel(wxT(""), cursor_column);
}


// -- View Creation/Editing --

void TableDoc::onViewEditFinished(ViewPanel* panel)
{
    panel->updateTableDocViewEnum(this);
}

void TableDoc::showViewPanel()
{
    // flush the active view, if any changes were made
    flushActiveView();
    
    cfw::IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("ColumnViewsPanel"));

    if (site.isNull())
    {
        ViewPanel* panel = new ViewPanel(this);
        panel->sigOkPressed.connect(this, &TableDoc::onViewEditFinished);

        site = g_app->getMainFrame()->createSite(panel,
                                                 cfw::sitetypeModeless,
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

void TableDoc::onIndexEditFinished(IndexPanel* panel)
{
    // get the original indexes that exist on this set
    tango::IIndexInfoEnumPtr orig_indexes = m_set->getIndexEnum();
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
        tango::IIndexInfoPtr index = orig_indexes->getItem(i);
        wxString index_tag = towx(index->getTag());
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
            m_set->deleteIndex(index->getTag());
    }
    
    
    // we may have deleted some of the original indexes;
    // get the list of indexes again
    orig_indexes = m_set->getIndexEnum();
    count = (int)orig_indexes->size();
    
    // rename indexes
    for (i = count-1; i >= 0; --i)
    {
        tango::IIndexInfoPtr index = orig_indexes->getItem(i);
        wxString index_tag = towx(index->getTag());
        
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
                    getBaseSet()->renameIndex(towstr(index_tag),
                                              towstr(info->name));
                    
                    // we also need to update the IndexInfo structure
                    info->orig_name = info->name;
                    break;
                }
            }
        }
    }
    
    
    // now that we've deleting and renamed indexes, we need
    // to set up an index job to either reindex some of the original
    // indexes (if their expression has changed) or create new indexes
    // (if their tag name can't be found in the original list of indexes)
    
    
    // create an index job
    IndexJob* job = new IndexJob;
    
    
    // we may have deleted some of the original indexes;
    // get the list of indexes again
    orig_indexes = m_set->getIndexEnum();
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
            job->addInstruction(getBaseSet(), info->name, info->expr);
            continue;
        }
        
        tango::IIndexInfoPtr index;
        wxString index_tag, index_expr;
        
        // we may have deleted some of the original indexes;
        // get the list of indexes again
        if (index_deleted)
        {
            orig_indexes = m_set->getIndexEnum();
            count = (int)orig_indexes->size();
            index_deleted = false;
        }
        
        // try to find the original index
        for (i = 0; i < count; ++i)
        {
            index = orig_indexes->getItem(i);
            index_tag = towx(index->getTag());
            
            if (info->orig_name.CmpNoCase(index_tag) == 0)
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            // we couldn't find the orginal index; create a new index
            job->addInstruction(getBaseSet(), info->name, info->expr);
            continue;
        }
        
        // we found the index; there are a couple of options...
        
        // 1) the expression hasn't changed; do nothing
        index_expr = towx(index->getExpression());
        if (index_expr.CmpNoCase(info->expr) == 0)
            continue;
        
        // 2) the expression has changed; delete the old index
        //    and create a new one
        job->addInstruction(getBaseSet(), info->name, info->expr);
        if (index.isOk())
        {
            m_set->deleteIndex(index->getTag());
            index_deleted = true;
        }
    }
    
    // the index job has no instructions; we're done
    if (job->getInstructions().size() == 0)
    {
        delete job;
        return;
    }
    
    // the index job has some instructions; start the job
    g_app->getJobQueue()->addJob(job, cfw::jobStateRunning);
}

void TableDoc::showIndexPanel()
{
    cfw::IDocumentSitePtr site;
    site = g_app->getMainFrame()->lookupSite(wxT("IndexPanel"));
    
    if (site.isNull())
    {
        IndexPanel* panel = new IndexPanel(this);
        panel->sigOkPressed.connect(this, &TableDoc::onIndexEditFinished);
        
        site = g_app->getMainFrame()->createSite(panel,
                                                 cfw::sitetypeModeless,
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
    cfw::IDocumentSitePtr site;
    site = m_frame->lookupSite(wxT("GroupPanel"));
    if (site.isNull())
    {
        GroupPanel* panel = new GroupPanel;
        site = g_app->getMainFrame()->createSite(panel,
                                                 cfw::sitetypeModeless,
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
    ITangoGridModelPtr model = m_grid->getModel();
    if (model.isNull())
        return;

    DlgKeyBuilder dlg(this);

    dlg.setIterator(m_iter);
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

    tango::objhandle_t handle;

    for (i = 0; i < mark_count; ++i)
    {
        mark = markvec->getItem(i);

        if (mark->getMarkActive())
        {
            handle = getTemporaryHandle(mark->getExpression());

            if (handle &&
                m_iter->getType(handle) == tango::typeBoolean &&
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
        
        if (m_relsync_mark_expr.Length() > 0)
        {
            // mark for relationship syncing with context rows
            handle = getTemporaryHandle(m_relsync_mark_expr);

            if (handle &&
                m_iter->getType(handle) == tango::typeBoolean &&
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
                                     tango::IStructurePtr v_struct)
{
    view->deleteAllColumns();

    if (v_struct)
    {
        tango::IColumnInfoPtr colinfo;
        ITableDocViewColPtr viewcol;

        int col_count;
        col_count = v_struct->getColumnCount();
        for (int i = 0; i < col_count; i++)
        {
            colinfo = v_struct->getColumnInfoByIdx(i);
            
            viewcol = view->createColumn(-1);
            viewcol->setName(towx(colinfo->getName()));
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

    cfw::IDocumentSiteEnumPtr docsites;
    docsites = g_app->getMainFrame()->getDocumentSites(cfw::sitetypeNormal);

    int site_count = docsites->size();
    for (int i = 0; i < site_count; ++i)
    {
        cfw::IDocumentSitePtr site;
        ITableDocPtr table_doc;

        site = docsites->getItem(i);
        table_doc = site->getDocument();

        if (table_doc.p == (ITableDoc*)this)
            continue;

        if (table_doc.isOk())
        {
            if (table_doc->getBaseSet() != getBaseSet())
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
        m_frame->postEvent(new cfw::Event(wxT("tabledoc.viewChanged")));
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
    cfw::IDocumentSitePtr site;
    cfw::IDocumentSiteEnumPtr sites;

    sites = g_app->getMainFrame()->getDocumentSites(cfw::sitetypeNormal);
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

        if (getBaseSet() == table_doc->getBaseSet() ||
            getBrowseSet() == table_doc->getBrowseSet())
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
            cfw::IAppPreferencesPtr prefs = g_app->getAppPreferences();
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
                // entry is a column separators
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
                int colpos = m_grid->insertColumn(-1, model_idx);
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
                tango::IStructurePtr s = m_iter->getStructure();
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


tango::objhandle_t TableDoc::getTemporaryHandle(const wxString& expr)
{
    if (m_iter.isNull())
        return 0;

    std::map<wxString, tango::objhandle_t>::iterator it;
    
    it = m_handle_map.find(expr);
    if (it != m_handle_map.end())
        return it->second;
        

    tango::objhandle_t handle = m_iter->getHandle(towstr(expr));
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
        
    std::map<wxString, tango::objhandle_t>::iterator it;
    for (it = m_handle_map.begin(); it != m_handle_map.end(); ++it)
    {
        if (it->second != 0)
        {
            m_iter->releaseHandle(it->second);
        }
    }

    m_handle_map.clear();
}
