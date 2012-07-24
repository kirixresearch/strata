/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2007-11-14
 *
 */


#include "bitmapcombo.h"
#include <wx/settings.h>
#include <wx/imaglist.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>


// -- vista non-hover dropdown button --

/* XPM */
static const char* xpm_vistadropdown_button[] = {
"17 19 2 1",
"x c #000000",
"  c None",
"                 ",
"                 ",
"                 ",
"                 ",
"                 ",
"                 ",
"                 ",
"                 ",
"     xxxxxxx     ",
"      xxxxx      ",
"       xxx       ",
"        x        ",
"                 ",
"                 ",
"                 ",
"                 ",
"                 ",
"                 ",
"                 "
};




// -- event definitions --

DEFINE_EVENT_TYPE(wxEVT_KCLBITMAPCOMBO_BEGIN_DRAG)


// total width before the text control begins is equal to
// BMP_INDENT + width of the bitmap + BMP_PADDING
const int CTRL_PADDING = 3;
const int BMP_PADDING = 5;
const int CUSTOM_PAINT_WIDTH = 19;


namespace kcl
{

wxCursor g_ibeam_cursor;


static void autoSizeListHeader(wxListCtrl* ctrl)
{
    int cli_width, cli_height;
    ctrl->GetClientSize(&cli_width, &cli_height);
    if (ctrl->GetColumnCount() > 0)
    {
        // leave space for vscroll bar
        ctrl->SetColumnWidth(0, cli_width-35);
    }
}


// -- BitmapComboPopup class implementation --

BEGIN_EVENT_TABLE(BitmapComboPopup, wxListCtrl)
    EVT_LIST_ITEM_SELECTED(7523, BitmapComboPopup::onItemSelected)
    EVT_MOTION(BitmapComboPopup::onMouseMove)
    EVT_SIZE(BitmapComboPopup::onSize)
END_EVENT_TABLE()

void BitmapComboPopup::Init()
{
    // initialize member variables
    m_selection_idx = 0;
    m_absolute_max_popup_height = 180;
    m_options = BitmapComboPopup::ShowItemBitmap;
}

bool BitmapComboPopup::Create(wxWindow* parent)
{
    // create popup control
    if (!wxListCtrl::Create(parent,
                            7523,
                            wxPoint(0,0),
                            wxDefaultSize,
                            wxLC_REPORT |
                            wxLC_NO_HEADER |
                            wxLC_SINGLE_SEL))
    {
        return false;
    }
                                     
    if (m_options & BitmapComboPopup::ShowItemBitmap)
    {
        wxImageList* list = new wxImageList;
        list->Create(16, 16, true, 0);
        AssignImageList(list, wxIMAGE_LIST_SMALL);
    }
    
    InsertColumn(0, wxT(""), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);

    autoSizeListHeader(this);
    
    //m_combo->SetPopupMaxHeight(21);
    return true;
}

wxWindow* BitmapComboPopup::GetControl()
{
    // return pointer to the created control
    return this;
}

void BitmapComboPopup::setOptions(int options)
{
    m_options = options;
}

void BitmapComboPopup::setDefaultBitmap(const wxBitmap& bitmap)
{
    m_default_bmp = bitmap;
}

void BitmapComboPopup::setItemBitmap(int idx, const wxBitmap& bitmap)
{
    if (m_options & BitmapComboPopup::ShowItemBitmap)
    {
        int count = GetItemCount();
        if (idx >= 0 && idx < count)
        {
            int imagelist_idx = -1;
            
            // lookup the bitmap in the imagelist
            wxImageList* list = GetImageList(wxIMAGE_LIST_SMALL);
            int i, img_list_count = list->GetImageCount();
            for (i = 0; i < img_list_count; ++i)
            {
                if (bitmap.IsSameAs(list->GetBitmap(i)))
                {
                    imagelist_idx = i;
                    break;
                }
            }
            
            // we couldn't find the bitmap in the imagelist, so add it
            if (imagelist_idx == -1)
            {
                imagelist_idx = list->Add(bitmap);
                m_bitmaps[imagelist_idx] = bitmap;
            }

            SetItemImage(idx, imagelist_idx);
        }
    }
}

wxBitmap BitmapComboPopup::getItemBitmap(int idx)
{
    if (m_options & BitmapComboPopup::ShowItemBitmap)
    {
        int count = GetItemCount();
        if (idx >= 0 && idx < count)
        {
            wxListItem item;
            item.SetMask(wxLIST_MASK_IMAGE);
            item.SetId(idx);
            item.SetColumn(0);
            GetItem(item);
            
            int image_idx = item.GetImage();
            if (image_idx >= 0 && image_idx < (int)m_bitmaps.size())
                return m_bitmaps[image_idx];
        }
    }
    
    return wxNullBitmap;
}

wxBitmap BitmapComboPopup::getBitmap()
{
    return getItemBitmap(m_selection_idx);
}

void BitmapComboPopup::SetStringValue(const wxString& s)
{
    // translate string into a list selection
    long idx = wxListCtrl::FindItem(-1, s);
    
    int count = GetItemCount();
    if (idx >= 0 && idx < count)
        m_selection_idx = idx;
    
    if (idx == -1)
        m_selection_idx = -1;
}

wxString BitmapComboPopup::GetStringValue() const
{
    // get list selection as a string
    int count = GetItemCount();
    if (m_selection_idx >= 0 && m_selection_idx < count)
        return GetItemText(m_selection_idx);
    
    return wxEmptyString;
}

void BitmapComboPopup::insert(const wxString& label,
                              const wxBitmap& bitmap,
                              int idx)
{
    int imagelist_idx = -1;
    
    if (m_options & BitmapComboPopup::ShowItemBitmap)
    {
        // lookup the bitmap in the imagelist
        wxImageList* list = GetImageList(wxIMAGE_LIST_SMALL);
        int i, count = list->GetImageCount();
        for (i = 0; i < count; ++i)
        {
            if (bitmap.IsSameAs(list->GetBitmap(i)))
            {
                imagelist_idx = i;
                break;
            }
        }
        
        // we couldn't find the bitmap in the imagelist, so add it
        if (imagelist_idx == -1)
        {
            imagelist_idx = list->Add(bitmap);
            m_bitmaps[imagelist_idx] = bitmap;
        }
    }
    
    // add the list item to the list control
    long item_idx = InsertItem(idx, wxEmptyString);
    SetItem(item_idx, 0, label, imagelist_idx);
    
    // adjust the popup height based on how many items are in the list control
    int item_count = GetItemCount();
    if (item_count < 1)
        return;
    
    wxRect rect;
    GetItemRect(0, rect);
    
    // top and bottom padding are 2 pixels
#ifdef __WXMSW__
    int new_popup_height = 2+(item_count*rect.height)+2;
#else
    int new_popup_height = 4+(item_count*rect.height)+4;
#endif

    if (new_popup_height < m_absolute_max_popup_height)
        m_combo->SetPopupMaxHeight(new_popup_height);
         else
        m_combo->SetPopupMaxHeight(m_absolute_max_popup_height);
}

int BitmapComboPopup::getSelection() const
{
    return m_selection_idx;
}

void BitmapComboPopup::PaintComboControl(wxDC& dc, const wxRect& rect)
{
    wxColor c = m_combo->GetBackgroundColour();
    dc.SetPen(c);
    dc.SetBrush(c);
    dc.DrawRectangle(rect);
    
    wxWindow* focus_wnd = wxWindow::FindFocus();
    wxTextCtrl* textctrl = m_combo->GetTextCtrl();
    textctrl->SetBackgroundColour(c);
    
    // if the combo control doesn't have the focus and
    // we've set some overlay text, draw it now
    BitmapComboControl* combo = (BitmapComboControl*)m_combo;
    wxString overlay_text = combo->getOverlayText();
    
    bool draw_overlay_text = true;
    if (overlay_text.IsEmpty())
        draw_overlay_text = false;
         else if (focus_wnd == m_combo || focus_wnd == textctrl)
        draw_overlay_text = false;
    
    if (draw_overlay_text)
    {
        int text_w, text_h;
        dc.GetTextExtent(overlay_text, &text_w, &text_h);
        
        int text_x = rect.x + BMP_PADDING;
        int text_y = rect.y+(rect.height-text_h-1)/2;
        
        // draw the overlay text
        dc.SetTextForeground(*wxLIGHT_GREY);
        dc.DrawText(overlay_text, text_x, text_y);
    }
     else
    {
        wxBitmap bmp = getBitmap();
        
        // it we don't have a valid bitmap to draw,
        // try drawing the default bitmap
        if (!bmp.IsOk())
            bmp = m_default_bmp;
        
        // we still don't have a valid bitmap to draw, we're done
        if (!bmp.IsOk())
            return;
        
        // draw the bitmap
        int bmp_x = rect.x+CTRL_PADDING;
        int bmp_y = rect.y+(rect.height-bmp.GetHeight())/2;
        dc.DrawBitmap(bmp, bmp_x, bmp_y, true);
    }
}

void BitmapComboPopup::onItemSelected(wxListEvent& evt)
{
    m_selection_idx = evt.GetIndex();
    
    wxString val = GetItemText(evt.GetIndex());
    m_combo->SetValue(val);
    
    // send selection event
    wxCommandEvent e(wxEVT_COMMAND_COMBOBOX_SELECTED,
                     m_combo->GetId());
    e.SetInt(evt.GetIndex());
    e.SetString(val);
    ProcessEvent(e);
    
    // unselect and unhighlight the item in the list
    SetItemState((long)m_selection_idx, ~wxLIST_STATE_SELECTED &
                                        ~wxLIST_STATE_DROPHILITED,
                                        wxLIST_STATE_SELECTED |
                                        wxLIST_STATE_DROPHILITED);
    Dismiss();
}

void BitmapComboPopup::onMouseMove(wxMouseEvent& evt)
{
    int flags;
    long item = HitTest(evt.GetPosition(), flags);
    
    int i, count = GetItemCount();
    for (i = 0; i < count; ++i)
    {
        if (i != item)
            SetItemState((long)i, ~wxLIST_STATE_DROPHILITED, wxLIST_STATE_DROPHILITED);
    }
    
    if (flags & wxLIST_HITTEST_ONITEM)
        SetItemState(item, wxLIST_STATE_DROPHILITED, wxLIST_STATE_DROPHILITED);
}

void BitmapComboPopup::onSize(wxSizeEvent& evt)
{
    autoSizeListHeader(this);
    evt.Skip();
}




// -- BitmapComboControl class implementation --

BEGIN_EVENT_TABLE(BitmapComboControl, wxComboCtrl)
    EVT_LEFT_DOWN(BitmapComboControl::onMouseLeftDown)
    EVT_LEFT_UP(BitmapComboControl::onMouseLeftUp)
    EVT_MOTION(BitmapComboControl::onMouseMove)
    EVT_SET_FOCUS(BitmapComboControl::onSetFocus)
    EVT_SIZE(BitmapComboControl::onSize)
END_EVENT_TABLE()

BitmapComboControl::BitmapComboControl(wxWindow* parent,
                                       wxWindowID id,
                                       BitmapComboPopup* popup)
                        : wxComboCtrl(parent,
                                      id,
                                      wxEmptyString,
                                      wxDefaultPosition,
                                      wxDefaultSize,
                                      wxCC_STD_BUTTON | wxTE_PROCESS_ENTER)
{
    if (popup)
        m_popup = popup;
         else
        m_popup = new BitmapComboPopup;
    
    m_show_bitmap = false;
    m_cli_width = 0;
    m_cli_height = 0;
    m_overlay_text = wxEmptyString;
    m_drag_x_threshold = wxSystemSettings::GetMetric(wxSYS_DRAG_X);
    m_drag_y_threshold = wxSystemSettings::GetMetric(wxSYS_DRAG_Y);
    m_action_pos = wxPoint(-1,-1);
    m_overlay_text_shown = false;
    m_single_click_select = false;
    m_show_bitmap = true;
    m_had_focus = false;
    m_dragging = false;

    SetPopupControl(m_popup);
    SetPopupMinWidth(160);
    SetCustomPaintWidth(CUSTOM_PAINT_WIDTH);
    
    // custom dropdown button drawing for vista (since wxComboCtrl is not
    // a native control, it doesn't look the same as wxComboBox on Vista)
    if ((wxPlatformInfo::Get().GetOperatingSystemId() & wxOS_WINDOWS) &&
        wxPlatformInfo::Get().GetOSMajorVersion() >= 6 /* Windows Vista */)
    {
        wxRect rect(0,0,17,19);
        wxBitmap normal_bmp(17,19);
        wxBitmap hover_bmp(17,19);
        wxBitmap pressed_bmp(17,19);
        
        wxMemoryDC memdc;
        memdc.SelectObject(normal_bmp);
        wxRendererNative::Get().DrawComboBoxDropButton(this, memdc, rect, 0);
        memdc.SelectObject(hover_bmp);
        wxRendererNative::Get().DrawComboBoxDropButton(this, memdc, rect, wxCONTROL_CURRENT);
        memdc.SelectObject(pressed_bmp);
        wxRendererNative::Get().DrawComboBoxDropButton(this, memdc, rect, wxCONTROL_PRESSED);
        
        // replace the normal combo dropdown bitmap with the vist dropdown bitmap
        normal_bmp = wxBitmap(xpm_vistadropdown_button);
        SetButtonBitmaps(normal_bmp, false, pressed_bmp, hover_bmp);
    }

    
    wxTextCtrl* textctrl = GetTextCtrl();
    textctrl->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(BitmapComboControl::onMouseLeftDown), NULL, this);
    textctrl->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(BitmapComboControl::onMouseLeftUp), NULL, this);
    textctrl->Connect(wxEVT_MOTION, wxMouseEventHandler(BitmapComboControl::onMouseMove), NULL, this);
    textctrl->Connect(wxEVT_SET_FOCUS, wxFocusEventHandler(BitmapComboControl::onSetFocus), NULL, this);
    textctrl->Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(BitmapComboControl::onKillFocus), NULL, this);
}

