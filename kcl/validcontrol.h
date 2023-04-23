/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2002-01-19
 *
 */


#ifndef H_KCL_VALIDCONTROL_H
#define H_KCL_VALIDCONTROL_H


#include <wx/wx.h>


namespace kcl
{


class ValidControl : public wxControl
{
public:

    enum
    {
        alignLeft = 0,
        alignCenter = 1,
        alignRight = 2
    };

    ValidControl(wxWindow* parent, wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxPoint(0,0),
                 const wxSize& size = wxSize(70,25));

    void setValidLabel(const wxString& label);
    void setInvalidLabel(const wxString& label);
    
    void setValidBitmap(const wxBitmap& bitmap);
    void setInvalidBitmap(const wxBitmap& bitmap);
    
    void setBorderVisible(bool state);
    void setLabelVisible(bool state);
    void setAlignment(int align);
    void setValid(bool state);

    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);

private:

    bool m_valid_state;
    bool m_border_visible;
    bool m_label_visible;
    int  m_alignment;
    
    wxString m_valid_label;
    wxString m_invalid_label;

    wxBitmap m_valid_bitmap;
    wxBitmap m_invalid_bitmap;
    
    wxBrush m_bgbrush;

    // this window does not accept focus, and should
    // not be included in tab traversal lists
    bool AcceptsFocus() const { return FALSE; }

    DECLARE_EVENT_TABLE()
};


}; // namespace kcl



#endif

