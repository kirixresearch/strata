/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2006-08-21
 *
 */


#ifndef __KCL_BUTTONBAR_H
#define __KCL_BUTTONBAR_H


#include <wx/wx.h>
#include <vector>


namespace kcl
{


class ButtonBarItem
{
public:

    ButtonBarItem()
    {
        bitmap = wxNullBitmap;
        label = wxEmptyString;
        id = wxID_ANY;
        x = 0;
        y = 0;
        height = 40;
        width = 40;
    }
    
    ButtonBarItem(const wxBitmap& _bitmap,
                  const wxString& _label,
                  wxWindowID _id)
    {
        bitmap = _bitmap;
        label = _label;
        id = _id;
        x = 0;
        y = 0;
        height = 40;
        width = 40;
    }

public:

    wxBitmap bitmap;
    wxString label;
    wxWindowID id;
    int x;
    int y;
    int height;
    int width;
};


class ButtonBar : public wxControl
{
public:
    
    enum
    {
        modeNormal = 0,
        modeToggle
    };
    
    enum
    {
        gradientNone = 0,
        gradientVertical
    };
    
    enum
    {
        borderNone = 0,
        borderLeft = 0x01,
        borderRight = 0x02,
        borderTop = 0x04,
        borderBottom = 0x08,
        borderAll = borderLeft | borderRight | borderTop | borderBottom
    };

    ButtonBar(wxWindow* parent,
              wxWindowID id = -1,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize);
    ~ButtonBar();

    void refresh();

    void selectItem(int id, bool silent = false);
    bool isItemSelected(int id);

    ButtonBarItem* addItem(int id,
                           const wxBitmap& bitmap,
                           const wxString& label = wxEmptyString);
    ButtonBarItem* getItem(size_t idx);
    size_t getItemCount();
    
    void setMode(int mode);
    void setMultiLine(bool multiline);
    void setSpaceEvenly(bool space_evenly);
    void setGradientDirection(int direction);
    void setStartColor(const wxColor& color);
    void setEndColor(const wxColor& color);
    void setBorderColor(const wxColor& color);
    void setBorder(int border_sides);

    void setItemMinSize(int width, int height);
    void setItemPadding(int left, int top, int right, int bottom);
    void setItemBorderColor(const wxColor& color);

    bool Enable(bool state = true);
    
private:

    // this window does not accept focus, and should
    // not be included in tab traversal lists
    bool AcceptsFocus() const { return FALSE; }
    
    void allocBitmap(int width, int height);
    void drawItem(ButtonBarItem* item, int x, int y);
    void calcItemSize(ButtonBarItem* item);
    void sizeAllItems();
    
    void _selectItem(ButtonBarItem* item, bool silent = false);
    bool _isItemSelected(ButtonBarItem* item);
    ButtonBarItem* getItemById(int id);
    ButtonBarItem* getItemByPosition(wxPoint& pt);
    
    void render();
    void repaint(wxDC* dc = NULL);
    
    bool SendClickEvent(wxWindowID id);
    
    // events
    void onMouseLeftDown(wxMouseEvent& evt);
    void onMouseLeftUp(wxMouseEvent& evt);
    void onEraseBackground(wxEraseEvent& evt);
    void onPaint(wxPaintEvent& evt);
    void onSize(wxSizeEvent& evt);
    
protected:

    std::vector<ButtonBarItem*> m_items;
    std::vector<ButtonBarItem*> m_selected_items;   // toggle mode
    ButtonBarItem* m_last_selected_item;            // normal mode
    ButtonBarItem* m_selected_item;                 // normal mode
    
private:

    wxBitmap m_bmp;
    wxMemoryDC m_memdc;

    bool m_multiline;
    bool m_space_evenly;
    int m_mode;
    int m_border_sides;
    int m_background_gradient_direction;

    int m_item_padding_left;
    int m_item_padding_top;
    int m_item_padding_right;
    int m_item_padding_bottom;
    int m_item_min_width;
    int m_item_min_height;
    
    int m_allitems_max_height;
    
    int m_bmp_width;
    int m_bmp_height;
    int m_cli_width;
    int m_cli_height;

    wxColor m_highlight_color;
    wxColor m_item_border_color;
    wxColor m_border_color;
    wxColor m_start_color;
    wxColor m_end_color;

    DECLARE_EVENT_TABLE()
};


}; // namespace kcl


#endif


