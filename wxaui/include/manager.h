///////////////////////////////////////////////////////////////////////////////
// Name:        manager.h
// Purpose:     wxaui: wx advanced user interface - docking window manager
// Author:      Benjamin I. Williams
// Modified by:
// Created:     2005-05-17
// RCS-ID:      
// Copyright:   Copyright (c) 2005, Kirix Research, LLC.  All rights reserved.
// Licence:     wxWindows Library Licence, Version 3.1
///////////////////////////////////////////////////////////////////////////////


#ifndef __WXAUI_WXAUI_H
#define __WXAUI_WXAUI_H


enum wxFrameManagerDock
{
    wxAUI_DOCK_NONE = 0,
    wxAUI_DOCK_TOP = 1,
    wxAUI_DOCK_RIGHT = 2,
    wxAUI_DOCK_BOTTOM = 3,
    wxAUI_DOCK_LEFT = 4,
    wxAUI_DOCK_CENTER = 5,
    wxAUI_DOCK_CENTRE = wxAUI_DOCK_CENTER
};

enum wxFrameManagerOption
{
    wxAUI_MGR_ALLOW_FLOATING        = 1 << 0,
    wxAUI_MGR_ALLOW_ACTIVE_PANE     = 1 << 1,
    wxAUI_MGR_TRANSPARENT_DRAG      = 1 << 2,
    wxAUI_MGR_TRANSPARENT_HINT      = 1 << 3,
    wxAUI_MGR_TRANSPARENT_HINT_FADE = 1 << 4,

    wxAUI_MGR_DEFAULT = wxAUI_MGR_ALLOW_FLOATING |
                        wxAUI_MGR_TRANSPARENT_HINT |
                        wxAUI_MGR_TRANSPARENT_HINT_FADE
};

enum wxPaneDockArtSetting
{
    wxAUI_ART_SASH_SIZE = 0,
    wxAUI_ART_CAPTION_SIZE = 1,
    wxAUI_ART_GRIPPER_SIZE = 2,
    wxAUI_ART_PANE_BORDER_SIZE = 3,
    wxAUI_ART_PANE_BUTTON_SIZE = 4,
    wxAUI_ART_BACKGROUND_COLOUR = 5,
    wxAUI_ART_SASH_COLOUR = 6,
    wxAUI_ART_ACTIVE_CAPTION_COLOUR = 7,
    wxAUI_ART_ACTIVE_CAPTION_GRADIENT_COLOUR = 8,
    wxAUI_ART_INACTIVE_CAPTION_COLOUR = 9,
    wxAUI_ART_INACTIVE_CAPTION_GRADIENT_COLOUR = 10,
    wxAUI_ART_ACTIVE_CAPTION_TEXT_COLOUR = 11,
    wxAUI_ART_INACTIVE_CAPTION_TEXT_COLOUR = 12,
    wxAUI_ART_BORDER_COLOUR = 13,
    wxAUI_ART_GRIPPER_COLOUR = 14,
    wxAUI_ART_CAPTION_FONT = 15,
    wxAUI_ART_GRADIENT_TYPE = 16
};

enum wxPaneDockArtGradients
{
    wxAUI_GRADIENT_NONE = 0,
    wxAUI_GRADIENT_VERTICAL = 1,
    wxAUI_GRADIENT_HORIZONTAL = 2
};

enum wxPaneButtonState
{
    wxAUI_BUTTON_STATE_NORMAL = 0,
    wxAUI_BUTTON_STATE_HOVER = 1,
    wxAUI_BUTTON_STATE_PRESSED = 2
};

enum wxPaneInsertLevel
{
    wxAUI_INSERT_PANE = 0,
    wxAUI_INSERT_ROW = 1,
    wxAUI_INSERT_DOCK = 2
};



// forwards and array declarations
class wxDockUIPart;
class wxPaneButton;
class wxPaneInfo;
class wxDockInfo;
class wxDockArt;
class wxFrameManagerEvent;

