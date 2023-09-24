/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2006-09-27
 *
 */


#include <iterator>
#include "kcanvas_int.h"
#include "componenttable.h"
#include "componenttextbox.h"
#include "componentimage.h"

#include "dnd.h"
#include "util.h"


namespace kcanvas
{


const wxString ACTION_NONE          = wxT("table.action.none");
const wxString ACTION_EDITCELL      = wxT("table.action.editcell");
const wxString ACTION_RESIZECELLS   = wxT("table.action.resizecells");
const wxString ACTION_POPULATECELLS = wxT("table.action.populatecells");
const wxString ACTION_MOVECELLS     = wxT("table.action.movecells");
const wxString ACTION_SELECTCELLS   = wxT("table.action.selectcells");

const wxString TAG_PREFIX           = wxT("tag.");


// helper class for rendering cell content
class CellRect
{
public:

    CellRect()
    {
        m_row = 0;
        m_col = 0;
    }
    
    ~CellRect()
    {
    }

public:

    // note: the boundary rectangle of the cell is the natural one
    // that corresponds to the grid; if the cells don't have text, the 
    // content rectangle and the clipping rectangle are the same as
    // the boundary rectangle; if the cell does have text, the content
    // rectangle is the actual rectangle taken up by the text and the
    // clipping rectangle is the dimensions to which this content is
    // clipped, which may be less than or greater than the actual
    // content; both the content rectangle and the clipping rectangle
    // have a minimum size equal to the boundary rectangle

    wxRect m_rect;              // boundary rectangle of cell
    wxRect m_content_rect;      // content rectangle of cell
    wxRect m_clipping_rect;     // clipping rectangle of cell
    int m_row;                  // cell row
    int m_col;                  // cell column
};


// helper class for rendering cell borders
class CellBorder
{
public:

    CellBorder()
    {
    }

    ~CellBorder()
    {
    }

public:

    wxString m_style;
    Color m_color;
    int m_width;