void BitmapComboControl::setOverlayText(const wxString& val)
{
    m_overlay_text = val;
    showOverlayText((val.Length() > 0) ? true : false);
}

wxString BitmapComboControl::getOverlayText() const
{
    return m_overlay_text;
}

void BitmapComboControl::recalcCustomPaintWidth()
{
    if (isOverlayTextShown())
    {
        int button_width = GetButtonSize().GetWidth();
        int paint_width = m_cli_width-button_width-(CTRL_PADDING*2);
        if (paint_width >= 0 && GetCustomPaintWidth() != paint_width)
            SetCustomPaintWidth(paint_width);
    }
     else
    {
        int paint_width = (m_show_bitmap ? CUSTOM_PAINT_WIDTH : 0);
        if (paint_width >= 0 && GetCustomPaintWidth() != paint_width)
            SetCustomPaintWidth(paint_width);
    }
}

void BitmapComboControl::showOverlayText(bool show)
{
    m_overlay_text_shown = show;
    recalcCustomPaintWidth();
}

bool BitmapComboControl::isOverlayTextShown() const
{
    return m_overlay_text_shown;
}

void BitmapComboControl::setSingleClickSelect(bool select)
{
    m_single_click_select = select;
}

void BitmapComboControl::setDefaultBitmap(const wxBitmap& bitmap)
{
    m_popup->setDefaultBitmap(bitmap);
}

