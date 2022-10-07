/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2007-03-14
 *
 */


#include "colorselector.h"
#include "popupcontainer.h"
#include "colorpanel.h"


// helper functions

static void getColorRectangles(int cli_height,
                               wxRect* fgcolor_rect,
                               wxRect* bgcolor_rect)
{
    int x1, y1, h;
    x1 = 0;
    y1 = 0;
    h = cli_height;
    h *= 2;
    h /= 3;
    
    //set the foreground rectangle area
    *fgcolor_rect = wxRect(x1, y1, h, h);

    //set the background rectangle area
    *bgcolor_rect = wxRect(x1+(h/2), y1+(h/2), h, h);
}


namespace kcl
{


// -- ColorSelector class implementation --

BEGIN_EVENT_TABLE(ColorSelector, wxControl)
    EVT_PAINT(ColorSelector::onPaint)
    EVT_SIZE(ColorSelector::onSize)
    EVT_MOVE(ColorSelector::onMove)
    EVT_LEFT_DOWN(ColorSelector::onMouseButtonDown)
    EVT_LEFT_DCLICK(ColorSelector::onMouseButtonDown)
    EVT_RIGHT_DOWN(ColorSelector::onMouseButtonDown)
    EVT_RIGHT_DCLICK(ColorSelector::onMouseButtonDown)
    EVT_LEFT_UP(ColorSelector::onMouseButtonUp)
    EVT_RIGHT_UP(ColorSelector::onMouseButtonUp)
    EVT_ERASE_BACKGROUND(ColorSelector::onEraseBackground)
END_EVENT_TABLE()


IMPLEMENT_CLASS(ColorSelector, wxControl)


ColorSelector::ColorSelector(wxWindow* parent,
                             wxWindowID id,
                             const wxColor& fg_color,
                             const wxColor& bg_color,
                             const wxPoint& position,
                             const wxSize& size) :
                   wxControl(parent,
                             id,
                             position,
                             (size == wxDefaultSize) ? wxSize(42,42)
                                                     : size,
                             wxNO_BORDER)
{
    SetBackgroundColour(parent->GetBackgroundColour());
    
    GetClientSize(&m_cli_width, &m_cli_height);
    m_minWidth = m_cli_width;
    m_minHeight = m_cli_height;

    m_fgcolor = fg_color;
    m_bgcolor = bg_color;

    m_mouse_down = wxPoint(-1,-1);
    
    // resize our color rectangles
    getColorRectangles(m_cli_height, &m_fgcolor_rect, &m_bgcolor_rect);
}

void ColorSelector::setForegroundColor(const wxColor& color)
{
    m_fgcolor = color;
}

void ColorSelector::setBackgroundColor(const wxColor& color)
{
    m_bgcolor = color;
}

wxColor ColorSelector::getForegroundColor()
{
    return m_fgcolor;
}

wxColor ColorSelector::getBackgroundColor()
{
    return m_bgcolor;
}

void ColorSelector::onPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    
    wxColor c = GetBackgroundColour();
    dc.SetPen(c);
    dc.SetBrush(c);
    dc.DrawRectangle(0, 0, m_cli_width, m_cli_height);
    
    wxBrush bgcolor_brush;
    
    if (m_bgcolor == wxNullColour)
        bgcolor_brush = wxBrush(*wxWHITE, wxBRUSHSTYLE_SOLID);
     else
        bgcolor_brush = wxBrush(m_bgcolor);
    
    wxRect bgcolor_rect = m_bgcolor_rect;
    wxRect fgcolor_rect = m_fgcolor_rect;
    
    // draw background color box
    dc.SetPen(*wxBLACK);
    dc.SetBrush(*wxBLACK);
    dc.DrawRectangle(bgcolor_rect);
    dc.SetPen(*wxWHITE);
    dc.SetBrush(bgcolor_brush);
    bgcolor_rect.Deflate(1,1);
    dc.DrawRectangle(bgcolor_rect);