    int m_x1;
    int m_y1;
    int m_x2;
    int m_y2;
};

static void putBorderPropertiesInMap(ICompTablePtr table,
                                     const CellProperties& cell_properties,
                                     std::map< std::pair<int,int>, CellBorder>& horz_borders,
                                     std::map< std::pair<int,int>, CellBorder>& vert_borders)
{
    if (table.isNull())
        return;

    // get the border properties; if we don't have any, we're done
    Properties props;
    props.add(PROP_BORDER_STYLE);
    props.add(PROP_BORDER_TOP_STYLE);
    props.add(PROP_BORDER_BOTTOM_STYLE);
    props.add(PROP_BORDER_LEFT_STYLE);
    props.add(PROP_BORDER_RIGHT_STYLE);
    
    if (!cell_properties.getProperties(props))
        return;

    CellRange range = cell_properties.m_range;
    int r1 = range.row1();
    int r2 = range.row2();
    int c1 = range.col1();
    int c2 = range.col2();

    if (r1 == -1)
        r1 = 0;
    if (c1 == -1)
        c1 = 0;
    if (r2 == -1)
        r2 = table->getRowCount();
    if (c2 == -1)
        c2 = table->getColumnCount();
    
    int row, col;
    for (row = r1; row <= r2; ++row)
    {
        for (col = c1; col <= c2; ++col)
        {
            PropertyValue style;
            CellBorder border;
            border.m_color = COLOR_BLACK;       // TODO: allow different colors
            border.m_width = 1;                 // TODO: allow different widths

            if (props.get(PROP_BORDER_TOP_STYLE, style))
            {
                std::pair<int,int> pair;
                pair.first = row;
                pair.second = col;
            
                border.m_style = style.getString();

                if (border.m_style == BORDER_STYLE_NONE)
                {
                    horz_borders.erase(pair);
                }
                else
                {
                    border.m_x1 = table->getColumnPosByIdx(pair.second);
                    border.m_y1 = table->getRowPosByIdx(pair.first);
                    border.m_x2 = border.m_x1 + table->getColumnSize(pair.second);
                    border.m_y2 = border.m_y1;

                    horz_borders[pair] = border;
                }
            }

            if (props.get(PROP_BORDER_BOTTOM_STYLE, style))
            {
                std::pair<int,int> pair;
                pair.first = row + 1;
                pair.second = col;
            
                border.m_style = style.getString();

                if (border.m_style == BORDER_STYLE_NONE)
                {
                    horz_borders.erase(pair);
                }
                else
                {
                    border.m_x1 = table->getColumnPosByIdx(pair.second);
                    border.m_y1 = table->getRowPosByIdx(pair.first);
                    border.m_x2 = border.m_x1 + table->getColumnSize(pair.second);
                    border.m_y2 = border.m_y1;                
                
                    horz_borders[pair] = border;
                }
            }

            if (props.get(PROP_BORDER_LEFT_STYLE, style))
            {
                std::pair<int,int> pair;
                pair.first = row;
                pair.second = col;    
            
                border.m_style = style.getString();

                if (border.m_style == BORDER_STYLE_NONE)
                {
                    vert_borders.erase(pair);
                }
                else
                {
                    border.m_x1 = table->getColumnPosByIdx(pair.second);
                    border.m_y1 = table->getRowPosByIdx(pair.first);
                    border.m_x2 = border.m_x1;
                    border.m_y2 = border.m_y1 + table->getRowSize(pair.first);
                
                    vert_borders[pair] = border;
                }
            }

            if (props.get(PROP_BORDER_RIGHT_STYLE, style))
            {
                std::pair<int,int> pair;
                pair.first = row;
                pair.second = col + 1;    
            
                border.m_style = style.getString();

                if (border.m_style == BORDER_STYLE_NONE)
                {
                    vert_borders.erase(pair);
                }
                else
                {
                    border.m_x1 = table->getColumnPosByIdx(pair.second);
                    border.m_y1 = table->getRowPosByIdx(pair.first);
                    border.m_x2 = border.m_x1;
                    border.m_y2 = border.m_y1 + table->getRowSize(pair.first);                
                
                    vert_borders[pair] = border;
                }
            }        
        }
    }
}

static void putBorderPropertiesInRenderer(IComponentPtr renderer, 
                                          int row, int col,
                                          std::map< std::pair<int,int>, CellBorder>& horz_borders,
                                          std::map< std::pair<int,int>, CellBorder>& vert_borders)
{
    if (renderer.isNull())
        return;

    Properties props;
    props.add(PROP_BORDER_TOP_STYLE, BORDER_STYLE_NONE);
    props.add(PROP_BORDER_BOTTOM_STYLE, BORDER_STYLE_NONE);
    props.add(PROP_BORDER_LEFT_STYLE, BORDER_STYLE_NONE);
    props.add(PROP_BORDER_RIGHT_STYLE, BORDER_STYLE_NONE);    

    // top border
    std::pair<int,int> top_border;
    top_border.first = row;
    top_border.second = col;
    if (horz_borders.find(top_border) != horz_borders.end())
    {
        props.add(PROP_BORDER_TOP_STYLE, BORDER_STYLE_SOLID);
    }
    
    // bottom border
    std::pair<int,int> bottom_border;
    bottom_border.first = row + 1;
    bottom_border.second = col;
    if (horz_borders.find(bottom_border) != horz_borders.end())
    {
        props.add(PROP_BORDER_BOTTOM_STYLE, BORDER_STYLE_SOLID);
    }

    // left border
    std::pair<int,int> left_border;
    left_border.first = row;
    left_border.second = col;    
    if (vert_borders.find(left_border) != vert_borders.end())
    {
        props.add(PROP_BORDER_LEFT_STYLE, BORDER_STYLE_SOLID);
    }

    // right border
    std::pair<int,int> right_border;
    right_border.first = row;
    right_border.second = col + 1;
    if (vert_borders.find(right_border) != vert_borders.end())
    {
        props.add(PROP_BORDER_RIGHT_STYLE, BORDER_STYLE_SOLID);
    }
    
    renderer->addProperties(props);
}

CompTableModel::CompTableModel()
{
    // default colors
    Color default_color_text = COLOR_BLACK;
    Color default_color_cellbackground = COLOR_WHITE;
    Color default_color_gridlines = COLOR_LIGHT_GREY;

    Font default_font;
    PropertyValue font_facename;
    PropertyValue font_size;
    PropertyValue font_style;
    PropertyValue font_weight;
    PropertyValue font_underscore;

    setPropertiesFromFont(default_font,
                          font_facename,
                          font_size,
                          font_style,
                          font_weight,
                          font_underscore);

    // create list of default cell properties
    m_cell_properties_default.add(PROP_SIZE_W, TABLE_CELL_WIDTH);
    m_cell_properties_default.add(PROP_SIZE_H, TABLE_CELL_HEIGHT);
    m_cell_properties_default.add(PROP_COLOR_BG, default_color_cellbackground);
    m_cell_properties_default.add(PROP_COLOR_FG, default_color_gridlines);
    m_cell_properties_default.add(PROP_LINE_WIDTH, 1);
    m_cell_properties_default.add(PROP_BORDER_TOP_STYLE, BORDER_STYLE_NONE);
    m_cell_properties_default.add(PROP_BORDER_BOTTOM_STYLE, BORDER_STYLE_NONE);
    m_cell_properties_default.add(PROP_BORDER_LEFT_STYLE, BORDER_STYLE_NONE);
    m_cell_properties_default.add(PROP_BORDER_RIGHT_STYLE, BORDER_STYLE_NONE);
    m_cell_properties_default.add(PROP_FONT_FACENAME, font_facename);
    m_cell_properties_default.add(PROP_FONT_SIZE, font_size);
    m_cell_properties_default.add(PROP_FONT_STYLE, font_style);
    m_cell_properties_default.add(PROP_FONT_WEIGHT, font_weight);
    m_cell_properties_default.add(PROP_FONT_UNDERSCORE, font_underscore);
    m_cell_properties_default.add(PROP_TEXT_COLOR, default_color_text);
    m_cell_properties_default.add(PROP_TEXT_HALIGN, ALIGNMENT_LEFT);
    m_cell_properties_default.add(PROP_TEXT_VALIGN, ALIGNMENT_MIDDLE);
    m_cell_properties_default.add(PROP_TEXT_SPACING, TEXTBOX_LINE_SPACING);
    m_cell_properties_default.add(PROP_TEXT_WRAP, false);
    m_cell_properties_default.add(PROP_CONTENT_MIMETYPE, wxString(wxT("plain/text")));
    m_cell_properties_default.add(PROP_CONTENT_ENCODING, wxString(wxT("")));
    m_cell_properties_default.add(PROP_CONTENT_VALUE, wxString(wxT("")));
    m_cell_properties_default.add(PROP_LOCKED, false);

    // row and column count
    m_row_count = 0;
    m_col_count = 0;
}

CompTableModel::CompTableModel(const CompTableModel& c)
{
    // copy the cell properties
    m_cell_properties = c.m_cell_properties;
    m_cell_properties_default = c.m_cell_properties_default;

    // copy the merged cells
    m_cell_merges = c.m_cell_merges;

    // copy the row and column count
    m_row_count = c.m_row_count;
    m_col_count = c.m_col_count;

    // copy the cache
    m_cache_row_sizes = c.m_cache_row_sizes;
    m_cache_col_sizes = c.m_cache_col_sizes;
}

CompTableModel::~CompTableModel()
{
}

CompTableModel& CompTableModel::operator=(const CompTableModel& c)
{
    if (this == &c)
        return *this;

    // copy the cell properties
    m_cell_properties = c.m_cell_properties;
    m_cell_properties_default = c.m_cell_properties_default;

    // copy the merged cells
    m_cell_merges = c.m_cell_merges;

    // copy the row and column count
    m_row_count = c.m_row_count;
    m_col_count = c.m_col_count;

    // copy the cache
    m_cache_row_sizes = c.m_cache_row_sizes;
    m_cache_col_sizes = c.m_cache_col_sizes;

    return *this;
}

bool CompTableModel::operator==(const CompTableModel& c) const
{
    if (this == &c)
        return true;

    if (m_cell_properties != c.m_cell_properties)
        return false;

    if (m_cell_properties_default != c.m_cell_properties_default)
        return false;

    if (m_cell_merges != c.m_cell_merges)
        return false;

    if (m_row_count != c.m_row_count)
        return false;
        
    if (m_col_count != c.m_col_count)
        return false;

    return true;
}

bool CompTableModel::operator!=(const CompTableModel& c) const
{
    return !(*this == c);
}

ITableModelPtr CompTableModel::create()
{
    return static_cast<ITableModel*>(new CompTableModel);
}

ITableModelPtr CompTableModel::clone()
{
    CompTableModel* c = new CompTableModel;
    c->copy(this);

    return static_cast<ITableModel*>(c);
}

void CompTableModel::copy(ITableModelPtr model)
{
    if (model.isNull())
        return;

    CompTableModel* model_ptr = static_cast<CompTableModel*>(model.p);

    // copy the cell properties
    m_cell_properties = model_ptr->m_cell_properties;
    m_cell_properties_default = model_ptr->m_cell_properties_default;

    // copy the merged cells
    m_cell_merges = model_ptr->m_cell_merges;

    // copy the row and column count
    m_row_count = model_ptr->m_row_count;
    m_col_count = model_ptr->m_col_count;

    // copy the cache
    m_cache_row_sizes = model_ptr->m_cache_row_sizes;
    m_cache_col_sizes = model_ptr->m_cache_col_sizes;   
}

bool CompTableModel::setRowCount(int count)
{
    // note: sets the row count for the table; if count
    // is less than zero, the row count is set to zero

    if (count < 0)
        m_row_count = 0;

    m_row_count = count;
    return true;
}

int CompTableModel::getRowCount() const
{
    // note: returns the row count

    return m_row_count;
}

bool CompTableModel::setColumnCount(int count)
{
    // note: sets the column count for the table; if count
    // is less than zero, the column count is set to zero

    if (count < 0)
        m_col_count = 0;

    m_col_count = count;
    return true;
}

int CompTableModel::getColumnCount() const
{
    // note: returns the column count

    return m_col_count;
}

bool CompTableModel::insertRow(int idx, int count)
{
    // note: inserts rows at a row position; the number of 
    // rows to insert is specified by count and the position 
    // at which to insert them is specified by idx; returns
    // true if rows are inserted, and false otherwise

    // if the count is less than 1, return false
    if (count < 1)
        return false;

    // clear the cache
    clearCache();

    // insert the row in the cell properties and merged cells
    insertRowIntoProperties(m_cell_properties, idx, count);
    insertRowIntoProperties(m_cell_merges, idx, count);

    // increment the row count
    m_row_count += count;
    return true;
}

bool CompTableModel::removeRow(int idx, int count)
{
    // note: removes rows at a row position; the number of
    // rows to remove is specified by count and the position
    // at which to start removing them is specified by idx;
    // returns true if rows are removed, and false otherwise

    // if the count is less than 1, return false
    if (count < 1)
        return false;

    // clear the cache
    clearCache();

    // remove the row from the cell properties and merged cells
    removeRowFromProperties(m_cell_properties, idx, count);
    removeRowFromProperties(m_cell_merges, idx, count);

    // decrement the row count
    m_row_count -= count;
    
    // make sure the row count isn't less than zero
    m_row_count = wxMax(0, m_row_count);
        
    return true;
}

bool CompTableModel::insertColumn(int idx, int count)
{
    // note: inserts columns at a column position; the number of 
    // columns to insert is specified by count and the position 
    // at which to insert them is specified by idx; returns
    // true if columns are inserted, and false otherwise

    // if the count is less than 1, return false
    if (count < 1)
        return false;

    // clear the cache
    clearCache();

    // insert the column in the cell properties and merged cells
    insertColumnIntoProperties(m_cell_properties, idx, count);
    insertColumnIntoProperties(m_cell_merges, idx, count);

    // increment the column count
    m_col_count += count;
    return true;
}

bool CompTableModel::removeColumn(int idx, int count)
{
    // note: removes columns at a column position; the number of
    // columns to remove is specified by count and the position
    // at which to start removing them is specified by idx;
    // returns true if columns are removed, and false otherwise

    // if the count is less than 1, return false
    if (count < 1)
        return false;

    // clear the cache
    clearCache();

    // remove the column from the cell properties and merged cells
    removeColumnFromProperties(m_cell_properties, idx, count);
    removeColumnFromProperties(m_cell_merges, idx, count);

    // decrement the column count
    m_col_count -= count;
    
    // make sure the column count isn't less than zero
    m_col_count = wxMax(0, m_col_count);

    return true;
}

bool CompTableModel::setRowSize(int row, int size)
{
    // check row bounds
    if (row >= m_row_count - 1)
        row = m_row_count - 1;

    if (row < 0)
        row = 0;

    // set the row size
    return setCellProperty(CellRange(row, -1), PROP_SIZE_H, size);
}

int CompTableModel::getRowSize(int row)
{
    // check row bounds
    if (row >= m_row_count - 1)
        row = m_row_count - 1;

    if (row < 0)
        row = 0;
        
    // if the cache is empty, repopulate it
    if (isCacheEmpty())
        populateCache();
    
    // see if there's a specific size in the cache;
    // if there is, return the specific value
    std::map<int,int>::iterator it = m_cache_row_sizes.find(row);
    if (it != m_cache_row_sizes.end())
        return it->second;
        
    // return the default size    
    return m_cache_row_sizes[-1];
}

bool CompTableModel::setColumnSize(int col, int size)
{
    // check column bounds
    if (col > m_col_count - 1)
        col = m_col_count - 1;

    if (col < 0)
        col = 0;

    // set the column size
    return setCellProperty(CellRange(-1, col), PROP_SIZE_W, size);
}

int CompTableModel::getColumnSize(int col)
{
    // check column bounds
    if (col > m_col_count - 1)
        col = m_col_count - 1;

    if (col < 0)
        col = 0;

    // if the cache is empty, repopulate it
    if (isCacheEmpty())
        populateCache();
    
    // see if there's a specific size in the cache;
    // if there is, return the specific value
    std::map<int,int>::iterator it = m_cache_col_sizes.find(col);
    if (it != m_cache_col_sizes.end())
        return it->second;
        
    // return the default size
    return m_cache_col_sizes[-1];
}

int CompTableModel::getColumnPosByIdx(int col)
{
    // check column bounds
    if (col > m_col_count)
        col = m_col_count;

    if (col < 0)
        col = 0;

    // if the cache is empty, repopulate it
    if (isCacheEmpty())
        populateCache();

    int pos = 0;
    int default_columns = col;

    // the column position is equal to the sum of the column
    // widths of all column indexes less than the column in
    // question; so iterate through the column widths and add 
    // up the non-default widths, then add the default width
    // times the number of remaining columns occuring before the
    // column in question
    std::map<int,int>::iterator it, it_end;
    it_end = m_cache_col_sizes.end();

    for (it = m_cache_col_sizes.begin(); it != it_end; ++it)
    {
        // if the column index is less than zero, we're in the
        // "default column width" area, so just continue
        if (it->first < 0)
            continue;

        // if we're past the column in question, break
        if (it->first >= col)
            break;

        // if we're any where else, add the width of the column
        // onto the position and subtract one from the default 
        // column count
        pos += it->second;
        --default_columns;
    }

    // the default column width is stored with an index of -1;
    // get the default width and multiply it by the number of
    // default columns left-over; add this to the total width
    // of non-default-width columns
    pos += default_columns*m_cache_col_sizes[-1];
    return pos;
}

int CompTableModel::getColumnIdxByPos(int x)
{
    // if the cache is empty, repopulate it
    if (isCacheEmpty())
        populateCache();
 
    // if x is less than zero, set the column marker to
    // -1 to indicate a range of column, which will
    // highlight the row
    if (x < 0)
        return -1;
        
    // if x is zero, set the column index to the first
    // column, which is 0
    if (x == 0)
        return 0;
 
    // go through the non-default column sizes and add up their
    // widths; the "gaps" in the non-default column sizes are
    // default column sizes, and for these, mulitply the number
    // of default-width columns by the default column size

    int total_default = 0;              // total number of default columns
    int total_non_default = 0;          // total number of non-default columns
    int total_default_width = 0;        // total width of default columns
    int total_non_default_width = 0;    // total width of non-default columns

    int current_pos = 0;
    int default_column_width = m_cache_col_sizes[-1];
    
    std::map<int,int>::iterator it, it_end;
    it_end = m_cache_col_sizes.end();

    for (it = m_cache_col_sizes.begin(); it != it_end; ++it)
    {
        // if the column index is less than zero, we're in the
        // "default column width" area, so just continue
        if (it->first < 0)
            continue;
        
        // calculate the total default width up to this column
        total_default = (it->first - total_non_default);
        total_default_width = total_default*default_column_width;
        
        // calculate the current position up to this point without
        // including the current column
        current_pos = total_default_width + total_non_default_width;
        
        // see if the current position up to this point without
        // the current column exceeds the position we're looking for;
        // if it does, then break: we can calculate the index
        if (current_pos >= x)
            break;
        
        // now see if the current position up to and including the current
        // column exceeds the position we're looking for; if it does, then
        // we know the index: the current column
        if (current_pos + it->second >= x)
            return it->first;
        
        // if we still have not exceeded the position we're looking for,
        // add on the position of the current column and include it
        // in the non-default idx
        total_non_default_width += it->second;

        // increment the number of non-default columns
        total_non_default++;
        
        // now calculate the current position up to this point including
        // the current column
        current_pos = total_default_width + total_non_default_width;
    }

    // calculate the total number of default-width columns up 
    // to this point
    total_default = (x - total_non_default_width)/default_column_width;

    // the index of the current position is the total number of
    // default-width columns up to this point, plus the total
    // number of non-default-width columns up to this point
    int idx = total_default + total_non_default;
    
    // if the idx is greater than or equal to the number of columns, 
    // then return the index of the last column
    if (idx >= m_col_count)
        return wxMax(0, m_col_count - 1);

    // return the index
    return idx;
}

int CompTableModel::getRowPosByIdx(int row)
{
    // check row bounds
    if (row > m_row_count)
        row = m_row_count;

    if (row < 0)
        row = 0;

    // if the cache is empty, repopulate it
    if (isCacheEmpty())
        populateCache();

    int pos = 0;
    int default_rows = row;

    // the row position is equal to the sum of the row heights
    // of all row indexes less than the row in question; so iterate 
    // through the row heights and add up the non-default heights, 
    // then add the default height times the number of remaining rows
    // occuring before the row in question
    std::map<int,int>::iterator it, it_end;
    it_end = m_cache_row_sizes.end();

    for (it = m_cache_row_sizes.begin(); it != it_end; ++it)
    {
        // if the row index is less than zero, we're in the
        // "default row height" area, so just continue
        if (it->first < 0)
            continue;

        // if we're past the row in question, break
        if (it->first >= row)
            break;

        // if we're any where else, add the height of the row
        // onto the position and subtract one from the default 
        // row count
        pos += it->second;
        --default_rows;
    }

    // the default row height is stored with an index of -1;
    // get the default height and multiply it by the number of
    // default rows left-over; add this to the total height
    // of non-default-height rows
    pos += default_rows*m_cache_row_sizes[-1];
    return pos;
}

int CompTableModel::getRowIdxByPos(int y)
{
    // if the cache is empty, repopulate it
    if (isCacheEmpty())
        populateCache();
 
    // if y is less than zero, set the row marker to
    // -1 to indicate a range of rows, which will
    // highlight the column
    if (y < 0)
        return -1;
        
    // if y is zero, set the row index to the first
    // row, which is 0
    if (y == 0)
        return 0;
 
    // go through the non-default row sizes and add up their
    // widths; the "gaps" in the non-default row sizes are
    // default row sizes, and for these, mulitply the number
    // of default-width rows by the default row size

    int total_default = 0;              // total number of default rows
    int total_non_default = 0;          // total number of non-default rows
    int total_default_width = 0;        // total width of default rows
    int total_non_default_width = 0;    // total width of non-default rows

    int current_pos = 0;
    int default_row_width = m_cache_row_sizes[-1];
    
    std::map<int,int>::iterator it, it_end;
    it_end = m_cache_row_sizes.end();

    for (it = m_cache_row_sizes.begin(); it != it_end; ++it)
    {
        // if the row index is less than zero, we're in the
        // "default row width" area, so just continue
        if (it->first < 0)
            continue;
        
        // calculate the total default width up to this row
        total_default = (it->first - total_non_default);
        total_default_width = total_default*default_row_width;
        
        // calculate the current position up to this point without
        // including the current row
        current_pos = total_default_width + total_non_default_width;
        
        // see if the current position up to this point without
        // the current row exceeds the position we're looking for;
        // if it does, then break: we can calculate the index
        if (current_pos >= y)
            break;
        
        // now see if the current position up to and including the current
        // row exceeds the position we're looking for; if it does, then
        // we know the index: the current row
        if (current_pos + it->second >= y)
            return it->first;
        
        // if we still have not exceeded the position we're looking for,
        // add on the position of the current row and include it
        // in the non-default idx
        total_non_default_width += it->second;

        // increment the number of non-default rows
        total_non_default++;
        
        // now calculate the current position up to this point including
        // the current row
        current_pos = total_default_width + total_non_default_width;
    }

    // calculate the total number of default-width rows up 
    // to this point
    total_default = (y - total_non_default_width)/default_row_width;

    // the index of the current position is the total number of
    // default-width rows up to this point, plus the total number
    // of non-default-width rows up to this point
    int idx = total_default + total_non_default;
    
    // if the idx is greater than or equal to the number of rows, 
    // then return the index of the last row
    if (idx >= m_row_count)
        return wxMax(0, m_row_count - 1);

    // return the index
    return idx;
}

void CompTableModel::getCellIdxByPos(int x, int y, int* row, int* col, bool merged)
{
    // note: this function returns the row and column of the cell from
    // an input position; if merged is false, then the function returns
    // the row and column of the cell under any merged cell; if merged
    // is true, then the function returns the row and column of the 
    // merged cell for the input position

    // get an initial row and column from the x position
    int r, c;
    r = getRowIdxByPos(y);
    c = getColumnIdxByPos(x);

    // if row or column are range markers, or the merged flag is false,
    // or the cell isn't merged, simply return the column and row index, 
    // and we're done
    CellRange cell(r, c);
    if (r == -1 || c == -1 || !merged || !isMerged(cell))
    {
        *row = r;
        *col = c;
        return;
    }

    // if the cell is a merged cell, return the row and column
    // of the merged cell
    *row = cell.row1();
    *col = cell.col1();
}
 
void CompTableModel::getCellPosByIdx(int row, int col, int* x, int* y, bool merged)
{
    // note: this function returns the x and y position of the cell from
    // an input row and column; if merged is false, then the function 
    // returns the position of the cell under any merged cell; if merged
    // is true, then the function returns the position of the merged
    // cell for the input row and column

    // if row or column are range markers, or if the merged flag is false,
    // or if the cell isn't merged, simply return the column and row sizes
    CellRange cell(row, col);
    if (row == -1 || col == -1 || !merged || !isMerged(cell))
    {
        *x = getColumnPosByIdx(col);
        *y = getRowPosByIdx(row);
        return;
    }

    // find the position of the merged cell
    *x = getColumnPosByIdx(cell.col1());
    *y = getRowPosByIdx(cell.row1());
}
 
void CompTableModel::getCellSizeByIdx(int row, int col, int* w, int* h, bool merged)
{
    // note: this function returns the width and height of the cell from
    // an input row and column; if merged is false, then the function 
    // returns the width and height of the cell under any merged cell;
    // if merged is true, then the function returns the width and height
    // of the merged cell for the input row and column

    // if the merged flag is false or if the cell isn't merged, simply 
    // return the column and row sizes
    CellRange merged_cells(row, col);
    if (!merged || !isMerged(merged_cells))
    {
        *w = getColumnSize(col);
        *h = getRowSize(row);
        return;
    }

    // the cell is merged, so get the range
    int row1, col1, row2, col2;
    merged_cells.get(&row1, &col1, &row2, &col2);

    // find the positions of the starting and ending cell, and
    // add on the width and height of the second cell to get
    // the overall size of the span; note: we have to use
    // getColumnPosByIdx() and getRowPosByIdx() to get the
    // position of the (row2, col2), because getCellPosByIdx()
    // returns the position of (row1, col1) for merged cells
    int x1, y1, x2, y2;
    getCellPosByIdx(row1, col1, &x1, &y1);
    x2 = getColumnPosByIdx(col2);
    y2 = getRowPosByIdx(row2);
    x2 += getColumnSize(col2);
    y2 += getRowSize(row2);

    // subtract the starting point from the ending point to
    // get the width and the height of the cell
    *w = x2 - x1;
    *h = y2 - y1;
}
 
void CompTableModel::getCellCenterByIdx(int row, int col, int* x, int* y, bool merged)
{
    // note: this function returns the x and y position of the center of
    // a cell from an input row and column; if merged is false, then the 
    // function returns the position of the center of the cell under any 
    // merged cell; if merged is true, then the function returns the center
    // position of the merged cell for the input row and column

    // get the dimensions of the cell
    int w, h;
    getCellPosByIdx(row, col, x, y, merged);
    getCellSizeByIdx(row, col, &w, &h, merged);

    // calculate and return the center position of the cell
    *x = *x + w/2;
    *y = *y + h/2;
}

void CompTableModel::addCellProperty(const wxString& prop_name,
                                     const PropertyValue& value)
{
    // note: in order to set properties on a range of cells using 
    // setCellProperty(), a property must exist in the default list 
    // of cell properties; this function allows additional properties 
    // to be added to the default properties so this can be done

    m_cell_properties_default.add(prop_name, value);
}

void CompTableModel::addCellProperties(const std::vector<CellProperties>& properties,
                                       bool replace)
{
    // note: this function adds the properties across their respective
    // ranges to the current table; if the properties don't already exist 
    // in the list of default properties, they are added; if replace is 
    // true, the added properties replace the existing properties of the 
    // same type, without regard to range; if replace is false, the 
    // properties and their corresponding ranges are added on top of 
    // the existing properties and their ranges

    // compile a list of the new properties
    Properties new_properties;

    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = properties.end();
    
    for (it = properties.begin(); it != it_end; ++it)
    {
        new_properties.add(it->properties());
    }

    // if we're adding a column width or row height property
    // (to overwrite a previous value), clear the cache
    if (new_properties.has(PROP_SIZE_W) || 
        new_properties.has(PROP_SIZE_H))
    {
        clearCache();
    }

    // add the new properties to the default properties; but if 
    // a property already exists, use the value already defined
    // in the default properties
    Properties new_default_properties = new_properties;
    new_default_properties.add(m_cell_properties_default);
    m_cell_properties_default = new_default_properties;

    // if the replace flag is set, then remove the existing
    // properties of the type that are being added
    if (replace)
    {
        // create a list of properties to save
        std::vector<CellProperties> cell_properties_saved;
        cell_properties_saved.reserve(m_cell_properties.size());

        std::vector<CellProperties>::iterator it, it_end;
        it_end = m_cell_properties.end();
        
        for (it = m_cell_properties.begin(); it != it_end; ++it)
        {
            // remove any new properties we're adding
            it->m_properties.remove(new_properties);
          
            // if there are properties left over, add them to
            // the list of properties to save
            if (!it->m_properties.isEmpty())
                cell_properties_saved.push_back(*it);
        }

        // copy the saved properties to the cell properties
        m_cell_properties = cell_properties_saved;
    }

    // add the new properties
    int new_size = m_cell_properties.size() + properties.size();
    m_cell_properties.reserve(new_size);

    for (it = properties.begin(); it != it_end; ++it)
    {
        m_cell_properties.push_back(*it);
    }
}

bool CompTableModel::removeCellProperty(const wxString& prop_name)
{
    // note: this function removes properties from the
    // default list of properties
    
    // TODO: shouldn't allow removal of some key properties,
    // such as row/column size

    // if we can't find the property, return false
    PropertyValue value;
    if (!m_cell_properties_default.get(prop_name, value))
        return false;

    // remove the property and return true
    m_cell_properties_default.remove(prop_name);
    return true;
}

bool CompTableModel::setCellProperty(const CellRange& range, 
                                     const wxString& prop_name, 
                                     const PropertyValue& value)
{
    // note: this function checks to see if a given property can be
    // set as a cell property; if it's a cell property and the value
    // is different than the value across the cell range in question,
    // the function returns true; false otherwise
    
    // see if the property is a cell property; if not, return false
    PropertyValue current_value;
    if (!m_cell_properties_default.get(prop_name, current_value))
        return false;
 
    // get the value across the cell range; if the value is the same,
    // return false to indicate the value isn't changed
    getCellProperty(range, prop_name, current_value);
    if (current_value == value)
        return false;

    // if we're setting the row or column size, clear the cache
    if (prop_name == PROP_SIZE_W || prop_name == PROP_SIZE_H)
        clearCache();

    // set the property range, the properties and add it to the collection
    CellProperties c;
    c.m_range = range;
    c.addProperty(prop_name, value);
    m_cell_properties.push_back(c);
    return true;
}

bool CompTableModel::setCellProperties(const CellRange& range, 
                                       const Properties& properties)
{
    // TODO: move delete code from clearCells() to this function to make
    // sure properties don't start accumulating needlessly

    // note: this function takes a list of input properties and tries to
    // set them on the range of input cells; if any of the input properties
    // are set across the range, the function returns true; false otherwise

    // limit the input properties to those we allow on cells; if none of
    // the properties are cell properties, return false
    Properties valid_properties = properties;
    m_cell_properties_default.get(valid_properties);
    valid_properties.set(properties);
    if (valid_properties.count() == 0)
        return false;

    // get all cell properties that intersect the input range;
    std::vector<CellProperties> cell_properties;
    getCellProperties(range, cell_properties);

    // look for variations between the cell properties that intersect the
    // input range and the input properties; if there are differences, add
    // them to the list of new properties
    Properties new_properties;
    std::vector<CellProperties>::iterator it, it_end;
    it_end = cell_properties.end();
 
    for (it = cell_properties.begin(); it != it_end; ++it)
    {
        // copy the cell properties and set the values of the copy
        // to the new property value; then, take the difference between 
        // the newly set cell properties and the original; if there are 
        // any differences, add them to the list of new properties
        Properties p = it->m_properties;
        p.set(valid_properties);
        new_properties.add(p.subtract(it->m_properties));
    }

    // because cell properties completely cover the range, we may have
    // detected differences between the input properties and some range
    // of cell properties that's superceded by another range of the same
    // property on top of it; we don't care about setting the properties
    // for these cells as long as the property that covers it is the
    // same as the input property; so, in the new list of properties to
    // set, remove any properties that completely cover the input range
    // and have the same value as the input property
    Properties constant_properties;
    getCellProperties(range, constant_properties);
    new_properties.subtract(constant_properties);

    // if there's no difference between the current cell properties
    // and any of the input properties, we're done
    if (new_properties.count() == 0)
        return false;

    // if one of the properties we're setting is the width or height,
    // clear the cache
    PropertyValue value;
    if (new_properties.get(PROP_SIZE_W, value) ||
        new_properties.get(PROP_SIZE_H, value))
    {
        clearCache();
    }

    // set the cell properties
    CellProperties p;
    p.m_range = range;
    p.m_properties = new_properties;
    m_cell_properties.push_back(p);
    return true;
}

bool CompTableModel::getCellProperty(const CellRange& range, 
                                     const wxString& prop_name, 
                                     PropertyValue& value) const
{
    // note: this function gets the value of the cell property
    // if it's constant across the input range; returns true
    // if the value exists, false otherwise
    Properties p;
    getCellProperties(range, p);
    return p.get(prop_name, value);
}

bool CompTableModel::getCellProperty(const CellRange& range,
                                     const wxString& prop_name,
                                     std::vector<CellProperties>& properties) const
{
    // note: this function returns the specified propery across all ranges 
    // that intersect the input range; returns true if the intersection
    // contains any members, and false otherwise

    // used throughout
    PropertyValue value;

    // make sure the list of cell properties are clear
    properties.clear();
    properties.reserve(m_cell_properties.size());

    // if the property exists in the default property, then add it
    if (m_cell_properties_default.get(prop_name, value))
    {
        CellProperties props;
        props.setRange(-1, -1, -1, -1);     // default properties cover the whole table
        props.addProperty(prop_name, value);
        properties.push_back(props);
    }

    // iterate through the list cell properties and copy any of the
    // specified property that intersect the input range to the output
    // cell properties
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_properties.end();

    for (it = m_cell_properties.begin(); it != it_end; ++it)
    {
        // if the range of cells we're interested in doesn't
        // intersect the range of cell properties, move on
        if (!range.intersects(it->m_range))
            continue;

        // if we have the property in the range, add it to the list
        if (it->m_properties.get(prop_name, value))
        {
            CellProperties props;
            props.setRange(it->m_range);
            props.addProperty(prop_name, value);
            properties.push_back(props);
        }
    }
    
    // if we couldn't find any of the named property intersecting
    // the specified range, return false
    if (properties.empty())
        return false;
        
    // otherwise, return true
    return true;
}

bool CompTableModel::getCellProperties(const CellRange& range, 
                                       Properties& properties) const
{
    // note: this function gets all cell properties that both
    // intersect the input range and are constant across the
    // input range; returns true if any properties are returned,
    // and false otherwise

    // TODO: handle locked cells

    // start with the default properties
    properties = m_cell_properties_default;

    // iterate through to get the properties
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_properties.end();
    
    for (it = m_cell_properties.begin(); it != it_end; ++it)
    {
        // see if the input range and the property range intersect;
        // if they don't, move on
        CellRange r = range;
        if (r.intersect(it->m_range).isEmpty())
            continue;

        // if the ranges intersect and the range of the intersection
        // is the same as the range of the input cells, the cell
        // properties completely cover the range, so add them
        // to the list of properties
        if (r == range)
        {
            properties.add(it->m_properties);
            continue;
        }

        // finally, if the ranges intersect, but the range of the
        // intersection is not the same as the range of the input
        // cells, the cell properties may still be the same as a
        // previous constant property range already added to the
        // list; we can find this out by intersecting the constant
        // properties we've already found with this new range of
        // cell properties that's on top of it; if there are any
        // differences between this and the previous constant
        // properties, they'll be removed from the list of constant
        // properties
        properties.intersect(it->m_properties);
    }
    
    // TODO: for now, we should always have properties across
    // a cell range, so the function will always return true;
    // this is fine, but it would be nice to make this function
    // consistent with the behavior of Properities::get(), which
    // returns properties that are specified in an initial list
    
    // if we don't have any properties, return false; otherwise
    // return true
    if (properties.count() == 0)
        return false;
        
    return true;
}

void CompTableModel::setDefaultCellProperties(Properties& properties)
{
    m_cell_properties_default = properties;
}

void CompTableModel::getDefaultCellProperties(Properties& properties) const
{
    properties = m_cell_properties_default;
}

void CompTableModel::getCellProperties(const CellRange& range,
                                       std::vector<CellProperties>& properties) const
{
    // note: this function returns all cell properties that
    // both intersect the input range, including those whose
    // values may vary across the input range

    // make sure the list of cell properties are clear
    properties.clear();
    properties.reserve(m_cell_properties.size());

    // add the default properties
    CellProperties cell_props(CellRange(-1, -1), m_cell_properties_default);
    properties.push_back(cell_props);

    // iterate through the list cell properties and copy any
    // properties that intersect the input range to the output
    // cell properties
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_properties.end();

    for (it = m_cell_properties.begin(); it != it_end; ++it)
    {
        // if the range of cells we're interested in doesn't
        // intersect the range of cell properties, move on
        if (!range.intersects(it->m_range))
            continue;

        // if it intersects, add the cell properties to the list
        properties.push_back(*it);
    }
}

void CompTableModel::mergeCells(const CellRange& range)
{
    // note: this function takes an input range of cells and merges
    // them; if the input range intersects any previously merged cells
    // the input range is exanded to include these merged cells and
    // the originals are removed

    // get the input range to merge
    int row1, col1, row2, col2;
    range.get(&row1, &col1, &row2, &col2);

    // if we only have a single cell, don't merge it; we're done
    if (row1 == row2 && col1 == col2 && row1 != -1 && col1 != -1)
        return;

    // get the bounding range of the input range and use this as the
    // actual range of the cells we're merging so that the cells we're
    // merging merge with previously merged cells
    CellRange new_merged_cells;
    getBoundingRange(m_cell_merges, range, new_merged_cells);

    // make a copy the previously merged cells and clear them
    std::vector<CellProperties> copy_cell_merges = m_cell_merges;
    m_cell_merges.clear();

    // iterate through the previously merged cells and re-add any 
    // that don't intersect the new cells we're merging
    std::vector<CellProperties>::iterator it, it_end;
    it_end = copy_cell_merges.end();
    
    for (it = copy_cell_merges.begin(); it != it_end; ++it)
    {
        if (!it->m_range.intersects(new_merged_cells))
            m_cell_merges.push_back(*it);
    }
    
    // add the newly merged cells
    CellProperties merged_cells;
    merged_cells.m_range = new_merged_cells;
    m_cell_merges.push_back(merged_cells);
}

void CompTableModel::unmergeCells(const CellRange& range)
{
    // if we don't have any merged cells, we're done
    if (m_cell_merges.empty())
        return;

    // scan through the merged cells and remove all those
    // that intersect the input range
    std::vector<CellProperties> merged_cells;
    merged_cells.reserve(m_cell_merges.size());
    
    std::vector<CellProperties>::iterator it, it_end;
    it_end = m_cell_merges.end();

    for (it = m_cell_merges.begin(); it != it_end; ++it)
    {
        CellRange r = range;
        if (!r.intersect(it->m_range).isEmpty())
            continue;
            
        merged_cells.push_back(*it);
    }
    
    m_cell_merges = merged_cells;
}

void CompTableModel::unmergeAllCells()
{
    m_cell_merges.clear();
}

void CompTableModel::getMergedCells(std::vector<CellRange>& merges) const
{
    // make sure the merges vector is clear
    merges.clear();
    merges.reserve(m_cell_merges.size());
    
    // add the merged cell ranges    
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_merges.end();

    for (it = m_cell_merges.begin(); it != it_end; ++it)
    {
        merges.push_back(it->m_range);
    }
}

bool CompTableModel::isMerged(CellRange& range) const
{
    // if we don't have any merged cells, we're done;
    // return false
    if (m_cell_merges.empty())
        return false;

    // if any merged cells intersect the input range,
    // set the input range to the range of the merged cells
    // and return true
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_merges.end();
    
    for (it = m_cell_merges.begin(); it != it_end; ++it)
    {
        if (range.intersects(it->m_range))
        {
            range = it->m_range;
            
            // if range markers are set on the row or column,
            // set them to the row or column start or end to
            // ease calculations using the range result
            CellRange r(0, 0, getRowCount() - 1, getColumnCount() - 1);
            range.intersect(r);

            return true;
        }
    }
    
    // if the input range doesn't intersect, return
    // false
    return false;
}

bool CompTableModel::tag(const wxString& tag)
{
    // note: this function adds a property to the stack of table 
    // properties with the given tag name so that the table can be 
    // returned to a given state later using restore(); this is 
    // intended to be a light-weight way of getting back to a given 
    // set of properties rather than a heavy-duty undo/redo mechanism, 
    // which exists as an official class; example uses include 
    // restoring the defaults of a table or adding on temporary 
    // properties in an action for real-time feedback

    // note: right now, the function only works with
    // properties, not merged cells

    // if we don't have a tag, we're done; return false
    if (tag.IsEmpty())
        return false;

    // add a default prefix onto the tagname to ensure it doesn't
    // conflict with other properties, and add the tagname as
    // a cell property
    wxString tag_name = TAG_PREFIX + tag;
    
    // add a cell property with the tag name; add the property
    // manually so we can bypass having to first add the
    // property as a default cell property
    CellProperties props(CellRange(-1,-1));
    props.addProperty(tag_name, wxT(""));
    m_cell_properties.push_back(props);

    // TODO: add tag layer for merged cells

    return true;
}

bool CompTableModel::restore(const wxString& tag)
{
    // note: this function restores the properties to what they 
    // were at a given tag, specified by tag(); if the tag doesn't 
    // exist, the function returns false without restoring any
    // previous states

    // note: right now, the function only works with properties, 
    // not merged cells

    // if we don't have a tag, we're done; return false
    if (tag.IsEmpty())
        return false;

    // add a tag prefix to reconstruct the full tagname
    wxString tag_name = TAG_PREFIX + tag;

    // clear the cache
    clearCache();

    // find the index of the tag
    std::vector<CellProperties>::reverse_iterator itr, itr_end;
    itr_end = m_cell_properties.rend();

    int idx = 0;
    bool has_tag = false;

    for (itr = m_cell_properties.rbegin(); itr != itr_end; ++itr)
    {
        PropertyValue value;
        if (itr->getProperty(tag_name, value))
        {
            has_tag = true;
            break;
        }

        ++idx;
    }

    // if we couldn't find the tag, we're done
    if (!has_tag)
        return false;

    // the index the index of the tag, relative
    // to the beginning of the properties
    idx = m_cell_properties.size() - idx - 1;

    // cut off the cell properties at the tag
    m_cell_properties.resize(idx);

    // TODO: add restore for merged cells

    return true;
}

void CompTableModel::compact()
{
    // note: this function removes cell properties that are completely 
    // hidden by other cell properties; for example, if the color of 
    // cell (2,2) is set to red and then later to green, compact() will
    // remove the red property because the green property covers it;
    // likewise, if the cells in the range (2,2,3,3) are set to red, 
    // and then the cells in the range (1,1,4,4) are set to green, 
    // compact will remove the red property because the green property 
    // covers the red property
    
    // note: currently, this function is O(n^2) where n is the number 
    // of cell properties; however, the very purpose of this function 
    // is to significantly cut down on the number of properties by 
    // removing all cell properties that don't contribute in some way 
    // to the final look of the table, so if it is used strategically 
    // (such as right before saving the table), the O(n^2) nature 
    // shouldn't be a problem

    std::vector<CellProperties>::reverse_iterator itr, itr_end;
    itr_end = m_cell_properties.rend();

    std::vector<CellProperties>::reverse_iterator itr_inner, itr_inner_end;
    itr_inner_end = m_cell_properties.rend();

    // reverse iterate through the cell properties; these are
    // the properties that may be covering other properties   
    for (itr = m_cell_properties.rbegin(); itr != itr_end; ++itr)
    {
        // for each cell property, flag all cell properties underneath 
        // the current cell properties that are hidden by them; these 
        // cell properties are hidden if all their property types are 
        // contained in the covering cell property types and if their 
        // range is also contained in the covering cell range

        // if the covering properties themselves are flagged, all 
        // cell properties that might have been covered by these 
        // cell properties are already covered; move on
        if (itr->isFlagged())
            continue;

        for (itr_inner = itr; itr_inner != itr_inner_end; ++itr_inner)
        {
            // if we're testing the covering cell properties against
            // themselves, move on, since we only want to flag other 
            // properties that are covered; the reason the covering 
            // cell properties are compared against themselves is
            // for simplicity in the for() loop construction
            if (itr_inner == itr)
                continue;
        
            // if the current cell properties are already flagged, 
            // move on
            if (itr_inner->isFlagged())
                continue;

            // if the range of the current cell properties aren't 
            // contained in the range of the covering cell properties, 
            // move on
            if (!(itr->range().contains(itr_inner->range())))
                continue;

            // if the property types of the current cell properties 
            // aren't contained in the property types of the covering 
            // cell properties, move on
            if (!(itr->properties().has(itr_inner->properties())))
                continue;

            // flag the current cell properties
            itr_inner->setFlag(true);
        }
    }

    // save all the cell properties that aren't flagged
    std::vector<CellProperties> cell_properties_saved;
    cell_properties_saved.reserve(m_cell_properties.size());

    std::vector<CellProperties>::iterator it, it_end;
    it_end = m_cell_properties.end();

    for (it = m_cell_properties.begin(); it != it_end; ++it)
    {
        if (!it->isFlagged())
            cell_properties_saved.push_back(*it);
    }

    // set the cell properties to the saved properties
    m_cell_properties = cell_properties_saved;
}

void CompTableModel::reset()
{
    clearCache();

    m_cell_properties.clear();
    m_cell_merges.clear();

    m_row_count = 0;
    m_col_count = 0;
}

void CompTableModel::eval(int row, int col, Properties& properties) const
{
    // note: this function evaluates the cell at the specified
    // row and column and returns the properties for that cell;

    // TODO: the following method of handling locked cells works 
    // fine from the standpoint of evaluating the properties of 
    // the cell; however, doing it this way puts the returned 
    // properties out of sync with other functions that rely on 
    // getCellProperty() or getCellProperties(), where locked 
    // cells are not taken into account; need to handle locked 
    // cells uniformly

    PropertyValue value;

    // get the default properties and add on row and column properties
    properties = m_cell_properties_default;

    // find out the initial cell locked state
    bool locked = false;
    if (properties.get(PROP_LOCKED, value))
        locked = value.getBoolean();

    // iterate through the cell properties and put them
    // in the renderer
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_properties.end();

    for (it = m_cell_properties.begin(); it != it_end; ++it)
    {
        CellRange cell(row, col);
        CellRange cell_bottom_adj(row+1, col);
        CellRange cell_top_adj(row-1, col);
        CellRange cell_left_adj(row, col-1);
        CellRange cell_right_adj(row, col+1);
        
        if (it->contains(cell))
        {
            // if the cell is contained in a cell range and the
            // cell isn't locked, add the properties
        
            if (!locked)
            {
                // add the properties for this cell to the list of
                // accumulated properties
                properties.add(it->m_properties);
                
                // set the locked property
                if (properties.get(PROP_LOCKED, value))
                    locked = value.getBoolean();
            }
        }
        else
        {
            // if the cell isn't contained in the cell range,
            // look to the cells adjacent to the current cell
            // for borders
            PropertyValue value;
            if (it->contains(cell_bottom_adj) && it->getProperty(PROP_BORDER_TOP_STYLE, value))
                properties.add(PROP_BORDER_BOTTOM_STYLE, value);
            if (it->contains(cell_top_adj) && it->getProperty(PROP_BORDER_BOTTOM_STYLE, value))
                properties.add(PROP_BORDER_TOP_STYLE, value);
            if (it->contains(cell_left_adj) && it->getProperty(PROP_BORDER_RIGHT_STYLE, value))
                properties.add(PROP_BORDER_LEFT_STYLE, value);
            if (it->contains(cell_right_adj) && it->getProperty(PROP_BORDER_LEFT_STYLE, value))
                properties.add(PROP_BORDER_RIGHT_STYLE, value);
        }
    }
}

bool CompTableModel::isCacheEmpty() const
{
    // if row size and column size caches are all populated, 
    // return false
    if (m_cache_row_sizes.size() > 0 && 
        m_cache_col_sizes.size() > 0)
    {
        return false;
    }

    // otherwise return true
    return true;
}

void CompTableModel::populateCache()
{
    // property value
    PropertyValue value;

    // make sure the cache is cleared
    clearCache();

    // set the default value for the column width
    m_cell_properties_default.get(PROP_SIZE_W, value);
    m_cache_col_sizes[-1] = value.getInteger();

    // set the default value for the row height    
    m_cell_properties_default.get(PROP_SIZE_H, value);
    m_cache_row_sizes[-1] = value.getInteger();
 
    // iterate through the cell properties and populate the row and 
    // column size caches with the column width and row height
    bool has_property;
    int row1, col1, row2, col2;
    
    std::vector<CellProperties>::iterator it, it_end;
    it_end = m_cell_properties.end();
    
    for (it = m_cell_properties.begin(); it != it_end; ++it)
    {
        it->getRange(&row1, &col1, &row2, &col2);
    
        // if this property range is a global range, and the width and 
        // height properties exist, then these supercede all previous
        // width and height values; so clear any individual values that
        // have already been stored and set new default values
        if (it->contains(-1, -1))
        {
            // if the column width exists, store it
            has_property = it->getProperty(PROP_SIZE_W, value);
            if (has_property)
            {
                m_cache_col_sizes.clear();
                m_cache_col_sizes[-1] = value.getInteger();
            }

            // if the column height exists, store it     
            has_property = it->getProperty(PROP_SIZE_H, value);
            if (has_property)
            {
                m_cache_row_sizes.clear();
                m_cache_row_sizes[-1] = value.getInteger();
            }

            // move onto the next property
            continue;
        }

        // if this property range is a column, and the width property exists, 
        // store it in the cache as the width for this column range
        if (it->contains(-1, col1))
        {
            // if the column width exists, store it
            has_property = it->getProperty(PROP_SIZE_W, value);
            if (has_property)
            {
                int i;
                int size = value.getInteger();
                for (i = col1; i <= col2; ++i)
                {
                    m_cache_col_sizes[i] = size;
                }
            }

            // move onto the next property
            continue;
        }

        // if this property range is a row, and the height property exists, 
        // store it in the cache as the height for this row range
        if (it->contains(row1, -1))
        {
            // if the column height exists, store it      
            has_property = it->getProperty(PROP_SIZE_H, value);
            if (has_property)
            {
                int i;
                int size = value.getInteger();
                for (i = row1; i <= row2; ++i)
                {
                    m_cache_row_sizes[i] = size;
                }
            }
        
            // move onto the next property
            continue;
        }
    }
}

void CompTableModel::clearCache()
{
    // clear the row size and column size caches
    m_cache_row_sizes.clear();
    m_cache_col_sizes.clear();
}


CompTable::CompTable()
{
    // initialize the model
    m_model = CompTableModel::create();

    // create list of cell properties that are changeable by the 
    // setProperties() function or by the cell editor
    m_cell_properties_editable.add(PROP_COLOR_BG);
    m_cell_properties_editable.add(PROP_COLOR_FG);
    m_cell_properties_editable.add(PROP_LINE_WIDTH);
    m_cell_properties_editable.add(PROP_BORDER_TOP_STYLE);
    m_cell_properties_editable.add(PROP_BORDER_BOTTOM_STYLE);
    m_cell_properties_editable.add(PROP_BORDER_LEFT_STYLE);
    m_cell_properties_editable.add(PROP_BORDER_RIGHT_STYLE);
    m_cell_properties_editable.add(PROP_FONT_FACENAME);
    m_cell_properties_editable.add(PROP_FONT_SIZE);
    m_cell_properties_editable.add(PROP_FONT_STYLE);
    m_cell_properties_editable.add(PROP_FONT_WEIGHT);
    m_cell_properties_editable.add(PROP_FONT_UNDERSCORE);
    m_cell_properties_editable.add(PROP_TEXT_COLOR);
    m_cell_properties_editable.add(PROP_TEXT_HALIGN);
    m_cell_properties_editable.add(PROP_TEXT_VALIGN);
    m_cell_properties_editable.add(PROP_TEXT_SPACING);
    m_cell_properties_editable.add(PROP_TEXT_WRAP);
    m_cell_properties_editable.add(PROP_CONTENT_MIMETYPE, wxString(wxT("plain/text")));
    m_cell_properties_editable.add(PROP_CONTENT_ENCODING, wxString(wxT("")));
    m_cell_properties_editable.add(PROP_CONTENT_VALUE);
    m_cell_properties_editable.add(PROP_LOCKED);
 
     // properties
    initProperties(m_properties);
 
    // add event handlers
    addEventHandler(EVENT_KEY, &CompTable::onKey);
    addEventHandler(EVENT_MOUSE_LEFT_DCLICK, &CompTable::onMouse);
    addEventHandler(EVENT_MOUSE_LEFT_DOWN, &CompTable::onMouse);
    addEventHandler(EVENT_MOUSE_LEFT_UP, &CompTable::onMouse);
    addEventHandler(EVENT_MOUSE_RIGHT_DCLICK, &CompTable::onMouse);
    addEventHandler(EVENT_MOUSE_RIGHT_DOWN, &CompTable::onMouse);
    addEventHandler(EVENT_MOUSE_RIGHT_UP, &CompTable::onMouse);
    addEventHandler(EVENT_MOUSE_MOTION, &CompTable::onMouse);
    addEventHandler(EVENT_MOUSE_OVER, &CompTable::onMouse);
    addEventHandler(EVENT_FOCUS_SET, &CompTable::onFocus);
    addEventHandler(EVENT_FOCUS_KILL, &CompTable::onFocus);

    // create default renderer and editors
    addRenderer(COMP_TYPE_TEXTBOX, CompTextBox::create());
    addRenderer(COMP_TYPE_IMAGE, CompImage::create());
    addEditor(COMP_TYPE_TEXTBOX, CompTextBox::create());
    
    // cursor position
    m_cursor_row = 0;
    m_cursor_col = 0;

    // default preferences
    m_preferences = 0;
    setPreference(prefCursor, true);
    setPreference(prefHorzGridLines, true);
    setPreference(prefVertGridLines, true);
    setPreference(prefResizeColumns, true);
    setPreference(prefResizeRows, true);
    setPreference(prefSelectColumns, true);
    setPreference(prefSelectRows, true);
    setPreference(prefMoveCells, true);
    setPreference(prefPopulateCells, true);
    setPreference(prefEditCells, true);

    // line thickness
    m_selection_line_width = 3;  // pixels

    // mouse variable initialization
    m_mouse_action = ACTION_NONE;   // name of current mouse action
    m_mouse_ctrl_start = false;     // control key state at start of action
    m_mouse_shift_start = false;    // shift key state at start of action
    m_mouse_alt_start = false;      // alt key state at start of action    
    m_mouse_x = 0;                  // current x position of the mouse
    m_mouse_y = 0;                  // current y position of the mouse
    m_mouse_x_last = 0;             // last x position of the mouse
    m_mouse_y_last = 0;             // last y position of the mouse
    m_mouse_x_start = 0;            // start x position of the mouse
    m_mouse_y_start = 0;            // start y position of the mouse
    
    m_mouse_row = -1;               // current row for mouse action; not always used
    m_mouse_col = -1;               // current column for mouse action; not always used
    m_mouse_row_start = -1;         // starting row for mouse action; not always used
    m_mouse_col_start = -1;         // starting column for mouse action; not always used
    m_mouse_row_size = 0;           // starting row size for mouse action; not always used
    m_mouse_col_size = 0;           // starting column size for mouse action; not always used    

    // cursor selection index; used to keep track of which
    // selection the cursor is in so it can flow through
    // the selection areas
    m_cursor_selection_idx = -1;

    // flag used to store if table just received the focus
    m_focus_set = false;

    // flags used to track editing states
    m_editing = false;
    m_cell_editing = false;
}

CompTable::~CompTable()
{
}

IComponentPtr CompTable::create()
{
    return static_cast<IComponent*>(new CompTable);
}

void CompTable::initProperties(Properties& properties)
{
    Component::initProperties(properties);
    properties.add(PROP_COMP_TYPE, COMP_TYPE_TABLE);
}

IComponentPtr CompTable::clone()
{
    CompTable* c = new CompTable;
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void CompTable::copy(IComponentPtr component)
{
    // copy the component properties
    Component::copy(component);

    // if the input component isn't a table component, this 
    // is all we can do; we're done
    ICompTablePtr table = component;
    if (table.isNull())
        return;

    CompTable* t = static_cast<CompTable*>(table.p);

    // copy the table model
    m_model->copy(t->m_model);

    // copy the cell selections
    m_cell_selections = t->m_cell_selections;

    // copy the editable cell properties
    m_cell_properties_editable = t->m_cell_properties_editable;

    // copy the renderers and editors;

    // TODO: copying the renderers/editors this way is causing
    // a crash when cell editing in a cloned table due to a null
    // m_editor; should figure this out since the renderers/
    // editors are important parts of the way a table looks/works

    // m_renderer_list = t->m_renderer_list;
    // m_editor_list = t->m_editor_list;

    // copy the cursor position and size
    m_cursor_row = t->m_cursor_row;
    m_cursor_col = t->m_cursor_col;

    // copy the preferences
    m_preferences = t->m_preferences;

    // copy the cursor selection index
    m_cursor_selection_idx = t->m_cursor_selection_idx;
}

bool CompTable::setProperty(const wxString& prop_name, const PropertyValue& value)
{
    // note: this function sets a single property; we use the
    // setProperties() function to consolidate the logic for
    // setting the property on the component or on the selected
    // cells

    // create a property container from the property and set
    // the properties
    Properties properties;
    properties.add(prop_name, value);
    return setProperties(properties);
}

bool CompTable::setProperties(const Properties& properties)
{
    // note: this functions sets properties for both the component
    // as well as the selected cells; first, the properties are
    // sent to the component so the component properties can be
    // set; then, they are sent to the selected cells, the
    // editor, and finally the cursor

    // copy the input properties
    bool result = false;
    Properties props = properties;

    // set the component properties
    if (Component::setProperties(props))
        result = true;

    // remove the non-editable properties; if there aren't any properties 
    // left over, we're done
    filterEditableProperties(props);
    if (props.count() == 0)
        return result;

    // for each selection, set the property on the range
    // of cells of the selection
    std::vector<CellProperties>::iterator it, it_end;
    it_end = m_cell_selections.end();
    
    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        if (setCellProperties(it->m_range, props))
            result = true;
    }

    // if we're editing, pass the properties onto the editor, otherwise
    // pass them onto the cursor cell
    if (isEditing())
    {
        if (m_editor->setProperties(props))
            result = true;
    }
    else
    {
        // set the property on the cursor cell
        if (setCellProperties(CellRange(m_cursor_row, m_cursor_col), props))
            result = true;
    }

    // return the result
    return result;
}

bool CompTable::getProperty(const wxString& prop_name, PropertyValue& value) const
{
    // note: this function gets a single property; we use the
    // getProperties() function to consolidate the logic for
    // getting the property from the component or from the 
    // selected cells

    // create a property container and add the property to the container
    Properties properties;
    properties.add(prop_name, value);
    
    // get the properties; if we can't get the property in question,
    // return false
    if (!getProperties(properties))
        return false;

    // get the value and return true
    properties.get(prop_name, value);
    return true;
}

bool CompTable::getProperties(Properties& properties) const
{
    // note: this functions gets requested properties from the input 
    // for both the component as well as the selected cells; first, 
    // we get the properties from the component, then, we get on the 
    // properties from the selected cells, the editor, as well as the
    // cursor

    // result flag; set it to false
    bool result = false;

    // get the component properties
    Properties component_properties = properties;
    if (Component::getProperties(component_properties))
        result = true;

    // get the editable cell properties;
    Properties cell_properties = properties;
    
    // first, filter the editable properties from the cell properties;
    // if there aren't any properties left over, set the properties
    // to those returned by the component, and we're done
    filterEditableProperties(cell_properties);
    if (cell_properties.count() == 0)
    {
        properties = component_properties;
        return result;
    }

    // next, get the cell properties from the selection
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_selections.end();
    
    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        if (getCellProperties(it->m_range, cell_properties))
            result = true;
    }

