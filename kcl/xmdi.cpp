/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2003-04-03
 *
 */


#include <wx/wx.h>
#include "xmdi.h"


#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif


namespace kcl
{


/* XPM */
static const char* gxmdi_xpm_close[] = {
"16 16 2 1",
"  c None",
"! c #000000",
"                ",
"                ",
"                ",
"    !!    !!    ",
"     !!  !!     ",
"      !!!!      ",
"       !!       ",
"      !!!!      ",
"     !!  !!     ",
"    !!    !!    ",
"                ",
"                ",
"                ",
"                ",
"                ",
"                "};



/* XPM */
static const char* gxmdi_xpm_maximize1[] = {
"16 16 3 1",
"  c None",
"! c #000000",
"# c #000000",
"                ",
"                ",
"   #########    ",
"   #########    ",
"   #       #    ",
"   #       #    ",
"   #       #    ",
"   #       #    ",
"   #       #    ",
"   #       #    ",
"   #########    ",
"                ",
"                ",
"                ",
"                ",
"                "};

/* XPM */
static const char* gxmdi_xpm_maximize2[] = {
"16 16 3 1",
"  c None",
"! c #000000",
"# c #000000",
"                ",
"                ",
"     ######     ",
"     ######     ",
"     #    #     ",
"   ###### #     ",
"   ###### #     ",
"   #    ###     ",
"   #    #       ",
"   #    #       ",
"   ######       ",
"                ",
"                ",
"                ",
"                ",
"                "};

static void ExposeWindow(wxWindow* _win)
{
    _win->Refresh(FALSE);

    const wxWindowList& children = _win->GetChildren();
    wxWindowList::compatibility_iterator node;

    for (node = children.GetFirst(); node; node = node->GetNext())
    {
        wxWindow *win = node->GetData();
        if (win->IsShown())
           ExposeWindow(win);
    }
}


static void GxMdiDrawButton(wxDC* dc,
                            int x,
                            int y,
                            int width,
                            int height,
                            wxBitmap& bitmap,
                            bool depressed)
{
    if (!depressed)
    {
        dc->SetBrush(wxBrush(wxColor(196,196,196), wxSOLID));
        dc->SetPen(*wxWHITE_PEN);
        dc->DrawRectangle(x, y, width, height);
        dc->SetPen(*wxBLACK_PEN);
        dc->DrawLine(x+width-1, y, x+width-1, y+height);
        dc->DrawLine(x, y+height-1, x+width-1, y+height-1);
        dc->SetPen(*wxGREY_PEN);
        dc->DrawLine(x+width-2, y+1, x+width-2, y+height-1);
        dc->DrawLine(x+1, y+height-2, x+width-2, y+height-2);
        dc->DrawBitmap(bitmap, x, y, TRUE);
    }
     else
    {
        dc->SetBrush(wxBrush(wxColor(196,196,196), wxSOLID));
        dc->SetPen(*wxWHITE_PEN);
        dc->DrawRectangle(x, y, width, height);
        dc->SetPen(*wxBLACK_PEN);
        dc->DrawLine(x, y, x+width-1, y);
        dc->DrawLine(x, y, x, y+height-1);
        //dc->SetPen(*wxGREY_PEN);
        //dc->DrawLine(x+width-2, y+1, x+width-2, y+height-1);
        //dc->DrawLine(x+1, y+height-2, x+width-2, y+height-2);
        dc->DrawBitmap(bitmap, x+1, y+1, TRUE);
    }
}




struct wxGxMDIButtonInfo
{
    wxBitmap bitmap;  // bitmap button
    int id;           // button id
    int offset;       // offset from right
    bool depressed;   // button state
};



static wxColour getActiveCaptionColor()
{
    static wxColour result;
    
    result = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

#ifdef __WXGTK__
    // -- darken the color a bit on GTK --
    int r = result.Red();
    int g = result.Green();
    int b = result.Blue();
    r = (r*17)/20;
    g = (g*17)/20;
    b = (b*17)/20;
    result.Set(r,g,b);
#endif

    return result;
}


class WXDLLEXPORT wxGxMDIChildContainer : public wxWindow
{
public:

    enum
    {
        ID_CursorTimer = 8001
    };

public:

    wxGxMDIChildContainer(wxGxMDIClientWindow* parent,
                          wxWindowID id,
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize)
                  : wxWindow(parent,
                             -1,
                             pos,
                             size,
                             wxNO_FULL_REPAINT_ON_RESIZE |
                             wxCLIP_CHILDREN |
                             wxCLIP_SIBLINGS)
    {
        m_title = wxT("");
        m_client = parent;
        m_child = NULL;
        m_caption_height = 20;
        m_border_width = 4;
        m_action = actionNone;
        m_action_button = NULL;
        m_resize_edge = 0;
        m_caption_font = wxFont(8, wxSWISS, wxNORMAL, wxBOLD);
        m_maximized = false;

#ifdef WIN32
        m_active_brush = getActiveCaptionColor();
        m_border_brush = wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE), wxSOLID);
        m_inactive_brush = wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTION), wxSOLID);
