/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Aaron L. Williams
 * Created:  2003-04-08
 *
 */


#include <wx/wx.h>
#include <vector>
#include <xcm/signal.h>
#include <cmath>


#ifdef WIN32
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#define max _cpp_max
#define min _cpp_min
#endif


#include "rulercontrol.h"


#define RULER_SCALE_MARGIN       6
#define RULER_SCALE_WIDTH       14
#define RULER_MIN_TICK_SPACE     5


namespace kcl
{

BEGIN_EVENT_TABLE(RulerControl, wxControl)
    EVT_SIZE(RulerControl::onSize)
    EVT_MOUSE_EVENTS(RulerControl::onMouse)
    EVT_PAINT(RulerControl::onPaint)
END_EVENT_TABLE()

RulerControl::RulerControl(wxWindow* parent,
                           wxWindowID id,
                           const wxPoint& pos,
                           const wxSize& size) :
                              wxControl(parent,
                              id,
                              pos,
                              size,
                              wxNO_FULL_REPAINT_ON_RESIZE |
                              wxCLIP_CHILDREN |
                              wxNO_BORDER)
{
    // intialize bitmap variables
    m_bmp_alloc_width = 0;
    m_bmp_alloc_height = 0;

    m_mouse_action = actionNone;
    m_mouse_xlast = 0;
    m_mouse_ylast = 0;
    m_mouse_x = 0;
    m_mouse_y = 0;
    m_action_x = 0;
    m_action_y = 0;

    m_scale = RULER_SCREEN_PPI;

    m_length = 0;
    m_width = 0;
    m_offset = 0;

    m_origin = 0;
    m_origin_old = 0;

    m_number_tick_interval = 0;
    m_major_tick_interval = 0;
    m_minor_tick_interval = 0;
    
    m_minWidth = size.x;
    m_minHeight = size.y;
    
    m_bkbrush = wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    m_bkpen = wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
}

RulerControl::~RulerControl()
{

}

void RulerControl::allocBitmap(int width, int height)
{
    if (width < 16)
        width = 16;

    if (height < 16)
        height = 16;

    if (width <= m_bmp_alloc_width && height <= m_bmp_alloc_height)
        return;

    m_bmp.Create(width, height, -1);
    m_memdc.SelectObject(m_bmp);

    m_bmp_alloc_width = width;
    m_bmp_alloc_height = height;
}

int RulerControl::mtod(int x)
{
    return int(x*((double)m_scale/(double)RULER_MODEL_PPI));
}

int RulerControl::dtom(int x)
{
    return int(x*((double)RULER_MODEL_PPI/(double)m_scale));
}

void RulerControl::setScale(int scale)
{
    m_scale = scale;
}

int RulerControl::getScale()
{
    return m_scale;
}

void RulerControl::setDimensions(int width, int length, int offset)
{
    m_width = width;
    m_length = length;
    m_offset = offset;
}

void RulerControl::getDimensions(int* width, int* length, int* offset)
{
    *width = m_width;
    *length = m_length;
    *offset = m_offset;
}

void RulerControl::setOrigin(int origin)
{
    if (origin == m_origin)
        return;

    // clamp at origin
    if (origin <= 0)
    {
        m_origin = 0;
        return;
    }

    // clamp at maximum length
    if (mtod(origin) > m_length)
    {
        m_origin = dtom(m_length);
        return;
    }

    // snap to scale
    int precision;
    precision = wxMin(m_number_tick_interval,
                wxMin(m_major_tick_interval, m_minor_tick_interval));

    if (precision == 0)
        return;

    int r = (origin % precision);
    if (r < precision/2)
    {
        m_origin = origin - r;
    }
    else
    {
        m_origin = origin + (precision - r);
    }
}

int RulerControl::getOrigin()
{
    return m_origin;
}

void RulerControl::setTickInterval(int number, int major, int minor)
{
    m_number_tick_interval = number;
    m_major_tick_interval = major;
    m_minor_tick_interval = minor;
}

void RulerControl::getTickInterval(int* number, int* major, int* minor)
{
    *number = m_number_tick_interval;
    *major = m_major_tick_interval;
    *minor = m_minor_tick_interval;
}

void RulerControl::addGuide(const RulerGuide& guide)
{
    m_guides.push_back(guide);
}

void RulerControl::removeGuide(const RulerGuide& guide)
{
    std::vector<RulerGuide>::iterator it;

    for (it = m_guides.begin(); it != m_guides.end(); ++it)
    {
        if (*it == guide)
        {
            m_guides.erase(it);
            return;
        }
    }
}

RulerGuide RulerControl::getGuideByIdx(int guide_idx)
{
    int count = getGuideCount();

    if (guide_idx < 0)
        return m_guides[0];

    if (guide_idx >= count)
        return m_guides[count - 1];

    return m_guides[guide_idx];
}

int RulerControl::getGuideCount()
{
    return m_guides.size();
}

void RulerControl::render()
{
    int tot_width, tot_height;
    GetClientSize(&tot_width, &tot_height);

    allocBitmap(tot_width, tot_height);

    // draw grey background
    m_memdc.SetPen(m_bkpen);
    m_memdc.SetBrush(m_bkbrush);
    m_memdc.DrawRectangle(0, 0, tot_width, tot_height);

    int scale_margin = RULER_SCALE_MARGIN;
    int scale_width = RULER_SCALE_WIDTH;
    int tick_threshold = RULER_MIN_TICK_SPACE;
    int length = m_length;

    // draw ruler background
    if (tot_width > tot_height)
    {
        // limit the length to the screen area
        length = wxMin(length, tot_width);
    
        // draw white background
        m_memdc.SetPen(*wxWHITE_PEN);
        m_memdc.SetBrush(*wxWHITE_BRUSH);
        m_memdc.DrawRectangle(m_offset, (tot_height-m_width)/2, length, m_width);

        // draw grey background for pre-origin area
        m_memdc.SetPen(*wxGREY_PEN);
        m_memdc.SetBrush(*wxGREY_BRUSH);
        m_memdc.DrawRectangle(m_offset, (tot_height-m_width)/2, mtod(m_origin), m_width);
    }
    else
    {
        // limit the length to the screen area
        length = wxMin(length, tot_height);
    
        // draw white background
        m_memdc.SetPen(*wxWHITE_PEN);
        m_memdc.SetBrush(*wxWHITE_BRUSH);
        m_memdc.DrawRectangle((tot_width-m_width)/2, m_offset, m_width, length);

        // draw grey background for pre-origin area
        m_memdc.SetPen(*wxGREY_PEN);
        m_memdc.SetBrush(*wxGREY_BRUSH);
        m_memdc.DrawRectangle((tot_width-m_width)/2, m_offset, m_width, mtod(m_origin));
    }

    // major tick mark size
    int a1 = scale_margin + 5;
    int a2 = scale_margin + scale_width - 4;

    // minor tick mark size
    int b1 = scale_margin + 6;
    int b2 = scale_margin + scale_width - 6;

    // tick count
    int minor_tick_count = 0;
    int major_tick_count = 0;
    int number_tick_count = 0;
    int tick_count = 0;

    if (!(m_minor_tick_interval == 0 && m_major_tick_interval == 0 && number_tick_count == 0))
    {
        minor_tick_count = (dtom(length)/m_minor_tick_interval)+1;
        major_tick_count = (dtom(length)/m_major_tick_interval)+1;
        number_tick_count = (dtom(length)/m_number_tick_interval)+1;
        tick_count = wxMax(minor_tick_count, wxMax(major_tick_count, number_tick_count));
    }

    // draw tick marks
    m_memdc.SetPen(*wxBLACK_PEN);
    m_memdc.SetFont(wxFont(7, wxSWISS, wxNORMAL, wxNORMAL));

    if (tot_width > tot_height)
    {
        for (int i = 0; i < tick_count; ++i)
        {
            int x_pos, x_neg, x, y;

            // draw numbers
            x_pos = m_offset + mtod(m_origin + i*m_number_tick_interval);
            x_neg = m_offset + mtod(m_origin - i*m_number_tick_interval);

            m_memdc.GetTextExtent(wxString::Format(wxT("%d"), i), &x, &y);

            if (x_pos <= length + m_offset)
                m_memdc.DrawText(wxString::Format(wxT("%d"), i), 
                    x_pos - x/2, scale_margin + scale_width/2 - y/2 + 1);

            if (x_neg >= m_offset)
                m_memdc.DrawText(wxString::Format(wxT("%d"), i),
                    x_neg - x/2, scale_margin + scale_width/2 - y/2 + 1);

            // draw major tick marks
            if (m_major_tick_interval*i % m_number_tick_interval > 0)
            {
                x_pos = m_offset + mtod(m_origin + i*m_major_tick_interval);
                x_neg = m_offset + mtod(m_origin - i*m_major_tick_interval);
                
                if (x_pos <= length + m_offset)
                    m_memdc.DrawLine(x_pos, a1, x_pos, a2);
                
                if (x_neg >= m_offset)
                    m_memdc.DrawLine(x_neg, a1, x_neg, a2);
            }

            // draw minor tick marks
            if (m_minor_tick_interval*i % m_number_tick_interval > 0 &&
                m_minor_tick_interval*i % m_major_tick_interval > 0)
            {
                x_pos = m_offset + mtod(m_origin + i*m_minor_tick_interval);
                x_neg = m_offset + mtod(m_origin - i*m_minor_tick_interval);

                if (x_pos <= length + m_offset)
                    m_memdc.DrawLine(x_pos, b1, x_pos, b2);

                if (x_neg >= m_offset)
                    m_memdc.DrawLine(x_neg, b1, x_neg, b2);
            }
        }
    }
    else
    {
        for (int i = 0; i < tick_count; ++i)
        {
            int x_pos, x_neg, x, y;

            // draw numbers
            x_pos = m_offset + mtod(m_origin + i*m_number_tick_interval);
            x_neg = m_offset + mtod(m_origin - i*m_number_tick_interval);

            m_memdc.GetTextExtent(wxString::Format(wxT("%d"), i), &x, &y);

            if (x_pos <= length + m_offset)
                m_memdc.DrawRotatedText(wxString::Format(wxT("%d"), i),
                    scale_margin + scale_width/2 - y/2, x_pos + x/2 + 1, 90);


            if (x_neg >= m_offset)
                m_memdc.DrawRotatedText(wxString::Format(wxT("%d"), i),
                    scale_margin + scale_width/2 - y/2, x_neg + x/2 + 1, 90);


            // draw major tick marks
            if (m_major_tick_interval*i % m_number_tick_interval > 0)
            {
                x_pos = m_offset + mtod(m_origin + i*m_major_tick_interval);
                x_neg = m_offset + mtod(m_origin - i*m_major_tick_interval);
                
                if (x_pos <= length + m_offset)
                    m_memdc.DrawLine(a1, x_pos, a2, x_pos);
                
                if (x_neg >= m_offset)
                    m_memdc.DrawLine(a1, x_neg, a2, x_neg);
            }

            // draw minor tick marks
            if (m_minor_tick_interval*i % m_number_tick_interval > 0 &&
                m_minor_tick_interval*i % m_major_tick_interval > 0)
            {
                x_pos = m_offset + mtod(m_origin + i*m_minor_tick_interval);
                x_neg = m_offset + mtod(m_origin - i*m_minor_tick_interval);

                if (x_pos <= length + m_offset)
                    m_memdc.DrawLine(b1, x_pos, b2, x_pos);

                if (x_neg >= m_offset)
                    m_memdc.DrawLine(b1, x_neg, b2, x_neg);
            }
        }
    }
}

void RulerControl::repaint(wxDC* dc)
{
    if (dc)
    {
        dc->Blit(0, 0, m_bmp_alloc_width, m_bmp_alloc_height, &m_memdc, 0, 0);
    }
     else
    {
        wxClientDC cdc(this);
        cdc.Blit(0, 0, m_bmp_alloc_width, m_bmp_alloc_height, &m_memdc, 0, 0);
    }
}

void RulerControl::onSize(wxSizeEvent& evt)
{
    render();
    repaint();
}

void RulerControl::onPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    repaint(&dc);
}

