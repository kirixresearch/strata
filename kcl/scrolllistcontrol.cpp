/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2006-12-06
 *
 */


#include <wx/tokenzr.h>
#include "scrolllistcontrol.h"
#include "util.h"
#include <kl/thread.h>


const int SCROLL_STEP = 1;


namespace kcl
{


// helper functions for drawing text in the DC

static void drawTextWrap(wxDC& dc,
                         const wxString& str,
                         int x,
                         int y,
                         int width,
                         wxSize* finished_size)
{
    int text_x, text_y, line_height;
    dc.GetTextExtent(wxT("ABCDXHM"), &text_x, &line_height);
    dc.GetTextExtent(str, &text_x, &text_y);

    // if the string was empty, at least return a default height
    if (str.IsEmpty())
    {
        *finished_size = wxSize(0,line_height);
        return;
    }
    
    if (text_x <= width)
    {
        // we don't need to wrap the text
        dc.DrawText(str, x, y);
        *finished_size = wxSize(text_x,line_height);
        return;
    }
    
    // draw the lines
    
    wxString line, temp_line;
    int xoff = x;
    int yoff = y;
    int max_line_width = 0, total_height = line_height;
    
    wxStringTokenizer tkz(str, wxT(" \r\t\n"));
    while (tkz.HasMoreTokens())
    {
        wxString token = tkz.GetNextToken();
        
        if (token.IsEmpty())
            continue;
        
        temp_line += token;
        
        dc.GetTextExtent(temp_line, &text_x, &text_y);
        if (text_x > width)
        {
            // remove the trailing spaces and draw the line
            line.Trim();
            line.Trim(false);
            dc.DrawText(line, xoff, yoff);
            
            // update the total width
            if (text_x > max_line_width)
                max_line_width = text_x;
            
            // reset the x and y offsets
            xoff = x;
            yoff += line_height;
            
            // start off the next line
            temp_line = token;
        }
        
        temp_line += wxT(" ");
        line = temp_line;
    }
    
    // draw the last line
    if (line.Length() > 0)
    {
        // remove the trailing spaces and draw the line
        line.Trim();
        line.Trim(false);
        dc.DrawText(line, xoff, yoff);
        
        // reset the x and y offsets
        xoff = x;
        yoff += line_height;
    }
    
    total_height = yoff-y;
    *finished_size = wxSize(max_line_width,total_height);
}

static void drawTextEllipses(wxDC& dc,
                             const wxString& str,
                             int x,
                             int y,
                             int width,
                             wxSize* finished_size,
                             int ellipses = kcl::ScrollListElement::textEllipsesRight)
{
    int text_x, text_y, line_height;
    int ellipses_x, ellipses_y;
    dc.GetTextExtent(wxT("..."), &ellipses_x, &ellipses_y);
    dc.GetTextExtent(wxT("ABCDXHM"), &text_x, &line_height);
    dc.GetTextExtent(str, &text_x, &text_y);

    // if the string was empty, at least return a default height
    if (str.IsEmpty())
    {
        *finished_size = wxSize(0,line_height);
        return;
    }
    
    if (text_x <= width)
    {
        // we don't need any ellipses
        dc.DrawText(str, x, y);
        *finished_size = wxSize(text_x,line_height);
        return;
    }
    
    if (width < ellipses_x)
    {
        // we don't even have enough room to draw ellipses, bail out
        *finished_size = wxSize(0,line_height);
        return;
    }
    
    if (ellipses == kcl::ScrollListElement::textEllipsesLeft)
    {
        wxString draw_text, base_text = str;
        while (text_x > width)
        {
            base_text = base_text.Mid(1);
            draw_text = wxT("...") + base_text;
            dc.GetTextExtent(draw_text, &text_x, &text_y);
        }
        
        dc.DrawText(draw_text, x, y);
        *finished_size = wxSize(text_x,line_height);
    }
     else if (ellipses == kcl::ScrollListElement::textEllipsesRight)
    {
        wxString draw_text, base_text = str;
        while (text_x > width)
        {
            base_text.RemoveLast();
            draw_text = base_text + wxT("...");
            dc.GetTextExtent(draw_text, &text_x, &text_y);
        }
        
        dc.DrawText(draw_text, x, y);
        *finished_size = wxSize(text_x,line_height);
    }
}

// this function was ripped directly from grid.cpp
static void lightenMemDC(wxMemoryDC& dc, const wxBitmap& bmp)
{
    wxImage img = bmp.ConvertToImage();
    unsigned char* data = img.GetData();
    
    int height = img.GetHeight();
    int width = img.GetWidth();
    
    int x, y;
    wxColour col;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            unsigned char* r = data + (y*(width*3))+(x*3);
            unsigned char* g = r+1;
            unsigned char* b = r+2;

            *r = (unsigned char)blendColor((double)*r, 255.0, 0.3);
            *g = (unsigned char)blendColor((double)*g, 255.0, 0.3);
            *b = (unsigned char)blendColor((double)*b, 255.0, 0.3);
        }
    }
    
    wxBitmap new_bmp(img);
    dc.SelectObject(new_bmp);
}