#else
        m_active_brush = getActiveCaptionColor();
        m_border_brush = wxBrush(wxColor(196,196,196), wxSOLID);
        m_inactive_brush = wxBrush(wxColor(196,196,196), wxSOLID);
#endif

        m_buttons[0].bitmap = wxBitmap(gxmdi_xpm_close);
        m_buttons[0].id = btnidClose;
        m_buttons[0].offset = 24;
        m_buttons[0].depressed = false;

        m_buttons[1].bitmap = wxBitmap(gxmdi_xpm_maximize1);
        m_buttons[1].id = btnidMaximize;
        m_buttons[1].offset = 42;
        m_buttons[1].depressed = false;
        
        GetClientSize(&m_cliwidth, &m_cliheight);
    }

    ~wxGxMDIChildContainer()
    {
        m_client->OnChildDestroy(this);
    }

    void SetChild(wxWindow* child)
    {
        m_child = child;
    }

    wxWindow* GetChild()
    {
        return m_child;
    }

    wxString GetTitle()
    {
        return m_title;
    }

    void SetTitle(const wxString& title)
    {
        //wxWindow::SetTitle(title);
        m_title = title;
    }

    void OnPaint(wxPaintEvent& event)
    {
        wxPaintDC dc(this);

        dc.SetBrush(m_border_brush);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, m_cliwidth, m_cliheight);

        dc.SetBrush(m_active ? m_active_brush : m_inactive_brush);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(m_border_width,
                         m_border_width,
                         m_cliwidth-(m_border_width*2),
                         m_caption_height);


        dc.SetPen(*wxWHITE_PEN);
        dc.DrawLine(0, 0, m_cliwidth, 0);
        dc.DrawLine(0, 0, 0, m_cliheight);
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawLine(m_cliwidth-1, 0, m_cliwidth-1, m_cliheight-1);
        dc.DrawLine(0, m_cliheight-1, m_cliwidth, m_cliheight-1);

        if (m_icon.Ok())
        {
            dc.DrawIcon(m_icon,
                     m_border_width+2,
                     m_border_width+((m_caption_height-m_icon.GetHeight())/2));
        }

        dc.SetFont(m_caption_font);

#ifdef WIN32
        dc.SetTextForeground(m_active ?
                wxSystemSettings::GetColour(wxSYS_COLOUR_CAPTIONTEXT) :
                wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTIONTEXT));
#else
        dc.SetTextForeground(m_active ? wxColor(255,255,255) : wxColor(128,128,128));
