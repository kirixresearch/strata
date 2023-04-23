/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2004-06-26
 *
 */


#ifndef H_KCANVAS_KCANVAS_H
#define H_KCANVAS_KCANVAS_H


namespace kcanvas
{


// forward class declarations
class Color;
class Pen;
class Brush;
class Font;
class PropertyValue;
class Property;
class Properties;
class CellProperties;
class CellRange;


// canvas unit of measure
const int CANVAS_MODEL_DPI = 1440; // dots per inch
const int CANVAS_SCREEN_DPI = 96;  // dots per inch

// canvas maximum number of layout interations before
// layout is considered complete
const int CANVAS_LAYOUT_ITERATION_MAX   = 100;

// zoom constants
const int ZOOM_SCALE = 100;
const int ZOOM_CHANGE = (int)(ZOOM_SCALE*0.05);   // 5%
const int ZOOM_MIN = (int)(ZOOM_SCALE*0.25);      // 25%
const int ZOOM_MAX = (int)(ZOOM_SCALE*10);        // 1000%

// scrolling constants
const int SCROLL_RATE_HORZ = 5;
const int SCROLL_RATE_VERT = 5;
const int SCROLL_LINE_PERC = 5;
const int SCROLL_PAGE_PERC = 95;
const int SCROLL_MOUSE_PERC = 15;

// component constants
const int COMPONENT_MIN_WIDTH = (int)(CANVAS_MODEL_DPI*0.25);
const int COMPONENT_MIN_HEIGHT = (int)(CANVAS_MODEL_DPI*0.25);

const int COMPONENT_MOVE_LARGE = (int)(CANVAS_MODEL_DPI*0.0625);
const int COMPONENT_MOVE_SMALL = (int)(CANVAS_MODEL_DPI*0.01);

const int COMPONENT_PASTE_OFFSET_X = (int)(CANVAS_MODEL_DPI*0.25);
const int COMPONENT_PASTE_OFFSET_Y = (int)(CANVAS_MODEL_DPI*0.25);

// table constants
const int TABLE_CELL_WIDTH = (int)(CANVAS_MODEL_DPI*0.75);
const int TABLE_CELL_HEIGHT = (int)(CANVAS_MODEL_DPI*0.1875);

const int TABLE_MIN_CELL_WIDTH = (int)(CANVAS_MODEL_DPI*0.03125);
const int TABLE_MIN_CELL_HEIGHT = (int)(CANVAS_MODEL_DPI*0.03125);

const int TABLE_MOUSE_RESIZE_TOL = 5;     // mouse tolerance in pixels for row/column resizing
const int TABLE_MOUSE_SELECTION_TOL = 3;  // mouse tolerance in pixels for selection
const int TABLE_MOUSE_GRIP_TOL = 4;       // mouse tolerance in pixels for cursor grip

// textbox constants
const int TEXTBOX_LINE_SPACING = 100;     // percentage of vertical text size
const int TEXTBOX_MIN_LINE_SPACING = 100; // percentage of vertical text size
const int TEXTBOX_MAX_LINE_SPACING = 300; // percentage of vertical text size
const int TEXTBOX_HORZ_PIXEL_BUFF = 2;    // pixels
const int TEXTBOX_VERT_PIXEL_BUFF = 2;    // pixels
const int TEXTBOX_CURSOR_WIDTH = 2;       // pixels

// cursor constants
const wxString CURSOR_INVALID       = wxT("cursor.invalid");
const wxString CURSOR_STANDARD      = wxT("cursor.standard");
const wxString CURSOR_SIZENS        = wxT("cursor.sizens");
const wxString CURSOR_SIZEWE        = wxT("cursor.sizewe");
const wxString CURSOR_SIZENWSE      = wxT("cursor.sizenwse");
const wxString CURSOR_SIZENESW      = wxT("cursor.sizenesw");
const wxString CURSOR_MOVE          = wxT("cursor.move");
const wxString CURSOR_TEXTEDIT      = wxT("cursor.textedit");
const wxString CURSOR_CROSS         = wxT("cursor.cross");

// alignment constants
const wxString ALIGNMENT_LEFT       = wxT("alignment.left");
const wxString ALIGNMENT_CENTER     = wxT("alignment.center");
const wxString ALIGNMENT_RIGHT      = wxT("alignment.right");
const wxString ALIGNMENT_TOP        = wxT("alignment.top");
const wxString ALIGNMENT_MIDDLE     = wxT("alignment.middle");
const wxString ALIGNMENT_BOTTOM     = wxT("alignment.bottom");

// border constants
const wxString BORDER_STYLE_NONE    = wxT("border.style.none");
const wxString BORDER_STYLE_SOLID   = wxT("border.style.solid");

// stock object color constants (red, green, blue from high to low order)
// highest order digits are reserved for an alpha channel and currently 
// only set to non-zero for the null color
const unsigned long COLOR_NULL            = 0xffffffff;
const unsigned long COLOR_BLACK           = 0x000000;
const unsigned long COLOR_WHITE           = 0xffffff;
const unsigned long COLOR_RED             = 0xff0000;
const unsigned long COLOR_GREEN           = 0x00ff00;
const unsigned long COLOR_BLUE            = 0x0000ff;
const unsigned long COLOR_GREY            = 0x808080;
const unsigned long COLOR_LIGHT_GREY      = 0xc0c0c0;

// non-themed control-related colors
const unsigned long COLOR_CONTROL         = 0xc0c0c0;
const unsigned long COLOR_HIGHLIGHT_FG    = 0x000000;
const unsigned long COLOR_HIGHLIGHT_BG    = 0x0000ff;

// font style constants
const wxString FONT_STYLE_NORMAL    = wxT("font.style.normal");
const wxString FONT_STYLE_ITALIC    = wxT("font.style.italic");

// font weight constants
const wxString FONT_WEIGHT_NORMAL   = wxT("font.weight.normal");
const wxString FONT_WEIGHT_BOLD     = wxT("font.weight.bold");

// font underline constants
const wxString FONT_UNDERSCORE_NORMAL   = wxT("font.underscore.normal");
const wxString FONT_UNDERSCORE_LINE     = wxT("font.underscore.line");

// event properties for NotifyEvents; TODO: review if this is
// the best way to pass parameters in NotifyEvents
const wxString EVENT_PROP_X                 = wxT("event.prop.x");
const wxString EVENT_PROP_Y                 = wxT("event.prop.y");
const wxString EVENT_PROP_W                 = wxT("event.prop.w");
const wxString EVENT_PROP_H                 = wxT("event.prop.h");
const wxString EVENT_PROP_ROW               = wxT("event.prop.row");
const wxString EVENT_PROP_COLUMN            = wxT("event.prop.column");
const wxString EVENT_PROP_INDEX             = wxT("event.prop.index");
const wxString EVENT_PROP_SIZE              = wxT("event.prop.size");
const wxString EVENT_PROP_EDIT_ACCEPTED     = wxT("event.prop.edit.accepted");
const wxString EVENT_PROP_PAGE_START        = wxT("event.prop.page.start");
const wxString EVENT_PROP_PAGE_END          = wxT("event.prop.page.end");
const wxString EVENT_PROP_PAGES             = wxT("event.prop.pages");
const wxString EVENT_PROP_PAGE              = wxT("event.prop.page");
const wxString EVENT_PROP_CURSOR            = wxT("event.prop.cursor");
const wxString EVENT_PROP_CARET_X           = wxT("event.prop.caret.x");
const wxString EVENT_PROP_CARET_Y           = wxT("event.prop.caret.y");
const wxString EVENT_PROP_CARET_H           = wxT("event.prop.caret.h");

// canvas events
const wxString EVENT_KEY                    = wxT("event.key");
const wxString EVENT_KEY_DOWN               = wxT("event.key.down");
const wxString EVENT_KEY_UP                 = wxT("event.key.up");
const wxString EVENT_MOUSE_LEFT_DCLICK      = wxT("event.mouse.left.dclick");
const wxString EVENT_MOUSE_LEFT_DOWN        = wxT("event.mouse.left.down");
const wxString EVENT_MOUSE_LEFT_UP          = wxT("event.mouse.left.up");
const wxString EVENT_MOUSE_RIGHT_DCLICK     = wxT("event.mouse_right.dclick");
const wxString EVENT_MOUSE_RIGHT_DOWN       = wxT("event.mouse.right.down");
const wxString EVENT_MOUSE_RIGHT_UP         = wxT("event.mouse.right.up");
const wxString EVENT_MOUSE_MIDDLE_DCLICK    = wxT("event.mouse.middle.dclick");
const wxString EVENT_MOUSE_MIDDLE_DOWN      = wxT("event.mouse.middle.down");
const wxString EVENT_MOUSE_MIDDLE_UP        = wxT("event.mouse.middle.up");
const wxString EVENT_MOUSE_MOTION           = wxT("event.mouse.motion");
const wxString EVENT_MOUSE_WHEEL            = wxT("event.mouse.wheel");
const wxString EVENT_MOUSE_ENTER            = wxT("event.mouse.enter");
const wxString EVENT_MOUSE_LEAVE            = wxT("event.mouse.leave");
const wxString EVENT_MOUSE_OVER             = wxT("event.mouse.over");
const wxString EVENT_FOCUS_SET              = wxT("event.focus.set");
const wxString EVENT_FOCUS_KILL             = wxT("event.focus.kill");
const wxString EVENT_DROP                   = wxT("event.drop");
const wxString EVENT_PRE_SELECT             = wxT("event.pre.select");
const wxString EVENT_SELECT                 = wxT("event.select");
const wxString EVENT_PRE_LAYOUT             = wxT("event.pre.layout");
const wxString EVENT_LAYOUT                 = wxT("event.layout");
const wxString EVENT_PRE_RENDER             = wxT("event.pre.render");
const wxString EVENT_RENDER                 = wxT("event.render");
const wxString EVENT_PRE_PAINT              = wxT("event.pre.paint");
const wxString EVENT_PAINT                  = wxT("event.paint");
const wxString EVENT_CANVAS_SCROLL          = wxT("event.canvas.scroll");
const wxString EVENT_CANVAS_ZOOM            = wxT("event.canvas.zoom");
const wxString EVENT_CANVAS_CHANGED         = wxT("event.canvas.changed");
const wxString EVENT_CANVAS_CURSOR          = wxT("event.canvas.cursor");
const wxString EVENT_CANVAS_CARET           = wxT("event.canvas.caret");
const wxString EVENT_PRINT_BEGIN            = wxT("event.print.begin");
const wxString EVENT_PRINT_PAGE             = wxT("event.print.page");
const wxString EVENT_PRINT_END              = wxT("event.print.end");

// notify events raised in general by components
const wxString EVENT_INVALIDATE                 = wxT("event.invalidate");
const wxString EVENT_EDIT_BEGIN                 = wxT("event.edit.begin");
const wxString EVENT_EDIT_END                   = wxT("event.edit.end");

// TODO: evaluate necessity of EVENT_EDIT_BEGIN, EVENT_EDIT_END events;
// perhaps individual components should care for this themselves,
// similar to the way tables have a cell editing-related event

// notify events raised by table
const wxString EVENT_TABLE_PRE_CURSOR_MOVE              = wxT("event.table.pre.cursor.move");
const wxString EVENT_TABLE_CURSOR_MOVE                  = wxT("event.table.cursor.move");
const wxString EVENT_TABLE_PRE_SELECTION_CLEAR          = wxT("event.table.pre.selection.clear");
const wxString EVENT_TABLE_SELECTION_CLEAR              = wxT("event.table.selection.clear");
const wxString EVENT_TABLE_PRE_SELECT_CELL              = wxT("event.table.pre.select.cell");
const wxString EVENT_TABLE_SELECT_CELL                  = wxT("event.table.select.cell");
const wxString EVENT_TABLE_SELECTING_CELL               = wxT("event.table.selecting.cell");
const wxString EVENT_TABLE_PRE_RESIZE_COLUMN            = wxT("event.table.pre.resize.column");
const wxString EVENT_TABLE_RESIZE_COLUMN                = wxT("event.table.resize.column");
const wxString EVENT_TABLE_RESIZING_COLUMN              = wxT("event.table.resizing.column");
const wxString EVENT_TABLE_PRE_RESIZE_ROW               = wxT("event.table.pre.resize.row");
const wxString EVENT_TABLE_RESIZE_ROW                   = wxT("event.table.resize.row");
const wxString EVENT_TABLE_RESIZING_ROW                 = wxT("event.table.resizing.row");
const wxString EVENT_TABLE_PRE_RESIZE_ROW_TO_CONTENT    = wxT("event.table.pre.resize.row.to.content");
const wxString EVENT_TABLE_RESIZE_ROW_TO_CONTENT        = wxT("event.table.resize.row.to.content");
const wxString EVENT_TABLE_PRE_RESIZE_COLUMN_TO_CONTENT = wxT("event.table.pre.resize.column.to.content");
const wxString EVENT_TABLE_RESIZE_COLUMN_TO_CONTENT     = wxT("event.table.resize.column.to.content");
const wxString EVENT_TABLE_PRE_POPULATE_CELL            = wxT("event.table.pre.populate.cell");
const wxString EVENT_TABLE_POPULATE_CELL                = wxT("event.table.populate.cell");
const wxString EVENT_TABLE_PRE_MOVE_CELL                = wxT("event.table.pre.move.cell");
const wxString EVENT_TABLE_MOVE_CELL                    = wxT("event.table.move.cell");
const wxString EVENT_TABLE_PRE_CELL_EDIT                = wxT("event.table.pre.cell.edit");
const wxString EVENT_TABLE_CELL_EDIT                    = wxT("event.table.cell.edit");
const wxString EVENT_TABLE_PRE_CELL_CLEAR               = wxT("event.table.pre.cell.clear");
const wxString EVENT_TABLE_CELL_CLEAR                   = wxT("event.table.cell.clear");
const wxString EVENT_TABLE_CELL_LEFT_CLICK              = wxT("event.table.cell.left.click");
const wxString EVENT_TABLE_CELL_RIGHT_CLICK             = wxT("event.table.cell.right.click");

// component types
const wxString COMP_TYPE_COMPONENT          = wxT("type.component");
const wxString COMP_TYPE_PAGE               = wxT("type.page");
const wxString COMP_TYPE_BOX                = wxT("type.box");
const wxString COMP_TYPE_LINE               = wxT("type.line");
const wxString COMP_TYPE_ELLIPSE            = wxT("type.ellipse");
const wxString COMP_TYPE_TEXTBOX            = wxT("type.textbox");
const wxString COMP_TYPE_IMAGE              = wxT("type.image");
const wxString COMP_TYPE_GRAPH              = wxT("type.graph");
const wxString COMP_TYPE_TABLE              = wxT("type.table");
const wxString COMP_TYPE_CONTROL            = wxT("type.control");

// component properties
const wxString PROP_COMP_TYPE               = wxT("type");
const wxString PROP_VISIBLE                 = wxT("visible");
const wxString PROP_ORIGIN_X                = wxT("origin.x");
const wxString PROP_ORIGIN_Y                = wxT("origin.y");
const wxString PROP_SIZE_W                  = wxT("origin.width");
const wxString PROP_SIZE_H                  = wxT("origin.height");
const wxString PROP_COLOR_FG                = wxT("color.foreground");
const wxString PROP_COLOR_BG                = wxT("color.background");
const wxString PROP_LINE_WIDTH              = wxT("line.width");
const wxString PROP_LOCKED                  = wxT("prop.locked");

const wxString PROP_MARGIN_LEFT             = wxT("margin.left");
const wxString PROP_MARGIN_RIGHT            = wxT("margin.right");
const wxString PROP_MARGIN_TOP              = wxT("margin.top");
const wxString PROP_MARGIN_BOTTOM           = wxT("margin.bottom");

const wxString PROP_FONT_FACENAME           = wxT("font.facename");
const wxString PROP_FONT_SIZE               = wxT("font.size");
const wxString PROP_FONT_STYLE              = wxT("font.style");
const wxString PROP_FONT_WEIGHT             = wxT("font.weight");
const wxString PROP_FONT_UNDERSCORE         = wxT("font.underscore");

const wxString PROP_BORDER_STYLE            = wxT("border.style");
const wxString PROP_BORDER_TOP_STYLE        = wxT("border.top.style");
const wxString PROP_BORDER_BOTTOM_STYLE     = wxT("border.bottom.style");
const wxString PROP_BORDER_LEFT_STYLE       = wxT("border.left.style");
const wxString PROP_BORDER_RIGHT_STYLE      = wxT("border.right.style");

// canvas color preferences
const wxString PROP_COLOR_HIGHLIGHT        = wxT("color.highlight");
const wxString PROP_COLOR_HIGHLIGHTTEXT    = wxT("color.highlighttext");
const wxString PROP_COLOR_3DFACE           = wxT("color.3dface");

// TODO: should generalize formatting properties, since other
// types of content can use formatting parameters; should we
// use PROP_FORMAT_*?

const wxString PROP_TEXT_COLOR              = wxT("text.color");
const wxString PROP_TEXT_HALIGN             = wxT("text.halign");
const wxString PROP_TEXT_VALIGN             = wxT("text.valign");
const wxString PROP_TEXT_SPACING            = wxT("text.spacing");  // multiples of line in 100
const wxString PROP_TEXT_WRAP               = wxT("text.wrap");

const wxString PROP_CONTENT_MIMETYPE        = wxT("content.mimetype");
const wxString PROP_CONTENT_ENCODING        = wxT("content.encoding");
const wxString PROP_CONTENT_VALUE           = wxT("content.value");


// cell properties; used to indicate the current renderer and editor
// as well as store the current row and column for the table renderer 
// and editor
const wxString PROP_CELL_ROW                = wxT("cell.row");
const wxString PROP_CELL_COLUMN             = wxT("cell.column");


// DEPRECATED: only exists for <= 4.3 XML report format; PROP_CONTENT_VALUE
// is now used to store values
const wxString PROP_TEXT_VALUE              = wxT("text.value");


// property types
enum PropType
{
    proptypeInvalid = 0,
    proptypeString = 1,
    proptypeColor = 2,
    proptypeInteger = 3,
    proptypeBoolean = 4,
    proptypeComponent = 5
};

// canvas tool enumerations
enum CanvasTools
{
    toolInvalid = 0,
    toolSelect = 1,
    toolNavigate = 2,
    toolCreate = 3
};


// forward interface declarations
xcm_interface ITableModel;

xcm_interface IEdit;

xcm_interface IEvent;
xcm_interface IMouseEvent;
xcm_interface IKeyEvent;
xcm_interface INotifyEvent;
xcm_interface IDragDropEvent;
xcm_interface IEventTarget;
xcm_interface IEventHandler;
xcm_interface IEventTimer;

xcm_interface IComponent;
xcm_interface ICompImage;
xcm_interface ICompPage;
xcm_interface ICompTable;
xcm_interface ICompTextBox;
xcm_interface ICanvas;
xcm_interface ICanvasControl;
xcm_interface ICanvasHistory;


// smart pointers
XCM_DECLARE_SMARTPTR(IEdit)

XCM_DECLARE_SMARTPTR(IEvent)
XCM_DECLARE_SMARTPTR(IMouseEvent);
XCM_DECLARE_SMARTPTR(IKeyEvent);
XCM_DECLARE_SMARTPTR(INotifyEvent);
XCM_DECLARE_SMARTPTR(IDragDropEvent);
XCM_DECLARE_SMARTPTR(IEventTarget)
XCM_DECLARE_SMARTPTR(IEventHandler)
XCM_DECLARE_SMARTPTR(IEventTimer)

XCM_DECLARE_SMARTPTR(IComponent)
XCM_DECLARE_SMARTPTR(ICompImage)
XCM_DECLARE_SMARTPTR(ICompPage)
XCM_DECLARE_SMARTPTR(ICompTable)
XCM_DECLARE_SMARTPTR(ICompTextBox)
XCM_DECLARE_SMARTPTR(ICanvas)
XCM_DECLARE_SMARTPTR(ICanvasControl)
XCM_DECLARE_SMARTPTR(ICanvasHistory)

XCM_DECLARE_SMARTPTR(ITableModel)


xcm_interface IEdit : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.IEdit")

public:

