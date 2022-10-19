/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2001-12-26
 *
 */


#ifndef __KCL_GRID_H
#define __KCL_GRID_H


#include <wx/wx.h>
#include <wx/hashmap.h>
#include <vector>
#include <kl/xcm.h>


namespace kcl
{


// forward declarations

xcm_interface IModelColumn;
xcm_interface IModel;
class Grid;


// smart pointer declarations

XCM_DECLARE_SMARTPTR(IModelColumn)
XCM_DECLARE_SMARTPTR(IModel)


// event declarations/classes

class GridEvent : public wxNotifyEvent
{
public:

    GridEvent() : wxNotifyEvent()
    {
        m_row = -1;
        m_col = -1;
        m_dest_row = -1;
        m_dest_col = -1;
        m_fgcolor = *wxBLACK;
        m_bgcolor = *wxWHITE;
        m_bool_val = false;
        m_double_val = 0.0;
        m_user_event = true;
        m_edit_cancelled = false;
    }

    GridEvent(int id,
              wxEventType type,
              wxObject* obj) : wxNotifyEvent(type, id)
    {
        m_row = -1;
        m_col = -1;
        m_dest_row = -1;
        m_dest_col = -1;
        m_fgcolor = *wxBLACK;
        m_bgcolor = *wxWHITE;
        m_bool_val = false;
        m_double_val = 0.0;
        m_user_event = true;
        m_edit_cancelled = false;
        SetEventObject(obj);
    }

    GridEvent(const GridEvent& r) : wxNotifyEvent(r)
    {
        m_row = r.m_row;
        m_col = r.m_col;
        m_dest_row = r.m_dest_row;
        m_dest_col = r.m_dest_col;
        m_fgcolor = r.m_fgcolor;
        m_bgcolor = r.m_bgcolor;
        m_bool_val = r.m_bool_val;
        m_double_val = r.m_double_val;
        m_user_event = r.m_user_event;
        m_edit_cancelled = r.m_edit_cancelled;
        m_key_event = r.m_key_event;
    }

    int GetRow() const
    {
        return m_row;
    }

    int GetColumn() const
    {
        return m_col;
    }

    int GetDestinationRow() const
    {
        return m_dest_row;
    }

    int GetDestinationColumn() const
    {
        return m_dest_col;
    }

    double GetDouble() const
    {
        return m_double_val;
    }

    bool GetBoolean() const
    {
        return m_bool_val;
    }

    bool GetEditCancelled() const
    {
        return m_edit_cancelled;
    }

    bool GetUserEvent() const
    {
        return m_user_event;
    }

    wxColor GetForegroundColor() const
    {
        return m_fgcolor;
    }

    wxColor GetBackgroundColor() const
    {
        return m_bgcolor;
    }

    const wxKeyEvent& GetKeyEvent() const
    {
        return m_key_event;
    }

    void SetRow(int new_val)
    {
        m_row = new_val;
    }

    void SetColumn(int new_val)
    {
        m_col = new_val;
    }

    void SetDestinationRow(int new_val)
    {
        m_dest_row = new_val;
    }

    void SetDestinationColumn(int new_val)
    {
        m_dest_col = new_val;
    }

    void SetDouble(double new_val)
    {
        m_double_val = new_val;
    }

    void SetBoolean(bool new_val)
    {
        m_bool_val = new_val;
    }

    void SetEditCancelled(bool new_val)
    {
        m_edit_cancelled = new_val;
    }

    void SetUserEvent(bool new_val)
    {
        m_user_event = new_val;
    }

    void SetKeyEvent(const wxKeyEvent& new_val)
    {
        m_key_event = new_val;
    }
    
    void SetForegroundColor(const wxColor& new_val)
    {
        m_fgcolor = new_val;
    }
    
    void SetBackgroundColor(const wxColor& new_val)
    {
        m_bgcolor = new_val;
    }

public:

    int m_row;
    int m_col;
    int m_dest_row;
    int m_dest_col;

    wxColor m_fgcolor;
    wxColor m_bgcolor;
    double m_double_val;
    bool m_bool_val;
    bool m_user_event;
    bool m_edit_cancelled;

