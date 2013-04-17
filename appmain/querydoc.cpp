/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2006-10-06
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "dbdoc.h"
#include "querydoc.h"
#include "sqldoc.h"
#include "tabledoc.h"
#include "toolbars.h"
#include "relationdiagram.h"
#include "dlgdatabasefile.h"
#include "structurevalidator.h"
#include "jobexportpkg.h"
#include <algorithm>
#include <set>


enum
{
    ID_AddParams = 10000,
    ID_Sash,
    ID_RelationWindow,
    
    ID_OutputPath_TextCtrl,
    ID_Browse_Button,
    ID_AddTable_Button,
    ID_SelectDistinct_CheckBox,
    
    ID_InsertField
};



const int colOutputFlag = 0;
const int colInputExpr = 1;
const int colOutputField = 2;
const int colGroupFunction = 3;
const int colSortOrder = 4;
const int colCondition = 5;
const int colOr1 = 6;
const int colOr2 = 7;
const int colOr3 = 8;
const int colOr4 = 9;
const int colOr5 = 10;

static wxString groupfunc2str(int group_func)
{
    wxString text;

    switch (group_func)
    {    
        default:   
        case QueryGroupFunction_None:     text = wxT("--"); break;
        case QueryGroupFunction_GroupBy:  text = wxT("Group By"); break;
        case QueryGroupFunction_First:    text = wxT("First"); break;
        case QueryGroupFunction_Last:     text = wxT("Last"); break;
        case QueryGroupFunction_Min:      text = wxT("Min"); break;
        case QueryGroupFunction_Max:      text = wxT("Max"); break;
        case QueryGroupFunction_Sum:      text = wxT("Sum"); break;
        case QueryGroupFunction_Avg:      text = wxT("Avg"); break;    
        case QueryGroupFunction_Count:    text = wxT("Count"); break;
        case QueryGroupFunction_Stddev:   text = wxT("Stddev"); break;
        case QueryGroupFunction_Variance: text = wxT("Variance"); break;            
        case QueryGroupFunction_GroupID:  text = wxT("Group ID"); break;
    }

    return text;
}

static int groupstr2func(const wxString& group_str)
{
    if (group_str.CmpNoCase(wxT("--")) == 0)       return QueryGroupFunction_None;
    if (group_str.CmpNoCase(wxT("Group By")) == 0) return QueryGroupFunction_GroupBy;
    if (group_str.CmpNoCase(wxT("First")) == 0)    return QueryGroupFunction_First;
    if (group_str.CmpNoCase(wxT("Last")) == 0)     return QueryGroupFunction_Last;
    if (group_str.CmpNoCase(wxT("Min")) == 0)      return QueryGroupFunction_Min;
    if (group_str.CmpNoCase(wxT("Max")) == 0)      return QueryGroupFunction_Max;
    if (group_str.CmpNoCase(wxT("Sum")) == 0)      return QueryGroupFunction_Sum;
    if (group_str.CmpNoCase(wxT("Avg")) == 0)      return QueryGroupFunction_Avg;
    if (group_str.CmpNoCase(wxT("Count")) == 0)    return QueryGroupFunction_Count;
    if (group_str.CmpNoCase(wxT("Stddev")) == 0)   return QueryGroupFunction_Stddev;
    if (group_str.CmpNoCase(wxT("Variance")) == 0) return QueryGroupFunction_Variance;    
    if (group_str.CmpNoCase(wxT("Group ID")) == 0) return QueryGroupFunction_GroupID;

    return -1;
}

static int sortCombo2sortId(int combo_idx)
{
    int arr[] = { 0, 1, 2, 3, 4, 5, 6, -1, -2, -3, -4, -5, -6 };

    if (combo_idx < 0 || combo_idx > 12)
        return 0;

    return arr[combo_idx];
}

static int sortId2sortCombo(int sort_id)
{
    int arr[] = { 12, 11, 10, 9, 8, 7, 0, 1, 2, 3, 4, 5, 6 };

    sort_id += 6;

    if (sort_id < 0 || sort_id > 12)
        return 0;

    return arr[sort_id];
}

static std::vector<RowErrorChecker> getRowErrorCheckerVector(
                                        kcl::Grid* grid,
                                        bool include_empty_fieldnames = true)
{
    std::vector<RowErrorChecker> vec;
    
    int row, row_count = grid->getRowCount();
    for (row = 0; row < row_count; ++row)
    {
        bool checked = grid->getCellBoolean(row, colOutputFlag);
        wxString fieldname = grid->getCellString(row, colOutputField);
        
        if (!checked)
            continue;
        
        if (fieldname.IsEmpty() && !include_empty_fieldnames)
            continue;
        
        vec.push_back(RowErrorChecker(row, fieldname));
    }
    
    return vec;
}


BEGIN_EVENT_TABLE(QueryDoc, wxWindow)
    EVT_MENU(ID_File_Save, QueryDoc::onSave)
    EVT_MENU(ID_File_SaveAs, QueryDoc::onSaveAs)
    EVT_MENU(ID_File_SaveAsExternal, QueryDoc::onSaveAsExternal)
    EVT_MENU(ID_File_Run, QueryDoc::onExecute)
    EVT_MENU(ID_Edit_Copy, QueryDoc::onCopy)
    EVT_MENU(ID_InsertField, QueryDoc::onInsertField)
    EVT_MENU(ID_Edit_Delete, QueryDoc::onDeleteField)
    EVT_MENU(ID_Edit_SelectAll, QueryDoc::onSelectAll)
    EVT_SASH_DRAGGED(ID_Sash, QueryDoc::onSashDragged)
    EVT_TEXT(ID_OutputPath_TextCtrl, QueryDoc::onOutputPathTextChanged)
    EVT_CHECKBOX(ID_SelectDistinct_CheckBox, QueryDoc::onSelectDistinctChecked)
    EVT_BUTTON(ID_AddTable_Button, QueryDoc::onAddTable)
    EVT_BUTTON(ID_Browse_Button, QueryDoc::onBrowse)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(QueryDoc::onGridPreGhostRowInsert)
    EVT_KCLGRID_PRE_INVALID_AREA_INSERT(QueryDoc::onGridPreInvalidAreaInsert)
    EVT_KCLGRID_CELL_RIGHT_CLICK(QueryDoc::onGridCellRightClick)
    EVT_KCLGRID_EDIT_CHANGE(QueryDoc::onGridEditChange)
    EVT_KCLGRID_END_EDIT(QueryDoc::onGridEndEdit)
    EVT_SIZE(QueryDoc::onSize)

    // disable data items
    EVT_UPDATE_UI_RANGE(ID_Data_First, ID_Data_Last, QueryDoc::onUpdateUI_DisableAlways)

    // disable canvas object items
    EVT_UPDATE_UI_RANGE(ID_Canvas_First, ID_Canvas_Last, QueryDoc::onUpdateUI_DisableAlways)

    // disable format items
    EVT_UPDATE_UI_RANGE(ID_Format_First, ID_Format_Last, QueryDoc::onUpdateUI_DisableAlways)

    // disable table items
    EVT_UPDATE_UI_RANGE(ID_Table_First, ID_Table_Last, QueryDoc::onUpdateUI_DisableAlways)

    // disable some of the file items
    EVT_UPDATE_UI(ID_File_PageSetup, QueryDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_File_Print, QueryDoc::onUpdateUI_DisableAlways)

    // disable the zoom
    EVT_UPDATE_UI(ID_View_ZoomCombo, QueryDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomIn, QueryDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomOut, QueryDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToFit, QueryDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToActual, QueryDoc::onUpdateUI_DisableAlways)
    EVT_UPDATE_UI(ID_View_ZoomToWidth, QueryDoc::onUpdateUI_DisableAlways)    

    // enable/disable the edit menu based on conditions
    EVT_UPDATE_UI_RANGE(ID_Edit_First, ID_Edit_Last, QueryDoc::onUpdateUI)

END_EVENT_TABLE()


QueryDoc::QueryDoc()
{
    m_changed = false;
    m_changed_execute = false;
    m_outputpath_textctrl = NULL;
    m_browse_button = NULL;
    m_selectdistinct_checkbox = NULL;
    m_sash = NULL;
    m_diagram = NULL;
    m_grid = NULL;
}

QueryDoc::~QueryDoc()
{
}