// ScrollListElement class implementation

ScrollListElement::ScrollListElement()
{
    m_control = NULL;
    m_text = wxEmptyString;
    
    m_relative_to = NULL;
    m_position = wxPoint(0,0);
    m_size = wxSize(0,0);
    
    m_visible = true;
    m_stretch = false;
    m_text_bold = false;
    m_text_wrap = true;
    m_text_ellipses = textEllipsesRight;
    m_relative_position = positionBelow;
    m_relative_alignment = alignLeft;
    m_padding_left = 0;
    m_padding_top = 0;
    m_padding_right = 0;
    m_padding_bottom = 0;
}

ScrollListElement::~ScrollListElement()
{
    if (m_control != NULL)
    {
        delete m_control;
        m_control = NULL;
    }
}

wxPoint ScrollListElement::getPosition() const
{
    if (m_relative_to == NULL)
        return m_position;
    
    wxPoint relative_point = m_relative_to->getPosition();
    int x, y;
    
    x = relative_point.x + m_position.x;
    y = relative_point.y + m_position.y;
    
    if (m_relative_position == positionBelow)
    {
        if (m_relative_alignment == alignRight)
        {
            x += m_relative_to->getWidth();
            x += m_relative_to->getPaddingLeft();
            x += m_relative_to->getPaddingRight();
            x -= getWidth();
            x -= getPaddingLeft();
            x -= getPaddingRight();
        }
        
        if (m_relative_to->isVisible())
        {
            y += m_relative_to->getHeight();
            y += m_relative_to->getPaddingTop();
            y += m_relative_to->getPaddingBottom();
        }
    }
     else if (m_relative_position == positionOnLeft)
    {
        // for positionOnLeft, disregard alignRight or alignLeft,
        // since they don't make sense for this case
        x -= getWidth();
        x -= getPaddingLeft();
        x -= getPaddingRight();
    }
     else if (m_relative_position == positionOnRight)
    {
        // for positionOnRight, disregard alignRight or alignLeft,
        // since they don't make sense for this case
        x += m_relative_to->getWidth();
        x += m_relative_to->getPaddingLeft();
        x += m_relative_to->getPaddingRight();
    }
    
    return wxPoint(x, y);
}

void ScrollListElement::setAbsolutePosition(const wxPoint& position)
{
    m_position = position;
}

void ScrollListElement::setRelativePosition(ScrollListElement* relative_to,
                                            int position,
                                            int alignment)
{
    m_relative_to = relative_to;
    m_relative_position = position;
    m_relative_alignment = alignment;
}

void ScrollListElement::setPadding(int left, int top, int right, int bottom)
{
    m_padding_left = left;
    m_padding_top = top;
    m_padding_right = right;
    m_padding_bottom = bottom;
}

void ScrollListElement::setVisible(bool visible)
{
    if (m_control != NULL)
        m_control->Show(visible);

    m_visible = visible;
}


    
    
// ScrollListControl class implementation

BEGIN_EVENT_TABLE(ScrollListControl, wxScrolledWindow)
    EVT_PAINT(ScrollListControl::onPaint)
    EVT_SIZE(ScrollListControl::onSize)
    EVT_SCROLLWIN(ScrollListControl::onScroll)
    EVT_LEFT_DOWN(ScrollListControl::onMouseLeftClick)
    EVT_ERASE_BACKGROUND(ScrollListControl::onEraseBackground)
END_EVENT_TABLE()


