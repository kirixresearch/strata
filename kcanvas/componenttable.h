/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2006-09-27
 *
 */


#ifndef H_KCANVAS_COMPONENTTABLE_H
#define H_KCANVAS_COMPONENTTABLE_H


#include "graphicsobj.h"
#include "property.h"
#include "range.h"


namespace kcanvas
{


// Canvas Table Component classes

class CompTable;

class CompTableModel : public ITableModel
{
    XCM_CLASS_NAME("kcanvas.CompTableModel")
    XCM_BEGIN_INTERFACE_MAP(CompTableModel)
        XCM_INTERFACE_ENTRY(ITableModel)
    XCM_END_INTERFACE_MAP()

public:

    CompTableModel();
    CompTableModel(const CompTableModel& c);
    virtual ~CompTableModel();

    CompTableModel& operator=(const CompTableModel& c);
    bool operator==(const CompTableModel& c) const;
    bool operator!=(const CompTableModel& c) const;

public:

    static ITableModelPtr create();

    ITableModelPtr clone();
    void copy(ITableModelPtr model);

    bool setRowCount(int count);
    int getRowCount() const;

    bool setColumnCount(int count);
    int getColumnCount() const;

    bool insertRow(int idx, int count = 1);
    bool removeRow(int idx, int count = 1);

    bool insertColumn(int idx, int count = 1);
    bool removeColumn(int idx, int count = 1);

    bool setRowSize(int row, int size);
    int getRowSize(int row);
    
    bool setColumnSize(int col, int size);
    int getColumnSize(int col);

    int getColumnPosByIdx(int col);
    int getColumnIdxByPos(int x);

    int getRowPosByIdx(int row);
    int getRowIdxByPos(int y);

    void getCellIdxByPos(int x, int y, int* row, int* col, bool merged = true);
    void getCellPosByIdx(int row, int col, int* x, int* y, bool merged = true);
    void getCellSizeByIdx(int row, int col, int* w, int* h, bool merged = true);
    void getCellCenterByIdx(int row, int col, int* x, int* y, bool merged = true);

    void addCellProperty(const wxString& prop_name,
                         const PropertyValue& value);

    void addCellProperties(const std::vector<CellProperties>& properties,
                           bool replace);

    bool removeCellProperty(const wxString& prop_name);

    bool setCellProperty(const CellRange& range,
                         const wxString& prop_name,
                         const PropertyValue& value);

    bool setCellProperties(const CellRange& range,
                           const Properties& properties);

    bool getCellProperty(const CellRange& range,
                         const wxString& prop_name,
                         PropertyValue& value) const;

    bool getCellProperty(const CellRange& range,
                         const wxString& prop_name,
                         std::vector<CellProperties>& properties) const;

    bool getCellProperties(const CellRange& range,
                           Properties& properties) const;

    void getCellProperties(const CellRange& range,
                           std::vector<CellProperties>& properties) const;

    void setDefaultCellProperties(Properties& properties);
    void getDefaultCellProperties(Properties& properties) const;

    void mergeCells(const CellRange& range);
    void unmergeCells(const CellRange& range);
    void unmergeAllCells();

    void getMergedCells(std::vector<CellRange>& merges) const;
    bool isMerged(CellRange& range) const;

    bool tag(const wxString& tag);
    bool restore(const wxString& tag);
    void compact();
    void reset();

    void eval(int row, int col, Properties& properties) const;

protected:

    bool isCacheEmpty() const;
    void populateCache();
    void clearCache();

protected:

    // note: cell properties are stored as compositions of cell ranges;
    // so the properties for a given cell are determined by starting with
    // the most recent cell properties that have been set and accumulating
    // additional properties by acquiring them from previously set properties
    // on other cell ranges

    // cell properties; these are the properties of the cells that 
    // define the content of the table; these properties are saved
    std::vector<CellProperties> m_cell_properties;
    Properties m_cell_properties_default;

    // cells that are merged; these are properties that contain the 
    // ranges of cells that are merged; these properties are saved
    std::vector<CellProperties> m_cell_merges;

    // property, row and column size caches
    std::map<int,int> m_cache_row_sizes;
    std::map<int,int> m_cache_col_sizes;

