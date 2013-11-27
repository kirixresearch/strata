/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2002-03-28
 *
 */


#include "appmain.h"
#include "panelmarkmgr.h"
#include "tabledoc.h"
#include "exprbuilder.h"
#include "appcontroller.h"
#include "panelcolordoc.h"


enum
{
    ID_AddMark = 10000,
    ID_DeleteMark,
    ID_EditMark,
    ID_ChangeMarkColor,
    ID_FilterRows,
    ID_ReplaceRows,
    ID_CopyRows,
    ID_DeleteRows
};


enum
{
    colActive = 0,
    colColor = 1,
    colDescription = 2,
    colEdit = 3
};


const int CHECKBOX_COL_WIDTH = 25;
const int COLOR_COL_WIDTH = 40;
const int EDITBUTTON_COL_WIDTH = 35;


// utility functions

static ITableDocPtr getActiveTableDoc()
{
    IDocumentSitePtr site = g_app->getMainFrame()->getActiveChild();
    if (site.isNull())
        return xcm::null;
    
    ITableDocPtr doc = site->getDocument();
    return doc;
}

static ITableDocModelPtr getActiveTableDocModel()
{
    ITableDocPtr doc = getActiveTableDoc();
    if (doc.isNull())
        return xcm::null;
    
    return doc->getModel();
}

static ITableDocMarkEnumPtr getActiveTableDocMarks()
{
    ITableDocModelPtr model = getActiveTableDocModel();
    if (model.isNull())
    {
        xcm::IVectorImpl<ITableDocMarkPtr>* vec;
        vec = new xcm::IVectorImpl<ITableDocMarkPtr>;
        return vec;
    }
    
    return model->getMarkEnum();
}




BEGIN_EVENT_TABLE(MarkMgrPanel, wxPanel)
    EVT_MENU(ID_AddMark, MarkMgrPanel::onAddMark)
    EVT_MENU(ID_EditMark, MarkMgrPanel::onEditMark)
    EVT_MENU(ID_DeleteMark, MarkMgrPanel::onDeleteMark)
    EVT_MENU(ID_FilterRows, MarkMgrPanel::onFilterRows)
    EVT_MENU(ID_CopyRows, MarkMgrPanel::onCopyRows)
    EVT_MENU(ID_ReplaceRows, MarkMgrPanel::onReplaceRows)
    EVT_MENU(ID_DeleteRows, MarkMgrPanel::onDeleteRows)
    EVT_KCLGRID_CELL_RIGHT_CLICK(MarkMgrPanel::onGridCellRightClick)
    EVT_KCLGRID_BEGIN_EDIT(MarkMgrPanel::onGridBeginEdit)
    EVT_KCLGRID_MODEL_CHANGE(MarkMgrPanel::onGridModelChange)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(MarkMgrPanel::onGridPreGhostRowInsert)
    EVT_KCLGRID_PRE_INVALID_AREA_INSERT(MarkMgrPanel::onGridPreInvalidAreaInsert)
    EVT_KCLGRID_BUTTON_CLICK(MarkMgrPanel::onGridEditExpressionClick)
    EVT_KCLGRID_COLOR_CLICK(MarkMgrPanel::onGridColorClick)
    EVT_SIZE(MarkMgrPanel::onSize)
    EVT_SET_FOCUS(MarkMgrPanel::onSetFocus)
    EVT_KILL_FOCUS(MarkMgrPanel::onKillFocus)
END_EVENT_TABLE()


MarkMgrPanel::MarkMgrPanel()
{
    m_grid = NULL;
    m_builder = NULL;
    m_color_panel = NULL;
    m_color_panel_cont = NULL;
    m_ghost_inserting = false;
    m_fgcolor_editing = false;
    
    m_cli_width = 0;
    m_cli_height = 0;
    
    m_color_column = 0;
    m_edit_mark = 0;
}

MarkMgrPanel::~MarkMgrPanel()
{
}



