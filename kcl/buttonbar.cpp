/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2006-08-21
 *
 */

#include "buttonbar.h"
#include <wx/tokenzr.h>
#include <kl/math.h>
#include "util.h"


namespace kcl
{


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


// drawing constants
const int DEFAULT_ITEM_PADDING_LEFT = 5;
const int DEFAULT_ITEM_PADDING_TOP = 5;
const int DEFAULT_ITEM_PADDING_RIGHT = 5;
const int DEFUALT_ITEM_PADDING_BOTTOM = 5;
const int DEFAULT_ITEM_MIN_WIDTH = 50;
const int DEFAULT_ITEM_MIN_HEIGHT = -1;
const int MULTILINE_TEXT_PADDING = 0;
const int MIDDLE_PADDING = 2;


BEGIN_EVENT_TABLE(ButtonBar, wxControl)
    EVT_ERASE_BACKGROUND(ButtonBar::onEraseBackground)
    EVT_PAINT(ButtonBar::onPaint)
    EVT_SIZE(ButtonBar::onSize)
    EVT_LEFT_DOWN(ButtonBar::onMouseLeftDown)
    EVT_LEFT_UP(ButtonBar::onMouseLeftUp)
END_EVENT_TABLE()



ButtonBar::ButtonBar(wxWindow* parent,
                     wxWindowID id,
                     const wxPoint& pos,
                     const wxSize& size)
                       : wxControl(parent,
                                   id,
                                   pos,
                                   (size==wxDefaultSize)?wxSize(100,50):size,
                                   wxNO_BORDER |
                                   wxNO_FULL_REPAINT_ON_RESIZE |
                                   wxCLIP_CHILDREN)
{
    // initialize variables
    m_last_selected_item = NULL;
    m_selected_item = NULL;
    m_multiline = false;
    m_space_evenly = false;
    m_uniform_size = false;
    m_mode = modeNormal;
    m_border_sides = borderBottom;
    m_background_gradient_direction = gradientNone;

    // set fonts and colors
    m_font = *wxNORMAL_FONT;
    m_highlight_color = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    m_border_color = kcl::getBorderColor();
    m_item_border_color = wxNullColour;
    m_start_color = *wxWHITE;
    m_end_color = *wxWHITE;
    
    m_item_padding_left = FromDIP(DEFAULT_ITEM_PADDING_LEFT);
    m_item_padding_top = FromDIP(DEFAULT_ITEM_PADDING_TOP);
    m_item_padding_right = FromDIP(DEFAULT_ITEM_PADDING_RIGHT);
    m_item_padding_bottom = FromDIP(DEFUALT_ITEM_PADDING_BOTTOM);
    m_item_min_width = FromDIP(DEFAULT_ITEM_MIN_WIDTH);
    m_item_min_height = FromDIP(DEFAULT_ITEM_MIN_HEIGHT);
    
    m_allitems_max_height = 0;
    m_bmp_width = 0;
    m_bmp_height = 0;
    
    GetClientSize(&m_cli_width, &m_cli_height);
    SetMinSize(wxSize(m_cli_width, m_cli_height));
    
    // allocate the bitmap for bit blitting
    allocBitmap(100, 100);
}

ButtonBar::~ButtonBar()
{
    std::vector<ButtonBarItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        delete (*it);
        (*it) = NULL;
    }
}

bool ButtonBar::Enable(bool enable)
{
    bool retval = wxWindow::Enable(enable);
    refresh();
    
    return retval;
}

void ButtonBar::refresh()
{
    render();
    repaint();
}

void ButtonBar::selectItem(int id, bool silent)
{
    // if 'silent' is true, don't fire notification events
    // that an item was selected
    
    ButtonBarItem* item = getItemById(id);
    _selectItem(item, silent);
}

bool ButtonBar::isItemSelected(int id)
{
    ButtonBarItem* item = getItemById(id);
    return _isItemSelected(item);
}

