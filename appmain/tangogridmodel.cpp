/*!
 *
 * Copyright (c) 2001-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2001-11-15
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "appmain.h"
#include "tangogridmodel.h"



TangoGridModel::TangoGridModel()
{
    m_it = xcm::null;
    m_forward_only = false;
    m_boolean_checkbox = true;

    m_grid = NULL;

    m_break_buf = NULL;
    m_break_buf_len = 0;
    m_break_expr = wxT("");
    m_break_handle = 0;
    m_break_result = false;

    m_current_row = -1;
    m_row_count = -1;
    m_current_mark_row = -1;

    m_find_value = wxT("");
    m_find_match_case = false;
    m_find_whole_cell = false;
    
    m_empty_cell.m_str_val = wxT("");
    m_empty_cell.m_int_val = 0;
    m_empty_cell.m_dbl_val = 0.0;
    m_empty_cell.m_bool_val = false;
    m_empty_cell.m_null = true;
}

TangoGridModel::~TangoGridModel()
{
    if (m_break_buf)
    {
        free(m_break_buf);
    }
}




void TangoGridModel::copyRowIntoCache(int row)
{
    int col;
    int column_count = m_columns.size();

    while ((int)m_cache.size() <= row)
    {
        m_cache.push_back(TangoGridRowData());
        m_cache.back().m_cols.resize(column_count);
    }

    TangoGridRowData& cache_row = m_cache[row];

    cache_row.rowid = m_it->getRowId();

    for (col = 0; col < column_count; ++col)
    {
        TangoGridCellData& cell = cache_row.m_cols[col];

        cell.m_null = m_it->isNull(m_columns[col].m_col_handle);

        switch (m_columns[col].m_col_type)
        {
            default:
            case tango::typeWideCharacter:
            case tango::typeCharacter:
                cell.m_str_val = towx(m_it->getWideString(m_columns[col].m_col_handle));
                break;

            case tango::typeDouble:
            case tango::typeNumeric:
                cell.m_dbl_val = m_it->getDouble(m_columns[col].m_col_handle);
                break;

            case tango::typeInteger:
                cell.m_int_val = m_it->getInteger(m_columns[col].m_col_handle);
                break;

            case tango::typeDate:
            {
                tango::datetime_t d;
                d = m_it->getDateTime(m_columns[col].m_col_handle);

                if (d == 0)
                {
                    cell.m_str_val = wxEmptyString;
                }
                 else
                {
                    tango::DateTime dt = d;
                    cell.m_str_val = wxString::Format(wxT("%02d/%02d/%04d"), dt.getMonth(), dt.getDay(), dt.getYear());
                }
                
                cell.m_datetime_val = d;
            }
            break;

            case tango::typeDateTime:
            {
                tango::datetime_t d;
                d = m_it->getDateTime(m_columns[col].m_col_handle);

                if (d == 0)
                {
                    cell.m_str_val = wxEmptyString;
                }
                 else
                {
                    tango::DateTime dt = d;
                    cell.m_str_val = wxString::Format(
                                          wxT("%02d/%02d/%04d %02d:%02d:%02d"),
                                          dt.getMonth(),
                                          dt.getDay(),
                                          dt.getYear(),
                                          dt.getHour(),
                                          dt.getMinute(),
                                          dt.getSecond());
                }
                
                cell.m_datetime_val = d;
            }
            break;

            case tango::typeBoolean:
                cell.m_bool_val = m_it->getBoolean(m_columns[col].m_col_handle);
                break;
        }
    }
}


void TangoGridModel::goRowFwdOnly(int row)
{
    if (m_current_row == -1)
    {
        m_it->goFirst();
        m_current_row = 0;
    }
     else
    {
        if (row < (int)m_cache.size())
            return;
    }
    
    if (m_it->eof())
        return;

    int i;
    for (i = m_current_row; i <= row; ++i)
    {
        copyRowIntoCache(i);
        m_it->skip(1);
        m_current_row++;
    }
}



void TangoGridModel::goRow(int row)
{
    if (m_forward_only)
    {
        goRowFwdOnly(row);
        return;
    }

    if (row == m_current_row)
        return;

    m_break_result = false;

    if (row == 0)
    {
        m_it->goFirst();
        m_current_row = 0;
        return;
    }

    // determine if we should do a long jump

    if (abs(row-m_current_row) > 100000)
    {
        // (this code is ok even if m_current_row is -1; it is just an approximation

        tango::ISetPtr set = m_it->getSet();
        if (set->getSetFlags() & tango::sfFastRowCount)
        {

            // if a large skip runs fast, we don't need to to the long jump approximation
            if (!(m_it->getIteratorFlags() & tango::ifFastSkip))
            {
                if (m_current_row < 0)
                    m_current_row = 1;

                double row_count = (tango::tango_int64_t)set->getRowCount();
                double dest = row;
                double pos = dest/row_count;

                if (row_count-dest < 100000)
                {
                    // if we are near the end of the set, we can
                    // count back from the end

                    int rows_from_end = int(row_count) - (row+1);

                    m_it->goLast();
                    m_it->skip(-rows_from_end);
                    m_current_row = row;
                    return;
                }

                if (pos < 0.005)
                {
                    // go to the first row
                    m_it->goFirst();
                    m_current_row = 0;
                    return;
                }
                 else if (pos > 0.995)
                {
                    m_it->goLast();
                    m_current_row = (int)row_count;
                    return;
                }
                 else
                {
                    if (m_it->setPos(pos))
                    {
                        m_current_row = row;
                        return;
                    }
                    
                    // setPos() failed. use regular skip
                }

            }
        }
    }


    if (m_current_row == -1)
    {
        m_it->goFirst();
        if (row > 0)
        {
            m_it->skip(row);
        }
    }
     else
    {
        bool check_break = false;

        if (m_break_handle && row-m_current_row == 1)
        {
            check_break = true;
            unsigned char* b = (unsigned char*)m_it->getRawPtr(m_break_handle);
            int break_len = m_it->getRawWidth(m_break_handle);

            if (break_len > m_break_buf_len)
            {
                m_break_buf = (unsigned char*)realloc(m_break_buf, break_len);
            }

            m_break_buf_len = break_len;
            memcpy(m_break_buf, b, break_len);
        }

        m_it->skip(row-m_current_row);

        if (check_break)
        {
            if (m_it->eof())
            {
                m_break_result = false;
            }
             else
            {
                unsigned char* b = (unsigned char*)m_it->getRawPtr(m_break_handle);
                if (memcmp(m_break_buf, b, m_break_buf_len) != 0)
                {
                    m_break_result = true;
                }
            }
        }
    }
    
    m_current_row = row;
}


void TangoGridModel::initModel(kcl::Grid* grid)
{
    m_grid = grid;
}

void TangoGridModel::refreshView()
{

}

void TangoGridModel::refresh()
{
    m_column_lookup.clear();

    if (m_it)
    {
        tango::IStructurePtr structure = m_it->getStructure();
        int col_count = structure->getColumnCount();
        int i;

        m_columns.resize(col_count);

        tango::IColumnInfoPtr spCol;
        for (i = 0; i < col_count; i++)
        {
            TangoGridColumnInfo& gci = m_columns[i];

            gci.m_col_name = towx(structure->getColumnName(i));
            gci.m_col_handle = m_it->getHandle(towstr(gci.m_col_name));
            spCol = m_it->getInfo(gci.m_col_handle);
            gci.m_col_type = spCol->getType();
            gci.m_col_width = spCol->getWidth();
            gci.m_col_scale = spCol->getScale();
            gci.m_col_calculated = spCol->getCalculated();
            gci.m_model_col.clear();

            switch (gci.m_col_type)
            {
                case tango::typeDate:
                    gci.m_col_width = 20;
                    break;
                case tango::typeDateTime:
                    gci.m_col_width = 20;
                    break;
                case tango::typeBoolean:
                    gci.m_col_width = 1;
                    break;
                case tango::typeInteger:
                    gci.m_col_width = 10;
                    break;
                case tango::typeDouble:
                    gci.m_col_width = 20;
                    break;
            }
        }

        if (m_grid)
        {
            m_grid->refresh(kcl::Grid::refreshColumnView);
        }
    }
}


void TangoGridModel::reset()
{
    m_current_row = -1;
}



void TangoGridModel::setIterator(tango::IIterator* it)
{
    if (m_it)
    {
        if (m_break_handle)
        {
            m_it->releaseHandle(m_break_handle);
            m_break_handle = 0;
        }

        std::vector<TangoGridColumnInfo>::iterator col_it;
        for (col_it = m_columns.begin(); col_it != m_columns.end(); ++col_it)
        {
            m_it->releaseHandle(col_it->m_col_handle);
            col_it->m_col_handle = 0;
            col_it->m_model_col.clear();
        }
    }


    if (m_it != it)
    {
        m_it = it;
    }

    m_current_row = -1;
    m_row_count = -1;
    m_current_mark_row = -1;
    m_forward_only = false;
    clearCache();

    if (m_it)
    {
        m_cursor_rowid = it->getRowId();

        m_forward_only = (it->getIteratorFlags() & tango::ifForwardOnly) ? true : false;

        if (m_grid)
        {
            m_grid->reset();
        }

        if (m_break_expr.length())
        {
            m_break_handle = m_it->getHandle(towstr(m_break_expr));
        }
        
        refresh();
    }
}

tango::IIteratorPtr TangoGridModel::getIterator()
{
    return m_it;
}


bool TangoGridModel::isEofKnown()
{
    tango::ISetPtr set = m_it->getSet();
    if (set.isOk() && (set->getSetFlags() & tango::sfFastRowCount))
        return true;

    return (m_row_count == -1) ? false : true;
}

void TangoGridModel::discoverEof()
{
    if (isEofKnown())
        return;

    int i = m_current_row;
    if (i < 0)
        i = 0;

    while (isRowValid(i))
        i++;

    m_row_count = i;
}


void TangoGridModel::setBooleanCheckbox(bool val)
{
    m_boolean_checkbox = val;
}

void TangoGridModel::setGroupBreakExpr(const wxString& new_val)
{
    if (m_break_handle)
    {
        if (m_it)
        {
            m_it->releaseHandle(m_break_handle);
        }

        m_break_handle = 0;
    }

    m_break_expr = new_val;

    if (m_break_expr.Length() == 0)
    {
        return;
    }

    if (m_it)
    {
        m_break_handle = m_it->getHandle(towstr(m_break_expr));
    }
}


wxString TangoGridModel::getGroupBreakExpr()
{
    return m_break_expr;
}


int TangoGridModel::getColumnCount()
{
    return m_columns.size();
}

kcl::IModelColumnPtr TangoGridModel::getColumnInfo(int idx)
{
    if (idx < 0 || (size_t)idx >= m_columns.size())
        return xcm::null;
    
    TangoGridColumnInfo& gci = m_columns[idx];
    if (gci.m_model_col)
        return gci.m_model_col;
        
    kcl::ModelColumn* m = new kcl::ModelColumn;
    m->setName(gci.m_col_name);

    switch (gci.m_col_type)
    {
        default:
        case tango::typeCharacter: m->setType(kcl::Grid::typeCharacter);     break;
        case tango::typeWideCharacter: m->setType(kcl::Grid::typeCharacter); break;
        case tango::typeInvalid: m->setType(kcl::Grid::typeInvalid);         break;
        case tango::typeNumeric: m->setType(kcl::Grid::typeDouble);          break;
        case tango::typeDouble: m->setType(kcl::Grid::typeDouble);           break;
        case tango::typeInteger: m->setType(kcl::Grid::typeInteger);         break;
        case tango::typeDate: m->setType(kcl::Grid::typeDate);               break;
        case tango::typeDateTime: m->setType(kcl::Grid::typeDateTime);       break;
        case tango::typeBoolean: m->setType(kcl::Grid::typeBoolean);         break;
    }

    m->setWidth(gci.m_col_width);
    m->setScale(gci.m_col_scale);
    
    gci.m_model_col = static_cast<kcl::IModelColumn*>(m);
    return gci.m_model_col;
}


int TangoGridModel::getColumnIndex(const wxString& col_name)
{
    if (m_column_lookup.size() != m_columns.size())
    {
        // create a column lookup cache
        int idx = 0;
        std::vector<TangoGridColumnInfo>::iterator col_it;
        for (col_it = m_columns.begin(); col_it != m_columns.end(); ++col_it)
        {
            wxString s = col_it->m_col_name;
            s.MakeUpper();
            m_column_lookup[s] = idx;
            idx++;
        }
    }

    wxString s = col_name;
    s.MakeUpper();
    
    ColLookupHashMap::iterator it;
    it = m_column_lookup.find(s);
    if (it == m_column_lookup.end())
        return -1;

    return it->second;
}


bool TangoGridModel::getColumnCalculated(int model_col)
{
    if (model_col < 0 || (size_t)model_col >= m_columns.size())
        return false;
        
    return m_columns[model_col].m_col_calculated;
}


void TangoGridModel::getColumnBitmap(int col,
                                     wxBitmap* bitmap,
                                     int* alignment)
{
    tango::IColumnInfoPtr spCol;
    if (m_columns[col].m_col_calculated)
    {
        *bitmap = GETBMP(gf_lightning_16);
        *alignment = kcl::Grid::alignCenter;
    }
}

void TangoGridModel::setColumnBitmap(int col,
                                     const wxBitmap& bitmap,
                                     int alignment)
{
}


bool TangoGridModel::setColumnProperties(int col,
                                         kcl::CellProperties* cell_props)
{
    // this model does not support this operation
    return false;
}

void TangoGridModel::getCellProperties(int row,
                                       int col,
                                       kcl::CellProperties* cell_props)
{
    if (!m_find_value.IsEmpty())
    {
        wxString value = getCellString(row, col);
        bool found = false;

        if (m_find_whole_cell)
        {
            if (m_find_match_case)
            {
                if (m_find_value == value)
                    found = true;
            }
             else
            {
                if (0 == m_find_value.CmpNoCase(value))
                    found = true;
            }
        }
         else
        {
            if (m_find_match_case)
            {
                if (-1 != value.Find(m_find_value))
                    found = true;
            }
             else
            {
                wxString find_upper = m_find_value;

                value.MakeUpper();
                find_upper.MakeUpper();

                if (-1 != value.Find(find_upper))
                    found = true;
            }
        }

        if (found)
        {
            cell_props->fgcolor.Set(0,0,255);
            cell_props->bgcolor = m_bgcolor;
            return;
        }

    }


    if (m_current_mark_row == -1 || m_current_mark_row != row)
    {
        goRow(row);
        m_current_mark_row = row;
        sigRequestRowColors(m_fgcolor, m_bgcolor);
    }

    if (m_fgcolor.Ok())
    {
        cell_props->fgcolor = m_fgcolor;
    }
    
    if (m_bgcolor.Ok())
    {
        cell_props->bgcolor = m_bgcolor;
    }

    if (col >= 0 && (size_t)col < m_columns.size())
    {
        if (m_boolean_checkbox)
        {
            if (m_columns[col].m_col_type == tango::typeBoolean)
            {
                cell_props->ctrltype = kcl::Grid::ctrltypeCheckBox;
            }
        }
         else
        {
            if (m_columns[col].m_col_type == tango::typeBoolean)
            {
                cell_props->ctrltype = kcl::Grid::ctrltypeText;
            }
        }
    }
}


bool TangoGridModel::setCellProperties(int row,
                                       int col,
                                       kcl::CellProperties* cell_props)
{
    // this model does not support this operation
    return false;
}

long TangoGridModel::getRowData(int row)
{
    return 0;
}

void TangoGridModel::setRowData(int row, long data)
{
}

void TangoGridModel::getCellBitmap(int row,
                                   int col,
                                   wxBitmap* bitmap,
                                   int* alignment)
{
}


wxString TangoGridModel::getCellString(int row, int col)
{
    goRow(row);

    if (inCache(row, col))
    {
        return getCacheCell(row, col).m_str_val;
    }

    if (m_it->eof())
        return wxEmptyString;

    switch (m_columns[col].m_col_type)
    {
        default:
        case tango::typeCharacter:
            return towx(m_it->getString(m_columns[col].m_col_handle));

        case tango::typeWideCharacter:
            return towx(m_it->getWideString(m_columns[col].m_col_handle));

        case tango::typeDouble:
        case tango::typeNumeric:
        {
            double d = m_it->getDouble(m_columns[col].m_col_handle);
            return wxString::Format(wxT("%.*f"), m_columns[col].m_col_scale, d);
        }

        case tango::typeInteger:
        {
            int i = m_it->getInteger(m_columns[col].m_col_handle);
            return wxString::Format(wxT("%d"), i);
        }

        case tango::typeDate:
        {
            tango::DateTime dt;
            tango::datetime_t d;
            
            d = m_it->getDateTime(m_columns[col].m_col_handle);
            if (d == 0)
            {
                return wxT("");
            }

            dt = d;

            return cfw::Locale::formatDate(dt.getYear(),
                                           dt.getMonth(),
                                           dt.getDay());
        }

        case tango::typeDateTime:
        {
            tango::DateTime dt;
            tango::datetime_t d;

            d = m_it->getDateTime(m_columns[col].m_col_handle);
            if (d == 0)
            {
                return wxT("");
            }

            dt = d;
            
            return cfw::Locale::formatDate(dt.getYear(),
                                           dt.getMonth(),
                                           dt.getDay(),
                                           dt.getHour(),
                                           dt.getMinute(),
                                           dt.getSecond());
        }

        case tango::typeBoolean:
            return m_it->getBoolean(m_columns[col].m_col_handle) ?
                                    wxT("T") : wxT("F");
    }

    return wxT("");
}

double TangoGridModel::getCellDouble(int row, int col)
{
    goRow(row);

    if (inCache(row, col))
    {
        return getCacheCell(row, col).m_dbl_val;
    }

    return m_it->getDouble(m_columns[col].m_col_handle);
}

int TangoGridModel::getCellInteger(int row, int col)
{
    goRow(row);

    if (inCache(row, col))
    {
        return getCacheCell(row, col).m_int_val;
    }

    return m_it->getInteger(m_columns[col].m_col_handle);
}

bool TangoGridModel::getCellBoolean(int row, int col)
{
    goRow(row);

    if (inCache(row, col))
    {
        return getCacheCell(row, col).m_bool_val;
    }

    return m_it->getBoolean(m_columns[col].m_col_handle);
}

tango::datetime_t TangoGridModel::getCellDateTime(int row, int col)
{
    goRow(row);

    if (inCache(row, col))
    {
        return getCacheCell(row, col).m_datetime_val;
    }

    return m_it->getDateTime(m_columns[col].m_col_handle);
}

int TangoGridModel::getCellComboSel(int row, int col)
{
    return -1;
}

bool TangoGridModel::isNull(int row, int col)
{
    goRow(row);

    if (inCache(row, col))
    {
        return getCacheCell(row, col).m_null;
    }

    return m_it->isNull(m_columns[col].m_col_handle);
}

bool TangoGridModel::setCellBitmap(int row,
                                   int col,
                                   const wxBitmap& bitmap,
                                   int alignment)
{
    goRow(row);

    return false;
}

bool TangoGridModel::setCellString(int row, int col, const wxString& value)
{
    return false;
}

bool TangoGridModel::setCellDouble(int row, int col, double value)
{
    return false;
}

bool TangoGridModel::setCellInteger(int row, int col, int value)
{
    return false;
}

bool TangoGridModel::setCellBoolean(int row, int col, bool value)
{
    return false;
}

bool TangoGridModel::setCellComboSel(int row, int col, int sel)
{
    return false;
}


int TangoGridModel::createColumn(int position,
                                 const wxString& name,
                                 int type,
                                 int width,
                                 int scale)
{
    // this model does not support column insertion
    return -1;
}

bool TangoGridModel::modifyColumn(int position,
                                  const wxString& str,
                                  int type,
                                  int width,
                                  int scale)
{
    // this model does not support column modification
    return false;
}

bool TangoGridModel::deleteColumn(int position)
{
    // this model does not support column deletion
    return false;
}

int TangoGridModel::insertRow(int position)
{
    // this model does not yet support row insertion
    return -1;
}

bool TangoGridModel::deleteRow(int position)
{
    // this model does not yet support row deletion
    return false;
}

bool TangoGridModel::deleteAllRows()
{
    // this model does not yet support row deletion
    return false;
}

int TangoGridModel::getRowCount()
{
    // next line is for 2.2 only (it fixes
    // the one-record mark problem)
    m_current_mark_row = -1;

    if (m_it.isNull())
    {
        wxFAIL_MSG(wxT("in TangoGridModel::getRowCount(), m_it is null"));
        return 0;
    }
        
    tango::ISetPtr set = m_it->getSet();
    if (set.isOk() && (set->getSetFlags() & tango::sfFastRowCount))
    {
        tango::rowpos_t row_count = set->getRowCount();

        // clamp it off at 2 billion
        if (row_count > 2000000000)
            row_count = 2000000000;

        return row_count;
    }
     else
    {
        if (m_row_count != -1)
            return m_row_count;

        if (m_current_row <= 0)
            return 100;
        return m_current_row+100;
    }

    return 0;
}


bool TangoGridModel::isRowValid(int row)
{
    goRow(row);

    if (m_forward_only)
    {
        return (row < (int)m_cache.size() ? true : false);
    }

    return m_it->eof() ? false : true;
}


bool TangoGridModel::getGroupBreak()
{
    return m_break_result;
}

bool TangoGridModel::getGroupBreaksActive()
{
    return (m_break_handle != 0 ? true : false);
}


void TangoGridModel::onCursorRowChanged()
{
}

tango::rowid_t TangoGridModel::getRowId(int row)
{
    goRow(row);

    if (inCache(row, 0))
    {
        return m_cache[row].rowid;
    }

    return m_it->getRowId();
}


TangoGridCellData& TangoGridModel::getCacheCell(int row, int col)
{
    if (!m_forward_only)
        return m_empty_cell;

    if ((int)m_cache.size() <= row)
        return m_empty_cell;

    if ((int)m_cache[row].m_cols.size() <= col)
        return m_empty_cell;

    return m_cache[row].m_cols[col];
}


bool TangoGridModel::inCache(int row, int col)
{
    if (!m_forward_only)
        return false;

    if ((int)m_cache.size() <= row)
        return false;

    return true;
}

void TangoGridModel::clearCache()
{
    m_cache.clear();
}

void TangoGridModel::setCurrentRow(int row)
{
    m_current_row = row;
}


void TangoGridModel::setFindHighlight(const wxString& value,
                                      bool match_case,
                                      bool whole_cell)
{
    m_find_value = value;
    m_find_match_case = match_case;
    m_find_whole_cell = whole_cell;
}