#endif

        wxString title = GetTitle();

        wxCoord tx, ty;
        dc.GetTextExtent(title, &tx, &ty);

        dc.DrawText(title,
                    m_border_width+20,
                    m_border_width+((m_caption_height-ty)/2));
                    
        // -- draw buttons on the right --

        int i;
        for (i = 0; i < sizeof(m_buttons)/sizeof(wxGxMDIButtonInfo); ++i)
        {
            DrawButton(&dc, &m_buttons[i]);
        }        
    }

    void OnSize(wxSizeEvent& event)
    {
        GetClientSize(&m_cliwidth, &m_cliheight);

        m_child->SetSize(m_border_width,
                         m_border_width+m_caption_height,
                         m_cliwidth-(m_border_width*2),
                         m_cliheight-m_caption_height-(m_border_width*2),
                         0x0fffffff);
    }

    void OnEraseBackground(wxEraseEvent& event)
    {
        // -- do nothing --
    }

    void OnMouse(wxMouseEvent& event)
    {
        int event_type = event.GetEventType();

        if (event_type == wxEVT_LEFT_DOWN)
        {
            m_client->ActivateChild(this);

            wxCoord x = event.GetX();
            wxCoord y = event.GetY();

            // -- test if a button was clicked --
            wxGxMDIButtonInfo* button = ButtonHitTest(x, y);
            if (button)
            {
                m_action = actionButtonClick;
                m_action_button = button;
                m_action_button->depressed = true;
                wxClientDC dc(this);
                DrawButton(&dc, m_action_button);
                CaptureMouse();
                return;
            }

            int edge_hit = EdgeHitTest(x, y);
            
            if (edge_hit)
            {
                m_action = actionResize;
                m_resize_edge = edge_hit;
                
                int w, h;
                GetPosition(&m_orig_x1, &m_orig_y1);
                GetClientSize(&w, &h);
                m_orig_x2 = m_orig_x1 + w;
                m_orig_y2 = m_orig_y1 + h;
                
                m_action_x = x;
                m_action_y = y;
                CaptureMouse();
            }
             else
            {
                m_action = actionMove;
                m_action_x = x;
                m_action_y = y;
                CaptureMouse();
            }
        }
         else if (event_type == wxEVT_LEFT_UP)
        {
            wxCoord x = event.GetX();
            wxCoord y = event.GetY();
            
            if (GetCapture() == this)
            {
                ReleaseMouse();
            }
            
            if (m_action == actionButtonClick)
            {
                m_action_button->depressed = false;
                
                wxClientDC dc(this);
                DrawButton(&dc, m_action_button);
                
                // -- if the button is still depressed on mouse
                //    button up, dispatch a command event --
                if (ButtonHitTest(x, y) == m_action_button)
                {
                    OnButtonClicked(m_action_button);
                }

                m_action_button = NULL;
            }
            
            m_action = actionNone;
        }
         else if (event_type == wxEVT_LEFT_DCLICK)
        {
            wxCoord x = event.GetX();
            wxCoord y = event.GetY();

            if (x < 20 && y < m_caption_height+m_border_width)
                m_child->Close();
        }
         else if (event_type == wxEVT_MOTION)
        {
            wxCoord x = event.GetX();
            wxCoord y = event.GetY();

            if (m_action == actionMove)
            {
                int parent_x, parent_y;

                parent_x = x;
                parent_y = y;
 
                ClientToScreen(&parent_x, &parent_y);
                GetParent()->ScreenToClient(&parent_x, &parent_y);

                if (parent_x < 5)
                    parent_x = 5;
                if (parent_y < 5)
                    parent_y = 5;

                Move(parent_x - m_action_x,
                     parent_y - m_action_y);
            }
             else if (m_action == actionResize)
            {   
                int orig_width = m_orig_x2 - m_orig_x1;
                int orig_height = m_orig_y2 - m_orig_y1;
                
                int parent_x, parent_y;
                parent_x = x;
                parent_y = y;
                ClientToScreen(&parent_x, &parent_y);
                GetParent()->ScreenToClient(&parent_x, &parent_y);
                
                int new_x, new_y, new_width, new_height;
                
                switch (m_resize_edge)
                {
                    case edgeBottomRight:
                        new_width = x;
                        new_height = y;
                        CheckDimBounds(NULL, NULL, &new_width, &new_height);
                        SetSize(-1, -1, new_width, new_height);
                        break;
                        
                    case edgeTopLeft:
                        new_x = parent_x;
                        new_y = parent_y;
                        new_height = m_orig_y2 - parent_y;
                        new_width = m_orig_x2 - parent_x;
                        CheckDimBounds(&new_x, &new_y, &new_width, &new_height);
                        SetSize(new_x, new_y, new_width, new_height);
                        break;
                    
                    case edgeBottom:
                        new_height = y;
                        CheckDimBounds(NULL, NULL, &new_width, &new_height);
                        SetSize(-1, -1, orig_width, new_height);
                        break;
                        
                    case edgeTop:
                        new_height = m_orig_y2 - parent_y;
                        new_y = parent_y;
                        CheckDimBounds(NULL, &new_y, NULL, &new_height);
                        SetSize(m_orig_x1, new_y, orig_width, new_height);
                        break;
                    
                    case edgeBottomLeft:
                        new_height = parent_y - m_orig_y1;
                        new_width  = m_orig_x2 - parent_x;
                        new_x = parent_x;
                        CheckDimBounds(&new_x, NULL, &new_width, &new_height);
                        SetSize(new_x, m_orig_y1, new_width, new_height);
                        break;
                          
                    case edgeTopRight:
                        new_height = m_orig_y2 - parent_y;
                        new_width = parent_x - m_orig_x1;
                        new_y = parent_y;
                        CheckDimBounds(NULL, &new_y, &new_width, &new_height);
                        SetSize(m_orig_x1, new_y, new_width, new_height);
                        break;
                    
                    case edgeLeft:
                        new_width = m_orig_x2 - parent_x;
                        new_x = parent_x;
                        CheckDimBounds(&new_x, NULL, &new_width, NULL);
                        SetSize(new_x, m_orig_y1, new_width, orig_height);
                        break;
                        
                    case edgeRight:
                        new_width = x;
                        CheckDimBounds(NULL, NULL, &new_width, NULL);
                        SetSize(-1, -1, new_width, orig_height);
                        break;
                }
                
                Refresh(FALSE);
            }
             else if (m_action == actionButtonClick)
            {
                // -- see if the button is still depressed --
                wxGxMDIButtonInfo* button = ButtonHitTest(x, y);
                
                bool last_state = m_action_button->depressed;
                m_action_button->depressed = (button == m_action_button) ?
                                                  true : false;
                      
                // -- if the button pressed state changed, redraw it --                            
                if (last_state != m_action_button->depressed)
                {
                    wxClientDC dc(this);
                    DrawButton(&dc, m_action_button);
                }
            }
             else if (m_action == actionNone)
            {
                int edge_hit = EdgeHitTest(x, y);
                wxStockCursor cursor = wxCURSOR_NONE;
                
                if (edge_hit)
                { 
                    switch (edge_hit)
                    {
                        case edgeBottomRight:                       
                        case edgeTopLeft:
                            cursor = wxCURSOR_SIZENWSE;
                            break;
                        
                        case edgeBottom:
                        case edgeTop:
                            cursor = wxCURSOR_SIZENS;
                            break;
                        
                        case edgeBottomLeft:    
                        case edgeTopRight:
                            cursor = wxCURSOR_SIZENESW;
                            break;
                        
                        case edgeLeft:
                        case edgeRight:
                            cursor = wxCURSOR_SIZEWE;
                            break;
                    }
                }

                if (cursor)
                {
                    SetCursor(cursor);
                    m_cursor_timer.SetOwner(this, ID_CursorTimer);
                    m_cursor_timer.Start(100);
                }
                 else
                {
                    SetCursor(wxNullCursor);
                }

            }
        }
    }
    
    void SetActive(bool active)
    {
        m_active = active;

        if (active)
        {
            Raise();
            
            //#ifdef __WXGTK__
            //gdk_window_raise(m_wxwindow->window);
            //#endif
        }

        Refresh(FALSE);
    }

    bool Destroy()
    {
        Show(false);

        if (!wxPendingDelete.Member(this))
            wxPendingDelete.Append(this);

        return true;
    }

    void SetIcon(const wxIcon& icon)
    {
        m_icon = icon;
        Refresh(FALSE);
    }

    void Maximize(bool maximize)
    {
        if (maximize)
        {
            if (!m_maximized)
            {
                GetPosition(&m_saved_x, &m_saved_y);
                GetSize(&m_saved_w, &m_saved_h);
            }
            
            int w, h;
            m_client->GetClientSize(&w, &h);
            SetSize(0, 0, w, h);
            m_maximized = true;
        }
         else
        {
            if (!m_maximized)
                return;
                
            SetSize(m_saved_x, m_saved_y, m_saved_w, m_saved_h);
            m_maximized = false;
        }
        
        // -- update any maximize buttons with maximize state info --
        int i;
        for (i = 0; i < sizeof(m_buttons)/sizeof(wxGxMDIButtonInfo); ++i)
        {
            if (m_buttons[i].id == btnidMaximize)
            {
                m_buttons[i].bitmap = m_maximized ?
                            wxBitmap(gxmdi_xpm_maximize2) :
                            wxBitmap(gxmdi_xpm_maximize1);
                Refresh(false);
            }
        }
    }
    
    bool IsMaximized() const
    {
        return m_maximized;
    }
    