bool QueryDoc::initDoc(IFramePtr frame,
                       IDocumentSitePtr doc_site,
                       wxWindow* docsite_wnd,
                       wxWindow* panesite_wnd)
{
    // store ptrs
    m_frame = frame;
    m_doc_site = doc_site;

    // create document's window
    bool result = Create(docsite_wnd,
                         -1,
                         wxPoint(0,0),
                         docsite_wnd->GetClientSize(),
                         wxCLIP_CHILDREN | wxNO_FULL_REPAINT_ON_RESIZE);

    if (!result)
        return false;

    SetBackgroundColour(kcl::getBaseColor());
    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);
    
    // update the caption and set the bitmap
    updateCaption();
    m_doc_site->setBitmap(GETBMP(gf_query_16));

    // create settings sizer
    
    m_add_button = new wxButton(this,
                                ID_AddTable_Button,
                                _("Add Table..."));
    
    m_selectdistinct_checkbox = new wxCheckBox(this,
                                               ID_SelectDistinct_CheckBox,
                                               _("Select Distinct"));
    
    wxStaticText* outputpath_label = new wxStaticText(this,
                                                      -1,
                                                      _("Output Table:"));
    m_outputpath_textctrl = new wxTextCtrl(this,
                                           ID_OutputPath_TextCtrl,
                                           wxEmptyString,
                                           wxDefaultPosition,
                                           wxSize(200,21));
    m_browse_button = new wxButton(this,
                                   ID_Browse_Button,
                                   _("Browse..."));
    
    m_settings_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_settings_sizer->AddSpacer(5);
    m_settings_sizer->Add(m_add_button, 0, wxALIGN_CENTER);
    m_settings_sizer->AddSpacer(5);
    m_settings_sizer->Add(m_selectdistinct_checkbox, 0, wxALIGN_CENTER);
    m_settings_sizer->AddStretchSpacer();
    m_settings_sizer->Add(outputpath_label, 0, wxALIGN_CENTER);
    m_settings_sizer->AddSpacer(5);
    m_settings_sizer->Add(m_outputpath_textctrl, 0, wxALIGN_CENTER);
    m_settings_sizer->AddSpacer(5);
    m_settings_sizer->Add(m_browse_button, 0, wxALIGN_CENTER);
    m_settings_sizer->AddSpacer(5);
    m_settings_sizer->SetMinSize(100, 31);
    
    wxFont banner_font = wxFont(8, wxSWISS, wxNORMAL, wxFONTWEIGHT_BOLD, false);
    
    // colors for drawing the banner controls
    wxColour base_color = kcl::getBaseColor();
    wxColour border_color = kcl::getBorderColor();
    wxColor caption_color = kcl::getCaptionColor();
    
    kcl::BannerControl* query_def_banner = new kcl::BannerControl(this,
                                                       _("Query Definition"),
                                                       wxPoint(0,0),
                                                       wxSize(200, 24));
    query_def_banner->setStartColor(kcl::stepColor(base_color, 170));
    query_def_banner->setEndColor(kcl::stepColor(base_color, 90));
    query_def_banner->setGradientDirection(kcl::BannerControl::gradientVertical);
    query_def_banner->setBorderColor(border_color);
    query_def_banner->setBorder(kcl::BannerControl::borderBottom |
                                kcl::BannerControl::borderTop);
    query_def_banner->setTextColor(caption_color);
    query_def_banner->setTextPadding(5);
    query_def_banner->setFont(banner_font);


    // create relationship diagram
    int sash_height = GetClientSize().GetHeight()/2;
    m_sash = new wxSashWindow(this,
                              ID_Sash,
                              wxPoint(0,0),
                              wxSize(200,sash_height),
                              wxCLIP_CHILDREN | wxSW_NOBORDER);
    m_sash->SetSashVisible(wxSASH_BOTTOM, true);
    m_sash->SetDefaultBorderSize(4);
    
    m_diagram = new RelationDiagram(m_sash, 
                                    ID_RelationWindow,
                                    wxDefaultPosition,
                                    wxDefaultSize,
                                    false,
                                    true);
    m_diagram->setOverlayText(_("To add source tables to this query, drag in\ntables from the Project Panel or double-click here"));
    
    m_diagram->sigSetAdded.connect(this, &QueryDoc::onDiagramSetAdded);
    m_diagram->sigSetRemoved.connect(this, &QueryDoc::onDiagramSetRemoved);
    m_diagram->sigLineActivated.connect(this, &QueryDoc::onDiagramLineActivated);
    m_diagram->sigLineRightClicked.connect(this, &QueryDoc::onDiagramLineRightClicked);
    m_diagram->sigLineAdded.connect(this, &QueryDoc::onDiagramLineAdded);
    m_diagram->sigLineDeleted.connect(this, &QueryDoc::onDiagramLineDeleted);
    m_diagram->sigFieldActivated.connect(this, &QueryDoc::onDiagramFieldActivated);
    m_diagram->sigBoxSizedMoved.connect(this, &QueryDoc::onDiagramBoxSizedMoved);

    wxFont overlay_font = wxFont(16, wxSWISS, wxNORMAL, wxNORMAL, false);

    // create group parameter grid
    m_grid = new kcl::RowSelectionGrid(this,
                                       -1,
                                       wxPoint(0,0),
                                       docsite_wnd->GetClientSize(),
                                       wxBORDER_NONE);
    m_grid->setDragFormat(wxT("querydoc"));
    m_grid->setRowSelectionGridFlags(kcl::RowSelectionGrid::refreshNone);
    m_grid->setOverlayFont(overlay_font);
    m_grid->setCursorType(kcl::Grid::cursorThin);
    m_grid->setAllowInvalidAreaInserts(true);
    m_grid->setAllowKeyboardInserts(true);
    m_grid->setAllowDeletes(true);
    m_grid->setCursorVisible(true);

    m_grid->createModelColumn(-1, wxEmptyString, kcl::Grid::typeBoolean, 0, 0);
    m_grid->createModelColumn(-1, _("Input Formula"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Output Field"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Function"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Sort Order"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Criteria"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Or"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Or"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Or"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Or"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createModelColumn(-1, _("Or"), kcl::Grid::typeCharacter, 80, 0);
    m_grid->createDefaultView();

    m_grid->setColumnSize(colOutputFlag, 23);
    m_grid->setColumnSize(colInputExpr, 140);
    m_grid->setColumnSize(colOutputField, 120);
    m_grid->setColumnSize(colGroupFunction, 75);
    m_grid->setColumnSize(colSortOrder, 90);
    m_grid->setColumnSize(colCondition, 140);
    m_grid->setColumnSize(colOr1, 120);
    m_grid->setColumnSize(colOr2, 120);
    m_grid->setColumnSize(colOr3, 120);
    m_grid->setColumnSize(colOr4, 120);
    m_grid->setColumnSize(colOr5, 120);

    // set cell properties for the grid
    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskAlignment;
    cellprops.alignment = kcl::Grid::alignRight;
    m_grid->setModelColumnProperties(colOutputFlag, &cellprops);
    
    cellprops.mask = kcl::CellProperties::cpmaskCtrlType |
                     kcl::CellProperties::cpmaskCbChoices;
    cellprops.ctrltype = kcl::Grid::ctrltypeDropList;
    cellprops.cbchoices.push_back(groupfunc2str(QueryGroupFunction_None));
    cellprops.cbchoices.push_back(groupfunc2str(QueryGroupFunction_GroupBy));
    cellprops.cbchoices.push_back(groupfunc2str(QueryGroupFunction_Min));
    cellprops.cbchoices.push_back(groupfunc2str(QueryGroupFunction_Max));
    cellprops.cbchoices.push_back(groupfunc2str(QueryGroupFunction_Sum));
    cellprops.cbchoices.push_back(groupfunc2str(QueryGroupFunction_Avg));
    cellprops.cbchoices.push_back(groupfunc2str(QueryGroupFunction_Count));
    cellprops.cbchoices.push_back(groupfunc2str(QueryGroupFunction_Stddev));
    cellprops.cbchoices.push_back(groupfunc2str(QueryGroupFunction_Variance));
    cellprops.cbchoices.push_back(groupfunc2str(QueryGroupFunction_GroupID));
    m_grid->setModelColumnProperties(colGroupFunction, &cellprops);

    cellprops.cbchoices.clear();
    cellprops.cbchoices.push_back(wxT("--"));
    cellprops.cbchoices.push_back(_("1. Ascending"));
    cellprops.cbchoices.push_back(_("2. Ascending"));
    cellprops.cbchoices.push_back(_("3. Ascending"));
    cellprops.cbchoices.push_back(_("4. Ascending"));
    cellprops.cbchoices.push_back(_("5. Ascending"));
    cellprops.cbchoices.push_back(_("6. Ascending"));
    cellprops.cbchoices.push_back(_("1. Descending"));
    cellprops.cbchoices.push_back(_("2. Descending"));
    cellprops.cbchoices.push_back(_("3. Descending"));
    cellprops.cbchoices.push_back(_("4. Descending"));
    cellprops.cbchoices.push_back(_("5. Descending"));
    cellprops.cbchoices.push_back(_("6. Descending"));
    m_grid->setModelColumnProperties(colSortOrder, &cellprops);
    checkOverlayText();

    // make this grid a drop target and connect the signal
    kcl::GridDataObjectComposite* drop_data;
    drop_data = new kcl::GridDataObjectComposite(NULL, wxT("querydoc"),
                                                 NULL, wxT("fieldspanel"));
    drop_data->Add(new RelationLineDataObject);
    
    kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(m_grid);
    drop_target->SetDataObject(drop_data);
    drop_target->sigDropped.connect(this, &QueryDoc::onGridDataDropped);
    m_grid->SetDropTarget(drop_target);

    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_settings_sizer, 0, wxEXPAND);
    //main_sizer->Add(source_banner, 0, wxEXPAND);
    main_sizer->Add(m_sash, 0, wxEXPAND);
    main_sizer->Add(query_def_banner, 0, wxEXPAND);
    main_sizer->Add(m_grid, 1, wxEXPAND);
    
    // doSizing() will call Layout() for us
    SetSizer(main_sizer);
    doSizing(GetClientSize(), sash_height);

    // populate the diagram and grid if the query template
    // information has been filled out
    populateDiagramFromTemplate();
    populateGridFromTemplate();

    // refresh the row selection grid
    if (m_grid->getRowCount() > 0)
        m_grid->setRowSelected(0, true);
    m_grid->moveCursor(0, colInputExpr, false);
    m_grid->refresh(kcl::Grid::refreshAll);

    // connect row selection grid signals
    m_grid->sigInsertingRows.connect(this, &QueryDoc::onInsertingRows);
    m_grid->sigDeletedRows.connect(this, &QueryDoc::onDeletedRows);

    // add frame event handlers
    frame->sigFrameEvent().connect(this, &QueryDoc::onFrameEvent);

    // create the statusbar items for this document
    IStatusBarItemPtr item;
    item = addStatusBarItem(wxT("querydoc_field_count"));
    item->setWidth(120);
    item->show(false);

    // reset the changed flag and return; set the changed flag 
    // at the end since this function triggers events that 
    // normally dirty the document
    setChanged(false);
    setChangedExecute(false);

    return true;
}

