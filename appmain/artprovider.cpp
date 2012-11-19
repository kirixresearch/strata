/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   David Z. Williams
 * Created:  2007-04-03
 *
 */

#include "appmain.h"
#include <wx/wx.h>
#include <wx/dc.h>
#include <wx/settings.h>
#include "artprovider.h"
#include "../kcl/util.h"


#if defined(__WXMSW__)
    #include <wx/msw/uxtheme.h>
#endif


const int PADDING = 4;
const int BOTTOMLINE_Y = 3;
const int UXTHEME_CLOSEBUTTON_SIZE = 13;



#if wxCHECK_VERSION(2,9,3)
    #define m_caption_font m_captionFont
    #define m_button_size m_buttonSize
    #define m_inactive_caption_gradient_colour m_inactiveCaptionGradientColour
    #define m_inactive_caption_text_colour m_inactiveCaptionTextColour
    #define m_button_size m_buttonSize
    #define m_base_colour m_baseColour
    #define m_border_pen m_borderPen
    #define m_base_colour_brush m_backgroundBrush
    #define m_disabled_close_bmp m_inactiveCloseBitmap
    #define m_active_close_bmp m_activeCloseBitmap
#endif

// these functions live in aui/dockart.cpp
wxColor wxAuiLightContrastColour(const wxColour& c);
wxString wxAuiChopText(wxDC& dc, const wxString& text, int max_size);




static unsigned char wxAuiBlendColour(unsigned char fg, unsigned char bg, double alpha)
{
    double result = bg + (alpha * (fg - bg));
    if (result < 0.0)
        result = 0.0;
    if (result > 255)
        result = 255;
    return (unsigned char)result;
}

// wxAuiStepColour() it a utility function that simply darkens
// or lightens a color, based on the specified percentage
// ialpha of 0 would be completely black, 100 completely white
// an ialpha of 100 returns the same colour
static wxColor wxAuiStepColour(const wxColor& c, int ialpha)
{
    if (ialpha == 100)
        return c;

    unsigned char r = c.Red(),
                  g = c.Green(),
                  b = c.Blue();
    unsigned char bg;

    // ialpha is 0..200 where 0 is completely black
    // and 200 is completely white and 100 is the same
    // convert that to normal alpha 0.0 - 1.0
    ialpha = wxMin(ialpha, 200);
    ialpha = wxMax(ialpha, 0);
    double alpha = ((double)(ialpha - 100.0))/100.0;

    if (ialpha > 100)
    {
        // blend with white
        bg = 255;
        alpha = 1.0 - alpha;  // 0 = transparent fg; 1 = opaque fg
    }
    else
    {
        // blend with black
        bg = 0;
        alpha += 1.0;         // 0 = transparent fg; 1 = opaque fg
    }

    r = wxAuiBlendColour(r, bg, alpha);
    g = wxAuiBlendColour(g, bg, alpha);
    b = wxAuiBlendColour(b, bg, alpha);

    return wxColour(r, g, b);
}






static void IndentPressedBitmap(wxRect* rect, int button_state)
{
    if (button_state == wxAUI_BUTTON_STATE_PRESSED)
    {
        rect->x++;
        rect->y++;
    }
}

inline bool IsUxThemed()
{
    #if defined(__WXMSW__)
    #if wxUSE_UXTHEME
        return true;
    #endif
    #endif

    return false;
}