    wxKeyEvent m_key_event;
};




// cell properties

struct CellProperties
{
    enum MaskValues
    {
        cpmaskCtrlType = 0x001,
        cpmaskFgColor = 0x002,
        cpmaskBgColor = 0x004,
        cpmaskCbChoices = 0x008,
        cpmaskAlignment = 0x010,
        cpmaskEditable = 0x020,
        cpmaskTextWrap = 0x040,
        cpmaskBitmapAlignment = 0x080,
        cpmaskVisible = 0x100
    };

    CellProperties()
    {
        mask = 0;
    }

    wxColour fgcolor;
    wxColour bgcolor;
    std::vector<wxString> cbchoices;
    int mask;
    int ctrltype;
    int alignment;
    int text_wrap;
    int bitmap_alignment;
    bool editable;
    bool visible;
};


// selection

class SelectionRect
{
public:
    int m_start_row;
    int m_start_col;
    int m_end_row;
    int m_end_col;
};


xcm_interface IModelColumn : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcl.IModelColumn");

public:

    virtual void setName(const wxString& newval) = 0;
    virtual wxString getName() = 0;

    virtual void setType(int newval) = 0;
    virtual int getType() = 0;

    virtual void setWidth(int newval) = 0;
    virtual int getWidth() = 0;

    virtual void setScale(int newval) = 0;
    virtual int getScale() = 0;
};


xcm_interface IModel : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcl.IModel")

public:

    virtual void initModel(Grid* grid) = 0;
    virtual void refresh() = 0;
    virtual void reset() = 0;

    virtual int getColumnCount() = 0;
    virtual IModelColumnPtr getColumnInfo(int col_idx) = 0;
    virtual int getColumnIndex(const wxString& col_name) = 0;

    virtual void getColumnBitmap(int model_col, wxBitmap* bitmap, int* alignment) = 0;
    virtual void setColumnBitmap(int model_col, const wxBitmap& bitmap, int alignment) = 0;

    virtual bool setColumnProperties(int col, CellProperties* cell_props) = 0;
    virtual void getCellProperties(int row, int col, CellProperties* cell_props) = 0;
    virtual bool setCellProperties(int row, int col, CellProperties* cell_props) = 0;

    virtual long getRowData(int row) = 0;
    virtual void setRowData(int row, long data) = 0;

    virtual void getCellBitmap(int row, int col, wxBitmap* bitmap, int* alignment) = 0;
    virtual wxString getCellString(int row, int col) = 0;
    virtual double getCellDouble(int row, int col) = 0;
    virtual int getCellInteger(int row, int col) = 0;
    virtual bool getCellBoolean(int row, int col) = 0;
    virtual int getCellComboSel(int row, int col) = 0;
    virtual bool isNull(int row, int col) = 0;

    virtual bool setCellBitmap(int row, int col, const wxBitmap& bitmap, int alignment) = 0;
    virtual bool setCellString(int row, int col, const wxString& value) = 0;
    virtual bool setCellDouble(int row, int col, double value) = 0;
    virtual bool setCellInteger(int row, int col, int value) = 0;
    virtual bool setCellBoolean(int row, int col, bool value) = 0;
    virtual bool setCellComboSel(int row, int col, int sel) = 0;

    virtual int createColumn(int position, const wxString& str, int type, int width, int scale) = 0;
    virtual bool modifyColumn(int position, const wxString& str, int type, int width, int scale) = 0;
    virtual bool deleteColumn(int position) = 0;
    virtual int insertRow(int position) = 0;
    virtual bool deleteRow(int position) = 0;
    virtual bool deleteAllRows() = 0;
    virtual int getRowCount() = 0;
    virtual bool isRowValid(int row) = 0;
    virtual bool getGroupBreak() = 0;
    virtual bool getGroupBreaksActive() = 0;

    virtual void onCursorRowChanged() = 0;
};



class ModelColumn : public IModelColumn
{
    XCM_CLASS_NAME("kcl.ModelColumn")
    XCM_BEGIN_INTERFACE_MAP(ModelColumn)
        XCM_INTERFACE_ENTRY(kcl::IModelColumn)
    XCM_END_INTERFACE_MAP()

private:
    wxString m_name;
    int m_type;
    int m_width;
    int m_scale;

public:

    ModelColumn();
    virtual ~ModelColumn();

    void setName(const wxString& newval);
    wxString getName();
    void setType(int newval);
    int getType();
    void setWidth(int newval);
    int getWidth();
    void setScale(int newval);
    int getScale();
};


class TestModel : public IModel
{
    XCM_CLASS_NAME("kcl.TestModel")
    XCM_BEGIN_INTERFACE_MAP(TestModel)
        XCM_INTERFACE_ENTRY(kcl::IModel)
    XCM_END_INTERFACE_MAP()

    int m_row;

public:

    TestModel();

    void initModel(kcl::Grid* grid);
    void refresh();
    void reset();

    int getColumnCount();
    IModelColumnPtr getColumnInfo(int idx);
    int getColumnIndex(const wxString& col_name);

    void getColumnBitmap(int col_idx, wxBitmap* bitmap, int* alignment);
    void setColumnBitmap(int col, const wxBitmap& bitmap, int alignment);

    bool setColumnProperties(int col, CellProperties* cell_props);
    void getCellProperties(int row, int col, CellProperties* cell_props);
    bool setCellProperties(int row, int col, CellProperties* cell_props);

    long getRowData(int row);
    void setRowData(int row, long data);

    void getCellBitmap(int row, int col, wxBitmap* bitmap, int* alignment);
    wxString getCellString(int row, int col);
    double getCellDouble(int row, int col);
    int getCellInteger(int row, int col);
    bool getCellBoolean(int row, int col);
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
};


class ViewColumn
{
public:
    wxBitmap m_bitmap;
    int m_bitmap_alignment;

    wxColor m_fgcolor;
    wxColor m_bgcolor;

    wxString m_caption;
    wxString m_colname;
    int m_dip_pixwidth;
    int m_pixwidth;
    int m_alignment;
    int m_text_wrap;
    int m_modelcol;
    int m_prop_size;
    bool m_separator;
    bool m_draw;
    bool m_shaded;
    bool m_resizable;
};

class CellData : public CellProperties
{
public:

    wxString m_strvalue;
    wxBitmap m_bitmap;
    int m_type;
    int m_width;
    int m_scale;
    int m_intvalue;
    double m_dblvalue;
    bool m_boolvalue;
    bool m_null;
    
    // these are used for row dragging only
    int m_row;
    int m_col;
};

class RowData
{
public:
    std::vector<CellData> m_coldata;
    bool m_blank;
    int m_model_row;
};

class ColumnRange
{
public:
    int xoff;
    int width;
    int viewcol_idx;
    ViewColumn* viewcol;
};

class Selection
{
private:
    std::vector<SelectionRect*> m_pieces;

public:

    Selection();
    ~Selection();

    bool getCellSelected(int row, int col);
    bool getColumnSelected(int col);
    bool setColumnSelected(int col, bool selected);
    bool getRowSelected(int row);
    bool setRowSelected(int row, bool selected);
    int lookupColumnSelection(int col);
    int lookupRowSelection(int col);
    bool columnContainsSelection(int col);
    bool rowContainsSelection(int row);

    SelectionRect* addSelection();
    bool removeSelection(SelectionRect* sel_rect);
    int getSelectionCount();
    SelectionRect* getSelection(int idx);
    void clear();
};


WX_DECLARE_STRING_HASH_MAP(int, wxStringToColIdxHashMap);

class Grid : public wxNavigationEnabled<wxControl>
{
friend class GridTextCtrl;
friend class GridChoiceCtrl;
friend class GridComboCtrl;
friend class GridDataObject;
friend class GridDataDropTarget;
friend class GridDataDropTargetTimer;

public:

    // constants

    enum GridDataType
    {
        typeInvalid = 0,
        typeUndefined = 1,
        typeCharacter = 2,
        typeDouble = 3,
        typeInteger = 4,
        typeDate = 5,
        typeDateTime = 6,
        typeBoolean = 7,
    };

