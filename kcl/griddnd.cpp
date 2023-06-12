/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2006-10-11
 *
 */


#include "grid.h"
#include "griddnd.h"
#include <algorithm>


namespace kcl
{


enum DragMetadata
{
    IDX_CELLDATA_COUNT = 0,
    IDX_ROWINFO_COUNT = 1,
    IDX_SOURCEGRID_ID = 2
};
const int DRAG_METADATA_COUNT = 3;


// utility functions

static bool isMouseInWindow(const wxWindow* wnd)
{
    wxPoint pt = ::wxGetMousePosition();
    wxRect rect = wnd->GetClientRect();
    wnd->ClientToScreen(&rect.x, &rect.y);
    return rect.Contains(pt);
}

void doRowDragDrop(kcl::Grid* grid,
                   kcl::GridDraggedRows rows,
                   int drop_row,
                   bool refresh)
{
    int row;
    
    // delete the dragged rows from the grid
    GridDraggedRows::reverse_iterator rit;
    for (rit = rows.rbegin(); rit != rows.rend(); ++rit)
    {
        row = rit->idx;
        
        if (row < drop_row)
            drop_row--;

        grid->deleteRow(row);
    }

    // start at the drop row when we add rows back to the grid
    row = drop_row;

    // clear any selection before we add the new row selections
    grid->clearSelection();

    // add the dragged rows back to the grid
    GridDraggedRows::iterator it;
    for (it = rows.begin(); it != rows.end(); ++it)
    {
        grid->insertRow(row);
        
        // set the row data for this row
        grid->setRowData(row, it->data);
            
        GridDraggedCells cells = it->cells;
        GridDraggedCells::iterator cit;
        for (cit = cells.begin(); cit != cells.end(); ++cit)
        {
            CellData* cell = (*cit);
            cell->m_row = row;
            
            // set the cell data for this cell
            grid->setCellData(cell->m_row, cell->m_col, cell);

            if (cell->m_bitmap.Ok())
            {
                grid->setCellBitmap(cell->m_row,
                                    cell->m_col,
                                    cell->m_bitmap,
                                    cell->bitmap_alignment);
            }
            
            switch (cell->m_type)
            {
                case Grid::typeCharacter:
                {
                    grid->setCellString(cell->m_row,
                                        cell->m_col,
                                        cell->m_strvalue);
                }
                break;
                
                case Grid::typeDouble:
                {
                    grid->setCellDouble(cell->m_row,
                                        cell->m_col,
                                        cell->m_dblvalue);
                }
                break;
                
                case Grid::typeInteger:
                {
                    grid->setCellInteger(cell->m_row,
                                         cell->m_col,
                                         cell->m_intvalue);
                }
                break;
                
                case Grid::typeBoolean:
                {
                    grid->setCellBoolean(cell->m_row,
                                         cell->m_col,
                                         cell->m_boolvalue);
                }
                break;
            }
        }
        
        grid->setRowSelected(row, true);
        row++;
    }
    
    // move the cursor to the drop row
    int cursor_col = grid->getCursorColumn();
    grid->moveCursor(drop_row, cursor_col, false);
    
    // if specified, refresh the grid
    if (refresh)
        grid->refresh(Grid::refreshAll);
}

// functors to help sort the cells and rows

static bool sortCells(const CellData* l, const CellData* r)
{
    if (l->m_row < r->m_row)
        return true;
        
    if (l->m_row == r->m_row)
    {
        if (l->m_col < r->m_col)
            return true;
    }
    
    return false;
}

// GridDataObject class implementation

GridDataObject::GridDataObject(Grid* grid,
                               const wxString& extended_format)
               : wxCustomDataObject(wxDataFormat(getGridDataFormat(extended_format)))
{
    unsigned long i = 0;
    SetData(wxDataFormat(GRID_DATA_OBJECT_FORMAT), sizeof(long), (void*)&i);

    if (grid != NULL)
    {
        // if we're editing anything in the grid, end the edit
        if (grid->isEditing())
            grid->endEdit(true);

        m_sourcegrid_id = grid->GetId();
        
        setDragInfo(grid);
    }
}

void* GridDataObject::GetData() const
{
    return wxCustomDataObject::GetData();
}

void GridDataObject::setDragInfo(Grid* grid, bool full_rows)
{
    GridDraggedCells dragged_cells;
    std::vector<GridDraggedRowInfo*> row_info;
    
    int i, j, k;
    int sel_count = grid->getSelectionCount();
    int col_count = grid->getColumnCount();
    int row_count = grid->getRowCount();
    
    // save these values so we can restore them when we're done
    int row_offset = grid->getRowOffset();
    int cursor_row = grid->getCursorRow();
    int cursor_col = grid->getCursorColumn();
    grid->Freeze();
    
    std::vector< std::pair<int,int> > added_cells;
    std::vector< std::pair<int,int> >::iterator added_it;
    bool cell_found;
    
    // populate the GridDraggedCells vector based on the grid selection
    for (i = 0; i < sel_count; ++i)
    {
        SelectionRect selrect;
        grid->getSelection(i, &selrect);

        for (j = selrect.m_start_row; j <= selrect.m_end_row; ++j)
        {
            // this row is out-of-bounds, continue
            if (j >= row_count)
                continue;
            
            // ensure that the current row is visible, even though
            // the grid is frozen to the user -- we have to do this
            // since the call to getCellData() only works on rows
            // that are visible
            grid->moveCursor(j, 0, false);
            grid->scrollVertToCursor();
            grid->updateData();

            // populate the cell data vector
            for (k = 0; k < col_count; ++k)
            {
                cell_found = false;
                
                if (full_rows || grid->isCellSelected(j, k))
                {
                    // doing this lookup here ensures that we never
                    // add the same cell to the drag info twice
                    for (added_it  = added_cells.begin();
                         added_it != added_cells.end(); ++added_it)
                    {
                        if (j == added_it->first && k == added_it->second)
                        {
                            cell_found = true;
                            break;
                        }
                    }
                    
                    // if we find the cell in the "added cells"
                    // vector, don't add it again
                    if (cell_found)
                        continue;
                    
                    CellData* cell = new CellData(*(grid->getCellData(j, k)));
                    cell->m_row = j;
                    cell->m_col = k;
                    
                    dragged_cells.push_back(cell);
                    added_cells.push_back(std::pair<int,int>(j,k));
                }
            }
            
            // populate the row data vector
            GridDraggedRowInfo* ri = new GridDraggedRowInfo;
            ri->idx = j;
            ri->data = grid->getRowData(j);
            row_info.push_back(ri);
        }
    }
    
    
    // fill out the untyped long info for the data object
    
    int celldata_count = dragged_cells.size();
    int rowdata_count = row_info.size();
    
    int entry_count = celldata_count + rowdata_count + DRAG_METADATA_COUNT;
    int data_size = entry_count * sizeof(long);
    unsigned long* data = new unsigned long[data_size];
    data[IDX_CELLDATA_COUNT] = celldata_count;
    data[IDX_ROWINFO_COUNT] = rowdata_count;
    data[IDX_SOURCEGRID_ID] = m_sourcegrid_id;

    // our data index starts right after the number of drag metadata entries
    int data_idx = DRAG_METADATA_COUNT;
    
    // set the cell data information
    GridDraggedCells::iterator it;
    for (it = dragged_cells.begin(); it != dragged_cells.end(); ++it)
    {
        CellData* cell = (*it);
        data[data_idx++] = (unsigned long)(cell);
    }
    
    // set the row data information
    std::vector<GridDraggedRowInfo*>::iterator it2;
    for (it2 = row_info.begin(); it2 != row_info.end(); ++it2)
    {
        GridDraggedRowInfo* ri = (*it2);
        data[data_idx++] = (unsigned long)(ri);
    }
    
    // restore the grid's row offset and cursor position
    grid->moveCursor(cursor_row, cursor_col, false);
    grid->setRowOffset(row_offset);
    grid->refresh(kcl::Grid::refreshAll);
    grid->Thaw();

    SetData(wxDataFormat(GRID_DATA_OBJECT_FORMAT), data_size, data);
    delete[] data;
}

GridDraggedCells GridDataObject::getDraggedCells()
{
    GridDraggedCells cells;
    GridDraggedCells::iterator it;
    
    unsigned long* data = (unsigned long*)GetData();

    int i, data_idx;
    int celldata_count = data[IDX_CELLDATA_COUNT];
    
    // our celldata begins after the drag metadata
    for (i = 0; i < celldata_count; ++i)
    {
        data_idx = i + DRAG_METADATA_COUNT;
        CellData* cell = (CellData*)(data[data_idx]);
        
        bool found = false;
        for (it = cells.begin(); it != cells.end(); ++it)
        {
            // we've already added this cell to the vector
            if ((*it)->m_row == cell->m_row &&
                (*it)->m_col == cell->m_col)
            {
                found = true;
                break;
            }
        }
        
        // if the cell wasn't found, add it to our cells vector
        if (!found)
            cells.push_back(cell);
    }

    // now, let's sort the cell list by row and then by column
    std::sort(cells.begin(), cells.end(), sortCells);
    return cells;
}

GridDraggedRowInfo* GridDataObject::getDraggedRowInfo(int row)
{
    unsigned long* data = (unsigned long*)GetData();

    int i, data_idx;
    int celldata_count = data[IDX_CELLDATA_COUNT];
    int rowinfo_count = data[IDX_ROWINFO_COUNT];
    
    for (i = 0; i < rowinfo_count; ++i)
    {
        data_idx = i + celldata_count + DRAG_METADATA_COUNT;
        
        GridDraggedRowInfo* ri;
        ri = (GridDraggedRowInfo*)(data[data_idx]);
        
        if (ri->idx == row)
            return ri;
    }
    
    return 0;
}

GridDraggedRows GridDataObject::getDraggedRows()
{
    bool first_time = true;
    int last_row = -1;

    GridDraggedCells cells = getDraggedCells();
    GridDraggedCells::iterator it;
    
    GridDraggedRows rows;
    GridDraggedRow row;
    
    for (it = cells.begin(); it != cells.end(); ++it)
    {
        CellData* cell = (*it);
        
        // either we're on a new row or we're at the end of the vector
        if (cell->m_row != last_row && !first_time)
        {
            // we're on a different row and the row number is valid (!= 0),
            // so add the last row to our rows vector
            if (last_row != -1)
            {
                GridDraggedRowInfo* ri = getDraggedRowInfo(last_row);
                row.idx = last_row;
                if (ri != NULL)
                    row.data = (long)(ri->data);
                rows.push_back(row);
            }
            
            // get ready for a new row
            row.cells.clear();
            row.idx = -1;
            row.data = 0;
        }

        // add the cell to the current row
        row.cells.push_back(cell);
        
        // save the row number for comparison with the next cell
        last_row = cell->m_row;
        first_time = false;
    }
    
    // if the row number is valid (!= 0), add the final row to the rows vector
    if (last_row != -1)
    {
        GridDraggedRowInfo* ri = getDraggedRowInfo(last_row);
        row.idx = last_row;
        if (ri != NULL)
            row.data = (long)(ri->data);
        rows.push_back(row);
    }

    return rows;
}

wxWindowID GridDataObject::getSourceGridId()
{
    unsigned long* data = (unsigned long*)GetData();
    wxWindowID sourcegrid_id = (wxWindowID)(data[IDX_SOURCEGRID_ID]);
    return sourcegrid_id;
}


// GridDataDropTargetTimer class implementation

class GridDataDropTargetTimer : public wxTimer
{
public:

    GridDataDropTargetTimer(GridDataDropTarget* drop_target,
                            GridScrollDirection scroll_direction)
        : wxTimer(this, wxID_ANY)
    {
        m_drop_target = drop_target;
        m_scroll_direction = scroll_direction;
    }
    
    void beginNotify()
    {
        Start(75);
    }
    
    void Notify()
    {
        // the user is no longer dragging data, so stop the timer notification
        if (!::wxGetMouseState().LeftIsDown())
            Stop();

        if (!m_drop_target)
            return;

        if (m_scroll_direction == GridScrollNone)
            return;
        
        kcl::Grid* grid = m_drop_target->getTargetGrid();
        
        // only scroll the grid if the mouse is out of grid's client area
        if (!isMouseInWindow(grid))
        {
            wxSize s = grid->GetClientSize();
            wxPoint pt = grid->ScreenToClient(::wxGetMousePosition());
            
            if (m_scroll_direction == GridScrollVertical)
            {
                if (pt.x > 0 && pt.x < s.GetWidth())
                {
                    if (pt.y < 0)
                    {
                        if (pt.y > -75)
                            m_drop_target->scrollVertical(-1);
                             else
                            m_drop_target->scrollVertical(-2);
                    }
                    
                    if (pt.y > s.GetHeight())
                    {
                        if (pt.y < s.GetHeight()+75)
                            m_drop_target->scrollVertical(1);
                             else
                            m_drop_target->scrollVertical(2);
                    }
                }
            }
            
            // horizontal scrolling
            if (m_scroll_direction == GridScrollHorizontal)
            {
                if (pt.y > 0 && pt.y < s.GetHeight())
                {
                    if (pt.x < 0)
                        m_drop_target->scrollHorizontal(-30);
                        
                    if (pt.x > s.GetWidth())
                        m_drop_target->scrollHorizontal(30);
                }
            }
        }
    }
    
private:

