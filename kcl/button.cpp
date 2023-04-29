/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2002-02-19
 *
 */


#include "button.h"
#include "util.h"
#include <wx/image.h>
#include <kl/klib.h>


#ifdef __WXGTK__
#include <gtk/gtk.h>
#if !wxCHECK_VERSION(2,9,0)
#include "wx/gtk/win_gtk.h"
#endif
#endif


namespace kcl
{


#ifndef WIN32
static bool isThemed()
{
    return false;
}

void getXpMenuColors(wxColor& dark, wxColor& light)
{
    unsigned int dc = 0x00C56A31;
    unsigned int lc = 0x00EED2C1;

    dark.Set((dc) & 0xff,
             (dc >> 8) & 0xff,
             (dc >> 16) & 0xff);

    light.Set((lc) & 0xff,
              (lc >> 8) & 0xff,
              (lc >> 16) & 0xff);
}
#endif




#ifdef WIN32

#if wxCHECK_VERSION(2,9,0)
#include "wx/msw/dc.h"
#endif

#ifndef _UXTHEME_H_

typedef void* HTHEME;
typedef BOOL (WINAPI *IsAppThemedFunc)();
typedef HTHEME (WINAPI *OpenThemeDataFunc)(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT (WINAPI *DrawThemeBackgroundFunc)(HTHEME hTheme,
                                                  HDC hdc,
                                                  int iPartId,
                                                  int iStateId,
                                                  const RECT *pRect,
                                                  const RECT *pClipRect);
#endif


static bool isThemed()
{
    static IsAppThemedFunc FbIsAppThemed = NULL;
    static bool first_time = true;

    if (first_time)
    {
        HMODULE hUxTheme = LoadLibrary(_T("uxtheme.dll"));
        if (hUxTheme)
        {
            FbIsAppThemed = (IsAppThemedFunc)GetProcAddress(hUxTheme, "IsAppThemed");
        }

        first_time = false;
    }

    if (!FbIsAppThemed)
        return false;

    return FbIsAppThemed() ? true : false;
}


static bool DrawPlatformButton(wxWindow* win,
                               wxDC& dc,
                               const wxRect& _rect,
                               const wxPen& bk_pen,
                               bool enabled,
                               bool mouse_over,
                               bool pushed)
{
    RECT rect;
    rect.left = _rect.x;
    rect.top = _rect.y;
    rect.right = _rect.width;
    rect.bottom = _rect.height;

    static HTHEME hTheme = NULL;
    static OpenThemeDataFunc FbOpenThemeData = NULL;
    static DrawThemeBackgroundFunc FbDrawThemeBackground = NULL;
    static bool first_time = true;


    if (first_time)
    {
        HMODULE hUxTheme = LoadLibrary(_T("uxtheme.dll"));
        if (hUxTheme)
        {
            FbOpenThemeData = (OpenThemeDataFunc)GetProcAddress(hUxTheme, "OpenThemeData");
            FbDrawThemeBackground = (DrawThemeBackgroundFunc)GetProcAddress(hUxTheme, "DrawThemeBackground");
        }

        first_time = false;
    }


    if (FbOpenThemeData && FbDrawThemeBackground)
    {
        if (!hTheme)
        {
            hTheme = FbOpenThemeData(NULL, L"Button");
        }

        if (hTheme)
        {
            int state;

            if (pushed && mouse_over)
            {
                state = 4;
            }
             else if (!enabled)
            {
                state = 3;
            }
             else if (mouse_over)
            {
                state = 2;
            }
             else
            {
                state = 0;
            }


            RECT r;
            CopyRect(&r, &rect);
            InflateRect(&r, 1, 1);

            dc.SetPen(bk_pen);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);

            dc.DrawRectangle(0,
                             0,
                             rect.right - rect.left,
                             rect.bottom - rect.top);

            dc.DrawRectangle(1,
                             1,
                             rect.right - rect.left - 2,
                             rect.bottom - rect.top - 2);

            FbDrawThemeBackground(hTheme, (HDC)getHdcFrom(dc), 1, state, &r, 0);

            return true;
        }
    }
    

    DrawFrameControl((HDC)getHdcFrom(dc),
                     &rect,
                     DFC_BUTTON,
                     DFCS_BUTTONPUSH |
                     ((pushed && mouse_over) ? DFCS_PUSHED : 0) |
                     (enabled ? 0 : DFCS_INACTIVE) |
                     0x0800 // (DFCS_TRANSPARENT)
                    );

    return true;
}

void getXpMenuColors(wxColor& dark, wxColor& light)
{
    unsigned int dc;
    unsigned int lc;

    dc = GetSysColor(COLOR_HIGHLIGHT);
    lc = 0;

    if (dc == 0x00C56A31)
    {
        lc = 0x00EED2C1;
    }
     else if (dc == 0x00BFB4B2)
    {
        lc = 0x00CAC7C7;
    }
     else if (dc == 0x0070A093)
    {
        lc = 0x00C3D1CE;
    }
     else
    {
        dc = 0x00C56A31;
        lc = 0x00EED2C1;
    }

    dark.Set((dc) & 0xff,
             (dc >> 8) & 0xff,
             (dc >> 16) & 0xff);

    light.Set((lc) & 0xff,
              (lc >> 8) & 0xff,
              (lc >> 16) & 0xff);
}

#endif

#ifdef __WXGTK__
static bool DrawPlatformButton(wxWindow* win,
                               wxDC& dc,
                               wxRect& rect,
                               wxPen& bk_pen,
                               bool enabled,
                               bool mouse_over,
                               bool pushed)
{
    static GtkWidget *s_button = NULL;
    static GtkWidget *s_window = NULL;

    if (s_button == NULL)
    {
        s_window = gtk_window_new(GTK_WINDOW_POPUP);
        gtk_widget_realize(s_window);

        s_button = gtk_button_new();
        gtk_container_add(GTK_CONTAINER(s_window), s_button);

        gtk_widget_realize(s_button);
    }

    GtkStateType state = GTK_STATE_NORMAL;

    if (enabled)
    {
        if (pushed)
        {
            state = GTK_STATE_ACTIVE;
        }
         else
        {
            if (mouse_over)
            {
                state = GTK_STATE_PRELIGHT;
            }
        }
    }
     else
    {
        state = GTK_STATE_INSENSITIVE;
    }


    GdkWindow* gdk_window;

    #if wxCHECK_VERSION(2,9,0)
    gdk_window = win->GTKGetDrawingWindow();
    #else
    gdk_window = GTK_PIZZA(win->m_wxwindow)->bin_window;
    #endif

    gtk_paint_box
    (
        s_button->style,
        gdk_window,
        state,
        GTK_SHADOW_OUT,
        NULL,
        s_button,
        "button",
        rect.x, rect.y, rect.width, rect.height
//        dc.XLOG2DEV(rect.x) -1, rect.y -1, rect.width +2, rect.height +2
    );

    return true;
}
#endif


#ifdef __WXMAC__
static bool DrawPlatformButton(wxWindow* win,
                               wxDC& dc,
                               wxRect& rect,
                               wxPen& bk_pen,
                               bool enabled,
                               bool mouse_over,
                               bool pushed)
{
}
#endif


BEGIN_EVENT_TABLE(Button, wxControl)
    EVT_PAINT(Button::onPaint)
    EVT_SIZE(Button::onSize)
    EVT_MOVE(Button::onMove)
    EVT_LEFT_DOWN(Button::onMouseButtonDown)
    EVT_LEFT_DCLICK(Button::onMouseButtonDown)
    EVT_LEFT_UP(Button::onMouseButtonUp)
    EVT_RIGHT_DOWN(Button::onMouseButtonDown)
    EVT_RIGHT_DCLICK(Button::onMouseButtonDown)
    EVT_RIGHT_UP(Button::onMouseButtonUp)
    EVT_ENTER_WINDOW(Button::onMouseEnterWindow)
    EVT_LEAVE_WINDOW(Button::onMouseLeaveWindow)
    EVT_MOTION(Button::onMouseMotion)
    EVT_ERASE_BACKGROUND(Button::onEraseBackground)
END_EVENT_TABLE()


IMPLEMENT_CLASS(kcl::Button, wxControl)


static const char* xpm_dropdown_arrow[] = {
"5 3 2 1",
"  c None",
"! c #000000",
"!!!!!",
" !!! ",
"  !  "};

const int dropdown_width = 11;


static int flatbutton_default_mode = -1;


int Button::getDefaultMode()
{
    return flatbutton_default_mode;
}

void Button::setDefaultMode(int default_mode)
{
    flatbutton_default_mode = default_mode;
}



Button::Button(wxWindow* parent,
               wxWindowID id,
               const wxPoint& pos,
               const wxSize& size,
               const wxString& text,
               const wxBitmap& bitmap,
               const wxBitmap& disabled_bitmap) :
                    wxControl(parent,
                              id,
                              pos,
                              size == wxDefaultSize ? wxSize(80,24) : size,
                              wxNO_BORDER)
{
    if (getDefaultMode() == -1)
    {
        if (isThemed())
        {
            setDefaultMode(modeXP);
        }
         else
        {
            setDefaultMode(modeFlat);
        }
    }

    m_mouse_over = false;
    m_button_down = false;
    m_dropdown_down = false;
    m_pushed_in = false;
    m_text_visible = true;
    m_bitmap_visible = false;
    m_enabled = true;
    m_toggle_mode = false;
    m_right_click_allowed = false;
    m_drop_down = false;
    m_indent = true;
    m_mode = Button::getDefaultMode();
    m_alignment = Button::alignCenter;
    m_bitmap_location = Button::bitmapLeft;
    m_text_width = 0;
    m_text_height = 0;
    m_spacer = 3;
    m_x_off = 3;
    m_tooltip = NULL;
    m_font = *wxNORMAL_FONT;
    m_darkgrey_pen = *wxThePenList->FindOrCreatePen(wxColour(64,64,64), 1, wxSOLID);
    m_grey_pen = *wxThePenList->FindOrCreatePen(wxColour(128,128,128), 1, wxSOLID);
    m_downarrow_bitmap = wxBitmap(xpm_dropdown_arrow);
    wxColor bk_color = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    m_bk_brush = *wxTheBrushList->FindOrCreateBrush(bk_color, wxSOLID);
    m_bk_pen = *wxThePenList->FindOrCreatePen(bk_color, 1, wxSOLID);

    if (isThemed())
    {
        wxColor dark, light;

        getXpMenuColors(dark, light);

        m_hover_pen = *wxThePenList->FindOrCreatePen(dark, 1, wxSOLID);
        m_hover_brush = *wxTheBrushList->FindOrCreateBrush(light, wxSOLID);
    }
     else
    {
        m_hover_pen = *wxThePenList->FindOrCreatePen(wxColor(10,36,106), 1, wxSOLID);
        m_hover_brush = *wxTheBrushList->FindOrCreateBrush(wxColor(182,189,210), wxSOLID);
    }

    m_sel_pen = m_hover_pen;
    m_sel_brush = m_bk_brush;

    m_text = wxT("");

    if (text.Length() > 0)
    {
        setText(text);
    }
     else
    {
        m_text_visible = false;
    }

    if (bitmap.Ok())
    {
        setBitmap(bitmap);
    }

    if (size == wxDefaultSize)
    {
        if (text.Length() == 0 && !bitmap.IsOk())
        {
            SetSize(80, 14);
        }
         else
        {
            makeMinSize();
        }
        
        wxSize s = GetClientSize();
        m_minWidth = s.x;
        m_minHeight = s.y;
    }
     else
    {
        m_minWidth = size.x;
        m_minHeight = size.y;
    }

    m_disabled_bitmap = disabled_bitmap;
}

bool Button::SetBackgroundColour(const wxColour& color)
{
    m_bk_brush = wxBrush(color, wxSOLID);
    Refresh(true);
    return true;
}

wxBitmap Button::getDisabledBitmap()
{
    if (m_disabled_bitmap.Ok())
        return m_disabled_bitmap;

    if (!m_bitmap.Ok())
        return wxNullBitmap;

    wxImage image = m_bitmap.ConvertToImage();

    int mr, mg, mb;
    
    mr = image.GetMaskRed();
    mg = image.GetMaskGreen();
    mb = image.GetMaskBlue();

    unsigned char* data = image.GetData();
    int width = image.GetWidth();
    int height = image.GetHeight();
    bool has_mask = image.HasMask();

    for (int y = height-1; y >= 0; --y)
    {
        for (int x = width-1; x >= 0; --x)
        {
            data = image.GetData() + (y*(width*3))+(x*3);
            unsigned char* r = data;
            unsigned char* g = data+1;
            unsigned char* b = data+2;

            if (has_mask && *r == mr && *g == mg && *b == mb)
                continue;

            int tot = 0;
            tot += *r;
            tot += *g;
            tot += *b;

            if (tot <= 255)
            {
                *r = 128;
                *g = 128;
                *b = 128;

                if (x < width-1 && y < height-1)
                {
                    data = image.GetData() + ((y+1)*(width*3))+((x+1)*3);
                    r = data;
                    g = data+1;
                    b = data+2;
                    if (has_mask && *r == mr && *g == mg && *b == mb)
                    {
                        *r = 255;
                        *g = 255;
                        *b = 255;
                    }
                }

            }
             else
            {
                *r = mr;
                *g = mg;
                *b = mb;
            }
        }
    }

    m_disabled_bitmap = wxBitmap(image);
    return m_disabled_bitmap;
}


wxBitmap Button::getShadowBitmap()
{
    if (m_shadow_bitmap.Ok())
        return m_shadow_bitmap;

    if (!m_bitmap.Ok())
        return wxNullBitmap;

    wxImage image = m_bitmap.ConvertToImage();

    int mr, mg, mb;
    
    mr = image.GetMaskRed();
    mg = image.GetMaskGreen();
    mb = image.GetMaskBlue();

    unsigned char* data = image.GetData();
    int width = image.GetWidth();
    int height = image.GetHeight();
    bool has_mask = image.HasMask();

    for (int y = height-1; y >= 0; --y)
    {
        for (int x = width-1; x >= 0; --x)
        {
            data = image.GetData() + (y*(width*3))+(x*3);
            unsigned char* r = data;
            unsigned char* g = data+1;
            unsigned char* b = data+2;

            if (has_mask && *r == mr && *g == mg && *b == mb)
                continue;

            *r = 128;
            *g = 128;
            *b = 128;
        }
    }

    m_shadow_bitmap = wxBitmap(image);
    return m_shadow_bitmap;
}

wxBitmap Button::getDropDownBitmap()
{
    return m_downarrow_bitmap;
}

void Button::setFont(const wxFont& font)
{
    m_font = font;

    wxClientDC dc(this);
    dc.SetFont(m_font);
    dc.GetTextExtent(m_text, &m_text_width, &m_text_height);

    Refresh(true);
}

void Button::setText(const wxString& text)
{
    m_text = text;

    SetLabel(text);

    if (text.Length() == 0)
    {
        m_text_visible = false;
    }
     else
    {
        m_text_visible = true;
    }

    Refresh(true);
}

void Button::setBitmap(const wxBitmap& bitmap)
{
    m_bitmap = bitmap;
    m_disabled_bitmap = wxNullBitmap;
    m_shadow_bitmap = wxNullBitmap;
    m_bitmap_visible = m_bitmap.Ok() ? true : false;
    Refresh(true);
}

wxBitmap Button::getBitmap()
{
    return m_bitmap;
}

wxString Button::getText()
{
    return m_text;
}

wxColor Button::getColor(int color_idx)
{
    switch (color_idx)
    {
        case colorNormalFg:     return m_bk_pen.GetColour();
        case colorNormalBg:     return m_bk_brush.GetColour();
        case colorHoverFg:      return m_hover_pen.GetColour();
        case colorHoverBg:      return m_hover_brush.GetColour();
        case colorSelectedFg:   return m_bk_pen.GetColour();
        case colorSelectedBg:   return m_sel_brush.GetColour();
    }

    return wxNullColour;
}

void Button::setTextVisible(bool state)
{
    m_text_visible = state;
    Refresh(true);
}

void Button::setBitmapVisible(bool state)
{
    m_bitmap_visible = state;
    Refresh(true);
}

void Button::setToggleMode(bool state)
{
    m_toggle_mode = state;
    Refresh(true);
}

void Button::setDepressed(bool state)
{
    m_pushed_in = state;
    Refresh(true);
}

void Button::setMode(int state)
{
    m_mode = state;
    Refresh(true);
}

void Button::setBitmapLocation(int location)
{
    m_bitmap_location = location;
    Refresh(true);
}

void Button::setAlignment(int align)
{
    m_alignment = align;
    Refresh(true);
}

void Button::setDropDown(bool state)
{
    m_drop_down = state;
    Refresh(true);
}

void Button::setRightClickAllowed(bool state)
{
    m_right_click_allowed = state;
}

void Button::setIndent(bool state)
{
    m_indent = state;
}

void Button::setColor(int color_idx, const wxColour& color)
{
    switch (color_idx)
    {
        case colorNormalFg:
            m_bk_pen = *wxThePenList->FindOrCreatePen(color, 1, wxSOLID);
            break;

        case colorNormalBg:
            m_bk_brush = *wxTheBrushList->FindOrCreateBrush(color, wxSOLID);
            break;

        case colorHoverFg:
            m_hover_pen = *wxThePenList->FindOrCreatePen(color, 1, wxSOLID);
            break;

        case colorHoverBg:
            m_hover_brush = *wxTheBrushList->FindOrCreateBrush(color, wxSOLID);
            break;

        case colorSelectedFg:
            m_bk_pen = *wxThePenList->FindOrCreatePen(color, 1, wxSOLID);
            break;

        case colorSelectedBg:
            m_sel_brush = *wxTheBrushList->FindOrCreateBrush(color, wxSOLID);
            break;
    }

    Refresh(true);
}


void Button::drawButtonFrame(wxDC& dc,
                             int x,
                             int y,
                             int width,
                             int height,
                             bool pushed_in,
                             bool button_down,
                             bool mouse_over)
{
    int x1, y1, x2, y2;
    x1 = x;
    x2 = x+width;
    y1 = y;
    y2 = y+height;

    // handle the drawing of the button when the user interacts with it
    if (m_mode == modeXP && mouse_over)
    {
        dc.SetPen(m_hover_pen);
        dc.SetBrush(m_hover_brush);

        dc.DrawRectangle(x, y, width, height);
    }
     else if ((m_mode == modeFlat && mouse_over) ||
               m_mode == mode3d)
    {
        dc.SetPen(*wxWHITE_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawLine(x1, y1, x1, y2-1);
        dc.DrawLine(x1, y1, x2-1, y1);
        dc.SetPen(m_darkgrey_pen);
        dc.DrawLine(x1, y2-1, x2, y2-1);
        dc.DrawLine(x2-1, y1, x2-1, y2-1);
    }
     else if (m_mode == modeCompatible)
    {
        dc.SetPen(*wxWHITE_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawLine(x1, y1, x1, y2-1);
        dc.DrawLine(x1, y1, x2-1, y1);
        dc.SetPen(m_darkgrey_pen);
        dc.DrawLine(x1, y2-1, x2, y2-1);
        dc.DrawLine(x2-1, y1, x2-1, y2-1);
        dc.SetPen(m_grey_pen);
        dc.DrawLine(x1+1, y2-2, x2-1, y2-2);
        dc.DrawLine(x2-2, y1+1, x2-2, y2-2);
    }


    if (button_down && mouse_over)
    {
        if (m_mode == modeCompatible)
        {
            dc.SetPen(*wxBLACK_PEN);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(x1, y1, width, height);
            dc.SetPen(m_grey_pen);
            dc.DrawRectangle(x1+1, x1+1, width-2, height-2);
        }
         else if (m_mode == modeFlat || m_mode == mode3d)
        {
            dc.SetPen(m_darkgrey_pen);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawLine(x1, y1, x1, y2-1);
            dc.DrawLine(x1, y1, x2-1, y1);
            dc.SetPen(*wxWHITE_PEN);
            dc.DrawLine(x1, y2-1, x2, y2-1);
            dc.DrawLine(x2-1, y1, x2-1, y2-1);
        }
    }

    if (pushed_in)
    {
        if (m_mode == modeXP)
        {
            dc.SetPen(m_sel_pen);
            dc.SetBrush(m_sel_brush);
            dc.DrawRectangle(x1, y1, width, height);
        }
         else
        {
            // draw a stipple background for the button
            int w, h, stipple_xoff;
            bool pix_offset = false;
            dc.SetPen(*wxWHITE_PEN);
            for (h = 1; h < height; h++)
            {
                stipple_xoff = pix_offset ? 1 : 2;
                for (w = 0; w < width; w++)
                {
                    dc.DrawPoint(stipple_xoff, h);
                    stipple_xoff += 2;
                }
                pix_offset = !pix_offset;
            }

            dc.SetPen(*wxGREY_PEN);
            dc.DrawLine(x1, y1, x1, y2-1);
            dc.DrawLine(x1, y1, x2-1, y1);
            dc.SetPen(*wxWHITE_PEN);
            dc.DrawLine(x1, y2-1, x2, y2-1);
            dc.DrawLine(x2-1, y1, x2-1, y2-1);
        }
    }
}


void Button::onEraseBackground(wxEraseEvent& evt)
{
    // do nothing
}


void Button::onPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);

    int render_flags = 0;
    if (IsEnabled())
        render_flags |= buttonEnabled;
    if (m_button_down || m_pushed_in)
        render_flags |= buttonDepressed;
    
    sigCustomRender(this, &dc, render_flags);
    if (sigCustomRender.isActive())
        return;
    
    // make sure we have the most current font
    wxFont font = GetFont();
    if (font != m_font)
    {
        setFont(font);
    }


    dc.SetFont(m_font);

    wxSize size = GetClientSize();
    int cli_height = size.GetHeight();
    int cli_width = size.GetWidth();


    bool draw_frame = true;

    if (m_mode == modeCompatible)
    {
        wxRect rect;
        rect.x = 0;
        rect.y = 0;
        rect.width = cli_width;
        rect.height = cli_height;

        if (DrawPlatformButton(this,
                               dc,
                               rect,
                               m_bk_pen,
                               m_enabled,
                               m_mouse_over,
                               m_button_down))
        {
            draw_frame = false;
        }
    }


    int button_width = cli_width;
    int button_height = cli_height;

    if (m_drop_down)
    {
        button_width -= dropdown_width;
    }


    int bitmap_yoff = 0;
    int text_yoff = 0;
    int bitmap_xoff = 0;
    int text_xoff = 0;
    int spacer = 0;

    // get the size of the bitmap if there is one
    int bitmap_width = 0;
    int bitmap_height = 0;

    if (m_bitmap_visible)
    {
        bitmap_width = m_bitmap.GetWidth();
        bitmap_height = m_bitmap.GetHeight();
    }

    if (m_text_visible)
    {
        dc.GetTextExtent(m_text, &m_text_width, &m_text_height);
    }

    // add 3 pixels between the bitmap and text if both exist
    if (m_bitmap_visible && m_text_visible)
    {
        spacer = m_spacer;
    }

    // handle the overall alignment of the bitmap and/or text

    int temp_width = bitmap_width + spacer;
    int temp_height = bitmap_height;
    int xoff = 0;

    if (m_bitmap_location == bitmapTop || m_bitmap_location == bitmapBottom)
    {
        temp_width = 0;
        temp_height = 0;
    }

    switch (m_alignment)
    {
        case Button::alignCenter:
            xoff = ((button_width - (m_text_width + temp_width))/2) - 1;
            bitmap_yoff = (button_height - bitmap_height)/2 - 1;
            text_yoff = (button_height - m_text_height)/2 - 1;
            break;

        case Button::alignLeft:
            xoff = m_x_off;
            bitmap_yoff = (button_height - bitmap_height)/2 - 1;
            text_yoff = (button_height - m_text_height)/2 - 1;
            break;

        case Button::alignRight:
            xoff = button_width - (m_text_width + temp_width) - m_x_off;
            bitmap_yoff = (button_height - bitmap_height)/2 - 1;
            text_yoff = (button_height - m_text_height)/2 - 1;
            break;
    }

    // add one pixel to compensate for the shaded border on the button
    xoff += 1;
    bitmap_yoff += 1;
    text_yoff += 1;

    switch (m_bitmap_location)
    {
        case Button::bitmapLeft:
            bitmap_xoff = xoff;
            text_xoff = xoff + bitmap_width + spacer;
            break;

        case Button::bitmapRight:
            text_xoff = xoff;
            bitmap_xoff = text_xoff + m_text_width + spacer;
            break;

        case Button::bitmapTop:
            bitmap_yoff = m_x_off;
            bitmap_xoff = (button_width-bitmap_width)/2;
            text_xoff = xoff;
            text_yoff = m_x_off+bitmap_height+spacer;
            break;

        case Button::bitmapBottom:
            bitmap_yoff = m_text_height+spacer;
            bitmap_xoff = (button_width-bitmap_width)/2;
            text_xoff = xoff;
            text_yoff = 0;
            break;
    }


    if (draw_frame)
    {
        // clear out our window
        dc.SetBrush(m_bk_brush);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, cli_width, cli_height);

        drawButtonFrame(dc,
                        0,
                        0,
                        button_width,
                        button_height,
                        m_pushed_in,
                        m_button_down,
                        m_mouse_over);

        if (m_drop_down)
        {
            int xpos = button_width;
            if (m_mode == modeXP)
                xpos--;

            drawButtonFrame(dc,
                            xpos,
                            0,
                            cli_width-xpos,
                            button_height,
                            m_pushed_in,
                            m_dropdown_down,
                            m_mouse_over);
        }
    }