bool DrawUxThemeCloseButton(wxDC& dc,
                            wxWindow* wnd,
                            const wxRect& rect,
                            int button_state)
{
#if defined(__WXMSW__)
#if wxUSE_UXTHEME

    // don't draw a close button for these states
    if (button_state == wxAUI_BUTTON_STATE_HIDDEN ||
        button_state == wxAUI_BUTTON_STATE_CHECKED)
        return true;

    if (wxUxThemeEngine::Get())
    {
        wxUxThemeHandle hTheme(wnd, L"WINDOW");
        if (hTheme)
        {
            int state = 1;
            switch (button_state)
            {
                case wxAUI_BUTTON_STATE_NORMAL:
                    state = 1; // CBS_NORMAL
                    break;
                case wxAUI_BUTTON_STATE_HOVER:
                    state = 2; // CBS_HOT
                    break;
                case wxAUI_BUTTON_STATE_PRESSED:
                    state = 3; // CBS_PUSHED
                    break;
                case wxAUI_BUTTON_STATE_DISABLED:
                    state = 4; //CBS_DISABLED
                    break;
                default:
                    wxASSERT_MSG(false, wxT("Unknown state"));
            }

            RECT rc;
            wxCopyRectToRECT(rect, rc);
            rc.top += (rect.height-13)/2;
            rc.right = rc.left+13;
            rc.bottom = rc.top+13;
            
            // draw the themed close button
            wxUxThemeEngine::Get()->DrawThemeBackground(
                                hTheme, 
                                (HDC)kcl::getHdcFrom(dc), 
                                19 /*WP_SMALLCLOSEBUTTON*/,
                                state, &rc, NULL);
            return true;
        }
    }
    
#endif  // wxUSE_UXTHEME
#endif  // defined(__WXMSW__)

    return false;
}




// CfwDockArt class implementation

void CfwDockArt::DrawPaneButton(wxDC& dc,
                                wxWindow* wnd,
                                int button,
                                int button_state,
                                const wxRect& rect,
                                wxAuiPaneInfo& pane)
{
    if (button == wxAUI_BUTTON_CLOSE)
    {
        if (DrawUxThemeCloseButton(dc, wnd, rect, button_state))
            return;
    }
    
    wxAuiDefaultDockArt::DrawPaneButton(dc, wnd, button, 
                                        button_state, rect, pane);
}

void CfwDockArt::DrawCaption(wxDC& dc,
                             wxWindow* window,
                             const wxString& text,
                             const wxRect& rect,
                             wxAuiPaneInfo& pane)
{
#ifdef WIN32

    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetFont(m_caption_font);

    wxColour c1, c2, text_color;
    
    if (pane.state & wxAuiPaneInfo::optionActive)
    {
        c1 = wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION);
        c1 = wxAuiStepColour(c1, 120);
        c2 = wxAuiLightContrastColour(c1);
        text_color = wxSystemSettings::GetColour(wxSYS_COLOUR_CAPTIONTEXT);
    }
     else
    {
        c1 = wxAuiStepColour(m_inactive_caption_gradient_colour, 97);
        c2 = wxAuiLightContrastColour(c1);
        text_color = m_inactive_caption_text_colour;
    }

    // draw background
    dc.GradientFillLinear(rect, c1, c2, wxNORTH);

    // set text color
    dc.SetTextForeground(text_color);

    wxCoord w,h;
    dc.GetTextExtent(wxT("ABCDEFHXfgkj"), &w, &h);

    wxRect clip_rect = rect;
    clip_rect.width -= 3; // text offset
    clip_rect.width -= 2; // button padding
    if (pane.HasCloseButton())
        clip_rect.width -= m_button_size;
    if (pane.HasPinButton())
        clip_rect.width -= m_button_size;
    if (pane.HasMaximizeButton())
        clip_rect.width -= m_button_size;

    wxString draw_text = wxAuiChopText(dc, text, clip_rect.width);

    dc.SetClippingRegion(clip_rect);
    dc.DrawText(draw_text, rect.x+3, rect.y+(rect.height/2)-(h/2)-1);
    dc.DestroyClippingRegion();

#else
    wxAuiDefaultDockArt::DrawCaption(dc, window, text, rect, pane);
#endif  // WIN32
}




// CfwTabArt class implementation