    // finally, get the cell properties from the editor or the cursor,
    // depending on if we're editing or not
    if (isEditing())
    {
        if (m_editor->getProperties(cell_properties))
            result = true;
    }
    else
    {
        // set the property on the cursor cell
        CellRange cursor(m_cursor_row, m_cursor_col);
        if (getCellProperties(cursor, cell_properties))
            result = true;
    }

    // clear the input properties, add the component properties
    // followed by the cell properties, and return the result
    properties.clear();
    properties.add(component_properties).add(cell_properties);
    return result;
}

void CompTable::extends(wxRect& rect)
{
    // get the row and column count
    int rows, cols;
    rows = getRowCount();
    cols = getColumnCount();

    // return the extent of the table content, which corresponds
    // to the end of the last column and the end of the last row
    rect.x = 0; rect.y = 0;
    rect.width = getColumnPosByIdx(cols);
    rect.height = getRowPosByIdx(rows);
}

void CompTable::render(const wxRect& rect)
{
    // if the row or column count is less than or
    // equal to zero, we're done
    if (getRowCount() <= 0 || getColumnCount() <= 0)
        return;

    // create an update rectangle from the table dimensions;
    // if we have a render rectangle, intersect it with the
    // update rectangle to find the area we need to update
    wxRect update_rect(0, 0, getWidth(), getHeight());
    if (!rect.IsEmpty())
        update_rect.Intersect(rect);

    // render the parts of the table
    renderContent(update_rect);
    //renderBorders(update_rect);
    renderCursor(update_rect);
    renderSelectionRect(update_rect);
    renderHighlight(update_rect);
    renderEditor(update_rect);
}

void CompTable::setModel(ITableModelPtr model)
{
    m_model = model;
}

ITableModelPtr CompTable::getModel()
{
    return m_model;
}

bool CompTable::setRowCount(int count)
{
    return m_model->setRowCount(count);
}

int CompTable::getRowCount() const
{
    return m_model->getRowCount();
}

bool CompTable::setColumnCount(int count)
{
    return m_model->setColumnCount(count);
}

int CompTable::getColumnCount() const
{
    return m_model->getColumnCount();
}

bool CompTable::insertRow(int idx, int count)
{
    // remove all selections
    removeAllCellSelections();

    // insert the rows
    return m_model->insertRow(idx, count);
}

bool CompTable::removeRow(int idx, int count)
{
    // remove all selections
    removeAllCellSelections();

    // remove the row
    bool result = false;
    result = m_model->removeRow(idx, count);

    // if the cursor row position is greater than or
    // equal to the row count and the row count is
    // greater than zero, move the cursor row position
    // back by one
    if (result)
    {
        int row_count = m_model->getRowCount();
        if (m_cursor_row >= row_count && row_count > 0)
            --m_cursor_row;
    }

    // return the result of removing the rows
    return result;
}

bool CompTable::insertColumn(int idx, int count)
{
    // remove all selections
    removeAllCellSelections();

    // insert the columns
    return m_model->insertColumn(idx, count);
}

bool CompTable::removeColumn(int idx, int count)
{
    // remove all selections
    removeAllCellSelections();

    // remove the column
    bool result = false;
    result = m_model->removeColumn(idx, count);

    // if the cursor column position is greater than or
    // equal to the column count and the column count is
    // greater than zero, move the cursor column position
    // back by one
    if (result)
    {
        int col_count = m_model->getColumnCount();
        if (m_cursor_col >= col_count && col_count > 0)
            --m_cursor_col;
    }

    // return the result of removing the columns
    return result;
}

bool CompTable::setRowSize(int row, int size)
{
    // set the row size
    return m_model->setRowSize(row, size);
}

int CompTable::getRowSize(int row)
{
    // get the row size
    return m_model->getRowSize(row);
}

bool CompTable::setColumnSize(int col, int size)
{
    // set the column size
    return m_model->setColumnSize(col, size);
}

int CompTable::getColumnSize(int col)
{
    // get the column size
    return m_model->getColumnSize(col);
}

int CompTable::getColumnPosByIdx(int col)
{
    // get the column position by index
    return m_model->getColumnPosByIdx(col);
}

int CompTable::getColumnIdxByPos(int x)
{
    // get the column index by position
    return m_model->getColumnIdxByPos(x);
}

int CompTable::getRowPosByIdx(int row)
{
    // get the row position by index
    return m_model->getRowPosByIdx(row);
}

int CompTable::getRowIdxByPos(int y)
{
    // get the row index by position
    return m_model->getRowIdxByPos(y);
}

void CompTable::getCellIdxByPos(int x, int y, int* row, int* col, bool merged)
{
    // get the cell index by position
    m_model->getCellIdxByPos(x, y, row, col, merged);
}

void CompTable::getCellPosByIdx(int row, int col, int* x, int* y, bool merged)
{
    // get the cell position by index
    m_model->getCellPosByIdx(row, col, x, y, merged);
}

void CompTable::getCellSizeByIdx(int row, int col, int* w, int* h, bool merged)
{
    // get the cell size by index
    m_model->getCellSizeByIdx(row, col, w, h, merged);
}

void CompTable::getCellCenterByIdx(int row, int col, int* x, int* y, bool merged)
{
    // get the cell center by index
    m_model->getCellCenterByIdx(row, col, x, y, merged);
}

void CompTable::addCellProperty(const wxString& prop_name,
                                const PropertyValue& value)
{
    // add a cell property
    m_model->addCellProperty(prop_name, value);
}

void CompTable::addCellProperties(const std::vector<CellProperties>& properties,
                                  bool replace)
{
    // add the cell properties
    m_model->addCellProperties(properties, replace);
}

bool CompTable::removeCellProperty(const wxString& prop_name)
{
    // remove the cell property
    return m_model->removeCellProperty(prop_name);
}

bool CompTable::setCellProperty(const CellRange& range, 
                                const wxString& prop_name, 
                                const PropertyValue& value)
{
    // set the cell property
    return m_model->setCellProperty(range, prop_name, value);
}

bool CompTable::setCellProperties(const CellRange& range, 
                                  const Properties& properties)
{
    // set the cell properties
    return m_model->setCellProperties(range, properties);
}

bool CompTable::getCellProperty(const CellRange& range, 
                                const wxString& prop_name, 
                                PropertyValue& value) const
{
    // get the cell property
    return m_model->getCellProperty(range, prop_name, value);
}

bool CompTable::getCellProperty(const CellRange& range,
                                const wxString& prop_name,
                                std::vector<CellProperties>& properties) const
{
    // get the cell property
    return m_model->getCellProperty(range, prop_name, properties);
}

bool CompTable::getCellProperties(const CellRange& range, 
                                  Properties& properties) const
{
    // get the cell properties
    return m_model->getCellProperties(range, properties);
}

void CompTable::getCellProperties(const CellRange& range,
                                 std::vector<CellProperties>& properties) const
{
    // get the cell properties
    m_model->getCellProperties(range, properties);
}

void CompTable::evalCell(int row, int col, Properties& properties) const
{
    // TODO: we added evalCell() to the interface since we're 
    // accounting for border properties in this function, but not
    // yet in the other getCellProperty() type functions; as a result
    // this is the only way to easily evaluate a cell to see how to
    // draw it, and it's needed for report layout engine in appmain;
    // should handle cell borders in the other cell property functions
    // and then remove this function

    m_model->eval(row, col, properties);
}

void CompTable::mergeCells(const CellRange& range)
{
    // merge the cells
    m_model->mergeCells(range);

    // it's possible that after merging cells, the cursor is on a merged
    // cell that has an underlying selection; when this happens, the
    // findNextCellSelection() function attempts to reposition the cursor
    // within the underlying selection, making it impossible to move the
    // cursor with the keys without first moving the cursor with the mouse;
    // to solve this, remove any selections contained within the cursor
    // cell if it's also a merged cell
    CellRange cursor(m_cursor_row, m_cursor_col);
    if (m_model->isMerged(cursor))
        removeCellSelections(cursor);
}

void CompTable::unmergeCells(const CellRange& range)
{
    // unmerge the cells
    m_model->unmergeCells(range);
}

void CompTable::unmergeAllCells()
{
    // unmerge all cells
    m_model->unmergeAllCells();
}

void CompTable::getMergedCells(std::vector<CellRange>& merges) const
{
    // get the merged cells
    m_model->getMergedCells(merges);
}

bool CompTable::isMerged(CellRange& range) const
{
    // find out whether or not the cell range is merged
    return m_model->isMerged(range);
}

bool CompTable::tag(const wxString& tag)
{
    // tag the cell properties so they can be restored
    return m_model->tag(tag);
}

bool CompTable::restore(const wxString& tag)
{
    // restore the cell properties at a particular point
    return m_model->restore(tag);
}

void CompTable::compact()
{
    m_model->compact();
}

void CompTable::reset()
{
    m_model->reset();
}

void CompTable::selectCells(const CellRange& range)
{
    // note: this function selects the range of cells specified
    // in the input

    // set the property range
    CellProperties c;
    c.m_range = range;

    // if we are selecting all the cells, remove previous
    // selections, since this covers all the cells; add
    // it to the list of selections, and we're done
    if (c.m_range.contains(-1, -1))
    {
        removeAllCellSelections();
        m_cell_selections.push_back(c);
        return;
    }

    // if the range is not a row or column range, find the bounding 
    // cell range to account for merged cells
    int row1, col1, row2, col2;
    range.get(&row1, &col1, &row2, &col2);
    if (row1 != -1 && col1 != -1 && row2 != -1 && col2 != -1)
    {
        std::vector<CellRange> merged_cells;
        m_model->getMergedCells(merged_cells);
        getBoundingRange(merged_cells, range, c.m_range);
    }

    // otherwise, simply add the selection range to the collection 
    // of selections without removing previous selections
    m_cell_selections.push_back(c);
}

void CompTable::removeCellSelections(const CellRange& range)
{
    // note: this function removes all selections contained
    // in the input range

    // create a copy of the selections and clear them
    std::vector<CellProperties> copy_cell_selections = m_cell_selections;
    m_cell_selections.clear();
    
    // iterate through the selections and save those selections that
    // aren't contained in the input range
    std::vector<CellProperties>::iterator it, it_end;
    it_end = copy_cell_selections.end();
    
    for (it = copy_cell_selections.begin(); it != it_end; ++it)
    {
        if (!range.contains(it->m_range))
            m_cell_selections.push_back(*it);
    }
}

void CompTable::removeAllCellSelections()
{
    // note: this function removes all cell selections
    
    m_cell_selections.clear();
}

void CompTable::getCellSelections(std::vector<CellRange>& selections, bool cursor)
{
    // note: this function returns a list of the current selections;
    // if cursor is true, then the cursor cell is returned in the list
    // of selections; if cursor is false, then the cursor cell is not
    // returned in the list of selections

    // make sure the selections vector is clear
    selections.clear();
    selections.reserve(m_cell_selections.size()+1);

    // if the cursor is visible and the cursor flag is true, 
    // add the selection for the cursor, taking into account 
    // that it may be a merged cell
    if (isCursorVisible() && cursor)
    {
        CellRange cursor(m_cursor_row, m_cursor_col);
        isMerged(cursor);
        selections.push_back(cursor);
    }

    // add the selections
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_selections.end();

    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        int r1, c1, r2, c2;
        it->getRange(&r1, &c1, &r2, &c2);

        CellRange cell_selection(r1, c1, r2, c2);
        selections.push_back(cell_selection);
    }
}

bool CompTable::isCellSelected(int row, int col, bool cursor)
{
    // note: the cursor flag determines whether the cursor should
    // be included as a selected cell in the selection test
    
    // if the cursor flag is set and the cursor is visible, and
    // it's a cursor cell, return true
    if (cursor && isCursorVisible() && isCursorCell(row, col))
        return true;

    // see if the cell in question is merged, and if so, get the range
    CellRange cell(row, col);
    isMerged(cell);

    // iterate through the selections
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_selections.end();

    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        // if the selection contains the cell, return true
        if (it->contains(cell))
            return true;
    }
    
    // otherwise return false
    return false;
}

bool CompTable::isColumnSelected(int col) const
{
    // if the selection contains the column, return true
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_selections.end();

    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        if (it->contains(-1, col))
            return true;
    }
    
    // otherwise return false
    return false;
}

bool CompTable::isRowSelected(int row) const
{
    // if the selection contains the row, return true
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_selections.end();

    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        if (it->contains(row, -1))
            return true;
    }
    
    // otherwise return false
    return false;
}

bool CompTable::isTableSelected() const
{
    // note: technically, if all cells are selected there should
    // only be one selection, since we remove other selections
    // when we select all the cells; however, for uniformity, we'll 
    // follow the logic we use to check for other selection ranges

    // if the selection contains the all the cells, return true
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_selections.end();

    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        if (it->contains(-1, -1))
            return true;
    }
    
    // otherwise return false
    return false;
}

bool CompTable::hasColumnSelections() const
{
    // if the table has column selections, return true
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_selections.end();

    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        int row1, col1, row2, col2;
        it->getRange(&row1, &col1, &row2, &col2);
        
        if (row1 == -1 && row2 == -1)
            return true;
    }
    
    // otherwise, return false
    return false;
}

bool CompTable::hasRowSelections() const
{
    // if the table has row selections, return true
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_selections.end();

    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        int row1, col1, row2, col2;
        it->getRange(&row1, &col1, &row2, &col2);
        
        if (col1 == -1 && col2 == -1)
            return true;
    }
    
    // otherwise, return false
    return false;
}

bool CompTable::hasSelections() const
{
    if (m_cell_selections.empty())
        return false;

    return true;
}

void CompTable::moveCursor(int row_diff, int col_diff, bool clear_selection)
{
    int row, col;
    getCursorPos(&row, &col);

    if (clear_selection)
    {
        invalidateSelection();
        removeAllCellSelections();
    }

    // adjust the cursor offset so the cursor placement will be in 
    // the selection area, then set the cursor position
    findNextCellSelection(row, col, &row_diff, &col_diff);
    setCursorPos(row + row_diff, col + col_diff);
}

void CompTable::setCursorPos(int row, int col)
{
    // see if the new position is the same as the old;
    // if so, we're done
    if (m_cursor_row == row && m_cursor_col == col)
        return;

    // fire the pre cursor move event to indicate that the cursor
    // is trying to move; note: we don't limit the cursor position 
    // to the table until after we fire the initial event so that 
    // cursor event handlers can detect that the cursor is trying 
    // to move off the edge off the table and as a result, take 
    // some appropriate action, such as switching the focus to
    // another table or scrolling to the other end of the grid;
    // if the event is vetoed, we're done
    INotifyEventPtr notify_evt_pre;
    notify_evt_pre = NotifyEvent::create(EVENT_TABLE_PRE_CURSOR_MOVE, this);
    notify_evt_pre->addProperty(EVENT_PROP_ROW, row);
    notify_evt_pre->addProperty(EVENT_PROP_COLUMN, col);
    dispatchEvent(notify_evt_pre);
    
    if (!notify_evt_pre->isAllowed())
        return;

    // we don't want the cursor to go off the edge of the table, 
    // so "clamp" the position; however, clamp the position *after* 
    // the initial "same-cell" check and pre cursor move event so
    // that event handlers can see that the cursor is trying to
    // move off the edge and take appropriate action; see above
    int row_count = getRowCount();
    int col_count = getColumnCount();

    if (row >= row_count - 1)
        row = row_count - 1;

    if (col > col_count - 1)
        col = col_count - 1;

    if (row < 0)
        row = 0;

    if (col < 0)
        col = 0;

    // set the cursor position
    m_cursor_row = row;
    m_cursor_col = col;
    
    // fire a cursor move event
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_TABLE_CURSOR_MOVE, this);
    notify_evt->addProperty(EVENT_PROP_ROW, m_cursor_row);
    notify_evt->addProperty(EVENT_PROP_COLUMN, m_cursor_col);
    dispatchEvent(notify_evt);
}

void CompTable::getCursorPos(int* row, int* col) const
{
    *row = m_cursor_row;
    *col = m_cursor_col;
}

bool CompTable::isCursorCell(int row, int col) const
{
    // note: this function returns true if the specified cell
    // is a cursor cell and false otherwise
    
    // if we have a row or column range, we're don't have a
    // cell, so it can't be the cursor cell; return false
    if (row == -1 || col == -1)
        return false;
    
    // get the cell range and adjust it for merged cells
    CellRange cell(row, col);
    isMerged(cell);
    
    // if the cell contains the cursor row or column, it's a
    // cursor cell
    return cell.contains(m_cursor_row, m_cursor_col);
}


void CompTable::clearCells(const CellRange& range, 
                           const Properties& properties)
{
    // note: this function restores the specified properties
    // to their default value on the specified range

    // get the default values for the input properties and limit
    // the properties being cleared to those that are editable
    Properties default_props;
    m_model->getDefaultCellProperties(default_props);
    filterEditableProperties(default_props);

    Properties props_to_clear = properties;
    props_to_clear.set(default_props);

    // set the default values on the input range
    setCellProperties(range, props_to_clear);
}

void CompTable::copyCells(const CellRange& range1,
                          const CellRange& range2,
                          ICompTablePtr target_table)
{
    // note: this function takes the cells in range1 and copies
    // them to the cells in range2
    
    CompTable* table;
    if (target_table.isNull())
        table = NULL;
    else
        table = static_cast<CompTable*>(target_table.p);

    copyCellProperties(range1, range2, false, false, false, table);
}

void CompTable::moveCells(const CellRange& range1,
                          const CellRange& range2,
                          ICompTablePtr target_table)
{
    // note: this function takes the cells in range1 and moves them
    // to the cells in range2

    CompTable* table;
    if (target_table.isNull())
        table = NULL;
    else
        table = static_cast<CompTable*>(target_table.p);

    copyCellProperties(range1, range2, true, false, false, table);
}

bool CompTable::resizeRowsToContent(const CellRange& range)
{
    // note: this function resizes each row that intersects
    // the given range to the vertical size of the content
    // within that row in the range; returns true if any 
    // rows are resized and false otherwise

    // used throughout
    bool result = false;
    std::map<int,int>::iterator it, it_end;

    // get the content sizes for the range
    std::map<int,int> sizes;
    getRowContentSize(range, sizes);

    // iterate through the row sizes that have content
    it_end = sizes.end();
    for (it = sizes.begin(); it != it_end; ++it)
    {
        // if the new row size is different, 
        // resize the row, and set the result
        // flag to true
        if (it->second != getRowSize(it->first))
        {
            setRowSize(it->first, it->second);
            result = true;
        }
    }

    // iterate through all rows; if we've already
    // set the row based on content, move on; otherwise
    // set the default row size if the row size is
    // different from the default
    int row1 = range.row1();
    int row2 = range.row2();

    if (row1 == -1)
        row1 = 0;

    if (row2 == -1)
        row2 = getRowCount() - 1;

    int row;
    for (row = row1; row <= row2; ++row)
    {
        if (sizes.find(row) != it_end)
            continue;

        if (TABLE_CELL_HEIGHT != getRowSize(row))
        {
            setRowSize(row, TABLE_CELL_HEIGHT);
            result = true;
        }
    }

    // return true if any rows were resized
    // and false otherwise
    return result;
}

bool CompTable::resizeColumnsToContent(const CellRange& range)
{
    // note: this function resizes each column that intersects
    // the given range to the horizontal size of the content
    // within that column in the range; returns true if any 
    // columns are resized and false otherwise

    // used throughout
    bool result = false;
    std::map<int,int> sizes;

    // get the content sizes for the range
    getColumnContentSize(range, sizes);

    // if we don't have any columns, return false
    if (sizes.empty())
        return result;

    // iterate through the column sizes; unlike rows,
    // if we don't have a column size, don't do anything
    std::map<int,int>::iterator it, it_end;
    it_end = sizes.end();

    for (it = sizes.begin(); it != it_end; ++it)
    {
        // if the new column size is different, 
        // resize the column, and set the result
        // flag to true
        if (it->second != getColumnSize(it->first))
        {
            setColumnSize(it->first, it->second);
            result = true;
        }
    }

    // return true if any columns were resized
    // and false otherwise
    return result;
}

void CompTable::getRowContentSize(const CellRange& range, std::map<int,int>& sizes)
{
    // note: this function returns the vertical size of each 
    // row in the given range that contains content; if the 
    // row within the range doesn't contain content, then it 
    // isn't included in the output; the results are returned 
    // as a map, where the first part of the map is the row 
    // index, and the second part of the map is the content
    // size

    // if we don't have a canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // get the text properties across the range
    std::vector<CellProperties> text_properties;
    getCellProperty(range, PROP_CONTENT_VALUE, text_properties);
    
    // if we don't have any text properties in the range,
    // we're done
    if (text_properties.empty())
        return;

    // clamp the row at the range of valid cells    
    int row1 = range.row1();
    int row2 = range.row2();
    
    if (row1 == -1)
        row1 = 0;
        
    if (row2 == -1)
        row2 = getRowCount() - 1;

    int row;
    for (row = row1; row <= row2; ++row)
    {
        // for each row, get the columns with the text 
        // values in them
        std::vector<int> columns;
        columns.reserve(100);

        CellRange row_range(row, range.col1(), row, range.col2());
        getTextColumns(row_range, text_properties, text_properties.rbegin(), columns);

        // if we don't have any text values in the column, move on
        if (columns.empty())
            continue;

        // if there's text, shrink up to the default row width, 
        // but no farther; note: this is different than for 
        // columns where we use the minimum width
        int result_size = TABLE_CELL_HEIGHT;

        // iterate through the cells with text properties        
        std::vector<int>::iterator it, it_end;
        it_end = columns.end();
        
        for (it = columns.begin(); it != it_end; ++it)
        {
            // load the cell properties into a the text renderer 
            // and get the extent of the renderer
            wxRect extent;
            putCellPropertiesInRenderer(row, *it);
            m_renderer->extends(extent);

            // note: no need to add padding on the row
            // since the row carries a minimum size

            // if the extent is greater than the current
            // resulting size, save it
            if (extent.height > result_size)
                result_size = extent.height;
        }

        // save the resulting size for the row
        sizes[row] = result_size;
    }
}

void CompTable::getColumnContentSize(const CellRange& range, std::map<int,int>& sizes)
{
    // note: this function returns the horizontal size of each 
    // column in the given range that contains content; if the 
    // column within the range doesn't contain content, then it 
    // isn't included in the output; the results are returned 
    // as a map, where the first part of the map is the column 
    // index, and the second part of the map is the content
    // size

    // if we don't have a canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // get the text properties across the range
    std::vector<CellProperties> text_properties;
    getCellProperty(range, PROP_CONTENT_VALUE, text_properties);
    
    // if we don't have any text properties in the range,
    // we're done
    if (text_properties.empty())
        return;

    // clamp the column at the range of valid cells    
    int col1 = range.col1();
    int col2 = range.col2();
    
    if (col1 == -1)
        col1 = 0;
        
    if (col2 == -1)
        col2 = getColumnCount() - 1;

    int col;
    for (col = col1; col <= col2; ++col)
    {
        // for each row, get the rows with the text 
        // values in them
        std::vector<int> rows;
        rows.reserve(100);

        CellRange column_range(range.row1(), col, range.row2(), col);
        getTextRows(column_range, text_properties, text_properties.rbegin(), rows);

        // if we don't have any text values in the rows, move on
        if (rows.empty())
            continue;

        // if there's text, shrink up to the minimum column width
        int result_size = TABLE_MIN_CELL_WIDTH;

        // iterate through the cells with text properties
        std::vector<int>::iterator it, it_end;
        it_end = rows.end();
        
        for (it = rows.begin(); it != it_end; ++it)
        {
            // load the cell properties into a the text renderer 
            // and get the extent of the renderer
            wxRect extent;
            putCellPropertiesInRenderer(*it, col);
            m_renderer->extends(extent);

            // add a little padding onto the extent
            int hpad = TEXTBOX_HORZ_PIXEL_BUFF;
            extent.width += 2*canvas->dtom_x(hpad);

            // if the extent is greater than the current
            // current resulting size, save it
            if (extent.width > result_size)
                result_size = extent.width;
        }

        // save the resulting size for the column
        sizes[col] = result_size;
    }
}

void CompTable::addRenderer(const wxString& name, IComponentPtr comp)
{
    // if the component is null, we're done
    if (comp.isNull())
        return;

    // add the row and column properties
    comp->addProperty(PROP_CELL_ROW, -1);
    comp->addProperty(PROP_CELL_COLUMN, -1);
 
    // add the component to the list of renderers
    m_renderer_list[name] = comp;
}