WX_DECLARE_OBJARRAY(wxDockInfo, wxDockInfoArray);
WX_DECLARE_OBJARRAY(wxDockUIPart, wxDockUIPartArray);
WX_DECLARE_OBJARRAY(wxPaneButton, wxPaneButtonArray);
WX_DECLARE_OBJARRAY(wxPaneInfo, wxPaneInfoArray);
WX_DEFINE_ARRAY_PTR(wxPaneInfo*, wxPaneInfoPtrArray);
WX_DEFINE_ARRAY_PTR(wxDockInfo*, wxDockInfoPtrArray);

extern wxDockInfo wxNullDockInfo;
extern wxPaneInfo wxNullPaneInfo;




class WXDLLEXPORT wxPaneInfo
{
public:

    wxPaneInfo()
    {
        window = NULL;
        frame = NULL;
        state = 0;
        dock_direction = wxAUI_DOCK_LEFT;
        dock_layer = 0;
        dock_row = 0;
        dock_pos = 0;
        floating_pos = wxDefaultPosition;
        floating_size = wxDefaultSize;
        best_size = wxDefaultSize;
        min_size = wxDefaultSize;
        max_size = wxDefaultSize;
        dock_proportion = 0;
        
        DefaultPane();
    }

    wxPaneInfo(const wxPaneInfo& c)
    {
        name = c.name;
        caption = c.caption;
        window = c.window;
        frame = c.frame;
        state = c.state;
        dock_direction = c.dock_direction;
        dock_layer = c.dock_layer;
        dock_row = c.dock_row;
        dock_pos = c.dock_pos;
        best_size = c.best_size;
        min_size = c.min_size;
        max_size = c.max_size;
        floating_pos = c.floating_pos;
        floating_size = c.floating_size;
        dock_proportion = c.dock_proportion;
        buttons = c.buttons;
        rect = c.rect;
    }

    wxPaneInfo& operator=(const wxPaneInfo& c)
    {
        name = c.name;
        caption = c.caption;
        window = c.window;
        frame = c.frame;
        state = c.state;
        dock_direction = c.dock_direction;
        dock_layer = c.dock_layer;
        dock_row = c.dock_row;
        dock_pos = c.dock_pos;
        best_size = c.best_size;
        min_size = c.min_size;
        max_size = c.max_size;
        floating_pos = c.floating_pos;
        floating_size = c.floating_size;
        dock_proportion = c.dock_proportion;
        buttons = c.buttons;
        rect = c.rect;
        return *this;
    }

    bool IsOk() const { return (window != NULL) ? true : false; }
    bool IsFixed() const { return !HasFlag(optionResizable); }
    bool IsResizable() const { return HasFlag(optionResizable); }
    bool IsShown() const { return !HasFlag(optionHidden); }
    bool IsFloating() const { return HasFlag(optionFloating); }
    bool IsDocked() const { return !HasFlag(optionFloating); }
    bool IsToolbar() const { return HasFlag(optionToolbar); }
    bool IsTopDockable() const { return HasFlag(optionTopDockable); }
    bool IsBottomDockable() const { return HasFlag(optionBottomDockable); }
    bool IsLeftDockable() const { return HasFlag(optionLeftDockable); }
    bool IsRightDockable() const { return HasFlag(optionRightDockable); }
    bool IsFloatable() const { return HasFlag(optionFloatable); }
    bool IsMovable() const { return HasFlag(optionMovable); }
    bool HasCaption() const { return HasFlag(optionCaption); }
    bool HasGripper() const { return HasFlag(optionGripper); }
    bool HasBorder() const { return HasFlag(optionPaneBorder); }
    bool HasCloseButton() const { return HasFlag(buttonClose); }
    bool HasMaximizeButton() const { return HasFlag(buttonMaximize); }
    bool HasMinimizeButton() const { return HasFlag(buttonMinimize); }
    bool HasPinButton() const { return HasFlag(buttonPin); }
    