    enum CellControlType
    {
        ctrltypeText = 0,
        ctrltypeComboBox = 1,
        ctrltypeDropList = 2,
        ctrltypeCheckBox = 3,
        ctrltypeButton = 4,
        ctrltypeColor = 5,
        ctrltypeProgress = 6
    };
    
    enum RefreshLevel
    {
        refreshScrollbars = 0x01,
        refreshData = 0x02,
        refreshPaint = 0x04,
        refreshColumnView = 0x08,
        refreshAll = refreshScrollbars | refreshData | refreshPaint
    };

    enum AlignmentValue
    {
        alignDefault = 0,
        alignLeft = 1,
        alignCenter = 2,
        alignRight = 3
    };
    
    enum GridTextWrapValue
    {
        wrapDefault = 0,
        wrapOn = 1,
        wrapOff = 2
    };

    enum GridOption
    {
        optSelect = 0x00001,
        optEdit = 0x00002,
        optColumnMove = 0x00004,
        optColumnResize = 0x00008,
        optRowResize = 0x00010,
        optHorzGridLines = 0x00020,
        optVertGridLines = 0x00040,
        optGhostRow = 0x00080,
        optContinuousScrolling = 0x00100,
        optEditBoxBorder = 0x00200,
        optSingleClickEdit = 0x00400,
        optSelectionList = 0x00800,
        optInvalidAreaClickable = 0x01000,
        optCaptionsEditable = 0x02000,
        optFullGreenbar = 0x04000,
        optCursorRowMarker = 0x08000,
        optActivateHyperlinks = 0x10000
    };

    enum GridVisibleState
    {
        stateHidden = 0x01,
        stateDisabled = 0x02,
        stateVisible = 0x03
    };

    enum GridCursorType
    {
        cursorNormal = 0x00,
        cursorThin = 0x01,
        cursorHighlight = 0x02,
        cursorRowHighlight = 0x03,
        cursorRowOutline = 0x04
    };



public:

    Grid(wxWindow* parent,
         wxWindowID id,
         const wxPoint& pos = wxDefaultPosition,
         const wxSize& size = wxDefaultSize,
         long style = 0);

    Grid();
    ~Grid();

    bool AcceptsFocus() const { return true; }

    bool Create(wxWindow* parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                int visible_state = stateVisible);

    void createDefaultView();

    void setModel(IModelPtr pModel);
    IModelPtr getModel();
    void reset();

    void setDragFormat(const wxString& format);
    void setOptionState(int option_mask, bool state);
    bool getOptionState(int option);
    void setBorderType(int border_type);
    void setVisibleState(int visible_state);
    int getVisibleState();
    void setCursorVisible(bool visible);
    void setCursorType(int cursor_type);
    int getCursorType();
    bool SetFont(const wxFont& font);
    bool setCaptionFont(const wxFont& font);
    bool setOverlayFont(const wxFont& font);
    void refresh(unsigned int refresh_level);
    void refreshColumn(unsigned int refresh_level, int column_idx);
    void refreshModel();
    void updateData();

    bool getRowRect(int row, wxRect& rect);
    bool getColumnRect(int col, wxRect& rect);

    void setRowHeight(int row_height);
    int getRowHeight();

    void beginEdit();
    void beginEdit(const wxString& default_text);
    void beginCaptionEdit(int view_col);
    void endEdit(bool accept);
    bool isEditing();

    bool hitTest(int x, int y, int* model_row, int* view_col);

    int getRowOffset();
    void setRowOffset(int new_value);

    int getHorizontalOffset();
    void setHorizontalOffset(int new_value);

    int getColumnFromXpos(int xpos);
    int getCursorColumn();
    int getCursorRow();
    int getColumnCount();
    int getRowCount();
    int getKnownRowCount();
    int getVisibleRowCount();
    int getVisibleGroupBreakCount();

    int createModelColumn(int position, const wxString& name, int type, int width, int scale);
    bool setModelColumnProperties(int model_col, kcl::CellProperties* props);
    void getCellProperties(int row, int model_col, CellProperties* cell_props);
    bool setCellProperties(int row, int model_col, kcl::CellProperties* props);

    void insertRow(int position);
    bool deleteRow(int position);
    bool deleteAllRows();

