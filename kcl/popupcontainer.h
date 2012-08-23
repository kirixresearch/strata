/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2002-08-13
 *
 */


#ifndef __KCL_POPUPCONTAINER_H
#define __KCL_POPUPCONTAINER_H


namespace kcl
{

class PopupContainer : public wxFrame
{
public:

    enum AnchorPoint
    {
        fromTopLeft = 0,
        fromTopRight = 1,
        fromBottomLeft = 2,
        fromBottomRight = 3
    };
    
    enum CloseOption
    {
        closeOnKillFocus = 0,
        closeOnClickOutside = 1  // the popup container still destroys its
                                 // children in onKillFocus(), but only if
                                 // none of the children received the focus
    };

    PopupContainer(const wxPoint& position = wxPoint(0,0),
                   int anchor_corner = fromTopLeft,
                   int close_option = closeOnKillFocus);
    
    void doPopup(wxWindow* child, bool auto_size = true);
    
    void setDestroyChild(bool b);

    // event handlers
    void onKillFocus(wxFocusEvent& evt);
    void onActivate(wxActivateEvent& evt);
    
private:

    wxWindow* m_child;
    int m_anchor_corner;
    int m_close_option;
    bool m_destroy_child;

    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(PopupContainer)
};


};

#endif