    // draw red line if this is a null color
    if (m_bgcolor == wxNullColour)
    {
        int x1, y1, x2, y2;
        x1 = bgcolor_rect.GetLeft();
        y1 = bgcolor_rect.GetBottom();
        x2 = bgcolor_rect.GetRight()+1;
        y2 = bgcolor_rect.GetTop()-1;
        dc.SetPen(wxColor(204,0,0));
        dc.DrawLine(x1, y1, x2, y2);
        dc.DrawLine(x1, y1-1, x2-1, y2);
        dc.DrawLine(x1+1, y1, x2, y2+1);
    }

    // draw foreground color box
    dc.SetPen(*wxBLACK);
    dc.SetBrush(*wxBLACK);
    dc.DrawRectangle(fgcolor_rect);
    dc.SetPen(*wxWHITE);
    dc.SetBrush(m_fgcolor);
    fgcolor_rect.Deflate(1,1);
    dc.DrawRectangle(fgcolor_rect);
}

void ColorSelector::onSize(wxSizeEvent& evt)
{
    GetClientSize(&m_cli_width, &m_cli_height);
    
    // resize our color rectangles
    getColorRectangles(m_cli_height, &m_fgcolor_rect, &m_bgcolor_rect);
    
    Refresh(true);
}

void ColorSelector::onMove(wxMoveEvent& evt)
{
    Refresh(true);
}

void ColorSelector::onMouseButtonDown(wxMouseEvent& evt)
{
    m_mouse_down = wxPoint(evt.GetX(),evt.GetY());
}

void ColorSelector::onMouseButtonUp(wxMouseEvent& evt)
{
    // right button was clicked up but the left mouse button is still down
    if (evt.LeftIsDown())
        return;
        
    // left button was clicked up but the right mouse button is still down
    if (evt.RightIsDown())
        return;
        
    wxPoint mouse_down = m_mouse_down;
    wxPoint mouse_up = evt.GetPosition();
    m_mouse_down = wxPoint(-1,-1);

    int colorpanel_mode = ColorPanel::ModeNone;
    
    if (m_fgcolor_rect.Contains(mouse_down) &&
        m_fgcolor_rect.Contains(mouse_up))
    {
        // foreground color selected
        colorpanel_mode = ColorPanel::ModeText;
    }
     else if (m_bgcolor_rect.Contains(mouse_down) &&
              m_bgcolor_rect.Contains(mouse_up))
    {
        // background color selected
        colorpanel_mode = ColorPanel::ModeFill;
    }
     else
    {
        // bail out, mouse down and up were not in the same color rectangle
        return;
    }

    m_color_panel_cont = new PopupContainer(::wxGetMousePosition());
    m_color_panel = new ColorPanel(m_color_panel_cont);
    m_color_panel->setMode(colorpanel_mode);
    m_color_panel->sigDestructing.connect(this,
                                  &ColorSelector::onColorPanelDestructing);
    m_color_panel->sigColorSelected.connect(this,
                                  &ColorSelector::onColorSelected);
    m_color_panel->sigCustomColorsRequested.connect(this,
                                  &ColorSelector::onCustomColorsRequested);
    m_color_panel->sigCustomColorsChanged.connect(this,
                                  &ColorSelector::onCustomColorsChanged);
    m_color_panel_cont->doPopup(m_color_panel);
}

void ColorSelector::onEraseBackground(wxEraseEvent& evt)
{

}


void ColorSelector::onColorPanelDestructing(kcl::ColorPanel* panel)
{
    m_color_panel = NULL;
}

void ColorSelector::onCustomColorsRequested(std::vector<wxColor>& colors)
{
    sigCustomColorsRequested(colors);
}

void ColorSelector::onCustomColorsChanged(std::vector<wxColor> colors)
{
    sigCustomColorsChanged(colors);
}

void ColorSelector::onColorSelected(wxColour color, int mode)
{
    if (mode == ColorPanel::ModeText)
        m_fgcolor = color;
        
    if (mode == ColorPanel::ModeFill)
        m_bgcolor = color;

    m_color_panel_cont->Show(false);
    m_color_panel_cont->Destroy();

    Refresh(false);
}




};  // namespace kcl