    // TODO: may want to make beginEdit/endEdit into a setfocus/killfocus
    // event to handle embedded controls better; need to explore
    virtual void beginEdit() = 0;
    virtual void endEdit(bool accept) = 0;
    virtual bool isEditing() const = 0;

    virtual bool canCut() const = 0;
    virtual bool canCopy() const = 0;
    virtual bool canPaste() const = 0;

    virtual void cut() = 0;
    virtual void copy() = 0;
    virtual void paste() = 0;

    virtual void selectAll() = 0;
    virtual void selectNone() = 0;

    virtual void clear(bool text = true) = 0;
};

// event interface definitions
xcm_interface IEvent : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.IEvent")

public:

    virtual IEventPtr clone() = 0;

    virtual void setName(const wxString& name) = 0;
    virtual const wxString& getName() = 0;
    
    virtual void setTarget(IEventTargetPtr target) = 0;
    virtual IEventTargetPtr getTarget() = 0;

    virtual void setTimeStamp(long timestamp) = 0;
    virtual long getTimeStamp() = 0;

    virtual void setPhase(int phase) = 0;
    virtual int getPhase() = 0;
    
    virtual void async(bool async = false) = 0;
    virtual bool isAsync() = 0;

    virtual void handle(bool handle = true) = 0;
    virtual bool isHandled() = 0;
    
    virtual void stopPropogation() = 0;
    virtual bool isCancelled() = 0;
};