ScrollListControl::ScrollListControl(wxWindow* parent,
                         wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style)
                           : wxScrolledWindow(parent,
                                              id,
                                              pos,
                                              size,
                                              style)
{
    // initialize variables
    m_bmp_width = 0;
    m_bmp_height = 0;
    m_scroll_pos = 0;
    m_virtual_height = 0;
    GetClientSize(&m_cli_width, &m_cli_height);

    SetBackgroundColour(*wxWHITE);
    
    // set the highlight color
    m_highlight_colour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    m_highlight_colour = kcl::stepColor(m_highlight_colour, 170); // same as toolbar
    
    // set our fonts
    m_font = *wxNORMAL_FONT;
    m_bold_font = *wxNORMAL_FONT;
    m_bold_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_overlay_font = wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false);

    // set initial properties for the control
    SetScrollRate(SCROLL_STEP, SCROLL_STEP);
    EnableScrolling(false , true);
    calcVirtualHeight();
    
    // allocate the bitmap for bit blitting
    allocBitmap(100, 100);
}

ScrollListControl::~ScrollListControl()
{
    std::vector<ScrollListItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        delete (*it);
        (*it) = NULL;
    }
}

void ScrollListControl::setOverlayText(const wxString& text)
{
    m_overlay_text = text;
}

void ScrollListControl::calcVirtualHeight()
{
    int total_height = 0;
    
    std::vector<ScrollListItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        ScrollListItem* item = (*it);
        if (item == NULL)
            continue;

        calcItemHeight(item);
        total_height += item->getHeight();
    }
    
    m_virtual_height = total_height;
    SetVirtualSize(-1, m_virtual_height);
}

void ScrollListControl::clearItems()
{
    std::vector<ScrollListItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        delete (*it);
        (*it) = NULL;
    }

    m_items.clear();
}

size_t ScrollListControl::getItemCount()
{
    return m_items.size();
}

int ScrollListControl::getItemIdx(ScrollListItem* item)
{
    int idx = 0;
    std::vector<ScrollListItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        if (item == (*it))
            return idx;
        
        idx++;
    }
    
    return -1;
}

ScrollListItem* ScrollListControl::getItem(size_t idx)
{
    if (idx >= m_items.size())
        return NULL;
        
    return m_items[idx];
}

void ScrollListControl::addItem(ScrollListItem* item)
{
    if (item == NULL)
        return;
    
    m_items.push_back(item);
}

void ScrollListControl::removeItem(ScrollListItem* item)
{
    if (item == NULL)
        return;
    
    std::vector<ScrollListItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        if (item == (*it))
        {
            delete (*it);
            (*it) = NULL;
            m_items.erase(it);
            return;
        }
    }
}

void ScrollListControl::refresh()
{
    wxASSERT_MSG(kl::thread::isMain(), wxT("Can't call this function from a thread, only from the gui/main thread!"));

    calcVirtualHeight();
    render();
    repaint();
}

void ScrollListControl::allocBitmap(int width, int height)
{
    if (width < 16)
        width = 16;

    if (height < 16)
        height = 16;

    if (width <= m_bmp_width && height <= m_bmp_height)
        return;

    // create a bitmap for our memdc with the same scaling factor as the screen
    wxClientDC dc(this);
    m_bmp.Create(width, height, dc);
    m_memdc.SelectObject(m_bmp);
    
    m_bmp_width = width;
    m_bmp_height = height;
}

void ScrollListControl::calcItemHeight(kcl::ScrollListItem* item)
{
    wxClientDC cdc(this);
    int w, h, x = 10000, y = 10000;
    int new_height = 0;
    
    // calculate the height of the text elements
    std::vector<ScrollListElement*>::iterator it;
    for (it = item->m_elements.begin(); it != item->m_elements.end(); ++it)
    {
        ScrollListElement* element = (*it);
        
        if (element->getBitmap().Ok())
            continue;
        
        if (element->getControl() != NULL)
            continue;
        
        w = m_cli_width - element->getPosition().x
                        - element->getPaddingLeft()
                        - element->getPaddingRight();
        
        if (element->isTextBold())
            cdc.SetFont(m_bold_font);
             else
            cdc.SetFont(m_font);

        wxSize finished_size;

        if (element->isTextWrap())
        {
            drawTextWrap(cdc,
                         element->getText(),
                         x, y, w, &finished_size);
        }
         else
        {
            drawTextEllipses(cdc,
                             element->getText(),
                             x, y, w, &finished_size,
                             element->getTextEllipses());
        }

        element->setSize(finished_size);

        if (element->isStretchable())
            element->setWidth(w);
    }

    // find the bottom-most element to calculate the item height
    for (it = item->m_elements.begin(); it != item->m_elements.end(); ++it)
    {
        ScrollListElement* element = (*it);

        if (!element->isVisible())
            continue;

        h = element->getPosition().y + element->getHeight()
                                     + element->getPaddingTop()
                                     + element->getPaddingBottom();

        if (h > new_height)
            new_height = h;
    }
    
    // update the item's height
    if (new_height != item->getHeight())
    {
        item->setHeight(new_height);
    }
}