void ButtonBar::_selectItem(ButtonBarItem* item, bool silent)
{
    if (m_mode == modeNormal)
    {
        if (!item)
        {
            m_selected_item = NULL;
        }
         else
        {
            std::vector<ButtonBarItem*>::iterator it;
            for (it = m_items.begin(); it != m_items.end(); ++it)
            {
                if (item == (*it))
                {
                    m_selected_item = item;
                    break;
                }
            }
        }
    }
     else if (m_mode == modeToggle)
    {
        if (!item)
            return;
        
        bool found = false;
        std::vector<ButtonBarItem*>::iterator it;
        for (it = m_selected_items.begin(); it != m_selected_items.end(); ++it)
        {
            // if we can find the item in the selected items vector
            // then toggle it off (remove it from the vector)
            if (item == (*it))
            {
                m_selected_items.erase(it);
                found = true;
                break;
            }
        }
        
        // if we can't find the item in the selected items vector
        // then toggle it on (add it to the vector)
        if (found == false)
            m_selected_items.push_back(item);
    }
    
    render();
    repaint();
    
    if (item && !silent)
        SendClickEvent(item->id);
}

bool ButtonBar::_isItemSelected(const ButtonBarItem* item)
{
    if (!item)
        return false;
    
    if (m_mode == modeNormal)
    {
        if (item == m_selected_item)
            return true;
    }
     else if (m_mode == modeToggle)
    {
        bool found = false;
        std::vector<ButtonBarItem*>::iterator it;
        for (it = m_selected_items.begin(); it != m_selected_items.end(); ++it)
        {
            if (item == (*it))
                return true;
        }
    }

    return false;
}

ButtonBarItem* ButtonBar::addItem(int id,
                                  const wxBitmap& bitmap,
                                  const wxString& label)
{
    ButtonBarItem* item = new ButtonBarItem;
    item->id = id;
    item->bitmap = bitmap;
    item->label = label;
    
    // calculate the size of the item based on the text and bitmap
    calcItemSize(item);
    
    // add the item
    m_items.push_back(item);
    
    // update the height of all the items
    sizeAllItems();
    
    return item;
}

size_t ButtonBar::getItemCount()
{
    return m_items.size();
}

ButtonBarItem* ButtonBar::getItem(size_t idx)
{
    if (idx < 0 || idx >= m_items.size())
        return NULL;
        
    return m_items[idx];
}

void ButtonBar::sizeAllItems()
{
    int min_width = 0;
    int min_height = 0;

    int max_item_width = wxMax(m_item_min_width, 0);
    int max_item_height = wxMax(m_item_min_height, 0);

    std::vector<ButtonBarItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        calcItemSize(*it);

        if ((*it)->height > max_item_height)
            max_item_height = (*it)->height;
        if ((*it)->width > max_item_width)
            max_item_width = (*it)->width;
    }

    if (m_uniform_size)
    {
        for (it = m_items.begin(); it != m_items.end(); ++it)
        {
            (*it)->height = max_item_height;
            (*it)->width = max_item_width;
        }
    }


    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        // make sure all items have the same height
        if ((*it)->height < m_allitems_max_height)
            (*it)->height = m_allitems_max_height;

        // add each item to the min_width calculation
        min_width += (*it)->width;
        
        // items need to overlay by one pixel so items have borders,
        // the borders overlap
        if (it != m_items.begin())
            min_width -= 1;
    }

    // add in left and right borders if they exist
    if (m_border_sides & borderLeft)
        min_width += 1;
    if (m_border_sides & borderRight)
        min_width += 1;

    // set the min height to the max of all of the items' heights
    // and add in top and boottom borders if they exist
    min_height = m_allitems_max_height;
    if (m_border_sides & borderTop)
        min_height += 1;
    if (m_border_sides & borderBottom)
        min_height += 1;
    
    // set the minimum size for the control based on
    // the item height (for sizing in sizers)
    SetMinSize(wxSize(min_width, min_height));
}

void ButtonBar::setItemMinSize(int width, int height)
{
    if (width != -1)
        m_item_min_width = width;
    if (height != -1)
        m_item_min_height = height;
        
    sizeAllItems();
}

void ButtonBar::setItemPadding(int left, int top, int right, int bottom)
{
    if (left != -1)
        m_item_padding_left = left;
    if (top != -1)
        m_item_padding_top = top;
    if (right != -1)
        m_item_padding_right = right;
    if (bottom != -1)
        m_item_padding_bottom = bottom;

    sizeAllItems();
}