xcm_interface IMouseEvent : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.IMouseEvent")

public:

    virtual void setX(int x) = 0;
    virtual int getX() = 0;
    
    virtual void setY(int y) = 0;
    virtual int getY() = 0;

    virtual void setClientX(int x) = 0;
    virtual int getClientX() = 0;
    
    virtual void setClientY(int y) = 0;
    virtual int getClientY() = 0;

    virtual void setRotation(int r) = 0;
    virtual int getRotation() = 0;
    
    virtual void setAltDown(bool key_down) = 0;
    virtual bool isAltDown() = 0;
    
    virtual void setCtrlDown(bool key_down) = 0;
    virtual bool isCtrlDown() = 0;
    
    virtual void setShiftDown(bool key_down) = 0;
    virtual bool isShiftDown() = 0;
};

xcm_interface IKeyEvent : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.IKeyEvent")

public:

    virtual void setX(int x) = 0;
    virtual int getX() = 0;
    
    virtual void setY(int y) = 0;
    virtual int getY() = 0;

    virtual void setClientX(int x) = 0;
    virtual int getClientX() = 0;
    
    virtual void setClientY(int y) = 0;
    virtual int getClientY() = 0;
    
    virtual void setKeyCode(int keycode) = 0;
    virtual int getKeyCode() = 0;
    
    virtual void setAltDown(bool key_down) = 0;
    virtual bool isAltDown() = 0;
    
    virtual void setCtrlDown(bool key_down) = 0;
    virtual bool isCtrlDown() = 0;
    
    virtual void setShiftDown(bool key_down) = 0;
    virtual bool isShiftDown() = 0;
};