bool MarkMgrPanel::initDoc(IFramePtr frame,
                           IDocumentSitePtr doc_site,
                           wxWindow* docsite_wnd,
                           wxWindow* panesite_wnd)
{
    if (!Create(docsite_wnd,
                -1,
                wxDefaultPosition,
                wxDefaultSize,
                wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN))
    {
        return false;
    }
    
    // set the caption
    doc_site->setCaption(_("Marks"));
    
    m_frame = frame;
    m_doc_site = doc_site;
    
    SetWindowStyle(GetWindowStyle() & ~wxTAB_TRAVERSAL);
    SetBackgroundColour(*wxWHITE);
    
    GetClientSize(&m_cli_width, &m_cli_height);
    
    // create the grid
    m_grid = new kcl::RowSelectionGrid(this,
                                       -1,
                                       wxDefaultPosition,
                                       docsite_wnd->GetClientSize(),
                                       wxBORDER_NONE,
                                       false, false);
    m_grid->setAllowInvalidAreaInserts(true);
    m_grid->setAllowDeletes(true);
    m_grid->setOptionState(kcl::Grid::optColumnResize |
                           kcl::Grid::optEditBoxBorder, false);
    m_grid->setOptionState(kcl::Grid::optGhostRow |
                           kcl::Grid::optInvalidAreaClickable, true);
    
    m_grid->createModelColumn(colActive, wxEmptyString, kcl::Grid::typeBoolean, 0, 0);
    m_grid->createModelColumn(colColor, _("Color"), kcl::Grid::typeCharacter, 1, 0);
    m_grid->createModelColumn(colDescription, _("Description"), kcl::Grid::typeCharacter, 50, 0);
    m_grid->createModelColumn(colEdit, _("Formula"), kcl::Grid::typeCharacter, 1024, 0);
    m_grid->createDefaultView();
    
    m_grid->setHeaderSize(0);
    m_grid->setRowHeight(30);
    
    int description_col_width = m_cli_width;
    description_col_width -= CHECKBOX_COL_WIDTH;
    description_col_width -= COLOR_COL_WIDTH;
    description_col_width -= EDITBUTTON_COL_WIDTH;
    
    m_grid->setColumnSize(colActive, CHECKBOX_COL_WIDTH);
    m_grid->setColumnSize(colColor, COLOR_COL_WIDTH);
    m_grid->setColumnSize(colEdit, EDITBUTTON_COL_WIDTH);
    m_grid->setColumnProportionalSize(colDescription, 1);
    
    // set cell properties for the grid
    kcl::CellProperties cellprops;
    cellprops.mask = kcl::CellProperties::cpmaskAlignment;
    cellprops.alignment = kcl::Grid::alignRight;
    m_grid->setModelColumnProperties(colActive, &cellprops);
    
    cellprops.mask = kcl::CellProperties::cpmaskCtrlType;
    cellprops.ctrltype = kcl::Grid::ctrltypeButton;
    m_grid->setModelColumnProperties(colEdit, &cellprops);
    
    m_grid->setColumnTextWrapping(colDescription, kcl::Grid::wrapOn);
    
    // create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_grid, 1, wxEXPAND, 0);
    SetSizer(main_sizer);
    SetAutoLayout(true);
    
    // connect signals
    frame->sigFrameEvent().connect(this, &MarkMgrPanel::onFrameEvent);
    frame->sigActiveChildChanged().connect(this, &MarkMgrPanel::onActiveChildChanged);
    m_grid->sigDeletingRows.connect(this, &MarkMgrPanel::onDeletingRows);
    m_grid->sigDeletedRows.connect(this, &MarkMgrPanel::onDeletedRows);
    
    // populate, refresh and show the grid
    populate(false);
    m_grid->refresh(kcl::Grid::refreshAll);
    return true;
}

wxWindow* MarkMgrPanel::getDocumentWindow()
{
    return static_cast<wxWindow*>(this);
}

void MarkMgrPanel::setDocumentFocus()
{
    if (m_grid)
        m_grid->SetFocus();
}