void ButtonBar::setItemBorderColor(const wxColor& color)
{
    m_item_border_color = color;
}

void ButtonBar::setMode(int mode)
{
    m_mode = mode;
}

void ButtonBar::setMultiLine(bool multiline)
{
    m_multiline = multiline;
}

void ButtonBar::setSpaceEvenly(bool space_evenly)
{
    m_space_evenly = space_evenly;
}

void ButtonBar::setUniformSize(bool uniform_size)
{
    m_uniform_size = uniform_size;
}

void ButtonBar::setGradientDirection(int direction)
{
    m_background_gradient_direction = direction;
}

void ButtonBar::setStartColor(const wxColor& color)
{
    m_start_color = color;
}

void ButtonBar::setEndColor(const wxColor& color)
{
    m_end_color = color;
}

void ButtonBar::setBorderColor(const wxColor& color)
{
    m_border_color = color;
}

void ButtonBar::setBorder(int border_sides)
{
    m_border_sides = border_sides;
}

void ButtonBar::allocBitmap(int width, int height)
{
    if (width < 16)
        width = 16;

    if (height < 16)
        height = 16;

    if (width <= m_bmp_width && height <= m_bmp_height)
        return;

    wxClientDC cdc(this);
    m_bmp.Create(width, height, cdc);
    m_memdc.SelectObject(m_bmp);

    m_bmp_width = width;
    m_bmp_height = height;
}

void ButtonBar::calcItemSize(ButtonBarItem* item)
{
    // calculate the size of the item
    int w, h, tw = 0, th = 0;
    
    wxClientDC cdc(this);
    wxDC* dc = m_memdc.IsOk() ? (wxDC*)&m_memdc : (wxDC*)&cdc;
    dc->SetFont(m_font);

    if (!m_multiline)
    {
        wxString label = item->label;
        label.Replace(wxT("\n"), wxT(" "), true);
        label.Trim(false);
        label.Trim();
        dc->GetTextExtent(label, &tw, &th);
    }
     else
    {
        // calculate the text size for the multiline-label item
        int lw = 0, lh = 0;
        
        wxStringTokenizer t(item->label, wxT("\n"));
        while (t.HasMoreTokens())
        {
            dc->GetTextExtent(t.GetNextToken(), &lw, &lh);
            
            // calculate the total text height
            th += lh;
            if (t.HasMoreTokens())
                th += MULTILINE_TEXT_PADDING;
            
            // the max line width is the text width
            if (lw > tw)
                tw = lw;
        }
    }
    
    w = item->bitmap.GetWidth();
    w = (w > tw) ? w : tw;
    w += m_item_padding_left;
    w += m_item_padding_right;
    
    // make sure the item is at least as wide as the minimum item width
    if (w < m_item_min_width)
        w = m_item_min_width;

    // interior items have a width that appears one less than other items
    // when a border is present, so this code fixes that
    if ((m_border_sides & borderLeft) || (m_border_sides & borderRight))
    {
        if (m_items.size() > 0)
        {
            if (item != *(m_items.begin()) && item != *(m_items.end()-1))
                w += 1;
        }
    }

    h = item->bitmap.GetHeight();
    h += m_item_padding_top;
    h += m_item_padding_bottom;
    h += th;

    // add some padding between the bitmap and the text if they both exist
    if (item->bitmap.Ok() && !item->label.IsEmpty())
        h += MIDDLE_PADDING;
    
    // make sure the item is at least as tall as the minimum item height
    if (h < m_item_min_height)
        h = m_item_min_height;
    
    // update our max item height
    if (h > m_allitems_max_height)
        m_allitems_max_height = h;
    
    // set the size of the item
    item->width = w;
    item->height = h;
}