wxWindow* QueryDoc::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

wxString QueryDoc::getDocumentLocation()
{
    return m_path;
}

void QueryDoc::setDocumentFocus()
{
    if (m_grid)
    {
        m_grid->SetFocus();
    }

#ifdef WIN32
    // for gtk, see QueryDoc::onIdle() note
    if (m_diagram)
    {
        m_diagram->refresh();
    }
#endif
}

bool QueryDoc::onSiteClosing(bool force)
{
    if (force)
        return true;

    if (isChanged())
    {
        int result;
        result = appMessageBox(_("Would you like to save the changes made to this document?"),
                                    APPLICATION_NAME,
                                    wxYES_NO | wxCANCEL | wxICON_QUESTION | wxCENTER);
        
        // if we want to save the changes, try to save them and return
        // whatever value the doSave() function returns
        if (result == wxYES)
            return doSave();

        // if we cancel, do not save and do not close the site
        if (result == wxCANCEL)
            return false;
    }

    // if we're neither saving nor canceling, we want to close the
    // site without saving; return true
    return true;
}

void QueryDoc::onSiteActivated()
{
    if (m_grid)
        updateStatusBar();
}



// static
bool QueryDoc::newFile(const wxString& path)
{
    QueryTemplate t;
    return t.save(path, false);
}



void QueryDoc::doSizing(const wxSize& clisize, int sash_height)
{
    if (sash_height < 150)
        sash_height = 150;

    if (sash_height > clisize.y-200)
        sash_height = clisize.y-200;

    if (clisize.GetHeight() < 250)
        sash_height = clisize.GetHeight()/2;
        
    int min_width = 150;
    
    GetSizer()->SetItemMinSize(m_sash, min_width, sash_height);
    Layout();
}

bool QueryDoc::isChanged()
{
    return m_changed;
}

bool QueryDoc::isChangedExecute()
{
    return m_changed_execute;
}

void QueryDoc::setChanged(bool changed)
{
    // save the old change state
    bool m_old_changed = m_changed;
    
    // set the changed state for the changed flag
    m_changed = changed;

    // if the changed state has changed, update
    // the caption
    if (m_old_changed != m_changed)
        updateCaption();
}

void QueryDoc::setChangedExecute(bool changed)
{
    m_changed_execute = changed;
}

void QueryDoc::insertSelectedRows()
{
    kcl::SelectionRect rect;
    int sel_count = m_grid->getSelectionCount();
    
    // insert the selected number of rows into the grid
    for (int i = 0; i < sel_count; ++i)
    {
        m_grid->getSelection(i, &rect);
        int start_row = rect.m_start_row;
        int row = rect.m_end_row;
        
        while (row-- >= start_row)
        {
            insertRow(start_row, wxEmptyString);
        }
    }
    
    // the grid is empty, insert a starter row
    if (sel_count == 0 && m_grid->getRowCount() == 0)
    {
        insertRow(0, wxEmptyString);
        m_grid->setRowSelected(0, true);
    }
    
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void QueryDoc::onInsertingRows(std::vector<int> rows)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    // if multiple rows are selected, clear the selection and
    // select only the newly inserted row
    if (rows.size() > 1)
    {
        m_grid->clearSelection();
        m_grid->setRowSelected(rows[0], true);
        m_grid->moveCursor(rows[0], m_grid->getCursorColumn());
    }
    
    // insert the selected rows
    insertSelectedRows();
}

void QueryDoc::onDeletedRows(std::vector<int> rows)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
    updateStatusBar();
}

void QueryDoc::onSize(wxSizeEvent& evt)
{
    if (m_diagram)
    {
        doSizing(evt.GetSize(), m_sash->GetSize().GetHeight());
    }
    
    evt.Skip();
}

void QueryDoc::onUpdateUI_EnableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(true);
}

void QueryDoc::onUpdateUI_DisableAlways(wxUpdateUIEvent& evt)
{
    evt.Enable(false);
}


void QueryDoc::onUpdateUI(wxUpdateUIEvent& evt)
{
    int id = evt.GetId();
    
    // disable undo/redo
    if (id == ID_Edit_Undo ||
        id == ID_Edit_Redo)
    {
        evt.Enable(false);
        return;
    }
    
    // disable cut/copylink/paste (for now)
    if (id == ID_Edit_Cut ||
        id == ID_Edit_CopyLink ||
        id == ID_Edit_Paste)
    {
        evt.Enable(false);
        return;
    }
    
    // enable copy/delete if there's a selection
    if (id == ID_Edit_Copy ||
        id == ID_Edit_Delete)
    {
        if (m_grid->getSelectionCount() > 0)
            evt.Enable(true);
        else
            evt.Enable(false);
        return;
    }
    
    // disable find/replace (for now)
    if (id == ID_Edit_Find ||
        id == ID_Edit_Replace)
    {
        evt.Enable(false);
        return;
    }
    
    if (id == ID_Edit_FindPrev || id == ID_Edit_FindNext)
    {
        evt.Enable(false);
        return;
    }
    
    // disable goto
    if (id == ID_Edit_GoTo)
    {
        evt.Enable(false);
        return;
    }
    
    // enable other items by default
    evt.Enable(true);
    return;
}

void QueryDoc::getColumnListItems(std::vector<ColumnListItem>& items)
{
    // clear the list
    items.clear();

    // iterate through the source tables and add the columns to the list
    std::vector<QueryBuilderSourceTable>::iterator it, it_end;
    it_end = m_info.m_source_tables.end();

    for (it = m_info.m_source_tables.begin(); it != it_end; ++it)
    {
        // get the structure
        tango::IStructurePtr structure = it->structure;
        
        // if the structure is invalid, move on
        if (structure.isNull())
            continue;

        // add the columns to the list
        int i, col_count = structure->getColumnCount();
        for (i = 0; i < col_count; i++)
        {
            tango::IColumnInfoPtr colinfo = structure->getColumnInfoByIdx(i);
         
            ColumnListItem item;
            item.text = it->alias;
            item.text += wxT(".");
            item.text += makeProperIfNecessary(colinfo->getName());
            if (colinfo->getCalculated())
            {
                item.bitmap = GETBMP(gf_lightning_16);
            }
             else
            {
                item.bitmap = GETBMP(gf_field_16);
            }
            item.active = true;
            items.push_back(item);
        }
    }
}

void QueryDoc::onColumnListDblClicked(const std::vector<wxString>& items)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    // inserting fields in from the field panel
    std::vector<wxString>::const_iterator it;
    for (it = items.begin(); it != items.end(); ++it)
        insertRow(-1, (*it));
    
    m_grid->moveCursor(m_grid->getRowCount()-1, colInputExpr);
    m_grid->scrollVertToCursor();
    m_grid->refresh(kcl::Grid::refreshAll);
}