    kcl::GridDataDropTarget* m_drop_target;
    GridScrollDirection m_scroll_direction;
};




// GridDataDropTarget class implementation

GridDataDropTarget::GridDataDropTarget(Grid* target_grid, GridScrollDirection scroll_direction)
                        : wxDropTarget(new GridDataObjectComposite)
{
    m_target_grid = target_grid;
    m_drop_data = NULL;
    m_cur_highlight = -1;
    m_drop_row = -1;
    m_drop_col = -1;
    m_hint_orientation = wxHORIZONTAL;
    m_scroll_direction = scroll_direction;
    m_accepted = true;
    
    m_scroll_timer = NULL;
    if (scroll_direction != GridScrollNone && m_target_grid != NULL)
        m_scroll_timer = new kcl::GridDataDropTargetTimer(this, scroll_direction);
}

GridDataDropTarget::~GridDataDropTarget()
{
    if (m_scroll_timer)
        delete m_scroll_timer;
}

int getDropRowFromMousePos(kcl::Grid* grid, wxCoord x, wxCoord y)
{
    int row, viewcol;

    y += grid->getRowHeight()/2;
    grid->hitTest(x, y, &row, &viewcol);

    if (row == -1)
    {
        y -= grid->getRowHeight()/2;
        if (y <= grid->getHeaderSize())
            row = 0;
         else
            row = grid->getRowCount();
    }
    
    return row;
}

int getDropColumnFromMousePos(kcl::Grid* grid, wxCoord x, wxCoord y)
{
    int viewcol = grid->getColumnFromXpos(x);

    if (x <= grid->getRowLabelSize())
    {
        viewcol = 0;
    }
     else
    {
        if (viewcol >= 0)
        {
            wxRect col_rect;
            if (grid->getColumnRect(viewcol, col_rect))
            {
                if (x - col_rect.x >= col_rect.width/2)
                    viewcol++;
            }
        }
         else
        {
            viewcol = grid->getColumnCount();
        }
    }
    
    return viewcol;
}

wxDragResult GridDataDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    // if the data we're dragging is not of a valid type, don't
    // draw the row highlight in the drop target
    if (def == wxDragNone)
    {
        m_accepted = false;
        return def;
    }
     else
    {
        m_accepted = true;
    }
    
