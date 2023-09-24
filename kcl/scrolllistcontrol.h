/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2006-12-07
 *
 */


#ifndef H_KCL_SCROLLLISTCONTROL_H
#define H_KCL_SCROLLLISTCONTROL_H


#include <wx/wx.h>
#include <kl/xcm.h>
#include "util.h"


namespace kcl
{

class ScrollListElement
{
friend class ScrollListItem;

public:
    
    // alignment to another element
    enum
    {
        alignLeft = 0,
        alignRight = 1
    };
    
    // ellipse location for non-wrapped text
    enum
    {
        textEllipsesLeft = 0,
        textEllipsesRight = 1
    };
    
    // relative position to another element
    enum
    {
        positionBelow = 0,
        positionOnLeft = 1,
        positionOnRight = 2
    };
    
    ScrollListElement();
    ~ScrollListElement();
    
    wxControl* getControl() const { return m_control; }
    wxBitmap& getBitmap() { return m_bitmap;  }
    wxString getText() const { return m_text;    }
    
    wxPoint getPosition() const;
    wxSize  getSize() const  { return m_size; }
    wxString getName() const { return m_name; }
    
    int  getHeight() const        { return m_size.GetHeight(); }
    int  getWidth() const         { return m_size.GetWidth();  }
    int  getPaddingLeft() const   { return m_padding_left;     }
    int  getPaddingTop() const    { return m_padding_top;      }
    int  getPaddingRight() const  { return m_padding_right;    }
    int  getPaddingBottom() const { return m_padding_bottom;   }
    int  getTextEllipses() const  { return m_text_ellipses;    }
    bool isTextBold() const       { return m_text_bold;        }
    bool isTextWrap() const       { return m_text_wrap;        }
    bool isVisible() const        { return m_visible;          }
    bool isStretchable() const    { return m_stretch;          }

    void setSize(const wxSize& size)           { m_size = size; }
    void setWidth(int width)                   { m_size.SetWidth(width);     }
    void setHeight(int height)                 { m_size.SetHeight(height);   }
    void setName(const wxString& name)         { m_name = name;              }
    void setText(const wxString& text)         { m_text = text;              }
    void setBitmap(const wxBitmap& bitmap)     { m_bitmap = bitmap;          }
    
    void setTextEllipses(int ellipses)         { m_text_ellipses = ellipses; }
    void setTextBold(bool bold = true)         { m_text_bold = bold;         }
    void setTextWrap(bool wrap = true)         { m_text_wrap = wrap;         }
    void setStretchable(bool stretch = true)   { m_stretch = stretch;        }
    
    void setAbsolutePosition(const wxPoint& position);
    void setRelativePosition(ScrollListElement* relative_to,
                             int position,
                             int alignment = alignLeft);
    void setPadding(int left, int top, int right, int bottom);
    void setVisible(bool visible = true);
    
private:

    wxString m_name;        // for reference purposes
    
    wxControl* m_control;
    wxBitmap m_bitmap;
    wxString m_text;
    
    ScrollListElement* m_relative_to;
    int m_relative_position;
    int m_relative_alignment;
    
    wxPoint m_position;
    wxSize m_size;          // for non-wxControl-derived elements
    
    bool m_visible;
    bool m_stretch;
    bool m_text_bold;
    bool m_text_wrap;
    int  m_text_ellipses;
    int  m_padding_left;
    int  m_padding_top;
    int  m_padding_right;
    int  m_padding_bottom;
};


class ScrollListItem
{
friend class ScrollListControl;

public:

    ScrollListItem()
    {
        m_extra_long = 0;
        m_selected = false;
        m_enabled = true;
        m_update = true;
        m_height = 80;
    }

    ~ScrollListItem()
    {
        std::vector<ScrollListElement*>::iterator it;
        for (it = m_elements.begin(); it != m_elements.end(); ++it)
        {
            delete (*it);
            (*it) = NULL;
        }
    }
    
    ScrollListElement* addElement(wxControl* control,
                                  const wxPoint& position = wxPoint(0,0))
    {
        ScrollListElement* e = new ScrollListElement;
        e->m_control = control;
        e->m_position = position;
        e->m_size = control->GetSize();
        m_elements.push_back(e);
        return e;
    }
    
