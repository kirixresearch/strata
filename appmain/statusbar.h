/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-09-27
 *
 */


#ifndef __APP_STATUSBAR_H
#define __APP_STATUSBAR_H


#include <xcm/xcm.h>
#include "framework.h"


// NOTE: The current way we've implemented statusbars in cfw does not
//       allow the ability to create indexes for each statusbar item
//       (which would allow the notion of inserting an item at a certain
//       place in the statusbar).  If this becomes necessary, we'll deal
//       with it at that point.  The way things are done right now,
//       left-aligned global items are added to the statusbar, then
//       the statusbar provider's items are added to the statusbar
//       and then right-aligned global items are added at the end


class StatusBarItem : public IStatusBarItem
{

XCM_CLASS_NAME("cfw.StatusBarItem")
XCM_BEGIN_INTERFACE_MAP(StatusBarItem)
    XCM_INTERFACE_ENTRY(IStatusBarItem)
XCM_END_INTERFACE_MAP()

public:

    StatusBarItem()
    {
        m_id = -1;
        m_name = wxEmptyString;
        m_value = wxEmptyString;
        m_bitmap = wxNullBitmap;
        m_control = NULL;
        m_tooltip = wxEmptyString;
        m_separator = false;
        m_toggle_mode = false;
        m_toggled = false;
        m_show = true;
        m_proportion = 0;
        m_width = 0;
        m_padding_left = 0;
        m_padding_right = 0;
        m_dirty = true;
    }
    
    ~StatusBarItem()
    {
        if (m_control)
        {
            // pop and delete event handler
            m_control->PopEventHandler(true);
        }
    }

    // IStatusBarItem interface
    
    wxString   getName()    { return m_name;    }
    wxString   getValue()   { return m_value;   }
    wxBitmap   getBitmap()  { return m_bitmap;  }
    wxControl* getControl() { return m_control; }
    bool       isShown()    { return m_show;    }
    bool       isToggled()  { return m_toggled; }
    
    void setWidth(int width)                 { m_width = width;           }
    void setProportion(int proportion)       { m_proportion = proportion; }
    void setPadding(int left, int right)     { m_padding_left = left;
                                               m_padding_right = right;   }
    void setToolTip(const wxString& tooltip) { m_tooltip = tooltip;       }
    void setToggleMode(bool mode)            { m_toggle_mode = mode;      }
    
    void setValue(const wxString& value)
    {
        if (m_value.Cmp(value) != 0)
            m_dirty = true;
        
        m_value = value;
    }
    
    void setBitmap(const wxBitmap& bitmap)
    {
        if (!m_bitmap.IsSameAs(bitmap))
            m_dirty = true;
        
        m_bitmap = bitmap;
    }
    
    void setToggled(bool state)
    {
        if (m_toggled != state)
            m_dirty = true;
        
        m_toggled = state;
    }
    
    void show(bool show)
    {
        if (m_control != NULL)
            m_control->Show(show);
        
        m_show = show;
    }
    
    // accessor functions
    
    int getId()                 { return m_id;            }
    int getWidth()              { return m_width;         }
    int getProportion()         { return m_proportion;    }
    int getPaddingLeft()        { return m_padding_left;  }
    int getPaddingRight()       { return m_padding_right; }
    wxString getToolTip()       { return m_tooltip;       }
    bool getToggleMode()        { return m_toggle_mode;   }
    bool isSeparator()          { return m_separator;     }
    bool isDirty()              { return m_dirty;         }
    
    void setDirty(bool dirty)   { m_dirty = dirty;        }
    void setId(int id)          { m_id = id;              }
    
public:

    int m_id;
    wxString m_name;
    wxString m_value;
    wxBitmap m_bitmap;
    wxControl* m_control;
    wxString m_tooltip;
    bool m_separator;
    bool m_dirty;
    bool m_show;
    bool m_toggle_mode;
    bool m_toggled;
    int m_proportion;
    int m_width;
    int m_padding_left;
    int m_padding_right;
};




class StatusBar : public wxAuiToolBar,
                  public IStatusBar,
                  public xcm::signal_sink
{
friend class StatusBarItemEvtHandler;

XCM_CLASS_NAME_NOREFCOUNT("cfw.StatusBar")
XCM_BEGIN_INTERFACE_MAP(StatusBar)
    XCM_INTERFACE_ENTRY(IStatusBar)
XCM_END_INTERFACE_MAP()

XCM_IMPLEMENT_SIGNAL1(sigItemLeftDblClick, IStatusBarItemPtr)
XCM_IMPLEMENT_SIGNAL1(sigItemLeftClick, IStatusBarItemPtr)
XCM_IMPLEMENT_SIGNAL0(sigRefresh)

public:

    enum
    {
        LocationLeft = 0,
        LocationRight
    };
    
    StatusBar(IFramePtr frame);
    ~StatusBar();

    // IStatusBar
    wxAuiToolBar* getStatusBarCtrl();
    
    IStatusBarItemPtr addItem(
                                const wxString& item_name,
                                int location = LocationLeft);

    IStatusBarItemPtr addControl(
                                wxControl* control,
                                const wxString& item_name,
                                int location = LocationLeft);

    IStatusBarItemPtr addSeparator(
                                const wxString& item_name,
                                int location = LocationLeft);

    IStatusBarItemPtr getItem(const wxString& item_name);
    
    void showResizeGripper(bool show = true);
    void populate();
    void refresh();

    // frame events
    void onActiveChildChanged(IDocumentSitePtr doc_site);
    
    // event handlers
    void onMouseLeftDblClick(wxMouseEvent& evt);
    void onMouseLeftClick(wxMouseEvent& evt);
    
private:
    
    IStatusBarItemPtr getItemFromId(int id);
    void refreshItems(IStatusBarItemEnumPtr items);
    void addItems(IStatusBarItemEnumPtr items);
    
private:

    int m_id_counter;
    IFramePtr m_frame;
    IStatusBarProviderPtr m_statusbar_provider;
    IStatusBarItemEnumPtr m_left_items;
    IStatusBarItemEnumPtr m_right_items;
    
    DECLARE_EVENT_TABLE()
};




class StatusBarProviderBase : public IStatusBarProvider
{

XCM_CLASS_NAME_NOREFCOUNT("cfw.StatusBarProviderBase")
XCM_BEGIN_INTERFACE_MAP(StatusBarProviderBase)
    XCM_INTERFACE_ENTRY(IStatusBarProvider)
XCM_END_INTERFACE_MAP()

protected:

    StatusBarProviderBase()
    {
        // create the statusbar item enumerator
        m_items = new xcm::IVectorImpl<IStatusBarItemPtr>;
    }
    
    // IStatusBarProvider methods
    
    IStatusBarItemEnumPtr getStatusBarItemEnum()
    {
        return m_items;
    }
    
    IStatusBarItemPtr addStatusBarItem(const wxString& item_name)
    {
        StatusBarItem* c = new StatusBarItem;
        c->m_name = item_name;

        m_items->append(c);
        return c;
    }

    IStatusBarItemPtr addStatusBarSeparator(const wxString& item_name)
    {
        StatusBarItem* c = new StatusBarItem;
        c->m_name = item_name;
        c->m_separator = true;
        
        m_items->append(c);
        return c;
    }

protected:

    IStatusBarItemEnumPtr m_items;
};


#endif