private:

    void DrawButton(wxDC* dc, wxGxMDIButtonInfo* button)
    {
        GxMdiDrawButton(dc,
                    m_cliwidth-button->offset,
                    m_border_width+((m_caption_height-14)/2),
                    16, 14,
                    button->bitmap,
                    button->depressed);
    }
    
    wxGxMDIButtonInfo* ButtonHitTest(int x, int y)
    {
        int i;
        for (i = 0; i < sizeof(m_buttons)/sizeof(wxGxMDIButtonInfo); ++i)
        {
            wxGxMDIButtonInfo* button = &m_buttons[i];
            
            if (x > m_cliwidth-button->offset &&
                x <= m_cliwidth-button->offset + 16 &&
                y > m_border_width+((m_caption_height-14)/2) &&
                y <= m_border_width+((m_caption_height-14)/2)+14 )
            {
                return button;
            }
        }
        
        return NULL;
    }
    
    void OnButtonClicked(wxGxMDIButtonInfo* button)
    {
        if (button->id == btnidClose)
        {
            m_child->Close();
        }
         else if (button->id == btnidMaximize)
        {
            Maximize(!IsMaximized());
        }
    }
    
    int EdgeHitTest(int x, int y)
    {
        const int XMDI_EDGE_SIZE = 20;
        
        if (y < XMDI_EDGE_SIZE)
        {
            // -- top --
            if (x < XMDI_EDGE_SIZE)
            {
                if (x < m_border_width)
                    return edgeTopLeft;
                    
                if (y > m_border_width)
                    return 0;
                    
                return edgeTopLeft;
            }
             else if (x < m_cliwidth-XMDI_EDGE_SIZE)
            {
                if (y > m_border_width)
                    return 0;
                    
                return edgeTop;
            }
             else
            {
                if (y > m_border_width &&
                    x < m_cliwidth-m_border_width)
                {
                    return 0;
                }

                if (x >= m_cliwidth-XMDI_EDGE_SIZE)
                    return edgeTopRight;

                if (y > m_border_width)
                    return 0;
                    
                return edgeTopRight;
            }
        }
         else if (y < m_cliheight-XMDI_EDGE_SIZE)
        {
            // -- middle --
            if (x <= m_border_width)
            {
                return edgeLeft;
            }
             else if (x >= m_cliwidth-m_border_width)
            {
                return edgeRight;
            }
             else
            {
                return 0;
            }
        }
         else
        {
            // -- bottom --
            if (x < XMDI_EDGE_SIZE)
            {
                return edgeBottomLeft;
            }
             else if (x < m_cliwidth-XMDI_EDGE_SIZE)
            {
                return edgeBottom;
            }
             else
            {
                return edgeBottomRight;
            }
        }
        
        return 0;
    }
    
    void CheckDimBounds(int* x, int* y, int* w = NULL, int* h = NULL)
    {
        const int XMDI_MIN_WIDTH = 100;
        const int XMDI_MIN_HEIGHT = 28;
        
        if (x)
        {
            if (*x > m_orig_x2 - XMDI_MIN_WIDTH)
                *x = m_orig_x2 - XMDI_MIN_WIDTH;
        }
        
        if (y)
        {
            if (*y > m_orig_y2 - XMDI_MIN_HEIGHT)
                *y = m_orig_y2 - XMDI_MIN_HEIGHT;
        }
        
        if (w)
        {
            if (*w < XMDI_MIN_WIDTH)
                *w = XMDI_MIN_WIDTH;
        }
        
        if (h)
        {
            if (*h < XMDI_MIN_HEIGHT)
                *h = XMDI_MIN_HEIGHT;
        }
    }
    
    void OnCursorTimer(wxTimerEvent& event)
    {
        wxPoint pt = ::wxGetMousePosition();
        pt = ScreenToClient(pt);

        if (EdgeHitTest(pt.x, pt.y) == 0)
        {
            SetCursor(wxNullCursor);
            m_cursor_timer.Stop();
        }
    }

