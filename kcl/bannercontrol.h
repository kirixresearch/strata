/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams; David Z. Williams
 * Created:  2002-08-01
 *
 */


#ifndef H_KCL_BANNERCONTROL_H
#define H_KCL_BANNERCONTROL_H


#include <wx/wx.h>


namespace kcl
{


class BannerControl : public wxControl
{
public:

    enum
    {
        alignLeft = 0,
        alignCenter,
        alignRight
    };
    
    enum
    {
        gradientNone = 0,
        gradientVertical,
        gradientHorizontal
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

public:

    BannerControl(wxWindow* parent, const wxString& text, const wxPoint& point, const wxSize& size);

    // accessor methods
    void setFont(const wxFont& font);
    void setStartColor(const wxColour& new_value);
    void setEndColor(const wxColour& new_value);
    void setTextColor(const wxColour& new_value);
    void setText(const wxString& text);
    void setTextAlignment(int alignment);
    void setTextPadding(int padding);
    void setGradientDirection(int direction);
    void setBorder(int border_sides = borderAll);
    void setBorderColor(const wxColour& new_value);

    // event handlers
    void onPaint(wxPaintEvent& evt);
    void onSize(wxSizeEvent& evt);

private:

    // this window does not accept focus, and should
    // not be included in tab traversal lists
    bool AcceptsFocus() const { return FALSE; }

private:

    wxFont m_font;
    wxColour m_start_color;
    wxColour m_end_color;
    wxColour m_text_color;
    wxColour m_border_color;
    wxString m_text;
    int m_border_sides;
    int m_gradient_direction;
    int m_alignment;
    int m_padding;

    DECLARE_EVENT_TABLE()
};


} // namespace kcl


#endif

