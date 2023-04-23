/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2001-11-15
 *
 */


#ifndef __APP_XDGRIDMODEL_H
#define __APP_XDGRIDMODEL_H


#include <xd/xd.h>
#include <wx/hashmap.h>


xcm_interface IXdGridModel;
XCM_DECLARE_SMARTPTR(IXdGridModel)



xcm_interface IXdGridModel : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IXdGridModel")

public:

    virtual void setIterator(xd::IIterator* it) = 0;
    virtual xd::IIteratorPtr getIterator() = 0;

    virtual void discoverEof() = 0;
    virtual bool isEofKnown() = 0;

    virtual void setBooleanCheckbox(bool val) = 0;

    virtual void setGroupBreakExpr(const wxString& new_val) = 0;
    virtual wxString getGroupBreakExpr() = 0;

    virtual void refreshView() = 0;
    virtual bool getColumnCalculated(int model_col) = 0;

    virtual xd::rowid_t getRowId(int row) = 0;
    virtual void setCurrentRow(int row) = 0;

    virtual void setFindHighlight(const wxString& value,
                                  bool case_sense,
                                  bool whole_cell) = 0;

    virtual void clearCache() = 0;
    
    virtual wxString getCellString(int row, int col) = 0;
    virtual double getCellDouble(int row, int col) = 0;
    virtual int getCellInteger(int row, int col) = 0;
    virtual bool getCellBoolean(int row, int col) = 0;
    virtual xd::datetime_t getCellDateTime(int row, int col) = 0;
    virtual int getCellComboSel(int row, int col) = 0;
    virtual bool isNull(int row, int col) = 0;
};


class XdGridColumnInfo
{
public:
    wxString m_col_name;
    xd::objhandle_t m_col_handle;
    int m_col_type;
    int m_col_width;
    int m_col_scale;
    bool m_col_calculated;
    
    kcl::IModelColumnPtr m_model_col;
};


class XdGridCellData
{
public:
    wxString m_str_val;
    int m_int_val;
    double m_dbl_val;
    xd::datetime_t m_datetime_val;
    bool m_bool_val;
    bool m_null;
};

class XdGridRowData
{
public:
    std::vector<XdGridCellData> m_cols;
    xd::rowid_t rowid;
};



WX_DECLARE_STRING_HASH_MAP(int, ColLookupHashMap);


class XdGridModel : public kcl::IModel,
                       public IXdGridModel
{
    XCM_CLASS_NAME("appmain.XdGridModel")
    XCM_BEGIN_INTERFACE_MAP(XdGridModel)
        XCM_INTERFACE_ENTRY(kcl::IModel)
        XCM_INTERFACE_ENTRY(IXdGridModel)
    XCM_END_INTERFACE_MAP()

public:

    XdGridModel();
    virtual ~XdGridModel();

    void setIterator(xd::IIterator* it);
    xd::IIteratorPtr getIterator();

    void setBooleanCheckbox(bool val);

    bool isEofKnown();
    void discoverEof();

    void setGroupBreakExpr(const wxString& new_val);
    wxString getGroupBreakExpr();

    void refreshView();
    bool getColumnCalculated(int model_col);

    xd::rowid_t getRowId(int row);
    void setCurrentRow(int row);

    void setFindHighlight(const wxString& value,
                          bool match_case,
                          bool whole_cell);

    // -- kcl::IModel --
    void initModel(kcl::Grid* grid);
    void refresh();
    void reset();

    void clearCache();

    int getColumnCount();
    kcl::IModelColumnPtr getColumnInfo(int idx);
    int getColumnIndex(const wxString& col_name);

    void getColumnBitmap(int col, wxBitmap* bitmap, int* alignment);
    void setColumnBitmap(int col, const wxBitmap& bitmap, int alignment);

    bool setColumnProperties(int col, kcl::CellProperties* cell_props);
    void getCellProperties(int row, int col, kcl::CellProperties* cell_props);
    bool setCellProperties(int row, int col, kcl::CellProperties* cell_props);

    long getRowData(int row);
    void setRowData(int row, long data);

    void getCellBitmap(int row, int col, wxBitmap* bitmap, int* alignment);
    wxString getCellString(int row, int col);
    double getCellDouble(int row, int col);
    int getCellInteger(int row, int col);
    bool getCellBoolean(int row, int col);
    xd::datetime_t getCellDateTime(int row, int col);
    int getCellComboSel(int row, int col);
    bool isNull(int row, int col);

    bool setCellBitmap(int row, int col, const wxBitmap& bitmap, int alignment);
    bool setCellString(int row, int col, const wxString& value);
    bool setCellDouble(int row, int col, double value);
    bool setCellInteger(int row, int col, int value);
    bool setCellBoolean(int row, int col, bool value);
    bool setCellComboSel(int row, int col, int sel);

    int createColumn(int position, const wxString& name, int type, int width, int scale);
    bool modifyColumn(int position, const wxString& str, int type, int width, int scale);
    bool deleteColumn(int position);
    int insertRow(int position);
    bool deleteRow(int position);
    bool deleteAllRows();
    int getRowCount();
    bool isRowValid(int row);
    bool getGroupBreak();
    bool getGroupBreaksActive();

    void onCursorRowChanged();

public:

    xcm::signal2<wxColor&, wxColor&> sigRequestRowColors;

private:

    void goRow(int row);
    void goRowFwdOnly(int row);
    void copyRowIntoCache(int row);

    XdGridCellData& getCacheCell(int row, int col);
    bool inCache(int row, int col);

private:

    xd::IIteratorPtr m_it;
    bool m_forward_only;
    std::vector<XdGridColumnInfo> m_columns;
    ColLookupHashMap m_column_lookup;
    kcl::Grid* m_grid;
    xd::rowid_t m_cursor_rowid;
    int m_current_row;
    int m_current_mark_row;
    int m_row_count;

    unsigned char* m_break_buf;
    int m_break_buf_len;
    bool m_boolean_checkbox;
    bool m_break_result;
    wxString m_break_expr;
    xd::objhandle_t m_break_handle;

    wxColor m_fgcolor;
    wxColor m_bgcolor;

    wxString m_find_value;
    bool m_find_match_case;
    bool m_find_whole_cell;

    std::vector<XdGridRowData> m_cache;
    XdGridCellData m_empty_cell;
};




#endif