void ScrollListControl::drawItem(kcl::ScrollListItem* item, int item_y)
{
    int x, y, w;
    int lpad, tpad, rpad, bpad;
    bool item_selected = item->isSelected();
    bool item_enabled = item->isEnabled();
    
    // convert the scrolled position to screen coordinates
    CalcScrolledPosition(0, item_y, &x, &y);
    
    // make sure the item's height is up-to-date
    calcItemHeight(item);
    
    // draw the item's background
    if (item_selected && item_enabled)
    {
        m_memdc.SetPen(m_highlight_colour);
        m_memdc.SetBrush(m_highlight_colour);
        m_memdc.DrawRectangle(0, y, m_cli_width, item->getHeight());
    }
     else
    {
        m_memdc.SetPen(*wxWHITE);
        m_memdc.SetBrush(*wxWHITE);
        m_memdc.DrawRectangle(0, y, m_cli_width, item->getHeight());
    }
    
    // draw the item's elements
    std::vector<ScrollListElement*>::iterator it;
    for (it = item->m_elements.begin(); it != item->m_elements.end(); ++it)
    {
        ScrollListElement* element = (*it);

        // get the element's stored position
        wxPoint pt = element->getPosition();
        
        if (pt.x < 0)
            pt.x = m_cli_width + pt.x;

        // get the element's stored padding
        lpad = element->getPaddingLeft();
        tpad = element->getPaddingTop();
        rpad = element->getPaddingRight();
        bpad = element->getPaddingBottom();
        
        // size and position controls
        
        wxControl* control = element->getControl();
        if (control != NULL)
        {
            // make sure the control's background color is correct
            bool control_is_button = control->IsKindOf(CLASSINFO(wxButton));
            wxColor bg_color = control->GetBackgroundColour();
            
            if (!control_is_button)
            {
                // set the background of the control
                if (item_enabled && item_selected)
                {
                    if (bg_color != m_highlight_colour)
                        control->SetBackgroundColour(m_highlight_colour);
                }
                 else
                {
                    if (bg_color != *wxWHITE)
                        control->SetBackgroundColour(*wxWHITE);
                }
                
                // if the item is disabled, disable the item's
                // control elements (except buttons)
                control->Enable(item_enabled);
                
                // refresh the control
                control->Refresh();
            }
            
            // size the control
            if (element->isStretchable())
            {
                w = m_cli_width-pt.x-lpad-rpad;
                element->setWidth(w);
                control->SetSize(w, element->getHeight());
            }
            
            // position the control
            wxPoint cur_pos = control->GetPosition();
            wxPoint new_pos(x+pt.x+lpad, y+pt.y+tpad);
            
            // we don't need to move the control, it's in the right place
            if (cur_pos == new_pos)
                continue;
            
            // if the control's position is going to be outside the
            // client area, hide it and don't worry about its position
            if (new_pos.x > m_cli_width || new_pos.y > m_cli_height)
            {
                control->Show(false);
                continue;
            }
            
            control->Show(element->isVisible());
            control->SetPosition(new_pos);
            continue;
        }
        
        // draw bitmaps
        
        if (element->getBitmap().Ok())
        {
            // size the bitmap
            if (element->isStretchable())
            {
                w = m_cli_width-pt.x-lpad-rpad;
                element->setWidth(w);
                element->getBitmap().SetWidth(w);
            }
            
            // draw the bitmap
            if (element->isVisible())
            {
                m_memdc.DrawBitmap(element->getBitmap(),
                                   x+pt.x+lpad, y+pt.y+tpad);
            }
            
            continue;
        }
        
        // draw text
        
        int text_x = x+pt.x+lpad;
        int text_y = y+pt.y+tpad;
        int text_w = m_cli_width-text_x-rpad;

        if (element->isTextBold())
        {
            m_memdc.SetFont(m_bold_font);
            m_hidden_memdc.SetFont(m_bold_font);
        }
         else
        {
            m_memdc.SetFont(m_font);
            m_hidden_memdc.SetFont(m_font);
        }

        wxSize finished_size;

        // NOTE: even if the element is hidden, we still
        //       need to draw the text on a hidden memory dc
        //       so that we can calculate the size of the element
        if (element->isTextWrap())
        {
            drawTextWrap(element->isVisible() ? m_memdc : m_hidden_memdc,
                         element->getText(),
                         text_x, text_y, text_w, &finished_size);
        }
         else
        {
            drawTextEllipses(element->isVisible() ? m_memdc : m_hidden_memdc,
                             element->getText(),
                             text_x, text_y, text_w, &finished_size,
                             element->getTextEllipses());
        }

        element->setSize(finished_size);
        
        // size the text
        if (element->isStretchable())
            element->setWidth(text_w);
    }
    
    // gray out the item if it's disabled
    if (!item_enabled)
    {        
        int srcx = 0;
        int srcy = y;
        int width = m_cli_width;
        int height = item->getHeight()-1;
        
        // steps: 1 - create a temporary bitmap the size of the item to lighten
        //        2 - blit from m_memdc to our temporary dc
        //        3 - lighten bitmap in temp dc
        //        4 - blit back to m_memdc
        
        wxBitmap item_bmp(width, height);
        wxMemoryDC item_memdc(item_bmp);

        item_memdc.Blit(0, 0, width, height, &m_memdc, srcx, srcy);
        lightenMemDC(item_memdc, item_bmp);
        m_memdc.Blit(srcx, srcy, m_cli_width, item->getHeight()-1, &item_memdc, 0, 0);
    }
    
    // draw the bottom line for this item
    int line_y = y+item->getHeight()-1;
    m_memdc.SetPen(*wxLIGHT_GREY_PEN);
    m_memdc.DrawLine(0, line_y, m_cli_width, line_y);
}