private:

    enum
    {
        actionNone = 0,
        actionMove = 1,
        actionResize = 2,
        actionButtonClick = 3
    };
    
    enum
    {
        btnidNone = 0,
        btnidClose = 1,
        btnidMaximize = 2
    };
    
    enum
    {
        edgeTopLeft = 1,
        edgeTop,
        edgeTopRight,
        edgeRight,
        edgeBottomRight,
        edgeBottom,
        edgeBottomLeft,
        edgeLeft
    };

    wxBrush m_border_brush;
    wxBrush m_active_brush;
    wxBrush m_inactive_brush;
    wxFont m_caption_font;
    wxIcon m_icon;
    wxString m_title;
    wxTimer m_cursor_timer;
    wxGxMDIButtonInfo m_buttons[2];
    wxGxMDIClientWindow* m_client;
    wxWindow* m_child;
    int m_caption_height;
    int m_border_width;
    bool m_maximized;
    int m_saved_x;
    int m_saved_y;
    int m_saved_w;
    int m_saved_h;
    
    bool m_active;
    int m_action;
    wxCoord m_action_x;
    wxCoord m_action_y;
    int m_orig_x1;
    int m_orig_y1;
    int m_orig_x2;
    int m_orig_y2;
    
    wxGxMDIButtonInfo* m_action_button;
    int m_resize_edge;  // clockwise: 1 = top-left, 2=top, 3=top-right, etc.
    wxCoord m_cliwidth;
    wxCoord m_cliheight;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(wxGxMDIChildContainer, wxWindow)
    EVT_PAINT(wxGxMDIChildContainer::OnPaint)
    EVT_SIZE(wxGxMDIChildContainer::OnSize)
    EVT_MOUSE_EVENTS(wxGxMDIChildContainer::OnMouse)
    EVT_ERASE_BACKGROUND(wxGxMDIChildContainer::OnEraseBackground)
    EVT_TIMER(wxGxMDIChildContainer::ID_CursorTimer, wxGxMDIChildContainer::OnCursorTimer)
END_EVENT_TABLE()




const int wxID_GXMDI_CASCADE = 4002;
const int wxID_GXMDI_TILEHORIZONTAL = 4001;
const int wxID_GXMDI_TILEVERTICAL = 4005;
const int wxID_GXMDI_GONEXTCHILD = 4004;
const int wxID_GXMDI_GOPREVCHILD = 4006;




IMPLEMENT_DYNAMIC_CLASS(wxGxMDIParentFrame, wxFrame)

BEGIN_EVENT_TABLE(wxGxMDIParentFrame, wxFrame)
    //EVT_MENU(-1, wxGxMDIParentFrame::DoHandleMenu)
    EVT_MENU(wxID_GXMDI_TILEHORIZONTAL, wxGxMDIParentFrame::OnTileHorizontal)
    EVT_MENU(wxID_GXMDI_TILEVERTICAL, wxGxMDIParentFrame::OnTileVertical)
END_EVENT_TABLE()


wxGxMDIParentFrame::wxGxMDIParentFrame()
{
}

wxGxMDIParentFrame::wxGxMDIParentFrame(wxWindow *parent,
                                       wxWindowID id,
                                       const wxString& title,
                                       const wxPoint& pos,
                                       const wxSize& size,
                                       long style,
                                       const wxString& name) :
                          wxFrame(parent, id, title, pos, size, style, name)
{
    m_mdi_client = new wxGxMDIClientWindow(this, wxSUNKEN_BORDER);
}