xcm_interface INotifyEvent : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.INotifyEvent")

public:

    virtual void veto(bool veto = true) = 0;
    virtual bool isAllowed() = 0;

    virtual void addProperty(const wxString& prop_name, const PropertyValue& value) = 0;
    virtual bool getProperty(const wxString& prop_name, PropertyValue& value) = 0;
};

xcm_interface IDragDropEvent : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.IDragDropEvent")

public:

    virtual void setX(int x) = 0;
    virtual int getX() = 0;
    
    virtual void setY(int y) = 0;
    virtual int getY() = 0;

    virtual void setData(IComponentPtr component) = 0;
    virtual IComponentPtr getData() = 0;
};

xcm_interface IEventTarget : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.IEventTarget")

public:

    virtual bool addEventHandler(const wxString& name,
                                 IEventHandlerPtr handler,
                                 bool capture_phase = false) = 0;
    virtual bool removeEventHandler(const wxString& name,
                                    IEventHandlerPtr handler,
                                    bool capture_phase = false) = 0;
    virtual void removeAllEventHandlers() = 0;
    virtual void dispatchEvent(IEventPtr evt) = 0;
};

xcm_interface IEventHandler : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.IEventHandler")

public:

    virtual void handleEvent(IEventPtr evt) = 0;
};