void RulerControl::onMouse(wxMouseEvent& evt)
{
    int tot_width, tot_height;
    GetClientSize(&tot_width, &tot_height);

    evt.GetPosition(&m_mouse_x, &m_mouse_y);

    int origin = m_offset + mtod(m_origin);

    int tol = 5;

    // set the focus
    if (evt.GetEventType() == wxEVT_LEFT_DOWN ||
        evt.GetEventType() == wxEVT_RIGHT_DOWN ||
        evt.GetEventType() == wxEVT_MIDDLE_DOWN)
    {
        SetFocus();
    }

    if (evt.GetEventType() == wxEVT_LEFT_DOWN)
    {
        if ((tot_width > tot_height && abs(m_mouse_x - origin) <= tol) ||
            (tot_width <= tot_height && abs(m_mouse_y - origin) <= tol))
        {
            m_mouse_action = actionMoveOrigin;
            m_action_x = m_mouse_x;
            m_action_y = m_mouse_y;
            m_mouse_xlast = m_mouse_x;
            m_mouse_ylast = m_mouse_y;
            m_origin_old = getOrigin();

            // set the cursor
            SetCursor(*wxSTANDARD_CURSOR);

            // pass -1 for origin
            sigMoveGuideStart(this, -1);
        }

        CaptureMouse();
    }

    if (evt.GetEventType() == wxEVT_LEFT_UP)
    {
        if (m_mouse_action == actionMoveOrigin)
        {
            // pass -1 for origin
            sigMoveGuideEnd(this, -1);
        }

        m_mouse_action = actionNone;

        if (GetCapture() == this)
        {
            ReleaseMouse();
        }

        sigReturnFocus();
    }

    if (evt.GetEventType() == wxEVT_LEFT_DCLICK)
    {
        CaptureMouse();
    }

    if (evt.GetEventType() == wxEVT_MOTION)
    {
        // set the cursor
        if (m_mouse_action == actionNone)
        {
            int x, y;
            GetPosition(&x, &y);

            if (tot_width > tot_height)
            {
                if (abs(m_mouse_x - origin) <= tol && m_mouse_y >= y && m_mouse_y <= y + tot_height)
                    SetCursor(wxCursor(wxCURSOR_SIZEWE));
                else
                    SetCursor(*wxSTANDARD_CURSOR);
            }
            else
            {
                if (abs(m_mouse_y - origin) <= tol && m_mouse_x >= x && m_mouse_x <= x + tot_width)
                    SetCursor(wxCursor(wxCURSOR_SIZENS));
                else
                    SetCursor(*wxSTANDARD_CURSOR);
            }
        }

        // move the origin
        if (m_mouse_action == actionMoveOrigin)
        {
            if (tot_width > tot_height)
            {
                setOrigin(dtom(m_mouse_x - m_action_x) + m_origin_old);
            }
            else
            {
                setOrigin(dtom(m_mouse_y - m_action_y) + m_origin_old);
            }

            // pass -1 for origin
            sigMoveGuide(this, -1);

            render();
            repaint();
        }

        m_mouse_xlast = m_mouse_x;
        m_mouse_ylast = m_mouse_y;
    }
}


};