    void setRowLabelSize(int new_value);
    int getRowLabelSize();

    void setHeaderSize(int new_val);
    int getHeaderSize();

    void setGhostRowText(const wxString& new_value);
    wxString getGhostRowText();

    void setOverlayText(const wxString& new_value);
    wxString getOverlayText();

    void getColumnBitmap(int col, wxBitmap* bitmap, int* alignment);
    void setColumnBitmap(int col, const wxBitmap& bitmap, int alignment = kcl::Grid::alignCenter);

    void setCellBitmap(int row, int model_col, const wxBitmap& bitmap, int alignment = kcl::Grid::alignDefault);
    void setCellString(int row, int model_col, const wxString& value);
    void setCellDouble(int row, int model_col, double value);
    void setCellInteger(int row, int model_col, int value);
    void setCellBoolean(int row, int model_col, bool value);
    void setCellComboSel(int row, int model_col, int sel);

    bool setCellData(int model_row, int model_col, CellData* data);
    CellData* getCellData(int model_row, int model_col);

    void setRowData(int row, long data);
    long getRowData(int row);

    void getCellBitmap(int row, int model_col, wxBitmap* bitmap, int* alignment);
    wxString getCellString(int row, int model_col);
    double getCellDouble(int row, int model_col);
    int  getCellInteger(int row, int model_col);
    bool getCellBoolean(int row, int model_col);
    int getCellComboSel(int row, int model_col);

    void drawColumnDropHighlight(int col);
    void drawRowDropHighlight(int row, int style);

    bool isCellSelected(int row, int col);
    bool isColumnSelected(int col);
    bool isRowSelected(int row);
    void copySelection();

    void clearSelection();
    bool addSelection(SelectionRect* selrect);
    bool setColumnSelected(int col, bool selected);
    bool setRowSelected(int row, bool selected);
    void selectAll();
    
    int getSelectionCount();
    void getSelection(unsigned int idx, SelectionRect* selrect);
    void getAllSelections(std::vector<SelectionRect>& selrect);
    bool removeSelection(unsigned int idx);

    void hideColumn(int col);
    int insertColumn(int position, int modelcol_idx);
    int insertColumnSeparator(int position);
    void setColumn(int position, int modelcol_idx);
    void hideAllColumns();

    void moveCursor(int row, int col, bool repaint = true);
    void scrollToBottom();
    void scrollToTop();
    bool scrollVertToCursor();
    bool scrollHorzToCursor();
    bool scrollVertToRow(int row);
    bool scrollHorzToColumn(int col);    
    bool isCursorVisible();

    void autoColumnResize(int resize_col);
    void setColumnSize(int col, int new_value);
    void setColumnProportionalSize(int col, int new_value);
    void setColumnCaption(int col, const wxString& new_value);
    void setColumnAlignment(int col, int new_value);
    void setColumnShaded(int col, bool new_value);
    void setColumnColors(int col, const wxColor& fg, const wxColor& bg);
    void setColumnTextWrapping(int col, int new_value);
    void setColumnResizable(int col, bool new_value);

    void setSelectBackgroundColor(const wxColor& bg);
    void setSelectForegroundColor(const wxColor& fg);
    void setDefaultBackgroundColor(const wxColor& bg);
    void setDefaultForegroundColor(const wxColor& fg);
    void setDefaultLineColor(const wxColor& lc);
    void setDefaultInvalidAreaColor(const wxColor& iac);
    void setGreenBarColor(const wxColor& gbc);
    void setGreenBarInterval(int new_value);

    int getColumnSize(int col);
    int getColumnProportionalSize(int col);
    wxString getColumnCaption(int col);
    int getColumnAlignment(int col);
    bool getColumnShaded(int col);
    void getColumnColors(int col, wxColor& fg, wxColor& bg);
    int getColumnTextWrapping(int col);
    int getColumnDefaultAlignment(int col);
    
    wxColor getDefaultBackgroundColor();
    wxColor getDefaultForegroundColor();
    wxColor getDefaultLineColor();
    wxColor getDefaultInvalidAreaColor();
    wxColor getGreenBarColor();
    int getGreenBarInterval();

