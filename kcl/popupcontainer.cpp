/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#include <wx/wx.h>
#include "popupcontainer.h"


namespace kcl
{


BEGIN_EVENT_TABLE(PopupContainer, wxFrame)
    EVT_KILL_FOCUS(PopupContainer::onKillFocus)
    EVT_ACTIVATE(PopupContainer::onActivate)
END_EVENT_TABLE()


IMPLEMENT_CLASS(kcl::PopupContainer, wxFrame)


static int getTaskBarHeight()
{
#ifdef WIN32
    APPBARDATA abd;
    memset(&abd, 0, sizeof(APPBARDATA));

    abd.cbSize = sizeof(APPBARDATA);
    if (SHAppBarMessage(ABM_GETSTATE, &abd) & ABS_AUTOHIDE)
        return 2;

    HWND hWndAppBar = abd.hWnd;

    memset(&abd, 0, sizeof(APPBARDATA));
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hWndAppBar;
    SHAppBarMessage(ABM_GETTASKBARPOS, &abd);

    return abd.rc.bottom - abd.rc.top;

#else
    return 50; // default task bar height
#endif
}


PopupContainer::PopupContainer(const wxPoint& position,
                               int anchor_corner,
                               int close_option)
                    : wxFrame(NULL,
                              -1,
                              wxT(""),
                              position,
                              wxSize(100,100),
                              wxFRAME_NO_TASKBAR | wxNO_BORDER)
{
    m_child = NULL;
    m_destroy_child = true;
    m_anchor_corner = anchor_corner;
    m_close_option = close_option;
}

void PopupContainer::setDestroyChild(bool b)
{
    m_destroy_child = b;
}

void PopupContainer::doPopup(wxWindow* child, bool auto_size)
{
    m_child = child;
    if (auto_size)
    {
        SetClientSize(child->GetSize());
    }

    int screen_y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) -
                   getTaskBarHeight();
                   
    int screen_x = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
    wxPoint pt = GetPosition();
    wxSize size = GetSize();

    if (m_anchor_corner == fromBottomLeft ||
        m_anchor_corner == fromBottomRight)
    {
        pt.y -= size.GetHeight();
        Move(pt.x, pt.y);
    }
    
    if (m_anchor_corner == fromTopRight ||
        m_anchor_corner == fromBottomRight)
    {
        pt.x -= size.GetWidth();
        Move(pt.x, pt.y);
    }
    
    // move the window if we are off the screen
    // vertically (30 == ~task-bar size)
    if (pt.y + size.GetHeight() > screen_y)
    {
        Move(pt.x, screen_y - size.GetHeight());
    }

    // move the window if we are off the screen horizontally
    if (pt.x + size.GetWidth() > screen_x)
    {
        Move(pt.x - size.GetWidth() + 38, pt.y);
    }

    // create the child inside a sizer
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(child, 1, wxEXPAND);
    SetSizer(sizer);
    Layout();
    child->Show(true);

    Show(true);

    SetFocus();
}

void PopupContainer::onKillFocus(wxFocusEvent& event)
{
    if (m_close_option == closeOnKillFocus)
    {
        #ifdef __WXMSW__
        if (wxWindow::FindFocus() != m_child)
        {
            Show(false);
            if (m_destroy_child)
            {
                Destroy();
            }
        }
         else
        {
            // set the focus back to us
            SetFocus();
        }
        #else
        event.Skip();
        #endif
    }
     else if (m_close_option == closeOnClickOutside)
    {
        // NOTE: This functionality has not been tested on non-MSW platforms,
        //       so make sure that this work with GTK, etc.
    
        #ifdef __WXMSW__
        bool do_destroy = false;
        wxWindow* focus_wnd = wxWindow::FindFocus();
        
        // if the child window didn't receive the focus, the only thing that
        // can keep up from destroying the popup container is if one of the
        // children received the focus, which we'll check that below        
        if (focus_wnd != m_child)
            do_destroy = true;
        
        wxWindowList children = m_child->GetChildren();
        int i, count = children.GetCount();
        for (i = 0; i < count; ++i)
        {
            wxWindow* wnd = children.Item(i)->GetData();
            if (focus_wnd == wnd)
            {
                // one of the children received the focus; don't destroy
                do_destroy = false;
                
                // set the focus back to us (if we don't do this here,
                // the popup container will never be detroyed)
                SetFocus();
                break;
            }
        }
        
        if (do_destroy)
        {
            Show(false);
            if (m_destroy_child)
            {
                Destroy();
            }
        }
        #else
        event.Skip();
        #endif    
    }
}

void PopupContainer::onActivate(wxActivateEvent& event)
{
    #ifndef __WXMSW__
    if (!event.GetActive())
    {
        Show(false);
        if (m_destroy_child)
        {
            Destroy();
        }
    }
    #else
    event.Skip();
    #endif
}


};

