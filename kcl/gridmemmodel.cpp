/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2002-01-08
 *
 */


#include "gridmemmodel.h"


namespace kcl
{


GridMemModel::GridMemModel()
{
    m_grid = NULL;
    m_rows_it = m_rows.end();
    m_current_row = -1;
}

GridMemModel::~GridMemModel()
{
}


void GridMemModel::goRow(int row)
{
    if (m_current_row == row)
        return;

    if (m_current_row == -1)
    {
        m_rows_it = m_rows.begin();
        for (int i = 0; i < row; ++i)
        {
            if (m_rows_it == m_rows.end())
                break;
            ++m_rows_it;
        }
        if (m_rows_it == m_rows.end())
            return;
        m_current_row = row;
    }
     else
    {
        int delta = row - m_current_row;
        if (delta < 0)
        {
            while (delta < 0)
            {
                ++delta;
                --m_rows_it;
                --m_current_row;
            }
        }
         else
        {
            while (delta > 0)
            {
                --delta;
                ++m_rows_it;
                ++m_current_row;
            }
        }
    }
}

void GridMemModel::initModel(kcl::Grid* grid)
{
    m_grid = grid;
}

void GridMemModel::refresh()
{
}

void GridMemModel::reset()
{
    m_rows_it = m_rows.begin();
    m_current_row = -1;
}

int GridMemModel::getColumnCount()
{
    return m_columns.size();
}

int GridMemModel::getRowCount()
{
    return m_rows.size();
}

bool GridMemModel::isRowValid(int row)
{
    goRow(row);

    return (m_rows_it == m_rows.end() ? false : true);
}


kcl::IModelColumnPtr GridMemModel::getColumnInfo(int col)
{
    if (col < 0 || col >= (int)m_columns.size())
    {
        return xcm::null;
    }
    
    return m_columns[col];
}


int GridMemModel::getColumnIndex(const wxString& col_name)
{
    kcl::IModelColumnPtr model_col;
    int col_count = getColumnCount();
    for (int i = 0; i < col_count; i++)
    {
        model_col = getColumnInfo(i);
        if (!col_name.CmpNoCase(model_col->getName()))
        {
            return i;
        }
    }

    return -1;
}

void GridMemModel::setColumnBitmap(int col,
                                   const wxBitmap& bitmap,
                                   int alignment)
{
    if (col < 0 || col >= (int)m_colprops.size())
        return;

    m_colprops[col].bitmap = bitmap;
    m_colprops[col].bitmap_alignment = alignment;
}


void GridMemModel::getColumnBitmap(int col,
                                   wxBitmap* bitmap,
                                   int* alignment)
{
    if (col < 0 || col >= (int)m_colprops.size())
    {
        *bitmap = wxNullBitmap;
        *alignment = Grid::alignLeft;
        return;
    }

    *bitmap = m_colprops[col].bitmap;
    *alignment = m_colprops[col].bitmap_alignment;
}


void GridMemModel::setColumnType(int col, int type)
{

}


static void copyCellProps(kcl::CellProperties* dest,
                          const kcl::CellProperties* src)
{
    if (src->mask & kcl::CellProperties::cpmaskCtrlType)
    {
        dest->ctrltype = src->ctrltype;
        dest->mask |= kcl::CellProperties::cpmaskCtrlType;
    }
    if (src->mask & kcl::CellProperties::cpmaskFgColor)
    {
        dest->fgcolor = src->fgcolor;
        dest->mask |= kcl::CellProperties::cpmaskFgColor;
    }
    if (src->mask & kcl::CellProperties::cpmaskBgColor)
    {
        dest->bgcolor = src->bgcolor;
        dest->mask |= kcl::CellProperties::cpmaskBgColor;
    }
    if (src->mask & kcl::CellProperties::cpmaskAlignment)
    {
        dest->alignment = src->alignment;
        dest->mask |= kcl::CellProperties::cpmaskAlignment;
    }
    if (src->mask & kcl::CellProperties::cpmaskBitmapAlignment)
    {
        dest->bitmap_alignment = src->bitmap_alignment;
        dest->mask |= kcl::CellProperties::cpmaskBitmapAlignment;
    }
    if (src->mask & kcl::CellProperties::cpmaskCbChoices)
    {
        dest->cbchoices = src->cbchoices;
        dest->mask |= kcl::CellProperties::cpmaskCbChoices;
    }
    if (src->mask & kcl::CellProperties::cpmaskEditable)
    {
        dest->editable = src->editable;
        dest->mask |= kcl::CellProperties::cpmaskEditable;
    }
    if (src->mask & kcl::CellProperties::cpmaskVisible)
    {
        dest->visible = src->visible;
        dest->mask |= kcl::CellProperties::cpmaskVisible;
    }
    if (src->mask & kcl::CellProperties::cpmaskTextWrap)
    {
        dest->text_wrap = src->text_wrap;
        dest->mask |= kcl::CellProperties::cpmaskTextWrap;
    }
}

bool GridMemModel::setColumnProperties(int col,
                                       kcl::CellProperties* cell_props)
{
    if (!cell_props)
        return false;
    if (col < 0 || col >= (int)m_colprops.size())
        return false;

    copyCellProps(&m_colprops[col], cell_props);
    return true;
}


void GridMemModel::getCellProperties(int row,
                                     int col,
                                     kcl::CellProperties* cell_props)
{
    goRow(row);

    cell_props->mask |= kcl::CellProperties::cpmaskCtrlType;
    if (m_columns[col]->getType() == kcl::Grid::typeBoolean)
    {
        cell_props->ctrltype = kcl::Grid::ctrltypeCheckBox;
    }
     else
    {
        cell_props->ctrltype= kcl::Grid::ctrltypeText;
    }

    copyCellProps(cell_props, &m_colprops[col]);
    copyCellProps(cell_props, &(m_rows_it->cols[col].cell_props));
}


bool GridMemModel::setCellProperties(int row,
                                     int col,
                                     kcl::CellProperties* cell_props)
{
    if (!cell_props)
        return false;

    goRow(row);

    copyCellProps(&(m_rows_it->cols[col].cell_props), cell_props);
    return true;
}


intptr_t GridMemModel::getRowData(int row)
{
    goRow(row);

    if (m_rows_it == m_rows.end())
    {
        return 0;
    }

    return m_rows_it->rowdata;
}

void GridMemModel::setRowData(int row, intptr_t data)
{
    goRow(row);

    if (m_rows_it != m_rows.end())
    {
        m_rows_it->rowdata = data;
    }
}

void GridMemModel::getCellBitmap(int row,
                                 int col,
                                 wxBitmap* bitmap,
                                 int* alignment)
{
    goRow(row);

    *bitmap = m_rows_it->cols[col].cell_bitmap;
    *alignment = m_rows_it->cols[col].bmp_alignment;
}

wxString GridMemModel::getCellString(int row, int col)
{
    goRow(row);

    return m_rows_it->cols[col].cell_value;
}

double GridMemModel::getCellDouble(int row, int col)
{
    static wxChar search_ch[2] = wxT("\0");
    static wxChar replace_ch[2] = wxT("\0");

    if (!search_ch[0])
    {
        lconv* l = localeconv();
        replace_ch[0] = (unsigned char)*l->decimal_point;
        if (replace_ch[0] == L',')
        {
            search_ch[0] = L'.';
        }
         else
        {
            search_ch[0] = L',';
        }

        replace_ch[1] = 0;
        search_ch[1] = 0;
    }

    goRow(row);

    wxString s = m_rows_it->cols[col].cell_value;
    double d;
    s.Replace(search_ch, replace_ch);
    s.ToDouble(&d);

    return d;
}

int GridMemModel::getCellInteger(int row, int col)
{
    goRow(row);

    long l;
    m_rows_it->cols[col].cell_value.ToLong(&l);

    return l;
}

bool GridMemModel::getCellBoolean(int row, int col)
{
    goRow(row);

    return m_rows_it->cols[col].cell_value == wxT("1") ? true : false;
}

int GridMemModel::getCellComboSel(int row, int col)
{
    goRow(row);

    return m_rows_it->cols[col].combo_sel;
}

bool GridMemModel::isNull(int row, int col)
{
    return false;
}

bool GridMemModel::getGroupBreak()
{
    return false;
}

bool GridMemModel::getGroupBreaksActive()
{
    return false;
}


bool GridMemModel::setCellBitmap(int row,
                                 int col,
                                 const wxBitmap& bitmap,
                                 int alignment)
{
    goRow(row);

    m_rows_it->cols[col].cell_bitmap = bitmap;
    m_rows_it->cols[col].bmp_alignment = alignment;
    return false;
}

bool GridMemModel::setCellString(int row, int col, const wxString& value)
{
    goRow(row);

    if (m_current_row == -1)    
        return false;

    m_rows_it->cols[col].cell_value = value;
    return true;
}

bool GridMemModel::setCellDouble(int row, int col, double value)
{
    goRow(row);

    if (m_current_row == -1)    
        return false;

    m_rows_it->cols[col].cell_value = wxString::Format(wxT("%.*f"), m_columns[col]->getScale(), value);
    return true;
}

bool GridMemModel::setCellInteger(int row, int col, int value)
{
    goRow(row);

    if (m_current_row == -1)    
        return false;

    m_rows_it->cols[col].cell_value = wxString::Format(wxT("%d"), value);
    return true;
}

bool GridMemModel::setCellBoolean(int row, int col, bool value)
{
    goRow(row);

    if (m_current_row == -1)    
        return false;

    m_rows_it->cols[col].cell_value = (value ? wxT("1") : wxT("0"));
    return true;
}

bool GridMemModel::setCellComboSel(int row, int col, int sel)
{
    goRow(row);

    if (m_current_row == -1)    
        return false;

    m_rows_it->cols[col].combo_sel = sel;
    return true;
}

int GridMemModel::createColumn(int position,
                               const wxString& name,
                               int type,
                               int width,
                               int scale)
{
    std::vector<kcl::IModelColumnPtr>::iterator insert_it;

    if (position == -1 || position >=(int) m_columns.size())
    {
        insert_it = m_columns.end();
    }
     else
    {
        insert_it = m_columns.begin();

        for (int i = 0; i < position; i++)
        {
            ++insert_it;
        }
    }

    // add column to grid

    kcl::IModelColumnPtr newcol = static_cast<kcl::IModelColumn*>(new kcl::ModelColumn);

    newcol->setName(name);
    newcol->setType(type);
    newcol->setWidth(width);
    newcol->setScale(scale);

    m_columns.insert(insert_it, newcol);


    // add column properties

    std::vector<MemColumnProperties>::iterator propit;
    propit = m_colprops.insert(position == -1 ? m_colprops.end() : (m_colprops.begin() + position), MemColumnProperties());
    propit->mask = 0;

    // add column to existing rows

    std::list<MemRow>::iterator row_it;

    for (row_it = m_rows.begin(); row_it != m_rows.end(); ++row_it)
    {
        row_it->cols.insert(position == -1 ? row_it->cols.end() : row_it->cols.begin() + position, MemCell());
    }

    return (position == -1 ? m_columns.size() - 1 : position);
}



bool GridMemModel::modifyColumn(int position,
                                const wxString& name,
                                int type,
                                int width,
                                int scale)
{
    std::vector<kcl::IModelColumnPtr>::iterator modify_it;

    if (position < 0 || position >= (int)m_columns.size())
    {
        return false;
    }
     else
    {
        modify_it = m_columns.begin();

        for (int i = 0; i < position; i++)
        {
            ++modify_it;
        }
    }

    // modify the specified column properties

    kcl::IModelColumnPtr col = *modify_it;

    if (name.Length() != 0)
    {
        col->setName(name);
    }

    if (type != -1)
    {
        col->setType(type);
    }

    if (width != -1)
    {
        col->setWidth(width);
    }

    if (scale != -1)
    {
        col->setScale(scale);
    }

    return true;
}




bool GridMemModel::deleteColumn(int position)
{
    if (position < 0 || position >= (int)m_columns.size())
        return false;

    m_columns.erase(m_columns.begin() + position);

    std::list<MemRow>::iterator row_it;
    for (row_it = m_rows.begin(); row_it != m_rows.end(); ++row_it)
    {
        if (position < 0 || position >= (int)row_it->cols.size())
            continue;

        row_it->cols.erase(row_it->cols.begin() + position);
    }

    return true;
}


int GridMemModel::insertRow(int position)
{
    std::list<MemRow>::iterator insert_it;
    int retval;

    if (position >= 0)
    {
        goRow(position);
        insert_it = m_rows_it;
        retval = position;
    }
     else
    {
        insert_it = m_rows.end();
        retval = 0;
    }

    std::list<MemRow>::iterator new_row;

    new_row = m_rows.insert(insert_it, MemRow());
    new_row->cols.resize(m_columns.size());
    new_row->rowdata = 0;

    // if this is our first row, set the iterator to that row
    if (m_rows.size() == 1)
    {
        m_rows_it = new_row;
    }

    m_current_row = -1;

    // return the row number of the new row
    return retval;
}

bool GridMemModel::deleteRow(int position)
{
    std::list<MemRow>::iterator it;

    it = m_rows.begin();

    for (int i = 0; i < position; i++)
    {
        ++it;

        if (it == m_rows.end())
        {
            return false;
        }
    }
    
    if (m_rows_it == it)
    {
        ++m_rows_it;
    }

    m_rows.erase(it);

    m_current_row = -1;

    return true;
}

bool GridMemModel::deleteAllRows()
{
    m_rows.clear();
    m_rows_it = m_rows.end();
    m_current_row = -1;
    return true;
}


void GridMemModel::onCursorRowChanged()
{
}



};