    int getColumnViewIdx(int model_col);
    int getColumnViewIdxByName(const wxString& col_name);
    int getColumnModelIdx(int view_col);
    int getColumnModelIdxByName(const wxString& col_name);

private:

    int fromDIP(int d);
    int toDIP(int d);

    //void drawChiselRectangle(int x1, int y1, int width, int height, bool selected);
    void drawColumnHeaderRectangle(int x1, int y1, int width, int height, bool selected);
    void drawRowGripperRectangle(int x1, int y1, int width, int height, bool selected);
    void getBoundingBoxRect(int row1, int col1, int row2, int col2, wxRect* rect);
    void drawBoundingBox(int row1, int col1, int row2, int col2, bool thin = false, bool stipple = false);
    void drawDisabledStipple();
    bool rectInRect(wxRect* rect, int x, int y, int width, int height);

    void construct();
    void initGui();
    void initFont();
    void calcColumnWidths();
    void render(wxRect* update_rect = NULL, bool cursor_visible = true);
    void repaint(wxDC* dc = NULL, wxRect* rect = NULL);
    void scrollHorz(int pixels);
    void scrollVert(int rows);
    void updateScrollbars(int scrollbar = -1);
    
    void getCellRect(int row, int col, wxRect* rect);
    void moveCol(ViewColumn* from_col, ViewColumn* to_col);
    void beginEditWithPositioning(int x, int y);
    void doGhostRowInsert();
    void doInvalidAreaInsert(int mouse_x, int mouse_y);
    bool doButtonClick(int mouse_x, int mouse_y);
    void autoColumnResizeInternal(std::vector<int>& columns);
    bool allocBitmap(int width, int height);
    void removeToolTip();

    // event dispatchers
    bool fireEvent(int type, int row, int col, bool* allowed = NULL);
    bool fireEvent(int event_type, GridEvent& evt, bool* allowed = NULL);

private:

    // extra event handlers

    void onEditDestroy(const wxString& value,
                       int last_key_code,
                       bool focus_lost,
                       bool user_action);

    void onEndEditCleanup(wxCommandEvent& evt);
    void onEditChanged(const wxString& value);
    void onCellClicked(int row, int col);

protected:

    // event handlers

    void onSize(wxSizeEvent& evt);
    void onScroll(wxScrollWinEvent& evt);
    void onMouse(wxMouseEvent& evt);
    void onKeyDown(wxKeyEvent& evt);
    void onChar(wxKeyEvent& evt);
    void onPaint(wxPaintEvent& evt);
    void onEraseBackground(wxEraseEvent& evt);
    void onComboSelectionChanged(wxCommandEvent& evt);
    void onScrollTimer(wxTimerEvent& evt);

protected:

    wxString m_drag_format;

private:

    enum
    {
        actionNone = 0,
        actionResizeCol,
        actionResizeRow,
        actionMoveCol,
        actionSelect,
        actionSelectRows,
        actionSelectCol,
        actionPressButton,
        actionPressColor
    };

    // data member variables
    IModel* m_model;
    std::vector<ColumnRange> m_colranges;
    std::vector<RowData> m_rowdata;
    std::vector<ViewColumn*> m_viewcols;
    wxStringToColIdxHashMap m_viewcol_lookup;
    Selection m_selection;
    unsigned int m_options;
    int m_visible_state;
    int m_cursor_type;
    int m_border_type;
    bool m_cursor_visible;
    bool m_mask_paint;
    bool m_gui_initialized;
    bool m_destroying;
    bool m_last_edit_allowed;
    bool m_caption_editing;
    ViewColumn* m_caption_edit_col;
    
    // dimensions and positions

    wxSize m_render_size;
    int m_cliheight;
    int m_cliwidth;

    int m_total_pwidth;     // total proportional width
    int m_total_width;      // total pixel width
    int m_invalid_yoffset;
    int m_frozen_width;     // total frozen width

    int m_dip_header_height;
    int m_header_height;

    int m_dip_rowlabel_width;
    int m_rowlabel_width;

    int m_dip_row_height;
    int m_row_height;

