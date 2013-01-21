/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Aaron L. Williams
 * Created:  2002-04-09
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include <wx/list.h>
#include "notebookcontrol.h"
#include <kl/klib.h>


namespace kcl
{


static const char* xpm_scroll_left[] = {
"16 16 2 1",
"  c None",
"! c #000000",
"                ",
"                ",
"                ",
"                ",
"         !      ",
"        !!      ",
"       !!!      ",
"      !!!!      ",
"       !!!      ",
"        !!      ",
"         !      ",
"                ",
"                ",
"                ",
"                ",
"                "};


static const char* xpm_scroll_right[] = {
"16 16 2 1",
"  c None",
"! c #000000",
"                ",
"                ",
"                ",
"                ",
"      !         ",
"      !!        ",
"      !!!       ",
"      !!!!      ",
"      !!!       ",
"      !!        ",
"      !         ",
"                ",
"                ",
"                ",
"                ",
"                "};


const int notebook_tab_angle_offset = 8;
const int notebook_tab_start_noscroll = 20;
const int notebook_tab_start_scroll = 40;
const int notebook_tab_padding_horz = 4;
const int notebook_tab_padding_vert = 1;
const int notebook_bitmap_text_padding = 5;

enum
{
    ID_ScrollLeft = wxID_HIGHEST + 1,
    ID_ScrollRight
};


BEGIN_EVENT_TABLE(NotebookControl, wxControl)
    EVT_PAINT(NotebookControl::onPaint)
    EVT_SIZE(NotebookControl::onSize)
    EVT_MOUSE_EVENTS(NotebookControl::onMouse)
    EVT_BUTTON(ID_ScrollLeft, NotebookControl::onScrollLeft)
    EVT_BUTTON(ID_ScrollRight, NotebookControl::onScrollRight)
END_EVENT_TABLE()

NotebookControl::NotebookControl(wxWindow* parent,
                                 wxWindowID id,
                                 const wxPoint& position,
                                 const wxSize& size,
                                 long style) :
            wxControl(parent, id, position, size, style)
{
    m_notebook_tab_start = 0;
    m_active_index = -1;
    m_tab_height = 0;
    m_user_tab_height = -1;
    m_total_tab_width = 0;
    m_xoff = 0;
    m_current_scroll_page = 0;

    m_scroll_mode = notebookScrollAuto;

    m_brush_inactive.SetColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    m_brush_active.SetColour(*wxWHITE);

    m_extra_panel = NULL;
    m_tabscroll_panel = NULL;
    m_scroll_left_button = NULL;
    m_scroll_right_button = NULL;

    m_extra_panel_width = 0;


    // create a default tab scroll control

    m_tabscroll_panel = new wxPanel(this, -1);
    m_tabscroll_panel->SetSize(0, 0, 40, 16);

    wxBoxSizer* scroll_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_scroll_left_button = new kcl::Button(m_tabscroll_panel,
                                           ID_ScrollLeft,
                                           wxDefaultPosition,
                                           wxDefaultSize,
                                           wxT(""),
                                           wxBitmap(xpm_scroll_left));


    m_scroll_right_button = new kcl::Button(m_tabscroll_panel,
                                           ID_ScrollRight,
                                           wxDefaultPosition,
                                           wxDefaultSize,
                                           wxT(""),
                                           wxBitmap(xpm_scroll_right));

    m_scroll_left_button->setDimensions(16, 16);
    m_scroll_right_button->setDimensions(16, 16);

    m_scroll_left_button->setMode(kcl::Button::mode3d);
    m_scroll_right_button->setMode(kcl::Button::mode3d);

    scroll_sizer->Add(m_scroll_left_button);
    scroll_sizer->Add(m_scroll_right_button);

    m_tabscroll_panel->SetSizer(scroll_sizer);
    m_tabscroll_panel->SetAutoLayout(true);
    setScrollMode(notebookScrollAuto);
}

NotebookControl::~NotebookControl()
{
    deleteAllPages();
}

void NotebookControl::setScrollMode(int scroll_mode)
{
    m_scroll_mode = scroll_mode;

    doSizing();
    Refresh(true);
}

void NotebookControl::setExtraPanel(wxPanel* extra_panel)
{
    m_extra_panel = extra_panel;
    if (m_extra_panel != NULL)
    {
        doSizing();
    }
}

wxPanel* NotebookControl::getExtraPanel()
{
    return m_extra_panel;
}

void NotebookControl::setTabHeight(int tab_height)
{
    m_user_tab_height = tab_height;
    calcTabHeight();
}


bool NotebookControl::create(wxWindow* parent,
                             wxWindowID id,
                             const wxPoint& pos, 
                             const wxSize& size,
                             long style,
                             const wxString& name)
{
    return false;
}

bool NotebookControl::addPage(wxWindow* window,
                              const wxString& text,
                              const wxBitmap& bitmap,
                              bool select)
{
    if (window == NULL || window->IsTopLevel())
        return false;

    NotebookPage page;
    page.m_window = window;
    page.m_text = text;
    page.m_bitmap = bitmap;
    m_pages.push_back(page);
    calcTabHeight();
    calcTabWidths();

    doSizing();

    if (select || m_active_index == -1)
    {
        setSelection(m_pages.size()-1);
    }
     else
    {
        window->Show(false);
    }

    return true;
}

bool NotebookControl::insertPage(int index,
                                 wxWindow* window,
                                 const wxString& text,
                                 const wxBitmap& bitmap,
                                 bool select)
{
    if (window == NULL || window->IsTopLevel())
        return false;

    if (select)
    {
        setSelection(index);
    }

    return true;
}

bool NotebookControl::deletePage(int page)
{
    int page_count = getPageCount();

    if (page < 0 || page >= page_count)
        return false;
    
    if (!wxPendingDelete.Member(m_pages[page].m_window))
        wxPendingDelete.Append(m_pages[page].m_window);

    m_pages.erase(m_pages.begin() + page);

    calcTabWidths();

    doSizing();


    if (m_active_index < 0 || (size_t)m_active_index >= m_pages.size())
    {
        if (m_pages.size() == 0)
        {
            m_active_index = -1;
        }
         else
        {
            setSelection(0);
        }
    }
     else
    {
        if (m_active_index == page)
        {
            m_active_index = -1;
            setSelection(page);
        }
    }


    Refresh(true);

    return true;
}

bool NotebookControl::deleteAllPages()
{
    std::vector<NotebookPage>::iterator it;
    for (it = m_pages.begin(); it != m_pages.end(); ++it)
    {
        it->m_window->Destroy();
    }

    m_pages.clear();
    m_active_index = -1;

    return true;
}

wxWindow* NotebookControl::getPage(int page)
{
    if (page < 0 || (size_t)page >= m_pages.size())
        return NULL;

    return m_pages[page].m_window;
}

int NotebookControl::getPageCount()
{
    return m_pages.size();
}

bool NotebookControl::setSelection(int page)
{
    if (page < 0 || (size_t)page >= m_pages.size())
        return false;

    if (page == m_active_index)
        return true;
    
    if (m_active_index >= 0 && (size_t)m_active_index < m_pages.size())
    {
        m_pages[m_active_index].m_window->Show(false);
    }

    m_active_index = page;
    doSizing();
    m_pages[m_active_index].m_window->Show(true);
    Refresh(false);

    sigActivePageChanged(page);

    return true;
}

int NotebookControl::getSelection()
{
    return m_active_index;
}


bool NotebookControl::setPageText(int page, const wxString& text)
{
    if (page < 0 || (size_t)page >= m_pages.size())
        return false;

    m_pages[page].m_text = text;
    calcTabWidths();
    return true;
}

wxString NotebookControl::getPageText(int page)
{
    if (m_active_index < 0 || (size_t)m_active_index >= m_pages.size())
        return wxT("");

    return m_pages[m_active_index].m_text;
}

bool NotebookControl::setPageBitmap(int page, const wxBitmap& bitmap)
{
    if (page < 0 || (size_t)page >= m_pages.size())
        return false;

    m_pages[page].m_bitmap = bitmap;
    calcTabHeight();
    return true;
}

wxBitmap NotebookControl::getPageBitmap(int page)
{
    if (m_active_index < 0 || (size_t)m_active_index >= m_pages.size())
        return wxNullBitmap;

    return m_pages[m_active_index].m_bitmap;
}


void NotebookControl::calcTabHeight()
{        
    m_tab_height = 0;
    wxCoord w, h;

    wxClientDC dc(this);
    dc.SetFont(*wxNORMAL_FONT);

    dc.GetTextExtent(wxT("ABCDEFHXfgkj"), &w, &h);
    m_tab_height = h;

    std::vector<NotebookPage>::iterator it;
    for (it = m_pages.begin(); it != m_pages.end(); ++it)
    {
        if (it->m_bitmap.Ok())
        {
            m_tab_height = std::max(m_tab_height, it->m_bitmap.GetHeight());
        }
    }

    m_tab_height = h + 2*notebook_tab_padding_vert + 2;

    if (m_user_tab_height != -1)
    {
        m_tab_height = std::max(m_tab_height, m_user_tab_height);
    }
}


void NotebookControl::drawTab(wxDC& dc, int page)
{
    int x1, y1, x2, y2;
    int client_height, client_width;
    std::vector<NotebookPage>::iterator it;

    GetClientSize(&client_width, &client_height);
    it = m_pages.begin() + page;

    x1 = m_pages[page].m_tab_offset + m_xoff;
    y1 = client_height - m_tab_height;
    x2 = x1 + it->m_tab_width;
    y2 = y1 + m_tab_height - 1;

    wxPoint points[4];

    // fill out the hit rectangle
    it->m_hitrect.x = x1 - notebook_tab_angle_offset;
    it->m_hitrect.y = y1;
    it->m_hitrect.width = it->m_tab_width + notebook_tab_angle_offset;
    it->m_hitrect.height = m_tab_height;

    dc.SetBrush(m_active_index == page ? m_brush_active : m_brush_inactive);

    points[0].x = x1 - notebook_tab_angle_offset;
    points[0].y = y1;
    points[1].x = x1;
    points[1].y = y2;
    points[2].x = x2;
    points[2].y = y2;
    points[3].x = x2 + notebook_tab_angle_offset;
    points[3].y = y1;

    dc.DrawPolygon(4, points);

    if (it->m_bitmap.Ok())
    {
        // find out height of bitmap
        int bitmap_height;
        bitmap_height = it->m_bitmap.GetHeight();
        
        dc.DrawBitmap(it->m_bitmap,
                      x1 + notebook_tab_padding_horz,
                      y1 + (m_tab_height - bitmap_height)/2,
                      true);

        dc.DrawText(it->m_text,
                    x1 + notebook_tab_padding_horz +
                           it->m_bitmap.GetWidth() +
                           notebook_bitmap_text_padding,
                    y1 + notebook_tab_padding_vert + 1);
    }
    else
    {
        dc.DrawText(it->m_text,
                    x1 + notebook_tab_padding_horz,
                    y1 + notebook_tab_padding_vert);
    }

    if (m_active_index == page)
    {
        dc.SetPen(wxPen(m_brush_active.GetColour(), 1, wxSOLID));
        dc.DrawLine(points[0].x+1, points[0].y, points[3].x, points[3].y);
    }
}

void NotebookControl::setInactiveBackground(const wxBrush& brush)
{
    m_brush_inactive = brush;
}

void NotebookControl::setActiveBackground(const wxBrush& brush)
{
    m_brush_active = brush;
}

void NotebookControl::onPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);

    if (m_active_index == -1)
    {
        // if there are no active pages, don't draw anything
        return;
    }

    int client_height, client_width;
    GetClientSize(&client_width, &client_height);

    dc.SetFont(*wxNORMAL_FONT);
    dc.SetPen(*wxBLACK_PEN);
    dc.DrawLine(0, client_height-m_tab_height, client_width, client_height-m_tab_height);

    // draw tabs
    int page_count = m_pages.size();
    for (int page = 0; page < page_count; page++)
    {
        if (page != m_active_index)
        {
            drawTab(dc, page);
        }
    }

    drawTab(dc, m_active_index);


    int x1_scroll = 0;
    int x2_scroll = m_notebook_tab_start - notebook_tab_angle_offset;
    int y1_scroll = client_height - m_tab_height;
    int y2_scroll = client_height;

    if (m_extra_panel)
    {
        dc.SetPen(*wxBLACK_PEN);

        dc.DrawLine(client_width - m_extra_panel_width - 2, 
            y1_scroll, client_width, y1_scroll);

        if (m_total_tab_width > client_width - m_extra_panel_width)
        {
            dc.SetPen(*wxWHITE_PEN);

            dc.DrawLine(client_width - m_extra_panel_width - 3,
                        y1_scroll,
                        client_width - m_extra_panel_width - 3,
                        y2_scroll-1);

            dc.SetPen(*wxGREY_PEN);

            dc.DrawLine(client_width - m_extra_panel_width - 2,
                        y1_scroll,
                        client_width - m_extra_panel_width - 2,
                        y2_scroll);

            dc.SetPen(*wxBLACK_PEN);

            dc.DrawLine(client_width - m_extra_panel_width - 1,
                        y1_scroll,
                        client_width - m_extra_panel_width - 1,
                        y2_scroll);
        }
    }

    if (m_tabscroll_panel->IsShown())
    {
        dc.SetPen(*wxBLACK_PEN);

        dc.DrawLine(x1_scroll, y1_scroll, x2_scroll, y1_scroll);
        //dc.DrawLine(x2_scroll, y1_scroll, x2_scroll, y2_scroll);
    }
}