    if (m_indent)
    {
        if (m_mode == modeXP)
        {
            if (m_mouse_over && !m_pushed_in && !m_button_down)
            {
                bitmap_xoff--;
                bitmap_yoff--;
                text_xoff--;
                text_yoff--;
            }
        }
         else
        {
            if (m_button_down && m_mouse_over)
            {
                bitmap_xoff++;
                bitmap_yoff++;
                text_xoff++;
                text_yoff++;
            }
        }
    }

    if (m_mode == modeXP && m_mouse_over && !m_button_down && m_indent)
    {
        wxBitmap draw_bitmap = getShadowBitmap();

        if (draw_bitmap.Ok())
        {
            dc.DrawBitmap(draw_bitmap, bitmap_xoff+1, bitmap_yoff+1, true);
        }
    }

    if (m_bitmap_visible)
    {
        if (m_enabled)
        {
            dc.DrawBitmap(m_bitmap, bitmap_xoff, bitmap_yoff, true);
        }
         else
        {
            wxBitmap draw_bitmap = getDisabledBitmap();
            
            if (draw_bitmap.Ok())
            {
                dc.DrawBitmap(draw_bitmap, bitmap_xoff, bitmap_yoff, true);
            }
        }
    }

    if (m_text_visible)
    {
        dc.SetTextForeground(m_enabled ? GetForegroundColour() : wxColor(128,128,128));
        dc.DrawText(m_text, text_xoff, text_yoff);
    }