void MarkMgrPanel::onActiveChildChanged(IDocumentSitePtr doc_site)
{
    m_grid->endEdit(false);
    
    checkOverlayText();
    populate();
}

void MarkMgrPanel::onFrameEvent(FrameworkEvent& evt)
{
    if (evt.name == FRAMEWORK_EVT_TABLEDOC_MARK_CREATED)
    {
        Freeze();
        populate(false);
        m_grid->clearSelection();
        m_grid->setRowSelected(m_grid->getRowCount()-1, true);
        m_grid->refresh(kcl::Grid::refreshAll);
        Thaw();
    }
}

void MarkMgrPanel::onDeletingRows(std::vector<int> rows, bool* allow)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    ITableDocModelPtr model = getActiveTableDocModel();
    if (model.isNull())
        return;
    
    ITableDocMarkEnumPtr marks = getActiveTableDocMarks();
    
    std::vector<int>::reverse_iterator it;
    for (it = rows.rbegin(); it != rows.rend(); ++it)
    {
        ITableDocMarkPtr mark = marks->getItem(*it);
        if (mark.isNull())
            continue;
        
        model->deleteObject(mark);
        marks->deleteAt(*it);
    }
    
    // refresh the tabledoc's grid
    tabledoc->getGrid()->refresh(kcl::Grid::refreshAll);
}

void MarkMgrPanel::onDeletedRows(std::vector<int> rows)
{
    // NOTE: the kcl::RowSelectionGrid is in the middle of deleting rows,
    //       so it will take care of the grid refresh for us
    checkOverlayText();
}

void MarkMgrPanel::populateRow(int row, ITableDocMarkPtr mark)
{
    // set mark state
    m_grid->setCellBoolean(row, colActive, mark->getMarkActive());

    // set color
    kcl::CellProperties props;
    props.mask = kcl::CellProperties::cpmaskCtrlType |
                 kcl::CellProperties::cpmaskBgColor |
                 kcl::CellProperties::cpmaskFgColor |
                 kcl::CellProperties::cpmaskAlignment;

    props.ctrltype = kcl::Grid::ctrltypeColor;
    props.alignment = kcl::Grid::alignCenter;
    props.fgcolor = mark->getForegroundColor();
    props.bgcolor = mark->getBackgroundColor();

    m_grid->setCellProperties(row, colColor, &props);
    m_grid->setCellString(row, colColor, wxEmptyString);
    m_grid->setCellString(row, colDescription, mark->getDescription());
    m_grid->setCellBitmap(row, colEdit, GETBMP(gf_function_16), kcl::Grid::alignCenter);
}

void MarkMgrPanel::populate(bool refresh)
{
    // store the current cursor row
    int row = m_grid->getCursorRow();
    
    // delete all rows in the grid
    m_grid->deleteAllRows();
    
    // if we don't have a tabledoc to reference,
    // we can't show any marks in the grid
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
    {
        checkOverlayText();
        m_grid->refresh(kcl::Grid::refreshAll);
        return;
    }
    
    // get the marks vector and populate the grid
    ITableDocMarkEnumPtr marks = getActiveTableDocMarks();
    int i, mark_count = marks->size();
    for (i = 0; i < mark_count; i++)
    {
        m_grid->insertRow(-1);
        populateRow(i, marks->getItem(i));
    }
    
    // restore the cursor row's position
    if (row >= mark_count)
        m_grid->moveCursor(mark_count-1, colActive);
         else
        m_grid->moveCursor(row, colActive);
    
    // check the overlay text
    checkOverlayText();
    
    // refresh the grid
    if (refresh)
        m_grid->refresh(kcl::Grid::refreshAll);
}

void MarkMgrPanel::checkOverlayText()
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
    {
        m_grid->setOverlayText(wxEmptyString);
        return;
    }
    
    if (m_grid->getRowCount() == 0)
        m_grid->setOverlayText(_("To add a mark,\ndouble-click here"));
         else
        m_grid->setOverlayText(wxEmptyString);
}

