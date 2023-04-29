/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2008-03-11
 *
 */


#include "kcanvas_int.h"
#include "canvas.h"
#include "canvasdc.h"
#include "canvascontrol.h"
#include "dnd.h"

#include "componentimage.h"
#include "componenttextbox.h"


namespace kcanvas
{


// XPM: blue, editing selection rectangle
static const char* xpm_edit_blue[] = {
"8 8 2 1",
"  c #FFFFFF",
"! c #0000C0",
"!   !   ",
"   !   !",
"  !   ! ",
" !   !  ",
"!   !   ",
"   !   !",
"  !   ! ",
" !   !  "};

// XPM: red, editing selection rectangle
static const char* xpm_edit_red[] = {
"8 8 2 1",
"  c #FFFFFF",
"! c #FF0000",
"!   !   ",
"   !   !",
"  !   ! ",
" !   !  ",
"!   !   ",
"   !   !",
"  !   ! ",
" !   !  "};

// XPM: blue, non-editing selection rectangle
static const char* xpm_noedit_blue[] = {
"8 8 2 1",
"  c #FFFFFF",
"! c #0000C0",
"! ! ! ! ",
" ! ! ! !",
"! ! ! ! ",
" ! ! ! !",
"! ! ! ! ",
" ! ! ! !",
"! ! ! ! ",
" ! ! ! !"};

// XPM: red, non-editing selection rectangle
static const char* xpm_noedit_red[] = {
"8 8 2 1",
"  c #FFFFFF",
"! c #FF0000",
"! ! ! ! ",
" ! ! ! !",
"! ! ! ! ",
" ! ! ! !",
"! ! ! ! ",
" ! ! ! !",
"! ! ! ! ",
" ! ! ! !"};


const int ScrollTimerId = 24123;
const int CursorTimerId = 24124;


// canvas implementation

BEGIN_EVENT_TABLE(CanvasControl, wxControl)
    EVT_SIZE(CanvasControl::onSize)
    EVT_PAINT(CanvasControl::onPaint)
    EVT_ERASE_BACKGROUND(CanvasControl::onEraseBackground)
    EVT_MOUSE_CAPTURE_LOST(CanvasControl::onMouseCaptureLost)
    EVT_MOUSE_EVENTS(CanvasControl::onMouse)
    EVT_KEY_DOWN(CanvasControl::onChar)
    EVT_KEY_UP(CanvasControl::onChar)
    EVT_CHAR(CanvasControl::onChar)
    EVT_SET_FOCUS(CanvasControl::onSetFocus)
    EVT_KILL_FOCUS(CanvasControl::onKillFocus)
    EVT_SCROLLWIN(CanvasControl::onScroll)
    EVT_TIMER(ScrollTimerId, CanvasControl::onScrollTimer)
    EVT_TIMER(CursorTimerId, CanvasControl::onCursorTimer)
END_EVENT_TABLE()


CanvasControl::CanvasControl(wxWindow* parent,
                            wxWindowID id,
                            const wxPoint& pos,
                            const wxSize& size,
                            long style) : wxControl(parent, id, pos, size, style)
{
    // set the minimum size
    SetMinSize(size);

    // set the canvas drop target
    CanvasDropTarget* drop_target = new CanvasDropTarget;
    drop_target->sigItemDragged.connect(this, &CanvasControl::onItemDragged);
    drop_target->sigItemDropped.connect(this, &CanvasControl::onItemDropped);
    SetDropTarget(drop_target);

    // connect the default canvas control event handlers
    EventHandler* handler = new kcanvas::EventHandler;
    handler->sigEvent().connect(this, &CanvasControl::onCanvasEvent);
    addEventHandler(kcanvas::EVENT_CANVAS_SCROLL, handler);
    addEventHandler(kcanvas::EVENT_LAYOUT, handler);
    addEventHandler(kcanvas::EVENT_PRE_RENDER, handler);
    addEventHandler(kcanvas::EVENT_CANVAS_CARET, handler);
    addEventHandler(kcanvas::EVENT_FOCUS_SET, handler);
    addEventHandler(kcanvas::EVENT_FOCUS_KILL, handler);
    addEventHandler(kcanvas::EVENT_DROP, handler);
    addEventHandler(kcanvas::EVENT_EDIT_BEGIN, handler);
    addEventHandler(kcanvas::EVENT_EDIT_END, handler);

    // initialize the memory dc for off-screen drawable
    m_memwidth = 0;
    m_memheight = 0;
    allocBitmap(100, 100);

    // initialize timer
    m_scroll_timer.SetOwner(this, ScrollTimerId);
    m_cursor_timer.SetOwner(this, CursorTimerId);
    m_scroll_timer_voffset = 0;
    m_scroll_timer_hoffset = 0;

    // text edit cursor variables
    m_caret_x = 0;
    m_caret_y = 0;
    m_caret_h = 0;
    m_caret_active = false;
}

CanvasControl::~CanvasControl()
{
}

ICanvasPtr CanvasControl::create(wxWindow* parent,
                                 wxWindowID id,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style)
{
    return static_cast<ICanvas*>(new CanvasControl(parent, id, pos, size, style));
}

wxWindow* CanvasControl::getWindow()
{
    return this;
}

ICanvasPtr CanvasControl::clone()
{
    CanvasControl* c = new CanvasControl(NULL);
    c->copy(this);

    return static_cast<ICanvas*>(c);
}

void CanvasControl::copy(ICanvasPtr canvas)
{
    // copy the base canvas properties
    Canvas::copy(canvas);

    ICanvasControlPtr canvas_control = canvas;
    if (canvas_control.isNull())
        return;

    // TODO: copy additional control info
}

void CanvasControl::getViewSize(int* x, int* y) const
{
    // note: the view size is the client size in model coordinates;
    // with different zooms, the view takes up a larger or smaller 
    // percentage of the model; as a result the view size may increase 
    // or decrease not only from changing the actual client area, but
    // also from changing the zoom which can change at any time; to 
    // ensure the view size is always correct, no matter where we are,
    // calculate the view size directly rather than setting size
    // variables in the size event or the layout function
    
    GetClientSize(x, y);
    *x = dtom_x(*x);
    *y = dtom_y(*y);
}

int CanvasControl::getViewWidth() const
{
    int width, height;
    getViewSize(&width, &height);
    return width;
}

int CanvasControl::getViewHeight() const
{
    int width, height;
    getViewSize(&width, &height);
    return height;
}

void CanvasControl::render(const wxRect& rect)
{
    // allocate the memory bitmap
    int width, height;
    GetClientSize(&width, &height);
    allocBitmap(width, height);

    // render the canvas
    Canvas::render(rect);
    
    // repaint
    repaint();
}

void CanvasControl::beginEdit()
{
    // get the focus component
    IEditPtr edit;
    edit = getFocus();

    // if we don't have an edit component, we're done
    if (edit.isNull())
        return;

    // if we're already editing, we're done
    if (edit->isEditing())
        return;

    // start the caret        
    startCaret();        
}

void CanvasControl::endEdit(bool accept)
{
    // stop the timer; note: we have to stop the caret up
    // front because the component may stop the editing
    // itself, in which case, the isEditing() will return
    // false and we won't have a chance to turn off the
    // caret
    stopCaret();

    // get the focus component
    IEditPtr edit;
    edit = getFocus();

    // if we don't have an edit component, we're done
    if (edit.isNull())
        return;

    // if we're not editing, we're done
    if (!edit->isEditing())
    {
        // the edit has been stopped already, but we sill need to 
        // notify listeners that the canvas has changed; so if we 
        // accepted the changes, dispatch the canvas changed event
        if (accept)
            dispatchCanvasChangedEvent();

        return;
    }

    // stop the edit
    edit->endEdit(accept);

    // if we accepted the changes, dispatch the canvas 
    // changed event
    if (accept)
        dispatchCanvasChangedEvent();
}

bool CanvasControl::isEditing() const
{
    // get the focus component
    IEditPtr edit;
    edit = getFocus();

    // if we don't have an edit component, return false
    if (edit.isNull())
        return false;

    // return the editing state
    return edit->isEditing();
}

bool CanvasControl::canCut() const
{
    IEditPtr edit = getFocus();
    if (edit.isNull())
        return false;
        
    return edit->canCut();
}

bool CanvasControl::canCopy() const
{
    IEditPtr edit = getFocus();
    if (edit.isNull())
        return false;
        
    return edit->canCopy();
}

bool CanvasControl::canPaste() const
{
    IEditPtr edit = getFocus();
    if (edit.isNull())
        return false;

    return edit->canPaste();
}

void CanvasControl::cut()
{
    IEditPtr edit = getFocus();
    if (edit.isNull())
        return;
    
    edit->cut();

    // clear the canvas component cache
    clearCache();

    // dispatch canvas changed event
    dispatchCanvasChangedEvent();
}

void CanvasControl::copy()
{
    IEditPtr edit = getFocus();
    if (edit.isNull())
        return;
    
    edit->copy();
}

void CanvasControl::paste()
{
    IEditPtr edit = getFocus();
    if (edit.isNull())
        return;
    
    edit->paste();

    // clear the canvas component cache
    clearCache();

    // dispatch canvas changed event
    dispatchCanvasChangedEvent();
}

void CanvasControl::selectAll()
{
    IEditPtr edit = getFocus();
    if (edit.isNull())
        return;
    
    edit->selectAll();
}

void CanvasControl::selectNone()
{
    IEditPtr edit = getFocus();
    if (edit.isNull())
        return;
    
    edit->selectNone();
}

void CanvasControl::clear(bool text)
{
    IEditPtr edit = getFocus();
    if (edit.isNull())
        return;
    
    edit->clear(text);

    // clear the canvas component cache
    clearCache();

    // dispatch canvas changed event
    dispatchCanvasChangedEvent();
}

void CanvasControl::onCanvasEvent(IEventPtr evt)
{
    wxString name = evt->getName();
    
    // scroll events
    if (name == kcanvas::EVENT_CANVAS_SCROLL)
    {
        onCanvasScrollEvent(evt);
        return;
    }

    // layout events
    if (name == kcanvas::EVENT_LAYOUT)
    {
        onCanvasLayoutEvent(evt);
        return;
    }
    
    // render events
    if (name == kcanvas::EVENT_PRE_RENDER)
    {
        onCanvasPreRenderEvent(evt);
        return;
    }

    // focus events
    if (name == EVENT_FOCUS_SET ||
        name == EVENT_FOCUS_KILL)
    {
        onCanvasFocusEvent(evt);
        return;
    }

    // update the caret
    if (name == kcanvas::EVENT_CANVAS_CARET)
    {
        onCanvasCaretEvent(evt);
    }

    // drag/drop events
    if (name == EVENT_DROP)
    {
        onCanvasDragDropEvent(evt);
        return;
    }

    // begin edit event
    if (name == kcanvas::EVENT_EDIT_BEGIN)
    {
        onCanvasEditBeginEvent(evt);
        return;
    }
    
    // end edit event
    if (name == kcanvas::EVENT_EDIT_END)
    {
        onCanvasEditEndEvent(evt);
        return;
    }
}

void CanvasControl::onCanvasScrollEvent(IEventPtr evt)
{
    // update the scrollbars
    updateScrollbars();
}

void CanvasControl::onCanvasLayoutEvent(IEventPtr evt)
{
    // if we're done laying out the canvas, update
    // the scrollbars
    ICanvasPtr canvas = evt->getTarget();
    if (canvas.isOk())
        updateScrollbars();
}

void CanvasControl::onCanvasPreRenderEvent(IEventPtr evt)
{
    // get the update rectangle
    PropertyValue x, y, width, height;
    INotifyEventPtr notify_evt = evt;

    notify_evt->getProperty(EVENT_PROP_X, x);
    notify_evt->getProperty(EVENT_PROP_Y, y);
    notify_evt->getProperty(EVENT_PROP_W, width);
    notify_evt->getProperty(EVENT_PROP_H, height);

    // draw the background where we're updating    
    setPen(Color(COLOR_GREY));
    setBrush(Color(COLOR_GREY));
    drawRectangle(x.getInteger(),
                  y.getInteger(),
                  width.getInteger(),
                  height.getInteger());
}

void CanvasControl::onCanvasCaretEvent(IEventPtr evt)
{
    int x, y, h;
    INotifyEventPtr notify_evt = evt;

    PropertyValue prop;
    notify_evt->getProperty(EVENT_PROP_CARET_X, prop);
    x = prop.getInteger();
    
    notify_evt->getProperty(EVENT_PROP_CARET_Y, prop);
    y = prop.getInteger();
    
    notify_evt->getProperty(EVENT_PROP_CARET_H, prop);
    h = prop.getInteger();
    
    setCaret(x, y, h);
}

void CanvasControl::onCanvasFocusEvent(IEventPtr evt)
{
    // if we're not in the bubbling phase, we're done
    wxString name = evt->getName();
    if (evt->getPhase() != kcanvas::EVENT_BUBBLING_PHASE)
        return;

    // if the target receiving the focus event isn't a 
    // component, we're done
    IComponentPtr comp = evt->getTarget();
    if (comp.isNull())
        return;

    // if the event losing the focus is still editing,
    // stop the editing
    if (name == EVENT_FOCUS_KILL)
    {
        if (isEditing())
            endEdit(true);
    }
        
    if (name == EVENT_FOCUS_SET ||
        name == EVENT_FOCUS_KILL)
    {
        // when the focus changes, invalidate both the old
        // focus component and the new focus component
        invalidate(comp);
    }
}

void CanvasControl::onCanvasDragDropEvent(IEventPtr evt)
{
    wxString name = evt->getName();
    IDragDropEventPtr dragdrop_evt = evt;

    if (name == EVENT_DROP)
    {
        IComponentPtr drop_data;
        drop_data = dragdrop_evt->getData();
        
        if (drop_data.isNull())
            return;
            
        std::vector<IComponentPtr> drop_components;
        drop_data->getComponents(drop_components);
        
        std::vector<IComponentPtr>::iterator it, it_end;
        it_end = drop_components.end();
        
        for (it = drop_components.begin(); it != it_end; ++it)
        {
            IComponentPtr target = evt->getTarget();
            target->add(*it);
        }
        
        clearCache();
        dispatchCanvasChangedEvent();
    }
}

void CanvasControl::onCanvasEditBeginEvent(IEventPtr evt)
{
    beginEdit();
}

void CanvasControl::onCanvasEditEndEvent(IEventPtr evt)
{
    INotifyEventPtr notify_evt = evt;
    PropertyValue value;
    notify_evt->getProperty(EVENT_PROP_EDIT_ACCEPTED, value);
    bool accept = value.getBoolean();
    endEdit(accept);
}

void CanvasControl::onSize(wxSizeEvent& evt)
{
    layout();
    render();
}

void CanvasControl::onPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    repaint(wxRect(), &dc);
}