void QueryDoc::onGridDataDropped(kcl::GridDataDropTarget* drop_target)
{
    wxDataObject* obj = drop_target->GetDataObject();
    if (!obj)
        return;

    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    // get the row number where we dropped the data
    int drop_row = drop_target->getDropRow();

    // determine which type of data object was dropped
    kcl::GridDataObjectComposite* drop_data = (kcl::GridDataObjectComposite*)obj;
    wxDataFormat fmt = drop_data->GetReceivedFormat();
    
    // check to see if what type of data object we're dealing with
    if (drop_data->isGridData())
    {
        if (fmt.GetId() == kcl::getGridDataFormat(wxT("querydoc")))
        {
            // reordering rows in the grid
            drop_target->doRowDrag(true);
        }
         else if (fmt.GetId() == kcl::getGridDataFormat(wxT("fieldspanel")))
        {
            // dragging fields in from the field panel
            kcl::GridDraggedCells cells = drop_target->getDraggedCells();
            kcl::GridDraggedCells::iterator it;
            for (it = cells.begin(); it != cells.end(); ++it)
            {
                // only look at the first column from the fields panel
                if ((*it)->m_col != 0)
                    continue;
                
                insertRow(drop_row, (*it)->m_strvalue);
                drop_row++;
            }
            
            // if nothing was selected when we dropped the data,
            // select the row with the cursor in it now
            if (m_grid->getSelectionCount() == 0)
            {
                int cursor_row = m_grid->getCursorRow();
                m_grid->setRowSelected(cursor_row, true);
            }
            
            checkOverlayText();
            
            m_grid->refresh(kcl::Grid::refreshAll);
            return;
        }
    }
     else
    {
        // only accept relationship objects here
        if (fmt.GetId().CmpNoCase(RELATIONLINE_DATA_OBJECT_FORMAT) != 0)
            return;
            
        size_t len = drop_data->GetDataSize(fmt);
        unsigned char* data = new unsigned char[len];
        drop_data->GetDataHere(fmt, data);
        
        // copy the data from the wxDataObjectComposite to this new
        // RelLinDataObject so we can use it's accessor functions

        RelationLineDataObject* line_obj = new RelationLineDataObject;
        line_obj->SetData(fmt, len, data);
        
        // clear the diagram selection
        m_diagram->clearBoxSelection();

        // insert rows into our output grid
        wxString fields = line_obj->getFields();
        wxStringTokenizer t(fields, wxT("\t"));
        while (t.HasMoreTokens())
        {
            insertRow(drop_row, t.GetNextToken());
            drop_row++;
        }

        checkOverlayText();
        m_grid->refresh(kcl::Grid::refreshAll);

        delete[] data;
        delete line_obj;
    }
}

void QueryDoc::onGridPreGhostRowInsert(kcl::GridEvent& evt)
{
    // we'll handle the processing of this event ourselves
    evt.Veto();

    // make sure we're not editing the grid
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    // set the changed flag
    setChanged(true);
    setChangedExecute(true);
    
    insertRow(-1, wxEmptyString);
    int row = m_grid->getRowCount()-1;
    m_grid->moveCursor(row, evt.GetColumn(), false);
    m_grid->clearSelection();
    m_grid->setRowSelected(row, true);
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void QueryDoc::onGridPreInvalidAreaInsert(kcl::GridEvent& evt)
{
    // we'll handle the processing of this event ourselves
    evt.Veto();

    // make sure we're not editing the grid
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    // set the changed flag
    setChanged(true);
    setChangedExecute(true);
    
    insertRow(-1, wxEmptyString);
    int row = m_grid->getRowCount()-1;
    m_grid->moveCursor(row, evt.GetColumn(), false);
    m_grid->clearSelection();
    m_grid->setRowSelected(row, true);
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void QueryDoc::onGridCellRightClick(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (row < 0 || row >= m_grid->getRowCount() ||
        col < 0 || col >= m_grid->getColumnCount())
        return;

    // if the user clicked on a row that was not
    // previously selected, select the row
    if (!m_grid->isRowSelected(row))
    {
        m_grid->clearSelection();
        m_grid->setRowSelected(row, true);
        m_grid->refresh(kcl::Grid::refreshAll);
    }
    
    wxString insert_field_str = _("&Insert Field");
    
    int selected_row_count = m_grid->getSelectedRowCount();
    if (selected_row_count > 1)
        insert_field_str = _("&Insert Fields");
    
    wxMenu menuPopup;
    menuPopup.Append(ID_InsertField, insert_field_str);
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Edit_Copy, _("&Copy"));
    menuPopup.Append(ID_Edit_Delete, _("&Delete"));
    menuPopup.AppendSeparator();
    menuPopup.Append(ID_Edit_SelectAll, _("Select &All"));


    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);
    CommandCapture* cc = new CommandCapture;
    PushEventHandler(cc);
    PopupMenu(&menuPopup, pt_mouse);
    int command = cc->getLastCommandId();
    PopEventHandler(true);

    // post the event to the event handler
    wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, command);
    ::wxPostEvent(this, e);
}


void QueryDoc::onGridEndEdit(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (col == colOutputField)
    {
        if (evt.GetEditCancelled())
            return;
        
        // this will force the cell's text to be the text that we just
        // entered (this is necessary because we use getCellString()
        // in the checkDuplicateFieldnames() function below and the
        // cell's text has not yet changed)
        m_grid->setCellString(row, colOutputField , evt.GetString());
        
        clearProblemRows();
        checkDuplicateFieldnames(CheckMarkRows);
        checkInvalidFieldnames(CheckMarkRows);
        m_grid->refreshColumn(kcl::Grid::refreshAll, colOutputField);
    }

}

void QueryDoc::onGridEditChange(kcl::GridEvent& evt)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);
    
    
    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (col == colGroupFunction)
    {
        wxString output_column = m_grid->getCellString(row, colOutputField);
        
        if (groupstr2func(evt.GetString()) == QueryGroupFunction_Count)
        {
            m_grid->setCellString(row, colInputExpr, wxT(""));
            if (output_column.IsEmpty() || output_column == wxT("Group_id"))
                m_grid->setCellString(row, colOutputField, wxT("Row_count"));
            m_grid->refresh(kcl::Grid::refreshAll);
        }
         else if (groupstr2func(evt.GetString()) == QueryGroupFunction_GroupID)
        {
            m_grid->setCellString(row, colInputExpr, wxT(""));
            if (output_column.IsEmpty() || output_column == wxT("Row_count"))
                m_grid->setCellString(row, colOutputField, wxT("Group_id"));
            m_grid->refresh(kcl::Grid::refreshAll);
        }
    }
}

void QueryDoc::onCopy(wxCommandEvent& evt)
{
    if (!windowOrChildHasFocus(m_doc_site->getContainerWindow()))
    {
        evt.Skip();
        return;
    }

    AppBusyCursor c;
    m_grid->copySelection();
}

void QueryDoc::onInsertField(wxCommandEvent& evt)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    // insert the selected rows
    insertSelectedRows();
}

void QueryDoc::onDeleteField(wxCommandEvent& evt)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    // delete the selected rows
    m_grid->deleteSelectedRows(false);
    checkOverlayText();
    m_grid->refresh(kcl::Grid::refreshAll);
    updateStatusBar();
}

void QueryDoc::onSelectAll(wxCommandEvent& evt)
{
    if (m_grid->isEditing() || wxWindow::FindFocus() == m_grid)
        m_grid->selectAll();
}

void QueryDoc::onOutputPathTextChanged(wxCommandEvent& evt)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    // set the path
    m_info.m_output_path = m_outputpath_textctrl->GetValue();
}

void QueryDoc::onSelectDistinctChecked(wxCommandEvent& evt)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    // set the distinct flag
    m_info.m_distinct = m_selectdistinct_checkbox->IsChecked();
}

void QueryDoc::onAddTable(wxCommandEvent& evt)
{
    showAddTableDialog(m_diagram);
}

void QueryDoc::onBrowse(wxCommandEvent& evt)
{
    DlgDatabaseFile dlg(g_app->getMainWindow(), DlgDatabaseFile::modeSave);
    dlg.setAffirmativeButtonLabel(_("OK"));
    dlg.setCaption(_("Select Output Table"));
    
    if (dlg.ShowModal() == wxID_OK)
        m_outputpath_textctrl->SetValue(dlg.getPath());
}

