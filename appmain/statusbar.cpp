/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-09-27
 *
 */


#include "appmain.h"
#include "statusbar.h"
#include "framework.h"
#include "util.h"


const int RESIZE_GRIPPER_WIDTH = 12;


/* XPM */
static const char *xpm_window_resize_gripper[] = {
/* columns rows colors chars-per-pixel */
"12 12 3 1",
"+ c #F0F0F0",
"@ c #A0A0A0",
"  c None",
/* pixels */
"            ",
"         @@ ",
"         @@+",
"          ++",
"            ",
"     @@  @@ ",
"     @@+ @@+",
"      ++  ++",
"            ",
" @@  @@  @@ ",
" @@+ @@+ @@+",
"  ++  ++  ++"
};



class StatusBarArt : public wxAuiDefaultToolBarArt
{
public:
    
    StatusBarArt()
    {
        m_resize_gripper_bmp = wxBitmap(xpm_window_resize_gripper);
        m_show_resize_gripper = false;
    }
    
    void DrawBackground(wxDC& dc,
                        wxWindow* wnd,
                        const wxRect& rect)
    {
    
        wxAuiDefaultToolBarArt::DrawBackground(dc, wnd, rect);
        
        if (m_show_resize_gripper)
        {
            int bmp_x = rect.x+rect.width-m_resize_gripper_bmp.GetWidth();
            int bmp_y = rect.y+rect.height-m_resize_gripper_bmp.GetHeight()-1;
            dc.DrawBitmap(m_resize_gripper_bmp, bmp_x, bmp_y, true);
        }
    }
    
    bool isResizeGripperShown() { return m_show_resize_gripper; }
    void showResizeGripper(bool show = true)
    {
        m_show_resize_gripper = show;
    }

private:

    wxBitmap m_resize_gripper_bmp;
    bool m_show_resize_gripper;
};


class StatusBarItemEvtHandler : public wxEvtHandler
{
public:

    StatusBarItemEvtHandler(StatusBar* statusbar)
    {
        m_statusbar = statusbar;
        
        // connect these events to the statusbar's event handlers
        Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(StatusBar::onMouseLeftDblClick), NULL, m_statusbar);
        Connect(wxEVT_LEFT_UP, wxMouseEventHandler(StatusBar::onMouseLeftClick), NULL, m_statusbar);
    }
    
private:
    
    StatusBar* m_statusbar;
};




BEGIN_EVENT_TABLE(StatusBar, wxAuiToolBar)
    EVT_LEFT_UP(StatusBar::onMouseLeftClick)
    EVT_LEFT_DCLICK(StatusBar::onMouseLeftDblClick)
END_EVENT_TABLE()


StatusBar::StatusBar(IFramePtr frame) :
                wxAuiToolBar(frame->getFrameWindow(),
                             wxID_ANY,
                             wxDefaultPosition,
                             wxDefaultSize,
                               wxAUI_TB_DEFAULT_STYLE |
                               wxAUI_TB_NO_AUTORESIZE |
                               wxAUI_TB_TEXT |
                               wxNO_BORDER)
{
    #ifdef __WXGTK__
    limitFontSize(this, 10);
    #endif

    // set wxAuiToolBar options
    SetArtProvider(new StatusBarArt);
    SetToolTextOrientation(wxAUI_TBTOOL_TEXT_RIGHT);
    SetToolPacking(FromDIP(2));
    SetMargins(FromDIP(2), FromDIP(2), FromDIP(2), FromDIP(2));
    SetGripperVisible(false);
    SetOverflowVisible(false);
    SetMinSize(FromDIP(wxSize(30,26)));

    // add frame event handler
    frame->sigActiveChildChanged().connect(this, &StatusBar::onActiveChildChanged);
    m_frame = frame;
    
    // initialize our ID counter
    m_id_counter = 0;
    
    // we'll start without a statusbar provider
    m_statusbar_provider = xcm::null;
    
    // create our global item vector
    m_left_items = new xcm::IVectorImpl<IStatusBarItemPtr>;
    m_right_items = new xcm::IVectorImpl<IStatusBarItemPtr>;
    
    populate();
}