void CanvasControl::onEraseBackground(wxEraseEvent& evt)
{
}

void CanvasControl::onMouseCaptureLost(wxMouseCaptureLostEvent& evt)
{
    // TODO: handle event; however, seems to be firing when
    // it shouldn't be; something might be wrong
}

void CanvasControl::onMouse(wxMouseEvent& evt)
{
    // before anything else set the focus and capture 
    // the mouse
    if (evt.GetEventType() == wxEVT_LEFT_DOWN ||
        evt.GetEventType() == wxEVT_MIDDLE_DOWN ||
        evt.GetEventType() == wxEVT_RIGHT_DOWN)
    {
        SetFocus();
        CaptureMouse();
    }

    // if we have an up event, end mouse action state
    if (evt.GetEventType() == wxEVT_LEFT_UP ||
        evt.GetEventType() == wxEVT_MIDDLE_UP ||
        evt.GetEventType() == wxEVT_RIGHT_UP)
    {
        // if the mouse is captured, release the capture
        if (HasCapture())
            ReleaseMouse();
    }

    // start or stop the timer for view scrolling
    if (!HasCapture())
    {    
        // if the left mouse button isn't down (not captured) or 
        // we're in move mode (tool == navigate), stop the timer
        if (m_scroll_timer.IsRunning())
            m_scroll_timer.Stop();
    }
    else
    {
        // if tool is anything besides the movement tool, trigger 
        // timer if mouse goes outside the client area
        m_scroll_timer_voffset = 0;
        m_scroll_timer_hoffset = 0;
        
        int scroll_vert = wxMax(dtom_y(5), (getViewHeight()*SCROLL_RATE_VERT)/100);
        int scroll_horz = wxMax(dtom_x(5), (getViewWidth()*SCROLL_RATE_HORZ)/100);

        int client_width, client_height;
        GetClientSize(&client_width, &client_height);
        
        if (evt.GetY() > client_height)
            m_scroll_timer_voffset = scroll_vert;

        if (evt.GetY() < 0)
            m_scroll_timer_voffset = -1*scroll_vert;

        if (evt.GetX() > client_width)
            m_scroll_timer_hoffset = scroll_horz;

        if (evt.GetX() < 0)
            m_scroll_timer_hoffset = -1*scroll_horz;

        // if the scroll vertical or scroll horizontal values are anything
        // besides zero, start the timer; otherwise, stop the timer
        if (m_scroll_timer_voffset != 0 || m_scroll_timer_hoffset != 0)
        {
            if (!m_scroll_timer.IsRunning())
                m_scroll_timer.Start(30);
        }
        else
        {
            if (m_scroll_timer.IsRunning())
                m_scroll_timer.Stop();
        }
    }

    // create an event and set the target
    IEventPtr basic_evt = MouseEvent::create(wxT(""), this);
    IMouseEventPtr mouse_evt = basic_evt;

    // set the event location in model coordinates
    int event_x, event_y;
    clientToModel(evt.GetX(), evt.GetY(), &event_x, &event_y);
    mouse_evt->setX(event_x);
    mouse_evt->setY(event_y);
    mouse_evt->setClientX(evt.GetX());
    mouse_evt->setClientY(evt.GetY());

    // set the name and mouse button state of the event
    if (evt.GetEventType() == wxEVT_LEFT_DOWN)
    {
        basic_evt->setName(EVENT_MOUSE_LEFT_DOWN);
    }
     else if (evt.GetEventType() == wxEVT_RIGHT_DOWN)
    {    
        basic_evt->setName(EVENT_MOUSE_RIGHT_DOWN);
    }
     else if (evt.GetEventType() == wxEVT_MIDDLE_DOWN)
    {
        basic_evt->setName(EVENT_MOUSE_MIDDLE_DOWN);
    }
     else if (evt.GetEventType() == wxEVT_LEFT_UP)
    {
        basic_evt->setName(EVENT_MOUSE_LEFT_UP);
    }
     else if (evt.GetEventType() == wxEVT_RIGHT_UP)
    {    
        basic_evt->setName(EVENT_MOUSE_RIGHT_UP);
    }
     else if (evt.GetEventType() == wxEVT_MIDDLE_UP)
    {
        basic_evt->setName(EVENT_MOUSE_MIDDLE_UP);
    }
     else if (evt.GetEventType() == wxEVT_LEFT_DCLICK)
    {
        basic_evt->setName(EVENT_MOUSE_LEFT_DCLICK);
    }
     else if (evt.GetEventType() == wxEVT_RIGHT_DCLICK)
    {
        basic_evt->setName(EVENT_MOUSE_RIGHT_DCLICK);
    }
     else if (evt.GetEventType() == wxEVT_MIDDLE_DCLICK)
    {
        basic_evt->setName(EVENT_MOUSE_MIDDLE_DCLICK);
    }    
     else if (evt.GetEventType() == wxEVT_MOTION)
    {
        basic_evt->setName(EVENT_MOUSE_MOTION);
    }   
     else if (evt.GetEventType() == wxEVT_MOUSEWHEEL)
    {
        basic_evt->setName(EVENT_MOUSE_WHEEL);
        
        int rotation = evt.GetWheelRotation()/evt.GetWheelDelta();
        mouse_evt->setRotation(rotation);
    }
     else if (evt.GetEventType() == wxEVT_ENTER_WINDOW)
    {
        basic_evt->setName(EVENT_MOUSE_ENTER);
    }
     else if (evt.GetEventType() == wxEVT_LEAVE_WINDOW)
    {
        basic_evt->setName(EVENT_MOUSE_LEAVE);
    }

    // note: EVENT_MOUSE_OVER is set in preProcessEvent

    // set the key state
    mouse_evt->setAltDown(evt.AltDown());
    mouse_evt->setCtrlDown(evt.ControlDown());
    mouse_evt->setShiftDown(evt.ShiftDown());

    // if the event isn't one of the above, skip the window event 
    // to allow it to continue
    if (basic_evt->getName().IsEmpty())
    {
        evt.Skip();
        return;
    }

    // dispatch the canvas mouse event to the handlers
    dispatchEvent(mouse_evt);
    
    // if the middle mouse wheel moves fire a mouse move
    // event to simulate mouse movement
    if (evt.GetEventType() == wxEVT_MOUSEWHEEL)
    {
        // TODO: same thing in onScrollTimer(); is there
        // a better way to do this?
        wxPoint pt = ScreenToClient(::wxGetMousePosition());
        wxMouseEvent e(wxEVT_MOTION);
        e.m_x = pt.x;
        e.m_y = pt.y;
        onMouse(e);
    }
}