xcm_interface IEventTimer : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.IEventTimer")
    
public:

    virtual void setEvent(IEventPtr evt) = 0;
    virtual IEventPtr getEvent() = 0;

    virtual void setInterval(int interval) = 0;
    virtual int getInterval() = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
};

xcm_interface IComponent : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.IComponent")

public:

    virtual IComponentPtr clone() = 0;
    virtual void copy(IComponentPtr component) = 0;

    virtual wxString getId() = 0;
    virtual wxString getType() = 0;

    virtual bool add(IComponentPtr component) = 0;
    virtual bool remove(IComponentPtr component) = 0;
    virtual void removeAll() = 0;
    virtual void getComponents(std::vector<IComponentPtr>& components) const = 0;

    virtual void addProperty(const wxString& prop_name, const PropertyValue& value) = 0;
    virtual void addProperties(const Properties& properties) = 0;
    virtual void removeProperty(const wxString& prop_name) = 0;

    virtual bool setProperty(const wxString& prop_name, const PropertyValue& value) = 0;
    virtual bool setProperties(const Properties& properties) = 0;
    virtual bool getProperty(const wxString& prop_name, PropertyValue& value) const = 0;
    virtual bool getProperties(Properties& properties) const = 0;
    virtual Properties& getPropertiesRef() = 0;

    virtual void setOrigin(int x, int y) = 0;
    virtual void getOrigin(int* x, int* y) const = 0;

    virtual void setSize(int w, int h) = 0;
    virtual void getSize(int* w, int* h) const = 0;

    virtual void setWidth(int width) = 0;
    virtual int getWidth() const = 0;

    virtual void setHeight(int height) = 0;
    virtual int getHeight() const = 0;

    virtual void visible(bool visible) = 0;
    virtual bool isVisible() const = 0;

    virtual bool contains(int x, int y) = 0;
    virtual bool intersects(const wxRect& rect) = 0;
    virtual void extends(wxRect& rect) = 0;
    virtual void render(const wxRect& rect = wxRect()) = 0;
};