    if (m_drop_down)
    {
        int x = cli_width-dropdown_width+3;
        int y = ((button_height-5)/2)+1;

        if (m_dropdown_down)
        {
            x++;
            y++;
        }

        dc.DrawBitmap(m_downarrow_bitmap, x, y, true);
    }
}



void Button::setDimensions(int width, int height)
{
    int w = 14;
    int h = 6;

    // if you don't specify one of the parameters, just keep the flatbutton's
    // current size for that parameter

    if (width == -1 || height == -1)
    {
        if (width == -1 && height == -1)
        {
            return;
        }

        int temp = 0;

        if (width == -1)
        {
            makeMinSize();
            GetSize(&w, &temp);
            SetSize(0, 0, w, height);
        }

        if (height == -1)
        {
            makeMinSize();
            GetSize(&temp, &h);
            SetSize(0, 0, width, h);
        }
    }
     else
    {
        SetSize(0, 0, width, height);
    }

    Refresh(true);
}

void Button::makeMinSize()
{
    int w = 13;
    int h = 7;

    wxClientDC dc(this);
    dc.SetFont(m_font);
    dc.GetTextExtent(m_text, &m_text_width, &m_text_height);

    if (m_bitmap_visible)
    {
        w += m_bitmap.GetWidth();
    }

    if (m_text_visible)
    {
        w += m_text_width;
    }

    if (m_bitmap_visible && m_text_visible)
    {
        w += m_spacer;
        h += std::max(m_bitmap.GetHeight(), m_text_height);
    }
     else
    {
        if (m_bitmap_visible)
        {
            h += m_bitmap.GetHeight();
        }
         else
        {
            h += m_text_height;
        }
    }
    
    SetSize(w, h);

    // set wx's min size variables, which are used by sizers
    m_minWidth = w;
    m_minHeight = h;

    Refresh(true);
}

