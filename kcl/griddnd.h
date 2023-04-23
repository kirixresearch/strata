/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2006-10-11
 *
 */


#ifndef H_KCL_GRIDDND_H
#define H_KCL_GRIDDND_H


#include <wx/dataobj.h>
#include <wx/dnd.h>
#include <vector>


namespace kcl
{

const wxString GRID_DATA_OBJECT_FORMAT = wxT("application/kclgrid");

enum GridScrollDirection
{
    GridScrollNone = 0,
    GridScrollVertical = 1,
    GridScrollHorizontal = 2
};

// -- drag and drop data object class for the grid --

typedef std::vector<CellData*> GridDraggedCells;

struct GridDraggedRowInfo
{
    int idx;
    long data;
};

struct GridDraggedRow
{
    GridDraggedCells cells;
    int idx;
    long data;
};
    
typedef std::vector<GridDraggedRow> GridDraggedRows;

static GridDraggedCells EMPTY_GRID_DRAGGED_CELLS;
static GridDraggedRows  EMPTY_GRID_DRAGGED_ROWS;


static wxString getGridDataFormat(const wxString& extended_format)
{
    wxString retval = GRID_DATA_OBJECT_FORMAT;
    
    if (!extended_format.IsEmpty())
    {
        retval += wxT("/");
        retval += extended_format;
    }
    
    return retval;
}


// -- utility function (mainly to allow outside use of this function)

void doRowDragDrop(kcl::Grid* grid,
                   kcl::GridDraggedRows rows,
                   int drop_row,
                   bool refresh = true);




class GridDataObject : public wxCustomDataObject
{
public:

    GridDataObject(kcl::Grid* grid = NULL,
                   const wxString& extended_format = wxEmptyString);

    void* GetData() const;

    wxWindowID getSourceGridId();
    GridDraggedCells getDraggedCells();
    GridDraggedRows getDraggedRows();

private:

    void setDragInfo(kcl::Grid* grid, bool full_rows = true);
    GridDraggedRowInfo* getDraggedRowInfo(int row);

    wxWindowID* m_sourcegrid_id; // pointer to insert into our "clipboard" data
};



    
// this is just a container class which allows us to use the
// accessor functions in kcl::GridDataObject as well as handle multiple
// data objects -- whether they be extra kcl::GridDataObjects added through
// the constructor or other types of data objects added later with Add()

class GridDataObjectComposite : public wxDataObjectComposite
{
public:

    GridDataObjectComposite(kcl::Grid* grid1 = NULL, const wxString& extended_format1 = wxEmptyString,
                            kcl::Grid* grid2 = NULL, const wxString& extended_format2 = wxEmptyString,
                            kcl::Grid* grid3 = NULL, const wxString& extended_format3 = wxEmptyString)
    {
        m_grid_data_object1 = NULL;
        m_grid_data_object2 = NULL;
        m_grid_data_object3 = NULL;
        
        // add the default grid data object
        m_grid_data_object1 = new GridDataObject(grid1, extended_format1);
        Add(m_grid_data_object1, true);
        
        // if we have opted to add any further
        // grid data objects, add them as well

        if (!extended_format2.IsEmpty())
        {
            m_grid_data_object2 = new GridDataObject(grid2, extended_format2);
            Add(m_grid_data_object2, true);
        }
        
        if (!extended_format3.IsEmpty())
        {
            m_grid_data_object3 = new GridDataObject(grid3, extended_format3);
            Add(m_grid_data_object3, true);
        }
    }
    
    wxWindowID getSourceGridId()
    {
        kcl::GridDataObject* obj = getReceivedGridDataObject();
        if (obj)
            return obj->getSourceGridId();
        return -1;
    }

    GridDraggedCells getDraggedCells()
    {
        kcl::GridDataObject* obj = getReceivedGridDataObject();
        if (obj)
            return obj->getDraggedCells();
        return EMPTY_GRID_DRAGGED_CELLS;
    }

    GridDraggedRows getDraggedRows()
    {
        kcl::GridDataObject* obj = getReceivedGridDataObject();
        if (obj)
            return obj->getDraggedRows();
        return EMPTY_GRID_DRAGGED_ROWS;
    }
    
    bool isGridData()
    {
        if (getReceivedGridDataObject() != NULL)
            return true;
        
        return false;
    }
    
private:

    kcl::GridDataObject* getReceivedGridDataObject()
    {
        wxDataFormat fmt = GetReceivedFormat();

        if (m_grid_data_object1 != NULL &&
            fmt == m_grid_data_object1->GetFormat())
        {
            return m_grid_data_object1;
        }
        
        if (m_grid_data_object2 != NULL &&
            fmt == m_grid_data_object2->GetFormat())
        {
            return m_grid_data_object2;
        }
        
        if (m_grid_data_object3 != NULL &&
            fmt == m_grid_data_object3->GetFormat())
        {
            return m_grid_data_object3;
        }

        return NULL;
    }
    
private:

    kcl::GridDataObject* m_grid_data_object1;
    kcl::GridDataObject* m_grid_data_object2;
    kcl::GridDataObject* m_grid_data_object3;
};




class GridDataDropTargetTimer;
class GridDataDropTarget : public wxDropTarget
{
friend class GridDataDropTargetTimer;

public:

    GridDataDropTarget(kcl::Grid* target_grid,
                       GridScrollDirection = GridScrollVertical);
    ~GridDataDropTarget();

    void OnLeave();
    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);

    // wxHORIZONTAL if the hint should be for rows
    // wxVERTICAL if the hint should be for columns
    void setHintOrientation(int orientation);

    void setGridDataObjectFormats(const wxString& format1,
                                  const wxString& format2 = wxEmptyString,
                                  const wxString& format3 = wxEmptyString);
                           
    wxWindowID getSourceGridId();
    kcl::Grid* getTargetGrid();
    GridDraggedCells getDraggedCells();
    GridDraggedRows getDraggedRows();
    int getDropColumn();
    int getDropRow();
    
    void doRowDrag(bool refresh = true);
    bool scrollVertical(int rows);
    bool scrollHorizontal(int pixels);

public: // signals

    xcm::signal1<GridDataDropTarget*> sigDropped;

private:

    kcl::Grid* m_target_grid;
    kcl::GridDataObjectComposite* m_drop_data;
    kcl::GridDataDropTargetTimer* m_scroll_timer;
    GridScrollDirection m_scroll_direction;
    int m_hint_orientation;  // wxHORIZONTAL or wxVERTICAL
    int m_cur_highlight;     // current row or column highlighted
    int m_drop_row;          // stores the row dropped upon
    int m_drop_col;          // stores the column dropped upon
    bool m_accepted;         // true if the dragged data object can be accepted
};



    
// very basic text drop target for the grid

class GridTextDropTarget : public wxTextDropTarget
{
public:

    GridTextDropTarget(kcl::Grid* target_grid) : wxTextDropTarget()
    {
        m_target_grid = target_grid;
        m_highlight_row = -1;
        m_drop_row = -1;
    }

    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
    {
        int row, viewcol;
        
        y += (m_target_grid->getRowHeight()/2);

        if (m_target_grid->hitTest(x,
                                   y,
                                   &row,
                                   &viewcol))
        {
            if (m_highlight_row != row)
            {
                m_target_grid->drawRowDropHighlight(row, 0);
                m_highlight_row = row;
            }
        }
         else
        {
            row = m_target_grid->getRowCount();
            if (m_highlight_row != row && row != 0)
            {
                m_target_grid->drawRowDropHighlight(row, 0);
                m_highlight_row = row;
            }
        }

        return def;
    }

    void OnLeave()
    {
        m_target_grid->refresh(kcl::Grid::refreshAll);
        m_highlight_row = -1;
    }

    bool OnDropText(wxCoord x, wxCoord y, const wxString& text)
    {
        m_drop_text = text;

        y += (m_target_grid->getRowHeight()/2);

        // -- find out where the data was dropped --
        int row, viewcol;
        if (m_target_grid->hitTest(x, y, &row, &viewcol))
        {
            m_drop_row = row;
        }
         else
        {
            m_drop_row = -1;
        }

        m_highlight_row = -1;

        m_target_grid->refresh(kcl::Grid::refreshPaint);

        sigDropped(this);
        return true;
    }

    wxString getDropText()
    {
        return m_drop_text;
    }

    int getDropRow()
    {
        return m_drop_row;
    }

    kcl::Grid* getTargetGrid()
    {
        return m_target_grid;
    }

public: // signals

    xcm::signal1<GridTextDropTarget*> sigDropped;

private:

    kcl::Grid* m_target_grid;
    wxString m_drop_text;
    int m_drop_row;
    int m_highlight_row;
};




class GridDataObject;
class GridDropSource : public wxDropSource
{
public:
    
    GridDropSource();
    GridDropSource(kcl::GridDataObject& data,
                   kcl::Grid* grid = NULL);
    bool GiveFeedback(wxDragResult effect);

private:

    kcl::Grid* m_grid;
};

    
};  // namespace kcl


#endif