xcm_interface ICompImage : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.ICompImage")
    
public:

    virtual void setImage(const wxImage& image) = 0;
    virtual const wxImage& getImage() = 0;
};

xcm_interface ICompPage : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.ICompPage")

public:

    virtual void setMargins(int left_margin,
                            int right_margin,
                            int top_margin,
                            int bottom_margin) = 0;
                            
    virtual void getMargins(int* left_margin,
                            int* right_margin,
                            int* top_margin,
                            int* bottom_margin) const = 0;
};

xcm_interface ICompTable : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.ICompTable")
    
public:

    virtual void setModel(ITableModelPtr model) = 0;
    virtual ITableModelPtr getModel() = 0;

    virtual bool setRowCount(int count) = 0;
    virtual int getRowCount() const = 0;

    virtual bool setColumnCount(int count) = 0;
    virtual int getColumnCount() const = 0;

    virtual bool insertRow(int idx, int count = 1) = 0;
    virtual bool removeRow(int idx, int count = 1) = 0;

    virtual bool insertColumn(int idx, int count = 1) = 0;
    virtual bool removeColumn(int idx, int count = 1) = 0;

    virtual bool setRowSize(int row, int size) = 0;
    virtual int getRowSize(int row) = 0;

    virtual bool setColumnSize(int col, int size) = 0;
    virtual int getColumnSize(int col) = 0;

    virtual int getColumnPosByIdx(int col) = 0;
    virtual int getColumnIdxByPos(int x) = 0;

    virtual int getRowPosByIdx(int row) = 0;
    virtual int getRowIdxByPos(int y) = 0;

    virtual void getCellIdxByPos(int x, int y, int* row, int* col, bool merged = true) = 0;
    virtual void getCellPosByIdx(int row, int col, int* x, int* y, bool merged = true) = 0;
    virtual void getCellSizeByIdx(int row, int col, int* w, int* h, bool merged = true) = 0;
    virtual void getCellCenterByIdx(int row, int col, int* x, int* y, bool merged = true) = 0;

    virtual void addCellProperty(const wxString& prop_name,
                                 const PropertyValue& value) = 0;

    virtual void addCellProperties(const std::vector<CellProperties>& properties,
                                   bool replace = false) = 0;

    virtual bool removeCellProperty(const wxString& prop_name) = 0;

    virtual bool setCellProperty(const CellRange& range,
                                 const wxString& prop_name,
                                 const PropertyValue& value) = 0;

    virtual bool setCellProperties(const CellRange& range,
                                   const Properties& properties) = 0;

    virtual bool getCellProperty(const CellRange& range,
                                 const wxString& prop_name,
                                 PropertyValue& value) const = 0;

    virtual bool getCellProperty(const CellRange& range,
                                 const wxString& prop_name,
                                 std::vector<CellProperties>& properties) const = 0;

    virtual bool getCellProperties(const CellRange& range,
                                   Properties& properties) const = 0;

    virtual void getCellProperties(const CellRange& range,
                                   std::vector<CellProperties>& properties) const = 0;

    virtual void evalCell(int row, int col, Properties& properties) const = 0;

    virtual void mergeCells(const CellRange& range) = 0;
    virtual void unmergeCells(const CellRange& range) = 0;
    virtual void unmergeAllCells() = 0;

    virtual void getMergedCells(std::vector<CellRange>& merges) const = 0;
    virtual bool isMerged(CellRange& range) const = 0;

    virtual bool tag(const wxString& tag) = 0;
    virtual bool restore(const wxString& tag) = 0;
    virtual void compact() = 0;
    virtual void reset() = 0;

    virtual void selectCells(const CellRange& range) = 0;
    virtual void removeCellSelections(const CellRange& range) = 0;
    virtual void removeAllCellSelections() = 0;

    virtual void getCellSelections(std::vector<CellRange>& selections, bool cursor = true) = 0;
    virtual bool isCellSelected(int row, int col, bool cursor = true) = 0;
    virtual bool isColumnSelected(int col) const = 0;
    virtual bool isRowSelected(int row) const = 0;
    virtual bool isTableSelected() const = 0;
    virtual bool hasColumnSelections() const = 0;
    virtual bool hasRowSelections() const = 0;
    virtual bool hasSelections() const = 0;

    virtual void moveCursor(int row_diff, int col_diff, bool clear_selection = false) = 0;
    virtual void setCursorPos(int row, int col) = 0;
    virtual void getCursorPos(int* row, int* col) const = 0;
    virtual bool isCursorCell(int row, int col) const = 0;

    virtual void clearCells(const CellRange& range, 
                            const Properties& properties) = 0;
                            
    virtual void copyCells(const CellRange& range1,
                           const CellRange& range2,
                           ICompTablePtr target_table) = 0;
                           
    virtual void moveCells(const CellRange& range1,
                           const CellRange& range2,
                           ICompTablePtr target_table) = 0;

    virtual bool resizeRowsToContent(const CellRange& range) = 0;
    virtual bool resizeColumnsToContent(const CellRange& range) = 0;
    virtual void getRowContentSize(const CellRange& range, std::map<int,int>& sizes) = 0;
    virtual void getColumnContentSize(const CellRange& range, std::map<int,int>& sizes) = 0;

    virtual void addRenderer(const wxString& name, IComponentPtr comp) = 0;
    virtual void getRenderers(std::vector<IComponentPtr>& renderers) = 0;

    virtual void addEditor(const wxString& name, IComponentPtr comp) = 0;
    virtual void getEditors(std::vector<IComponentPtr>& editors) = 0;

    virtual void setPreference(int pref, bool active) = 0;
    virtual bool isPreference(int pref) = 0;
};