wxString MarkMgrPanel::getUniqueMarkName()
{
    wxString base = _("Mark #");
    wxString retval;
    
    int row, row_count = m_grid->getRowCount();
    int counter = row_count+1;
    
    while (1)
    {
        retval = base + wxString::Format(wxT("%d"), counter);
        bool dup = false;
        
        for (row = 0; row < row_count; ++row)
        {
            wxString name = m_grid->getCellString(row, colDescription);
            if (retval.CmpNoCase(name) == 0)
            {
                dup = true;
                break;
            }
        }
        
        counter++;
        
        // index name is unique, we're done
        if (!dup)
            break;
    }
    
    return retval;
}

void MarkMgrPanel::onExprEditFinished(wxString text)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    ITableDocModelPtr model = getActiveTableDocModel();
    if (model.isNull())
        return;
    
    ITableDocMarkEnumPtr marks = getActiveTableDocMarks();
    ITableDocMarkPtr mark = marks->getItem(m_edit_mark);
    if (mark.isNull())
        return;
    
    // save the mark's new expression
    mark->setExpression(towstr(text));
    model->writeObject(mark);
    
    // refresh the mark panel's grid
    m_grid->refresh(kcl::Grid::refreshAll);
    
    // refresh the tabledoc's grid
    tabledoc->getGrid()->refresh(kcl::Grid::refreshAll);
    m_builder = NULL;
}

void MarkMgrPanel::onExprEditCancelled()
{
    m_builder = NULL;
    
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
}

void MarkMgrPanel::editMark(int mark_idx)
{
    if (m_builder != NULL)
        return;
    
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    m_edit_mark = mark_idx;
    ITableDocMarkEnumPtr marks = getActiveTableDocMarks();
    ITableDocMarkPtr mark = marks->getItem(m_edit_mark);
    if (mark.isNull())
        return;
    
    xd::IIteratorPtr iter = tabledoc->getIterator();
    if (iter.isNull())
        return;
    
    if (mark_idx < 0 || mark_idx >= m_grid->getRowCount())
        return;
    
    wxString expr = mark->getExpression();
    wxString set_path = tabledoc->getPath();
    wxString caption = wxString::Format(_("Mark - [%s] - %s"),
                                        set_path.c_str(),
                                        mark->getDescription().c_str());
    
    m_builder = new DlgExprBuilder(this, caption);
    m_builder->SetMinSize(wxSize(560, 300));
    m_builder->SetMaxSize(wxSize(680, 420));
    m_builder->setTypeOnly(xd::typeBoolean);
    m_builder->sigExprEditFinished.connect(this, &MarkMgrPanel::onExprEditFinished);
    m_builder->sigExprEditCancelled.connect(this, &MarkMgrPanel::onExprEditCancelled);
    m_builder->setStructure(iter->getStructure());
    m_builder->setExpression(expr);
    m_builder->Show(true);
}

void MarkMgrPanel::onEditMark(wxCommandEvent& evt)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    if (m_grid->isEditing())
        m_grid->endEdit(true);

    if (m_grid->getCursorRow() >= m_grid->getRowCount())
    {
        wxCommandEvent unused;
        onAddMark(unused);
    }
    
    editMark(m_grid->getCursorRow());
}

void MarkMgrPanel::onAddMark(wxCommandEvent& evt)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    ITableDocModelPtr model = getActiveTableDocModel();
    if (model.isNull())
        return;
    
    if (m_grid->isEditing())
        m_grid->endEdit(true);
    
    // add a mark
    int row = m_grid->getRowCount();
    ITableDocMarkPtr mark = model->createMarkObject();
    mark->setBackgroundColor(model->getNextMarkColor());
    mark->setDescription(towstr(getUniqueMarkName()));
    mark->setExpression(wxEmptyString);
    model->writeObject(mark);
    
    // refresh the tabledoc's grid
    tabledoc->getGrid()->refresh(kcl::Grid::refreshAll);

    int old_rowoffset = m_grid->getRowOffset();
    
    // we want to eliminate flicker when we add a mark,
    // so freeze and thaw (below) this window
    Freeze();
    
    // repopulate
    populate(false);

    m_grid->moveCursor(row, colDescription);
    m_grid->setRowOffset(old_rowoffset);
    m_grid->setCellString(row, colDescription, mark->getDescription());
    m_grid->clearSelection();
    m_grid->setRowSelected(row, true);
    m_grid->scrollVertToCursor();
    m_grid->refresh(kcl::Grid::refreshAll);
    Thaw();
    
    editMark(row);
}