void NotebookControl::calcTabWidths()
{
    wxClientDC dc(this);
    dc.SetFont(*wxNORMAL_FONT);

    std::vector<NotebookPage>::iterator it;
    wxCoord w, h;
    int xoff = m_notebook_tab_start;
    for (it = m_pages.begin(); it != m_pages.end(); ++it)
    {
        dc.GetTextExtent(it->m_text, &w, &h);
        it->m_tab_offset = xoff;
        it->m_tab_width = w;
        it->m_tab_width += (notebook_tab_padding_horz*2);

        if (it->m_bitmap.Ok())
        {
            it->m_tab_width += (it->m_bitmap.GetWidth()+notebook_bitmap_text_padding);
        }

        xoff += (it->m_tab_width + 8);
    }

    m_total_tab_width = xoff;
}


void NotebookControl::doSizing()
{

    int client_height, client_width;
    GetClientSize(&client_width, &client_height);
 
    if (m_active_index >= 0 && (size_t)m_active_index < m_pages.size())
    {
        m_pages[m_active_index].m_window->SetSize(0, 0,
                                              client_width,
                                              client_height - m_tab_height);
    }


    if (m_extra_panel)
    {
        wxWindowList& children = m_extra_panel->GetChildren();
        int count = children.GetCount();
        wxWindowList::compatibility_iterator child;

        int width, height;
        int x_max = 0;

        for (int i = 0; i < count; ++i)
        {
            child = children.Item(i);
            child->GetData()->GetSize(&width, &height);
            x_max += width;
        }

        m_extra_panel_width = x_max;

        calcTabWidths();

        int extra_panel_offset = m_total_tab_width+8;
        if (client_width - m_extra_panel_width < extra_panel_offset)
        {
            extra_panel_offset = client_width - m_extra_panel_width;
        }

        m_extra_panel->SetSize(extra_panel_offset, 
                               client_height - m_tab_height+1,
                               m_extra_panel_width,
                               m_tab_height-1);
    }

    if (m_scroll_mode == notebookScrollOff)
    {
        m_tabscroll_panel->Show(false);
        m_xoff = 0;
        m_notebook_tab_start = notebook_tab_start_noscroll;
        calcTabWidths();
    }
     else if (m_scroll_mode == notebookScrollOn)
    {
        m_notebook_tab_start = notebook_tab_start_scroll;
        calcTabWidths();
        m_tabscroll_panel->Show(true);
    }
     else if (m_scroll_mode == notebookScrollAuto)
    {
        if ( m_total_tab_width <= client_width - m_extra_panel_width)
        {
            m_tabscroll_panel->Show(false);
            m_xoff = 0;
            m_notebook_tab_start = notebook_tab_start_noscroll;
            calcTabWidths();
        }
         else
        {
            m_notebook_tab_start = notebook_tab_start_scroll;
            calcTabWidths();
            m_tabscroll_panel->Show(true);
        }

    }

    if (m_tabscroll_panel)
    {
        m_tabscroll_panel->SetSize(0,
                             client_height - m_tab_height+1, 
                             m_notebook_tab_start - notebook_tab_angle_offset,
                             m_tab_height-1);
    }
}