    // we're not dragging to a grid, so we're done
    if (m_target_grid == NULL)
        return def;
        
    int row, viewcol;
    
    if (m_hint_orientation == wxHORIZONTAL)
    {
        row = getDropRowFromMousePos(m_target_grid, x, y);
        
        if (m_cur_highlight != row || m_cur_highlight == -1)
        {
            m_target_grid->drawRowDropHighlight(row, 0);
            m_cur_highlight = row;
        }
    }
     else if (m_hint_orientation == wxVERTICAL)
    {
        viewcol = getDropColumnFromMousePos(m_target_grid, x, y);
        
        if (m_cur_highlight != viewcol || m_cur_highlight == -1)
        {
            m_target_grid->drawColumnDropHighlight(viewcol);
            m_cur_highlight = viewcol;
        }
    }

    return def;
}

void GridDataDropTarget::OnLeave()
{
    // we're not dragging to a grid, so we're done
    if (m_target_grid == NULL)
        return;
    
    // if this drop target doesn't accept the dragged
    // data format, we're done
    if (!m_accepted)
        return;
    
    // make sure we refresh the drop highlight when we re-enter
    // the grid's client area if we're scrolling horizontally
    m_cur_highlight = -1;
    m_target_grid->refresh(kcl::Grid::refreshAll);
    
    if (!m_scroll_timer)
        return;
        
    // if we're leaving the drop target and the left mouse button
    // is down, begin the scroll timer for grid scrolling
    if (::wxGetMouseState().LeftIsDown())
        m_scroll_timer->beginNotify();
}