    ScrollListElement* addElement(const wxString& text,
                                  const wxPoint& position = wxPoint(0,0))
    {
        ScrollListElement* e = new ScrollListElement;
        e->m_text = text;
        e->m_position = position;
        m_elements.push_back(e);
        return e;
    }
    
    ScrollListElement* addElement(const wxBitmap& bitmap,
                                  const wxPoint& position = wxPoint(0,0))
    {
        ScrollListElement* e = new ScrollListElement;
        e->m_bitmap = bitmap;
        e->m_position = position;
        e->m_size = wxSize(bitmap.GetWidth(), bitmap.GetHeight());
        m_elements.push_back(e);
        return e;
    }
    
    ScrollListElement* getElement(const wxString& name)
    {
        std::vector<ScrollListElement*>::iterator it;
        for (it = m_elements.begin(); it != m_elements.end(); ++it)
        {
            if (name.CmpNoCase((*it)->m_name) == 0)
                return (*it);
        }
        
        return NULL;
    }

    ScrollListElement* getElementByIdx(size_t idx)
    {
        if (idx < 0 || idx >= m_elements.size())
            return NULL;
            
        return m_elements[idx];
    }

    void setExtraString(const wxString& extra_string) { m_extra_string = extra_string; }
    wxString getExtraString() const { return m_extra_string; }
    
    void setExtraLong(long extra_long) { m_extra_long = extra_long; }
    long getExtraLong() const { return m_extra_long; }
    
    void setSelected(bool selected = true) { m_selected = selected; }
    bool isSelected() const { return m_selected; }
    
    void setEnabled(bool enabled = true) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    void setUpdated(bool update) { m_update = update; }
    int getUpdated() const { return m_update; }
    
    void setHeight(int height) { m_height = height; }
    int getHeight() const { return m_height; }
    
private:

    std::vector<ScrollListElement*> m_elements;
    wxString m_extra_string;
    long m_extra_long;
    bool m_selected;
    bool m_enabled;
    bool m_update;
    int m_height;
};


class ScrollListControl : public wxScrolledWindow,
                          public xcm::signal_sink
{
public:

    ScrollListControl(wxWindow* parent,
                   wxWindowID id = -1,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = kcl::DEFAULT_BORDER);
    ~ScrollListControl();
    
    size_t getItemCount();
    int getItemIdx(ScrollListItem* item);
    ScrollListItem* getItem(size_t idx);
    
    void setOverlayText(const wxString& text);
    void addItem(ScrollListItem* item);
    void removeItem(ScrollListItem* item);
    void clearItems();
    void refresh();

public: // signals

    xcm::signal2<ScrollListItem* /* old_selected_item */,
                 ScrollListItem* /* new selected item */> sigSelectedItemChanged;
    
protected:

    void calcVirtualHeight();

private:

    void render();
    void repaint(wxDC* dc = NULL);

    void allocBitmap(int width, int height);
    void calcItemHeight(kcl::ScrollListItem* item);
    void drawItem(kcl::ScrollListItem* item, int item_y);
    void drawItems();
    void drawOverlayText();
    
    // event handlers
    void onMouseLeftClick(wxMouseEvent& evt);
    void onScroll(wxScrollWinEvent& evt);
    void onPaint(wxPaintEvent& evt);
    void onSize(wxSizeEvent& evt);
    void onDoRefresh(wxCommandEvent& evt);
    void onEraseBackground(wxEraseEvent& evt);

protected:

    std::vector<ScrollListItem*> m_items;

private:

    wxBitmap m_bmp;
    wxMemoryDC m_memdc;
    wxMemoryDC m_hidden_memdc;

    wxFont m_font;
    wxFont m_bold_font;
    wxFont m_overlay_font;
    
    int m_bmp_width;
    int m_bmp_height;
    int m_cli_width;
    int m_cli_height;
    int m_scroll_pos;
    int m_virtual_height;

    wxColour m_highlight_colour;
    wxString m_overlay_text;
    
    DECLARE_EVENT_TABLE()
};


} // namespace kcl


#endif