StatusBar::~StatusBar()
{

}

wxAuiToolBar* StatusBar::getStatusBarCtrl()
{
    return this;
}

IStatusBarItemPtr StatusBar::addItem(const wxString& item_name,
                                          int location)
{
    StatusBarItem* c = new StatusBarItem;
    c->m_name = item_name;

    if (location == StatusBar::LocationLeft)
        m_left_items->append(c);
     else
        m_right_items->append(c);
        
    return c;
}

IStatusBarItemPtr StatusBar::addSeparator(const wxString& item_name,
                                               int location)
{
    StatusBarItem* c = new StatusBarItem;
    c->m_name = item_name;
    c->m_separator = true;
    
    if (location == StatusBar::LocationLeft)
        m_left_items->append(c);
     else
        m_right_items->append(c);
        
    return c;
}

IStatusBarItemPtr StatusBar::addControl(wxControl* control,
                                             const wxString& item_name,
                                             int location)
{
    if (control == NULL)
        return xcm::null;
    
    // we need to set our own event handler for this control so
    // we can handle its events in the statusbar event handlers
    control->PushEventHandler(new StatusBarItemEvtHandler(this));
    
    StatusBarItem* c = new StatusBarItem;
    c->m_name = item_name;
    c->m_control = control;
    
    if (location == StatusBar::LocationLeft)
        m_left_items->append(c);
     else
        m_right_items->append(c);
        
    return c;
}

IStatusBarItemPtr StatusBar::getItem(const wxString& item_name)
{
    IStatusBarItemPtr item;

    // try to find the item in the left-aligned global items
    size_t i, count = m_left_items->size();
    for (i = 0; i < count; i++)
    {
        item = m_left_items->getItem(i);
        if (item_name.CmpNoCase(item->getName()) == 0)
            return item;
    }

    // try to find the item in the right-aligned global items
    count = m_right_items->size();
    for (i = 0; i < count; i++)
    {
        item = m_right_items->getItem(i);
        if (item_name.CmpNoCase(item->getName()) == 0)
            return item;
    }

    // if we have a statusbar provider, look through its items
    if (m_statusbar_provider.isOk())
    {
        IStatusBarItemEnumPtr items;
        items = m_statusbar_provider->getStatusBarItemEnum();
    
        count = items->size();
        for (i = 0; i < count; i++)
        {
            item = items->getItem(i);
            if (item_name.CmpNoCase(item->getName()) == 0)
                return item;
        }
    }

    return xcm::null;
}