#if wxCHECK_VERSION(2,9,3)
    #undef m_base_colour_brush
    #undef m_disabled_close_bmp
    #undef m_active_close_bmp

    #define m_base_colour_brush m_baseColourBrush
    #define m_disabled_close_bmp m_disabledCloseBmp
    #define m_active_close_bmp m_activeCloseBmp
    #define m_disabled_left_bmp m_disabledLeftBmp
    #define m_active_left_bmp m_activeLeftBmp
    #define m_disabled_right_bmp m_disabledRightBmp
    #define m_active_right_bmp m_activeRightBmp
    #define m_active_windowlist_bmp m_activeWindowListBmp
    #define m_disabled_windowlist_bmp m_disabledWindowListBmp
    #define m_selected_font m_selectedFont
    #define m_normal_font m_normalFont
    #define m_fixed_tab_width m_fixedTabWidth
    #define m_tab_ctrl_height m_tabCtrlHeight
    #define m_measuring_font m_measuringFont
#endif

void CfwTabArt::DrawBackground(wxDC& dc,
                               wxWindow* wnd,
                               const wxRect& rect)
{
    int x = rect.x-PADDING;
    int w = rect.width+(PADDING*2);
    int y = rect.y;
    int h = rect.height-BOTTOMLINE_Y;
    
    wxColor top_color = wxAuiStepColour(m_base_colour, 95);
    wxColor bottom_color = wxAuiStepColour(m_base_colour, 150);
    
    // draw background
    wxRect r1(x, y, w, h*3/5);
    dc.GradientFillLinear(r1, top_color, bottom_color, wxSOUTH);
    y += (h*3/5);
    h -= (h*3/5);
    y--; h++;
    wxRect r2(x, y, w, h);
    dc.GradientFillLinear(r2, top_color, bottom_color, wxNORTH);

    // draw base lines
    y = rect.GetHeight();
    w = rect.GetWidth();
    dc.SetPen(m_border_pen);
    dc.SetBrush(m_base_colour_brush);
    dc.DrawRectangle(-1, y-4, w+2, 4);
}     

inline void drawInactiveTabBackground(wxDC& dc,
                                      const wxRect& rect,
                                      const wxColor& base_color,
                                      const wxPen& border_pen1,
                                      const wxPen& border_pen2)
{
    // border_pen1 is the "normal" border drawing pen (something like
    // a dark gray), and border_pen2 specifies a light off-white which
    // makes the border between inactive tabs only a partial border

    wxRect rect_shine = rect;
    rect_shine.height /= 2;
    rect_shine.height += 2;
            
    // draw the base background color
    dc.SetPen(base_color);
    dc.SetBrush(base_color);
    dc.DrawRectangle(rect);

    // draw the "shine" for the tab
    wxColor c1 = wxAuiStepColour(base_color, 125);
    wxColor c2 = wxAuiStepColour(base_color, 160);
    dc.GradientFillLinear(rect_shine, c1, c2, wxNORTH);

    // draw left and right borders
    dc.SetPen(border_pen2);
    dc.DrawLine(rect.x, rect.y,
                rect.x, rect.y+rect.height);
    dc.SetPen(border_pen1);
    dc.DrawLine(rect.x, rect.y+2,
                rect.x, rect.y+rect.height-2);
    dc.DrawLine(rect.x+rect.width, rect.y,
                rect.x+rect.width, rect.y+rect.height);

    // draw white chisel outline around side and bottom borders
    dc.SetPen(wxAuiStepColour(base_color, 160));
    dc.DrawLine(rect.x+1, rect.y,
                rect.x+1, rect.y+rect.height-1);
    dc.DrawLine(rect.x+rect.width-1, rect.y,
                rect.x+rect.width-1, rect.y+rect.height-1);
    dc.DrawLine(rect.x+1, rect.y+rect.height-1,
                rect.x+rect.width, rect.y+rect.height-1);
}