void BitmapComboControl::setShowBitmap(bool show)
{
    m_show_bitmap = show;
    SetCustomPaintWidth(m_show_bitmap ? CUSTOM_PAINT_WIDTH : 0);
}

wxBitmap BitmapComboControl::getItemBitmap(size_t idx) const
{
    return m_popup->getItemBitmap(idx);
}

wxBitmap BitmapComboControl::getBitmap() const
{
    return m_popup->getBitmap();
}

void BitmapComboControl::setItemBitmap(size_t idx, const wxBitmap& bitmap)
{
    m_popup->setItemBitmap(idx, bitmap);
    Refresh(false);
}

void BitmapComboControl::Insert(const wxString& label,
                                const wxBitmap& bitmap,
                                size_t idx)
{
    m_popup->insert(label, bitmap, idx);
}

int BitmapComboControl::FindString(const wxString& string,
                                   bool case_sensitive)
{
    return ((wxListCtrl*)m_popup)->FindItem(-1, string);
}

int BitmapComboControl::GetSelection() const
{
    return m_popup->getSelection();
}

void BitmapComboControl::SetValue(const wxString& value)
{
    wxComboCtrl::SetValue(value);

    // if we have overlay text, hide it
    if (value.Length() > 0 && isOverlayTextShown())
        showOverlayText(false);
    
    // if we don't have any overlay text, show it
    if (value.IsEmpty() && !isOverlayTextShown())
        showOverlayText(true);
}

