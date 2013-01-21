/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2004-02-04
 *
 */


#ifndef __KCL_COLORDROPDOWN_H
#define __KCL_COLORDROPDOWN_H


namespace kcl
{


class PopupContainer;
class ColorPanel;

class ColorDropDown : public kcl::Button,
                      public xcm::signal_sink
{

public:

    ColorDropDown(wxWindow* parent,
                  wxWindowID id,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize);

    wxColor getColor();
    void setColor(const wxColor& color);
    void setCustomColors(const std::vector<wxColour>& custom_colors);
    void getCustomColors(std::vector<wxColour>& custom_colors);

public: // signals
    
    xcm::signal1<ColorDropDown*> sigCustomColorDialogOk;
    xcm::signal1<wxColor> sigColorSelected;
    
protected:

    void onDropDownClicked();
    void onButtonClicked();

private:

    wxBitmap createColorBitmap(const wxBitmap& bmp);
    void onColorPanelDestructing(kcl::ColorPanel* panel);
    void onColorSelected(wxColor color, int mode);
    void onCustomColorDialogOk();

private:

    wxColor m_color;
    std::vector<wxColor> m_custom_colors;
    PopupContainer* m_popup_container;
    ColorPanel* m_color_panel;
};


};


#endif