void CompTable::getRenderers(std::vector<IComponentPtr>& renderers)
{
    // make sure the renderer list is empty
    renderers.clear();
    
    // add the renderers to the list
    std::map<wxString, IComponentPtr>::iterator it, it_end;
    it_end = m_renderer_list.end();
    
    for (it = m_renderer_list.begin(); it != it_end; ++it)
    {
        renderers.push_back(it->second);
    }
}

void CompTable::addEditor(const wxString& name, IComponentPtr comp)
{
    // if the component is null, we're done
    if (comp.isNull())
        return;

    // if the component doesn't expose the editor
    // interface, we're done
    IEditPtr editor = comp;
    if (editor.isNull())
        return;

    // add the row and column properties
    comp->addProperty(PROP_CELL_ROW, -1);
    comp->addProperty(PROP_CELL_COLUMN, -1);

    // add an event handler to proxy the editor events
    // back to the canvas; used to invalidate the region 
    // occupied by the cell editor in reponse to editor 
    // invalidate events
    IEventTargetPtr target = comp;
    EventHandler* handler = new EventHandler;
    handler->sigEvent().connect(this, &CompTable::onEditorInvalidated);
    target->addEventHandler(EVENT_INVALIDATE, handler);

    // add the editor to the list
    m_editor_list[name] = comp;
}

void CompTable::getEditors(std::vector<IComponentPtr>& editors)
{
    // make sure the editor list is empty
    editors.clear();
    
    // add the editors to the list
    std::map<wxString, IComponentPtr>::iterator it, it_end;
    it_end = m_editor_list.end();
    
    for (it = m_editor_list.begin(); it != it_end; ++it)
    {
        editors.push_back(it->second);
    }
}

void CompTable::setPreference(int pref, bool active)
{
    if (active)
        m_preferences |= pref;
         else
        m_preferences &= ~pref;
}

bool CompTable::isPreference(int pref)
{
    return (m_preferences & pref) ? true : false;
}

void CompTable::beginEdit()
{
    // if we don't have a canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // if we're already editing, we're done
    if (isEditing())
        return;

    // if cell editing isn't allowed, return false
    if (!isPreference(prefEditCells))
        return;

    // property value to extract out various properties
    PropertyValue value;

    // get the properties of the cell we're on
    Properties props;
    getCellProperties(CellRange(m_cursor_row, m_cursor_col), props);

    // if the cell is locked, we're done
    props.get(PROP_LOCKED, value);
    if (value.getBoolean())
        return;

    // fire a pre-cell edit notify event; if it's vetoed, we're done
    INotifyEventPtr notify_pre_evt;
    notify_pre_evt = NotifyEvent::create(EVENT_TABLE_PRE_CELL_EDIT, this);
    notify_pre_evt->addProperty(EVENT_PROP_ROW, m_cursor_row);
    notify_pre_evt->addProperty(EVENT_PROP_COLUMN, m_cursor_col);
    dispatchEvent(notify_pre_evt);
    
    if (!notify_pre_evt->isAllowed())
        return;

    // dispatch a begin edit event; this the general event used by
    // the canvas; TODO: see notes related to EVENT_EDIT_BEGIN/EVENT_EDIT_END
    // in kcanvas.h
    INotifyEventPtr notify_edit_evt;
    notify_edit_evt = NotifyEvent::create(EVENT_EDIT_BEGIN, this);
    notify_edit_evt->addProperty(EVENT_PROP_ROW, m_cursor_row);
    notify_edit_evt->addProperty(EVENT_PROP_COLUMN, m_cursor_col);
    notify_edit_evt->addProperty(EVENT_PROP_EDIT_ACCEPTED, false);
    dispatchEvent(notify_edit_evt);

    // make sure the foreground color is null so we don't 
    // get a grey edge around the editor
    props.add(PROP_COLOR_FG, Color(COLOR_NULL));
    props.add(PROP_CELL_ROW, m_cursor_row);
    props.add(PROP_CELL_COLUMN, m_cursor_col);

    // TODO: for now, manually associate a MIME type with
    // an editor; should allow this to be set in the API
    // when a editor is added, so that a user can set
    // which editor is to be used for a given MIME type
    
    wxString editor_id = COMP_TYPE_TEXTBOX;
    wxString mime_type = value.getString();
    
    if (mime_type == wxT("plain/text"))
        editor_id = COMP_TYPE_TEXTBOX;

    // note: no editors for other mime types; above is for
    // example pattern and also mirrors logic for renderers
    
    // TODO: could simply the following logic by storing the mime
    // type along with the renderer list; then we just iterate in
    // order through the list and find the first one that has
    // the apprpriate mime type handler
    
    // set the editor
    std::map<wxString,IComponentPtr>::iterator it_e;
    it_e = m_editor_list.find(editor_id);
    
    if (it_e != m_editor_list.end())
    {
        m_editor = it_e->second;
    }
    else
    {
        // TODO: it would be nice to solidify the default handling in the event
        // of a bad property; we specify defualts in the constructor, but this
        // default value assumes the default value in the constructor is the
        // same; if it isn't we may still not have a valid renderer, in which
        // case the program will crash
        m_editor = m_editor_list[COMP_TYPE_TEXTBOX];
    }  

    // update the editor canvas member
    Component* ptr_comp = static_cast<Component*>(m_editor.p);
    ptr_comp->setCanvas(canvas);

    // set the properties of the editor
    
    // note: see putCellPropertiesInRenderer() for benefits/drawbacks of 
    // using setProperties() vs. getProperties(), especially before changing 
    // this
    m_editor->addProperties(props);
    
    // set the editor position and size
    positionEditor(m_cursor_row, m_cursor_col);
    
    // set the draw origin for the beginning of the edit to the location of 
    // the component; TODO: this is because we're using the draw origin as an 
    // absolute position to instantiate the control for component controls;
    // other editor components rely on their position to find out where to be
    // drawn; all this seems like too much is being assumed about the
    // implementations; probably should find a way of notifying components of
    // their absolute location at key events, such as a focus event, or a
    // render event, like we're doing with the mouse event
    int draw_origin_x, draw_origin_y;
    canvas->getDrawOrigin(&draw_origin_x, &draw_origin_y);
    
    int origin_x, origin_y;
    m_editor->getOrigin(&origin_x, &origin_y);
    canvas->setDrawOrigin(draw_origin_x + origin_x, draw_origin_y + origin_y);
    
    // start the cell editing mode
    IEditPtr editor = m_editor;
    editor->beginEdit();
    
    // restore the old draw position; TODO: see above
    canvas->setDrawOrigin(draw_origin_x, draw_origin_y);
    
    // if we were successful in starting the edit, then set
    // the mouse action to the edit cell action as well as the
    // cell editing flag; note: we set the mouse action here
    // because editing can start both from a key event as well
    // as from a mouse event
    if (editor->isEditing())
    {    
        m_mouse_action = ACTION_EDITCELL;
        m_cell_editing = true;
    }
}

void CompTable::endEdit(bool accept)
{
    // if we don't have an editor, we're done
    if (m_editor.isNull())
        return;

    // if we're not editing, we're done
    if (!isEditing())
        return;

    // if we want the changes, end the editing mode, get
    // the editor properties and set the cell properties
    if (accept)
    {
        // get the properties and filter out the editable properties
        Properties properties;
        properties = m_editor->getPropertiesRef();
        filterEditableProperties(properties);
        
        // in addition to the normal editable properties, also filter
        // out the foreground color, since this is set to wxNullColor
        // when the editor is instantiated so we don't draw the grey
        // cell border on edit
        properties.remove(PROP_COLOR_FG);
        
        // set the cell properties
        setCellProperties(CellRange(m_cursor_row, m_cursor_col), properties);
    }

    // invalidate the rows containing the cell editor
    invalidateEditor(true);

    // end the cell editing mode
    IEditPtr editor = m_editor;
    editor->endEdit(accept);
    m_mouse_action = ACTION_NONE;
    m_cell_editing = false;

    // dispatch an end edit event
    INotifyEventPtr notify_edit_evt;
    notify_edit_evt = NotifyEvent::create(EVENT_EDIT_END, this);
    notify_edit_evt->addProperty(EVENT_PROP_ROW, m_cursor_row);
    notify_edit_evt->addProperty(EVENT_PROP_COLUMN, m_cursor_col);
    notify_edit_evt->addProperty(EVENT_PROP_EDIT_ACCEPTED, true);
    dispatchEvent(notify_edit_evt);
    
    // fire a cell edit notify event
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_TABLE_CELL_EDIT, this);
    notify_evt->addProperty(EVENT_PROP_ROW, m_cursor_row);
    notify_evt->addProperty(EVENT_PROP_COLUMN, m_cursor_col);
    dispatchEvent(notify_evt);
}

bool CompTable::isEditing() const
{
    // if we don't have an editor, we're done
    if (m_editor.isNull())
        return false;

    return m_cell_editing;
}

bool CompTable::canCut() const
{
    if (isEditing())
    {
        IEditPtr editor = m_editor;
        return editor->canCut();
    }

    return true;
}

bool CompTable::canCopy() const
{
    if (isEditing())
    {
        IEditPtr editor = m_editor;
        return editor->canCopy();
    }

    return true;
}

bool CompTable::canPaste() const
{
    if (isEditing())
    {
        IEditPtr editor = m_editor;
        return editor->canPaste();
    }

    return true;
}

void CompTable::cut()
{
    if (isEditing())
    {
        IEditPtr editor = m_editor;
        editor->cut();
        return;
    }

    // copy the cells to the clipboard
    copy();

    // clear the selection of everything; note: set
    // the flag to true to clear only text
    clear(false);
}

void CompTable::copy()
{
    if (isEditing())
    {
        IEditPtr editor = m_editor;
        editor->copy();
        return;
    }
    
    // create a component to package this component up
    ComponentDataObject* clipboard_object = new ComponentDataObject;
    IComponentPtr clipboard_component = Component::create();
    
    // create a copy of this component and add it to the clipboard component;
    // then, set the clipboard component
    clipboard_component->add(clone());
    clipboard_object->SetComponent(clipboard_component);

    // copy the the clipboard object to the clipboard
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(clipboard_object);
        wxTheClipboard->Close();
    }
}

void CompTable::paste()
{
    if (isEditing())
    {
        IEditPtr editor = m_editor;
        editor->paste();
        return;
    }

    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->IsSupported(wxDF_TEXT))
        {
        }
        
        // if it's our own format, handle it
        if (wxTheClipboard->IsSupported(wxDataFormat(component_data_format)))
        {
            // get the component data object
            ComponentDataObject object;
            wxTheClipboard->GetData(object);

            // unpack the table component from the clipboard component
            ICompTablePtr table;
            IComponentPtr comp = object.GetComponent();
            if (!comp.isNull())
            {
                // note: the component we received is a package that contains
                // the components we're interested in; iterate through the
                // child components and adjust their position relative to
                // the mouse
                std::vector<IComponentPtr> components;
                comp->getComponents(components);
                
                std::vector<IComponentPtr>::iterator it, it_end;
                it_end = components.end();
                
                for (it = components.begin(); it != it_end; ++it)
                {
                    table = *it;
                    if (!table.isNull())
                        break;
                }
            }
            
            if (!table.isNull())
            {
                // get the source table
                CompTable* source_table = static_cast<CompTable*>(table.p);
                
                // get the selection range from the source table
                CellRange source_selection_range;
                source_table->getSelectionRange(source_selection_range);
                
                if (!source_selection_range.isEmpty())
                {
                    // the offset between the source selection range position
                    // and the cursor position in the current table
                    int row1, col1, row2, col2;
                    source_selection_range.get(&row1, &col1, &row2, &col2);
                    
                    int cursor_row, cursor_col;
                    getCursorPos(&cursor_row, &cursor_col);
                    
                    int row_offset = cursor_row - row1;
                    int col_offset = cursor_col - col1;

                    // clear the selections on the current table
                    removeAllCellSelections();
                    
                    // copy the values from the clipboard table to this table and
                    // select the ranges that have been copied
                    CellRange source = source_selection_range;
                    CellRange target = source_selection_range.shift(row_offset, col_offset);
                    copyCellProperties(source, target, true, false, false, source_table);
                    selectCells(target);
                }
            }
        }
        
        wxTheClipboard->Close();
    }
    
    // after we're done pasting, copy the contents so we can paste again;
    // TODO: this is a kludge because we're getting crashes on multiple
    // paste operations; need to do this right
    copy();
}

void CompTable::selectAll()
{
    if (isEditing())
    {
        IEditPtr editor = m_editor;
        editor->selectAll();
        return;
    }

    selectCells(CellRange(-1,-1));
}

void CompTable::selectNone()
{
    if (isEditing())
    {
        IEditPtr editor = m_editor;
        editor->selectNone();
        return;
    }
    
    removeAllCellSelections();
}

void CompTable::clear(bool text)
{
    // note: this function clears the contents in the editor if editing
    // is happening, otherwise in the selection area; if text is true, 
    // then it only clear the text, otherwise, it clears all the editable 
    // values and restores the default properties
    
    if (isEditing())
    {
        // if we're editing, clear the text in the editor, invalidate
        // the editor cell and return;
        IEditPtr editor = m_editor;
        editor->clear(text);
        invalidateCellRange(CellRange(m_cursor_row, m_cursor_col));
        return;
    }

    // get the selection areas
    std::vector<CellRange> selections;
    getCellSelections(selections);
    
    // create a list of properties to clear; add on the text property;
    // if the text flag isn't set, also add on all the editable properties
    Properties props;
    props.add(PROP_CONTENT_VALUE);
    if (!text)
    {
        props.add(m_cell_properties_editable);
    }
    
    // reverse iterate through the selections and clear the
    // cells that are selected
    std::vector<CellRange>::reverse_iterator itr;
    for (itr = selections.rbegin(); itr != selections.rend(); ++itr)
    {
        clearCells(*itr, props);
    }
}

void CompTable::copyCellProperties(const CellRange& range1,
                                   const CellRange& range2,
                                   bool clear_range1,
                                   bool reverse_horz,
                                   bool reverse_vert,
                                   CompTable* table)
{
    // note: this function takes the cell properties relative to range1, 
    // clips them, then copies them into the cells of range2; it also copies 
    // the merged cells, but does not clip them; if range2 is a different
    // size than range2, then the properties are replicated or clipped so
    // that range2 is filled; if clear_range1 is true, then the properties 
    // and merged cells in range1 are cleared after they are copied and before 
    // they are placed in range2; if reverse_horz is true, then the properties
    // in range1 populate the range2 space from right-to-left, otherwise left-to
    // right; if reverse_vert is true, then the properties in range1 populate
    // the range2 space from bottom-to_top, otherwise from top-to_bottom;
    // finally, if table isn't NULL, then it is used as the input for range1 
    // instead the of current table

    // get the source table
    if (table == NULL)
        table = this;

    // get the properties in range1
    std::vector<CellProperties> properties;
    table->getCellProperties(range1, properties);
    
    // get merged cells in range1
    std::vector<CellRange> merges;
    table->getMergedCells(merges);

    // if the clear_range1 flag is set, unmerge any merged cells 
    // in range1 and clear the properties
    if (clear_range1)
    {
        table->unmergeCells(range1);
        table->clearCells(range1, m_cell_properties_editable);
    }

    // unmerge cells in range2
    unmergeCells(range2);

    // find the offset between the two ranges
    int range1_row1, range1_col1, range1_row2, range1_col2;
    range1.get(&range1_row1, &range1_col1, &range1_row2, &range1_col2);

    int range2_row1, range2_col1, range2_row2, range2_col2;
    range2.get(&range2_row1, &range2_col1, &range2_row2, &range2_col2);

    int row_offset = range2_row1 - range1_row1;
    int col_offset = range2_col1 - range1_col1;
    
    // if we're populating the cells from right-to-left or
    // from bottom-to-top, use the lower-right portions of the
    // ranges to find the offset
    if (reverse_vert)
        row_offset = range2_row2 - range1_row2;
    if (reverse_horz)
        col_offset = range2_col2 - range1_col2;

    // find the row and column skip offset, which we need to populate
    // range2 with the properties when range2 is larger than range1;
    // the row and column skip offsets are the size of range1 after
    // it's been set to the beginning of range2 and clipped to fit
    // inside of range2; with these skip values, we can find how many
    // times range1 fits inside range2 by shifting range one by the
    // row and column skip values until the shifted range1 no longer 
    // intersects range2
    CellRange r = range1;
    r.shift(row_offset, col_offset);
    r.intersect(range2);

    int rr1, rc1, rr2, rc2;
    r.get(&rr1, &rc1, &rr2, &rc2);

    // if range1 (shifted) is the same size as range2 or the row or 
    // column values for the range are range markers, set the skip 
    // offset to zero, which we'll check for; otherwise, the skip 
    // offset is the size of the range1 after being shifted and 
    // clipped to range2
    int row_skip_offset = 0;
    int col_skip_offset = 0;
    if (r != range2)
    {
        row_skip_offset = (rr2 >= 0 && rr1 >= 0) ? rr2 - rr1 + 1 : 0;
        col_skip_offset = (rc2 >= 0 && rc1 >= 0) ? rc2 - rc1 + 1: 0;
    }

    // if we're populating the cells from bottom-to-top or
    // from right-to-left, negate the skip offset
    if (reverse_vert)
        row_skip_offset = -1*row_skip_offset;
    if (reverse_horz)
        col_skip_offset = -1*col_skip_offset;

    // iterate through the cell properties, clip them to the range, 
    // and shift them to the new range
    std::vector<CellProperties>::iterator itprops, itprops_end;
    itprops_end = properties.end();
    
    for (itprops = properties.begin(); itprops != itprops_end; ++itprops)
    {
        // get the range
        CellRange cell_property_range;
        itprops->getRange(cell_property_range);
        
        // intersect the property range with range1 and then 
        // shift it to range2
        cell_property_range.intersect(range1);
        cell_property_range.shift(row_offset, col_offset);
        
        // filter the cell properties to the editable properties
        Properties cell_properties = itprops->properties();
        filterEditableProperties(cell_properties);
        
        // if the skip or column offset are not zero, replicate
        // the properties of range1 inside range2 so that range2
        // is filled with them
        int total_col_skip_offset = 0;
        while (cell_property_range.intersects(range2))
        {
            while (cell_property_range.intersects(range2))
            {
                CellRange cr = cell_property_range;
                cr.intersect(range2);
                
                setCellProperties(cr, cell_properties);
                cell_property_range.shift(0, col_skip_offset);
                total_col_skip_offset += col_skip_offset;
                
                // if the column skip offset is zero, we can only fit 
                // the range inside the column once, so move on
                if (col_skip_offset == 0)
                    break;
            }
            
            cell_property_range.shift(row_skip_offset, 0);
            cell_property_range.shift(0, -total_col_skip_offset);
            total_col_skip_offset = 0;
            
            // if the row skip offset is zero, we can only fit the 
            // range inside the row once, so move on
            if (row_skip_offset == 0)
                break;
        }
    }

    // iterate through the merged cell copies, shift them to the new
    // range and add them to the list of merged cells
    std::vector<CellRange>::iterator itmerge, itmerge_end;
    itmerge_end = merges.end();
    
    for (itmerge = merges.begin(); itmerge != itmerge_end; ++itmerge)
    {
        // get the merged cell range; if it doesn't intersect 
        // range1, move on
        CellRange merged_cell_range = *itmerge;
        if (!merged_cell_range.intersects(range1))
            continue;

        // if it intersects range1, shift it to range2; note: unlike the 
        // properties we do not clip the cell range to fit it in either 
        // range1 or range2 so that the merged cell retains its original 
        // size
        merged_cell_range.shift(row_offset, col_offset);
        
        int total_col_skip_offset = 0;
        while (merged_cell_range.intersects(range2))
        {
            while (merged_cell_range.intersects(range2))
            { 
                mergeCells(merged_cell_range);
                merged_cell_range.shift(0, col_skip_offset);
                total_col_skip_offset += col_skip_offset;
                
                // if the column skip offset is zero, we can only fit the range
                // inside the column once, so move on
                if (col_skip_offset == 0)
                    break;
            }
            
            merged_cell_range.shift(row_skip_offset, 0);
            merged_cell_range.shift(0, -total_col_skip_offset);
            total_col_skip_offset = 0;
            
            // if the row skip offset is zero, we can only fit the range 
            // inside the row once, so move on
            if (row_skip_offset == 0)
                break;
        }
    }
}

void CompTable::getCellRangeIdx(const wxRect& rect,
                                CellRange& range,
                                bool merged)
{
    // note: this function finds the range of cells that contain 
    // the input rectangle and any merged cells that go outside
    // of it, unless "merged" is false, in which case merged cells 
    // are ignored

    // create a rectangle from the input rectangle
    // that doesn't exceed the bounds of the component
    wxRect r = wxRect(0, 0, getWidth(), getHeight());
    r.Intersect(rect);

    // get an initial cell range from the coordinates of the rectangle,
    // without worrying about merged cells, which we'll account for
    // by getting the bounding range
    int row1, col1, row2, col2;
    getCellIdxByPos(r.x, r.y, &row1, &col1, false);
    getCellIdxByPos(r.x + r.width, r.y + r.height, &row2, &col2, false);
    
    // set the range to the calculated rows and columns; if "merged"
    // is true, expand the range to include the merged cells
    range.set(row1, col1, row2, col2);
    if (merged)
    {
        std::vector<CellRange> merged_cells;
        m_model->getMergedCells(merged_cells);

        CellRange initial_range = range;
        getBoundingRange(merged_cells, initial_range, range);
    }
    
    // limit range markers to the range of the table
    range.intersect(CellRange(0, 0, getRowCount() - 1, getColumnCount() - 1));
}

void CompTable::getCellRangePos(const CellRange& range,
                                wxRect& rect,
                                bool merged)
{
    // note: this function finds the rectangular region of the
    // input range, taking into account merged cells, unless "merged"
    // is false, in which case merged cells are ignored; if merged
    // cells are included, the returned rectangle will expand to include 
    // the entire dimensions of all the merged cells, even those whose 
    // indices are in the input range but whose dimensions go outside 
    // the rectangular area of the non-merged cells; for example, if 
    // the input range is from cell (1,1) to cell (3,3) and there's a 
    // merged cell from cell (2,2) to cell (5,2), then if the input 
    // range of the function is CellRange(1,1,3,3), the dimensions 
    // returned will cover the cells in CellRange(1,1,5,3) because of 
    // the merged cell
    
    // if "merged" is true, get the bounding range of the input range 
    // to account for merged cells
    CellRange bounding_range = range;
    if (merged)
    {
        std::vector<CellRange> merged_cells;
        m_model->getMergedCells(merged_cells);
        getBoundingRange(merged_cells, range, bounding_range);
    }
    
    // get the position of the bounding range
    int row1, col1, row2, col2;
    bounding_range.get(&row1, &col1, &row2, &col2);
    
    // limit range markers to the range of the table    
    if (row1 < 0) {row1 = 0;}
    if (col1 < 0) {col1 = 0;}
    if (row2 < 0) {row2 = getRowCount() - 1;}
    if (col2 < 0) {col2 = getColumnCount() - 1;}    

    // get the position of the range, without worrying about
    // merged cells since we've already accounted for these
    // with the bounding range
    int x1, y1, x2, y2, w, h;
    getCellPosByIdx(row1, col1, &x1, &y1, false);
    getCellPosByIdx(row2, col2, &x2, &y2, false);
    getCellSizeByIdx(row2, col2, &w, &h, false);
    
    // return the rectangle corresponding to the position
    rect.x = x1;
    rect.y = y1;
    rect.width = (x2 + w) - x1;
    rect.height = (y2 + h) - y1;
}

bool CompTable::isResizeRowEdge(int x, int y, int* row)
{
    // note: this function takes a (x, y) position and returns true 
    // if that position is on a row edge that can be used for
    // resizing; if the function returns true, the function sets 
    // the row pointer to the index of the row, otherwise it sets 
    // it to -1

    // if row resizing isn't allowed, return false
    if (!isPreference(prefResizeRows))
        return false;

    // only allow row edges on the first column (zero-based)
    // or less to be used for resizing rows; so get the
    // position of the second column, and if we're on it
    // or past it, we're done
    int x_max = getColumnPosByIdx(1);
    if (x >= x_max)
        return false;

    // get the row count
    int row_count = getRowCount();
    
    // if we don't have any rows, return false
    if (row_count <= 0)
        return false;

    // calculate mouse tolerance in model coordinates
    int tol = 0;
    ICanvasPtr canvas = getCanvas();
    if (!canvas.isNull())
        tol = canvas->dtom_y(TABLE_MOUSE_RESIZE_TOL);

    // don't count the beginning of the first row as a row edge
    if (y <= tol)
    {
        *row = -1;
        return false;
    }

    // get the edges of the current row we're on
    *row = getRowIdxByPos(y);
    int y1 = getRowPosByIdx(*row);
    int y2 = getRowSize(*row) + y1;

    // if the row size is sufficiently small, the mouse
    // tolerance complicates row selection; so if the
    // row size is small, set the maximum tolerance
    // size to 10% of the row size so row selection
    // always has a chance
    int calc_tol = (y2 - y1)*0.10;
    tol = wxMin(tol, calc_tol);

    // if we're past the end of the last row, but
    // near the second edge, return true
    if (y > y2 && y - y2 <= tol && (*row == row_count - 1))
    {
        return true;
    }

    // if we're near the second edge, return true
    if (y2 >= y && y2 - y <= tol)
    {
        return true;
    }
    
    // count the first edge as the end of previous row;
    // so if we're near the first edge, move back one
    // row and return true
    if (y >= y1 && y - y1 <= tol)
    {
        *row = *row - 1;
        return true;
    }

    // we're not near a row edge, so return false
    *row = -1;
    return false;
}

bool CompTable::isResizeColumnEdge(int x, int y, int* col)
{
    // note: this function takes an (x, y) position and returns true 
    // if that position is on a column edge that can be used for
    // resizing; if the function returns true, the function sets 
    // the column pointer to the index of the column, otherwise it 
    // sets it to -1

    // if column resizing isn't allowed, return false
    if (!isPreference(prefResizeColumns))
        return false;

    // only allow column edges on the first row (zero-based)
    // or less to be used for resizing columns; so get the
    // position of the second row, and if we're on it or
    // past it, we're done
    int y_max = getRowPosByIdx(1);
    if (y >= y_max)
        return false;

    // get the column count
    int col_count = getColumnCount();
    
    // if we don't have any columns, return false
    if (col_count <= 0)
        return false;

    // calculate mouse tolerance in model coordinates
    int tol = 0;
    ICanvasPtr canvas = getCanvas();
    if (!canvas.isNull())
        tol = canvas->dtom_x(TABLE_MOUSE_RESIZE_TOL);

    // don't count the beginning of the first column as a column edge
    if (x <= tol)
    {
        *col = -1;
        return false;
    }

    // get the edges of the current column we're on
    *col = getColumnIdxByPos(x);
    int x1 = getColumnPosByIdx(*col);
    int x2 = getColumnSize(*col) + x1;

    // if the column size is sufficiently small, the mouse
    // tolerance complicates column selection; so if the
    // column size is small, set the maximum tolerance
    // size to 10% of the column size so column selection
    // always has a chance
    int calc_tol = (x2 - x1)*0.10;
    tol = wxMin(tol, calc_tol);

    // if we're past the end of the last column, but
    // near the second edge, return true
    if (x > x2 && x - x2 <= tol && (*col == col_count - 1))
    {
        return true;
    }

    // if we're near the second edge, return true
    if (x2 >= x && x2 - x <= tol)
    {
        return true;
    }
    
    // count the first edge as the end of previous column;
    // so if we're near the first edge, move back one
    // column and return true
    if (x >= x1 && x - x1 <= tol)
    {
        *col = *col - 1;
        return true;
    }

    // we're not near a column edge, so return false
    *col = -1;
    return false;
}

bool CompTable::isMoveCellEdge(int x, int y)
{
    // note: this function takes an (x,y) position and returns 
    // true if that position is on the move cell edge, and false 
    // otherwise

    PropertyValue value;

    // if the cursor isn't visible, return false
    if (!isCursorVisible())
        return false;

    // if we the move cells preferences isn't set, return false
    if (!isPreference(prefMoveCells))
        return false;

    // if we have more than one selection, we're not
    // on a selection edge; return false
    if (m_cell_selections.size() > 1)
        return false;

    // calculate the tolerance to use around the selected cells to
    // see if we're on a selection edge; if we have column selections, 
    // set the vertical tolerance to zero so we have to use the left 
    // and right sides of the selection as our test; if we have row 
    // selections, set the horizontal tolerance to zero so we have 
    // to use the top and bottom sides of the selection as our test
    int tolx = 0;
    int toly = 0;
    
    ICanvasPtr canvas = getCanvas();
    if (!canvas.isNull())
    {
        tolx = canvas->dtom_x(TABLE_MOUSE_SELECTION_TOL);
        toly = canvas->dtom_x(TABLE_MOUSE_SELECTION_TOL);
    }

    if (hasColumnSelections())
        toly = 0;

    if (hasRowSelections())
        tolx = 0;

    // get the selection rectangle
    wxRect rect;
    getSelectionRangePos(rect);
    
    // if the selection rectangle is empty, return false
    if (rect.IsEmpty())
        return false;
    
    wxRect rect_outside = rect;
    wxRect rect_inside = rect;
    rect_outside.Inflate(tolx, toly);
    rect_inside.Deflate(tolx, toly);

    // if we're not on the edge of the selection rectangle, 
    // return false
    if (!rect_outside.Contains(x, y) ||
         rect_inside.Contains(x, y))
    {
        return false;
    }
    
    // return true
    return true;
}