wxGxMDIParentFrame::~wxGxMDIParentFrame()
{
}

bool wxGxMDIParentFrame::Create(wxWindow *parent,
                                wxWindowID id,
                                const wxString& title,
                                const wxPoint& pos,
                                const wxSize& size,
                                long style,
                                const wxString& name)
{
    if (!wxFrame::Create(parent, id, title, pos, size, style, name))
        return false;

    m_mdi_client = new wxGxMDIClientWindow(this,
                                           wxSUNKEN_BORDER |
                                           wxCLIP_CHILDREN |
                                           wxNO_FULL_REPAINT_ON_RESIZE);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_mdi_client, 1, wxEXPAND);
    SetSizer(sizer);

    return true;
}


void wxGxMDIParentFrame::OnTileHorizontal(wxCommandEvent& event)
{
    Tile(wxHORIZONTAL);
}

void wxGxMDIParentFrame::OnTileVertical(wxCommandEvent& event)
{
    Tile(wxVERTICAL);
}


void wxGxMDIParentFrame::SetChildMenuBar(wxGxMDIChildFrame *pChild)
{
}


wxGxMDIChildFrame* wxGxMDIParentFrame::GetActiveChild() const
{
    wxGxMDIChildContainer* container = m_mdi_client->GetActiveChild();
    if (!container)
    {
        return NULL;
    }

    return (wxGxMDIChildFrame*)container->GetChild();
}

void wxGxMDIParentFrame::SetActiveChild(wxGxMDIChildFrame* pChildFrame)
{
    m_mdi_client->ActivateChild(pChildFrame->GetContainer());
}


wxGxMDIClientWindow* wxGxMDIParentFrame::GetClientWindow() const
{
    return m_mdi_client;
}

void wxGxMDIParentFrame::Cascade()
{
    m_mdi_client->Cascade();
}

void wxGxMDIParentFrame::Tile(wxOrientation orient)
{
    m_mdi_client->Tile(orient);
}

void wxGxMDIParentFrame::ArrangeIcons()
{
}

void wxGxMDIParentFrame::ActivateNext()
{
    m_mdi_client->ActivateNext();
}

void wxGxMDIParentFrame::ActivatePrevious()
{
    m_mdi_client->ActivatePrevious();
}




IMPLEMENT_DYNAMIC_CLASS(wxGxMDIChildFrame, wxWindow)

BEGIN_EVENT_TABLE(wxGxMDIChildFrame, wxWindow)
    //EVT_MENU_HIGHLIGHT_ALL(wxGxMDIChildFrame::OnMenuHighlight)
    //EVT_ACTIVATE(wxGxMDIChildFrame::OnActivate)
    //EVT_CLOSE(wxGxMDIChildFrame::OnCloseWindow)
    //EVT_SIZE(wxGxMDIChildFrame::OnSize)
    EVT_CHILD_FOCUS(wxGxMDIChildFrame::OnChildFocus)
    EVT_SET_FOCUS(wxGxMDIChildFrame::OnSetFocus)
END_EVENT_TABLE()



wxGxMDIChildFrame::wxGxMDIChildFrame()
{
    m_container = NULL;
}

wxGxMDIChildFrame::wxGxMDIChildFrame(wxGxMDIParentFrame* parent,
                  wxWindowID id,
                  const wxString& title,
                  const wxPoint& pos,
                  const wxSize& size,
                  long style,
                  const wxString& name)
{
    m_container = NULL;
    Create(parent, id, title, pos, size);
}


wxGxMDIChildFrame::~wxGxMDIChildFrame()
{
}


bool wxGxMDIChildFrame::Create(wxGxMDIParentFrame* parent,
                               wxWindowID id,
                               const wxString& title,
                               const wxPoint& pos,
                               const wxSize& size,
                               long style,
                               const wxString& name)
{   
    m_container = parent->GetClientWindow()->CreateContainer();

    if (!wxWindow::Create(m_container,
                          id,
                          wxPoint(20,20),
                          wxSize(50,50)))
    {
        return false;
    }

    m_container->SetChild(this);
    m_container->Show(false);

    
    wxPoint final_pos = wxPoint(0,0);
    wxSize final_size = wxSize(450,300);

    if (pos != wxDefaultPosition)
        final_pos = pos;
    if (size != wxDefaultSize)
        final_size = size;

    m_container->SetSize(final_pos.x,
                         final_pos.y,
                         final_size.x,
                         final_size.y);

    m_container->Show(true);

    m_title = title;
    m_parent_frame = parent;

    Activate();

    return true;
}

void wxGxMDIChildFrame::SetTitle(const wxString& title)
{
    if (m_container)
    {
        m_container->SetTitle(title);
        m_container->Refresh(false);
    }

    m_title = title;
}