void Button::onSize(wxSizeEvent& evt)
{
    Refresh(true);
}


void Button::onMove(wxMoveEvent& evt)
{
    Refresh(true);
}

void Button::onMouseButtonDown(wxMouseEvent& evt)
{
    if ((evt.GetEventType() == wxEVT_RIGHT_DOWN ||
         evt.GetEventType() == wxEVT_RIGHT_DCLICK) &&
        !m_right_click_allowed)
    {
        return;
    }

    m_dropdown_down = false;
    m_button_down = false;

    if (!m_drop_down)
    {
        m_button_down = true;
        Refresh(true);
        CaptureMouse();
    }
     else
    {
        int cli_width;
        int cli_height;

        GetClientSize(&cli_width, &cli_height);

        if (evt.m_x >= cli_width-dropdown_width)
        {
            m_dropdown_down = true;
            Refresh(true);
            onDropDownClicked();
        }
         else
        {
            m_button_down = true;
            Refresh(true);
            CaptureMouse();
        }
    }
}

void Button::onMouseButtonUp(wxMouseEvent& evt)
{
    bool right_button = false;

    if (evt.GetEventType() == wxEVT_RIGHT_UP)
    {
        right_button = true;
    }
    
    if (right_button && !m_right_click_allowed)
    {
        return;
    }

    if (GetCapture() == this)
    {
        ReleaseMouse();
    }

    if (m_mouse_over && m_button_down)
    {
        if (m_toggle_mode)
        {
            m_pushed_in = !m_pushed_in;
        }

        m_button_down = false;
        m_dropdown_down = false;
        m_mouse_over = false;

        Refresh(true);
        Update();

        m_mouse_over = true;

        sigButtonClicked(this, right_button ? 3 : 1);
        onButtonClicked();
        SendClickEvent();
    }
     else
    {
        m_button_down = false;
        m_dropdown_down = false;

        Refresh(true);
    }
}