bool CompTable::isPopulateCellGrip(int x, int y)
{
    // note: this function takes an (x,y) position and returns 
    // true if that position is on the populate cell grip, and 
    // false otherwise

    PropertyValue value;

    // if the cursor isn't visible, return false
    if (!isCursorVisible())
        return false;

    // if the populate cells preference isn't set, return false
    if (!isPreference(prefPopulateCells))
        return false;

    // if we have more than one selection, we don't
    // have a gripper; return false
    if (m_cell_selections.size() > 1)
        return false;

    // get the selection range;
    wxRect rect;
    getSelectionRangePos(rect);

    // if the selection rectangle is empty, return false
    if (rect.IsEmpty())
        return false;

    int grip_x = rect.x + rect.width;
    int grip_y = rect.y + rect.height;
    int tol = 0;

    ICanvasPtr canvas = getCanvas();
    if (!canvas.isNull())
        tol = canvas->dtom_x(TABLE_MOUSE_GRIP_TOL);
            
    // if we're not within the tolerance of the gripper location,
    // return false 
    if (abs(grip_x - x) > tol || abs(grip_y - y) > tol)
        return false;

    // return true
    return true;
}

bool CompTable::isCursorVisible()
{
    // if we don't have a valid canvas, return false
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return false;

    return isPreference(prefCursor);
}

bool CompTable::isSelectionContinuous() const
{
    // note: this function returns true if the selection is a single
    // continuous selection; false otherwise
    
    // if the cell selection size is zero, the cursor cell is
    // the only selection and is continuous; return true
    if (m_cell_selections.size() == 0)
        return true;
        
    // if there's more than 1 selection, return false
    if (m_cell_selections.size() > 1)
        return false;
        
    // if there's one selection and the cursor is contained
    // in that seleciton, return true
    CellRange cursor(m_cursor_row, m_cursor_col);
    isMerged(cursor);
    if (m_cell_selections.front().m_range.contains(cursor))
        return true;
    
    // return false
    return false;
}

void CompTable::getSelectionRange(CellRange& range) const
{
    // note: this function returns the range of cells that bound the
    // selection area; the exception to this is if there are no selections, 
    // in which case the cursor acts as the selection, and we have to see 
    // if the cursor is on a merged cell, in which case, we have to return 
    // the dimensions of the merged cell; cell range is returned is
    // bounded by the range of the cells in the table

    // the cursor cell acts as a selection, so use it as the initial
    // selection range
    range.set(m_cursor_row, m_cursor_col);
    isMerged(range);

    // if there are no selections, return the range of the cursor cell
    if (m_cell_selections.empty())
        return;

    // get the range of the cursor cell, which has been adjusted for
    // any merged cells and use this as the initial cell range to
    // compare with the other selections
    int row1, col1, row2, col2;
    range.get(&row1, &col1, &row2, &col2);

    // return the bounding column and row indexes of the
    // selection area
    int r1, c1, r2, c2;
    std::vector<CellProperties>::const_iterator it, it_end;
    it_end = m_cell_selections.end();

    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        // get the range
        it->getRange(&r1, &c1, &r2, &c2);

        // if the starting row or column index is less than
        // previous row or column indexes, set the new index
        // to the lesser value; note: this will work with
        // range markers, which are set to -1
        if (r1 < row1) {row1 = r1;}
        if (c1 < col1) {col1 = c1;}
 
        // if the ending row or column index is greater than
        // previous row or column indexes, set the new index
        // to the greater value: note: we have to explicitly
        // account for range markers, since they are -1, but
        // represent the end of the row or column, effectively
        // making them the largest possible value 
        if ((r2 > row2 && row2 >= 0) || r2 == -1) {row2 = r2;}
        if ((c2 > col2 && col2 >= 0) || c2 == -1) {col2 = c2;}
    }
    
    // set the range
    range.set(row1, col1, row2, col2);
    
    // limit range to the cells in the table
    range.intersect(CellRange(0, 0, getRowCount() - 1, getColumnCount() - 1));
}

void CompTable::getSelectionRangePos(wxRect& rect)
{
    // note: this function returns the bounding rectangle for the 
    // selection range; selections handle merged cells, so whatever
    // range getSelectionRange() returns takes into account the
    // merged cells, and we should use row and column position/size
    // functions on this range directly, to make sure the rectangle
    // matches the selection range exactly

    // get the cell range of the selection area
    CellRange range;
    getSelectionRange(range);

    // if the range is empty, set the rectangle to empty,
    // and we're done
    if (range.isEmpty())
    {
        rect = wxRect(0,0,-1,-1);
        return;
    }

    int row1, col1, row2, col2;
    range.get(&row1, &col1, &row2, &col2);

    // if the row or column is a range marker, then
    // set the row or column to the minimum or maximum
    // row or column
    if (row1 < 0) {row1 = 0;}
    if (col1 < 0) {col1 = 0;}
    if (row2 < 0) {row2 = getRowCount() - 1;}
    if (col2 < 0) {col2 = getColumnCount() - 1;}

    rect.x = getColumnPosByIdx(col1);
    rect.y = getRowPosByIdx(row1);
    rect.width = getColumnPosByIdx(col2) + getColumnSize(col2) - rect.x;
    rect.height = getRowPosByIdx(row2) + getRowSize(row2) - rect.y;
}

void CompTable::filterEditableProperties(Properties& properties) const
{
    // note: this function filters a set of input properties to a list of
    // properties that can be set on selection areas using the setProperty() 
    // function or the cell editor; the reason we need this is because 
    // setProperty() is overridden to pass through properties to selected 
    // cells; however, this same function is used by other component functions, 
    // such as setOrigin(); so if we pass through everything, these functions 
    // will set these properties
    
    // TODO: a more robust solution would create a separate list of
    // properties that the cells support, then convert back and forth
    // between cell properties and component properties; then we
    // wouldn't need this because the conversion function would handle
    // the exchange

    // copy the list of editable properties; copy the values for these
    // properties and reset the input properties to the new list
    Properties props = m_cell_properties_editable;
    properties.get(props);
    properties = props;
}

void CompTable::invalidateCellRange(const CellRange& range)
{
    // note: this function invalidates the bounding rectangle
    // for the given range, taking into account the fact that
    // the corners of merged cells inside the area being
    // invalidated may extend outside the cells at the start
    // and end of the range

    // get the cell range
    int row1, col1, row2, col2;
    range.get(&row1, &col1, &row2, &col2);

    // if either of the row parameters are -1, set row1 and row2 to
    // the minimum and maximum row values, repsectively
    if (row1 == -1 || row2 == -1)
    {
        row1 = 0;
        row2 = getRowCount();
    }
    
    // if either of the column parameters are -1, set col1 and col2 to
    // the minimum and maximum column values, repsectively
    if (col1 == -1 || col2 == -1)
    {
        col1 = 0;
        col2 = getColumnCount();
    }

    // get the bounds each of the cells in the four
    // corners of the input rectangle
    wxRect c11, c12, c21, c22;
    
    getCellPosByIdx(row1, col1, &c11.x, &c11.y);
    getCellSizeByIdx(row1, col1, &c11.width, &c11.height);

    getCellPosByIdx(row1, col2, &c12.x, &c12.y);
    getCellSizeByIdx(row1, col2, &c12.width, &c12.height);
    
    getCellPosByIdx(row2, col1, &c21.x, &c21.y);
    getCellSizeByIdx(row2, col1, &c21.width, &c21.height);
    
    getCellPosByIdx(row2, col2, &c22.x, &c22.y);
    getCellSizeByIdx(row2, col2, &c22.width, &c22.height);

    // compare the bounds of each of the cells to find the
    // minimum and maximum points of the bounding rectangle
    int x1a = wxMin(c11.x, c12.x);
    int x1b = wxMin(c21.x, c22.x);
    int y1a = wxMin(c11.y, c12.y);
    int y1b = wxMin(c21.y, c22.y);
    
    int x2a = wxMax(c11.x + c11.width, c12.x + c12.width);
    int x2b = wxMax(c21.x + c21.width, c22.x + c22.width);
    int y2a = wxMax(c11.y + c11.height, c12.y + c12.height);
    int y2b = wxMax(c21.y + c21.height, c22.y + c22.height);
    
    int x1 = wxMin(x1a, x1b);
    int y1 = wxMin(y1a, y1b);
    int x2 = wxMax(x2a, x2b);
    int y2 = wxMax(y2a, y2b);
    
    // create the update rectangle and expand it slightly
    // for the selection line width
    wxRect rect = wxRect(x1, y1, x2 - x1, y2 - y1);
    
    ICanvasPtr canvas = getCanvas();
    if (!canvas.isNull())
        rect.Inflate(canvas->dtom_x(m_selection_line_width));

    // invalidate the rectangle
    invalidate(rect);
}

void CompTable::invalidateCellRange(const std::vector<CellRange>& ranges)
{
    // iterate through the cell ranges and invalidate them
    std::vector<CellRange>::const_iterator it, it_end;
    it_end = ranges.end();
    
    for (it = ranges.begin(); it != it_end; ++it)
    {
        invalidateCellRange(*it);
    }
}

void CompTable::invalidateEditor(bool row)
{
    // note: this function invalidates the rows containing the cell editor
    // of row is true or just the area covered by the cell editor if row
    // is false; in the later case, the area invalidated, takes into account 
    // the size before and after the editor's been positioned, causing it to 
    // grow or shrink

    // get the old editor size
    wxRect old_editor_size;
    m_editor->getOrigin(&old_editor_size.x, &old_editor_size.y);
    m_editor->getSize(&old_editor_size.width, &old_editor_size.height);

    // update the position and size of the editor
    positionEditor(m_cursor_row, m_cursor_col);
    
    // get the new editor size
    wxRect new_editor_size;
    m_editor->getOrigin(&new_editor_size.x, &new_editor_size.y);
    m_editor->getSize(&new_editor_size.width, &new_editor_size.height);
    
    // find the larger of the old and new sizes and create an initial
    // invalid rectangle
    wxRect invalid_rect = new_editor_size.Union(old_editor_size);
    
    // if the row flag is set to true, find the size of the row containing 
    // the invalid rect and expand the invalid rect to maximum extent of the
    // original invalid rect and the rows containing that rect
    if (row)
    {
        // get the invalid range index
        CellRange invalid_range;
        getCellRangeIdx(invalid_rect, invalid_range);
        
        // set the range so that it spans the whole row
        invalid_range.set(invalid_range.row1(), -1, invalid_range.row2(), -1);
        
        // find the rectangle for the range of rows containing the
        // original invalid rect
        wxRect invalid_rows_rect;
        getCellRangePos(invalid_range, invalid_rows_rect);
        
        // expand the original invalid rect to encompass the rows;
        // note: if the invalid area of the editor exceeds the dimensions
        // of the table, this will correctly invalidate the area outside
        // the cell because we're taking the union of the original invalid
        // rectangle and the row rectangle
        invalid_rect.Union(invalid_rows_rect);
    }

    // invalidate the invalid area
    invalidate(invalid_rect);
}

void CompTable::invalidateSelection()
{
    // the cursor cell acts as a selection, so if the cursor
    // is visible, invalidate the cursor cell
    if (isCursorVisible())
        invalidateCellRange(CellRange(m_cursor_row, m_cursor_col));

    // invalidate the selections
    std::vector<CellProperties>::iterator it, it_end;
    it_end = m_cell_selections.end();

    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        invalidateCellRange(it->m_range);
    }
}

void CompTable::highlightCells(const CellRange& range)
{
    // note: this function highlights a range of cells; highlights 
    // are different from selections, in that they outline cells, 
    // but don't allow operations on the outline; they are useful 
    // for hints to show an action on a range of cells

    // for now, only allow one range of cells to be highlighted, so 
    // clear previous highlights before adding new highlight
    m_cell_highlights.clear();
    m_cell_highlights.push_back(range);
}

void CompTable::removeCellHighlight()
{
    m_cell_highlights.clear();
}

bool CompTable::hasCellHighlight()
{
    if (m_cell_highlights.size() > 0)
        return true;

    return false;
}

void CompTable::getCellHighlight(CellRange& range)
{
    if (m_cell_highlights.empty())
        return;
        
    range = m_cell_highlights.back();
}

void CompTable::getCellHighlightPos(wxRect& rect)
{
    // note: this function returns a rectangle corresponding to the
    // cell highlight; at this point, we want to use cell highlights 
    // to ignore merged cells, since it's being used for moving and 
    // populating cell hints; later this may change, but for now, we 
    // want to calculate the row and column positions directly, 
    // similar to getSelectionRangePos()
    
    // if we don't have any highlights, we're done
    if (m_cell_highlights.empty())
        return;

    // get the cell range highlight
    int row1, col1, row2, col2;
    m_cell_highlights.back().get(&row1, &col1, &row2, &col2);

    // if the row or column is a range marker, then set the row 
    // or column to the minimum or maximum row or column
    if (row1 < 0) {row1 = 0;}
    if (col1 < 0) {col1 = 0;}
    if (row2 < 0) {row2 = getRowCount() - 1;}
    if (col2 < 0) {col2 = getColumnCount() - 1;}

    // return the position of the highlight, using the column and
    // width calculations to ignore merged cells
    rect.x = getColumnPosByIdx(col1);
    rect.y = getRowPosByIdx(row1);
    rect.width = getColumnPosByIdx(col2) + getColumnSize(col2) - rect.x;
    rect.height = getRowPosByIdx(row2) + getRowSize(row2) - rect.y;
}

void CompTable::findNextCell(int row, int col,
                             int* row_diff, int* col_diff)
{
    // note: this function takes an input row and column as well
    // as a row and column offset to move the input row and column
    // by within the table, accounting for merged cells; the function 
    // then modifies the row and column offset so that they can be 
    // added to the input row and column to move to the next cell

    // if the row and column difference are zero, we're done
    if (*row_diff == 0 && *col_diff == 0)
        return;

    // if we're not in a merged cell, we're done
    CellRange cell(row, col);
    if (!isMerged(cell))
        return;

    // get the merged cell range
    int row1, col1, row2, col2;
    cell.get(&row1, &col1, &row2, &col2);

    // adjust the row and column for range markers
    // limit range markers to the range of the table    
    if (row1 < 0) {row1 = 0;}
    if (col1 < 0) {col1 = 0;}
    if (row2 < 0) {row2 = getRowCount() - 1;}
    if (col2 < 0) {col2 = getColumnCount() - 1;}

    // if we're in a merged cell, try to find new row and column offsets 
    // that will get us out; this is normally possible, unless the merged
    // cell is a range and we're moving in the same direction as the range
    if (*row_diff > 0 )
        *row_diff = wxMax(*row_diff, row2 - row + 1);

    if (*row_diff < 0)
        *row_diff = wxMin(*row_diff, row1 - row - 1);

    if (*col_diff > 0)
        *col_diff = wxMax(*col_diff, col2 - col + 1);

    if (*col_diff < 0)
        *col_diff = wxMin(*col_diff, col1 - col - 1);
}

void CompTable::findNextUnmergedCell(int row, int col, int* row_diff, int* col_diff)
{
    // note: this function finds the next unmerged cell, moving in the same
    // direction as the initial row or column difference, and returns the
    // row and column offset necessary to move there from the input row
    // and column
    
    // if the row and column difference are zero, don't try to do anything
    if (*row_diff == 0 && *col_diff == 0)
        return;

    // if the row or column difference takes us past the end of the table, 
    // set the row and column difference to zero; this is to avoid stack
    // overflows on rows or columns that are completely merged
    if (row + *row_diff >= getRowCount() || col + *col_diff >= getColumnCount())
    {
        *row_diff = 0;
        *col_diff = 0;
        return;
    }

    // first, find the next cell, moving in the direction of the initial row
    // and column offset        
    findNextCell(row, col, row_diff, col_diff);
        
    // if the cell we end up on is not a merged cell, we're done
    CellRange cell(row + *row_diff, col + *col_diff);
    if (!isMerged(cell))
        return;

    // if the cell we ended up on is a merged cell, keep moving in the same
    // direction we started moving by one cell
    int row_offset = (*row_diff > 0 ? 1 : (*row_diff < 0 ? -1 : 0));
    int col_offset = (*col_diff > 0 ? 1 : (*col_diff < 0 ? -1 : 0));
    findNextUnmergedCell(row + *row_diff, col + *col_diff, &row_offset, &col_offset);
    
    // add on the row and offset needed to get to the unmerged cell to 
    // the initial offset
    *row_diff += row_offset;
    *col_diff += col_offset;
}

void CompTable::findNextCellSelection(int row, int col, 
                                      int* row_diff, int* col_diff,
                                      bool select_cursor)
{
    // note: this function takes an input row and column as well as
    // a row and column offset to move the input row and column by
    // within the selection area; the function then modifies the row
    // and column offset so that they can be added to the input row
    // and column to move to the next selected cell; if select_cursor
    // is true, then the cell that the cursor before it is moved will 
    // be selected if it isn't already explicitly selected

    // if the row and column difference are zero, we're done
    if (*row_diff == 0 && *col_diff == 0)
        return;

    // next, find the next valid cell apart from the selection area, 
    // to see if we're in a merged cell
    findNextCell(row, col, row_diff, col_diff);

    // if we don't have a selection, we can move the input
    // row and column anywhere, and we're fine, so we're done
    if (m_cell_selections.size() < 1)
        return;

    // get the initial direction we're traveling and save it for
    // proper merged cell traversal, which we'll handle at the
    // end of the function
    int row_dir = (*row_diff > 0 ? 1 : (*row_diff < 0 ? -1 : 0));
    int col_dir = (*col_diff > 0 ? 1 : (*col_diff < 0 ? -1 : 0));

    // if the cursor is not in a non-cursor selection and the select
    // cursor flag is true, select the cells; this is so that when
    // we're scrolling through multiple selection areas, the original
    // cursor cell gets selected so that it's also included in the
    // selections that get scrolled through
    if (!isCellSelected(row, col, false) && select_cursor)
        selectCells(CellRange(row, col));
 
    // the selection index is set to -1 in the mouse cell selection
    // action; so if it's -1, then the cursor is in the last selection
    // area, so set the selection index to the last selection area;
    // otherwise leave it alone: the cursor is in another selection
    // area which we're using to track the scrolling through the
    // selection areas
    if (m_cursor_selection_idx == -1)
        m_cursor_selection_idx = m_cell_selections.size() - 1;

    // as a sanity check, make sure the cursor selection is inside
    // the bounds of the cell selection range
    int selection_idx_last = m_cell_selections.size() - 1;
    
    if (m_cursor_selection_idx > selection_idx_last)
        m_cursor_selection_idx = selection_idx_last;

    if (m_cursor_selection_idx < 0)
        m_cursor_selection_idx = 0;
 
    // determine the previous and next selection areas    
    int selection_idx_next = m_cursor_selection_idx + 1;
    int selection_idx_prev = m_cursor_selection_idx - 1;
    
    if (selection_idx_prev < 0)
        selection_idx_prev = selection_idx_last;
        
    if (selection_idx_next > selection_idx_last)
        selection_idx_next = 0;

    // set the current, next and previous selections
    CellRange current_selection;
    CellRange prev_selection;
    CellRange next_selection;

    current_selection = m_cell_selections[m_cursor_selection_idx].m_range;
    prev_selection = m_cell_selections[selection_idx_prev].m_range;
    next_selection = m_cell_selections[selection_idx_next].m_range;

    // adjust row_diff and col_diff so that the new row and column
    // position will "flow" to the next logical place in the selections;
    // first, get the range of the current selection area, then handle 
    // the various cases
    int row1, col1, row2, col2;
    current_selection.get(&row1, &col1, &row2, &col2);

    // if we have range markers, set the selection range to the
    // appropriate values for the table
    row1 = (row1 == -1) ? 0 : row1;
    col1 = (col1 == -1) ? 0 : col1;
    row2 = (row2 == -1) ? (getRowCount() - 1) : row2;
    col2 = (col2 == -1) ? (getColumnCount() - 1) : col2;

    // determine the position and set the new_cell flag to false
    int new_row = row + *row_diff;
    int new_col = col + *col_diff;
    bool new_cell = false;

    // case1: the offsets put the new row past row2 and the 
    // new column within the column range of the selection;
    // set the row to row1 and increment column
    if (!new_cell && new_row > row2 && new_col >= col1 && new_col < col2)
    {
        *row_diff = row1 - row;
        *col_diff = 1;
        new_cell = true;
    }

    // case2: the offsets put the new row before row1 and the 
    // new column within the column range of the selection;
    // set the row to row2 and decrement the column
    if (!new_cell && new_row < row1 && new_col > col1 && new_col <= col2)
    {
        *row_diff = row2 - row;
        *col_diff = -1;
        new_cell = true;
    }

    // case3: the offsets put the new column after column2 and 
    // the new row within the row range of the selection;
    // set the column to column1 and increment the row
    if (!new_cell && new_col > col2 && new_row >= row1 && new_row < row2)
    {
        *row_diff = 1;
        *col_diff = col1 - col;
        new_cell = true;
    }

    // case4: the offsets put the new column before column1 and 
    // the new row within the row range of the selection;
    // set the column to column2 and decrement the row
    if (!new_cell && new_col < col1 && new_row > row1 && new_row <= row2)
    {
        *row_diff = -1;
        *col_diff = col2 - col;
        new_cell = true;
    }

    // case5: the offsets put the new cell to the upper-right or
    // lower-right of the selection area; set the offsets so
    // the new position will be in the first cell of the next 
    // selection area; increment the selection index
    if (!new_cell && ((new_col > col2) ||
        (new_col == col2 && new_row > row2)))
    {
        // get the range of the next selection area
        int row1_next, col1_next, row2_next, col2_next;
        next_selection.get(&row1_next, &col1_next,
                           &row2_next, &col2_next);

        // adjust the position for range markers
        row1_next = (row1_next == -1) ? 0 : row1_next;
        col1_next = (col1_next == -1) ? 0 : col1_next;

        // determine the offset to get to the new position
        *row_diff = row1_next - row;
        *col_diff = col1_next - col;
        new_cell = true;

        // set the cursor selection to the index of the
        // next selection area
        m_cursor_selection_idx = selection_idx_next;
    }

    // case6: the offsets put the new cell to the upper-left or
    // lower-left of the selection area; set the offsets so
    // the new position will be in the last cell of the previous 
    // selection area
    if (!new_cell && ((new_col < col1) ||
        (new_col == col1 && new_row < row1)))
    {
        // get the range of the previous selection area
        int row1_prev, col1_prev, row2_prev, col2_prev;
        prev_selection.get(&row1_prev, &col1_prev,
                           &row2_prev, &col2_prev);

        // adjust the position for range markers
        row2_prev = (row2_prev == -1) ? (getRowCount() - 1) : row2_prev;
        col2_prev = (col2_prev == -1) ? (getColumnCount() - 1) : col2_prev;

        // determine the offset to get to the new position
        *row_diff = row2_prev - row;
        *col_diff = col2_prev - col;
        new_cell = true;

        // set the cursor selection to the index of the
        // previous selection area
        m_cursor_selection_idx = selection_idx_prev;
    }
    
    // finally, we only want to move into a merged cell once per 
    // selection area; so, if the new cell we're entering into is
    // a merged cell, make sure we're entering it from an allowed
    // direction
    CellRange new_cell_range(row + *row_diff, col + *col_diff);
    if (isMerged(new_cell_range))
    {
        // if the new cell we're trying to enter is a merged cell, we
        // only want to enter the merged cell if we're approaching it
        // from the proper direction, ensuring that we only enter the
        // merged cell once per selection area, no matter what direction
        // we're traversing the selection; so get the new cell range 
        // row and column values
        int row1, col1, row2, col2;
        new_cell_range.get(&row1, &col1, &row2, &col2);
        
        // next, determine the entry row and column based on the initial 
        // row and column direction we're traveling; this is the row 
        // and column we're able to enter the merged cell from
        int entry_row = row_dir >= 0 ? row1 : row2;
        int entry_col = col_dir >= 0 ? col1 : col2;
        
        // set the new row and column from the initial row and column 
        // plus the row and column offset we calculated
        int new_row = row + *row_diff;
        int new_col = col + *col_diff;
        
        // if the new row and column isn't on a merged cell row or
        // column entry point, find the next valid selection cell and
        // add on the new row and column offset to the row and column
        // offset we've already calculated
        if (new_row != entry_row || new_col != entry_col)
        {
            // find the next cell selection; set the select_cursor flag to
            // false so we don't select merged cells that are being skipped
            // and aren't selected, which might happen if we're moving down
            // a selected column and the column passes through unselected
            // merged cells that are wider than the selected column
            findNextCellSelection(new_row, new_col, &row_dir, &col_dir, false);
            *row_diff += row_dir;
            *col_diff += col_dir;
        }
    }
}

void CompTable::onEditorInvalidated(IEventPtr evt)
{
    // note: event handler to proxy the editor events
    // back to the canvas; used to invalidate the
    // region occupied by the cell editor in reponse
    // to editor invalidate events

    INotifyEventPtr notify_evt = evt;
    if (notify_evt.isNull())
        return;

    // get the invalid region from the event
    int x, y, w, h;
    PropertyValue value;
       
    notify_evt->getProperty(EVENT_PROP_X, value);
    x = value.getInteger();
    
    notify_evt->getProperty(EVENT_PROP_Y, value);
    y = value.getInteger();
    
    notify_evt->getProperty(EVENT_PROP_W, value);
    w = value.getInteger();
    
    notify_evt->getProperty(EVENT_PROP_H, value);
    h = value.getInteger();
    
    // translate the invalid region into table coordinates
    // and invalidate the region
    int x_editor, y_editor;
    m_editor->getOrigin(&x_editor, &y_editor);
    x += x_editor;
    y += y_editor;
    
    wxRect rect(x, y, w, h);
    invalidate(rect);
}

void CompTable::onKey(IEventPtr evt)
{
    // if we're not in the at-target phase,
    // we're done
    if (evt->getPhase() != EVENT_AT_TARGET)
        return;

    IKeyEventPtr key_evt = evt;
    if (key_evt.isNull())
        return;

    // reset the focus flag
    m_focus_set = false;

    // get the key code parameters
    int key_code = key_evt->getKeyCode();

    // set a direction flag based on the state
    // of the shift key
    int direction = key_evt->isShiftDown() ? -1 : 1;

    // get the cursor position
    int old_cursor_row, old_cursor_col;
    getCursorPos(&old_cursor_row, &old_cursor_col);

    if (isEditing())
    {
        switch (key_code)
        {
            case WXK_TAB:
                endEdit(true);
                moveCursor(0, direction, false);
                break;
        
            case WXK_RETURN:
                {
                    // if the control or the alt keys are down, dispatch the
                    // return key to the editor; otherwise, end the edit
                    if (key_evt->isCtrlDown() || key_evt->isAltDown())
                    {
                        evt->setTarget(m_editor);
                        IEventTargetPtr target = m_editor;
                        target->dispatchEvent(evt);
                    }
                    else
                    {
                        endEdit(true);
                        moveCursor(direction, 0, false);
                    }
                }
                break;

            case WXK_ESCAPE:
                endEdit(false);
                break;

            default:
                // dispatch the event
                evt->setTarget(m_editor);
                IEventTargetPtr target = m_editor;
                target->dispatchEvent(evt);
                break;
        }
    }
    else
    {
        switch (key_code)
        {
            case WXK_LEFT:
            case WXK_NUMPAD_LEFT:
                {
                    if (!key_evt->isShiftDown())
                        moveCursor(0, -1, true);
                    else
                        resizeSelection(0, -1);
                }
                break;

            case WXK_RIGHT:
            case WXK_NUMPAD_RIGHT:
                {
                    if (!key_evt->isShiftDown())
                        moveCursor(0, 1, true);
                    else
                        resizeSelection(0, 1);
                }
                break;
                
            case WXK_UP:
            case WXK_NUMPAD_UP:
                {
                    if (!key_evt->isShiftDown())
                        moveCursor(-1, 0, true);
                    else
                        resizeSelection(-1, 0);
                }
                break;

            case WXK_DOWN:
            case WXK_NUMPAD_DOWN:
                {
                    if (!key_evt->isShiftDown())
                        moveCursor(1, 0, true);
                    else
                        resizeSelection(1, 0);
                }
                break;

            case WXK_HOME:
            case WXK_NUMPAD_HOME:
                break;
                
            case WXK_END:
            case WXK_NUMPAD_END:
                break;

            case WXK_PAGEUP:
            case WXK_NUMPAD_PAGEUP:
                break;

            case WXK_PAGEDOWN:
            case WXK_NUMPAD_PAGEDOWN:
                break;

            case WXK_TAB:
                moveCursor(0, direction, false);
                break;
                
            case WXK_RETURN:
                moveCursor(direction, 0, false);
                break;

            case WXK_DELETE:
                {
                    // create a pre clear cell event; if the event
                    // isn't vetoed, process it
                    INotifyEventPtr notify_pre_evt;
                    notify_pre_evt = NotifyEvent::create(EVENT_TABLE_PRE_CELL_CLEAR, this);
                    dispatchEvent(notify_pre_evt);

                    if (notify_pre_evt->isAllowed())
                    {
                        // clear the selected cells and invalidate
                        // the selection
                        clear(true);
                        invalidateSelection();

                        // dispatch the clear cell event
                        INotifyEventPtr notify_evt;
                        notify_evt = NotifyEvent::create(EVENT_TABLE_CELL_CLEAR, this);
                        dispatchEvent(notify_evt);
                    }
                }
                break;

            // TODO: need to stop edit when the editor doesn't process 
            // the keystroke so that values like escape don't trigger
            // the editing mode
            case WXK_ESCAPE:
                break;

            default:
            
                // try to start editing
                beginEdit();

                // if we're editing, dispatch the edit event
                if (isEditing())
                {
                    // if we have an edit interface, clear the text
                    // contents before dispatching the event; we do
                    // this here instead of in beginEdit() because
                    // we want to clear the contents of the cell when
                    // we start editing by pressing a key, but not
                    // when we click on the cell with the mouse
                    IEditPtr edit = m_editor;
                    if (!edit.isNull())
                        edit->clear(true);
                    
                    // dispatch the key event to the target
                    evt->setTarget(m_editor);
                    IEventTargetPtr target = m_editor;
                    target->dispatchEvent(evt);
                }
                break;
        }
    }

    // if we're editing, invalidate the area covered by the cell editor;
    // if we're not editing, invalidate the old and new cursor cells, 
    // if the position has changed
    if (isEditing())
    {
        invalidateEditor(false);
    }
    else
    {
        // invalidate the cell ranges where the cursor has changed
        int new_cursor_row, new_cursor_col;
        getCursorPos(&new_cursor_row, &new_cursor_col);
        
        CellRange new_cursor(new_cursor_row, new_cursor_col);
        CellRange old_cursor(old_cursor_row, old_cursor_col);
        
        std::vector<CellRange> invalid_ranges;
        CellRange::Xor(new_cursor, old_cursor, invalid_ranges);
        invalidateCellRange(invalid_ranges);
    }
}