void ButtonBar::drawItem(ButtonBarItem* item, int x, int y)
{
    // there are no items to draw
    if (m_items.size() == 0)
        return;
    
    int item_w;
    int bmp_x, bmp_y, bmp_w, bmp_h;
    int text_x, text_y, text_w, text_h;
    
    // set these parameters based on where the item is drawn
    item->x = x;
    item->y = y;
    
    // calculate bitmap and text position
    item_w = item->width;
    
    bmp_w = item->bitmap.GetWidth();
    bmp_h = item->bitmap.GetHeight();
    bmp_x = x+(item_w/2)-(bmp_w/2);
    bmp_y = y+m_item_padding_top;
    
    m_memdc.GetTextExtent(item->label, &text_w, &text_h);
    text_x = x+(item_w/2)-(text_w/2);
    
    if (item->bitmap.IsOk())
    {
        text_y = m_item_padding_top+bmp_h+MIDDLE_PADDING;
    }
     else
    {
       // if (!m_multiline)
            text_y = (m_allitems_max_height-text_h)/2;
    }

    bool selected_item_found = false;
    if (m_mode == modeToggle)
    {
        std::vector<ButtonBarItem*>::iterator it;
        for (it = m_selected_items.begin(); it != m_selected_items.end(); ++it)
        {
            if (item == (*it))
                selected_item_found = true;
        }
    }
    
    // if the item is the selected item, draw the selection background
    if ((m_mode == modeNormal && item == m_selected_item) ||
        (m_mode == modeToggle && selected_item_found))
    {
        wxColor light_highlight_color = 
                kcl::stepColor(m_highlight_color, 170); // same as toolbar

        m_memdc.SetPen(light_highlight_color);
        m_memdc.SetBrush(light_highlight_color);

        // draw the selection rectangle
        wxRect r(item->x, item->y, item->width, item->height);
        m_memdc.DrawRectangle(r);
        
        // draw darker left and right border lines
        if (m_item_border_color.IsOk())
        {
            int x = item->x, y = item->y-1, w = item->width, h = item->height+2;
            
            if (item == *(m_items.begin()))
            {
                x -= 1;
                w += 1;
            }
            
            if (item == *(m_items.end()-1))
                w += 1;
            
            m_memdc.SetPen(m_item_border_color);
            m_memdc.SetBrush(*wxTRANSPARENT_BRUSH);
            m_memdc.DrawRectangle(x,y,w,h);
        }
        
        // set the brush and pen colors back to what they were
        m_memdc.SetBrush(*wxWHITE);
        m_memdc.SetPen(wxColour(192,192,192));
    }

    // draw the bitmap
    if (item->bitmap.IsOk())
        m_memdc.DrawBitmap(item->bitmap, bmp_x, bmp_y, true);
    
    // draw the text
    
    if (!m_multiline)
    {
        wxString label = item->label;
        label.Replace(wxT("\n"), wxT(" "), true);
        label.Trim(false);
        label.Trim();
        m_memdc.DrawText(label, text_x+1, text_y);
    }
     else
    {
        // draw each line of the label for the multiline-label item
        int lw = 0, lh = 0;
        
        wxStringTokenizer t(item->label, wxT("\n"));
        while (t.HasMoreTokens())
        {
            wxString line = t.GetNextToken();
            line.Trim(false);
            line.Trim();
            m_memdc.GetTextExtent(line, &text_w, &text_h);

            // draw the line of text
            text_x = x+(item_w/2)-(text_w/2);
            m_memdc.DrawText(line, text_x+1, text_y);
            
            // move down to draw the next line of text
            text_y += text_h;
            text_y += MULTILINE_TEXT_PADDING;
        }
    }
}