void QueryDoc::onTreeDataDropped(FsDataObject* data)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);
    
    IFsItemEnumPtr items = data->getFsItems();
    
    std::vector<wxString>::iterator it;
    std::vector<wxString> res;
    DbDoc::getFsItemPaths(items, res, true);
    
    wxPoint pt = ::wxGetMousePosition();
    pt = ScreenToClient(pt);
    
    DbDoc* dbdoc = g_app->getDbDoc();
    
    int i, count = items->size();
    for (i = 0; i < count; ++i)
    {
        IFsItemPtr item = items->getItem(i);
        if (item.isNull())
            continue;

        IDbFolderFsItemPtr folder = item;
        if (folder.isOk())
        {
            appMessageBox(_("One or more of the items is a folder.  Folders cannot be added to the relationship diagram."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }
        
        if (dbdoc->isFsItemExternal(item))
        {
            appMessageBox(_("One or more of the items is an external table.  External tables cannot be added to the relationship diagram."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }

        IDbObjectFsItemPtr obj = item;
        if (obj.isOk())
        {
            if (obj->getType() != dbobjtypeSet)
            {
                appMessageBox(_("One or more of the items is not a table.  Items that are not tables cannot be added to the relationship diagram."),
                                   APPLICATION_NAME,
                                   wxOK | wxICON_EXCLAMATION | wxCENTER);

                return;
            }
        }
    }

    // make sure all paths (even singleton mounts on the linkbar)
    // are converted to their _REAL_ path in the project
    for (it = res.begin(); it != res.end(); ++it)
        getRemotePathIfExists(*it);

    for (it = res.begin(); it != res.end(); ++it)
    {
        wxString path = *it;
        if (!g_app->getDatabase()->getFileExist(towstr(path)))
        {
            // some of the paths couldn't be found, bail out
            appMessageBox(_("One or more of the items could not be found in the project."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return;
        }
    }
    
    for (it = res.begin(); it != res.end(); ++it)
    {
        wxString path = *it;
        tango::IFileInfoPtr finfo = g_app->getDatabase()->getFileInfo(towstr(path));
        if (finfo.isOk() && finfo->getType() == tango::filetypeTable)
        {
            tango::IStructurePtr structure = g_app->getDatabase()->describeTable(towstr(path));

            if (structure.isOk())
            {
                QueryBuilderSourceTable s;
                s.structure = structure;
                s.alias = it->AfterLast(wxT('/'));

                m_info.m_source_tables.push_back(s);
            }
        }
    }
}

void QueryDoc::onDiagramSetAdded(wxString path, bool* allow)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    std::vector<wxString> boxes;

    // find out which boxes are open in the diagram
    m_diagram->getBoxPaths(boxes);

    // make sure the set hasn't already been added
    std::vector<wxString>::iterator boxpath_it;
    for (boxpath_it = boxes.begin();
         boxpath_it != boxes.end(); ++boxpath_it)
    {
        if (0 == path.CmpNoCase(*boxpath_it))
        {
            m_diagram->refresh();

            appMessageBox(_("The data set already exists in the diagram."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);

            *allow = false;
            return;
        }
    }

    tango::IFileInfoPtr finfo = g_app->getDatabase()->getFileInfo(towstr(path));
    if (finfo.isNull() || finfo->getType() != tango::filetypeTable)
        return;

    tango::IStructurePtr structure = g_app->getDatabase()->describeTable(towstr(path));
    if (structure.isNull())
        return;

    // choose unique alias
    wxString orig_alias = path.AfterLast(wxT('/'));
    wxString alias = orig_alias;
    int idx = 1;

    while (1)
    {
        bool found = false;

        std::vector<QueryBuilderSourceTable>::iterator it;
        for (it = m_info.m_source_tables.begin();
             it != m_info.m_source_tables.end(); ++it)
        {
            if (0 == alias.CmpNoCase(it->alias))
            {
                found = true;
                break;
            }
        }

        if (!found)
            break;

        idx++;
        alias.Printf(wxT("%s%d"), orig_alias.c_str(), idx);
    }

    QueryBuilderSourceTable s;
    s.path = path;
    s.structure = structure;
    s.alias = alias;

    m_info.m_source_tables.push_back(s);
    
    // update the column list
    updateColumnList();
}

void QueryDoc::onDiagramSetRemoved(wxString path)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    m_diagram->deleteLines(path);

    std::vector<QueryBuilderSourceTable>::iterator it;

    for (it = m_info.m_source_tables.begin();
         it != m_info.m_source_tables.end(); ++it)
    {
        if (0 == path.CmpNoCase(it->path))
        {
            m_info.m_source_tables.erase(it);
            break;
        }
    }

    // update the join structures for all windows
    for (it = m_info.m_source_tables.begin();
         it != m_info.m_source_tables.end(); ++it)
    {
        updateJoinStructure(it->path);
    }
    
    // update the column list
    updateColumnList();
}

void QueryDoc::onDiagramLineAdded(RelationLine* line, bool* allowed)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    std::vector<RelationLine*> lines;
    m_diagram->getLines(line->left_path,
                        line->right_path,
                        lines);

    if (lines.size() == 1)
    {
        // first line added
        line->user = QueryJoinInner;
        line->left_bitmap = wxNullBitmap;
        line->right_bitmap = wxNullBitmap;
    }
     else
    {
        line->user = lines[0]->user;
        line->left_bitmap = lines[0]->left_bitmap;
        line->right_bitmap = lines[0]->right_bitmap;
    }

    updateJoinStructure(line->left_path);
}

void QueryDoc::onDiagramLineDeleted(RelationLine* line)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    // update the join structure
    updateJoinStructure(line->left_path);
}

void QueryDoc::onDiagramLineActivated(RelationLine* line)
{

}

void QueryDoc::onDiagramLineRightClicked(RelationLine* line, int* command)
{
    enum
    {
        MenuID_InnerJoin = wxID_HIGHEST+1,
        MenuID_LeftOuterJoin
    };
    
    wxMenu menuPopup;
    menuPopup.AppendRadioItem(MenuID_InnerJoin, _("Inner Join"));
    menuPopup.AppendRadioItem(MenuID_LeftOuterJoin, _("Left Outer Join"));
    menuPopup.AppendSeparator();
    menuPopup.Append(RelationDiagram::MenuID_DeleteLine, _("Delete Relation"));
    menuPopup.AppendSeparator();
    menuPopup.Append(RelationDiagram::MenuID_Cancel, _("Cancel"));

    // set the radio menu items based on the join criteria
    int join_type = line->user;
    if (join_type == QueryJoinNone)
        join_type = QueryJoinInner;
        
    if (join_type == QueryJoinInner)
        menuPopup.Check(MenuID_InnerJoin, true);
     else
        menuPopup.Check(MenuID_LeftOuterJoin, true);
        
    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);
    CommandCapture* cc = new CommandCapture;
    PushEventHandler(cc);
    PopupMenu(&menuPopup, pt_mouse);
    *command = cc->getLastCommandId();
    PopEventHandler(true);
    
    // if the command is canceled, don't do anything
    if (*command == RelationDiagram::MenuID_Cancel)
        return;

    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    // set the join bitmap
    if (*command == MenuID_InnerJoin)
    {
        line->user = QueryJoinInner;
        line->left_bitmap = wxNullBitmap;
        line->right_bitmap = wxNullBitmap;
    }
     else if (*command == MenuID_LeftOuterJoin)
    {
        line->user = QueryJoinLeftOuter;
        line->left_bitmap = GETBMP(kx_asterisk_16);
        line->right_bitmap = wxNullBitmap;
    }
    
    // update the join structure
    updateJoinStructure(line->left_path);
}

void QueryDoc::onDiagramFieldActivated(wxString path, wxString field)
{
    // set the changed flag
    setChanged(true);
    setChangedExecute(true);

    QueryBuilderSourceTable* tbl = m_info.lookupTableByPath(path);
    if (tbl)
    {
        wxString f;

        f = tbl->alias;
        f += wxT(".");
        f += field;

        insertRow(-1, f);
    }
    
    m_grid->Freeze();
    m_grid->moveCursor(m_grid->getRowCount()-1, colInputExpr);
    m_grid->scrollVertToCursor();
    m_grid->refresh(kcl::Grid::refreshAll);
    m_grid->Thaw();
}

void QueryDoc::onDiagramBoxSizedMoved(wxString path, wxRect rect)
{
    // set the changed flag
    setChanged(true);
    
    // no need to reset the execute flag since this doesn't
    // change the query

    QueryBuilderSourceTable* tbl = m_info.lookupTableByPath(path);
    if (tbl)
    {
        tbl->x = rect.x;
        tbl->y = rect.y;
        tbl->width = rect.width;
        tbl->height = rect.height;
    }
}

void QueryDoc::updateCaption()
{
    wxString caption = _("(Untitled)");
    if (m_path.Length() == 0)
        m_doc_site->setCaption(caption);
    else
    {
        caption = m_path.AfterLast(wxT('/'));
        caption.Append(isChanged() ? wxT("*") : wxT(""));

        m_doc_site->setCaption(caption);
    }

    // if we have a sqldoc, update its caption
    IDocumentSitePtr sqldoc_site;
    sqldoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.SqlDoc");
    if (sqldoc_site.isOk())
        sqldoc_site->setCaption(caption);
}

void QueryDoc::updateStatusBar()
{
/*
    // if the grid hasn't been created yet, bail out
    if (!m_grid)
        return;

    // field count
    int row_count = m_grid->getRowCount();
    wxString field_count_str = wxString::Format(_("Field Count: %d"), row_count);
    
    IStatusBarItemPtr item;
    item = m_frame->getStatusBar()->getCell(wxT("querydoc_field_count"));
    if (item.isOk())
        item->setValue(field_count_str);

    // refresh the statusbar
    g_app->getMainFrame()->getStatusBar()->refresh();
*/
}

void QueryDoc::updateColumnList()
{
    g_app->getMainFrame()->postEvent(new FrameworkEvent(FRAMEWORK_EVT_COLUMNLISTPANEL_UPDATE));
}

void QueryDoc::updateJoinStructure(const wxString& left_path)
{
    std::vector<RelationInfo> info;
    m_diagram->getRelationInfo(left_path, info);

    // find the table in our list of tables
    std::vector<QueryBuilderSourceTable>::iterator tbl_it;
    for (tbl_it = m_info.m_source_tables.begin();
         tbl_it != m_info.m_source_tables.end(); ++tbl_it)
    {
        if (left_path.CmpNoCase(tbl_it->path) == 0)
            break;
    }
    
    if (tbl_it == m_info.m_source_tables.end())
        return;

    tbl_it->joins.clear();

    std::vector<RelationInfo>::iterator it;
    for (it = info.begin(); it != info.end(); ++it)
    {
        if (it->lines.size() == 0)
            continue;

        QueryJoin j;
        j.join_type = it->lines[0].user;
        j.right_path = it->right_path;

        std::vector<RelationLine>::iterator line_it;
        for (line_it = it->lines.begin();
             line_it != it->lines.end(); ++line_it)
        {
            if (!j.left_columns.IsEmpty())
                j.left_columns += wxT(",");

            if (!j.right_columns.IsEmpty())
                j.right_columns += wxT(",");

            j.left_columns += line_it->left_expr;
            j.right_columns += line_it->right_expr;
        }

        tbl_it->joins.push_back(j);
    }
}

void QueryDoc::onSashDragged(wxSashEvent& evt)
{
    if (evt.GetId() == ID_Sash)
    {
        doSizing(GetClientSize(), evt.GetDragRect().GetHeight());
    }
}



struct QueryBuilderFieldInfo
{
    wxString field;
    bool dynamic;
};


class QBFILess
{
public:
     bool operator()(const QueryBuilderFieldInfo& x,
                     const QueryBuilderFieldInfo& y) const                
     {
        return x.field < y.field;
     }
};




void QueryDoc::populateDiagramFromTemplate()
{
    if (!m_diagram)
        return;

    // disconnect this signal for the duration of this function

    m_diagram->sigLineAdded.disconnect();

    m_diagram->clear();

    std::vector<QueryBuilderSourceTable>::iterator it;
    std::vector<QueryJoin>::iterator join_it;

    for (it = m_info.m_source_tables.begin();
         it != m_info.m_source_tables.end(); ++it)
    {
        m_diagram->addBox(it->path,
                          it->path,
                          wxColor(0,0,128),
                          it->x,
                          it->y,
                          it->width,
                          it->height);
    }

    // add joins
    for (it = m_info.m_source_tables.begin();
         it != m_info.m_source_tables.end(); ++it)
    {
        for (join_it = it->joins.begin();
             join_it != it->joins.end(); ++join_it)
        {
            std::vector<wxString> left_columns;
            std::vector<wxString> right_columns;

            wxStringTokenizer left(join_it->left_columns, wxT(","));
            while (left.HasMoreTokens())
            {
                left_columns.push_back(left.GetNextToken());
            }

            wxStringTokenizer right(join_it->right_columns, wxT(","));
            while (right.HasMoreTokens())
            {
                right_columns.push_back(right.GetNextToken());
            }

            if (left_columns.size() == right_columns.size())
            {
                int count = left_columns.size();
                int i;

                for (i = 0; i < count; ++i)
                {
                    RelationLine* line;
                    line = m_diagram->addLine(it->path,
                                              left_columns[i],
                                              join_it->right_path,
                                              right_columns[i]);

                    int join_type = join_it->join_type;

                    line->user = join_type;

                    if (join_type == QueryJoinInner)
                    {
                        line->left_bitmap = wxNullBitmap;
                        line->right_bitmap = wxNullBitmap;
                    }
                     else if (join_type == QueryJoinLeftOuter)
                    {
                        line->left_bitmap = GETBMP(kx_asterisk_16);
                        line->right_bitmap = wxNullBitmap;
                    }
                }
            }
        }
    }

    m_diagram->sigLineAdded.connect(this, &QueryDoc::onDiagramLineAdded);
    m_diagram->refreshBoxes();

    // update controls
    m_outputpath_textctrl->SetValue(m_info.m_output_path);
    m_selectdistinct_checkbox->SetValue(m_info.m_distinct);
}

void QueryDoc::populateGridFromTemplate()
{
    if (!m_grid)
        return;

    std::vector<QueryBuilderParam>::iterator it;

    m_grid->deleteAllRows();
    int row = 0;
    
    for (it = m_info.m_params.begin();
         it != m_info.m_params.end();
         ++it)
    {
        m_grid->insertRow(-1);
        m_grid->setCellBoolean(row, colOutputFlag, it->output);
        m_grid->setCellString(row, colInputExpr, it->input_expr);
        m_grid->setCellString(row, colOutputField, it->output_field);
        m_grid->setCellString(row, colGroupFunction, groupfunc2str(it->group_func));
        m_grid->setCellComboSel(row, colSortOrder, sortId2sortCombo(it->sort_order));

        std::vector<wxString>::iterator cond_it;
        int i = 0;
        for (cond_it = it->conditions.begin();
             cond_it != it->conditions.end();
             ++cond_it)
        {
            m_grid->setCellString(row, colCondition+i, *cond_it);
            ++i;
        }

        row++;
    }

    checkOverlayText();
}

void QueryDoc::populateTemplateFromInterface()
{
    // if there was any existing query template information,
    // clear it out before we fill it out below
    
    m_info.m_source_tables.clear();
    m_info.m_params.clear();


    // fill out the query template from the diagram
    int i, box_count = m_diagram->getBoxCount();
    for (i = 0; i < box_count; ++i)
    {
        RelationBox* box = m_diagram->getBox(i);
        
        QueryBuilderSourceTable tbl;
        tbl.path = box->getSetPath();
        tbl.alias = tbl.path.AfterLast(wxT('/'));
        box->GetPosition(&tbl.x, &tbl.y);
        box->GetSize(&tbl.width, &tbl.height);
        tbl.structure = box->getStructure();
        
        // add the source table to the template
        m_info.m_source_tables.push_back(tbl);
        
        // update the joins
        updateJoinStructure(tbl.path);
    }
    

    

    // fill out the query template from the grid
    int row_count = m_grid->getRowCount();
    for (int row = 0; row < row_count; ++row)
    {
        QueryBuilderParam param;
        param.output = m_grid->getCellBoolean(row, colOutputFlag);
        param.input_expr = m_grid->getCellString(row, colInputExpr);
        param.output_field = m_grid->getCellString(row, colOutputField);
        param.group_func = groupstr2func(m_grid->getCellString(row, colGroupFunction));
        param.sort_order = sortCombo2sortId(m_grid->getCellComboSel(row, colSortOrder));
        param.conditions.push_back(m_grid->getCellString(row, colCondition));
        param.conditions.push_back(m_grid->getCellString(row, colOr1));
        param.conditions.push_back(m_grid->getCellString(row, colOr2));
        param.conditions.push_back(m_grid->getCellString(row, colOr3));
        param.conditions.push_back(m_grid->getCellString(row, colOr4));
        param.conditions.push_back(m_grid->getCellString(row, colOr5));

        // add the parameters to the template
        m_info.m_params.push_back(param);
    }

    m_info.m_distinct = m_selectdistinct_checkbox->GetValue();
    m_info.m_output_path = m_outputpath_textctrl->GetValue();
}

void QueryDoc::insertRow(int row, const wxString& _input_field)
{
    wxString input_field = _input_field;
    
    if (row == -1)
        row = m_grid->getRowCount();
    
    
    if (input_field.Freq('.') > 0)
    {
        wxString alias = input_field.BeforeLast(wxT('.'));
        wxString field = input_field.AfterLast(wxT('.'));
        input_field = alias;
        input_field += wxT(".");
        input_field += field;
    }
    
    
    wxString output_field = input_field.AfterLast(wxT('.'));
    
    
    m_grid->insertRow(row);
    m_grid->setCellBoolean(row, colOutputFlag, true);
    m_grid->setCellString(row, colInputExpr, input_field);
    m_grid->setCellString(row, colOutputField, output_field);
    m_grid->setCellComboSel(row, colGroupFunction, 0 /* no group function */);
    m_grid->setCellComboSel(row, colSortOrder, 0 /* no sort order */);
    
    checkOverlayText();
}

void QueryDoc::checkOverlayText()
{
    if (m_grid->getRowCount() == 0)
        m_grid->setOverlayText(_("To add output fields to this query, drag in\nfields from the above tables or double-click here"));
         else
        m_grid->setOverlayText(wxEmptyString);
}

void QueryDoc::clearProblemRows()
{
    int row, row_count = m_grid->getRowCount();
    for (row = 0; row < row_count; ++row)
        m_grid->setCellBitmap(row, colOutputField, wxNullBitmap);
}

void QueryDoc::markProblemRow(int row, bool scroll_to)
{
    m_grid->setCellBitmap(row, colOutputField, GETBMP(gf_exclamation_16));
    
    if (scroll_to)
    {
        m_grid->moveCursor(row, colOutputField, false);
        if (!m_grid->isCursorVisible())
            m_grid->scrollVertToCursor();
    }
}

int QueryDoc::checkDuplicateFieldnames(int check_flags)
{
    // if we're editing, end the edit
    if (m_grid->isEditing())
        m_grid->endEdit(true);
    
    bool include_empty_fieldnames = false;
    if (check_flags & CheckEmptyFieldnames)
        include_empty_fieldnames = true;
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid, include_empty_fieldnames);

    bool mark_rows = (check_flags & CheckMarkRows);
    bool errors_found = StructureValidator::findDuplicateFieldNames(check_rows);
    if (errors_found && mark_rows)
    {
        std::vector<RowErrorChecker>::iterator it;
        for (it = check_rows.begin(); it != check_rows.end(); ++it)
        {
            if (it->errors != StructureValidator::ErrorNone)
                markProblemRow(it->row, false);
        }
    }

    return (errors_found ? StructureValidator::ErrorDuplicateFieldNames
                         : StructureValidator::ErrorNone);
}