void CompTable::onMouse(IEventPtr evt)
{
    // if we're not in the at-target phase,
    // we're done
    if (evt->getPhase() != EVENT_AT_TARGET)
        return;

    IMouseEventPtr mouse_evt = evt;
    if (mouse_evt.isNull())
        return;

    // set the last mouse position state variables
    m_mouse_x_last = m_mouse_x;
    m_mouse_y_last = m_mouse_y;
    
    // set the current mouse position state variables
    m_mouse_x = mouse_evt->getX();
    m_mouse_y = mouse_evt->getY();

    // if the table just received the focus and this change
    // in focus was initiated by a click, set the cursor to
    // the new position; this ensures that if the ctrl or
    // shift key is being held down when the table receives
    // the focus, that the old cursor position isn't saved 
    // as a selection; currently, this is the only reason
    // we track the change in focus
    if (m_focus_set &&
        (evt->getName() == EVENT_MOUSE_LEFT_DOWN ||
         evt->getName() == EVENT_MOUSE_MIDDLE_DOWN ||
         evt->getName() == EVENT_MOUSE_RIGHT_DOWN))
    {
        int row, col;
        getCellIdxByPos(m_mouse_x, m_mouse_y, &row, &col);
        setCursorPos(row, col);
    }
    
    // reset the focus flag
    m_focus_set = false;
    
    // handle the mouse event
    mouseFeedback(mouse_evt);
    mouseCellEdit(mouse_evt);
    mouseRightClick(mouse_evt);
    mouseResizeCellsToContent(mouse_evt);
    mouseResizeCells(mouse_evt);
    mousePopulateCells(mouse_evt);
    mouseMoveCells(mouse_evt);
    mouseSelectCells(mouse_evt);
}

void CompTable::onFocus(IEventPtr evt)
{
    if (evt->getName() == EVENT_FOCUS_SET)
    {
        // set the focus flag; will be set false on the 
        // first mouse or key event
        m_focus_set = true;
    }
    
    if (evt->getName() == EVENT_FOCUS_KILL)
    {
        // if the focus is lost, make sure to reset the
        // action
        resetAction();
    }
}

void CompTable::mouseFeedback(IEventPtr evt)
{
    if (evt->getName() != EVENT_MOUSE_OVER)
        return;

    // set the cursor
    if (m_mouse_action != ACTION_NONE)
        return;

    int col, row;
    wxString cursor = CURSOR_STANDARD;

    if (isResizeColumnEdge(m_mouse_x, m_mouse_y, &col))
        cursor = CURSOR_SIZEWE;

    if (isResizeRowEdge(m_mouse_x, m_mouse_y, &row))
        cursor = CURSOR_SIZENS;

    if (isMoveCellEdge(m_mouse_x, m_mouse_y))
        cursor = CURSOR_MOVE;

    if (isPopulateCellGrip(m_mouse_x, m_mouse_y))
        cursor = CURSOR_CROSS;

    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_CANVAS_CURSOR, this);
    notify_evt->addProperty(EVENT_PROP_CURSOR, cursor);
    dispatchEvent(notify_evt);
}

void CompTable::mouseCellEdit(IEventPtr evt)
{
    // TODO: when dispatching events to the component, we should create
    // and fire a new event so as not to change the old event, in case
    // there are additional handlers downstream

    IMouseEventPtr mouse_evt = evt;

    // if we're in the middle of another action, we're done
    if (m_mouse_action != ACTION_NONE && m_mouse_action != ACTION_EDITCELL)
        return;

    // get the event name
    wxString name = evt->getName();

    // if we don't have an action and the event is a double-click
    // event, then start the action
    if (m_mouse_action == ACTION_NONE && name == EVENT_MOUSE_LEFT_DCLICK)
    {
        // try to start editing
        beginEdit();
        
        // if we successfully started the editing, change the event type to
        // a single left click so that when the event gets patched through
        // to the editor, editor actions that start with single clicks, such
        // as selections, can start right away without having to wait for 
        // another click
        if (isEditing())
        {
            evt->setName(EVENT_MOUSE_LEFT_DOWN);
        }
    }

    // if we're performing a cell edit action and we click outside
    // the editor, stop the edit; otherwise, patch through the event
    if (m_mouse_action == ACTION_EDITCELL)
    {
        // get the position of the editor
        int x1, y1, x2, y2;
        m_editor->getOrigin(&x1, &y1);
        m_editor->getSize(&x2, &y2);
        x2 += x1;
        y2 += y1;
    
        // see if the mouse was clicked
        wxString name = evt->getName();
        if (name == EVENT_MOUSE_LEFT_DCLICK ||
            name == EVENT_MOUSE_LEFT_DOWN ||
            name == EVENT_MOUSE_RIGHT_DCLICK ||
            name == EVENT_MOUSE_RIGHT_DOWN)
        {
            // if the mouse is outside the editor, then stop the edit
            if (m_mouse_x < x1 || m_mouse_x > x2 || 
                m_mouse_y < y1 || m_mouse_y > y2)
            {
                endEdit(true);
            }
        }
        
        // if the edit is still ongoing, patch the events through
        // to the editor
        if (isEditing())
        {
            IEventTargetPtr target = m_editor;
            if (!target.isNull())
            {
                // translate the mouse coordinates to the local
                // editor coordinates
                mouse_evt->setX(m_mouse_x - x1);
                mouse_evt->setY(m_mouse_y - y1);

                // dispatch the event to the editor
                evt->setTarget(m_editor);
                target->dispatchEvent(mouse_evt);
                
                // translate the mouse coordinates back to the
                // table coordinates
                mouse_evt->setX(m_mouse_x);
                mouse_evt->setY(m_mouse_y);
            }
        }
    }
}

void CompTable::mouseRightClick(IEventPtr evt)
{
    IMouseEventPtr mouse_evt = evt;

    // if we're in the middle of another action, we're done
    if (m_mouse_action != ACTION_NONE)
        return;

    // get the event name
    wxString name = evt->getName();

    // if the event is a mouse-right-up event, dispatch a 
    // cell right-click event, and we're done
    if (name == EVENT_MOUSE_RIGHT_UP)
    {
        // we've simply right-clicked on a cell, so create a 
        // table cell right-click event
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_CELL_RIGHT_CLICK, this);
        dispatchEvent(notify_evt);
        return;
    }

    // if the event isn't a mouse-right-down event, 
    // and we're done
    if (name != EVENT_MOUSE_RIGHT_DOWN)
        return;

    // create a pre select cell event; if the event
    // is vetoed, reset the action and we're done
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_TABLE_PRE_SELECT_CELL, this);
    dispatchEvent(notify_evt);

    if (!notify_evt->isAllowed())
        return;

    // get the cell index by position
    int row, col;
    getCellIdxByPos(m_mouse_x, m_mouse_y, &row, &col);

    // if we allow row selections on the first column, set the 
    // column to -1 to trigger a row selection
    if (isPreference(prefSelectRows) && col <= 0)
        col = -1;

    // if we allow column selections on the first row, set
    // the column to -1 to trigger a column selection
    if (isPreference(prefSelectColumns) && row <= 0)
        row = -1;

    // if the cell/column/row we click on isn't 
    // already selected, select it
    if (!isTableSelected() && 
        !isRowSelected(row) &&        
        !isColumnSelected(col) && 
        !isCellSelected(row, col))
    {
        // dispatch an event to indicate we're about to remove
        // selections
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_PRE_SELECTION_CLEAR, this);
        dispatchEvent(notify_evt);

        // if the clear selection event is allowed, remove the cell
        // selections
        if (notify_evt->isAllowed())
        {
            invalidateSelection();
            removeAllCellSelections();
        }

        // set the new cursor position; if the whole row or column
        // is selected, keep the cursor in the same column or row, 
        // respectively, that it was in before
        int cursor_row = (row == -1 ? m_cursor_row : row);
        int cursor_col = (col == -1 ? m_cursor_col : col);
        setCursorPos(cursor_row, cursor_col);

        // if the clear selection event is allowed, dispatch a
        // table selection cleared event
        if (notify_evt->isAllowed())
        {
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_TABLE_SELECTION_CLEAR, this);
            dispatchEvent(notify_evt);
        }

        // select the cells
        selectCells(CellRange(row, col));

        // invalidate the new selection
        invalidateSelection();

        // create a selecting cell event
        notify_evt = NotifyEvent::create(EVENT_TABLE_SELECTING_CELL, this);
        dispatchEvent(notify_evt);

        // create a select cells end event
        notify_evt = NotifyEvent::create(EVENT_TABLE_SELECT_CELL, this);
        dispatchEvent(notify_evt);
    }
}

void CompTable::mouseResizeCellsToContent(IEventPtr evt)
{
    // note: this handler resizes a row/column when
    // it is double-clicked; the row/column is resized
    // to the content

    // if we're already performing and action, we're done
    if (m_mouse_action != ACTION_NONE)
        return;

    // if the event isn't a mouse left-double-click
    // event, we're done
    if (evt->getName() != EVENT_MOUSE_LEFT_DCLICK)
        return;

    int row = -1;
    int col = -1;

    // get the cell selections
    std::vector<CellRange> col_selections;
    std::vector<CellRange> row_selections;

    std::vector<CellProperties>::iterator it, it_end;
    it_end = m_cell_selections.end();

    for (it = m_cell_selections.begin(); it != it_end; ++it)
    {
        // get the range of the cell properties
        int r1, c1, r2, c2;
        it->getRange(&r1, &c1, &r2, &c2);

        // save the column selections
        if (r1 == -1 && r2 == -1)
            col_selections.push_back(it->range());

        // save the row selections
        if (c1 == -1 && c2 == -1)
            row_selections.push_back(it->range());
    }

    // if we're on a column edge that can be used for
    // resizing, resize the column to the content
    if (isResizeColumnEdge(m_mouse_x, m_mouse_y, &col))
    {
        // dispatch an event that the column is about to be resized
        INotifyEventPtr notify_pre_evt;
        notify_pre_evt = NotifyEvent::create(EVENT_TABLE_PRE_RESIZE_COLUMN_TO_CONTENT, this);
        notify_pre_evt->addProperty(EVENT_PROP_INDEX, col);
        notify_pre_evt->addProperty(EVENT_PROP_SIZE, getColumnSize(col));
        dispatchEvent(notify_pre_evt);

        // if the event isn't allowed, we're done
        if (!notify_pre_evt->isAllowed())
            return;

        // if the current column isn't selected, resize only this column;
        // otherwise resize this selected column along with the other 
        // selected columns
        if (!isColumnSelected(row))
        {
            resizeColumnsToContent(CellRange(-1, col, -1, col));
        }
        else
        {
            std::vector<CellRange>::iterator it, it_end;
            it_end = col_selections.end();
            
            for (it = col_selections.begin(); it != it_end; ++it)
            {
                resizeColumnsToContent(*it);
            }
        }
        
        // dispatch an event that the column has been resized
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_RESIZE_COLUMN_TO_CONTENT, this);
        notify_evt->addProperty(EVENT_PROP_INDEX, col);
        notify_evt->addProperty(EVENT_PROP_SIZE, getColumnSize(col));
        dispatchEvent(notify_evt);
    }
    
    // if we're on a row edge that can be used for
    // resizing, resize the row to the content
    if (isResizeRowEdge(m_mouse_x, m_mouse_y, &row))
    {
        // dispatch an event that the row is about to be resized
        INotifyEventPtr notify_pre_evt;
        notify_pre_evt = NotifyEvent::create(EVENT_TABLE_PRE_RESIZE_ROW_TO_CONTENT, this);
        notify_pre_evt->addProperty(EVENT_PROP_INDEX, row);
        notify_pre_evt->addProperty(EVENT_PROP_SIZE, getRowSize(row));
        dispatchEvent(notify_pre_evt);

        // if the event isn't allowed, we're done
        if (!notify_pre_evt->isAllowed())
            return;

        // if the current row isn't selected, resize only this row;
        // otherwise resize this selected row along with the other 
        // selected rows
        if (!isRowSelected(row))
        {
            resizeRowsToContent(CellRange(row, -1, row, -1));
        }
        else
        {
            std::vector<CellRange>::iterator it, it_end;
            it_end = row_selections.end();
            
            for (it = row_selections.begin(); it != it_end; ++it)
            {
                resizeRowsToContent(*it);
            }
        }

        // dispatch an event that the row has been resized
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_RESIZE_ROW_TO_CONTENT, this);
        notify_evt->addProperty(EVENT_PROP_INDEX, row);
        notify_evt->addProperty(EVENT_PROP_SIZE, getRowSize(row));
        dispatchEvent(notify_evt);
    }

    // invalidate the table, and we're done
    invalidate();
}

void CompTable::mouseResizeCells(IEventPtr evt)
{
    // note: this handler resizes a row/column when
    // it is dragged

    IMouseEventPtr mouse_evt = evt;

    if (startAction(mouse_evt, ACTION_RESIZECELLS))
    {
        int row = -1;
        int col = -1;

        // if we're not on a column or row edge that can be used
        // for resizing, stop the action
        if (!isResizeColumnEdge(m_mouse_x, m_mouse_y, &col) && 
            !isResizeRowEdge(m_mouse_x, m_mouse_y, &row))
        {
            resetAction();
            return;
        }

        // initiailize the mouse row and column size variables
        m_mouse_row_size = 0;
        m_mouse_col_size = 0;

        // set the mouse row and column
        m_mouse_row = row;
        m_mouse_col = col;

        // if we're resizing a row, create a pre resize row event;
        // if the event is vetoed, reset the action, and we're done
        if (m_mouse_row >= 0 && m_mouse_col < 0)
        {
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_TABLE_PRE_RESIZE_ROW, this);
            notify_evt->addProperty(EVENT_PROP_INDEX, m_mouse_row);
            notify_evt->addProperty(EVENT_PROP_SIZE, getRowSize(m_mouse_row));
            dispatchEvent(notify_evt);
            
            if (!notify_evt->isAllowed())
            {
                resetAction();
                return;
            }
            
            // get the starting row size
            m_mouse_row_size = getRowSize(m_mouse_row);
        }
        
        // if we're resizing a column, create a pre resize column event;
        // if the event is vetoed, reset the action, and we're done
        if (m_mouse_row < 0 && m_mouse_col >= 0)
        {
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_TABLE_PRE_RESIZE_COLUMN, this);
            notify_evt->addProperty(EVENT_PROP_INDEX, m_mouse_col);
            notify_evt->addProperty(EVENT_PROP_SIZE, getColumnSize(m_mouse_col));
            dispatchEvent(notify_evt);
            
            if (!notify_evt->isAllowed())
            {
                resetAction();
                return;
            }
            
            // get the starting column size
            m_mouse_col_size = getColumnSize(m_mouse_col);
        }
 
        // we're actually resizing; add a tag to mark the cell properties 
        // at the time we're starting to resize
        tag(ACTION_RESIZECELLS);
    }

    if (endAction(mouse_evt, ACTION_RESIZECELLS))
    {
        int size = 0;
        int size_diff = 0;
        int size_new = 0;

        bool resize_row = false;
        bool resize_col = false;

        // restore the properties to the state they were in
        // when we started the action; this clears out all of
        // the temporary row/column sizes we added when we
        // were doing the real-time update
        restore(ACTION_RESIZECELLS);

        // if the mouse is on a row and the y position of the
        // mouse changes, then calculate the new size of the
        // row and set the row resize flag
        if (m_mouse_row >= 0 && m_mouse_col < 0 && 
            m_mouse_y_start != m_mouse_y)
        {
            size = m_mouse_row_size;
            size_diff = m_mouse_y - m_mouse_y_start;
            size_new = wxMax(size + size_diff, TABLE_MIN_CELL_HEIGHT);
            resize_row = true;
        }

        // if the mouse is on a column and the x position of the
        // mouse changes, then calculate the new size of the
        // column and set the column resize flag
        if (m_mouse_row < 0 && m_mouse_col >= 0 &&
            m_mouse_x_start != m_mouse_x)
        {
            size = m_mouse_col_size;
            size_diff = m_mouse_x - m_mouse_x_start;
            size_new = wxMax(size + size_diff, TABLE_MIN_CELL_WIDTH);
            resize_col = true;
        }

        // get the cell selections
        std::vector<CellRange> col_selections;
        std::vector<CellRange> row_selections;

        std::vector<CellProperties>::iterator it, it_end;
        it_end = m_cell_selections.end();

        for (it = m_cell_selections.begin(); it != it_end; ++it)
        {
            // get the range of the cell properties
            int r1, c1, r2, c2;
            it->getRange(&r1, &c1, &r2, &c2);

            // save the column selections
            if (r1 == -1 && r2 == -1)
                col_selections.push_back(it->range());

            // save the row selections
            if (c1 == -1 && c2 == -1)
                row_selections.push_back(it->range());
        }

        // if we have a resize row action, resize the action
        // row and all selected rows
        if (resize_row)
        {        
            // if the current row isn't selected, resize only this row;
            // otherwise resize this selected row along with the other 
            // selected rows
            if (!isRowSelected(m_mouse_row))
            {
                setRowSize(m_mouse_row, size_new);
            }
            else
            {
                std::vector<CellRange>::iterator it, it_end;
                it_end = row_selections.end();
                
                for (it = row_selections.begin(); it != it_end; ++it)
                {
                    Properties p;
                    p.add(PROP_SIZE_H, size_new);
                    setCellProperties(*it, p);
                }
            }
            
            // fire a row resize event
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_TABLE_RESIZE_ROW, this);
            notify_evt->addProperty(EVENT_PROP_INDEX, m_mouse_row);
            notify_evt->addProperty(EVENT_PROP_SIZE, size_new);
            dispatchEvent(notify_evt);
        }
        
        // if we have a resize column action, resize the action
        // column and all selected columns
        if (resize_col)
        {
            // if the current column isn't selected, resize only this 
            // column; otherwise resize this selected column along with 
            // the other selected columns
            if (!isColumnSelected(m_mouse_col))
            {
                setColumnSize(m_mouse_col, size_new);
            }
            else
            {
                std::vector<CellRange>::iterator it, it_end;
                it_end = col_selections.end();
                
                for (it = col_selections.begin(); it != it_end; ++it)
                {
                    Properties p;
                    p.add(PROP_SIZE_W, size_new);
                    setCellProperties(*it, p);
                }
            }

            // fire a column resize event
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_TABLE_RESIZE_COLUMN, this);
            notify_evt->addProperty(EVENT_PROP_INDEX, m_mouse_col);
            notify_evt->addProperty(EVENT_PROP_SIZE, size_new);
            dispatchEvent(notify_evt);
        }

        // invalidate the table
        invalidate();
    }

    if (isAction(mouse_evt, ACTION_RESIZECELLS))
    {
        int size = 0;
        int size_diff = 0;
        int size_new = 0;

        // restore and re-rag the properties to the state they 
        // were in when we started the action; this clears out 
        // all of the temporary row/column sizes we might have 
        // just added while doing the last "real time" update,
        // and gives us a fresh tag to restore at the next
        // "real time" update or at the end of the action
        restore(ACTION_RESIZECELLS);
        tag(ACTION_RESIZECELLS);

        // if the mouse is on a row and the y position of the
        // mouse changes, then calculate the new size of the
        // row and fire a row resizing event
        if (m_mouse_row >= 0 && m_mouse_col < 0 && 
            m_mouse_y_start != m_mouse_y)
        {
            size = m_mouse_row_size;
            size_diff = m_mouse_y - m_mouse_y_start;
            size_new = wxMax(size + size_diff, TABLE_MIN_CELL_HEIGHT);

            // set a new temporary rowsize for real-time feedback
            setRowSize(m_mouse_row, size_new);

            // fire a row resizing event
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_TABLE_RESIZING_ROW, this);
            notify_evt->addProperty(EVENT_PROP_INDEX, m_mouse_row);
            notify_evt->addProperty(EVENT_PROP_SIZE, size_new);
            dispatchEvent(notify_evt);
        }

        // if the mouse is on a column and the x position of the
        // mouse changes, then calculate the new size of the
        // column and fire a column resizing event
        if (m_mouse_row < 0 && m_mouse_col >= 0 &&
            m_mouse_x_start != m_mouse_x)
        {
            size = m_mouse_col_size;
            size_diff = m_mouse_x - m_mouse_x_start;
            size_new = wxMax(size + size_diff, TABLE_MIN_CELL_WIDTH);

            // set a new temporary rowsize for real-time feedback
            setColumnSize(m_mouse_col, size_new);

            // fire a column resizing event
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_TABLE_RESIZING_COLUMN, this);
            notify_evt->addProperty(EVENT_PROP_INDEX, m_mouse_col);
            notify_evt->addProperty(EVENT_PROP_SIZE, size_new);
            dispatchEvent(notify_evt);
        }

        // if the size has changed, invalidate the table;
        // TODO: only invalidate the area that changes
        if (size_diff != 0)
            invalidate();
    }
}

void CompTable::mousePopulateCells(IEventPtr evt)
{
    IMouseEventPtr mouse_evt = evt;

    if (startAction(mouse_evt, ACTION_POPULATECELLS))
    {
        // if we're not on the populate cell grip, reset 
        // the action; we're done
        if (!isPopulateCellGrip(m_mouse_x, m_mouse_y))
        {
            resetAction();
            return;
        }

        // create a pre populate cell event; if the event
        // is vetoed, reset the action, and we're done
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_PRE_POPULATE_CELL, this);
        dispatchEvent(notify_evt);
        
        if (!notify_evt->isAllowed())
        {
            resetAction();
            return;
        }
    }

    if (endAction(mouse_evt, ACTION_POPULATECELLS))
    {
        // get the cell higlight and selection; limit the
        // selection to the range of the table
        CellRange highlight, selection, table;
        getCellHighlight(highlight);
        getSelectionRange(selection);
        table.set(0, 0, getRowCount(), getColumnCount());
        selection.intersect(table);
        
        // remove the cell highlight
        removeCellHighlight();
        
        // if we didn't have a selection highlight, or the selection 
        // and the highlight are the same, we're done
        if (highlight.isEmpty() || selection == highlight)
        {
            // invalidate the selection
            invalidateSelection();
            return;
        }

        // if the selection and highlight ranges share the
        // cell in the upper-left of the range, populate
        // the highlight range in normal order (left-to-right
        // and top-to-bottom); otherwise populate the
        // highlight range in reverse order (right-to-left
        // and bottom-to-top)        
        bool h_reverse = (selection.col1() == highlight.col1() ? false : true);
        bool v_reverse = (selection.row1() == highlight.row1() ? false : true);

        // populate the new range of cells with the values in the
        // original selection
        copyCellProperties(selection, highlight, true, h_reverse, v_reverse);
        
        // remove the previous selection and select the
        // newly copied cells
        removeAllCellSelections();
        selectCells(highlight);

        // invalidate the table; we invalidate the table rather than the 
        // selection, because cause the populate cell operation may unmerge 
        // merged cells, and it's easier to invalidate the whole component 
        // than calculate which cells have been unmerged
        invalidate();
        
        // create a populate cells event
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_POPULATE_CELL, this);
        dispatchEvent(notify_evt);
    }

    if (isAction(mouse_evt, ACTION_POPULATECELLS))
    {
        // get the selection area and limit it to the range
        // of the table
        CellRange selection, table;
        getSelectionRange(selection);
        table.set(0, 0, getRowCount(), getColumnCount());
        selection.intersect(table);

        // set the starting and ending rows and columns to the
        // lower-right of the selection area, which contains the
        // cursor gripper
        int sel_row1, sel_col1, sel_row2, sel_col2;
        selection.get(&sel_row1, &sel_col1, &sel_row2, &sel_col2);

        int row1, col1, row2, col2;
        row1 = sel_row1;
        col1 = sel_col1;
        row2 = sel_row2;
        col2 = sel_col2;

        // get the cell at the mouse position without regard to merged cells
        // and set the new end cell to this cell; get the center of this cell
        int xc, yc;
        getCellIdxByPos(m_mouse_x, m_mouse_y, &row2, &col2, false);
        getCellCenterByIdx(row2, col2, &xc, &yc, false);

        // if the mouse isn't at least half way through the cell
        // move back one cell
        if (col2 > col1 && m_mouse_x < xc)
            col2--;

        if (col2 < col1 && m_mouse_x > xc)
            col2++;

        if (row2 > row1 && m_mouse_y < yc)
            row2--;

        if (row2 < row1 && m_mouse_y > yc)
            row2++;

        // if we're still on the same selected cells and we don't have
        // a highlight, we're done; this is to prevent the highlight
        // from starting until we start moving the gripper to other
        // cells
        if (row1 == sel_row1 && col1 == sel_col1 &&
            row2 == sel_row2 && col2 == sel_col2 &&
            !hasCellHighlight())
        {
            return;
        }

        // if the end row or column is less than the start row or column,
        // set the start row or column to the end of the selection area
        // so that the highlight encompasses the selected cells
        if (row2 < row1)
            row1 = sel_row2;
        
        if (col2 < col1)
            col1 = sel_col2;

        // compare the position of the mouse to the closest corner of the 
        // selected cells; then, if there's a choice between populating 
        // the cells along the rows or the columns, then populate the cells 
        // along the columns if the mouse is moving more horizontally than
        // vertically; otherwise populate the cells along the rows
        wxRect rect;
        getCellRangePos(selection, rect, false);
        int sel_xc = rect.x + rect.width/2;
        int sel_yc = rect.y + rect.height/2;
        
        int start_x = rect.x + rect.width;
        int start_y = rect.y + rect.height;
        
        if (m_mouse_x < sel_xc)
            start_x = rect.x;
        
        if (m_mouse_y < sel_yc)
            start_y = rect.y;
            
        if (abs(m_mouse_x - start_x) > abs(m_mouse_y - start_y) &&
            abs(col2 - col1) != abs(sel_col2 - sel_col1))
        {
            row1 = sel_row1;
            row2 = sel_row2;
        }

        if (abs(m_mouse_x - start_x) <= abs(m_mouse_y - start_y) &&
            abs(row2 - row1) != abs(sel_row2 - sel_row1))
        {
            col1 = sel_col1;
            col2 = sel_col2;
        }

        // clamp the row and column ranges to the range of the table
        int row_count = getRowCount();
        int col_count = getColumnCount();
        
        if (row1 < 0) {row1 = 0;}
        if (col1 < 0) {col1 = 0;}
        if (row1 >= row_count - 1) {row1 = row_count - 1;}
        if (col1 >= col_count - 1) {col1 = col_count - 1;}
        if (row2 < 0) {row2 = 0;}
        if (col2 < 0) {col2 = 0;}
        if (row2 >= row_count - 1) {row2 = row_count - 1;}
        if (col2 >= col_count - 1) {col2 = col_count - 1;}

        // if we already have a highlight, use it as our
        // starting highlight
        CellRange old_highlight;
        getCellHighlight(old_highlight);

        // highlight the cells to populate
        CellRange new_highlight(row1, col1, row2, col2);
        highlightCells(new_highlight);

        // find out the difference between the old cell highlight range
        // and the new one, and invalidate it
        std::vector<CellRange> highlight_difference;
        CellRange::Xor(new_highlight, old_highlight, highlight_difference);
        invalidateCellRange(highlight_difference);
    }
}