void CfwTabArt::DrawButton(wxDC& dc,
                           wxWindow* wnd,
                           const wxRect& in_rect,
                           int bitmap_id,
                           int button_state,
                           int orientation,
                           wxRect* out_rect)
{
    wxBitmap bmp;

    switch (bitmap_id)
    {
        case wxAUI_BUTTON_CLOSE:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_close_bmp;
                 else
                bmp = m_active_close_bmp;
            break;
        case wxAUI_BUTTON_LEFT:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_left_bmp;
                 else
                bmp = m_active_left_bmp;
            break;
        case wxAUI_BUTTON_RIGHT:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_right_bmp;
                 else
                bmp = m_active_right_bmp;
            break;
        case wxAUI_BUTTON_WINDOWLIST:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bmp = m_disabled_windowlist_bmp;
                 else
                bmp = m_active_windowlist_bmp;
            break;
    }

    if (!bmp.IsOk())
        return;

    int x, y, w, h, bmp_x, bmp_y, bmp_w, bmp_h;
    y = in_rect.y-1;
    w = bmp.GetWidth();
    h = in_rect.height - BOTTOMLINE_Y - 1;
    bmp_y = in_rect.y + (in_rect.height-bmp.GetHeight())/2 - BOTTOMLINE_Y + 1;
    bmp_w = bmp.GetWidth();
    bmp_h = bmp.GetHeight();

    if (orientation == wxLEFT)
    {
        // compensate for off-by-one's
        x = in_rect.x-1;
        bmp_x = in_rect.x+1;
    }
     else
    {
        x = in_rect.x + in_rect.width - bmp.GetWidth();
        bmp_x = x+1;
    }

    wxRect rect(x,y,w+1,h);
    wxRect bmp_rect(bmp_x,bmp_y,bmp_w,bmp_h);
    
    // draw the hover background
    if ((button_state == wxAUI_BUTTON_STATE_HOVER ||
         button_state == wxAUI_BUTTON_STATE_PRESSED) &&
        !(button_state & wxAUI_BUTTON_STATE_DISABLED))
    {
        drawInactiveTabBackground(dc, rect, m_base_colour,
                                      m_border_pen, m_border_pen);

        if (bitmap_id == wxAUI_BUTTON_RIGHT)
        {
            // draw white chisel outline to the left of the scroll right
            // button's border to separate it from the chopped-off tab
            dc.SetPen(wxAuiStepColour(m_base_colour, 160));
            dc.DrawLine(rect.x-1, rect.y, rect.x-1, rect.y+rect.height);
        }
    }

    // draw the button's bitmap
    IndentPressedBitmap(&bmp_rect, button_state);
    dc.DrawBitmap(bmp, bmp_rect.x, bmp_rect.y, true);

    *out_rect = wxRect(x,y,w,h);
}