int QueryDoc::checkInvalidFieldnames(int check_flags)
{
    // if we're editing, end the edit
    if (m_grid->isEditing())
        m_grid->endEdit(true);
    
    bool include_empty_fieldnames = false;
    if (check_flags & CheckEmptyFieldnames)
        include_empty_fieldnames = true;
    
    std::vector<RowErrorChecker> check_rows;
    check_rows = getRowErrorCheckerVector(m_grid, include_empty_fieldnames);

    bool mark_rows = (check_flags & CheckMarkRows);
    bool errors_found = StructureValidator::findInvalidFieldNames(check_rows);
    if (errors_found && mark_rows)
    {
        std::vector<RowErrorChecker>::iterator it;
        for (it = check_rows.begin(); it != check_rows.end(); ++it)
        {
            if (it->errors != StructureValidator::ErrorNone)
                markProblemRow(it->row, false);
        }
    }

    return (errors_found ? StructureValidator::ErrorInvalidFieldNames
                         : StructureValidator::ErrorNone);
}

int QueryDoc::validateStructure()
{
    // CHECK: check for empty structure
    if (m_grid->getRowCount() == 0)
        return StructureValidator::ErrorNoFields;

    // clear rows that have exlamation mark icons in them
    clearProblemRows();

    // CHECK: check for duplicate and invalid field names
    int duplicatefields_errorcode = checkDuplicateFieldnames(
                                        CheckMarkRows | CheckEmptyFieldnames);
    int invalidfields_errorcode = checkInvalidFieldnames(
                                        CheckMarkRows | CheckEmptyFieldnames);
    
    if (duplicatefields_errorcode != StructureValidator::ErrorNone)
    {
        m_grid->refresh(kcl::Grid::refreshAll);
        return duplicatefields_errorcode;
    }
    
    if (invalidfields_errorcode != StructureValidator::ErrorNone)
    {
        m_grid->refresh(kcl::Grid::refreshAll);
        return invalidfields_errorcode;
    }

    m_grid->refreshColumn(kcl::Grid::refreshAll, colOutputField);
    return StructureValidator::ErrorNone;
}

