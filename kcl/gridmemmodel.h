/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2002-01-08
 *
 */


#ifndef __KCL_GRIDMEMMODEL_H
#define __KCL_GRIDMEMMODEL_H


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "grid.h"
#include <xcm/xcm.h>
#include <list>


namespace kcl
{


class MemCell
{
public:
    
    MemCell()
    {
        bmp_alignment = kcl::Grid::alignLeft;
        combo_sel = -1;
    }

    wxString cell_value;
    int combo_sel;
    kcl::CellProperties cell_props;
    wxBitmap cell_bitmap;
    int bmp_alignment;
};


class MemRow
{

public:
    std::vector<MemCell> cols;
    long rowdata;
};


class MemColumnProperties : public CellProperties
{

public:

    wxBitmap bitmap;
    int bitmap_alignment;
};


class GridMemModel : public kcl::IModel
{
    XCM_CLASS_NAME("kcl.GridMemModel")
    XCM_BEGIN_INTERFACE_MAP(GridMemModel)
        XCM_INTERFACE_ENTRY(kcl::IModel)
    XCM_END_INTERFACE_MAP()

    kcl::Grid* m_grid;
    std::list<MemRow> m_rows;
    std::list<MemRow>::iterator m_rows_it;
    int m_current_row;
    std::vector<kcl::IModelColumnPtr> m_columns;
    std::vector<MemColumnProperties> m_colprops;

    void goRow(int row);

public:

    GridMemModel();
    virtual ~GridMemModel();

    void initModel(kcl::Grid* grid);
    void refresh();
    void reset();

    bool isRowValid(int row);
    int getRowCount();
    int getColumnCount();
    kcl::IModelColumnPtr getColumnInfo(int idx);
    int getColumnIndex(const wxString& col_name);

    void setColumnBitmap(int col, const wxBitmap& bitmap, int alignment);
    void getColumnBitmap(int col, wxBitmap* icon, int* alignment);

    void setColumnType(int idx, int type);

    bool setColumnProperties(int col, kcl::CellProperties* cell_props);
    bool setCellProperties(int row, int col, kcl::CellProperties* cell_props);
    void getCellProperties(int row, int col, kcl::CellProperties* cell_props);

    long getRowData(int row);
    void setRowData(int row, long data);

    void getCellBitmap(int row, int col, wxBitmap* bitmap, int* alignment);
    wxString getCellString(int row, int col);
    double getCellDouble(int row, int col);
    int getCellInteger(int row, int col);
    bool getCellBoolean(int row, int col);
    int getCellComboSel(int row, int col);
    bool isNull(int row, int col);
    bool getGroupBreak();
    bool getGroupBreaksActive();

    bool setCellBitmap(int row, int col, const wxBitmap& bitmap, int alignment);
    bool setCellString(int row, int col, const wxString& value);
    bool setCellDouble(int row, int col, double value);
    bool setCellInteger(int row, int col, int value);
    bool setCellBoolean(int row, int col, bool value);
    bool setCellComboSel(int row, int col, int sel);

    int createColumn(int position, const wxString& name, int type, int width, int scale);
    bool modifyColumn(int position, const wxString& name, int type, int width, int scale);

    bool deleteColumn(int position);
    int insertRow(int position);
    bool deleteRow(int position);
    bool deleteAllRows();

    void onCursorRowChanged();
};



}; // namespace kcl


#endif