    wxPaneInfo& Window(wxWindow* w) { window = w; return *this; }
    wxPaneInfo& Name(const wxString& n) { name = n; return *this; }
    wxPaneInfo& Caption(const wxString& c) { caption = c; return *this; }
    wxPaneInfo& Left() { dock_direction = wxAUI_DOCK_LEFT; return *this; }
    wxPaneInfo& Right() { dock_direction = wxAUI_DOCK_RIGHT; return *this; }
    wxPaneInfo& Top() { dock_direction = wxAUI_DOCK_TOP; return *this; }
    wxPaneInfo& Bottom() { dock_direction = wxAUI_DOCK_BOTTOM; return *this; }
    wxPaneInfo& Center() { dock_direction = wxAUI_DOCK_CENTER; return *this; }
    wxPaneInfo& Centre() { dock_direction = wxAUI_DOCK_CENTRE; return *this; }
    wxPaneInfo& Direction(int direction) { dock_direction = direction; return *this; }
    wxPaneInfo& Layer(int layer) { dock_layer = layer; return *this; }
    wxPaneInfo& Row(int row) { dock_row = row; return *this; }
    wxPaneInfo& Position(int pos) { dock_pos = pos; return *this; }
    wxPaneInfo& BestSize(const wxSize& size) { best_size = size; return *this; }
    wxPaneInfo& MinSize(const wxSize& size) { min_size = size; return *this; }
    wxPaneInfo& MaxSize(const wxSize& size) { max_size = size; return *this; }
    wxPaneInfo& BestSize(int x, int y) { best_size.Set(x,y); return *this; }
    wxPaneInfo& MinSize(int x, int y) { min_size.Set(x,y); return *this; }
    wxPaneInfo& MaxSize(int x, int y) { max_size.Set(x,y); return *this; }
    wxPaneInfo& FloatingPosition(const wxPoint& pos) { floating_pos = pos; return *this; }
    wxPaneInfo& FloatingPosition(int x, int y) { floating_pos.x = x; floating_pos.y = y; return *this; }
    wxPaneInfo& FloatingSize(const wxSize& size) { floating_size = size; return *this; }
    wxPaneInfo& FloatingSize(int x, int y) { floating_size.Set(x,y); return *this; }
    wxPaneInfo& Fixed() { return SetFlag(optionResizable, false); }
    wxPaneInfo& Resizable(bool resizable = true) { return SetFlag(optionResizable, resizable); }
    wxPaneInfo& Dock() { return SetFlag(optionFloating, false); }
    wxPaneInfo& Float() { return SetFlag(optionFloating, true); }
    wxPaneInfo& Hide() { return SetFlag(optionHidden, true); }
    wxPaneInfo& Show(bool show = true) { return SetFlag(optionHidden, !show); }
    wxPaneInfo& CaptionVisible(bool visible = true) { return SetFlag(optionCaption, visible); }
    wxPaneInfo& PaneBorder(bool visible = true) { return SetFlag(optionPaneBorder, visible); }
    wxPaneInfo& Gripper(bool visible = true) { return SetFlag(optionGripper, visible); }
    wxPaneInfo& CloseButton(bool visible = true) { return SetFlag(buttonClose, visible); }  
    wxPaneInfo& MaximizeButton(bool visible = true) { return SetFlag(buttonMaximize, visible); }
    wxPaneInfo& MinimizeButton(bool visible = true) { return SetFlag(buttonMinimize, visible); }
    wxPaneInfo& PinButton(bool visible = true) { return SetFlag(buttonPin, visible); }
    wxPaneInfo& DestroyOnClose(bool b = true) { return SetFlag(optionDestroyOnClose, b); }
    wxPaneInfo& TopDockable(bool b = true) { return SetFlag(optionTopDockable, b); }
    wxPaneInfo& BottomDockable(bool b = true) { return SetFlag(optionBottomDockable, b); }
    wxPaneInfo& LeftDockable(bool b = true) { return SetFlag(optionLeftDockable, b); }
    wxPaneInfo& RightDockable(bool b = true) { return SetFlag(optionRightDockable, b); } 
    wxPaneInfo& Floatable(bool b = true) { return SetFlag(optionFloatable, b); }
    wxPaneInfo& Movable(bool b = true) { return SetFlag(optionMovable, b); }
    wxPaneInfo& Dockable(bool b = true)
    {
        return TopDockable(b).BottomDockable(b).LeftDockable(b).RightDockable(b);
    }