void CfwTabArt::DrawTab(wxDC& dc,
                        wxWindow* wnd,
                        const wxAuiNotebookPage& page,
                        const wxRect& in_rect,
                        int close_button_state,
                        wxRect* out_tab_rect,
                        wxRect* out_button_rect,
                        int* x_extent)
{
    wxCoord normal_textx, normal_texty;
    wxCoord selected_textx, selected_texty;
    wxCoord textx, texty;

    // if the caption is empty, measure some temporary text
    wxString caption = page.caption;
    wxString text_extent_str = page.caption;
    if (text_extent_str.empty())
        text_extent_str = wxT("Xj");

    dc.SetFont(m_selected_font);
    dc.GetTextExtent(text_extent_str, &selected_textx, &selected_texty);

    dc.SetFont(m_normal_font);
    dc.GetTextExtent(text_extent_str, &normal_textx, &normal_texty);

    // figure out the size of the tab
    wxSize tab_size = GetTabSize(dc,
                                 wnd,
                                 page.caption,
                                 page.bitmap,
                                 page.active,
                                 close_button_state,
                                 x_extent);

    wxCoord tab_height = m_tab_ctrl_height - BOTTOMLINE_Y;
    wxCoord tab_width = tab_size.x;
    wxCoord tab_y = in_rect.y;
    wxCoord tab_x = in_rect.x;
    
    // select pen, brush and font for the tab to be drawn
    if (page.active)
    {
        dc.SetFont(m_selected_font);
        textx = selected_textx;
        texty = selected_texty;
    }
     else
    {
        dc.SetFont(m_normal_font);
        textx = normal_textx;
        texty = normal_texty;
    }

    // set tab drawing clipping region
    int clip_width = tab_width;
    if (tab_x + clip_width > in_rect.x + in_rect.width)
        clip_width = (in_rect.x + in_rect.width) - tab_x;
    dc.SetClippingRegion(tab_x, tab_y, clip_width+1, tab_height);

    if (page.active)
    {
        // draw active tab

        wxRect tab_rect(tab_x+1, tab_y, tab_width-1, tab_height);

        wxRect rect_shine = tab_rect;
        rect_shine.Deflate(2);
        rect_shine.y--;
        rect_shine.height /= 2;
        rect_shine.height += 2;
        
        // draw the gradient background
        wxColor c1 = m_base_colour;
        wxColor c2 = wxAuiStepColour(m_border_pen.GetColour(), 125);
        dc.GradientFillLinear(tab_rect, c1, c2, wxNORTH);
        
        // draw the "shine" for the tab
        c1 = m_base_colour;
        c2 = wxAuiStepColour(m_base_colour, 160);
        dc.GradientFillLinear(rect_shine, c1, c2, wxNORTH);
        
        // draw left and right borders
        dc.SetPen(wxAuiStepColour(m_border_pen.GetColour(), 90));
        dc.DrawLine(tab_x, tab_y,
                    tab_x, tab_y+tab_height);
        dc.DrawLine(tab_x+tab_width, tab_y,
                    tab_x+tab_width, tab_y+tab_height);
    }
     else
    {
        // draw inactive tab

        wxPen border_pen2(wxAuiStepColour(m_base_colour, 160));
        wxRect tab_rect(tab_x, tab_y, tab_width, tab_height-1);
        drawInactiveTabBackground(dc, tab_rect, m_base_colour,
                                  m_border_pen, border_pen2);
                                  
        // this code compensates for the scenario when the
        // scroll left/scroll right arrows are showing -- the left border
        // of the inactive tab wasn't drawn all the way to the bottom
        if (tab_x < m_active_left_bmp.GetWidth()+10)
        {
            dc.SetPen(m_border_pen);
            dc.DrawLine(tab_x, tab_y, tab_x, tab_y+tab_height);
        }
    }

    int close_button_width = 0;
    if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
    {
        if (IsUxThemed())
            close_button_width = UXTHEME_CLOSEBUTTON_SIZE;
         else
            close_button_width = m_active_close_bmp.GetWidth();
    }

    int bmp_offset = tab_x + (PADDING*2);
    int text_offset = tab_x + (PADDING*2);
    
    if (page.bitmap.IsOk())
    {
        int bmp_height = page.bitmap.GetHeight();
        int bmp_width = page.bitmap.GetWidth();
        
        // draw the tab's bitmap
        dc.DrawBitmap(page.bitmap,
                      bmp_offset,
                      tab_y + (tab_height-bmp_height)/2,
                      true);

        text_offset += bmp_width;
        text_offset += PADDING;     // bitmap padding
    }

    int max_text_size = tab_width;
    max_text_size -= (text_offset-tab_x);
    max_text_size -= close_button_width;
    max_text_size -= (PADDING+1);
    wxString draw_text = wxAuiChopText(dc, caption, max_text_size);

    // draw tab text
    dc.DrawText(draw_text,
                text_offset,
                tab_y + (tab_height-texty)/2);
    
    // draw close button if necessary
    if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
    {
        wxRect rect;
        rect.SetLeft(tab_x + tab_width - close_button_width - PADDING);
        rect.SetTop(tab_y + (tab_height-close_button_width)/2);
        rect.SetWidth(close_button_width);
        rect.SetHeight(close_button_width);

        if (!DrawUxThemeCloseButton(dc, wnd, rect, close_button_state))
        {
            // we couldn't draw the Windows XP mini close button,
            // so draw the default 'x'
            wxBitmap bmp = m_disabled_close_bmp;
            int bmp_height = m_disabled_close_bmp.GetHeight();
            int bmp_width = m_disabled_close_bmp.GetWidth();

            rect.SetLeft(tab_x + tab_width - close_button_width - PADDING);
            rect.SetTop(tab_y + (tab_height-bmp_height)/2);
            rect.SetWidth(close_button_width);
            rect.SetHeight(bmp_width);

            if (close_button_state == wxAUI_BUTTON_STATE_HOVER ||
                close_button_state == wxAUI_BUTTON_STATE_PRESSED)
            {
                bmp = m_active_close_bmp;
            }

            IndentPressedBitmap(&rect, close_button_state);
            dc.DrawBitmap(bmp, rect.x, rect.y, true);
        }
        
        *out_button_rect = rect;
    }

    *out_tab_rect = wxRect(tab_x, tab_y, tab_width, tab_height);

    dc.DestroyClippingRegion();
}