void CanvasControl::onChar(wxKeyEvent& evt)
{
    // create a new key event and set the target
    IEventPtr basic_evt = KeyEvent::create(wxT(""), this);
    IKeyEventPtr key_evt = basic_evt;

    // set the event location in model coordinates
    int event_x, event_y;
    clientToModel(evt.GetX(), evt.GetY(), &event_x, &event_y);
    key_evt->setX(event_x);
    key_evt->setY(event_y);
    key_evt->setClientX(evt.GetX());
    key_evt->setClientY(evt.GetY());

    // set the key code
    key_evt->setKeyCode(evt.GetKeyCode());
    
    // set the name
    if (evt.GetEventType() == wxEVT_KEY_DOWN)
    {
        basic_evt->setName(EVENT_KEY_DOWN);
    }
     else if (evt.GetEventType() == wxEVT_KEY_UP)
    {
        basic_evt->setName(EVENT_KEY_UP);
    }
     else if (evt.GetEventType() == wxEVT_CHAR)
    {
        basic_evt->setName(EVENT_KEY);
    }

    // set the key state
    key_evt->setAltDown(evt.AltDown());
    key_evt->setCtrlDown(evt.ControlDown());
    key_evt->setShiftDown(evt.ShiftDown());

    // dispatch the canvas key event to the handlers
    dispatchEvent(key_evt);

    // if the canvas key event wasn't handled, skip the 
    // window mouse event so other handlers have a chance
    // to process the event; also, if the event was a key
    // down event, skip it since so that we can process
    // it as a char event
    if (!basic_evt->isHandled() || 
         basic_evt->getName() == EVENT_KEY_DOWN)
    {
        evt.Skip();
    }
}