void Button::onMouseEnterWindow(wxMouseEvent& evt)
{
    m_mouse_over = true;
    if (!m_pushed_in || m_button_down || (m_mode == modeXP && m_toggle_mode))
    {
        Refresh(true);
    }
}

void Button::onMouseLeaveWindow(wxMouseEvent& evt)
{
    m_mouse_over = false;
    if (!m_pushed_in || m_button_down || (m_mode == modeXP && m_toggle_mode))
    {
        Refresh(true);
    }
}

void Button::onMouseMotion(wxMouseEvent& evt)
{
    if (m_button_down && !m_pushed_in)
    {
        int w, h;
        GetClientSize(&w, &h);
        if (evt.m_x >= 0 && evt.m_x < w && evt.m_y >= 0 && evt.m_y < h)
        {
            if (!m_mouse_over)
            {
                m_mouse_over = true;
                Refresh(true);
            }
        }
         else
        {
            if (m_mouse_over)
            {
                m_mouse_over = false;
                Refresh(true);
            }
        }
    }
}

bool Button::SendClickEvent()
{
    wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
    evt.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(evt);
}

int Button::getHeight()
{
    int w, h;
    GetSize(&w, &h);
    return h;
}

int Button::getWidth()
{
    int w, h;
    GetSize(&w, &h);
    return w;
}

bool Button::getDepressed()
{
    return m_pushed_in;
}

void Button::setTooltip(const wxString& tip)
{
    SetToolTip(tip);
}


bool Button::Enable(bool enable)
{
    if (m_enabled == enable)
        return true;

    m_enabled = enable;

    Refresh(true);
    
    return wxWindow::Enable(enable);
}

void Button::setSpacer(int spacer)
{
    m_spacer = spacer;
}

void Button::setXOffset(int x_offset)
{
    m_x_off = x_offset;
}


}; // namespace kcl