wxString BitmapComboControl::GetValue() const
{
    return wxComboCtrl::GetValue();
}

void BitmapComboControl::onMouseLeftDown(wxMouseEvent& evt)
{
    int x,y;
    ::wxGetMousePosition(&x,&y);
    ScreenToClient(&x,&y);
    
    wxWindow* focus_wnd = wxWindow::FindFocus();
    m_had_focus = (focus_wnd == this ||
                   focus_wnd == GetTextCtrl() ? true : false);
    m_action_pos = wxPoint(x,y);
    m_dragging = false;
    
    if (isOverlayTextShown())
        showOverlayText(false);
    evt.Skip();
}

void BitmapComboControl::onMouseLeftUp(wxMouseEvent& evt)
{
    int x,y;
    ::wxGetMousePosition(&x,&y);
    ScreenToClient(&x,&y);
    
    // if we single-clicked on the text area of the combo control
    // and the flag is set, select all of the text
    if (m_single_click_select && !m_had_focus &&
        abs(m_action_pos.x-x) <= m_drag_x_threshold)
    {
        GetTextCtrl()->SetSelection(-1,-1);
    }
    
    if (m_dragging)
    {
        // reset drag and drop member variables
        m_dragging = false;
        m_action_pos = wxPoint(-1,-1);
    }
    
    evt.Skip();
}

