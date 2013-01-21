/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-12
 *
 */


#include "kxinter.h"
#include "hexview.h"
#include "datadoc.h"


BEGIN_EVENT_TABLE(HexView, wxWindow)
    EVT_PAINT(HexView::onPaint)
    EVT_CHAR(HexView::onChar)
    EVT_SIZE(HexView::onSize)
    EVT_SCROLLWIN(HexView::onScroll)
    EVT_MOUSEWHEEL(HexView::onMouseWheel)
END_EVENT_TABLE()



HexView::HexView(wxWindow* parent,
                 DataDoc* doc,
                 kcl::RulerControl* top_ruler,
                 wxStatusBar* status_bar) :
         DataViewBase(parent, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN)
{
    SetBackgroundColour(*wxWHITE);
    m_doc = doc;
    m_cur_line = 0;
    m_xoffset = 0;

    wxClientDC dc(this);

    wxCoord char_width, char_height;
    dc.GetTextExtent(wxT("X"), &char_width, &char_height);

    m_line_height = char_height;
    m_char_width = char_width;
    m_max_lines = 0;
}

HexView::~HexView()
{
}

void HexView::onPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);

    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    dc.SetFont(m_doc->m_font);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(*wxWHITE_BRUSH);

    wxCoord char_width, char_height;
    dc.GetTextExtent(wxT("X"), &char_width, &char_height);

    wxCoord offsettext_width, offsettext_height;
    dc.GetTextExtent(wxT("XXXXXXXX"), &offsettext_width, &offsettext_height);

    wxCoord hexbyte_width, hexbyte_height;
    dc.GetTextExtent(wxT("XX"), &hexbyte_width, &hexbyte_height);

    int text_x = offsettext_width+30;

    wxUChar buf[2];
    wxUChar rowbuf[256];

    buf[1] = 0;

    int row, col;
    int y = 0;
    int invalid_x = 0;

    row = m_cur_line;

    while (1)
    {
        int i;
        for (i = 0; i < 16; ++i)
        {
            rowbuf[i] = m_doc->getByte((row*16)+i);
        }


        dc.DrawRectangle(10, y, offsettext_width, offsettext_height);
        dc.DrawText(wxString::Format(wxT("%08X"), (row * 16)), 10, y);

        int x = offsettext_width + 30;

        for (col = 0; col < 16; ++col)
        {
            if (col == 8)
            {
                dc.DrawRectangle(x, y, char_width, char_height);
                dc.DrawText(wxT("-"), x, y);
                x += char_width;
            }

            dc.DrawRectangle(x, y, hexbyte_width+10, hexbyte_height);
            x += 5;
            dc.DrawText(wxString::Format(wxT("%02X"), rowbuf[col]), x, y);
            x += (hexbyte_width+5);

            if (invalid_x < x)
                invalid_x = x;
            if (x >= cli_width)
                break;
        }

        x += 20;

        for (col = 0; col < 16; ++col)
        {
            if (m_doc->m_ebc)
            {
                buf[0] = cfw::ebcdic2ascii(rowbuf[col]);
            }
             else
            {
                buf[0] = rowbuf[col];
            }

            dc.DrawRectangle(x, y, char_width, char_height);
            dc.DrawText(buf, x, y);
            x += char_width;

            if (invalid_x < x)
                invalid_x = x;
            if (x >= cli_width)
                break;
        }

        y += char_height;
        x = text_x;
        row++;

        if (y >= cli_height)
            break;
    }
    
    // -- draw invalid area --
    dc.DrawRectangle(invalid_x, 0, cli_width-invalid_x, cli_height);
}


void HexView::onChar(wxKeyEvent& event)
{
    char c = event.GetKeyCode();

    if (c == '+' || c == '=')
    {
        m_doc->setRecordLength(m_doc->getRecordLength()+1);
        Refresh(false);
    }
     else if (c == '-')
    {
        int new_reclen = m_doc->getRecordLength() - 1;
        if (new_reclen < 1)
            new_reclen = 1;

        m_doc->setRecordLength(new_reclen);

        Refresh(false);
    }

    updateScrollbars();
}

void HexView::onSize(wxSizeEvent& event)
{
    updateScrollbars();
}

void HexView::updateScrollbars()
{
    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    if (m_cur_line > m_max_lines)
        m_max_lines = m_cur_line;

    SetScrollbar(wxVERTICAL, m_cur_line, cli_height/m_line_height, m_max_lines+500);
    //SetScrollbar(wxHORIZONTAL, 0, (cli_width-100)/m_char_width, m_doc->m_record_len);
}

int HexView::getLinesPerPage()
{
    if (m_line_height == 0)
        return 0;

    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    return (cli_height/m_line_height);
}

void HexView::onScroll(wxScrollWinEvent& event)
{
/*
    if (event.GetOrientation() == wxHORIZONTAL)
    {
        int newpos = m_xoffset;

        if (event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
        {
            if (newpos > 0)
            {
                newpos--;
            }
        }

        if (event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
        {
            newpos++;
        }
    
        if (event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
        {
        }

        if (event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
        {
        }

        if (event.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK)
        {
            newpos = event.GetPosition();
        }

        if (newpos < 0)
            newpos = 0;

        if (newpos != m_xoffset)
        {
            m_xoffset = newpos;
            updateScrollbars();
            Refresh(false);
        }

    }
*/

    if (event.GetOrientation() == wxVERTICAL)
    {
        int newpos = m_cur_line;

        if (event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
        {
            newpos -= 1;
        }

        if (event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
        {
            newpos += 1;
        }

        if (event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
        {
            newpos -= (getLinesPerPage()-1);
        }

        if (event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
        {
            newpos += (getLinesPerPage()-1);
        }

        if (event.GetEventType() == wxEVT_SCROLLWIN_THUMBRELEASE)
        {
            newpos = event.GetPosition();
        }

        if (newpos < 0)
            newpos = 0;

        if (newpos != m_cur_line)
        {
            m_cur_line = newpos;
            updateScrollbars();
            Refresh(false);
        }
    }
}

void HexView::onMouseWheel(wxMouseEvent& event)
{
    if (event.GetWheelRotation() > 0)
    {
        m_cur_line -= 5;
        if (m_cur_line < 0)
            m_cur_line = 0;
    }
     else
    {
        m_cur_line += 5;
    }

    Refresh(false);
}



void HexView::setOffset(int offset)
{
    m_cur_line = offset/16;
    Refresh(false);
}


