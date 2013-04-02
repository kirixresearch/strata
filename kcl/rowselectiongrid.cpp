/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2006-11-07
 *
 */


#include "rowselectiongrid.h"
#include "griddnd.h"
#include "util.h"


// helper function

static bool rowInVector(int row, std::vector<int> rows)
{
    std::vector<int>::iterator it;
    for (it = rows.begin(); it != rows.end(); ++it)
    {
        if (row == *it)
            return true;
    }
    
    return false;
}




namespace kcl
{


// RowSelectionGrid class implementation

BEGIN_EVENT_TABLE(RowSelectionGrid, kcl::Grid)
    EVT_SIZE(RowSelectionGrid::onSize)
    EVT_KCLGRID_KEY_DOWN(RowSelectionGrid::onKeyDown)
    EVT_KCLGRID_END_EDIT(RowSelectionGrid::onEndEdit)
    EVT_KCLGRID_CURSOR_MOVE(RowSelectionGrid::onCursorMove)
    EVT_KCLGRID_SELECTION_CHANGE(RowSelectionGrid::onSelectionChanged)
    EVT_KCLGRID_INVALID_AREA_CLICK(RowSelectionGrid::onInvalidAreaClick)
    EVT_KCLGRID_PRE_GHOST_ROW_INSERT(RowSelectionGrid::onPreGhostRowInsert)
    EVT_KCLGRID_PRE_INVALID_AREA_INSERT(RowSelectionGrid::onPreInvalidAreaInsert)
    EVT_KCLGRID_BEGIN_DRAG(RowSelectionGrid::onBeginDrag)
END_EVENT_TABLE()


RowSelectionGrid::RowSelectionGrid(wxWindow* parent,
                                   wxWindowID id,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style,
                                   bool drag_source,
                                   bool drop_target)
{
    // initialize row selection variables
    m_row_selection_grid_flags = RowSelectionGrid::refreshAll;
    m_was_editing = false;
    m_allow_deletes = false;
    m_allow_keyboard_inserts = false;
    m_allow_invalidarea_inserts = false;
    m_drag_source = drag_source;
    m_drop_target = drop_target;
    m_accepts_focus = true;
    
    // this is really quite idiotic, but the kcl::Grid _REALLY_ needs
    // us to call this function in order to set the border style
    setBorderType(style & wxBORDER_MASK);
    
    // create the row selection grid
    Create(parent, id, pos, size, style, kcl::Grid::stateVisible);
    
    // set our default row selection options
    setOptionState(kcl::Grid::optHorzGridLines |
                   kcl::Grid::optVertGridLines |
                   kcl::Grid::optColumnMove |
                   kcl::Grid::optGhostRow |
                   kcl::Grid::optInvalidAreaClickable |
                   kcl::Grid::optSelect |
                   kcl::Grid::optActivateHyperlinks, false);
                       
    setOptionState(kcl::Grid::optContinuousScrolling |
                   kcl::Grid::optSelectionList |
                   kcl::Grid::optFullGreenbar, true);
    
    setCursorVisible(false);
    setCursorType(kcl::Grid::cursorRowOutline);
    setGreenBarInterval(2);
    setRowLabelSize(0);

    // make this grid a drop target and connect the signal
    if (m_drop_target)
    {
        kcl::GridDataDropTarget* drop_target = new kcl::GridDataDropTarget(this);
        drop_target->sigDropped.connect(this, &RowSelectionGrid::onDataDropped);
        SetDropTarget(drop_target);
    }
}

RowSelectionGrid::~RowSelectionGrid()
{

}

void RowSelectionGrid::setRowSelectionGridFlags(int flags)
{
    m_row_selection_grid_flags = flags;
}

void RowSelectionGrid::setAllowDeletes(bool allow_deletes)
{
    m_allow_deletes = allow_deletes;
}

void RowSelectionGrid::setAllowKeyboardInserts(bool allow_keyboard_inserts)
{
    m_allow_keyboard_inserts = allow_keyboard_inserts;
}

void RowSelectionGrid::setAllowInvalidAreaInserts(bool allow_invalidarea_inserts)
{
    m_allow_invalidarea_inserts = allow_invalidarea_inserts;
    setOptionState(kcl::Grid::optGhostRow, allow_invalidarea_inserts);
    setOptionState(kcl::Grid::optInvalidAreaClickable, allow_invalidarea_inserts);
}

void RowSelectionGrid::onSize(wxSizeEvent& evt)
{
    Grid::onSize(evt);
    
    if (getVisibleRowCount() >= getRowCount() && getRowOffset() > 0)
    {
        scrollToTop();
        refresh(kcl::Grid::refreshAll);
    }
    
    evt.Skip();
}

void RowSelectionGrid::onKeyDown(kcl::GridEvent& evt)
{
    int row_count = getRowCount();
    int col_count = getColumnCount();
    int row = getCursorRow();
    int col = getCursorColumn();
    
    // since this is a _ROW_ selection grid, we need to convert all
    // non-row selections into single row selections (e.g. a single
    // selection which spans multiple rows will become multiple
    // selections that only contain one row)
    
    
    // handle the key that was pressed
    
    int key_code = evt.GetKeyEvent().GetKeyCode();
    int shift_down = evt.GetKeyEvent().ShiftDown();

    if (key_code == WXK_RETURN ||
        key_code == WXK_NUMPAD_ENTER)
    {
        if (shift_down)
            key_code = WXK_UP;
             else
            key_code = WXK_DOWN;
    }

    if (key_code == WXK_TAB ||
        key_code == WXK_NUMPAD_TAB)
    {
        if (shift_down)
            key_code = WXK_LEFT;
             else
            key_code = WXK_RIGHT;
    }
            
    switch (key_code)
    {
        case WXK_INSERT:
        case WXK_NUMPAD_INSERT:
            {
                if (m_allow_keyboard_inserts)
                {
                    // insert the rows and refresh the grid
                    insertSelectedRows(true);
                }
            }
            return;
        
        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
            {
                if (m_allow_deletes)
                {
                    // delete the rows and refresh the grid
                    deleteSelectedRows(true);
                }
            }
            return;

        case WXK_LEFT:
        case WXK_NUMPAD_LEFT:
            {
                if (col != 0)
                {
                    // move left one cell
                    moveCursor(row, col-1, false);
                    scrollHorzToCursor();
                    refresh(kcl::Grid::refreshAll);
                }
                 else
                {
                    if (row != 0)
                    {
                        // if the shift key is pressed, don't
                        // clear the selection, add to it
                        if (!shift_down)
                            clearSelection();
                        
                        // select the row we're going to and deselect
                        // the row we're on if the shift key is down
                        if (!isRowSelected(row-1))
                            setRowSelected(row-1, true);
                         else
                            setRowSelected(row, false);
                        
                        // move up one row and to the last cell in that row
                        moveCursor(row-1, col_count-1, false);
                        if (!isCursorVisible())
                            scrollVertToCursor();
                        scrollHorzToCursor();
                        refresh(kcl::Grid::refreshAll);
                    }
                }
            }
            return;
            
        case WXK_RIGHT:
        case WXK_NUMPAD_RIGHT:
            {
                if (col != col_count-1)
                {
                    // move right one cell
                    moveCursor(row, col+1, false);
                    scrollHorzToCursor();
                    refresh(kcl::Grid::refreshAll);
                }
                 else
                {
                    // the cursor can go one row further
                    // when a ghost row exists
                    if (getOptionState(optGhostRow) == true)
                        row_count += 1;
                    
                    if (row != row_count-1)
                    {
                        // if the shift key is pressed, don't
                        // clear the selection, add to it
                        if (!shift_down)
                            clearSelection();
                        
                        // using getRowCount() here will ensure that we compare the
                        // target row against the _REAL_ row count -- even if we
                        // have a ghost row
                        if (row+1 < getRowCount())
                        {
                            // select the row we're going to and deselect
                            // the row we're on if the shift key is down
                            if (!isRowSelected(row+1))
                                setRowSelected(row+1, true);
                             else
                                setRowSelected(row, false);
                        }
                        
                        // move down one row and to the first cell in that row
                        moveCursor(row+1, 0, false);
                        if (!isCursorVisible())
                            scrollVertToCursor();
                        scrollHorzToCursor();
                        refresh(kcl::Grid::refreshAll);
                    }
                }
            }
            return;
            
        case WXK_UP:
        case WXK_NUMPAD_UP:
            {
                if (row != 0)
                {
                    // if the shift key is pressed, don't
                    // clear the selection, add to it
                    if (!shift_down)
                        clearSelection();
                    
                    // select the row we're going to and deselect
                    // the row we're on if the shift key is down
                    if (!isRowSelected(row-1))
                        setRowSelected(row-1, true);
                     else
                        setRowSelected(row, false);
                        
                    moveCursor(row-1, col, false);
                    if (!isCursorVisible())
                        scrollVertToCursor();
                    refresh(kcl::Grid::refreshAll);
                }
            }
            return;

        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
            {
                // the cursor can go one row further
                // when a ghost row exists
                if (getOptionState(optGhostRow) == true)
                    row_count += 1;
                
                if (row != row_count-1)
                {
                    // if the shift key is pressed, don't
                    // clear the selection, add to it
                    if (!shift_down)
                        clearSelection();
                    
                    // using getRowCount() here will ensure that we compare the
                    // target row against the _REAL_ row count -- even if we
                    // have a ghost row
                    if (row+1 < getRowCount())
                    {
                        // select the row we're going to and deselect
                        // the row we're on if the shift key is down
                        if (!isRowSelected(row+1))
                            setRowSelected(row+1, true);
                         else
                            setRowSelected(row, false);
                    }
                    
                    moveCursor(row+1, col, false);
                    if (!isCursorVisible())
                        scrollVertToCursor();
                    refresh(kcl::Grid::refreshAll);
                }
            }
            return;

        case WXK_HOME:
        case WXK_NUMPAD_HOME:
            {
                // clear the existing selection
                clearSelection();
                
                // if the shift key is pressed, select all rows
                // between the cursor row and the first row,
                // otherwise, just select the first row
                if (!shift_down)
                {
                    setRowSelected(0, true);
                }
                 else
                {
                    while (row >= 0)
                        setRowSelected(row--, true);
                }
                
                moveCursor(0, col, false);
                if (!isCursorVisible())
                    scrollVertToCursor();
                refresh(kcl::Grid::refreshAll);
            }
            return;
            
        case WXK_END:
        case WXK_NUMPAD_END:
            {
                // clear the existing selection
                clearSelection();
                
                // if the shift key is pressed, select all rows
                // between the cursor row and the first row,
                // otherwise, just select the first row
                if (!shift_down)
                {
                    setRowSelected(0, true);
                }
                 else
                {
                    while (row <= row_count-1)
                        setRowSelected(row++, true);
                }
                
                moveCursor(row_count-1, col, false);
                if (!isCursorVisible())
                    scrollVertToCursor();
                refresh(kcl::Grid::refreshAll);
            }
            return;
            
        case WXK_PAGEUP:
        case WXK_NUMPAD_PAGEUP:
        case WXK_PAGEDOWN:
        case WXK_NUMPAD_PAGEDOWN:
            {
                // don't allow these keys to do anything
                evt.Veto();
            }
            return;

        default:
            {
                evt.Skip();
            }
            return;
    }
}

void RowSelectionGrid::onEndEdit(kcl::GridEvent& evt)
{
    m_was_editing = true;
    evt.Skip();
}

void RowSelectionGrid::onCursorMove(kcl::GridEvent& evt)
{
    int row = evt.GetRow();
    int dest_row = evt.GetDestinationRow();
    
    // this code is only called when the user has ended a cell edit and
    // the row is changing -- this is here because we want to update the
    // row selection based on the cursor position which is updated when
    // the user hit enter, shift-enter, etc. after their cell edit
    if (m_was_editing && (row != dest_row))
    {
        clearSelection();
        setRowSelected(dest_row, true);
        refresh(kcl::Grid::refreshAll);
    }
    
    m_was_editing = false;
    evt.Skip();
}

void RowSelectionGrid::onSelectionChanged(kcl::GridEvent& evt)
{
    // since this is a _ROW_ selection grid, we need to convert
    // all multi-row selections into single row selections

    std::vector<int>::iterator it;
    std::vector<int> to_delete;
    std::vector<int> to_add;
    
    // find all multi-row selections
    kcl::SelectionRect rect;
    int i, selection_count = getSelectionCount();
    for (i = selection_count-1; i >= 0; --i)
    {
        getSelection(i, &rect);
        if (rect.m_start_row == rect.m_end_row)
            continue;
        
        to_delete.push_back(i);
    }
    
    // delete multi-row selections
    for (it = to_delete.begin(); it != to_delete.end(); ++it)
    {
        // add the single rows 
        getSelection(*it, &rect);
        for (i = rect.m_start_row; i <= rect.m_end_row; ++i)
            to_add.push_back(i);
        
        removeSelection(*it);
    }
    
    // add single-row selections
    for (it = to_add.begin(); it != to_add.end(); ++it)
    {
        if (!isRowSelected(*it))
            setRowSelected(*it, true);
    }
}

void RowSelectionGrid::onInvalidAreaClick(kcl::GridEvent& evt)
{
    clearSelection();
    refresh(kcl::Grid::refreshAll);
    evt.Skip();
}

void RowSelectionGrid::onPreGhostRowInsert(kcl::GridEvent& evt)
{
    if (!m_allow_invalidarea_inserts)
    {
        evt.Veto();
        return;
    }
    
    clearSelection();
    setRowSelected(evt.GetRow(), true);
    moveCursor(evt.GetRow(), evt.GetColumn(), false);

    if (m_row_selection_grid_flags &
        RowSelectionGrid::refreshAfterPreGhostRowInsert)
    {
        refresh(kcl::Grid::refreshAll);
    }

    // skip the event in case the class which has the RowSelectionGrid
    // in it wants to do anything further
    evt.Skip();
}

void RowSelectionGrid::onPreInvalidAreaInsert(kcl::GridEvent& evt)
{
    if (!m_allow_invalidarea_inserts)
    {
        evt.Veto();
        return;
    }
    
    clearSelection();
    setRowSelected(evt.GetRow(), true);
    moveCursor(evt.GetRow(), evt.GetColumn(), false);

    if (m_row_selection_grid_flags &
        RowSelectionGrid::refreshAfterPreInvalidAreaInsert)
    {
        refresh(kcl::Grid::refreshAll);
    }

    // skip the event in case the class which has the
    // RowSelectionGrid in it wants to do anything further
    evt.Skip();
}

void RowSelectionGrid::onBeginDrag(kcl::GridEvent& evt)
{
    if (!m_drag_source)
    {
        evt.Veto();
        return;
    }
    
    kcl::GridDataObject data(this, m_drag_format);
    kcl::GridDropSource dragSource(data, this);
    wxDragResult result = dragSource.DoDragDrop(TRUE);
    
    int x, y, w, h;
    ::wxGetMousePosition(&x,&y);
    ScreenToClient(&x,&y);
    GetClientSize(&w,&h);
    
    if (result == wxDragNone)
    {
        int offset = getRowOffset();
        int visrows = getVisibleRowCount();
        
        kcl::GridDraggedRows rows = data.getDraggedRows();
        int drop_row = offset;
        
        if (y <= getHeaderSize())
        {
            if (!m_drop_target)
            {
                evt.Veto();
                return;
            }
            
            // if we're doing a drag and drop operation and
            // we dragged the rows above the source grid,
            // drop them at the row offset in the grid
            kcl::doRowDragDrop(this, rows, drop_row, true);
        }
         else if (y >= h)
        {
            if (!m_drop_target)
            {
                evt.Veto();
                return;
            }
            
            if (offset + visrows >= getRowCount())
                drop_row = getRowCount();
            
            // if we're doing a drag and drop operation and
            // we dragged the rows above the source grid, drop
            // them at the row offset + visible rows in the grid
            kcl::doRowDragDrop(this, rows, drop_row, true);
        }
         else
        {
            deleteSelectedRows(true);
        }
    }
}

void RowSelectionGrid::onDataDropped(kcl::GridDataDropTarget* drop_target)
{
    // this code handles row dragging (reordering) in this grid.
    
    // in order for row dragging to work, the source
    // of the data object must be this grid
    if (drop_target->getTargetGrid() == this)
    {
        if (m_row_selection_grid_flags &
            RowSelectionGrid::refreshAfterDataDropped)
        {
            drop_target->doRowDrag(true);
        }
         else
        {
            drop_target->doRowDrag(false);
        }
    }
    
    // fire this signal in case the class which has the
    // RowSelectionGrid in it wants to do anything further
    sigDataDropped(drop_target);
}

void RowSelectionGrid::insertSelectedRows(bool refresh_rows)
{        
    // get the selected rows
    std::vector<int> rows = getSelectedRows();

    // fire a signal that we're inserting these rows
    sigInsertingRows(rows);
    
    // if the signal isn't active, insert the rows ourselves
    if (!sigInsertingRows.isActive())
    {
        // insert the rows
        std::vector<int>::iterator it;
        for (it = rows.begin(); it != rows.end(); ++it)
        {
            insertRow(*it);
        }

        // the grid has no rows, so get things started
        if (rows.size() == 0 && getRowCount() == 0)
        {
            insertRow(0);
            setRowSelected(0, true);
        }
    }

    // fire a signal that we've inserted these rows
    sigInsertedRows(rows);

    if (refresh_rows)
        refresh(kcl::Grid::refreshAll);
}

void RowSelectionGrid::deleteSelectedRows(bool refresh_rows)
{
    if (!m_allow_deletes)
        return;
    
    // get the selected rows
    std::vector<int> rows = getSelectedRows();
    
    // there are no rows selected, bail out
    if (rows.size() == 0)
        return;
    
    // fire a signal that we're deleting these rows
    bool allow = true;
    sigDeletingRows(rows, &allow);
    
    // deleting rows is not allowed; bail out
    if (!allow)
        return;
    
    // if the signal isn't active, or we allow the delete,
    // delete the rows ourselves
    if (!sigDeletingRows.isActive() || allow)
    {
        // delete the rows from the highest row number to the lowest
        std::vector<int>::reverse_iterator it;
        for (it = rows.rbegin(); it != rows.rend(); ++it)
                deleteRow(*it);
    }
    
    // now that we've deleted rows, we need to delete any grid
    // selections which are out of the range of the new row count
    kcl::SelectionRect rect;
    int row_count = getRowCount();
    int i, selection_count = getSelectionCount();
    
    for (i = selection_count-1; i >= 0 ; --i)
    {
        getSelection(i, &rect);

        // the entire selection is out of range, delete it
        if (rect.m_start_row >= row_count &&
            rect.m_end_row >= row_count)
        {
            removeSelection(i);
            continue;
        }
        
        // part of the selection is out of range, truncate it
        if (rect.m_end_row >= row_count)
        {
            rect.m_end_row = row_count-1;
            continue;
        }
    }
    
    // if we've deleted all of the existing selections and the grid
    // still has some rows left, select the last row
    if (getSelectionCount() == 0 && row_count != 0)
        setRowSelected(row_count-1, true);
    
    // fire a signal that we've deleted these rows
    sigDeletedRows(rows);
    
    if (refresh_rows)
        refresh(kcl::Grid::refreshAll);
}

size_t RowSelectionGrid::getSelectedRowCount()
{
    std::vector<int> rows = getSelectedRows();
    return rows.size();
}

std::vector<int> RowSelectionGrid::getSelectedRows()
{
    std::vector<int> rows;
    
    kcl::SelectionRect rect;
    int i, count = getSelectionCount();
    
    // add all of the selected rows to the rows vector
    for (i = 0; i < count; ++i)
    {
        getSelection(i, &rect);

        int row = rect.m_start_row;
        while (row <= rect.m_end_row)
        {
            if (!rowInVector(row, rows))
                rows.push_back(row);
                
            row++;
        }
    }

    std::sort(rows.begin(), rows.end());
    return rows;
}




};  // namespace kcl


