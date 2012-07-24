/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2007-03-14
 *
 */


#ifndef __KCL_COLORSELECTOR_H
#define __KCL_COLORSELECTOR_H


#include <wx/wx.h>
#include <xcm/signal.h>


namespace kcl
{


class PopupContainer;
class ColorPanel;


class ColorSelector : public wxControl,
                      public xcm::signal_sink
{
public:

    ColorSelector(wxWindow* parent,
                  wxWindowID id = -1,
                  const wxColor& fg_color = *wxBLACK,
                  const wxColor& bg_color = *wxWHITE,
                  const wxPoint& position = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize);

    void setForegroundColor(const wxColor& color);
    void setBackgroundColor(const wxColor& color);
    
    wxColor getForegroundColor();
    wxColor getBackgroundColor();
    
public: // signals

    xcm::signal1< std::vector<wxColor>& > sigCustomColorsRequested;
    xcm::signal1< std::vector<wxColor>  > sigCustomColorsChanged;

private:

    // -- event handlers --
    void onPaint(wxPaintEvent& evt);
    void onSize(wxSizeEvent& evt);
    void onMove(wxMoveEvent& evt);
    void onMouseButtonDown(wxMouseEvent& evt);
    void onMouseButtonUp(wxMouseEvent& evt);
    void onEraseBackground(wxEraseEvent& evt);

    // -- color panel events --
    void onColorPanelDestructing(kcl::ColorPanel* panel);
    void onCustomColorsRequested(std::vector<wxColor>& colors);
    void onCustomColorsChanged(std::vector<wxColor> colors);
    void onColorSelected(wxColour color, int mode);

    // this window does not accept focus, and should
    // not be included in tab traversal lists
    bool AcceptsFocus() const { return FALSE; }
    
private:

    PopupContainer* m_color_panel_cont;
    ColorPanel* m_color_panel;

    int m_cli_width;
    int m_cli_height;
    
    wxColor m_fgcolor;
    wxColor m_bgcolor;
    
    wxRect m_fgcolor_rect;
    wxRect m_bgcolor_rect;
    
    wxPoint m_mouse_down;
    
    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(ColorSelector)
};


#define EVT_FGCOLOR_CHANGED(fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_COLORSELECTOR_FGCOLOR_CHANGED, wxID_ANY, wxID_ANY, (wxObjectEventFunction)(wxEventFunction)&fn, (wxObject*)NULL),
#define EVT_BGCOLOR_CHANGED(fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_COLORSELECTOR_BGCOLOR_CHANGED, wxID_ANY, wxID_ANY, (wxObjectEventFunction)(wxEventFunction)&fn, (wxObject*)NULL),


};  // namespace kcl


#endif  // __KCL_COLORSELECTOR_H