void CompTable::mouseMoveCells(IEventPtr evt)
{
    IMouseEventPtr mouse_evt = evt;

    if (startAction(mouse_evt, ACTION_MOVECELLS))
    {
        // if the mouse x or y position is negative, reset the action;
        // we're done; note: this is because the selection action looks 
        // to the negative area to select columns and we want that to 
        // have precedence in the event that the cursor row is on a row 
        // or column with index of 0
        if (m_mouse_x < 0 || m_mouse_y < 0)
        {
            resetAction();
            return;
        }    

        // if we're not on a move cell edge, reset the action;
        // we're done
        if (!isMoveCellEdge(m_mouse_x, m_mouse_y))
        {
            resetAction();
            return;
        }
        
        // create a pre move cell event; if the event is
        // vetoed, reset the action, and we're done
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_PRE_MOVE_CELL, this);
        dispatchEvent(notify_evt);
        
        if (!notify_evt->isAllowed())
        {
            resetAction();
            return;
        }
    }

    if (endAction(mouse_evt, ACTION_MOVECELLS))
    {
        // get the original selection and the cell higlight
        CellRange selection, highlight;
        getSelectionRange(selection);
        getCellHighlight(highlight);
        
        // remove the cell highlight
        removeCellHighlight();

        // if we didn't have a selection highlight, or the selection 
        // and the highlight are the same, we're done
        if (highlight.isEmpty() || selection == highlight)
        {
            invalidateSelection();
            return;
        }

        // if the control key was down at the start of the action,
        // copy the cells, otherwise move them
        if (m_mouse_ctrl_start)
            copyCells(selection, highlight, xcm::null);
        else
            moveCells(selection, highlight, xcm::null);
        
        // remove the previous selection and select the
        // newly copied cells
        removeAllCellSelections();
        selectCells(highlight);
        
        // set the cursor to the beginning of the new range
        setCursorPos(highlight.row1(), highlight.col1());

        // invalidate the table; we invalidate the table rather than 
        // the selection, because the copy/move operation may unmerge 
        // merged cells, and it's easier to invalidate the whole component 
        // than calculate which cells have been unmerged
        invalidate();
        
        // create a move cells end event
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_MOVE_CELL, this);
        dispatchEvent(notify_evt);
    }

    if (isAction(mouse_evt, ACTION_MOVECELLS))
    {
        // if the mouse hasn't moved more than the allotted tolerance
        // for determining the edge, we're done
        int x_diff = 0;
        int y_diff = 0;

        ICanvasPtr canvas = getCanvas();
        if (!canvas.isNull())
        {
            x_diff = canvas->mtod_x(abs(m_mouse_x_start - m_mouse_x));
            y_diff = canvas->mtod_y(abs(m_mouse_y_start - m_mouse_y));
        }

        if (x_diff <= TABLE_MOUSE_GRIP_TOL &&
            y_diff <= TABLE_MOUSE_GRIP_TOL &&
            !hasCellHighlight())
        {
            return;
        }

        // get the cells in the selection
        CellRange selection;
        getSelectionRange(selection);

        // get end row and column, which is the cell that the mouse
        // is currently over
        int row_end, col_end;
        getCellIdxByPos(m_mouse_x, m_mouse_y,
                        &row_end, &col_end,
                        false);

        // get the start row and column, which is the cell that's
        // clicked on inside the selection or if the cell is
        // outside the selection (because of the mouse tolerance),
        // the cell inside the selection that's nearest to the
        // cell that's clicked on; first get an initial start cell
        int row_start, col_start;
        getCellIdxByPos(m_mouse_x_start, m_mouse_y_start,
                        &row_start, &col_start,
                        false);
        
        CellRange start_cell(row_start, col_start);
        if (!start_cell.intersects(selection))
        {
            // if the start cell doesn't intersect the selection, 
            // expand the start cell by a single cell along
            // in the same column, row, and both and save each
            // as a new range to test
            CellRange range1 = start_cell;
            CellRange range2 = start_cell;
            CellRange range3 = start_cell;
            range1.expand(0, 1);
            range2.expand(1, 0);
            range3.expand(1, 1);
            
            // see if any of these three ranges intersect,
            // and if they do, the interesecting cell is the
            // starting cell
            int row1, col1, row2, col2;
            if (!range1.intersect(selection).isEmpty())
            {
                range1.get(&row1, &col1, &row2, &col2);
            }
            else if (!range2.intersect(selection).isEmpty())
            {
                range2.get(&row1, &col1, &row2, &col2);
            }
            else if (!range3.intersect(selection).isEmpty())
            {
                range3.get(&row1, &col1, &row2, &col2);
            }
            else
            {
                // shouldn't happen, but it if it does,
                // do nothing: our initial starting
                // row and column is good enough
                row1 = row_start;
                col1 = col_start;
            }

            row_start = row1;
            col_start = col1;
        }
            
        // get the selection position
        int sel_row1, sel_col1, sel_row2, sel_col2;
        selection.get(&sel_row1, &sel_col1, &sel_row2, &sel_col2);

        // clamp the row difference so that the moved cells so the
        // entire range of moved cells remains in the table boundaries
        int row_count = getRowCount();
        int col_count = getColumnCount();
        int row_diff = row_end - row_start;
        int col_diff = col_end - col_start;
        if (row_diff < 0 && -1*row_diff > sel_row1)
            row_diff = -1*sel_row1;
        if (col_diff < 0 && -1*col_diff > sel_col1)
            col_diff = -1*sel_col1;
        if (sel_row2 + row_diff >= row_count - 1)
            row_diff = row_count - sel_row2 - 1;
        if (sel_col2 + col_diff >= col_count - 1)
            col_diff = col_count - sel_col2 - 1;

        // if we already have a highlight, use it as our
        // starting highlight
        CellRange old_highlight;
        getCellHighlight(old_highlight);

        // calculate the new selection position and highlight it        
        CellRange new_highlight = selection;
        new_highlight.shift(row_diff, col_diff);
        highlightCells(new_highlight);

        // find out the difference between the old cell highlight range
        // and the new one, and invalidate it
        std::vector<CellRange> highlight_difference;
        CellRange::Xor(new_highlight, old_highlight, highlight_difference);
        invalidateCellRange(highlight_difference);
    }
}

void CompTable::mouseSelectCells(IEventPtr evt)
{
    IMouseEventPtr mouse_evt = evt;

    // note: in this action, we try to be careful to not select
    // the cursor cell when we don't need to; although it's not too
    // harmful to do so, it's nice to have clean selection areas
    // that don't have an unnecessary, secondary selection on the
    // cursor cell
    if (startAction(mouse_evt, ACTION_SELECTCELLS))
    {
        // create a pre select cell event; if the event
        // is vetoed, reset the action and we're done
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_PRE_SELECT_CELL, this);
        dispatchEvent(notify_evt);

        if (!notify_evt->isAllowed())
        {
            resetAction();
            return;
        }
    
        // set the mouse row and column from the mouse position
        int row, col;
        getCellIdxByPos(m_mouse_x, m_mouse_y, &row, &col);

        // if we allow row selections on the first column, set the 
        // column to -1 to trigger a row selection
        if (isPreference(prefSelectRows) && col <= 0)
            col = -1;

        // if we allow column selections on the first row, set
        // the column to -1 to trigger a column selection
        if (isPreference(prefSelectColumns) && row <= 0)
            row = -1;

        m_mouse_row = row;
        m_mouse_col = col;
        m_mouse_row_start = row;
        m_mouse_col_start = col;

        // determine the new cursor position; if the whole row or column
        // is selected, move the cursor to the top of the row or column
        // respectively
        int cursor_row = (m_mouse_row == -1 ? 0 : m_mouse_row);
        int cursor_col = (m_mouse_col == -1 ? 0 : m_mouse_col);
        
        // if the new cursor position is in a merged cell and we have a
        // row or column selection, try to move out of the cell
        CellRange cursor(cursor_row, cursor_col);
        if (isMerged(cursor) && (m_mouse_row == -1 || m_mouse_col == -1))
        {
            int row_diff = (m_mouse_row == -1) ? 1 : 0;
            int col_diff = (m_mouse_col == -1) ? 1 : 0;
            findNextUnmergedCell(cursor_row, cursor_col, &row_diff, &col_diff);
            cursor_row += row_diff;
            cursor_col += col_diff;
        }

        // invalidate the current selection
        invalidateSelection();

        // if the control and shift keys aren't down or they are both down,
        // clear any selections and set the new cursor position; if we're
        // on a range marker, select the new range of cells
        if (!(m_mouse_ctrl_start || m_mouse_shift_start) ||
             (m_mouse_ctrl_start && m_mouse_shift_start))
        {
            // dispatch an event to indicate we're about to remove
            // selections
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_TABLE_PRE_SELECTION_CLEAR, this);
            dispatchEvent(notify_evt);
            
            // if the clear selection event is allowed, remove the cell
            // selections
            if (notify_evt->isAllowed())
                removeAllCellSelections();

            // set the cursor position regardless of whether or not
            // the selections are removed; note: call setCursorPos()
            // immediately after removeAllCellSelections so the two
            // form a single operation from the standpoint of the
            // notify events that begin and end the operation
            setCursorPos(cursor_row, cursor_col);

            // if the clear selection event is allowed, dispatch a
            // table selection cleared event
            if (notify_evt->isAllowed())
            {
                INotifyEventPtr notify_evt;
                notify_evt = NotifyEvent::create(EVENT_TABLE_SELECTION_CLEAR, this);
                dispatchEvent(notify_evt);
            }

            // if we're on a range marker, set the appropriate range
            if (m_mouse_row == -1 || m_mouse_col == -1)
                selectCells(CellRange(m_mouse_row, m_mouse_col));
        }

        // if the control key is down, and the cell isn't currently
        // selected apart from the cursor, then add a selection at 
        // the cursor cell before we move the cursor
        if (m_mouse_ctrl_start)
        {
            // select the cursor cell if it isn't already selected
            if (!isCellSelected(m_cursor_row, m_cursor_col, false))
                 selectCells(CellRange(m_cursor_row, m_cursor_col));
        
            // move the cursor to the new position
            setCursorPos(cursor_row, cursor_col);

            // if we're on a range marker, select the range of cells
            if (m_mouse_row == -1 || m_mouse_col == -1)
                selectCells(CellRange(m_mouse_row, m_mouse_col));

            // temporarily select the cursor cell; this selection
            // marks the starting point of the move selection and
            // will be deleted when the mouse moves to another cell,
            // creating a new selection, or when the mouse is released
            // from this cell, leaving the cursor free of other
            // selections
            selectCells(CellRange(cursor_row, cursor_col));
        }

        // if the shift key is down, select the area from the old 
        // cursor position to the new position
        if (m_mouse_shift_start)
        {
            // if the cursor cell is selected apart from the
            // cursor, remove the last selection
            if (isCellSelected(m_cursor_row, m_cursor_col, false))
                removeLastSelection();
            
            // temporarily select a new range of cells; this will selection
            // marks the starting point of the move selection and will be
            // deleted when the mouse moves to another cell, createing a
            // new selection, or when the mouse is released from this cell,
            // leaving the cursor cell free of other selections
            selectCells(CellRange(m_cursor_row, m_cursor_col, 
                                  m_mouse_row, m_mouse_col));
        }

        // create a selecting cell event
        INotifyEventPtr notify_selection_evt;
        notify_selection_evt = NotifyEvent::create(EVENT_TABLE_SELECTING_CELL, this);
        dispatchEvent(notify_selection_evt);

        // invalidate the new selection
        invalidateSelection();
    }

    if (endAction(mouse_evt, ACTION_SELECTCELLS))
    {
        // if the last selection area is something other than the
        // cursor cell, leave it; otherwise remove it    
        if (m_cell_selections.size() > 0)
        {
            int row1, col1, row2, col2;
            m_cell_selections.back().getRange(&row1, &col1, &row2, &col2);
            
            // note: following code will get called with multiple selection
            // areas when holding the control key down
            if (row1 == row2 && col1 == col2 && row1 != -1 && col1 != -1)
                removeLastSelection();
        }

        // set the selection index to -1 to indicate we need to
        // calculate the selection index in findNextCellSelection()
        m_cursor_selection_idx = -1;

        // if we don't have any actual selection areas, we're simply clicking 
        // on a cell, so dispatch a cell clicked event; TODO: have to figure 
        // out a better system for this: right clicks? middle-clicks? what about 
        // sending the cell idx so we don't have to rely on the cursor position?
        if (m_cell_selections.empty())
        {
            // we've simply left-clicked on a cell, so create a table left-cell 
            // clicked event
            INotifyEventPtr notify_evt;
            notify_evt = NotifyEvent::create(EVENT_TABLE_CELL_LEFT_CLICK, this);
            dispatchEvent(notify_evt);
        }

        // create a select cells end event
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_SELECT_CELL, this);
        dispatchEvent(notify_evt);
    }

    if (isAction(mouse_evt, ACTION_SELECTCELLS))
    {
        // get the current cell position
        int row, col;
        getCellIdxByPos(m_mouse_x, m_mouse_y, &row, &col);

        // if we're in row selection mode, remain in it
        if (m_mouse_col_start == -1)
            col = -1;

        // if we're in column selection mode, remain in it
        if (m_mouse_row_start == -1)
            row = -1;

        // if the mouse is still on the same cell, we're done
        if (row == m_mouse_row && col == m_mouse_col)
            return;

        // reset the current mouse row and column
        m_mouse_row = row;
        m_mouse_col = col;

        // get the last selection
        CellRange old_selection;
        getLastSelection(old_selection);

        // if we didn't start with a range selection (initial
        // mouse row or mouse column < 0), don't allow one now;
        // this is to prevent rows and columns from being selected 
        // when the initial selection started on a table cell and 
        // the mouse then moves past the beginning of the row or 
        // column
        if (m_mouse_row_start >= 0 && m_mouse_row < 0)
            m_mouse_row = 0;
            
        if (m_mouse_col_start >= 0 && m_mouse_col < 0)
            m_mouse_col = 0;

        // remove the last selection; if the control and shift key
        // aren't down, this won't remove anything; if either of
        // them are down, this removes the temporary selection we
        // created and allows us to begin the selection of the range
        // of cells
        removeLastSelection();
        
        // select the new range of cells
        selectCells(CellRange(m_cursor_row, m_cursor_col, 
                              m_mouse_row, m_mouse_col));

        // create a selecting cell event
        INotifyEventPtr notify_evt;
        notify_evt = NotifyEvent::create(EVENT_TABLE_SELECTING_CELL, this);
        dispatchEvent(notify_evt);

        // because of merged cells, the last selection may have been
        // expanded to accomodate them (unless the selection is a row
        // or column selection: see below), so use it rather than the
        // the range we selected to find out which cells have changed
        CellRange new_selection;
        getLastSelection(new_selection);

        // find out the difference between the two selections and 
        // invalidate it
        std::vector<CellRange> selection_difference;
        CellRange::Xor(new_selection, old_selection, selection_difference);
        invalidateCellRange(selection_difference);
        
        // in addition, we have one special case we have to handle 
        // directly: merged cells that intersect a row or column 
        // selection; the selection doesn't automatically expand to 
        // include these since the row or column selection passes 
        // through the merged cell; so, invalidate any merged cells 
        // that intersect the last row or column selection
        if (new_selection.isRow() || new_selection.isColumn())
        {
            std::vector<CellRange> merged_cells;
            m_model->getMergedCells(merged_cells);

            std::vector<CellRange>::iterator it, it_end;
            it_end = merged_cells.end();
            
            for (it = merged_cells.begin(); it != it_end; ++it)
            {
                if (new_selection.intersects(*it))
                    invalidateCellRange(*it);
            }
        }
    }
}

bool CompTable::startAction(IEventPtr evt, wxString action)
{
    IMouseEventPtr mouse_evt = evt;
    if (mouse_evt.isNull())
        return false;

    // if the event is a left-mouse button down and we
    // don't already have an action, set the action and 
    // return true
    if (evt->getName() == EVENT_MOUSE_LEFT_DOWN &&
        m_mouse_action == ACTION_NONE)
    {
        // set the mouse action
        m_mouse_action = action;

        // set the key state at the start of the event
        m_mouse_alt_start = mouse_evt->isAltDown();
        m_mouse_ctrl_start = mouse_evt->isCtrlDown();
        m_mouse_shift_start = mouse_evt->isShiftDown();

        // set the starting mouse position
        m_mouse_x_start = m_mouse_x;
        m_mouse_y_start = m_mouse_y;

        return true;
    }
    
    // otherwise, return false
    return false;
}

bool CompTable::endAction(IEventPtr evt, wxString action)
{
    // if the event is a left-mouse button up and we
    // already have an action that matches the input 
    // action, reset the action and return true
    if (evt->getName() == EVENT_MOUSE_LEFT_UP &&
        m_mouse_action == action)
    {
        m_mouse_action = ACTION_NONE;
        return true;
    }
    
    // otherwise, return false
    return false;
}

bool CompTable::isAction(IEventPtr evt, wxString action)
{
    // if the event is a mouse move event and we
    // already have an action that matches the input 
    // action, return true
    if (evt->getName() == EVENT_MOUSE_MOTION &&
        m_mouse_action == action)
    {
        return true;
    }

    // otherwise, return false
    return false;
}

void CompTable::resetAction()
{
    // reset the mouse action state; this is used to bail out
    // of an action if some condition in the start action isn't 
    // true, so other handlers have a chance to process the event
    m_mouse_action = ACTION_NONE;
}

inline void getCellRenderOrder(CompTable* table, 
                               const CellRange& range,
                               std::vector<CellRect>& cells)
{
    // note: this function determines the order in which to draw the cells
    // in the given range, as well as their dimensions and clipping regions;
    // the reason we need this function is so we can draw the cells without 
    // text before we draw the cells with text so the text can flow outside 
    // the cell and not be drawn over when other cells are rendered; the order 
    // to draw the cells by row, then within a row, 1) non-merged cells without 
    // text, 2) non-merged cells with text, and 3) merged cells (without regard 
    // to whether or not it has text since text for merged cells is always 
    // clipped and therefore doesn't overlap other cells); using this order, 
    // text will flow horizontally over other cells that don't have text and 
    // are unmerged, but will be clipped by other rows or by other cells that 
    // have text or are merged

    // get the range of cells to order; range markers should already be
    // accounted for in input range
    int row_start, col_start, row_end, col_end;
    range.get(&row_start, &col_start, &row_end, &col_end);

    // clear the cells and reserve space
    cells.clear();
    cells.reserve((row_end - row_start + 1)*(col_end - col_start + 1));

    // temporary vector to hold the indexes of the text columns;
    // reserve space for the maximum number of to render in a row,
    // which is the number of columns plus one extra column on each 
    // side of the range
    std::vector<int> text_columns;
    text_columns.reserve(col_end - col_start + 3);

    // get the text properties for all the rows we're rendering;
    // note: we have to get the text properties for all the columns, 
    // not just those we're rendering so that we can include the
    // text of cells that may overflow into the visible area we're 
    // rendering even though they are outside it
    std::vector<CellProperties> text_properties;
    CellRange range_rows(row_start, 0, row_end, table->getColumnCount() - 1);
    table->getCellProperty(range_rows, PROP_CONTENT_VALUE, text_properties);

    // iterate through the rows
    int row;
    for (row = row_start; row <= row_end; ++row)
    {
        // first, get the row position for use throughout
        int row_pos = table->getRowPosByIdx(row);
        int row_size = table->getRowSize(row);

        // iterator for traversing columns
        std::vector<int>::iterator it, it_end;
    
        // clear the text and non-text columns
        text_columns.clear();

        // for the cells in the given row, find the cells
        // that have text
        CellRange range_row(row, col_start, row, col_end);
        std::vector<CellProperties>::reverse_iterator itr = text_properties.rbegin();
        getTextColumns(range_row, text_properties, itr, text_columns);
        
        // add on the first cell with text before and after the range
        // of cells in the row
        int idx_start, idx_end;

        int c1a = 0;
        int c2a = col_start - 1;

        if (c1a <= c2a)
        {
            itr = text_properties.rbegin();
            CellRange range_before(row, c1a, row, c2a);
            if (getBoundingTextColumnIdx(range_before, text_properties, itr, &idx_start, true))
                text_columns.push_back(idx_start);
        }
        
        int c1b = col_end + 1;
        int c2b = table->getColumnCount() - 1;

        if (c1b <= c2b)
        {
            itr = text_properties.rbegin();
            CellRange range_after(row, c1b, row, c2b);
            if (getBoundingTextColumnIdx(range_after, text_properties, itr, &idx_end, false))
                text_columns.push_back(idx_end);
        }

        // sort the text column list
        std::sort(text_columns.begin(), text_columns.end());

        // find the non-text columns
        std::vector<int> columns, non_text_columns;
        columns.reserve(col_end - col_start + 1);
        non_text_columns.reserve(col_end - col_start + 1);

        int c;
        for (c = col_start; c <= col_end; ++c)
        {
            columns.push_back(c);
        }
        
        std::set_difference(columns.begin(), columns.end(),
                            text_columns.begin(), text_columns.end(),
                            std::back_inserter(non_text_columns));

        // iterate through the non-text columns and add the output
        it_end = non_text_columns.end();
        for (it = non_text_columns.begin(); it != it_end; ++it)
        {
            // if the cell is a merged cell, move on
            CellRange c(row, *it);
            if (table->isMerged(c))
                continue;

            // if the cell isn't merged, fill out the cell dimension info,
            // which is relative to the table origin
            CellRect output_cell;
            output_cell.m_row = row;
            output_cell.m_col = *it;

            int col_pos = table->getColumnPosByIdx(output_cell.m_col);
            int col_size = table->getColumnSize(output_cell.m_col);
            
            output_cell.m_rect = wxRect(col_pos,
                                        row_pos,
                                        col_size,
                                        row_size);

            // for non-text cells, the content and clipping rectangles 
            // are the same as the cell rectangle; note: the content and
            // clipping rectangles are relative to the cell origin, not 
            // the table origin
            output_cell.m_content_rect = wxRect(0, 0, col_size, row_size);
            output_cell.m_clipping_rect = wxRect(0, 0, col_size, row_size);

            // add the cell to the list of output cells
            cells.push_back(output_cell);
        }

        // iterate through the text columns and add the output        
        int clip_x1 = 0;
        int clip_x2 = 0;

        it_end = text_columns.end();
        for (it = text_columns.begin(); it != it_end; ++it)
        {
            // find the initial x clipping position; the x start position 
            // of the first clipping region is the minimum of the first text 
            // column position or the first cell column position; this ensures 
            // that all the text in the first cell draws, but prevents the text 
            // from flowing over into other cells that may have text in them;
            // this later scenario isn't an issue when drawing the whole table, 
            // but makes a difference when only drawing parts of the table, 
            // such as when rendering a selection area
            if (it == text_columns.begin())
            {
                int min_idx = *it < col_start ? *it : col_start;
                clip_x1 = table->getColumnPosByIdx(min_idx);
            }

            // if the cell is a merged cell, move on
            CellRange c(row, *it);
            if (table->isMerged(c))
                continue;

            // TODO: clip text when merged cells are in non-text area
            // between text cells

            // if the cell isn't merged, fill out the cell dimension info,
            // which is relative to the table origin
            CellRect output_cell;
            output_cell.m_row = row;
            output_cell.m_col = *it;

            int col_pos = table->getColumnPosByIdx(output_cell.m_col);
            int col_size = table->getColumnSize(output_cell.m_col);
            
            output_cell.m_rect = wxRect(col_pos,
                                        row_pos,
                                        col_size,
                                        row_size);

            // set the initial content rectangle to the same as the cell
            // rectangle, except in coordinates local to the cell, not
            // the table; it's atual size will be set when we render the
            // cell
            output_cell.m_content_rect = wxRect(0, 0, col_size, row_size);
        
            // fill out the clipping info; the region to clip is the end of 
            // the previous text column to the beginning of the next text 
            // column along the horizontal, and the start of the row to the 
            // end of the row along the vertical, which in local coordinates 
            // is zero to the height of the row; exception: if we're on the 
            // last cell with text, the x end position of the last clipping 
            // region is either the end of the text cell or the end of the
            // last column, whichever is greater; this logic parallels the
            // logic for the initial clipping position above
            if (it < text_columns.end() - 1)
            {
                clip_x2 = table->getColumnPosByIdx(*(it + 1));
            }
            else
            {
                int last_idx = text_columns.back();
                int max_idx = last_idx > col_end ? last_idx : col_end;
                clip_x2 = table->getColumnPosByIdx(max_idx) + 
                          table->getColumnSize(max_idx);
            }

            // make sure the clipping info is the relative to the cell origin
            output_cell.m_clipping_rect = wxRect(clip_x1 - col_pos,
                                                 0,
                                                 clip_x2 - clip_x1,
                                                 row_size);

            // set the new clipping region starting position, which is
            // the end of the current cell
            clip_x1 = col_pos + col_size;

            // add the cell to the list of output cells            
            cells.push_back(output_cell);
        }
    }

    // up to this point, we haven't added any of the merged cells
    // to the list of cells of output cells; now, add the merged 
    // cells to the output cells
    std::vector<CellRange> cell_merges;
    table->getMergedCells(cell_merges);
    
    std::vector<CellRange>::iterator it_merged, it_merged_end;
    it_merged_end = cell_merges.end();

    CellRange range_table(0, 0, table->getRowCount() - 1, table->getColumnCount() - 1);
    for (it_merged = cell_merges.begin(); it_merged != it_merged_end; ++it_merged)
    {
        // if the merged cell doesn't intersect the range of cells
        // we're ordering, move on
        CellRange range_merged = *it_merged;
        if (!range.intersects(range_merged))
            continue;

        // account for range markers by limiting the merged cell
        // range to the range of the table
        range_merged.intersect(range_table);

        int row1 = range_merged.row1();
        int col1 = range_merged.col1();
        int row2 = range_merged.row2();
        int col2 = range_merged.col2();

        int x, y, w, h;
        x = table->getColumnPosByIdx(col1);
        y = table->getRowPosByIdx(row1);
        w = table->getColumnPosByIdx(col2) + table->getColumnSize(col2) - x;
        h = table->getRowPosByIdx(row2) + table->getRowSize(row2) - y;

        CellRect cell;
        cell.m_row = row1;
        cell.m_col = col1;
        cell.m_rect = wxRect(x, y, w, h);
        cell.m_content_rect = wxRect(0, 0, w, h);
        cell.m_clipping_rect = wxRect(0, 0, w, h);

        // save the cell
        cells.push_back(cell);
    }
}