void CfwTabArt::SetSizingInfo(const wxSize& tab_ctrl_size,
                              size_t tab_count)
{
    m_fixed_tab_width = 100;

    int tot_width = (int)tab_ctrl_size.x - GetIndentSize() - 4;

    if (m_flags & wxAUI_NB_CLOSE_BUTTON)
        tot_width -= m_active_close_bmp.GetWidth();
    if (m_flags & wxAUI_NB_WINDOWLIST_BUTTON)
        tot_width -= m_active_windowlist_bmp.GetWidth();

    if (tab_count > 0)
    {
        m_fixed_tab_width = tot_width/(int)tab_count;
    }


    if (m_fixed_tab_width < 100)
        m_fixed_tab_width = 100;

    if (m_fixed_tab_width > tot_width/2)
        m_fixed_tab_width = tot_width/2;

    if (m_fixed_tab_width > 250)
        m_fixed_tab_width = 250;

    m_tab_ctrl_height = tab_ctrl_size.y;
}

wxSize CfwTabArt::GetTabSize(wxDC& dc,
                             wxWindow* WXUNUSED(wnd),
                             const wxString& caption,
                             const wxBitmap& bitmap,
                             bool WXUNUSED(active),
                             int close_button_state,
                             int* x_extent)
{
    wxCoord measured_textx, measured_texty, tmp;

    dc.SetFont(m_measuring_font);
    dc.GetTextExtent(caption, &measured_textx, &measured_texty);
    dc.GetTextExtent(wxT("ABCDEFXj"), &tmp, &measured_texty);

    wxCoord tab_width = measured_textx;
    wxCoord tab_height = measured_texty;

    // if the close button is showing, add space for it
    if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
        tab_width += m_active_close_bmp.GetWidth() + PADDING;

    // if there's a bitmap, add space for it
    if (bitmap.IsOk())
    {
        tab_width += bitmap.GetWidth();
        tab_width += PADDING; // right side bitmap padding
        tab_height = wxMax(tab_height, bitmap.GetHeight());
    }

    // add padding
    tab_width += 16;
    tab_height += 7;

    if (m_flags & wxAUI_NB_TAB_FIXED_WIDTH)
        tab_width = m_fixed_tab_width;

    *x_extent = tab_width;

    return wxSize(tab_width, tab_height);
}

int CfwTabArt::GetIndentSize()
{
    return -1;
}

wxAuiTabArt* CfwTabArt::Clone()
{
    return static_cast<wxAuiTabArt*>(new CfwTabArt);
}