void MarkMgrPanel::onGridBeginEdit(kcl::GridEvent& evt)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    // if we double-click on the invalid area of the grid, it will
    // automatically insert a row and start editing the cell where we
    // double-clicked, and we don't want to do the edit in that case
    if (m_ghost_inserting)
    {
        m_ghost_inserting = false;
        evt.Veto();
    }
}

void MarkMgrPanel::onGridPreGhostRowInsert(kcl::GridEvent& evt)
{
    m_ghost_inserting = true;

    // we'll handle the processing of this event ourselves
    evt.Veto();

    // add a mark to the end of the grid
    wxCommandEvent unused;
    onAddMark(unused);
}

void MarkMgrPanel::onGridPreInvalidAreaInsert(kcl::GridEvent& evt)
{
    m_ghost_inserting = true;
    
    // we'll handle the processing of this event ourselves
    evt.Veto();

    // add a mark to the end of the grid
    wxCommandEvent unused;
    onAddMark(unused);
}

void MarkMgrPanel::onGridModelChange(kcl::GridEvent& evt)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    ITableDocModelPtr model = getActiveTableDocModel();
    if (model.isNull())
        return;
    
    int row = evt.GetRow();
    int model_col = evt.GetColumn();
    
    ITableDocMarkEnumPtr marks = getActiveTableDocMarks();
    ITableDocMarkPtr mark = marks->getItem(row);
    if (mark.isNull())
        return;
    
    if (model_col == colActive)
    {
        bool new_val = m_grid->getCellBoolean(row, model_col);
        mark->setMarkActive(new_val);
        model->writeObject(mark);
    }
     else if (model_col == colDescription)
    {
        wxString new_val = m_grid->getCellString(row, model_col);
        mark->setDescription(towstr(new_val));
        model->writeObject(mark);
    }
    
    // refresh the tabledoc's grid
    tabledoc->getGrid()->refresh(kcl::Grid::refreshAll);
}

void MarkMgrPanel::onGridEditExpressionClick(kcl::GridEvent& evt)
{
    editMark(evt.GetRow());
}

void MarkMgrPanel::onGridColorClick(kcl::GridEvent& evt)
{
    // this really should be handled by the grid, but i'm just
    // too lazy to take care of it right now, and this is the
    // only place we have a color control in the grid
    m_grid->moveCursor(evt.GetRow(), evt.GetColumn());
    
    int col = evt.GetColumn();
    if (col != colColor)
        return;
    
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    ITableDocModelPtr model = getActiveTableDocModel();
    if (model.isNull())
        return;
    
    // find out if we clicked the foreground or background color
    m_fgcolor_editing = evt.GetBoolean();
    
    // if we had an old color panel, destroy it before we pop up a new one
    if (m_color_panel)
    {
        m_color_panel_cont->Destroy();
        m_color_panel = NULL;
    }
    
    // create the color panel container
    m_color_panel_cont = new kcl::PopupContainer(::wxGetMousePosition());
    
    // layout the color panel properly
    if (m_fgcolor_editing)
    {
        m_color_panel = new kcl::ColorPanel(
            m_color_panel_cont, 
            kcl::ColorPanelLayout(kcl::ColorPanelLayout::DefaultColorButton,
                                  kcl::ColorPanelLayout::ColorGrid));
        
        ITableDocMarkPtr temp = model->createMarkObject();
        m_color_panel->setDefaultColor(temp->getForegroundColor());
        m_color_panel->setMode(kcl::ColorPanel::ModeText);
    }
     else
    {
        m_color_panel = new kcl::ColorPanel(
            m_color_panel_cont, 
            kcl::ColorPanelLayout(kcl::ColorPanelLayout::NoColorButton,
                                  kcl::ColorPanelLayout::ColorGrid));
        
        ITableDocMarkPtr temp = model->createMarkObject();
        m_color_panel->setDefaultColor(temp->getBackgroundColor());
        m_color_panel->setMode(kcl::ColorPanel::ModeFill);
    }
    
    // connect the color panel's signals
    m_color_panel->sigDestructing.connect(this, &MarkMgrPanel::onColorPanelDestructing);
    m_color_panel->sigColorSelected.connect(this, &MarkMgrPanel::onColorSelected);
    m_color_panel->sigCustomColorsRequested.connect(g_app->getAppController(),
                                  &AppController::onCustomColorsRequested);
    m_color_panel->sigCustomColorsChanged.connect(g_app->getAppController(),
                                  &AppController::onCustomColorsChanged);
    
    // show the color panel
    m_color_panel_cont->doPopup(m_color_panel);
}