void CanvasControl::onSetFocus(wxFocusEvent& evt)
{
    // dispatch a set focus event with the focus component
    // as the target; if we don't have a focus component, 
    // use the canvas
    IEventTargetPtr target = getFocus();
    if (target.isNull())
        target = static_cast<IEventTarget*>(this);
     
    INotifyEventPtr notify_set_focus_evt;
    notify_set_focus_evt = NotifyEvent::create(EVENT_FOCUS_SET, target);
    dispatchEvent(notify_set_focus_evt);
}

void CanvasControl::onKillFocus(wxFocusEvent& evt)
{
    // TODO: review how endEdit() is being handled in other
    // places; we really only have to re-render the invalid
    // areas; for now, we'll render everything if we're
    // ending the edit, but this should be changed
    
    // if we're editing, turn off any editing and render
    if (isEditing())
    {
        endEdit(true);
        render();
    }
    
    // dispatch a kill focus event with the focus component
    // as the target; if we don't have a focus component, 
    // use the canvas
    IEventTargetPtr target = getFocus();
    if (target.isNull())
        target = static_cast<IEventTarget*>(this);
     
    INotifyEventPtr notify_kill_focus_evt;
    notify_kill_focus_evt = NotifyEvent::create(EVENT_FOCUS_KILL, target);
    dispatchEvent(notify_kill_focus_evt);
}