bool QueryDoc::load(const wxString& path)
{
    if (!m_info.load(path))
        return false;
    
    m_path = path;

    // refresh if we are already initialized
    if (m_diagram && m_grid)
    {
        // update caption;
        updateCaption();

        // populate diagram and grid
        populateDiagramFromTemplate();
        populateGridFromTemplate();
        m_grid->refresh(kcl::Grid::refreshAll);
    }

    // fire this event so that the URL will be updated with the new path
    if (m_frame.isOk())
        m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));

    // reset the changed flag and return
    setChanged(false);
    setChangedExecute(true);
    return true;
}

void QueryDoc::onFrameEvent(FrameworkEvent& evt)
{
    // if a file is renamed, update this file with the new file path
    if (evt.name == FRAMEWORK_EVT_TREEPANEL_OFS_FILE_RENAMED)
    {
        if (evt.s_param == m_path)
        {
            m_path = evt.s_param2;

            // update caption
            updateCaption();

            //  fire a frame event    
            IDocumentSitePtr doc_site = m_frame->getActiveChild();
            if (doc_site.isOk() && doc_site == m_doc_site)
            {
                // fire this event so that the URL combobox will be updated
                // with the new path if this is the active child
                m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));
            }
        }
    }

    if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_QUERY_AVAILABLE_VIEW)
    {
        IDocumentSitePtr active_child;
        active_child = g_app->getMainFrame()->getActiveChild();
        
        if (active_child.isNull() || m_doc_site.isNull())
            return;
            
        if (active_child != m_doc_site)
            return;

        if (active_child->getContainerWindow() != m_doc_site->getContainerWindow())
            return;

        // site ptrs to check the active site
        IDocumentSitePtr tabledoc_site;
        IDocumentSitePtr sqldoc_site;
        sqldoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.SqlDoc");
        tabledoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.TableDoc");
        
        // populate the view switcher list with the available views
        ViewSwitcherList* list = (ViewSwitcherList*)(evt.o_param);
        
        // normally, we populate the view switcher from the TableDoc's
        // onFrameEvent() handler, but no tabledoc exists in this container
        // (most likely because we are createing a new query), so make sure
        // we populate the list ourselves
        ITableDocPtr tabledoc = lookupOtherDocument(m_doc_site, "appmain.TableDoc");
        if (tabledoc.isNull())
        {
            list->addItem(ID_View_SwitchToSourceView, _("SQL View"),
                          (sqldoc_site == active_child) ? true : false);
            
            list->addItem(ID_View_SwitchToDesignView, _("Design View"),
                          (m_doc_site == active_child) ? true : false);
                          
            list->addItem(ID_View_SwitchToLayoutView, _("Table View"),
                          (tabledoc_site == active_child) ? true : false);
        }
    }
     else if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGING)
    {
        int id = (int)(evt.l_param);
        
        if (id == ID_View_SwitchToLayoutView)
        {
            // if the query is running, don't allow the view
            // to be changed to the table view
            if (isRunning())
            {
                bool* is_allowed = (bool*)evt.l_param2;
                *is_allowed = false;
                
                appMessageBox(_("A query is already running.  Please wait until the query has finished running or cancel the job."));
                return;
            }
        }
        
        // if there are any errors in the output structure,
        // don't allow the view to be changed
        if (!doErrorCheck())
        {
            bool* is_allowed = (bool*)evt.l_param2;
            *is_allowed = false;
            return;
        }
    }
     else if (evt.name == FRAMEWORK_EVT_APPMAIN_VIEW_SWITCHER_ACTIVE_VIEW_CHANGED)
    {
        int id = (int)(evt.l_param);
        
        // make sure we are in the active container
        IDocumentSitePtr active_site;
        active_site = g_app->getMainFrame()->getActiveChild();
        if (active_site.isNull() || m_doc_site.isNull())
            return;
        if (active_site->getContainerWindow() != m_doc_site->getContainerWindow())
            return;
        
        if (id == ID_View_SwitchToSourceView)
        {
            if (active_site == m_doc_site)
            {
                if (m_path.Length() > 0 && isChanged())
                    doSave();
            }
            
            ISqlDocPtr sqldoc = lookupOtherDocument(m_doc_site, "appmain.SqlDoc");

            // we don't have a sql doc yet, create one
            if (sqldoc.isNull())
                createSqlDoc();
                 else
                updateSqlDoc();
            
            switchToOtherDocument(m_doc_site, "appmain.SqlDoc");
            return;
        }
         else if (id == ID_View_SwitchToLayoutView)
        {
            // if we are on structure doc, we might need to prompt for saving
            IDocumentSitePtr tabledoc_site, sqldoc_site;
            
            sqldoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.SqlDoc");
            tabledoc_site = lookupOtherDocumentSite(m_doc_site, "appmain.TableDoc");
            active_site = g_app->getMainFrame()->getActiveChild();

            if (active_site == m_doc_site || active_site == sqldoc_site)
            {
                if (!isChangedExecute() && tabledoc_site.isOk())
                {
                    // query has not changed and tabledoc already exists;
                    // just switch to the other tabledoc
                    switchToOtherDocument(m_doc_site, "appmain.TableDoc");
                    return;
                }
                
                if (m_path.Length() > 0 && isChanged())
                    doSave();
                
                // if we don't have a tabledoc yet, we need
                // to execute the query (the tabledoc will be
                // created when the query job is finished)
                if (tabledoc_site.isNull() || isChangedExecute())
                {
                    execute();
                }
            }
        }
    }
}

bool QueryDoc::createSqlDoc()
{
    // try to do a lookup of an existing SqlDoc
    ISqlDocPtr sqldoc = lookupOtherDocument(m_doc_site, "appmain.SqlDoc");
    if (sqldoc.isOk())
    {
        updateSqlDoc();
        return true;
    }
    
    {
        AppBusyCursor bc;
        
        // make sure we're done editing
        m_grid->endEdit(true);
        populateTemplateFromInterface();
        
        SqlDoc* doc = new SqlDoc;
        doc->setText(m_info.getQueryString());
        
        wxWindow* container = m_doc_site->getContainerWindow();
        
        IDocumentSitePtr site;
        site = g_app->getMainFrame()->createSite(container,
                                         static_cast<IDocument*>(doc),
                                         true);
        site->setVisible(true);
    }
    
    return true;
}