    int m_xoff;
    int m_row_offset;
    int m_row_count;
    int m_row_break_count;
    int m_cursor_row;
    int m_cursor_col;
    int m_cursor_modelcol;
    int m_greenbar_interval;

    // graphic/ui member variables
    wxMemoryDC m_memdc;
    wxBitmap m_bmp;
    wxFont m_caption_font;
    wxFont m_font;
    wxFont m_underlined_font;
    wxFont m_overlay_font;
    int m_bmp_alloc_width;
    int m_bmp_alloc_height;

    wxBrush m_white_brush;
    wxBrush m_grey_brush;
    wxPen m_grey_pen;
    wxPen m_darkgrey_pen;
    wxPen m_gridline_pen;
    wxPen m_stipple_pen;

    wxColor m_def_bgcolor;            // grid's default background color
    wxColor m_def_fgcolor;            // grid's default foreground (text) color
    wxColor m_def_gridline_color;     // grid's default gridline color
    wxColor m_def_invalid_area_color; // grid's default invalid area color
    wxColor m_def_greenbar_color;     // grid's default greenbar color
    wxColor m_hc_barfg_color;         // highlight cursor's bar foreground color
    wxColor m_hc_barbg_color;         // highlight cursor's bar background color
    wxColor m_hc_cellfg_color;        // highlight cursor's cell foreground color
    wxColor m_hc_cellbg_color;        // highlight cursor's cell background color
    wxColor m_select_fgcolor;         // selection foreground color
    wxColor m_select_bgcolor;         // selection background color
    wxColor m_base_color;             // 3d face color

    CellData m_empty_cell;
    RowData m_empty_row;

    static wxBitmap m_bmp_checkbox_on;
    static wxBitmap m_bmp_checkbox_off;
    static wxBitmap m_bmp_checkbox_neutral;
    static wxBitmap m_bmp_cursorrowmarker;
    static wxBitmap m_bmp_ghostrowmarker;

    wxString m_ghostrow_text;
    wxString m_overlay_text;

    wxTimer m_scroll_timer;
    int m_scroll_timer_voffset;
    int m_scroll_timer_hoffset;
    
    // in-place editing variables
    wxControl* m_control;
    wxString m_edit_value;
    int m_edit_combosel;
    int m_last_key_code;

    // mouse event variables
    int m_mouse_action;         // action indicator
    ViewColumn* m_action_col;   // action column
    int m_action_colxoff;       // x offset of the column at start of action
    int m_action_xpos;          // x pos of the mouse at start of action
    int m_action_ypos;          // y pos of the mouse at start of action
    int m_action_lastxpos;      // x pos of the mouse during the last event handling
    int m_action_lastypos;      // y pos of the mouse during the last event handling
    int m_action_viewcol;       // viewcol number of the action column
    int m_action_row;           // row number of the action

    int m_last_tooltip_row;     // last tooltip row pos - used for tooltip tracking
    int m_last_tooltip_col;     // last tooltip row col - used for tooltip tracking
    
    SelectionRect* m_action_selrect;
    wxMemoryDC m_movecol_dc;
    wxBitmap m_movecol_bmp;

    DECLARE_EVENT_TABLE()
};



};  // namespace kcl




// events

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_SELECTION_CHANGE,        2100)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_BEGIN_EDIT,              2101)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_END_EDIT,                2102)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_EDIT_CHANGE,             2103)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_BEGIN_CAPTION_EDIT,      2104)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_END_CAPTION_EDIT,        2105)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_CELL_LEFT_CLICK,         2106)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_CELL_RIGHT_CLICK,        2107)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_CELL_LEFT_DCLICK,        2108)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_CELL_RIGHT_DCLICK,       2109)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_LEFT_CLICK,       2110)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_RIGHT_CLICK,      2111)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_ROW_LEFT_CLICK,          2112)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_ROW_RIGHT_CLICK,         2113)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_ROW_RESIZE,              2114)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_ROW_SASH_DCLICK,         2115)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_PRE_CURSOR_MOVE,         2116)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_CURSOR_MOVE,             2117)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_RESIZE,           2118)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_HIDE,             2119)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_INSERT,           2120)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_COLUMN_MOVE,             2121)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_MODEL_CHANGE,            2122)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_PRE_GHOST_ROW_INSERT,    2123)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_GHOST_ROW_INSERT,        2124)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_PRE_INVALID_AREA_INSERT, 2125)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_INVALID_AREA_INSERT,     2126)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_INVALID_AREA_CLICK,      2127)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_BUTTON_CLICK,            2128)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_COLOR_CLICK,             2129)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_KEY_DOWN,                2130)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_NEED_TOOLTIP_TEXT,       2131)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_BEGIN_DRAG,              2132)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_HSCROLL,                 2133)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_VSCROLL,                 2134)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_LINK_LEFTCLICK,          2135)
    DECLARE_EVENT_TYPE(wxEVT_KCLGRID_LINK_MIDDLECLICK,        2136)    