void BitmapComboControl::onMouseMove(wxMouseEvent& evt)
{
    int x,y;
    ::wxGetMousePosition(&x,&y);
    ScreenToClient(&x,&y);
    
    if (!m_dragging &&
        m_action_pos.x <= GetCustomPaintWidth() &&
        m_action_pos != wxPoint(-1,-1) &&
        (abs(x - m_action_pos.x) + abs(y - m_action_pos.y) > 5))
    {
        // if we've captured the mouse, release it
        if (wxWindow::GetCapture() == this)
            ReleaseMouse();
        
        // revert back to default cursor
        SetCursor(wxNullCursor);
        
        // start a drag event
        m_dragging = true;
        
        BitmapComboEvent e(wxEVT_KCLBITMAPCOMBO_BEGIN_DRAG, GetId());
        e.SetEventObject(this);
        ProcessEvent(e);
        return;
    }
    
    // draw ibeam cursor when hovering over the text control area
    // (this code is necessary because the custom paint area of
    // the combo control is changed in order to draw the overlay
    // text and when the custom paint area is changed, the text
    // control is moved to accomodate the paint area)

    int button_width = GetButtonSize().GetWidth();
    wxRect ibeam_rect = wxRect(CTRL_PADDING,
                               CTRL_PADDING,
                               m_cli_width-button_width-(CTRL_PADDING*2),
                               m_cli_height-(CTRL_PADDING*2));
    wxBitmap bmp = getBitmap();
    if (bmp.IsOk())
    {
        ibeam_rect.x += (bmp.GetWidth()+BMP_PADDING);
        ibeam_rect.width -= ibeam_rect.x;
    }
     else if (m_show_bitmap)
    {
        // forcing a bitmap to be shown; take this into account
        ibeam_rect.x += (16+BMP_PADDING);
        ibeam_rect.width -= ibeam_rect.x;
    }
    
    if (ibeam_rect.Contains(wxPoint(x,y)))
    {
        if (!g_ibeam_cursor.IsOk())
            g_ibeam_cursor = wxCursor(wxCURSOR_IBEAM);

        SetCursor(g_ibeam_cursor);
    }
     else
    {
        SetCursor(wxNullCursor);
    }
    
    evt.Skip();
}

void BitmapComboControl::onSetFocus(wxFocusEvent& evt)
{
    if (isOverlayTextShown())
        showOverlayText(false);
    evt.Skip();
}

void BitmapComboControl::onKillFocus(wxFocusEvent& evt)
{
    if (!isOverlayTextShown() &&
        m_overlay_text.Length() > 0 &&
        GetValue().IsEmpty())
    {
        showOverlayText(true);
    }
    
    evt.Skip();
}

void BitmapComboControl::onSize(wxSizeEvent& evt)
{
    GetClientSize(&m_cli_width, &m_cli_height);
    recalcCustomPaintWidth();
    evt.Skip();
}




};  // namespace kcl