xcm_interface ICompTextBox : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.ICompTextBox")

public:

    virtual void setText(const wxString& text) = 0;
    virtual const wxString& getText() = 0;

    virtual void setInsertionPointByPos(int x, int y) = 0;
    virtual void getInsertionPointByPos(int* x, int* y) = 0;
    
    virtual void setSelectionByPos(int x1, int y1, int x2, int y2) = 0;
    virtual void getSelectionByPos(int* x1, int* y1, int* x2, int* y2) = 0;
};

xcm_interface ICanvas : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.ICanvas")

public:

    // canvas clone/copy functions
    virtual ICanvasPtr clone() = 0;
    virtual void copy(ICanvasPtr canvas) = 0;

    // scale and conversion functions
    virtual bool setScale(int x, int y) = 0;
    virtual void getScale(int* x, int* y) = 0;    
    
    virtual int mtod_x(int x) const = 0;
    virtual int mtod_y(int y) const = 0;
    virtual int dtom_x(int x) const = 0;
    virtual int dtom_y(int y) const = 0;

    virtual void clientToModel(int x_client, int y_client,
                               int* x_model, int* y_model) const = 0;
                       
    virtual void modelToClient(int x_model, int y_model,
                              int* x_client, int* y_client) const = 0;

    // canvas model size
    virtual void setSize(int x, int y) = 0;
    virtual void getSize(int* x, int* y) const = 0;
    
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    // canvas view functions    
    virtual void setViewOrigin(int x, int y) = 0;
    virtual void getViewOrigin(int* x, int* y) const = 0;

    virtual void setViewSize(int x, int y) = 0;
    virtual void getViewSize(int* x, int* y) const = 0;
    
    virtual int getViewWidth() const = 0;
    virtual int getViewHeight() const = 0;

    virtual bool scrollTo(int x, int y) = 0;
    virtual bool scrollBy(int x_diff, int y_diff) = 0;
    
    virtual void setZoom(int percent) = 0;
    virtual int getZoom() const = 0;

    // canvas model component
    virtual bool setModelComponent(IComponentPtr component) = 0;
    virtual IComponentPtr getModelComponent() = 0;

    // canvas component add and remove functions
    virtual bool add(IComponentPtr component) = 0;
    virtual bool remove(IComponentPtr component) = 0;
    virtual void removeAll() = 0;
    virtual void getComponents(std::vector<IComponentPtr>& components) const = 0;

    // canvas properties
    virtual void addProperty(const wxString& prop_name, const PropertyValue& value) = 0;
    virtual void addProperties(const Properties& properties) = 0;
    virtual void removeProperty(const wxString& prop_name) = 0;

    virtual bool setProperty(const wxString& prop_name, const PropertyValue& value) = 0;
    virtual bool setProperties(const Properties& properties) = 0;
    virtual bool getProperty(const wxString& prop_name, PropertyValue& value) const = 0;
    virtual bool getProperties(Properties& properties) const = 0;
    virtual Properties& getPropertiesRef() = 0;

    // canvas component location functions
    virtual IComponentPtr getComponentById(const wxString& id) = 0;
    virtual IComponentPtr getComponentByPos(int x, int y) = 0;
    virtual void getComponentsByPos(std::vector<IComponentPtr>& components, int x, int y) = 0;
    virtual void getComponentsByRect(std::vector<IComponentPtr>& components, const wxRect& rect = wxRect()) = 0;
    virtual void getComponentsByType(std::vector<IComponentPtr>& components, const wxString& type) = 0;
    virtual void getComponentPos(IComponentPtr component, int* x, int* y) = 0;

    // selection functions
    virtual bool select(IComponentPtr component) = 0;
    virtual bool removeSelection(IComponentPtr component) = 0;
    virtual void removeAllSelections() = 0;

    virtual void getSelections(std::vector<IComponentPtr>& components) const = 0;
    virtual bool isSelected(IComponentPtr component) const = 0;
    virtual bool hasSelections() const = 0;

    // focus functions
    virtual bool setFocus(IComponentPtr component) = 0;
    virtual IComponentPtr getFocus() const = 0;
    virtual bool hasFocus(IComponentPtr component) const = 0;

    // layout/render/printing functions
    virtual void layout() = 0;
    virtual void render(const wxRect& rect = wxRect()) = 0;
    virtual bool isPrinting() const = 0;

    // drawing functions    
    virtual bool setDrawOrigin(int x, int y) = 0;
    virtual void getDrawOrigin(int* x, int* y) const = 0;
    virtual void moveDrawOrigin(int x, int y) = 0;

    virtual bool setPen(const Pen& pen) = 0;
    virtual bool setPen(const Properties& properties) = 0;
    
    virtual bool setBrush(const Brush& brush) = 0;
    virtual bool setBrush(const Properties& properties) = 0;

    virtual bool setFont(const Font& font) = 0;
    virtual bool setTextBackground(const Color& color) = 0;
    virtual bool setTextForeground(const Color& color) = 0;

    virtual void drawImage(const wxImage& image,
                           int x, int y, int w, int h) = 0;

    virtual void drawCheckBox(int x, int y, int w, int h, bool checked = true) = 0;
    virtual void drawCircle(int x, int y, int r) = 0;
    virtual void drawEllipse(int x, int y, int w, int h) = 0;
    virtual void drawLine(int x1, int  y1, int x2, int y2) = 0;
    virtual void drawPoint(int x, int y) = 0;
    virtual void drawPolygon(int n, wxPoint points[], int x = 0, int y = 0) = 0;
    virtual void drawGradientFill(const wxRect& rect,
                                  const Color& start_color,
                                  const Color& end_color,
                                  const wxDirection& direction) = 0;
    virtual void drawRectangle(int x, int y, int w, int h) = 0;
    virtual void drawText(const wxString& text, int x, int y) = 0;

    virtual bool getTextExtent(const wxString& text,
                               int* x,
                               int* y,
                               int* descent = NULL,
                               int* external = NULL) const = 0;

    virtual bool getPartialTextExtents(const wxString& text,
                                       wxArrayInt& widths) const = 0;

    virtual int getCharHeight() const = 0;
    virtual int getCharWidth() const = 0;

    virtual void addClippingRectangle(int x, int y, int w, int h) = 0;
    virtual void removeClippingRectangle() = 0;
    virtual void removeAllClippingRectangles() = 0;
};

