/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2002-01-23
 *
 */


#ifndef H_KCL_COLORPANEL_H
#define H_KCL_COLORPANEL_H


#include "util.h"


class wxStaticLine;


namespace kcl
{


// this helper class specifies the vertical order
// of the elements in the color panel

class ColorPanelLayout
{
public:

    enum
    {
        Empty = -1,
        DefaultColorButton = 0,
        NoColorButton = 1,
        ColorGrid = 2
    };
    
    ColorPanelLayout(int _element1,
                     int _element2 = Empty,
                     int _element3 = Empty)
    {
        // this logic prohibits an element from being added
        // to the layout more than once
        int element1 = _element1;
        int element2 = (_element1 == _element2) ? Empty : _element2;
        int element3 = (_element3 == _element1 || _element3 == _element2)
                           ? Empty : _element3;
        
        if (element1 == Empty && element2 == Empty && element3 == Empty)
            element1 = ColorGrid;

        elements.push_back(element1);
        elements.push_back(element2);
        elements.push_back(element3);
    }

    bool getDefaultColorButtonExists()
    {
        std::vector<int>::iterator it;
        for (it = elements.begin(); it != elements.end(); ++it)
        {
            if (*it == DefaultColorButton)
                return true;
        }
        return false;
    }

    bool getNoColorButtonExists()
    {
        std::vector<int>::iterator it;
        for (it = elements.begin(); it != elements.end(); ++it)
        {
            if (*it == NoColorButton)
                return true;
        }
        return false;
    }
    
public:

    std::vector<int> elements;
};


static inline ColorPanelLayout DefaultColorPanelLayout()
{
    return ColorPanelLayout(ColorPanelLayout::ColorGrid);
};


class Button;
class ColorPanel :  public wxPanel,
                    public xcm::signal_sink
{
public:
    
    enum
    {
        ModeFill = 0,
        ModeLine = 1,
        ModeText = 2,
        ModeNone = 3
    };
    
    ColorPanel(wxWindow* parent,
               ColorPanelLayout layout = DefaultColorPanelLayout(),
               long style = kcl::DEFAULT_BORDER);

    ~ColorPanel();

    void setMode(int mode);
    void setDefaultColor(const wxColor& color,
                         const wxString& label = wxEmptyString);
    void setCustomColors(const std::vector<wxColour>& custom_colors);
    void getCustomColors(std::vector<wxColour>& custom_colors);

public: // signals

    xcm::signal1<ColorPanel*> sigDestructing;
    xcm::signal1< std::vector<wxColor>& > sigCustomColorsRequested;
    xcm::signal1< std::vector<wxColor>  > sigCustomColorsChanged;
    xcm::signal2<wxColour, int> sigColorSelected;
    xcm::signal0 sigCustomColorDialogOk;

private:

    void populateColorVector();

    void onButtonRender(kcl::Button* button,
                        wxDC* dc,
                        int flags);

    void onButtonClicked(kcl::Button* button, int b);
    void onNoColorClicked(kcl::Button* button, int b);
    void onDefaultColorClicked(kcl::Button* button, int b);
    void onMoreColorsClicked(kcl::Button* button, int b);

    void onSize(wxSizeEvent& event);

private:
    
    wxBoxSizer*  m_main_sizer;
    wxGridSizer* m_grid_sizer;
    wxStaticLine* m_staticline;
    kcl::Button* m_default_color_button;
    kcl::Button* m_more_colors_button;
    kcl::Button* m_no_color_button;

    std::vector<wxColor> m_colors;
    std::vector<wxColor> m_custom_colors;
    wxColor m_default_color;
    
    // for drawing button hover and selection
    wxColor m_highlight_color;
    
    int m_last_mode;
    int m_mode;

    DECLARE_EVENT_TABLE()
};


};


#endif