void ScrollListControl::drawItems()
{
    // set information for dc drawing
    m_memdc.SetPen(*wxLIGHT_GREY_PEN);
    m_memdc.SetTextForeground(*wxBLACK);

    int item_y = 0;
    std::vector<ScrollListItem*>::reverse_iterator it;
    for (it = m_items.rbegin(); it != m_items.rend(); ++it)
    {
        // draw the item
        kcl::ScrollListItem* item = (*it);
        drawItem(item, item_y);

        // increment the y-offset for the next item
        item_y += item->getHeight();
    }
}

void ScrollListControl::render()
{
    // allocate the bitmap to draw on
    allocBitmap(m_cli_width+50, m_cli_height+50);

    // draw background
    m_memdc.SetPen(*wxWHITE);
    m_memdc.SetBrush(*wxWHITE);
    m_memdc.DrawRectangle(-1, -1, m_bmp_width, m_bmp_height);
    
    // draw items
    drawItems();
    
    // draw overlay text
    drawOverlayText();
}

void ScrollListControl::drawOverlayText()
{
    if (m_overlay_text.Length() <= 0)
        return;
    
    // store the font we've been using and
    // set the font to the overlay font
    wxFont dc_font = m_memdc.GetFont();
    m_memdc.SetFont(m_overlay_font);
    
    wxColor dc_text_fg_color = m_memdc.GetTextForeground();
    m_memdc.SetTextForeground(*wxBLACK);
    
    wxArrayString arr;
    wxStringTokenizer tkz(m_overlay_text, wxT("\n"));
    while (tkz.HasMoreTokens())
        arr.Add(tkz.GetNextToken());
    
    int text_x, text_y, line_height;
    m_memdc.GetTextExtent(wxT("ABCDXHM"), &text_x, &line_height);
    
    line_height += 5;
    int line_count = arr.Count();
    int yoff = (m_cli_height/2) - ((line_height * line_count)/2);
    
    for (int i = 0; i < line_count; ++i)
    {
        m_memdc.GetTextExtent(arr.Item(i), &text_x, &text_y);
        m_memdc.DrawText(arr.Item(i), (m_cli_width-text_x)/2, yoff);
        yoff += line_height;
    }
    
    // restore the original font and text color the dc was using
    m_memdc.SetFont(dc_font);
    m_memdc.SetTextForeground(dc_text_fg_color);
}