void CompTable::renderContent(const wxRect& update_rect)
{
    // if we don't have a valid canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // determine whether or not to draw the gridlines
    bool horz_gridlines = isPreference(prefHorzGridLines);
    bool vert_gridlines = isPreference(prefVertGridLines);

    // get the cell range to update; note: because we're drawing the 
    // merged cells separately and testing them explicity to see if 
    // they intersect the following range, we need only get the range 
    // of cells that correspond to the non-merged cells; this is more 
    // efficient than getting the range of cells that contains the merged 
    // cells, since this range may contain additional non-merged cells
    // that are entirely outside the update rectangle, simply to 
    // accomodate the merged cells
    CellRange range;
    getCellRangeIdx(update_rect, range, false);

    // get the cell content to render and the order
    // in which to render it
    std::vector<CellRect> cells;
    getCellRenderOrder(this, range, cells);

    std::vector<CellRect>::iterator it, it_end;
    it_end = cells.end();

    // render the cell grid lines and content
    for (it = cells.begin(); it != it_end; ++it)
    {
        // if we're editing and on the cursor cell, we'll render this
        // later, so move on; this is to ensure that the contents of
        // the cursor cell don't spill over into other cells after
        // the value in the editor has changed, but hasn't yet been 
        // accepted
        if (isEditing() && isCursorCell(it->m_row, it->m_col))
            continue;

        // put the cell properties in the renderer
        putCellPropertiesInRenderer(it->m_row, it->m_col);

        // set the content rectangle for the cells; currently, it's
        // set to the boundaries of the cell; if the content of the 
        // cell is larger than the boundaries, adjust the content 
        // rectangle to the larger size
        wxRect content_rect;
        m_renderer->extends(content_rect);
        it->m_content_rect.Union(content_rect);

        // if we're printing, or not drawing all the grid lines, set the
        // foreground color to the same as the background color; we'll
        // then draw the lines manually; note: now that we have transparency, 
        // we can set the foreground color to be transparent, but we need 
        // to fix the transparency settings in the PDF canvas

        // TODO: for now do it this way, probably can draw this a single 
        // rectangle on top of cells to save time; have to figure out how 
        // it would interact with cursor etc, but may save time
        PropertyValue value;
        m_renderer->getProperty(PROP_COLOR_FG, value);
        Color grid_line_color = value.getColor();

        if (canvas->isPrinting() || !horz_gridlines || !vert_gridlines)
        {
            // if we're printing or the grid lines are off, then get rid of 
            // the cell lines by setting the foreground color to be the same 
            // as the background
            PropertyValue value;
            m_renderer->getProperty(PROP_COLOR_BG, value);
            m_renderer->setProperty(PROP_COLOR_FG, value);
        }

        // change the color of the cell to the selection color if we're
        // not printing, and the cell is part of an actual selection range
        // (i.e., a non-cursor cell); however, do not color the cell if the
        // cursor itself is visible and its in this selection range
        if (!canvas->isPrinting() &&
             isCellSelected(it->m_row, it->m_col, false) &&
             (!isCursorCell(it->m_row, it->m_col) || !isCursorVisible()))
        {
            // create a blended color from the background color of
            // the renderer with the selection background color to
            // create a transparent effect; use the default highlight
            // color, but use a preference if it's specified
            Color color_highlight = COLOR_HIGHLIGHT_BG;
            kcanvas::PropertyValue prop_color_highlight;
            if (canvas->getProperty(PROP_COLOR_HIGHLIGHT, prop_color_highlight))
                color_highlight = prop_color_highlight.getColor();            
            
            PropertyValue value;
            m_renderer->getProperty(PROP_COLOR_BG, value);
            
            Color bg = stepColor(color_highlight, value.getColor(), 30);
            m_renderer->setProperty(PROP_COLOR_BG, bg);
        }

        // move the draw origin into position
        canvas->moveDrawOrigin(it->m_rect.x, it->m_rect.y);

        // set the size of the cell
        m_renderer->setSize(it->m_rect.width, it->m_rect.height);

        // if the content of the cell extends beyond the calculated
        // clipping rectangle, add a clipping rectangle to limit the 
        // content
        bool clipped = false;
        if (!it->m_clipping_rect.Contains(it->m_content_rect))
        {
            clipped = true;
            canvas->addClippingRectangle(it->m_clipping_rect.x,
                                         it->m_clipping_rect.y,
                                         it->m_clipping_rect.width,
                                         it->m_clipping_rect.height);
        }

        // render the cell
        m_renderer->render(update_rect);

        // if either the horizontal gridlines or the vertical gridlines
        // are showing, but not both, draw the horizontal or vertical
        // gridlines; the reason we don't draw it if both are showing
        // is because the renderer takes care of it; TODO: as noted
        // above, this should be streamlined
        
        // TODO: right now, these are drawn on top of the text, whereas
        // when both gridlines are active, it's drawn by the renderer,
        // and therefore is below the text; this is obviously inconsistent,
        // and should be fixed; however, it works for the way we're 
        // currently using horz and vert gridlines
        if (horz_gridlines && !vert_gridlines)
        {
            canvas->setPen(grid_line_color);
            canvas->drawLine(0, 0, it->m_rect.width, 0);
            canvas->drawLine(0, it->m_rect.height, it->m_rect.width, it->m_rect.height);
        }

        if (vert_gridlines && !horz_gridlines)
        {
            canvas->setPen(grid_line_color);
            canvas->drawLine(0, 0, 0, it->m_rect.height);
            canvas->drawLine(it->m_rect.width, 0, it->m_rect.width, it->m_rect.height);
        }

        // if we added a clipping rectangle, remove it
        if (clipped)
            canvas->removeClippingRectangle();

        // move the draw origin back
        canvas->moveDrawOrigin(-it->m_rect.x, -it->m_rect.y);
    }
}

void CompTable::renderBorders(const wxRect& update_rect)
{
    // if we don't have a valid canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    std::map< std::pair<int,int> , CellBorder > horz_borders;
    std::map< std::pair<int,int> , CellBorder > vert_borders;
    std::map< std::pair<int,int> ,CellBorder>::iterator itb, itb_end;

    // get the cell range to update
    CellRange range;
    getCellRangeIdx(update_rect, range, false);

    // get all cell properties that intersect the input range
    std::vector<CellProperties> cell_properties;
    getCellProperties(range, cell_properties);

    std::vector<CellProperties>::iterator it, it_end;
    it_end = cell_properties.end();
    
    for (it = cell_properties.begin(); it != it_end; ++it)
    {
        putBorderPropertiesInMap(this, *it, horz_borders, vert_borders);
    }

    // render the horizontal borders
    itb_end = horz_borders.end();
    for (itb = horz_borders.begin(); itb != itb_end; ++itb)
    {
        CellBorder border;
        border = itb->second;

        Properties props;
        props.add(PROP_COLOR_FG, border.m_color);
        props.add(PROP_LINE_WIDTH, border.m_width);
        canvas->setPen(props);
        canvas->drawLine(border.m_x1, border.m_y1, border.m_x2, border.m_y2);
    }

    // render the vertical borders
    itb_end = vert_borders.end();
    for (itb = vert_borders.begin(); itb != itb_end; ++itb)
    {
        CellBorder border;
        border = itb->second;

        Properties props;
        props.add(PROP_COLOR_FG, border.m_color);
        props.add(PROP_LINE_WIDTH, border.m_width);
        canvas->setPen(props);
        canvas->drawLine(border.m_x1, border.m_y1, border.m_x2, border.m_y2);
    }
}

void CompTable::renderCursor(const wxRect& update_rect)
{
    // if we don't have a valid canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // if we're printing, we're done
    if (canvas->isPrinting())
        return;

    // if the cursor is outside the range of the table,
    // don't draw it
    if (m_cursor_row < 0 || m_cursor_row >= getRowCount())
        return;
        
    if (m_cursor_col < 0 || m_cursor_col >= getColumnCount())
        return;

    // if the cursor isn't visible or we don't have
    // the cursor, we're done
    if (!isCursorVisible())
        return;

    // get the position and size of the cursor
    // render the cursor
    int cursor_x, cursor_y, cursor_w, cursor_h;
    getCellPosByIdx(m_cursor_row, m_cursor_col, &cursor_x, &cursor_y);
    getCellSizeByIdx(m_cursor_row, m_cursor_col, &cursor_w, &cursor_h);

    // render the cursor
    canvas->setPen(Pen(COLOR_BLACK));
    canvas->setBrush(Brush(COLOR_BLACK, wxTRANSPARENT));
    canvas->drawRectangle(cursor_x, cursor_y, cursor_w, cursor_h);
}

void CompTable::renderSelectionRect(const wxRect& update_rect)
{
    // if we don't have a valid canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // if we're printing, we're done
    if (canvas->isPrinting())
        return;

    PropertyValue value;

    // if the cursor isn't visible, we're done
    if (!isCursorVisible())
        return;

    // get selection rectangle
    wxRect sel_rect;
    getSelectionRangePos(sel_rect);

    // if the selection rectangle is empty or it doesn't
    // intersect the update rectangle, we're done
    if (sel_rect.IsEmpty() || !sel_rect.Intersects(update_rect))
        return;

    // if the move cells preference is set and we have
    // a continuous selection, then draw the grip
    if (isPreference(prefMoveCells) && 
        isSelectionContinuous())
    {
        // draw the selection line
        Pen pen(COLOR_BLACK, m_selection_line_width, wxPENSTYLE_SOLID);
        pen.setCap(wxCAP_BUTT);
        pen.setJoin(wxJOIN_MITER);
        canvas->setPen(pen);
        
        Brush brush(COLOR_BLACK, wxBRUSHSTYLE_TRANSPARENT);
        canvas->setBrush(brush);

        canvas->drawRectangle(sel_rect.x,
                              sel_rect.y,
                              sel_rect.width,
                              sel_rect.height);
    }

    // if the populate cells preference is set and we have 
    // a continuous selection, draw the selection gripper
    if (isPreference(prefPopulateCells) && 
        isSelectionContinuous())
    {
        // draw the gripper
        Pen gripper_pen;
        gripper_pen.setColor(COLOR_WHITE);

        Brush gripper_brush;
        gripper_brush.setColor(COLOR_BLACK);
        
        canvas->setPen(gripper_pen);
        canvas->setBrush(gripper_brush);

        int adj = 1;
        if (m_selection_line_width == 3)
            adj = 0;

        int offset = canvas->dtom_x(adj);
        int g = canvas->dtom_x(m_selection_line_width);
        int gripper_x = sel_rect.x + sel_rect.width;
        int gripper_y = sel_rect.y + sel_rect.height;
        canvas->drawRectangle(gripper_x - g - offset,
                              gripper_y - g - offset,
                              2*g + offset,
                              2*g + offset);
    }
}

void CompTable::renderHighlight(const wxRect& update_rect)
{
    // if we don't have a valid canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // if we're printing, we're done
    if (canvas->isPrinting())
        return;

    // if we don't have a highlight, we're done
    if (!hasCellHighlight())
        return;
        
    // get the highlight rectangle
    wxRect highlight_rect;
    getCellHighlightPos(highlight_rect);

    // draw the highlight rectangle
    Pen pen(COLOR_BLUE, m_selection_line_width, wxPENSTYLE_SOLID);
    pen.setCap(wxCAP_BUTT);
    pen.setJoin(wxJOIN_MITER);
    canvas->setPen(pen);
    
    Brush brush(COLOR_BLACK, wxBRUSHSTYLE_TRANSPARENT);
    canvas->setBrush(brush);

    canvas->drawRectangle(highlight_rect.x,
                          highlight_rect.y,
                          highlight_rect.width,
                          highlight_rect.height);
}

void CompTable::renderEditor(const wxRect& update_rect)
{
    // if we don't have a valid Canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // if we're printing, we're done
    if (canvas->isPrinting())
        return;

    // if we're not editing, we're done
    if (!isEditing())
        return;

    // get the position of the editor
    int editor_x, editor_y;
    m_editor->getOrigin(&editor_x, &editor_y);
    
    // move to the position of the editor and render it
    canvas->moveDrawOrigin(editor_x, editor_y);
    
    // add a clipping rectangle; make it one pixel smaller than the
    // cell size so the editor doesn't draw over the cell lines
    int tol = canvas->dtom_x(1);
    int clip_w, clip_h;
    m_editor->getSize(&clip_w, &clip_h);
    canvas->addClippingRectangle(tol, tol, clip_w - 2*tol, clip_h - 2*tol);
    
    // render the editor
    m_editor->render(update_rect);
    
    // remove the clipping rectangle
    canvas->removeClippingRectangle();
    
    // move the draw origin back again
    canvas->moveDrawOrigin(-editor_x, -editor_y);
}

void CompTable::putCellPropertiesInRenderer(int row, int col)
{
    // note: cell ranges can have different renderers, so we 
    // evaluate the properties on the cell, then look at those
    // properties to find the renderer type; once we have the
    // renderer type, we set the renderer and load the properties
    // into the renderer

    // evaluate the model the given row and column
    Properties props;
    m_model->eval(row, col, props);

    // add the row and column properties
    props.add(PROP_CELL_ROW, row);
    props.add(PROP_CELL_COLUMN, col);

    // get the mime type for the current cell
    PropertyValue value;
    props.get(PROP_CONTENT_MIMETYPE, value);

    // TODO: for now, manually associate a MIME type with
    // a renderer; should allow this to be set in the API
    // when a renderer is added, so that a user can set
    // which renderer is to be used for a given MIME type
    
    wxString renderer_id = COMP_TYPE_TEXTBOX;
    wxString mime_type = value.getString();
    
    if (mime_type == wxT("plain/text"))
        renderer_id = COMP_TYPE_TEXTBOX;
    if (mime_type == wxT("image/png"))
        renderer_id = COMP_TYPE_IMAGE;
    
    // TODO: could simply the following logic by storing the mime
    // type along with the renderer list; then we just iterate in
    // order through the list and find the first one that has
    // the apprpriate mime type handler
    
    // set the renderer
    std::map<wxString,IComponentPtr>::iterator it_r;
    it_r = m_renderer_list.find(renderer_id);
    
    if (it_r != m_renderer_list.end())
    {
        m_renderer = it_r->second;
    }
    else
    {
        // TODO: it would be nice to solidify the default handling in the event
        // of a bad property; we specify defualts in the constructor, but this
        // default value assumes the default value in the constructor is the
        // same; if it isn't we may still not have a valid renderer, in which
        // case the program will crash
        m_renderer = m_renderer_list[COMP_TYPE_TEXTBOX];
    }

    // update the renderer canvas member
    Component* ptr_comp = static_cast<Component*>(m_renderer.p);
    ptr_comp->setCanvas(getCanvas());

/*  
    // set the renderer properties; we use set properties so that only renderers
    // have a chance to see if any of there properties are changed, so that if
    // they have a cache, they can clear it; if we use addProperties(), the
    // renderer has to assume the properties are different because there's no
    // check, and as a result, if they keep a cache, they are always forced to
    // clear the cache; that said, right now, there's still no advatange to using 
    // setProperties() since the properties will always be different because we're 
    // setting the cell row and column property for each cell; however, in the future, 
    // we may have renderers for invidual cells that have lengthy calculations, in 
    // which case, the cell row and column won't change and using setProperties() 
    // will be more efficient
    m_renderer->setProperties(props);
*/
    // TODO: the above comment is the old thinking; however, it's very desirable to
    // add additional cell properties on the table that get passed to the renderer
    // and then use the properties in a callback that's hooked up to the renderer;
    // in this case setProperties() will not allow these additional properties to come 
    // in unless they already exist, which becomes cumbersome because every property
    // we want to pass from the cell properties has to be added first to the renderer
    
    // note: equivalent logic in beginEdit() for editor, so whatever we do here, we
    // should do there
    m_renderer->addProperties(props);
}

void CompTable::positionEditor(int row, int col)
{
    // note: this function sets the editor dimensions based on
    // the input row and column

    // get the position and size of the row and column
    wxRect editor_size;
    getCellPosByIdx(row, col, &editor_size.x, &editor_size.y);
    getCellSizeByIdx(row, col, &editor_size.width, &editor_size.height);

    // set an initial position and size for the editor
    m_editor->setOrigin(editor_size.x, editor_size.y);
    m_editor->setSize(editor_size.width, editor_size.height);

    // get the extent of the editor, which may be different
    // than the size of the editor if the content of the area,
    // such as text, exceeds the size
    wxRect editor_extent(0,0,0,0);
    m_editor->extends(editor_extent);
    
    // the extent is in the editor's coordinate system, so add the editor 
    // position onto the editor extent so that the editor extent is in 
    // table coordinates
    editor_extent.x += editor_size.x;
    editor_extent.y += editor_size.y;
    
    // find the bounding box of the editor size and extent
    editor_extent.Union(editor_size);

    // get the column and row indexes at the borders of the extent
    // of the editor
    int x1_idx = getColumnIdxByPos(editor_extent.x);
    int y1_idx = getRowIdxByPos(editor_extent.y);
    int x2_idx = getColumnIdxByPos(editor_extent.x + editor_extent.width);
    int y2_idx = getRowIdxByPos(editor_extent.y + editor_extent.height);

    // get the minimum and maximum positions of the cell range
    // corresponding to the borders of the editor extent
    int x1_min = getColumnPosByIdx(x1_idx);
    int y1_min = getRowPosByIdx(y1_idx);
    int x2_max = getColumnPosByIdx(x2_idx) + getColumnSize(x2_idx);
    int y2_max = getRowPosByIdx(y2_idx) + getRowSize(y2_idx);
    
    // make sure the the minimum and maximum positions of the cell
    // range aren't less than the extent in each direction
    x1_min = wxMin(x1_min, editor_extent.x);
    y1_min = wxMin(y1_min, editor_extent.y);
    x2_max = wxMax(x2_max, editor_extent.x + editor_extent.width);
    y2_max = wxMax(y2_max, editor_extent.y + editor_extent.height);

    // determine the position of the editor; use the initial position
    // as the default, and change the coordinates when the contents
    // extend outside the boundary
    int x1 = editor_size.x;
    int y1 = editor_size.y;
    int x2 = editor_size.width + x1;
    int y2 = editor_size.height + y1;

    // horizontal contents extend out the right, but not the left
    if (editor_extent.x >= editor_size.x &&
        (editor_extent.x + editor_extent.width) > 
         (editor_size.x + editor_size.width))
    {
        x2 = x2_max;
    }
    
    // horizontal contents extend out the left, but not the right
    if (editor_extent.x < editor_size.x &&
        (editor_extent.x + editor_extent.width) <= 
         (editor_size.x + editor_size.width))
    {
        x1 = x1_min;
    }
    
    // horizontal contents extend out both the right and the left
    if (editor_extent.x < editor_size.x &&
        (editor_extent.x + editor_extent.width) > 
         (editor_size.x + editor_size.width))
    {
        // find the minimum amount to add to each size
        int offset = wxMin(x1 - x1_min, x2_max - x2);
        x1 -= offset;
        x2 += offset;
    }
    
    // vertical contents extend out the bottom, but not the top
    if (editor_extent.y >= editor_size.y &&
        (editor_extent.y + editor_extent.height) > 
         (editor_size.y + editor_size.height))
    {
        y2 = y2_max;
    }    

    // vertical contents extend out the top, but not the bottom
    if (editor_extent.y < editor_size.y &&
        (editor_extent.y + editor_extent.height) <= 
         (editor_size.y + editor_size.height))
    {
        y1 = y1_min;
    }    

    // vertical contents extend out both the top and the bottom
    if (editor_extent.y < editor_size.y &&
        (editor_extent.y + editor_extent.height) > 
         (editor_size.y + editor_size.height))
    {
        // find the minimum amount to add to each size
        int offset = wxMin(y1 - y1_min, y2_max - y2);
        y1 -= offset;
        y2 += offset;
    }

    // set the position and size of the editor
    m_editor->setOrigin(x1, y1);
    m_editor->setSize(x2 - x1, y2 - y1);
}

void CompTable::removeLastSelection()
{
    // if the selection size is zero, don't do anything
    if (m_cell_selections.size() <= 0)
        return;

    // otherwise, remove the last selection
    m_cell_selections.pop_back();
}

void CompTable::getLastSelection(CellRange& range)
{
    // if the selection size is greater than zero,
    // return the range for the last selection
    if (m_cell_selections.size() > 0)
    {
        m_cell_selections.back().getRange(range);
        return;
    }
    
    // if we don't have a selection, use the cursor
    range = CellRange(m_cursor_row, m_cursor_col);
}

void CompTable::resizeSelection(int row_diff, int col_diff)
{
    // get the merged cells; used throughout
    std::vector<CellRange> merged_cells;
    m_model->getMergedCells(merged_cells);

    // if we don't have any selections, create an initial selection 
    // based on the cursor position
    if (m_cell_selections.empty())
    {
        int row, col;
        getCursorPos(&row, &col);
        
        CellRange selection(row, col, row, col);
        selectCells(selection);
    }
    
    // we have a selection, so by default, get the last selection;
    // however, if we have a selection index, the cursor has been
    // moved to a particular selection, so get that selection as
    // the one we want to resize
    CellRange* selection = &m_cell_selections.back().m_range;
    if (m_cursor_selection_idx != -1 &&
         m_cursor_selection_idx >= 0 && 
          m_cursor_selection_idx < (int)m_cell_selections.size())
    {
        selection = &m_cell_selections[m_cursor_selection_idx].m_range;
    }
    
    // save a copy of the old selection so we can invalidate the
    // change in the selection area at the end of the function
    CellRange selection_old = *selection;
    
    // get the selection range
    int row1, col1, row2, col2;
    selection->get(&row1, &col1, &row2, &col2);
    
    // get the cursor position
    int cursor_row, cursor_col;
    getCursorPos(&cursor_row, &cursor_col);

    // create default new row and column variables
    int new_row1 = row1;
    int new_col1 = col1;
    int new_row2 = row2;
    int new_col2 = col2;

    // create ranges for each of the edges of the current selection
    CellRange edge1(row1, col1, row1, col2);
    CellRange edge2(row2, col1, row2, col2);
    CellRange edge3(row1, col1, row2, col1);
    CellRange edge4(row1, col2, row2, col2);

    // get the bounding ranges for each of the edges to see if any
    // of the edges intersect a merged cell 
    getBoundingRange(merged_cells, edge1, edge1);
    getBoundingRange(merged_cells, edge2, edge2);
    getBoundingRange(merged_cells, edge3, edge3);
    getBoundingRange(merged_cells, edge4, edge4);
    
    // get the row and column start and end of the bounding range of the top 
    // and bottom edges of the selection area
    int e1_row1, e1_col1, e1_row2, e1_col2;
    int e2_row1, e2_col1, e2_row2, e2_col2;
    edge1.get(&e1_row1, &e1_col1, &e1_row2, &e1_col2);
    edge2.get(&e2_row1, &e2_col1, &e2_row2, &e2_col2);

    if (row_diff > 0)
    {    
        if (e1_row2 >= cursor_row)
        {
            // advancing the top of the selection area would cause the
            // cursor to go outside the selection area, so advance the
            // bottom
            new_row1 = row1;
            new_row2 = row2 + row_diff;
        }
        else
        {
            // advance the top of the selection area, since the advance
            // will not cause the cursor to go outside the selection area;
            // advance by either the row difference or if the current top
            // edge intersects merged cells, by enough to get out of the
            // merged cells
            new_row1 = row1 + wxMax(row_diff, e1_row2 - e1_row1 + 1);
            new_row2 = row2;
        }
    }

    if (row_diff < 0)
    {    
        if (e2_row1 <= cursor_row)
        {
            // advancing the bottom of the selection area would cause the
            // cursor to go outside the selection area, so advance the top      
            new_row1 = row1 + row_diff;
            new_row2 = row2;
        }
        else
        {
            // advance the bottom of the selection area, since the advance
            // will not cause the cursor to go outside the selection area;
            // advance by either the row difference or if the current bottom
            // edge intersects merged cells, by enough to get out of the
            // merged cells        
            new_row1 = row1;
            new_row2 = row2 + wxMin(row_diff, e2_row1 - e2_row2 - 1);
        }
    }

    // get the row and column start and end of the bounding range of the left 
    // and right edges of the selection area
    int e3_row1, e3_col1, e3_row2, e3_col2;
    int e4_row1, e4_col1, e4_row2, e4_col2;
    edge3.get(&e3_row1, &e3_col1, &e3_row2, &e3_col2);
    edge4.get(&e4_row1, &e4_col1, &e4_row2, &e4_col2);

    if (col_diff > 0)
    {   
        if (e3_col2 >= cursor_col)
        {
            // advancing the left of the selection area would cause the
            // cursor to go outside the selection area, so advance the
            // right
            new_col1 = col1;
            new_col2 = col2 + col_diff;
        }
        else
        {
            // advance the left of the selection area, since the advance
            // will not cause the cursor to go outside the selection area;
            // advance by either the column difference or if the current 
            // left edge intersects merged cells, by enough to get out of 
            // the merged cells        
            new_col1 = col1 + wxMax(col_diff, e3_col2 - e3_col1 + 1);
            new_col2 = col2;
        }
    }

    if (col_diff < 0)
    {    
        if (e4_col1 <= cursor_col)
        {
            // advancing the right of the selection area would cause the
            // cursor to go outside the selection area, so advance the
            // left        
            new_col1 = col1 + col_diff;
            new_col2 = col2;
        }
        else
        {
            // advance the right of the selection area, since the advance
            // will not cause the cursor to go outside the selection area;
            // advance by either the column difference or if the current 
            // right edge intersects merged cells, by enough to get out 
            // of the merged cells         
            new_col1 = col1;
            new_col2 = col2 + wxMin(col_diff, e4_col1 - e4_col2 - 1);
        }
    }
  
    // limit the range of the limits of the table; note: if the
    // original selection is a range marker, row1 and col1 will
    // be set to zero; however, row2 and col2 will still be -1;
    // as a result, the range marker is preserved and when the
    // selection is set, it will be set for the range, which
    // gives correct behavior
    if (new_row1 < 0) {new_row1 = 0;}
    if (new_col1 < 0) {new_col1 = 0;}
    if (new_row2 >= getRowCount()) {new_row2 = getRowCount() - 1;}
    if (new_col2 >= getColumnCount()) {new_col2 = getColumnCount() - 1;}
  
    // set the new row and column start and end for the selection area
    // and get the bounding range, as an added check to make sure we
    // include all merged cells, which we already should
    CellRange new_range(new_row1, new_col1, new_row2, new_col2);
    getBoundingRange(merged_cells, new_range, new_range);
    new_range.get(&new_row1, &new_col1, &new_row2, &new_col2);
  
    // set the new selection
    selection->set(new_row1, new_col1, new_row2, new_col2);
    
    // if we have a single selection and the selection is the cursor
    // cell, delete the selection; note: the reason for this is that 
    // the cursor iterates through selections when they are present
    // using the return and tab keys, and we don't want a cursor to 
    // scroll infinitely through this selection, which is hidden by 
    // the cursor itself, causing the cursor to appear glued in place
    if (m_cell_selections.size() == 1 && 
         new_row1 == new_row2 && new_col1 == new_col2 &&
          new_row1 == cursor_row && new_col1 == cursor_col)
    {
        removeAllCellSelections();
    } 

    // find out the difference between the two selections and invalidate it
    std::vector<CellRange> selection_difference;
    CellRange::Xor(*selection, selection_old, selection_difference);
    invalidateCellRange(selection_difference);
}


CompTableHeader::CompTableHeader(int type)
{
    // set the type
    m_type = type;

    // get the default properties; set the foreground line to light grey 
    // and the background color to transparent
    Properties default_properties;
    m_model->getDefaultCellProperties(default_properties);
    
    default_properties.set(PROP_COLOR_BG, Color(COLOR_NULL));
    default_properties.set(PROP_COLOR_FG, Color(COLOR_LIGHT_GREY));
    
    m_model->setDefaultCellProperties(default_properties);

    // set horz/vert gridline, row/column selection and resizing preferences, 
    // depending on the type
    if (m_type == CompTableHeader::Column)
    {
        setPreference(CompTable::prefHorzGridLines, false);
        setPreference(CompTable::prefVertGridLines, true);
        setPreference(CompTable::prefSelectRows, false);
        setPreference(CompTable::prefResizeRows, false);
        setPreference(CompTable::prefSelectColumns, true);
        setPreference(CompTable::prefResizeColumns, true);
    }

    if (m_type == CompTableHeader::Row)
    {
        setPreference(CompTable::prefHorzGridLines, true);
        setPreference(CompTable::prefVertGridLines, false);
        setPreference(CompTable::prefSelectRows, true);
        setPreference(CompTable::prefResizeRows, true);
        setPreference(CompTable::prefSelectColumns, false);
        setPreference(CompTable::prefResizeColumns, false);
    }

    // always turn off the cursor, move cell, populate cell,
    // and edit cell preferences
    setPreference(CompTable::prefCursor, false);
    setPreference(CompTable::prefMoveCells, false);
    setPreference(CompTable::prefPopulateCells, false);
    setPreference(CompTable::prefEditCells, false);

    // make sure the row count and column count are zero
    setRowCount(0);
    setColumnCount(0);
    
    // hook up event handlers
    addEventHandler(EVENT_TABLE_PRE_RESIZE_ROW_TO_CONTENT, &CompTableHeader::onResizeRowToContent);
    addEventHandler(EVENT_TABLE_PRE_RESIZE_COLUMN_TO_CONTENT, &CompTableHeader::onResizeColumnToContent);
}

CompTableHeader::~CompTableHeader()
{
}

IComponentPtr CompTableHeader::create(int type)
{
    return static_cast<IComponent*>(new CompTableHeader(type));
}

IComponentPtr CompTableHeader::clone()
{
    CompTableHeader* c = new CompTableHeader(m_type);
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void CompTableHeader::copy(IComponentPtr component)
{
    // copy the table properties
    CompTable::copy(component);

    // copy the type
    CompTableHeader* c = static_cast<CompTableHeader*>(component.p);
    m_type = c->m_type;
}

void CompTableHeader::render(const wxRect& rect)
{
    // if we don't have a canvas, we're done
    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // before rendering anything else, render a
    // gradient background; the cells background
    // will be set to transparent, and so the
    // gradient background will show through
    wxDirection direction = wxNORTH;

    if (m_type == CompTableHeader::Row)
        direction = wxWEST;
        
    if (m_type == CompTableHeader::Column)
        direction = wxNORTH;

    // set the base color; get the preference from the canvas if specified
    Color base_color = COLOR_CONTROL;
    kcanvas::PropertyValue prop_color;
    if (canvas->getProperty(PROP_COLOR_3DFACE, prop_color))
        base_color = prop_color.getColor();

    Color fade_color = kcanvas::stepColor(COLOR_WHITE, base_color, 70);

    int width, height;
    getSize(&width, &height);

    wxRect header(0, 0, width, height);
    canvas->drawGradientFill(header, base_color, fade_color, direction);
    
    // after drawing the background, render the table 
    // the normal way
    CompTable::render(rect);
}

void CompTableHeader::onResizeRowToContent(IEventPtr evt)
{
    // don't allow resizing based on the content of 
    // the header itself, since the resizing should 
    // be based on the content of the table associated 
    // with the header
    INotifyEventPtr notify_evt = evt;
    if (!notify_evt.isNull())
        notify_evt->veto();
}

void CompTableHeader::onResizeColumnToContent(IEventPtr evt)
{
    // don't allow resizing based on the content of 
    // the header itself, since the resizing should 
    // be based on the content of the table associated 
    // with the header
    INotifyEventPtr notify_evt = evt;
    if (!notify_evt.isNull())
        notify_evt->veto();
}


}; // namespace kcanvas