    // row and column count
    int m_row_count;
    int m_col_count;
};


class CompTable : public Component,
                  public ICompTable,
                  public IEdit
{
    XCM_CLASS_NAME("kcanvas.CompTable")
    XCM_BEGIN_INTERFACE_MAP(CompTable)
        XCM_INTERFACE_ENTRY(ICompTable)
        XCM_INTERFACE_ENTRY(IEdit)
        XCM_INTERFACE_CHAIN(Component)
    XCM_END_INTERFACE_MAP()

public:

    CompTable();
    virtual ~CompTable();

    static IComponentPtr create();
    static void initProperties(Properties& properties);

    // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);
    bool setProperty(const wxString& prop_name, const PropertyValue& value);
    bool setProperties(const Properties& properties);
    bool getProperty(const wxString& prop_name, PropertyValue& value) const;
    bool getProperties(Properties& properties) const;
    void extends(wxRect& rect);
    void render(const wxRect& rect = wxRect());

    // ICompTable interface
    void setModel(ITableModelPtr model);
    ITableModelPtr getModel();
    
    bool setRowCount(int count);
    int getRowCount() const;

    bool setColumnCount(int count);
    int getColumnCount() const;

    bool insertRow(int idx, int count = 1);
    bool removeRow(int idx, int count = 1);

    bool insertColumn(int idx, int count = 1);
    bool removeColumn(int idx, int count = 1);

    bool setRowSize(int row, int size);
    int getRowSize(int row);
    
    bool setColumnSize(int col, int size);
    int getColumnSize(int col);

    int getColumnPosByIdx(int col);
    int getColumnIdxByPos(int x);

    int getRowPosByIdx(int row);
    int getRowIdxByPos(int y);

    void getCellIdxByPos(int x, int y, int* row, int* col, bool merged = true);
    void getCellPosByIdx(int row, int col, int* x, int* y, bool merged = true);
    void getCellSizeByIdx(int row, int col, int* w, int* h, bool merged = true);
    void getCellCenterByIdx(int row, int col, int* x, int* y, bool merged = true);

    void addCellProperty(const wxString& prop_name,
                         const PropertyValue& value);

    void addCellProperties(const std::vector<CellProperties>& properties,
                           bool replace);

    bool removeCellProperty(const wxString& prop_name);

    bool setCellProperty(const CellRange& range,
                         const wxString& prop_name,
                         const PropertyValue& value);

    bool setCellProperties(const CellRange& range,
                           const Properties& properties);

    bool getCellProperty(const CellRange& range,
                         const wxString& prop_name,
                         PropertyValue& value) const;

    bool getCellProperty(const CellRange& range,
                         const wxString& prop_name,
                         std::vector<CellProperties>& properties) const;

    bool getCellProperties(const CellRange& range,
                           Properties& properties) const;

    void getCellProperties(const CellRange& range,
                           std::vector<CellProperties>& properties) const;

    void evalCell(int row, int col, Properties& properties) const;

    void mergeCells(const CellRange& range);
    void unmergeCells(const CellRange& range);
    void unmergeAllCells();

    void getMergedCells(std::vector<CellRange>& merges) const;
    bool isMerged(CellRange& range) const;

    bool tag(const wxString& tag);
    bool restore(const wxString& tag);
    void compact();
    void reset();

    void selectCells(const CellRange& range);
    void removeCellSelections(const CellRange& range);
    void removeAllCellSelections();

    void getCellSelections(std::vector<CellRange>& selections, bool cursor = true);
    bool isCellSelected(int row, int col, bool cursor = true);
    bool isColumnSelected(int col) const;
    bool isRowSelected(int row) const;
    bool isTableSelected() const;
    bool hasColumnSelections() const;
    bool hasRowSelections() const;
    bool hasSelections() const;

    void moveCursor(int row_diff, int col_diff, bool clear_selection = false);
    void setCursorPos(int row, int col);
    void getCursorPos(int* row, int* col) const;
    bool isCursorCell(int row, int col) const;

    void clearCells(const CellRange& range,
                    const Properties& properties);
                    
    void copyCells(const CellRange& range1,
                   const CellRange& range2,
                   ICompTablePtr target_table);

    void moveCells(const CellRange& range1,
                   const CellRange& range2,
                   ICompTablePtr target_table);

    bool resizeRowsToContent(const CellRange& range);
    bool resizeColumnsToContent(const CellRange& range);
    void getRowContentSize(const CellRange& range, std::map<int,int>& sizes);
    void getColumnContentSize(const CellRange& range, std::map<int,int>& sizes);

    void addRenderer(const wxString& name, IComponentPtr comp);
    void getRenderers(std::vector<IComponentPtr>& renderers);

    void addEditor(const wxString& name, IComponentPtr comp);
    void getEditors(std::vector<IComponentPtr>& editors);

    void setPreference(int pref, bool active);
    bool isPreference(int pref);

    // IEdit interface
    void beginEdit();
    void endEdit(bool accept);
    bool isEditing() const;

    bool canCut() const;
    bool canCopy() const;
    bool canPaste() const;

    void cut();
    void copy();
    void paste();

    void selectAll();
    void selectNone();

    void clear(bool text = true);

protected:

    void copyCellProperties(const CellRange& range1,
                            const CellRange& range2,
                            bool clear_range1,
                            bool reverse_horz = false,
                            bool reverse_vert = false,
                            CompTable* table = NULL);

    void getCellRangeIdx(const wxRect& rect,
                         CellRange& range,
                         bool merged = true);
    void getCellRangePos(const CellRange& range,
                         wxRect& rect,
                         bool merged = true);

    bool isResizeRowEdge(int x, int y, int* row);
    bool isResizeColumnEdge(int x, int y, int* col);
    bool isMoveCellEdge(int x, int y);
    bool isPopulateCellGrip(int x, int y);

    bool isCursorVisible();
    bool isSelectionContinuous() const;
    void getSelectionRange(CellRange& range) const;
    void getSelectionRangePos(wxRect& rect);
    void filterEditableProperties(Properties& properties) const;

    void invalidateCellRange(const CellRange& range);
    void invalidateCellRange(const std::vector<CellRange>& ranges);
    void invalidateEditor(bool row);
    void invalidateSelection();
    
    void highlightCells(const CellRange& range);
    void removeCellHighlight();
    bool hasCellHighlight();
    void getCellHighlight(CellRange& range);
    void getCellHighlightPos(wxRect& rect);

    void findNextCell(int row, int col, int* row_diff, int* col_diff);
    void findNextUnmergedCell(int row, int col, int* row_diff, int* col_diff);
    void findNextCellSelection(int row, int col, int* row_diff, int* col_diff, bool select_cursor = true);

private:

    // event handlers
    void onEditorInvalidated(IEventPtr evt);
    void onKey(IEventPtr evt);
    void onMouse(IEventPtr evt);
    void onFocus(IEventPtr evt);

    // mouse event handler helpers
    void mouseFeedback(IEventPtr evt);
    void mouseCellEdit(IEventPtr evt);
    void mouseRightClick(IEventPtr evt);
    void mouseResizeCellsToContent(IEventPtr evt);
    void mouseResizeCells(IEventPtr evt);
    void mousePopulateCells(IEventPtr evt);
    void mouseMoveCells(IEventPtr evt);
    void mouseSelectCells(IEventPtr evt);
    
    bool startAction(IEventPtr evt, wxString action);
    bool endAction(IEventPtr evt, wxString action);
    bool isAction(IEventPtr evt, wxString action);
    void resetAction();

private:

    void renderContent(const wxRect& update_rect);
    void renderBorders(const wxRect& update_rect);
    void renderCursor(const wxRect& update_rect);
    void renderSelectionRect(const wxRect& update_rect);
    void renderHighlight(const wxRect& update_rect);
    void renderEditor(const wxRect& update_rect);

    void putCellPropertiesInRenderer(int row, int col);
    void positionEditor(int row, int col);

    void removeLastSelection();
    void getLastSelection(CellRange& range);
    void resizeSelection(int row_diff, int col_diff);

public:

    // table preference enumerations
    // TODO: this allows us to specify preferences that are
    // configuration related, rather than content related,
    // similar to the visible flag in component; ideally, we
    // would store these as properties, but have some type
    // of flag that would prevent them from being saved as
    // part of the content
    enum
    {
        prefNone = 0x0000,
        prefCursor = 0x0001,
        prefHorzGridLines = 0x0002,
        prefVertGridLines = 0x0004,
        prefResizeColumns = 0x0008,
        prefResizeRows = 0x0010,
        prefSelectRows = 0x0020,
        prefSelectColumns = 0x0040,
        prefMoveCells = 0x0080,
        prefPopulateCells = 0x0100,
        prefEditCells = 0x0200
    };

protected:

    // note: cell properties are stored as compositions of cell ranges;
    // so the properties for a given cell are determined by starting with
    // the most recent cell properties that have been set and accumulating
    // additional properties by acquiring them from previously set properties
    // on other cell ranges

    // table model; contains cell ranges of properties
    ITableModelPtr m_model;

    // cell selections; these are temporary properties that contain the 
    // ranges of the cells that are selected; not saved
    std::vector<CellProperties> m_cell_selections;

    // cell highlights
    std::vector<CellRange> m_cell_highlights;

    // editable default cell properties
    Properties m_cell_properties_editable;

    // components that can be used to render or edit a cell; these both have
    // at least one default value, which is added in the constructor; other 
    // renderers and editors can be added to the list
    std::map<wxString, IComponentPtr> m_renderer_list;
    std::map<wxString, IComponentPtr> m_editor_list;
    IComponentPtr m_renderer; // current renderer being used from the list
    IComponentPtr m_editor;   // current editor being used from the list

    // cursor position
    int m_cursor_row;
    int m_cursor_col;

    // preferences
    int m_preferences;
    
    // line thickness
    int m_selection_line_width;          // selection line width in pixels
    
private:

    // mouse variables
    wxString m_mouse_action;    // name of current mouse action
    bool m_mouse_ctrl_start;    // control key state at start of action
    bool m_mouse_shift_start;   // shift key state at start of action
    bool m_mouse_alt_start;     // alt key state at start of action
    
    int m_mouse_x;              // current x position of the mouse
    int m_mouse_y;              // current y position of the mouse
    int m_mouse_x_last;         // last x position of the mouse
    int m_mouse_y_last;         // last y position of the mouse
    int m_mouse_x_start;        // start x position of the mouse
    int m_mouse_y_start;        // start y position of the mouse
    
    int m_mouse_row;            // current row for mouse action; not always used
    int m_mouse_col;            // current column for mouse action; not always used
    int m_mouse_row_start;      // starting row for mouse action; not always used
    int m_mouse_col_start;      // starting column for mouse action; not always used
    int m_mouse_row_size;       // starting row size for mouse action; not always used
    int m_mouse_col_size;       // starting column size for mouse action; not always used
    
    // cursor selection index; used to keep track of which
    // selection the cursor is in so it can flow through
    // the selection areas
    int m_cursor_selection_idx;

    // flag used to store if table just received the focus
    bool m_focus_set;
    
    // flags used to track editing states
    bool m_editing;
    bool m_cell_editing;
};


class CompTableHeader : public CompTable
{
    XCM_CLASS_NAME("kcanvas.CompTableHeader")
    XCM_BEGIN_INTERFACE_MAP(CompTableHeader)
        XCM_INTERFACE_CHAIN(kcanvas::CompTable)
    XCM_END_INTERFACE_MAP()

public:

    CompTableHeader(int type);
    virtual ~CompTableHeader();

    static IComponentPtr create(int type);
    
    // IComponent interface
    IComponentPtr clone();
    void copy(IComponentPtr component);

public:

    // IComponent
    void render(const wxRect& rect = wxRect());

private:

    void onResizeRowToContent(IEventPtr evt);
    void onResizeColumnToContent(IEventPtr evt);

public:

   // enum for horizontal or vertical type
    enum
    {
        Column = 0,
        Row = 1
    };

private:
    
    // identifier to indicate whether the control is
    // for columns or rows
    int m_type;
};


}; // namespace kcanvas


#endif

