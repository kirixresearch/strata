/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2003-05-12
 *
 */


#include "kxinter.h"
#include "textview.h"
#include "datadoc.h"


BEGIN_EVENT_TABLE(TextView, wxWindow)
    EVT_PAINT(TextView::onPaint)
    EVT_CHAR(TextView::onChar)
    EVT_SIZE(TextView::onSize)
    EVT_SCROLLWIN(TextView::onScroll)
    EVT_MOUSE_EVENTS(TextView::onMouse)
END_EVENT_TABLE()



TextView::TextView(wxWindow* parent,
                   DataDoc* doc,
                   kcl::RulerControl* top_ruler,
                   wxStatusBar* status_bar) :
            DataViewBase(parent, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN)
{
    SetBackgroundColour(*wxWHITE);
    m_doc = doc;
    m_cur_line = 0;
    m_xoffset = 0;
    m_top_ruler = top_ruler;

    wxClientDC dc(this);
    dc.SetFont(m_doc->m_font);

    wxCoord char_width, char_height;
    dc.GetTextExtent(wxT("X"), &char_width, &char_height);

    wxCoord offsettext_width, offsettext_height;
    dc.GetTextExtent(wxT("XXXXXXXX"), &offsettext_width, &offsettext_height);

    m_line_height = char_height;
    m_char_width = char_width;
    m_char_height = char_height;
    m_offsettext_width = offsettext_width;
    m_offsettext_height = offsettext_height;
    m_max_lines = 0;
    m_mouse_action = actionNone;
    m_colselect_offset = -1;
    m_colselect_length = -1;
    m_colselect_reclen = -1;

    m_buf = new unsigned char[65536];


    // -- update status bar --
    m_status_bar = status_bar;

    if (status_bar)
    {
        int widths[] = { 120, 120, -1, 150 };
        status_bar->SetFieldsCount(4, widths);
        updateStatusBar();
    }
}

TextView::~TextView()
{
    delete m_buf;
}

void TextView::updateStatusBar(int field)
{
    if (!m_status_bar)
        return;

    if (field == -1 || field == 0)
    {
        wxString value;

        if (m_doc->m_dfd.type == dfdFixed)
        {
            value = wxString::Format(_("(Fixed) Length: %d"), m_doc->getRecordLength());
        }
         else if (m_doc->m_dfd.type == dfdVariable)
        {
            value = wxString::Format(_("(Variable)"));
        }
         else if (m_doc->m_dfd.type == dfdDelimited)
        {
            value = wxString::Format(_("(Delimited)"));
        }

        m_status_bar->SetStatusText(value, 0);
    }

    if (field == -1 || field == 1)
    {
        wxPoint pt = ScreenToClient(::wxGetMousePosition());
        int offset = positionToColumn(pt.x);
        if (offset < 0)
            offset = 0;
        m_status_bar->SetStatusText(wxString::Format(_("Column: %d"), offset), 1);
    }


    if (field == -1 || field == 3 || m_mouse_action == actionSelectColumn)
    {
        if (m_colselect_offset == -1)
        {
            m_status_bar->SetStatusText(wxEmptyString, 3);
        }
         else
        {       
            wxString value = wxString::Format(_("Offset: %d"), m_colselect_offset);
        
            if (m_mouse_action == actionSelectColumn && m_colselect_length > 0)
            {
                value += wxT(", ");
                value += wxString::Format(_("Width: %d"), m_colselect_length);
            }

            m_status_bar->SetStatusText(value, 3);
        }
    }
}


int TextView::positionToColumn(int x)
{
    x -= (m_offsettext_width+30);
    return (x/m_char_width)+m_xoffset;
}

int TextView::columnToPosition(int col)
{
    int x = col-m_xoffset;
    if (x < 0)
        x = 0;
     
    return (x*m_char_width)+(m_offsettext_width+30);
}

int TextView::rowToPosition(int row)
{
    return (row-m_cur_line)*m_line_height;
}

int TextView::positionToRow(int y)
{
    return m_cur_line + (y / m_line_height);
}