xcm_interface ICanvasControl : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.ICanvasControl")
    
public:

    virtual wxWindow* getWindow() = 0;
};

xcm_interface ICanvasHistory : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.ICanvasHistory")

public:

    virtual bool canUndo() const = 0;
    virtual bool canRedo() const = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual void track() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
    virtual void clear() = 0;
};

xcm_interface ITableModel : public xcm::IObject
{
    XCM_INTERFACE_NAME("kcanvas.ITableModel")

public:

    virtual ITableModelPtr clone() = 0;
    virtual void copy(ITableModelPtr model) = 0;

    virtual bool setRowCount(int count) = 0;
    virtual int getRowCount() const = 0;

    virtual bool setColumnCount(int count) = 0;
    virtual int getColumnCount() const = 0;

    virtual bool insertRow(int idx, int count = 1) = 0;
    virtual bool removeRow(int idx, int count = 1) = 0;

    virtual bool insertColumn(int idx, int count = 1) = 0;
    virtual bool removeColumn(int idx, int count = 1) = 0;

    virtual bool setRowSize(int row, int size) = 0;
    virtual int getRowSize(int row) = 0;
    
    virtual bool setColumnSize(int col, int size) = 0;
    virtual int getColumnSize(int col) = 0;

    virtual int getColumnPosByIdx(int col) = 0;
    virtual int getColumnIdxByPos(int x) = 0;

    virtual int getRowPosByIdx(int row) = 0;
    virtual int getRowIdxByPos(int y) = 0;

    virtual void getCellIdxByPos(int x, int y, int* row, int* col, bool merged = true) = 0;
    virtual void getCellPosByIdx(int row, int col, int* x, int* y, bool merged = true) = 0;
    virtual void getCellSizeByIdx(int row, int col, int* w, int* h, bool merged = true) = 0;
    virtual void getCellCenterByIdx(int row, int col, int* x, int* y, bool merged = true) = 0;

    virtual void addCellProperty(const wxString& prop_name,
                                 const PropertyValue& value) = 0;

    virtual void addCellProperties(const std::vector<CellProperties>& properties,
                                   bool replace) = 0;

    virtual bool removeCellProperty(const wxString& prop_name) = 0;

    virtual bool setCellProperty(const CellRange& range,
                                 const wxString& prop_name,
                                 const PropertyValue& value) = 0;

    virtual bool setCellProperties(const CellRange& range,
                                   const Properties& properties) = 0;

    virtual bool getCellProperty(const CellRange& range,
                                 const wxString& prop_name,
                                 PropertyValue& value) const = 0;

    virtual bool getCellProperty(const CellRange& range,
                                 const wxString& prop_name,
                                 std::vector<CellProperties>& properties) const = 0;

    virtual bool getCellProperties(const CellRange& range,
                                   Properties& properties) const = 0;

    virtual void getCellProperties(const CellRange& range,
                                   std::vector<CellProperties>& properties) const = 0;

    virtual void setDefaultCellProperties(Properties& properties) = 0;
    virtual void getDefaultCellProperties(Properties& properties) const = 0;

    virtual void mergeCells(const CellRange& range) = 0;
    virtual void unmergeCells(const CellRange& range) = 0;
    virtual void unmergeAllCells() = 0;

    virtual void getMergedCells(std::vector<CellRange>& merges) const = 0;
    virtual bool isMerged(CellRange& range) const = 0;

    virtual bool tag(const wxString& tag) = 0;
    virtual bool restore(const wxString& tag) = 0;
    virtual void compact() = 0;
    virtual void reset() = 0;

    virtual void eval(int row, int col, Properties& properties) const = 0;
};


};  // namespace kcanvas


#endif

