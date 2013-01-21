/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2008-03-11
 *
 */


#ifndef __KCANVAS_CANVASCONTROL_H
#define __KCANVAS_CANVASCONTROL_H


namespace kcanvas
{


// Canvas Control Classes

class CanvasDataObjectComposite;

class CanvasControl : public wxControl,
                      public CanvasDC,
                      public IEdit,                      
                      public ICanvasControl
{
    XCM_CLASS_NAME_NOREFCOUNT("kcanvas.CanvasControl")
    XCM_BEGIN_INTERFACE_MAP(CanvasControl)
        XCM_INTERFACE_ENTRY(ICanvasControl)
        XCM_INTERFACE_CHAIN(CanvasDC)
        XCM_INTERFACE_ENTRY(IEdit)
    XCM_END_INTERFACE_MAP()

public:

    CanvasControl(wxWindow* parent,
                  wxWindowID id = -1,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = (0 | wxHSCROLL | wxVSCROLL | wxCLIP_CHILDREN | wxWANTS_CHARS));
 
    virtual ~CanvasControl();

    static ICanvasPtr create(wxWindow* parent,
                             wxWindowID id = -1,
                             const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize,
                             long style = (0 | wxHSCROLL | wxVSCROLL | wxCLIP_CHILDREN | wxWANTS_CHARS));

public:

    // ICanvasControl

    wxWindow* getWindow();

public:

    // ICanvas

    ICanvasPtr clone();
    void copy(ICanvasPtr canvas);

    void getViewSize(int* x, int* y) const;
    int getViewWidth() const;
    int getViewHeight() const;

    void render(const wxRect& rect = wxRect());

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

private:

    void onCanvasEvent(IEventPtr evt);
    void onCanvasScrollEvent(IEventPtr evt);
    void onCanvasLayoutEvent(IEventPtr evt);    
    void onCanvasPreRenderEvent(IEventPtr evt);
    void onCanvasCaretEvent(IEventPtr evt);    
    void onCanvasFocusEvent(IEventPtr evt);
    void onCanvasDragDropEvent(IEventPtr evt);
    void onCanvasEditBeginEvent(IEventPtr evt);
    void onCanvasEditEndEvent(IEventPtr evt);

private:

    // events handlers
    void onSize(wxSizeEvent& evt);
    void onPaint(wxPaintEvent& evt);
    void onEraseBackground(wxEraseEvent& evt);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& evt);
    void onMouse(wxMouseEvent& evt);
    void onChar(wxKeyEvent& evt);
    void onSetFocus(wxFocusEvent& evt);
    void onKillFocus(wxFocusEvent& evt);
    void onScroll(wxScrollWinEvent& evt);
    void onScrollTimer(wxTimerEvent& evt);
    void onCursorTimer(wxTimerEvent& evt);
    void onItemDragged(CanvasDataObjectComposite& data, wxDragResult& result);
    void onItemDropped(CanvasDataObjectComposite& data, wxDragResult& result);

    DECLARE_EVENT_TABLE()

private:

    // function for allocating the memory bitmap and
    // transferring the memory bitmap the the screen
    void allocBitmap(int width, int height);
    void repaint(const wxRect& rect = wxRect(), wxDC* dc = NULL);

    // function for rendering the text-edit caret
    void renderCaret(wxDC* dc);

    // functions for setting/starting/stopping caret
    void setCaret(int x, int y, int h);
    void startCaret();
    void stopCaret();
    
    // function for updating the scrollbars
    void updateScrollbars();

    // functions for rendering the component selection indicator
    // and the selection rubber band
    void renderSelection(const wxRect& rect);
    void renderSelector(wxRect* rect1, wxRect* rect2);

private:

    // memory variables for off-screen drawable
    wxBitmap m_bmp;
    wxMemoryDC m_memdc;
    int m_memwidth;
    int m_memheight;

    // timer variables
    wxTimer m_cursor_timer;
    wxTimer m_scroll_timer;
    int m_scroll_timer_voffset;
    int m_scroll_timer_hoffset;

    // text edit cursor variables
    int m_caret_x;
    int m_caret_y;
    int m_caret_h;
    bool m_caret_active;
    
    // selection hint rectangles and drawing flags
    wxRect m_selection_old;
    wxRect m_selection_new;    
};


}; // namespace kcanvas


#endif