void TextView::onPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);

    updateStatusBar(0);

    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    dc.SetFont(m_doc->m_font);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(*wxWHITE_BRUSH);
    //dc.SetBackgroundMode(wxSOLID);

    wxColor normal_text_bgcolor = *wxWHITE;
    wxColor normal_text_fgcolor = *wxBLACK;
    wxColor selected_text_bgcolor = *wxBLACK;
    wxColor selected_text_fgcolor = *wxWHITE;

    wxBrush normal_text_bgbrush(normal_text_bgcolor, wxSOLID);
    wxBrush selected_text_bgbrush(selected_text_bgcolor, wxSOLID);

    int text_x = m_offsettext_width+30;

    wxUChar buf[2];
    buf[1] = 0;

    int row, col;
    int x = text_x;
    int y = 0;
    int invalid_x = columnToPosition(0);

    row = m_cur_line;


    int rec_len = m_doc->getRecordLength();

    while (1)
    {
        y = rowToPosition(row);

        dc.SetBrush(*wxLIGHT_GREY_BRUSH);
        dc.SetTextForeground(*wxBLACK);
        dc.DrawRectangle(0, y, m_offsettext_width+20, m_offsettext_height);
        dc.DrawText(wxString::Format(wxT("%08X"), (row * rec_len)), 10, y);

        dc.SetBrush(*wxWHITE_BRUSH);

        if (y >= cli_height)
            break;

        RecordInfo ri;
        m_doc->getRecord(row, m_buf, 65535, &ri);

        dc.DrawRectangle(columnToPosition(ri.len), y,
                         cli_width, m_line_height);

        for (col = m_xoffset; col < ri.len; ++col)
        {
            x = columnToPosition(col);

            if (y < 0)
                break;

            if (x >= cli_width)
                break;
   
            bool selected = false;

            // -- determine if we are in the selection area --
            
            if (m_colselect_offset != -1 &&
                col >= m_colselect_offset &&
                col <= (m_colselect_offset + m_colselect_length - 1))
            {
                selected = true;
            }

            if (m_colselect_reclen != -1)
            {
                if (ri.len != m_colselect_reclen)
                    selected = false;
            }


            if (selected)
            {
                dc.SetBrush(selected_text_bgbrush);
                dc.SetTextForeground(selected_text_fgcolor);
            }
             else
            {
                dc.SetBrush(normal_text_bgbrush);
                dc.SetTextForeground(normal_text_fgcolor);
            }
            
            if (m_doc->m_ebc)
            {
                buf[0] = cfw::ebcdic2ascii(m_buf[col]);
            }
             else
            {
                buf[0] = m_buf[col];
            }

            dc.DrawRectangle(x, y, m_char_width, m_char_height);

            dc.DrawText(buf, x, y);

            if (invalid_x < x + m_char_width)
                invalid_x = x + m_char_width;

        }

        row++;
    }
    
    // -- draw invalid area --
    dc.DrawRectangle(invalid_x, 0, cli_width-invalid_x, cli_height);
}


void TextView::onChar(wxKeyEvent& event)
{
    char c = event.GetKeyCode();

    int new_reclen = 0xffffff;

    if (c == '+' || c == '=')
    {
        new_reclen = m_doc->getRecordLength() + 1;
    }
     else if (c == '-')
    {
        new_reclen = m_doc->getRecordLength() - 1;
    }
     else if (c == '<')
    {
        new_reclen = m_doc->getRecordLength() - 5;
    }
     else if (c == '>')
    {
        new_reclen = m_doc->getRecordLength() + 5;
    }

    if (new_reclen != 0xffffff)
    {
        if (new_reclen < 1)
            new_reclen = 1;

        m_doc->setRecordLength(new_reclen);
        Refresh(false);
    }


    updateScrollbars();
}

void TextView::onSize(wxSizeEvent& event)
{
    updateScrollbars();
}

void TextView::updateScrollbars()
{
    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    if (m_cur_line > m_max_lines)
        m_max_lines = m_cur_line;

    SetScrollbar(wxVERTICAL, m_cur_line, cli_height/m_line_height, m_max_lines+500);
    SetScrollbar(wxHORIZONTAL, m_xoffset, (cli_width-100)/m_char_width, m_doc->getRecordLength());
}

int TextView::getLinesPerPage()
{
    if (m_line_height == 0)
        return 0;

    int cli_width, cli_height;
    GetClientSize(&cli_width, &cli_height);

    return (cli_height/m_line_height);
}

void TextView::onScroll(wxScrollWinEvent& event)
{

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
            if (m_top_ruler)
            {   

            }
            updateScrollbars();
            Refresh(false);   
        }
    }

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

void TextView::setColumnSelection(int offset, int len, int reclen)
{
    m_colselect_offset = offset;
    m_colselect_length = len;
    m_colselect_reclen = reclen;

    Refresh(false);
}


void TextView::getColumnSelection(int* offset, int* length, int* reclen)
{
    *offset = m_colselect_offset;
    *length = m_colselect_length;
    *reclen = m_colselect_reclen;
}

void TextView::clearColumnSelection()
{
    m_colselect_offset = -1;
    m_colselect_length = -1;
}

void TextView::refresh()
{
    Refresh(false);
}


void TextView::onMouse(wxMouseEvent& event)
{
    wxEventType event_type = event.GetEventType();

    if (event_type == wxEVT_MOUSEWHEEL)
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
        return;
    }
     else if (event_type == wxEVT_LEFT_DOWN)
    {
        if (event.ControlDown())
        {
        }
         else
        {
            wxPoint pt = event.GetPosition();
        
            m_mouse_action = actionSelectColumn;
            m_mouse_start = positionToColumn(pt.x);

            int row = positionToRow(pt.y);

            RecordInfo ri;
            m_doc->getRecord(row, m_buf, 65535, &ri);
        
            setColumnSelection(-1,-1, ri.len);

            CaptureMouse();
        }
    }
     else if (event_type == wxEVT_LEFT_UP)
    {
        if (GetCapture() == this)
        {
            ReleaseMouse();
        }

        m_mouse_action = actionNone;
    }
     else if (event_type == wxEVT_MOTION)
    {
        updateStatusBar(1);

        wxPoint pt = event.GetPosition();

        if (m_mouse_action == actionSelectColumn)
        {
            m_mouse_end = positionToColumn(pt.x);
            int select_start = std::min(m_mouse_start, m_mouse_end);
            int select_end = std::max(m_mouse_start, m_mouse_end);

            setColumnSelection(select_start, select_end-select_start+1, m_colselect_reclen);
        }

    }
}


void TextView::setOffset(int offset)
{
    m_cur_line = offset/m_doc->getRecordLength();
    Refresh(false);
}