void StatusBar::addItems(IStatusBarItemEnumPtr items)
{
    StatusBarItem* item_raw;
    IStatusBarItemPtr item;
    
    bool toggled;
    bool toggle_mode;
    int proportion;
    wxString tooltip;
    wxControl* control;
    
    size_t i = 0, count = items->size();
    for (i = 0; i < count; i++)
    {
        item = items->getItem(i);
        item_raw = (StatusBarItem*)item.p;
        
        // if the item isn't shown, don't add it to the toolbar
        if (!item_raw->isShown())
        {
            item_raw->setId(-1);
            continue;
        }
        
        proportion = item_raw->getProportion();
        control = item_raw->getControl();
        tooltip = item_raw->getToolTip();
        toggle_mode = item_raw->getToggleMode();
        toggled = item->isToggled();
        
        // add left padding, if necessary
        if (item_raw->getPaddingLeft() > 0)
            AddSpacer(item_raw->getPaddingLeft());
        
        if (control != NULL)
        {
            AddControl(control);
            
            // handle proportional items
            if (proportion > 0)
                SetToolProportion(control->GetId(), proportion);
            
            // set the tooltip for the item if it exists
            if (!tooltip.IsEmpty())
                SetToolShortHelp(m_id_counter, tooltip);
            
            item_raw->setId(control->GetId());
        }
         else if (item_raw->isSeparator())
        {
            AddSeparator();
        }
         else if (item_raw->getBitmap().IsOk())
        {
            AddTool(m_id_counter, item->getBitmap(), wxNullBitmap, toggle_mode, 0);
            
            // handle proportional items
            if (proportion > 0)
                SetToolProportion(m_id_counter, proportion);
            
            // set the tooltip for the item if it exists
            if (!tooltip.IsEmpty())
                SetToolShortHelp(m_id_counter, tooltip);
            
            // set the toggle state for the item
            if (toggle_mode)
                ToggleTool(m_id_counter, toggled);
            
            item_raw->setId(m_id_counter);
            m_id_counter++;
        }
         else
        {
            AddLabel(m_id_counter, item_raw->getValue(), item_raw->getWidth());
            
            // handle proportional items
            if (proportion > 0)
                SetToolProportion(m_id_counter, proportion);
            
            // set the tooltip for the item if it exists
            if (!tooltip.IsEmpty())
                SetToolShortHelp(m_id_counter, tooltip);
            
            item_raw->setId(m_id_counter);
            m_id_counter++;
        }

        // add right padding, if necessary
        if (item_raw->getPaddingRight() > 0)
            AddSpacer(item_raw->getPaddingRight());
    }
}

void StatusBar::refreshItems(IStatusBarItemEnumPtr items)
{
    StatusBarItem* item_raw;
    IStatusBarItemPtr item;
    
    wxBitmap bmp;
    wxString str;
    bool toggle_mode = false;
    bool toggled = false;
    
    size_t i = 0, count = items->size();
    for (i = 0; i < count; i++)
    {
        item = items->getItem(i);
        item_raw = (StatusBarItem*)item.p;
        
        // the item does not need to be refreshed
        if (!item_raw->isDirty())
            continue;
        
        // items that aren't shown don't need to be refreshed
        if (!item_raw->isShown())
            continue;
        
        // controls refresh themselves
        if (item_raw->getControl() != NULL)
            continue;
        
        // separators don't need to be refreshed
        if (item_raw->isSeparator())
            continue;
        
        wxAuiToolBarItem* tool = FindTool(item_raw->getId());
        if (tool == NULL)
            continue;
            
        bmp = item_raw->getBitmap();
        str = item_raw->getValue();
        toggle_mode = item_raw->getToggleMode();
        toggled = item_raw->isToggled();
        
        // refresh the bitmap
        if (bmp.IsOk())
            tool->SetBitmap(bmp);
        
        // refresh the label
        tool->SetLabel(str);
        
        // refresh the toggle state
        if (toggle_mode)
            ToggleTool(tool->GetId(), toggled);
        
        item_raw->setDirty(false);
    }
}

void StatusBar::showResizeGripper(bool show)
{
    // if the resize gripper is already shown/hidden, do nothing
    StatusBarArt* art = (StatusBarArt*)GetArtProvider();
    if (art->isResizeGripperShown() == show)
        return;
    
    // make sure the art provider knows if it
    // should show a window resize gripper
    art->showResizeGripper(show);
    
    int pad = FromDIP(2);

    // add extra space to allow for the gripper drawing
    if (show)
        SetMargins(pad, RESIZE_GRIPPER_WIDTH + pad, pad, pad);
         else
        SetMargins(pad, pad, pad, pad);
    
    Realize();
}

void StatusBar::populate()
{
    // start fresh, remove all toolbar items
    m_id_counter = 0;
    Clear();
    
    addItems(m_left_items);
    if (m_statusbar_provider.isOk())
        addItems(m_statusbar_provider->getStatusBarItemEnum());
    addItems(m_right_items);
    
    Realize();
}

void StatusBar::refresh()
{
    refreshItems(m_left_items);
    if (m_statusbar_provider.isOk())
        refreshItems(m_statusbar_provider->getStatusBarItemEnum());
    refreshItems(m_right_items);
    
    Realize();
    
    sigRefresh().fire();
}

