/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams; Aaron L. Williams
 * Created:  2009-12-11
 *
 */


#ifndef __KCL_BORDERPANEL_H
#define __KCL_BORDERPANEL_H


#include "util.h"


class wxStaticLine;


namespace kcl
{


class Button;
class BorderPanel :  public wxPanel,
                     public xcm::signal_sink
{
public:
    

    BorderPanel(wxWindow* parent,
                long style = kcl::DEFAULT_BORDER);

    ~BorderPanel();

    // note: enum order must match order of items as they
    // are inserted into the panel; TODO: should remove
    // this dependency
    enum
    {
        // reduced list
        border_noborder = 1,
        border_bottom = 2,
        border_left = 3,
        border_right = 4,
        border_bottom_top = 5,
        border_outside = 6   
/*
        // full list    
        border_noborder = 1,
        border_bottom = 2,
        border_left = 3,
        border_right = 4,
        border_bottom_dbl = 5,
        border_bottom_thick = 6,
        border_bottom_top = 7,
        border_bottom_dbl_t = 8,
        border_bottom_tk_t = 9,
        borders_all = 10,
        border_outside = 11,
        border_thick_out = 12
*/        
    };

public: // signals

    xcm::signal1<BorderPanel*> sigDestructing;
    xcm::signal1<int> sigBorderSelected;

private:

    void onSize(wxSizeEvent& event);
    void onButtonRender(kcl::Button* button,
                        wxDC* dc,
                        int flags);

    void onButtonClicked(kcl::Button* button, int b);

private:
    
    wxBoxSizer*  m_main_sizer;
    wxGridSizer* m_grid_sizer;

    // for drawing button hover and selection
    wxColor m_highlight_color;

    DECLARE_EVENT_TABLE()
};


};


#endif