wxString wxGxMDIChildFrame::GetTitle() const
{
    return m_title;
}

void wxGxMDIChildFrame::Activate()
{
    m_parent_frame->GetClientWindow()->ActivateChild(GetContainer());
}

bool wxGxMDIChildFrame::Destroy()
{
    if (!m_container)
    {
        return wxWindow::Destroy();
    }

    m_container->Destroy();
    return true;
}


void wxGxMDIChildFrame::SetSizeHints(int WXUNUSED(minW),
                                     int WXUNUSED(minH),
                                     int WXUNUSED(maxW),
                                     int WXUNUSED(maxH),
                                     int WXUNUSED(incW),
                                     int WXUNUSED(incH))
{
}


void wxGxMDIChildFrame::SetIcon(const wxIcon &icon)
{
    m_container->SetIcon(icon);
}

void wxGxMDIChildFrame::SetIcons(const wxIconBundle &icons)
{
}


void wxGxMDIChildFrame::Maximize(bool maximize)
{
    m_container->Maximize(maximize);
}

void wxGxMDIChildFrame::Restore()
{
}

void wxGxMDIChildFrame::Iconize(bool iconize)
{
}

bool wxGxMDIChildFrame::IsMaximized() const
{
    return m_container->IsMaximized();
}

bool wxGxMDIChildFrame::IsIconized() const
{
    return FALSE;
}

bool wxGxMDIChildFrame::ShowFullScreen(bool show, long style)
{
    return FALSE;
}

bool wxGxMDIChildFrame::IsFullScreen() const
{
    return FALSE;
}

bool wxGxMDIChildFrame::IsTopLevel() const
{
    return FALSE;
}

void wxGxMDIChildFrame::SetMDIParentFrame(wxGxMDIParentFrame* parent_frame)
{
    m_parent_frame = parent_frame;
}

wxGxMDIParentFrame* wxGxMDIChildFrame::GetMDIParentFrame() const
{
    return m_parent_frame;
}


void wxGxMDIChildFrame::DoSetSize(int x, int y,
                                   int width, int height,
                                   int sizeFlags)
{
    if (sizeFlags == 0x0fffffff)
    {
        wxWindow::DoSetSize(x,y,width,height,sizeFlags);
    }
     else
    {
        m_container->SetSize(x, y, width, height, sizeFlags);
    }
}


void wxGxMDIChildFrame::OnSetFocus(wxFocusEvent& event)
{
    m_parent_frame->SetActiveChild(this);
}

void wxGxMDIChildFrame::OnChildFocus(wxChildFocusEvent& event)
{
    m_parent_frame->SetActiveChild(this);
}




IMPLEMENT_DYNAMIC_CLASS(wxGxMDIClientWindow, wxControl)

BEGIN_EVENT_TABLE(wxGxMDIClientWindow, wxControl)
END_EVENT_TABLE()


wxGxMDIClientWindow::wxGxMDIClientWindow()
{
    m_active_child = NULL;
}

wxGxMDIClientWindow::wxGxMDIClientWindow(wxGxMDIParentFrame *parent,
                                         long style)
                          : wxControl(parent,
                                      -1,
                                      wxDefaultPosition,
                                      wxDefaultSize,
                                      style)
{
    m_active_child = NULL;

#ifdef WIN32
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));
#else
    SetBackgroundColour(wxColor(128,128,128));
#endif

}

wxGxMDIClientWindow::~wxGxMDIClientWindow()
{
}

wxGxMDIChildContainer* wxGxMDIClientWindow::GetActiveChild() const
{
    return m_active_child;
}


void wxGxMDIClientWindow::ActivateChild(wxGxMDIChildContainer* child)
{
    if (m_active_child == child)
        return;

    if (m_active_child)
    {
        m_active_child->SetActive(false);
        
        wxActivateEvent deactivate_event(wxEVT_ACTIVATE,
                                         FALSE,
                                         m_active_child->GetId());
                                         
        deactivate_event.SetEventObject(m_active_child->GetChild());
        m_active_child->GetChild()->GetEventHandler()->ProcessEvent(deactivate_event);
    }

    m_active_child = child;

    if (m_active_child)
    {
        m_active_child->SetActive(true);
        wxActivateEvent activate_event(wxEVT_ACTIVATE, TRUE, child->GetId());
        activate_event.SetEventObject(child->GetChild());
        child->GetChild()->GetEventHandler()->ProcessEvent(activate_event);
    }
}

wxGxMDIChildContainer* wxGxMDIClientWindow::CreateContainer()
{
    return new wxGxMDIChildContainer(this, -1, wxPoint(4000,4000), wxSize(10,10));
}