void StatusBar::onActiveChildChanged(IDocumentSitePtr doc_site)
{
    // there's no active document
    if (!doc_site)
    {
        m_statusbar_provider = xcm::null;
        populate();
        return;
    }
    
    // can't get the IDocument from the active child
    IDocumentPtr doc = doc_site->getDocument();
    if (!doc)
    {
        m_statusbar_provider = xcm::null;
        populate();
        return;
    }
    
    // the active child isn't a status bar provider
    IStatusBarProviderPtr provider = doc;
    if (!provider)
    {
        m_statusbar_provider = xcm::null;
        populate();
        return;
    }
    
    // assign the statusbar provider
    m_statusbar_provider = provider;
    populate();
}

IStatusBarItemPtr StatusBar::getItemFromId(int id)
{
    StatusBarItem* item_raw;
    IStatusBarItemPtr item;
    
    size_t i = 0, count = m_left_items->size();
    for (i = 0; i < count; i++)
    {
        item = m_left_items->getItem(i);
        item_raw = (StatusBarItem*)item.p;
        if (item_raw->getId() == id)
            return item;
    }
    
    if (m_statusbar_provider.isOk())
    {
        IStatusBarItemEnumPtr items;
        items = m_statusbar_provider->getStatusBarItemEnum();
        
        count = items->size();
        for (i = 0; i < count; i++)
        {
            item = items->getItem(i);
            item_raw = (StatusBarItem*)item.p;
            if (item_raw->getId() == id)
                return item;
        }
    }
    
    count = m_right_items->size();
    for (i = 0; i < count; i++)
    {
        item = m_right_items->getItem(i);
        item_raw = (StatusBarItem*)item.p;
        if (item_raw->getId() == id)
            return item;
    }
    
    return xcm::null;
}

// this function allows us to get the screen coordinates of
// the mouse event regardless of whether the event happened
// on the statusbar or on a control in the statusbar
static wxPoint getEventPosition(StatusBar* statusbar, wxMouseEvent& evt)
{
    wxPoint pt;
    
    wxObject* evt_obj = evt.GetEventObject();
    if (evt_obj->IsKindOf(CLASSINFO(StatusBar)))
    {
        pt = evt.GetPosition();
    }
     else
    {
        // we clicked on a control, so we need to do ClientToScreen()
        // on that control and not the statusbar
        wxControl* control = (wxControl*)evt_obj;
        pt = control->ClientToScreen(evt.GetPosition());
        pt = statusbar->ScreenToClient(pt);
    }
    
    return pt;
}

void StatusBar::onMouseLeftDblClick(wxMouseEvent& evt)
{
    // get the coordinates relative to the statusbar
    wxPoint pt = getEventPosition(this, evt);

    // get the toolbar item we double-clicked on
    wxAuiToolBarItem* hit_item;
    hit_item = FindToolByPosition(pt.x, pt.y);
    if (!hit_item)
        return;
    
    // find the associated statusbar item
    IStatusBarItemPtr item = getItemFromId(hit_item->GetId());
    if (item.isNull())
        return;
    
    sigItemLeftDblClick().fire(item);
}

void StatusBar::onMouseLeftClick(wxMouseEvent& evt)
{
    // allow the wxAuiToolBar to do its handling
    wxAuiToolBar::OnLeftUp(evt);
    
    // get the coordinates relative to the statusbar
    wxPoint pt = getEventPosition(this, evt);

    // get the toolbar item we left-clicked on
    wxAuiToolBarItem* hit_item;
    hit_item = FindToolByPosition(pt.x, pt.y);
    if (!hit_item)
        return;
    
    // find the associated statusbar item
    IStatusBarItemPtr item = getItemFromId(hit_item->GetId());
    if (item.isNull())
        return;
    
    sigItemLeftClick().fire(item);
}