void MarkMgrPanel::onGridCellRightClick(kcl::GridEvent& evt)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;

    wxPoint pt_mouse = ::wxGetMousePosition();
    pt_mouse = ScreenToClient(pt_mouse);

    if (m_color_panel)
    {
        m_color_panel_cont->Destroy();
        m_color_panel = NULL;
    }

    int row = evt.GetRow();
    int col = evt.GetColumn();
    
    if (row < 0 || row >= m_grid->getRowCount() ||
        col < 0 || col >= m_grid->getColumnCount())
        return;

    m_grid->moveCursor(row, col);
    
    // the user clicked on a row that was not previously selected
    if (!m_grid->isRowSelected(row))
    {
        m_grid->clearSelection();
        m_grid->setRowSelected(row, true);
        m_grid->refresh(kcl::Grid::refreshAll);
    }

    wxMenu menuPopup;
    
    if (m_grid->getSelectedRowCount() > 1)
    {
        menuPopup.Append(ID_DeleteMark, _("Delete &Marks"));
    }
     else
    {
        menuPopup.Append(ID_AddMark, _("&Add Mark..."));
        menuPopup.Append(ID_EditMark, _("&Edit Mark..."));

        if (m_builder == NULL)
        {
            menuPopup.AppendSeparator();
            menuPopup.Append(ID_DeleteMark, _("Delete &Mark"));
        }
        
        menuPopup.AppendSeparator();
        menuPopup.Append(ID_FilterRows, _("&Filter Records"));
        menuPopup.Append(ID_CopyRows, _("C&opy Records"));
        menuPopup.Append(ID_DeleteRows, _("&Delete Records"));
        menuPopup.Append(ID_ReplaceRows, _("&Update Records"));

        // don't allow replaces or deletes on child sets
        if (tabledoc->getIsChildSet())
        {
            menuPopup.Enable(ID_DeleteRows, false);
            menuPopup.Enable(ID_ReplaceRows, false);
        }
    }
    
    PopupMenu(&menuPopup, pt_mouse);
}

void MarkMgrPanel::onDeleteMark(wxCommandEvent& evt)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;

    m_grid->endEdit(false);
    
    // calling this here will actually have the kcl::RowSelectionGrid
    // fire an event back to the MarkManagerPanel so all mark deleting
    // happens in the same place (in MarkMgrPanel::onDeletingRows)
    m_grid->deleteSelectedRows();
}

void MarkMgrPanel::onFilterRows(wxCommandEvent& evt)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    int row = m_grid->getCursorRow();
    ITableDocMarkEnumPtr marks = getActiveTableDocMarks();
    ITableDocMarkPtr mark = marks->getItem(row);
    if (mark.isNull())
        return;

    std::wstring expr = mark->getExpression();
    if (expr.length() > 0)
        tabledoc->setFilter(expr);
}