wxDragResult GridDataDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
    if (!GetData())
        return wxDragNone;

    m_drop_data = (GridDataObjectComposite*)GetDataObject();
    if (!m_drop_data)
        return wxDragNone;

    // if we're dropping on a non-kcl::Grid control,
    // fire the signal and we're done
    if (m_target_grid == NULL)
    {
        sigDropped(this);
        return def;
    }

    // find out the drop row and drop insert column
    // (this is only useful for inserting)
    m_drop_row = getDropRowFromMousePos(m_target_grid, x, y);
    m_drop_col = getDropColumnFromMousePos(m_target_grid, x, y);
    
    m_cur_highlight = -1;
    m_target_grid->refresh(Grid::refreshPaint);

    sigDropped(this);
    return def;
}

void GridDataDropTarget::setHintOrientation(int orientation)
{
    m_hint_orientation = orientation;
}

void GridDataDropTarget::setGridDataObjectFormats(const wxString& format1,
                                                  const wxString& format2,
                                                  const wxString& format3)
{
    GridDataObjectComposite* drop_data;
    drop_data = new GridDataObjectComposite(NULL, format1,
                                            NULL, format2,
                                            NULL, format3);
    SetDataObject(drop_data);
}

GridDraggedCells GridDataDropTarget::getDraggedCells()
{
    GridDraggedCells empty;
    
    if (!m_drop_data)
        return empty;
        
    return m_drop_data->getDraggedCells();
}

GridDraggedRows GridDataDropTarget::getDraggedRows()
{
    GridDraggedRows empty;
    
    if (!m_drop_data)
        return empty;
        
    return m_drop_data->getDraggedRows();
}

Grid* GridDataDropTarget::getTargetGrid()
{
    return m_target_grid;
}

int GridDataDropTarget::getDropRow()
{
    return m_drop_row;
}

int GridDataDropTarget::getDropColumn()
{
    return m_drop_col;
}

wxWindowID GridDataDropTarget::getSourceGridId()
{
    if (!m_drop_data)
        return 0;
        
    return m_drop_data->getSourceGridId();
}

void GridDataDropTarget::doRowDrag(bool refresh)
{
    if (m_target_grid == NULL)
        return;
    
    if (m_target_grid->isEditing())
        m_target_grid->endEdit(true);

    // try to make sure that the source and destination grids are the same
    wxWindowID source_id = getSourceGridId();
    if (source_id != -1 &&
        (wxWindow::FindWindowById(source_id) != m_target_grid))
        return;
    
    int drop_row = getDropRow();
    kcl::GridDraggedRows rows = getDraggedRows();
    kcl::doRowDragDrop(m_target_grid, rows, drop_row, refresh);
}

bool GridDataDropTarget::scrollVertical(int rows)
{
    if (!m_target_grid)
        return false;
    
    if (m_target_grid->isEditing())
        m_target_grid->endEdit(true);
    
    int offset = m_target_grid->getRowOffset();
    
    // only scroll the grid if it has more rows than can be shown
    if (m_target_grid->getRowCount() > m_target_grid->getVisibleRowCount())
        m_target_grid->scrollVert(rows);
    
    // no scrolling occured
    if (offset == m_target_grid->getRowOffset())
    {
        // we're at the top of the grid and the drop highlight
        // isn't all the way at the top, so draw it once more
        if (rows < 0 && m_cur_highlight != 0)
        {
            m_target_grid->drawRowDropHighlight(0, 0);
            m_cur_highlight = 0;
        }
        
        // we're at the bottom of the grid and the drop highlight
        // isn't all the way at the bottom, so draw it once more
        int row_count = m_target_grid->getRowCount();
        if (rows > 0 && m_cur_highlight != row_count)
        {
            m_target_grid->drawRowDropHighlight(row_count, 0);
            m_cur_highlight = row_count;
        }
        
        return false;
    }
    
    m_target_grid->refresh(kcl::Grid::refreshAll);
    return true;
}

bool GridDataDropTarget::scrollHorizontal(int pixels)
{
    if (!m_target_grid)
        return false;
    
    if (m_target_grid->isEditing())
        m_target_grid->endEdit(true);
    
    int offset = m_target_grid->getHorizontalOffset();
    m_target_grid->scrollHorz(pixels);
    m_target_grid->refresh(kcl::Grid::refreshAll);
    return true;
}




// GridDropSource class implementation

GridDropSource::GridDropSource()
{

}

GridDropSource::GridDropSource(kcl::GridDataObject& data,
                               kcl::Grid* grid)
                                    : wxDropSource(data, (wxWindow*)grid)
{
    m_grid = grid;
}

bool GridDropSource::GiveFeedback(wxDragResult effect)
{
    if (!m_grid)
        return false;
    
    // default feedback
    return false;
}


};  // namespace kcl


