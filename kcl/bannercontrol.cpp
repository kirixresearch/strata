/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2002-08-01
 *
 */


#include "bannercontrol.h"
#include "util.h"


namespace kcl
{

// written at 3:30 a.m. ;-)


BEGIN_EVENT_TABLE(BannerControl, wxControl)
    EVT_PAINT(BannerControl::onPaint)
    EVT_SIZE(BannerControl::onSize)
END_EVENT_TABLE()


const int GRADIENT_START_POINT = 25;         // number between 0 and 100
const int GRADIENT_END_POINT = 100;         // number between 0 and 100
const int COLOR_STEP_COUNT = 75;


BannerControl::BannerControl(wxWindow* parent,
                         const wxString& text = wxEmptyString,
                         const wxPoint& position = wxDefaultPosition,
                         const wxSize& size = wxDefaultSize) :
                                 wxControl(parent,
                                           -1,
                                           position,
                                           (size==wxDefaultSize)?wxSize(100,40):size,
                                           wxNO_BORDER),
                                 m_text(text)

{
    m_start_color = kcl::stepColor(kcl::getBaseColor(), 170);
    m_end_color = kcl::stepColor(kcl::getBaseColor(), 90);
    m_text_color = kcl::getCaptionColor();
    m_border_color = kcl::getBorderColor();
    m_font = wxFont(11, wxSWISS, wxNORMAL, wxFONTWEIGHT_BOLD, false);

    m_border_sides = borderNone;
    m_gradient_direction = gradientVertical;
    m_alignment = alignLeft;
    m_padding = 10;

    wxSize s = GetSize();
    m_minWidth = 100;
    m_minHeight = s.y;
}

void BannerControl::setFont(const wxFont& font)
{
    m_font = font;
    Refresh(false);
}

void BannerControl::setStartColor(const wxColour& new_value)
{
    m_start_color = new_value;
    Refresh(false);
}

void BannerControl::setEndColor(const wxColour& new_value)
{
    m_end_color = new_value;
    Refresh(false);
}

void BannerControl::setTextColor(const wxColour& new_value)
{
    m_text_color = new_value;
    Refresh(false);
}

void BannerControl::setText(const wxString& text)
{
    m_text = text;
    Refresh(false);
}

void BannerControl::setTextAlignment(int alignment)
{
    m_alignment = alignment;
    Refresh(false);
}

void BannerControl::setTextPadding(int padding)
{
    // NOTE: text padding only applies if alignment
    //       is set to alignLeft and alignRight
        
    m_padding = padding;
    Refresh(false);
}

void BannerControl::setGradientDirection(int direction)
{
    m_gradient_direction = direction;
    Refresh(false);
}

void BannerControl::setBorder(int border_sides)
{
    m_border_sides = border_sides;
    Refresh(false);
}

void BannerControl::setBorderColor(const wxColour& new_value)
{
    m_border_color = new_value;
    Refresh(false);
}

void BannerControl::onPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);

    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);
    
    dc.SetBackgroundMode(wxTRANSPARENT);
    dc.SetFont(m_font);

    wxBrush brush(m_start_color, wxSOLID);

    dc.SetBrush(brush);
    dc.SetPen(*wxTRANSPARENT_PEN);

    if (m_gradient_direction == gradientNone)
    {
        dc.DrawRectangle(0, 0, cli_width, cli_height);
    }
     else
    {
        wxDirection dir = wxEAST;
        if (m_gradient_direction == gradientVertical)
            dir = wxSOUTH;
        if (m_gradient_direction == gradientHorizontal)
            dir = wxEAST;
            
        dc.GradientFillLinear(wxRect(0, 0, cli_width, cli_height),
                              m_start_color, m_end_color, dir);
    }
    
    wxCoord text_height, text_width;
    dc.GetTextExtent(m_text, &text_width, &text_height);

    int xoff, yoff;

    // calculate text x-offset
    switch (m_alignment)
    {
        case alignLeft:
            xoff = m_padding;
            break;
        case alignCenter:
            xoff = (cli_width/2)-(text_width/2);
            break;
        case alignRight:
            xoff = cli_width-text_width-m_padding;
            break;
    }
    
    // calculate text y-offset
    yoff = (cli_height/2)-(text_height/2);
    if ((m_border_sides & borderTop) && !(m_border_sides & borderBottom))
        yoff++;
    if ((m_border_sides & borderBottom) && !(m_border_sides & borderTop))
        yoff--;
    
    dc.SetTextForeground(m_text_color);
    dc.DrawText(m_text, xoff, yoff);
    
    // if there is a border, draw it last
    
    dc.SetPen(wxPen(m_border_color));
    
    if (m_border_sides & borderLeft)
        dc.DrawLine(0, 0, 0, cli_height);
    if (m_border_sides & borderRight)
        dc.DrawLine(cli_width-1, 0, cli_width-1, cli_height);
    if (m_border_sides & borderTop)
        dc.DrawLine(0, 0, cli_width, 0);
    if (m_border_sides & borderBottom)
        dc.DrawLine(0, cli_height-1, cli_width, cli_height-1);
}


void BannerControl::onSize(wxSizeEvent& event)
{
    Refresh(false);
}



};