void CanvasControl::onScroll(wxScrollWinEvent& evt)
{
    if (evt.GetEventType() == wxEVT_SCROLLWIN_THUMBRELEASE)
    {
    }

    if (evt.GetOrientation() == wxHORIZONTAL)
    {
        int line = wxMax(dtom_x(5), getViewWidth()*SCROLL_LINE_PERC/100);
        int page = wxMax(dtom_x(5), getViewWidth()*SCROLL_PAGE_PERC/100);

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
        {
            scrollBy(-line, 0);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
        {  
            scrollBy(line, 0);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
        {
            scrollBy(-page, 0);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
        {
            scrollBy(page, 0);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK)
        {        
            int x, y;
            getViewOrigin(&x, &y);
            scrollTo(evt.GetPosition(), y);
        }
    }

    if (evt.GetOrientation() == wxVERTICAL)
    {
        int line = wxMax(dtom_y(5), getViewHeight()*SCROLL_LINE_PERC/100);
        int page = wxMax(dtom_y(5), getViewHeight()*SCROLL_PAGE_PERC/100);

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
        {
            scrollBy(0, -line);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
        {
            scrollBy(0, line);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
        {
            scrollBy(0, -page);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
        {
            scrollBy(0, page);
        }

        if (evt.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK)
        {   
            int x, y;
            getViewOrigin(&x, &y);
            scrollTo(x, evt.GetPosition());
        }
    }
}

void CanvasControl::onScrollTimer(wxTimerEvent& evt)
{
    if (m_scroll_timer_voffset == 0 && 
        m_scroll_timer_hoffset == 0)
    {
        return;
    }

    if (m_scroll_timer_voffset)
    {
        scrollBy(0, m_scroll_timer_voffset);
    }

    if (m_scroll_timer_hoffset)
    {
        scrollBy(m_scroll_timer_hoffset, 0);
    }

    // trigger mouse motion event to reset mouse position in 
    // stored variables
    
    // TODO: same thing in onMouse(); is there a better way 
    // to do this?    
    wxPoint pt = ScreenToClient(::wxGetMousePosition());
    wxMouseEvent e(wxEVT_MOTION);
    e.m_x = pt.x;
    e.m_y = pt.y;
    onMouse(e);
}

void CanvasControl::onCursorTimer(wxTimerEvent& evt)
{
    wxClientDC dc(this);
    renderCaret(&dc);
}

void CanvasControl::onItemDragged(CanvasDataObjectComposite& data, 
                                  wxDragResult& result)
{
}

void CanvasControl::onItemDropped(CanvasDataObjectComposite& data, 
                                  wxDragResult& result)
{
    // get the mouse position
    wxPoint pt = ScreenToClient(::wxGetMousePosition());
    clientToModel(pt.x, pt.y, &pt.x, &pt.y);
    
    // get the received data format
    wxDataFormat format = data.GetReceivedFormat();
    
    // if it's invalid, we're done
    if (format == wxDF_INVALID)
    {
        result = wxDragNone;
        return;
    }

    // get the component that we're dropping into; if
    // we don't have a component, we're done
    IComponentPtr target_component;
    target_component = getComponentByPos(pt.x, pt.y);
    if (target_component.isNull())
        return;

    // get the position of the component we're
    // dropping into
    int comp_x, comp_y;
    getComponentPos(target_component, &comp_x, &comp_y);

    // create a component to package up the drop data
    IComponentPtr drop_data = Component::create();

    // if it's our own format, set the drop data component
    // to the component that's already packaged up the
    // data
    if (format == wxDataFormat(component_data_format))
    {   
        // get the component data object
        ComponentDataObject* object;
        object = static_cast<ComponentDataObject*>(data.GetDataObject(component_data_format));
        
        if (!object)
            return;

        // get the component
        drop_data = object->GetComponent();
    }

    // if it's a bitmap, create a bitmap component and
    // package it up into the drop data component
    if (format == wxDF_BITMAP || format == wxDF_DIB)
    {
        // get the bitmap data object
        wxBitmapDataObject* object;
        object = static_cast<wxBitmapDataObject*>(data.GetDataObject(bitmap_data_format));
        
        if (!object)
            return;
    
        // get the image data
        wxBitmap bitmap = object->GetBitmap();
        wxImage wx_image = bitmap.ConvertToImage();
    
        // create an image component
        kcanvas::IComponentPtr comp;
        comp = CompImage::create();

        // set the image
        kcanvas::ICompImagePtr image = comp;
        image->setImage(wx_image);

        // set an initial width and height
        int width = dtom_x(wx_image.GetWidth());
        int height = dtom_y(wx_image.GetHeight());
        comp->setOrigin(pt.x - comp_x, pt.y - comp_y);
        comp->setSize(width, height);
        
        drop_data->add(comp);
    }

    // if it's text, create a text component and
    // package it up into the drop data component
    if (format == wxDF_TEXT || format == wxDF_UNICODETEXT)
    {
        // get the text data object
        wxTextDataObject* object;
        object = static_cast<wxTextDataObject*>(data.GetDataObject(text_data_format));
    
        // if the object is null, return
        if (!object)
            return;
    
        // get the text data
        wxString text = object->GetText();
    
        // create a text component
        kcanvas::IComponentPtr comp;
        comp = CompTextBox::create();

        // turn off the text wrap and set the foreground and 
        // background colors to transparent
        comp->setProperty(PROP_TEXT_WRAP, false);
        comp->setProperty(PROP_COLOR_FG, Color(COLOR_NULL));
        comp->setProperty(PROP_COLOR_BG, Color(COLOR_NULL));

        // set the text
        kcanvas::ICompTextBoxPtr textbox = comp;
        textbox->setText(text);

        // set the size of the component to the
        // space occupied by the text
        wxRect extent;
        comp->extends(extent);
        comp->setOrigin(pt.x - comp_x, pt.y - comp_y);
        comp->setSize(extent.width, extent.height);

        drop_data->add(comp);
    }

    // create a drop event and add the drop data
    IDragDropEventPtr drop_evt;
    drop_evt = DragDropEvent::create(EVENT_DROP, target_component);
    drop_evt->setX(pt.x - comp_x);
    drop_evt->setY(pt.y - comp_y);
    drop_evt->setData(drop_data);
    dispatchEvent(drop_evt);

    result = wxDragNone;
}

void CanvasControl::allocBitmap(int width, int height)
{
    if (width < 16)
        width = 16;

    if (height < 16)
        height = 16;

    if (width <= m_memwidth && height <= m_memheight)
        return;

    m_memwidth = width;
    m_memheight = height;

    m_bmp.Create(m_memwidth, m_memheight, -1);
    m_memdc.SelectObject(m_bmp);
    setDC(&m_memdc);
}

void CanvasControl::repaint(const wxRect& rect, wxDC* dc)
{
    // note: the input rect is in device coordinates, not
    // model coordinates like render(); TODO: make repaint
    // work with model coordinates, like render(), for
    // consistency

    // dispatch a pre-paint event
    INotifyEventPtr notify_pre_evt;
    notify_pre_evt = NotifyEvent::create(EVENT_PRE_PAINT, this);
    dispatchEvent(notify_pre_evt);

    if (!notify_pre_evt->isAllowed())
        return;

    // get the client rectangle
    wxRect blit_rect;
    GetClientSize(&blit_rect.width, &blit_rect.height);

    if (!rect.IsEmpty())
        blit_rect = rect;

    // make sure the draw origin is zero again
    setDrawOrigin(0,0);
    
    wxRect r;
    getViewOrigin(&r.x, &r.y);
    getViewSize(&r.width, &r.height);

    // do the repaint
    if (dc)
    {
        dc->Blit(blit_rect.x,
                 blit_rect.y,
                 blit_rect.width,
                 blit_rect.height, getDC(), blit_rect.x, blit_rect.y);
    }
    else
    {
        wxClientDC cdc(this);
        cdc.Blit(blit_rect.x,
                 blit_rect.y,
                 blit_rect.width,
                 blit_rect.height, getDC(), blit_rect.x, blit_rect.y);
    }
    
    // dispatch a paint event
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_PAINT, this);
    dispatchEvent(notify_evt);
}

void CanvasControl::renderCaret(wxDC* dc)
{
    IEditPtr editor;
    editor = getFocus();

    // return if there's no edit component
    if (editor.isNull())
        return;

    // return if the caret is not active
    if (!m_caret_active)
        return;

    // draw the caret
    dc->SetPen(*wxBLACK_PEN);
    dc->SetBrush(*wxBLACK_BRUSH);
    dc->SetLogicalFunction(wxINVERT);

    int x1, y1, x2, y2;
    modelToClient(m_caret_x, m_caret_y, &x1, &y1);
    modelToClient(m_caret_x, m_caret_y + m_caret_h, &x2, &y2);

    // draw cursor
    dc->DrawLine(x1, y1, x2, y2);
}

void CanvasControl::setCaret(int x, int y, int h)
{
    // set the caret position
    m_caret_x = x;
    m_caret_y = y;
    m_caret_h = h;
}

void CanvasControl::startCaret()
{
    // set the caret active flag to true
    m_caret_active = true;

    // start the timer
    if (!m_cursor_timer.IsRunning())
        m_cursor_timer.Start(500);
}

void CanvasControl::stopCaret()
{
    // set the caret active flag to false
    m_caret_active = false;

    // stop the timer
    if (m_cursor_timer.IsRunning())
        m_cursor_timer.Stop();

    // repaint
    repaint();
}

void CanvasControl::updateScrollbars()
{
    // get the canvas size
    int width, height;
    getSize(&width, &height);

    // get the canvas view size
    int view_x, view_y, view_w, view_h;
    getViewOrigin(&view_x, &view_y);
    getViewSize(&view_w, &view_h);

    // update the scrollbars
    width = wxMax(view_w, width);
    height = wxMax(view_h, height);

    if (wxHSCROLL & GetWindowStyleFlag())
        SetScrollbar(wxHORIZONTAL, view_x, view_w, width + 1, true);

    if (wxVSCROLL & GetWindowStyleFlag())
        SetScrollbar(wxVERTICAL, view_y, view_h, height + 1, true);
}


void CanvasControl::renderSelection(const wxRect& rect)
{
    wxDC* dc = getDC();

    if (!dc)
        return;

    int size = 5;

    int x, y, w, h;
    x = -size + mtod_x(m_draw_origin_x);
    y = -size + mtod_y(m_draw_origin_y);
    w = mtod_x(rect.width) + 2*size;
    h = mtod_y(rect.height) + 2*size;

    wxBitmap bm, bitmap;
    wxImage image;

    bitmap = wxBitmap(xpm_noedit_blue);
    image = bitmap.ConvertToImage();

    image.SetMaskColour(255,255,255);
    image.SetMask(true);
    bitmap = wxBitmap(image);

    int x_pos, y_pos;

    int bitmap_width = bitmap.GetWidth();
    int bitmap_height = bitmap.GetHeight();

    int t_width = wxMin(size, bitmap_width);
    int t_height = wxMin(size, bitmap_height);

    // draw top
    bm = bitmap.GetSubBitmap(wxRect(0, 0, bitmap_width, t_height));
    for (x_pos = 0; x_pos + bitmap_width <= w; x_pos = x_pos + bitmap_width)
        dc->DrawBitmap(bm, x + x_pos, y, true);

    // draw bottom
    bm = bitmap.GetSubBitmap(wxRect(0, 0, bitmap_width, t_height));
    for (x_pos = 0; x_pos + bitmap_width <= w; x_pos = x_pos + bitmap_width)
        dc->DrawBitmap(bm, x + x_pos, y + h - t_height, true);

    // draw left
    bm = bitmap.GetSubBitmap(wxRect(0, 0, t_width, bitmap_height));
    for (y_pos = 0; y_pos + bitmap_height <= h; y_pos = y_pos + bitmap_height)
        dc->DrawBitmap(bm, x, y + y_pos, true);

    // draw right
    bm = bitmap.GetSubBitmap(wxRect(0, 0, t_width, bitmap_height));
    for (y_pos = 0; y_pos + bitmap_height <= h; y_pos = y_pos + bitmap_height)
        dc->DrawBitmap(bm, x + w - t_width, y + y_pos, true);

    // set the size of the line
    int line_size = 1;
    int handle_size = dtom_x(size);
    
    // draw the selection rectangle
    setPen(Pen(COLOR_BLUE, line_size));
    setBrush(Brush(COLOR_BLACK, wxTRANSPARENT));
    drawRectangle(0, 0, rect.width, rect.height);
    
    // set handle brush
    setBrush(Brush(COLOR_BLUE));
    
    // draw corner handles
    drawRectangle(-handle_size, -handle_size, handle_size, handle_size);
    drawRectangle(rect.width, -handle_size, handle_size, handle_size);
    drawRectangle(rect.width, rect.height, handle_size, handle_size);
    drawRectangle(-handle_size, rect.height, handle_size, handle_size);
}

void CanvasControl::renderSelector(wxRect* rect1, wxRect* rect2)
{
    // TODO: should move this to CanvasControl; however,
    // it's here for now since it's called in the
    // mouse action code
/*
    if (rect1 == NULL || rect2 == NULL)
        return;

    wxClientDC dc(this);

    Pen hint_pen(COLOR_BLACK, 1, wxDOT);
    dc.SetPen(hint_pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetLogicalFunction(wxINVERT);

    if (!m_selection_drawn)
    {
        dc.DrawRectangle(rect1->GetX(), rect1->GetY(), rect1->GetWidth(), rect1->GetHeight());
    }
    else
    {
        dc.DrawRectangle(rect2->GetX(), rect2->GetY(), rect2->GetWidth(), rect2->GetHeight());
        dc.DrawRectangle(rect1->GetX(), rect1->GetY(), rect1->GetWidth(), rect1->GetHeight());
    }

    m_selection_drawn = true;
*/
}


}; // namespace kcanvas