void NotebookControl::onSize(wxSizeEvent& event)
{
    doSizing();
    Refresh(true);
}

void NotebookControl::onMouse(wxMouseEvent& event)
{
    if (event.GetEventType() == wxEVT_LEFT_DOWN)
    {
        // check to see where we clicked
        std::vector<NotebookPage>::iterator it;
        int idx = 0;
        for (it = m_pages.begin(); it != m_pages.end(); ++it)
        {
            if (event.m_x >= it->m_hitrect.x &&
                event.m_x < it->m_hitrect.x + it->m_hitrect.width)
            {
                setSelection(idx);
            }
            idx++;
        }
    }

    if (event.GetEventType() == wxEVT_RIGHT_DOWN)
    {
        // check to see where we clicked
        std::vector<NotebookPage>::iterator it;
        int idx = 0;
        for (it = m_pages.begin(); it != m_pages.end(); ++it)
        {
            if (event.m_x >= it->m_hitrect.x &&
                event.m_x < it->m_hitrect.x + it->m_hitrect.width &&
                event.m_y >= it->m_hitrect.y &&
                event.m_y < it->m_hitrect.y + it->m_hitrect.height)
            {
                sigTabRightClicked(idx);
                break;
            }

            idx++;
        }
    }
}

void NotebookControl::onScrollLeft(wxCommandEvent& event)
{
    if (m_current_scroll_page <= 0)
        return;

    m_current_scroll_page -= 1;
    m_xoff = -1*(m_pages[m_current_scroll_page].m_tab_offset - m_notebook_tab_start);

    Refresh(true);
}

void NotebookControl::onScrollRight(wxCommandEvent& event)
{
    if (m_current_scroll_page >= getPageCount()-1)
        return;

    m_xoff = -1*(m_pages[m_current_scroll_page].m_tab_offset +
                 notebook_tab_angle_offset +
                 m_pages[m_current_scroll_page].m_tab_width -
                 m_notebook_tab_start);

    m_current_scroll_page += 1;
    Refresh(true);
}



}; // namespace kcl