void ScrollListControl::repaint(wxDC* dc)
{
    int x, y, w, h;

    x = 0;
    y = 0;
    w = m_cli_width;
    h = m_cli_height;

    if (!m_bmp.Ok())
    {
        if (dc)
        {
            // if there is no bitmap
            dc->SetBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
            dc->SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
            dc->DrawRectangle(x,y,w,h);
        }
         else
        {
            wxClientDC cdc(this);
            cdc.SetBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
            cdc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
            cdc.DrawRectangle(x,y,w,h);
        }
        return;
    }

    if (dc)
    {
        dc->Blit(x, y, w, h, &m_memdc, x, y);
    }
     else
    {
        wxClientDC cdc(this);
        cdc.Blit(x, y, w, h, &m_memdc, x, y);
    }
}

void ScrollListControl::onMouseLeftClick(wxMouseEvent& evt)
{
    int mouse_x = evt.GetX();
    int mouse_y = evt.GetY();
    
    // x and y offset based on the scrollbar location
    int x = 0, y = 0;

    int item_height = 0;
    int item_y = 0;
    
    ScrollListItem *old_item = NULL, *new_item = NULL;
    
    // determine the "old" selected item
    std::vector<ScrollListItem*>::reverse_iterator it;
    for (it = m_items.rbegin(); it != m_items.rend(); ++it)
    {
        if ((*it)->isSelected())
        {
            old_item = *it;
            break;
        }
    }
    
    for (it = m_items.rbegin(); it != m_items.rend(); ++it)
    {
        item_height = (*it)->getHeight();
        CalcScrolledPosition(0, item_y, &x, &y);
        
        if (mouse_y >= y && mouse_y < y+item_height)
        {
            // we left-clicked on an item that was already selected, we're done
            if ((*it) == old_item)
                return;
        }
        
        item_y += item_height;
    }
    
    // we left-clicked on an item that wasn't selected
    item_y = 0;
    for (it = m_items.rbegin(); it != m_items.rend(); ++it)
    {
        item_height = (*it)->getHeight();
        CalcScrolledPosition(0, item_y, &x, &y);
        
        // if the mouse is in the item's area, select it
        if (mouse_y >= y && mouse_y < y+item_height)
        {
            (*it)->setSelected(true);
            new_item = *it;
        }
         else
        {
            (*it)->setSelected(false);
        }
        
        item_y += item_height;
    }
    
    // fire a signal that the selected item has changed
    sigSelectedItemChanged(old_item, new_item);
    
    // if the signal isn't active, refresh the control
    if (!sigSelectedItemChanged.isActive())
        refresh();
}

void ScrollListControl::onScroll(wxScrollWinEvent& evt)
{
    wxEventType evt_type = evt.GetEventType();
    
    if (evt_type == wxEVT_SCROLLWIN_LINEUP)
        m_scroll_pos -= 10;
    if (evt_type == wxEVT_SCROLLWIN_LINEDOWN)
        m_scroll_pos += 10;
    if (evt_type == wxEVT_SCROLLWIN_PAGEUP)
        m_scroll_pos -= 50;
    if (evt_type == wxEVT_SCROLLWIN_PAGEDOWN)
        m_scroll_pos += 50;
    if (evt_type == wxEVT_SCROLLWIN_TOP)
        m_scroll_pos = 0;
    if (evt_type == wxEVT_SCROLLWIN_BOTTOM)
        m_scroll_pos = m_virtual_height-m_cli_height;
    if (evt_type == wxEVT_SCROLLWIN_THUMBTRACK)
        m_scroll_pos = evt.GetPosition();

    if (m_scroll_pos < 0)
        m_scroll_pos = 0;
        
    if (m_scroll_pos > (m_virtual_height-m_cli_height))
        m_scroll_pos = m_virtual_height-m_cli_height;
        
    Scroll(-1, m_scroll_pos);
    
    render();
    repaint();
}

void ScrollListControl::onPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    //DoPrepareDC(dc);
    repaint(&dc);
}

void ScrollListControl::onSize(wxSizeEvent& evt)
{
    // update our client size member variables
    GetClientSize(&m_cli_width, &m_cli_height);
    refresh();
}

void ScrollListControl::onDoRefresh(wxCommandEvent& evt)
{
    refresh();
}

void ScrollListControl::onEraseBackground(wxEraseEvent& evt)
{

}




} // namespace kcl