    wxPaneInfo& DefaultPane()
    {
        state |= optionTopDockable | optionBottomDockable |
                 optionLeftDockable | optionRightDockable |
                 optionFloatable | optionMovable | optionResizable |
                 optionCaption | optionPaneBorder | buttonClose;
        return *this;
    }
    
    wxPaneInfo& CentrePane() { return CenterPane(); }
    wxPaneInfo& CenterPane()
    {
        state = 0;
        return Center().PaneBorder().Resizable();
    }
     
    wxPaneInfo& ToolbarPane()
    {
        DefaultPane();
        state |= (optionToolbar | optionGripper);
        state &= ~(optionResizable | optionCaption);
        if (dock_layer == 0)
            dock_layer = 10;
        return *this;
    }

    wxPaneInfo& SetFlag(unsigned int flag, bool option_state)
    {
        if (option_state)
            state |= flag;
             else
            state &= ~flag;
        return *this;
    }
    
    bool HasFlag(unsigned int flag) const
    {
        return (state & flag) ? true:false;
    }

public:

    enum wxPaneState
    {
        optionFloating        = 1 << 0,
        optionHidden          = 1 << 1,
        optionLeftDockable    = 1 << 2,
        optionRightDockable   = 1 << 3,
        optionTopDockable     = 1 << 4,
        optionBottomDockable  = 1 << 5,
        optionFloatable       = 1 << 6,
        optionMovable         = 1 << 7,
        optionResizable       = 1 << 8,
        optionPaneBorder      = 1 << 9,
        optionCaption         = 1 << 10,
        optionGripper         = 1 << 11,
        optionDestroyOnClose  = 1 << 12,
        optionToolbar         = 1 << 13,
        optionActive          = 1 << 14,

        buttonClose           = 1 << 24,
        buttonMaximize        = 1 << 25,
        buttonMinimize        = 1 << 26,
        buttonPin             = 1 << 27,
        buttonCustom1         = 1 << 28,
        buttonCustom2         = 1 << 29,
        buttonCustom3         = 1 << 30,
        actionPane            = 1 << 31  // used internally
    };

public:
    wxString name;        // name of the pane
    wxString caption;     // caption displayed on the window

    wxWindow* window;     // window that is in this pane
    wxWindow* frame;      // floating frame window that holds the pane
    unsigned int state;   // a combination of wxPaneState values

    int dock_direction;   // dock direction (top, bottom, left, right, center)
    int dock_layer;       // layer number (0 = innermost layer)
    int dock_row;         // row number on the docking bar (0 = first row)
    int dock_pos;         // position inside the row (0 = first position)

    wxSize best_size;     // size that the layout engine will prefer
    wxSize min_size;      // minimum size the pane window can tolerate
    wxSize max_size;      // maximum size the pane window can tolerate

    wxPoint floating_pos; // position while floating
    wxSize floating_size; // size while floating
    int dock_proportion;  // proportion while docked

    wxPaneButtonArray buttons; // buttons on the pane

    wxRect rect;              // current rectangle (populated by wxAUI)
};





class WXDLLEXPORT wxFrameManager : public wxEvtHandler
{
friend class wxFloatingPane;

public:

    wxFrameManager(wxFrame* frame = NULL,
                   unsigned int flags = wxAUI_MGR_DEFAULT);
    virtual ~wxFrameManager();
    void UnInit();
    
    void SetFlags(unsigned int flags);
    unsigned int GetFlags() const;
    
    void SetFrame(wxFrame* frame);
    wxFrame* GetFrame() const;
    
    void SetArtProvider(wxDockArt* art_provider);
    wxDockArt* GetArtProvider() const;

    wxPaneInfo& GetPane(wxWindow* window);
    wxPaneInfo& GetPane(const wxString& name);
    wxPaneInfoArray& GetAllPanes();

    bool AddPane(wxWindow* window,
                 const wxPaneInfo& pane_info);
                 
    bool AddPane(wxWindow* window,
                 int direction = wxLEFT,
                 const wxString& caption = wxEmptyString);
                 
    bool InsertPane(wxWindow* window,
                 const wxPaneInfo& pane_info,
                 int insert_level = wxAUI_INSERT_PANE);
                 