void MarkMgrPanel::onCopyRows(wxCommandEvent& evt)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    int row = m_grid->getCursorRow();
    ITableDocMarkEnumPtr marks = getActiveTableDocMarks();
    ITableDocMarkPtr mark = marks->getItem(row);
    if (mark.isNull())
        return;
    
    std::wstring expr = mark->getExpression();
    if (expr.length() > 0)
        tabledoc->copyRecords(expr);
}

void MarkMgrPanel::onReplaceRows(wxCommandEvent& evt)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    int row = m_grid->getCursorRow();
    ITableDocMarkEnumPtr marks = getActiveTableDocMarks();
    ITableDocMarkPtr mark = marks->getItem(row);
    if (mark.isNull())
        return;
    
    wxString expr = mark->getExpression();
    if (expr.Length() > 0)
        tabledoc->showReplacePanel(expr);
}

void MarkMgrPanel::onDeleteRows(wxCommandEvent& evt)
{
    if (!g_app->getAppController()->doReadOnlyCheck())
        return;
    
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    int row = m_grid->getCursorRow();
    ITableDocMarkEnumPtr marks = getActiveTableDocMarks();
    ITableDocMarkPtr mark = marks->getItem(row);
    if (mark.isNull())
        return;
    
    std::wstring expr = mark->getExpression();
    if (expr.length() > 0)
    {
        wxString message = wxString::Format(_("Performing this operation will permanently delete data.  Are you sure\nyou want to delete the records that meet the selected condition?"));

        int res = wxMessageBox(message,
                               APPLICATION_NAME,
                               wxYES_NO | wxICON_EXCLAMATION | wxCENTER);
        
        if (res == wxYES)
            tabledoc->deleteRecords(expr);
    }
}

void MarkMgrPanel::onSize(wxSizeEvent& evt)
{
    GetClientSize(&m_cli_width, &m_cli_height);
    Layout();
}

void MarkMgrPanel::onSetFocus(wxFocusEvent& evt)
{
    setDocumentFocus();
}

void MarkMgrPanel::onKillFocus(wxFocusEvent& evt)
{
}


// manage color panel events

void MarkMgrPanel::onColorPanelDestructing(kcl::ColorPanel* panel)
{
    m_color_panel = NULL;
}

void MarkMgrPanel::onColorSelected(wxColour color, int mode)
{
    ITableDocPtr tabledoc = getActiveTableDoc();
    if (tabledoc.isNull())
        return;
    
    ITableDocModelPtr model = getActiveTableDocModel();
    if (model.isNull())
        return;

    int row = m_grid->getCursorRow();
    ITableDocMarkEnumPtr marks = getActiveTableDocMarks();
    ITableDocMarkPtr mark = marks->getItem(row);
    if (mark.isNull())
        return;
    
    if (m_fgcolor_editing)
    {
        // save the mark foreground color
        mark->setForegroundColor(color);
        model->writeObject(mark);
        
        // set the mark foreground color in the mark panel's grid
        kcl::CellProperties props;
        props.mask = kcl::CellProperties::cpmaskFgColor;
        props.fgcolor = color;
        m_grid->setCellProperties(row, colColor, &props);
    }
     else
    {
        // save the mark background color
        mark->setBackgroundColor(color);
        model->writeObject(mark);

        // set the mark background color in the mark panel's grid
        kcl::CellProperties props;
        props.mask = kcl::CellProperties::cpmaskBgColor;
        props.bgcolor = color;
        m_grid->setCellProperties(row, colColor, &props);
    }
    
    // refresh the mark panel's grid
    m_grid->refresh(kcl::Grid::refreshAll);
    
    // refresh the tabledoc's grid
    tabledoc->getGrid()->refresh(kcl::Grid::refreshAll);
    
    // destroy the color panel popup
    m_color_panel_cont->Show(false);
    m_color_panel_cont->Destroy();
}