void ButtonBar::render()
{
    // allocate the bitmap to draw on
    allocBitmap(m_cli_width+50, m_cli_height+50);

    // draw background
    m_memdc.SetPen(m_start_color);
    m_memdc.SetBrush(m_start_color);
    
    if (m_background_gradient_direction == gradientNone)
    {
        m_memdc.DrawRectangle(-1, -1, m_bmp_width, m_bmp_height);
    }
     else
    {
        m_memdc.GradientFillLinear(wxRect(0, 0, m_cli_width, m_cli_height),
                                   m_start_color, m_end_color, wxSOUTH);
    }
    
    // draw the items
    m_memdc.SetTextForeground(*wxBLACK);
    m_memdc.SetFont(m_font);

    // calculate spacer size if we're spacing the items evenly
    int space_w = m_cli_width;
    std::vector<ButtonBarItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        space_w -= (*it)->width;
    }
    if (space_w < 0)
    {
        space_w = 0;
    }
    space_w /= (m_items.size()+1);
    
    // draw border
    m_memdc.SetPen(m_border_color);
    
    if (m_border_sides & borderLeft)
        m_memdc.DrawLine(0, 0, 0, m_cli_height);
    if (m_border_sides & borderRight)
        m_memdc.DrawLine(m_cli_width-1, 0, m_cli_width-1, m_cli_height);
    if (m_border_sides & borderTop)
        m_memdc.DrawLine(0, 0, m_cli_width, 0);
    if (m_border_sides & borderBottom)
        m_memdc.DrawLine(0, m_cli_height-1, m_cli_width, m_cli_height-1);

    int x = 0, y = 0;
    
    // take into account the borders, if they exists
    if (m_border_sides & borderLeft)
        x += 1;
    if (m_border_sides & borderTop)
        y += 1;
    
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        // if we're spacing the items evenly, add the spacer
        if (m_space_evenly)
            x += space_w;

        drawItem(*it, x, y);
        
        // increment our position
        x += (*it)->width;
        
        // make sure the item borders overlap (this prevents
        // the ugly "double border" problem)
        x -= 1;
    }

    // if the buttonbar has been disabled, lighten it
    if (!IsEnabled())
    {
        wxBitmap temp_bmp(m_bmp_width, m_bmp_height);
        wxMemoryDC temp_memdc(temp_bmp);

        temp_memdc.Blit(0, 0, m_bmp_width, m_bmp_height, &m_memdc, 0, 0);
        lightenMemDC(temp_memdc, temp_bmp);
        m_memdc.Blit(0, 0, m_bmp_width, m_bmp_height, &temp_memdc, 0, 0);
    }
}

void ButtonBar::repaint(wxDC* dc)
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

bool ButtonBar::SendClickEvent(wxWindowID id)
{
    wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, id);
    evt.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(evt);
}

ButtonBarItem* ButtonBar::getItemById(int id)
{
    std::vector<ButtonBarItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        if ((*it)->id == id)
            return (*it);
    }
    
    return NULL;
}

ButtonBarItem* ButtonBar::getItemByPosition(wxPoint& pt)
{
    std::vector<ButtonBarItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        wxRect rect((*it)->x, (*it)->y, (*it)->width, (*it)->height);
        if (rect.Contains(pt))
            return (*it);
    }
    
    return NULL;
}

void ButtonBar::onMouseLeftDown(wxMouseEvent& evt)
{
    wxPoint pt = evt.GetPosition();
    
    // store the last selected item in case
    // the user clicks on an invalid area
    m_last_selected_item = m_selected_item;
    m_selected_item = getItemByPosition(pt);
}

void ButtonBar::onMouseLeftUp(wxMouseEvent& evt)
{
    wxPoint pt = evt.GetPosition();
    ButtonBarItem* item = getItemByPosition(pt);

    // we clicked down on an area that was not a button
    if (m_selected_item == NULL)
    {
        if (m_mode == modeNormal)
        {
            m_selected_item = m_last_selected_item;
            m_last_selected_item = NULL;
        }
        return;
    }
    
    // if the mouse clicks up in the same rectangle that it was
    // clicked down, then we should try to fire an item click event
    if (item == m_selected_item)
    {
        _selectItem(item);
        return;
    }
    
    if (m_mode == modeNormal)
    {
        // the user moved out of the area they originally clicked down in,
        // so we need to reset the selection rectangle to what it was
        m_selected_item = m_last_selected_item;
        m_last_selected_item = NULL;
    }
}

void ButtonBar::onEraseBackground(wxEraseEvent& evt)
{
    // do nothing
}

void ButtonBar::onPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    repaint(&dc);
}

void ButtonBar::onSize(wxSizeEvent& evt)
{
    // update our client size member variables
    GetClientSize(&m_cli_width, &m_cli_height);
    
    render();
    repaint();
}


}; // namespace kcl