bool QueryDoc::updateSqlDoc()
{
    ISqlDocPtr sqldoc = lookupOtherDocument(m_doc_site, "appmain.SqlDoc");
    if (sqldoc.isNull())
    {
        createSqlDoc();
        return true;
    }

    // make sure we're done editing
    m_grid->endEdit(true);
    populateTemplateFromInterface();

    sqldoc->setText(m_info.getQueryString());
    return true;
}


// this function encapsulates all of the logic/error checking
// for when we want to save a document or switch views
bool QueryDoc::doErrorCheck()
{
    bool block = false;
    int errorcode = validateStructure();
    StructureValidator::showErrorMessage(errorcode, &block);
    
    // there is an error in the structure that must be fixed
    if (block)
        return false;
    
    return true;
}


bool QueryDoc::doSave(bool force)
{
    if (m_grid->isEditing())
        m_grid->endEdit(true);
    
    if (!doErrorCheck())
        return false;
    
    bool new_save = false;
    if (m_path.IsEmpty() || force)
    {
        DlgDatabaseFile dlg(m_doc_site->getContainerWindow(), DlgDatabaseFile::modeSave);
        dlg.setCaption(_("Save As"));
        dlg.setOverwritePrompt(true);
        if (dlg.ShowModal() != wxID_OK)
            return false;
            
        m_path = dlg.getPath();
        new_save = true;

        // fire this event so that the URL will be updated with the new path
        if (m_frame.isOk())
            m_frame->postEvent(new FrameworkEvent(FRAMEWORK_EVT_CFW_LOCATION_CHANGED));
    }
    
    // put the work from the UI into the template
    populateTemplateFromInterface();

    // try to save the template
    if (!m_info.save(m_path))
    {
        appMessageBox(_("There was an error saving the specified query."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return false;
    }

    wxString caption = m_path.AfterLast(wxT('/'));
        
    // update the project panel and the document caption
    if (new_save)
    {
        updateCaption();
        g_app->getAppController()->refreshDbDoc();
    }

    // reset the changed flag and return
    setChanged(false);

    return true;
}

void QueryDoc::onSave(wxCommandEvent& evt)
{
    doSave();
}

void QueryDoc::onSaveAs(wxCommandEvent& evt)
{
    m_path = wxT("");
    doSave();
}

void QueryDoc::onSaveAsExternal(wxCommandEvent& evt)
{
    wxString filter;

    // NOTE: if you add or remove items from this
    // list, make sure you adjust the case statement
    // below, because it affects which file type the
    // target will be
    filter += _("Package Files");
    filter += wxT(" (*.kpg)|*.kpg|");
    filter.RemoveLast(); // get rid of the last pipe sign

    wxString filename = getFilenameFromPath(m_path, false);    
    wxFileDialog dlg(g_app->getMainWindow(),
                     _("Save As"),
                     wxT(""),
                     filename,
                     filter,
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() != wxID_OK)
        return;

    int type = 0;
    
    switch (dlg.GetFilterIndex())
    {
        case 0: type = dbtypePackage; break;
        default:
            wxFAIL_MSG(wxT("invalid filter index"));
            return;
    }
    
    
    
    // create an export job
    
    if (type == dbtypePackage)
    {
        ExportPkgJob* job = new ExportPkgJob;
        job->setPkgFilename(dlg.GetPath(), ExportPkgJob::modeOverwrite);

        job->addExportObject(m_doc_site->getCaption(),
                             m_path,
                             true /* compress */);

        g_app->getJobQueue()->addJob(job, jobStateRunning);
    }
     else
    {
        // empty right now
    }
}


void QueryDoc::onExecute(wxCommandEvent& evt)
{
    execute();
}

bool QueryDoc::isRunning()
{
    return (m_job_info.isOk() && m_job_info->getState() == jobStateRunning) ? true : false;
}

bool QueryDoc::execute()
{
    m_grid->endEdit(true);
    
    populateTemplateFromInterface();
    m_info.updateValidationStructure();

    std::vector<wxString> boxes;
    std::vector<wxString> top_paths;

    // find out which boxes are open in the diagram
    m_diagram->getBoxPaths(boxes);


    // find "top" sets
    std::vector<wxString>::iterator box_it;
    
    for (box_it = boxes.begin(); box_it != boxes.end(); ++box_it)
    {
        std::vector<RelationLine*> lines;
        m_diagram->getLines(wxEmptyString,
                            *box_it,
                            lines);

        if (lines.size() == 0)
            top_paths.push_back(*box_it);
    }

    if (top_paths.size() == 0)
    {
        appMessageBox(_("Please specify a primary selection table."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return false;
    }
    
    // in the future, we should support more than one source table,
    // allowing for cross joins and other features

    if (top_paths.size() != 1)
    {
        appMessageBox(_("Please specify at the most one primary selection table."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return false;
    }


    if (m_info.m_params.size() == 0)
    {
        appMessageBox(_("No query parameters are currently specified.  Please add at least one parameter to the query definition."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return false;
    }

    // this function does all of the general error checking for
    // the grid area of the query (duplicate output field names, etc.)
    if (!doErrorCheck())
        return false;

    for (box_it = boxes.begin();
         box_it != boxes.end(); ++box_it)
    {
        std::vector<RelationLine*> lines;

        m_diagram->getLines(wxEmptyString,
                            *box_it,
                            lines);

        if (lines.size() == 0)
            top_paths.push_back(*box_it);
    }


    std::set<wxString> unique_fields;
    int row = 0;

    std::vector<QueryBuilderParam>::iterator it;
    for (it = m_info.m_params.begin();
         it != m_info.m_params.end(); ++it)
    {
        if (it->output && 
            it->group_func != QueryGroupFunction_Count &&
            it->group_func != QueryGroupFunction_GroupID &&
            it->input_expr.empty())
        {
            appMessageBox(_("An input field or formula is missing."),
                               APPLICATION_NAME,
                               wxICON_EXCLAMATION | wxOK);
            return false;
        }

        // do a check

        wxString field_name;

        if (!it->output_field.empty())
            field_name = it->output_field;
             else
            field_name = it->input_expr;

        field_name.MakeUpper();
        
        // this check is already happening in the
        // doErrorCheck() function above
        /*
        if (it->output && unique_fields.find(field_name) != unique_fields.end())
        {
            m_grid->setCellBitmap(row,
                                  colOutputField,
                                  GETBMP(gf_exclamation_16));
            m_grid->refresh(kcl::Grid::refreshAll);

            appMessageBox(_("Two or more fields have the same output name.  Please make sure that the name of each output field is unique."),
                               APPLICATION_NAME,
                               wxOK | wxICON_EXCLAMATION | wxCENTER);
            return false;
        }
        */
        
        if (it->output)
            unique_fields.insert(field_name);

        ++row;
    }



    if (m_info.m_output_path.length() > 0)
    {
        if (!doOutputPathCheck(m_info.m_output_path))
            return false;
    }



    // check output order for errors
    int order;
    for (order = 1; order <= 50; ++order)
    {
        bool found = false;
        
        int param_idx = 0;
        int sort_order = 0;
        int p = 0;

        for (it = m_info.m_params.begin();
             it != m_info.m_params.end();
             ++it)
        {
            if (abs(it->sort_order) == order)
            {
                if (found)
                {
                    // two identical sort specifiers found: this is not allowed
                    appMessageBox(_("The sort order has an error.  Two or more identical sort specifiers have been found."),
                                       APPLICATION_NAME,
                                       wxOK | wxICON_EXCLAMATION | wxCENTER);
                    return false;
                }

                found = true;
            }

            ++p;
        }                
    }


    // check the condition string to make sure it works

    size_t i;
    for (i = 0; i < 255; ++i)
    {
        int row = 0;

        for (it = m_info.m_params.begin();
             it != m_info.m_params.end();
             ++it)
        {
            wxString piece = wxT("");

            if (i >= 0 && i < it->conditions.size())
            {
                piece = it->conditions[i];
            }

            if (it->group_func == QueryGroupFunction_None &&
                !piece.IsEmpty())
            {
                wxString out = m_info.completeFilter(piece, it->input_expr);
                if (out.IsEmpty())
                {
                    m_grid->setCellBitmap(row,
                                          i+colCondition,
                                          GETBMP(gf_exclamation_16));

                    m_grid->refresh(kcl::Grid::refreshAll);

                    // two identical sort specifiers found: this is not allowed
                    appMessageBox(_("The query condition contains an error."),
                                       APPLICATION_NAME,
                                       wxOK | wxICON_EXCLAMATION | wxCENTER);
                    return false;
                }
            }

            row++;
        }
    }


    // create query string and execute
    wxString sql = m_info.getQueryString();
    if (sql.IsEmpty())
    {
        appMessageBox(_("The constructed SQL statement is empty."),
                           APPLICATION_NAME,
                           wxOK | wxICON_EXCLAMATION | wxCENTER);
        return false;
    }
    
    setChangedExecute(false);

    jobs::IJobPtr job = m_info.execute(m_doc_site->getId());
    if (job.isOk())
    {
        m_job_info = job->getJobInfo();
        return true;
    }
     else
    {
        m_job_info.clear();
    }
    
    return false;
}