END_DECLARE_EVENT_TYPES()


typedef void (wxEvtHandler::*wxKclGridEventFunction)(kcl::GridEvent&);


#define EVT_KCLGRID_SELECTION_CHANGE(fn)        DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_SELECTION_CHANGE, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_BEGIN_EDIT(fn)              DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_BEGIN_EDIT, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_END_EDIT(fn)                DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_END_EDIT, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_EDIT_CHANGE(fn)             DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_EDIT_CHANGE, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_BEGIN_CAPTION_EDIT(fn)      DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_BEGIN_CAPTION_EDIT, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_END_CAPTION_EDIT(fn)        DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_END_CAPTION_EDIT, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_CELL_LEFT_CLICK(fn)         DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_CELL_LEFT_CLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_CELL_RIGHT_CLICK(fn)        DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_CELL_RIGHT_CLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_CELL_LEFT_DCLICK(fn)        DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_CELL_LEFT_DCLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_CELL_RIGHT_DCLICK(fn)       DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_CELL_RIGHT_DCLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_COLUMN_LEFT_CLICK(fn)       DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_COLUMN_LEFT_CLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_COLUMN_RIGHT_CLICK(fn)      DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_COLUMN_RIGHT_CLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_ROW_LEFT_CLICK(fn)          DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_ROW_LEFT_CLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_ROW_RIGHT_CLICK(fn)         DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_ROW_RIGHT_CLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_ROW_RESIZE(fn)              DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_ROW_RESIZE, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_ROW_SASH_DCLICK(fn)         DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_ROW_SASH_DCLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_PRE_CURSOR_MOVE(fn)         DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_PRE_CURSOR_MOVE, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_CURSOR_MOVE(fn)             DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_CURSOR_MOVE, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_COLUMN_RESIZE(fn)           DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_COLUMN_RESIZE, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_COLUMN_HIDE(fn)             DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_COLUMN_HIDE, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_COLUMN_INSERT(fn)           DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_COLUMN_INSERT, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_COLUMN_MOVE(fn)             DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_COLUMN_MOVE, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_MODEL_CHANGE(fn)            DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_MODEL_CHANGE, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_PRE_GHOST_ROW_INSERT(fn)    DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_PRE_GHOST_ROW_INSERT, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_GHOST_ROW_INSERT(fn)        DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_GHOST_ROW_INSERT, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_PRE_INVALID_AREA_INSERT(fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_PRE_INVALID_AREA_INSERT, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_INVALID_AREA_INSERT(fn)     DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_INVALID_AREA_INSERT, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_INVALID_AREA_CLICK(fn)      DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_INVALID_AREA_CLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_BUTTON_CLICK(fn)            DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_BUTTON_CLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_COLOR_CLICK(fn)             DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_COLOR_CLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_KEY_DOWN(fn)                DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_KEY_DOWN, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_NEED_TOOLTIP_TEXT(fn)       DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_NEED_TOOLTIP_TEXT, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_BEGIN_DRAG(fn)              DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_BEGIN_DRAG, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_HSCROLL(fn)                 DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_HSCROLL, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_VSCROLL(fn)                 DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_VSCROLL, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_LINK_LEFTCLICK(fn)          DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_LINK_LEFTCLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),
#define EVT_KCLGRID_LINK_MIDDLECLICK(fn)        DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLGRID_LINK_MIDDLECLICK, -1, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclGridEventFunction)&fn, NULL),


#endif