    bool DetachPane(wxWindow* window);

    wxString SavePerspective();
    
    bool LoadPerspective(const wxString& perspective,
                 bool update = true);
    
    void Update();

private:

    void DrawHintRect(wxWindow* pane_window,
                       const wxPoint& pt,
                       const wxPoint& offset);

    void DoFrameLayout();

    void LayoutAddPane(wxSizer* container,
                       wxDockInfo& dock,
                       wxPaneInfo& pane,
                       wxDockUIPartArray& uiparts,
                       bool spacer_only);

    void LayoutAddDock(wxSizer* container,
                       wxDockInfo& dock,
                       wxDockUIPartArray& uiparts,
                       bool spacer_only);

    wxSizer* LayoutAll(wxPaneInfoArray& panes,
                       wxDockInfoArray& docks,
                       wxDockUIPartArray& uiparts,
                       bool spacer_only = false);

    bool DoDrop(wxDockInfoArray& docks,
                wxPaneInfoArray& panes,
                wxPaneInfo& drop,
                const wxPoint& pt,
                const wxPoint& action_offset = wxPoint(0,0));

    wxPaneInfo& LookupPane(wxWindow* window);
    wxPaneInfo& LookupPane(const wxString& name);
    wxDockUIPart* HitTest(int x, int y);
    wxDockUIPart* GetPanePart(wxWindow* pane);
    int GetDockPixelOffset(wxPaneInfo& test);
    void OnFloatingPaneMoveStart(wxWindow* window);
    void OnFloatingPaneMoving(wxWindow* window);
    void OnFloatingPaneMoved(wxWindow* window);
    void OnFloatingPaneActivated(wxWindow* window);
    void OnFloatingPaneClosed(wxWindow* window);
    void OnFloatingPaneResized(wxWindow* window, const wxSize& size);
    void Render(wxDC* dc);
    void Repaint(wxDC* dc = NULL);
    void ProcessMgrEvent(wxFrameManagerEvent& event);
    void UpdateButtonOnScreen(wxDockUIPart* button_ui_part,
                              const wxMouseEvent& event);
    void GetPanePositionsAndSizes(wxDockInfo& dock,
                              wxArrayInt& positions,
                              wxArrayInt& sizes);
    void ShowHint(const wxRect& rect);
    void HideHint();
    void RemoveHint();

private:

    // events
    void OnPaint(wxPaintEvent& event);
    void OnEraseBackground(wxEraseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnSetCursor(wxSetCursorEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnLeaveWindow(wxMouseEvent& event);
    void OnPaneButton(wxFrameManagerEvent& event);
    void OnChildFocus(wxChildFocusEvent& event);
    void OnHintFadeTimer(wxTimerEvent& event);

private:

    enum
    {
        actionNone = 0,
        actionResize,
        actionClickButton,
        actionClickCaption,
        actionDragToolbarPane,
        actionDragFloatingPane
    };

private:

    wxFrame* m_frame;            // the frame being managed
    wxDockArt* m_art;            // dock art object which does all drawing
    unsigned int m_flags;        // manager flags wxAUI_MGR_*

    wxPaneInfoArray m_panes;     // array of panes structures
    wxDockInfoArray m_docks;     // array of docks structures
    wxDockUIPartArray m_uiparts; // array of UI parts (captions, buttons, etc)

    int m_action;                // current mouse action
    wxPoint m_action_start;      // position where the action click started
    wxPoint m_action_offset;     // offset from upper left of the item clicked
    wxDockUIPart* m_action_part; // ptr to the part the action happened to
    wxWindow* m_action_window;   // action frame or window (NULL if none)
    wxRect m_action_hintrect;    // hint rectangle for the action
    wxDockUIPart* m_hover_button;// button uipart being hovered over
    wxRect m_last_hint;          // last hint rectangle
    wxPoint m_last_mouse_move;   // last mouse move position (see OnMotion)

    wxWindow* m_hint_wnd;        // transparent hint window (for now, only msw)
    wxTimer m_hint_fadetimer;    // transparent fade timer (for now, only msw)
    int m_hint_fadeamt;          // transparent fade amount (for now, only msw)
    
    DECLARE_EVENT_TABLE()
};



// event declarations/classes

class WXDLLEXPORT wxFrameManagerEvent : public wxEvent
{
public:
    wxFrameManagerEvent(wxEventType type) : wxEvent(0, type)
    {
        pane = NULL;
        button = 0;
    }

    wxFrameManagerEvent(const wxFrameManagerEvent& c) : wxEvent(c)
    {
        pane = c.pane;
        button = c.button;
    }

    wxEvent *Clone() const { return new wxFrameManagerEvent(*this); }

    void SetPane(wxPaneInfo* p) { pane = p; }
    void SetButton(int b) { button = b; }
    wxPaneInfo* GetPane() { return pane; }
    int GetButton() { return button; }

public:
    wxPaneInfo* pane;
    int button;
};




// dock art provider code - a dock provider provides all drawing
// functionality to the wxAui dock manager.  This allows the dock
// manager to have plugable look-and-feels

class wxDockArt
{
public:

    wxDockArt() { }
    virtual ~wxDockArt() { }

    virtual int GetMetric(int id) = 0;
    virtual void SetMetric(int id, int new_val) = 0;
    virtual void SetFont(int id, const wxFont& font) = 0;
    virtual wxFont GetFont(int id) = 0;
    virtual wxColour GetColour(int id) = 0;
    virtual void SetColour(int id, const wxColor& colour) = 0;
    wxColor GetColor(int id) { return GetColour(id); }
    void SetColor(int id, const wxColor& color) { SetColour(id, color); }
    
    virtual void DrawSash(wxDC& dc,
                          int orientation,
                          const wxRect& rect) = 0;

    virtual void DrawBackground(wxDC& dc,
                          int orientation,
                          const wxRect& rect) = 0;

    virtual void DrawCaption(wxDC& dc,
                          const wxString& text,
                          const wxRect& rect,
                          wxPaneInfo& pane) = 0;

    virtual void DrawGripper(wxDC& dc,
                          const wxRect& rect,
                          wxPaneInfo& pane) = 0;

    virtual void DrawBorder(wxDC& dc,
                          const wxRect& rect,
                          wxPaneInfo& pane) = 0;

    virtual void DrawPaneButton(wxDC& dc,
                          int button,
                          int button_state,
                          const wxRect& rect,
                          wxPaneInfo& pane) = 0;
};


// this is the default art provider for wxFrameManager.  Dock art
// can be customized by creating a class derived from this one,
// or replacing this class entirely

class wxDefaultDockArt : public wxDockArt
{
public:

    wxDefaultDockArt();

    int GetMetric(int metric_id);
    void SetMetric(int metric_id, int new_val);
    wxColour GetColour(int id);
    void SetColour(int id, const wxColor& colour);
    void SetFont(int id, const wxFont& font);
    wxFont GetFont(int id);

    void DrawSash(wxDC& dc,
                  int orientation,
                  const wxRect& rect);

    void DrawBackground(wxDC& dc,
                  int orientation,
                  const wxRect& rect);

    void DrawCaption(wxDC& dc,
                  const wxString& text,
                  const wxRect& rect,
                  wxPaneInfo& pane);

    void DrawGripper(wxDC& dc,
                  const wxRect& rect,
                  wxPaneInfo& pane);

    void DrawBorder(wxDC& dc,
                  const wxRect& rect,
                  wxPaneInfo& pane);

    void DrawPaneButton(wxDC& dc,
                  int button,
                  int button_state,
                  const wxRect& rect,
                  wxPaneInfo& pane);

protected:

    void DrawCaptionBackground(wxDC& dc, const wxRect& rect, bool active);

protected:

    wxPen m_border_pen;
    wxBrush m_sash_brush;
    wxBrush m_background_brush;
    wxBrush m_gripper_brush;
    wxFont m_caption_font;
    wxBitmap m_inactive_close_bitmap;
    wxBitmap m_inactive_pin_bitmap;
    wxBitmap m_active_close_bitmap;
    wxBitmap m_active_pin_bitmap;
    wxPen m_gripper_pen1;
    wxPen m_gripper_pen2;
    wxPen m_gripper_pen3;
    wxColour m_active_caption_colour;
    wxColour m_active_caption_gradient_colour;
    wxColour m_active_caption_text_colour;
    wxColour m_inactive_caption_colour;
    wxColour m_inactive_caption_gradient_colour;
    wxColour m_inactive_caption_text_colour;
    int m_border_size;
    int m_caption_size;
    int m_sash_size;
    int m_button_size;
    int m_gripper_size;
    int m_gradient_type;
};


class wxDockInfo
{
public:
    wxDockInfo()
    {
        dock_direction = 0;
        dock_layer = 0;
        dock_row = 0;
        size = 0;
        min_size = 0;
        resizable = true;
        fixed = false;
        toolbar = false;
    }

    wxDockInfo(const wxDockInfo& c)
    {
        dock_direction = c.dock_direction;
        dock_layer = c.dock_layer;
        dock_row = c.dock_row;
        size = c.size;
        min_size = c.min_size;
        resizable = c.resizable;
        fixed = c.fixed;
        toolbar = c.toolbar;
        panes = c.panes;
        rect = c.rect;
    }

    wxDockInfo& operator=(const wxDockInfo& c)
    {
        dock_direction = c.dock_direction;
        dock_layer = c.dock_layer;
        dock_row = c.dock_row;
        size = c.size;
        min_size = c.min_size;
        resizable = c.resizable;
        fixed = c.fixed;
        toolbar = c.toolbar;
        panes = c.panes;
        rect = c.rect;
        return *this;
    }

    bool IsOk() const { return (dock_direction != 0) ? true : false; }
    bool IsHorizontal() const { return (dock_direction == wxAUI_DOCK_TOP ||
                             dock_direction == wxAUI_DOCK_BOTTOM) ? true:false; }
    bool IsVertical() const { return (dock_direction == wxAUI_DOCK_LEFT ||
                             dock_direction == wxAUI_DOCK_RIGHT ||
                             dock_direction == wxAUI_DOCK_CENTER) ? true:false; }
public:
    wxPaneInfoPtrArray panes; // array of panes
    wxRect rect;              // current rectangle
    int dock_direction;       // dock direction (top, bottom, left, right, center)
    int dock_layer;           // layer number (0 = innermost layer)
    int dock_row;             // row number on the docking bar (0 = first row)
    int size;                 // size of the dock
    int min_size;             // minimum size of a dock (0 if there is no min)
    bool resizable;           // flag indicating whether the dock is resizable
    bool toolbar;             // flag indicating dock contains only toolbars
    bool fixed;               // flag indicating that the dock operates on
                              // absolute coordinates as opposed to proportional
};


class wxDockUIPart
{
public:
    enum
    {
        typeCaption,
        typeGripper,
        typeDock,
        typeDockSizer,
        typePane,
        typePaneSizer,
        typeBackground,
        typePaneBorder,
        typePaneButton,
    };

    int type;                // ui part type (see enum above)
    int orientation;         // orientation (either wxHORIZONTAL or wxVERTICAL)
    wxDockInfo* dock;        // which dock the item is associated with
    wxPaneInfo* pane;        // which pane the item is associated with
    wxPaneButton* button;    // which pane button the item is associated with
    wxSizer* cont_sizer;     // the part's containing sizer
    wxSizerItem* sizer_item; // the sizer item of the part
    wxRect rect;             // client coord rectangle of the part itself
};


class wxPaneButton
{
public:
    int button_id;        // id of the button (e.g. buttonClose)
};




// wx event machinery


// right now the only event that works is wxEVT_AUI_PANEBUTTON. A full
// spectrum of events will be implemented in the next incremental version

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_AUI_PANEBUTTON, 0)   
END_DECLARE_EVENT_TYPES()

typedef void (wxEvtHandler::*wxFrameManagerEventFunction)(wxFrameManagerEvent&);

#define wxFrameManagerEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxFrameManagerEventFunction, &func)

#define EVT_AUI_PANEBUTTON(func) \
   wx__DECLARE_EVT0(wxEVT_AUI_PANEBUTTON, wxFrameManagerEventHandler(func))


#endif