void wxGxMDIClientWindow::OnChildDestroy(wxGxMDIChildContainer* child)
{
    const wxWindowList& children = GetChildren();
    
    wxWindowList::compatibility_iterator node = children.Find(child);
    wxCHECK_RET(node, wxT("could not locate MDI child"));

    wxWindowList::compatibility_iterator next_child_node = node;
    
    while (1)
    {
        next_child_node = next_child_node->GetPrevious();
        if (!next_child_node)
            break;
        if (((wxWindow*)next_child_node->GetData())->IsShown())
            break;
    }

    if (!next_child_node)
    {
        next_child_node = node;

        while (1)
        {
            next_child_node = next_child_node->GetNext();
            if (!next_child_node)
                break;
            if (((wxWindow*)next_child_node->GetData())->IsShown())
                break;
        }
    }

    wxGxMDIChildContainer* next_active_child = NULL;

    if (next_child_node)
    {
        next_active_child = (wxGxMDIChildContainer*)next_child_node->GetData();
        wxCHECK_RET(next_active_child, wxT("could not locate next MDI child"));
    }

    ActivateChild(next_active_child);
}


void wxGxMDIClientWindow::Tile(wxOrientation orient) const
{
    if (orient == wxHORIZONTAL)
    {
        wxWindowList children = GetChildren();
        wxWindowList::compatibility_iterator node;

        // -- put the active child first --
        if (children.DeleteObject(GetActiveChild()))
        {
            children.Insert(GetActiveChild());
        }

        // -- count visible children --
        int child_count = 0;

        for (node = children.GetFirst(); node; node = node->GetNext())
        {
            wxWindow *win = node->GetData();
            if (win->IsShown())
                ++child_count;
        }

        if (child_count == 0)
            return;

        // -- partition client area --
        int cli_width, cli_height;
        GetClientSize(&cli_width, &cli_height);

        int win_yoff = 0;
        int win_height = cli_height/child_count;

        for (node = children.GetFirst(); node; node = node->GetNext())
        {
            wxWindow *win = node->GetData();
            if (win->IsShown())
            {
                win->SetSize(0, win_yoff, cli_width, win_height);
                win_yoff += win_height;
            }
        }
    }
     else
    {
        wxWindowList children = GetChildren();
        wxWindowList::compatibility_iterator node;

        // -- put the active child first --
        if (children.DeleteObject(GetActiveChild()))
        {
            children.Insert(GetActiveChild());
        }
        
        // -- count visible children --
        int child_count = 0;

        for (node = children.GetFirst(); node; node = node->GetNext())
        {
            wxWindow *win = node->GetData();
            if (win->IsShown())
                ++child_count;
        }

        if (child_count == 0)
            return;

        // -- partition client area --
        int cli_width, cli_height;
        GetClientSize(&cli_width, &cli_height);

        int win_xoff = 0;
        int win_width = cli_width/child_count;

        for (node = children.GetFirst(); node; node = node->GetNext())
        {
            wxWindow *win = node->GetData();
            if (win->IsShown())
            {
                win->SetSize(win_xoff, 0, win_width, cli_height);
                win_xoff += win_width;
            }
        }
    }
}

void wxGxMDIClientWindow::Cascade() const
{
    wxWindowList children = GetChildren();
    wxWindowList::compatibility_iterator node;

    // -- put the active child last --
    if (children.DeleteObject(GetActiveChild()))
    {
        children.Append(GetActiveChild());
    }
    
    // -- count visible children --
    int child_count = 0;

    for (node = children.GetFirst(); node; node = node->GetNext())
    {
        wxWindow *win = node->GetData();
        if (win->IsShown())
            ++child_count;
    }

    if (child_count == 0)
        return;

    // -- partition client area --
    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    int win_xoff = 0;
    int win_yoff = 0;
    int win_width = cli_width*2/3;
    int win_height = cli_height*2/3;
    
    for (node = children.GetFirst(); node; node = node->GetNext())
    {
        wxWindow *win = node->GetData();
        if (win->IsShown())
        {
            win->SetSize(win_xoff, win_yoff, win_width, win_height);
            win_xoff += 22;
            win_yoff += 24;
        }
    }
    
}

void wxGxMDIClientWindow::ActivateNext()
{
    wxWindow* active = GetActiveChild();
    if (!active)
        return;
        
    wxWindowList children = GetChildren();
    
    int count = children.GetCount();
    if (count == 0)
        return;
        
    int idx = children.IndexOf(active);
    if (idx == wxNOT_FOUND)
        return;
    
    idx++;
    if (idx >= count)
        idx = 0;
        
    ActivateChild((wxGxMDIChildContainer*)children.Item(idx)->GetData());
}


void wxGxMDIClientWindow::ActivatePrevious()
{
    wxWindow* active = GetActiveChild();
    if (!active)
        return;
        
    wxWindowList children = GetChildren();
    
    int count = children.GetCount();
    if (count == 0)
        return;
        
    int idx = children.IndexOf(active);
    if (idx == wxNOT_FOUND)
        return;
    
    idx--;
    if (idx < 0)
        idx = count-1;
        
    ActivateChild((wxGxMDIChildContainer*)children.Item(idx)->GetData());
}


};
