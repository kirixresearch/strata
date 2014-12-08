/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2012-10-23
 *
 */

#include <wx/wx.h>

#if defined(__WXMSW__) && wxCHECK_VERSION(2,9,0)
#include "wx/msw/dc.h"
#endif

#include "util.h"


namespace kcl
{



double blendColor(double fg, double bg, double alpha)
{
    double result = bg + (alpha * (fg - bg));
    if (result < 0.0)
        result = 0.0;
    if (result > 255)
        result = 255;
    return result;
}

wxColor stepColor(const wxColor& c, int ialpha)
{
    if (ialpha == 100)
        return c;
        
    double r = c.Red(), g = c.Green(), b = c.Blue();
    double bg;
    
    // ialpha is 0..200 where 0 is completely black
    // and 200 is completely white and 100 is the same
    // convert that to normal alpha 0.0 - 1.0
    ialpha = wxMin(ialpha, 200);
    ialpha = wxMax(ialpha, 0);
    double alpha = ((double)(ialpha - 100.0))/100.0;
    
    if (ialpha > 100)
    {
        // blend with white
        bg = 255.0;
        alpha = 1.0 - alpha;  // 0 = transparent fg; 1 = opaque fg
    }
     else
    {
        // blend with black
        bg = 0.0;
        alpha = 1.0 + alpha;  // 0 = transparent fg; 1 = opaque fg
    }
    
    r = blendColor(r, bg, alpha);
    g = blendColor(g, bg, alpha);
    b = blendColor(b, bg, alpha);
    
    return wxColor((int)r, (int)g, (int)b);
}

wxColor getBaseColor()
{

#ifdef __WXMAC__
/*
    wxBrush toolbarbrush;
    toolbarbrush.MacSetTheme( kThemeBrushToolbarBackground );
    wxColor base_color = toolbarbrush.GetColour();
*/
    wxColor base_color = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
#else
    wxColor base_color = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
#endif

    // the base_colour is too pale to use as our base colour,
    // so darken it a bit --
    if ((255-base_color.Red()) +
        (255-base_color.Green()) +
        (255-base_color.Blue()) < 60)
    {
        base_color = kcl::stepColor(base_color, 92);
    }

    return base_color;
}

wxColor getBorderColor()
{
    return stepColor(getBaseColor(), 75);
}

wxColor getHighlightColor()
{
    return wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
}

wxColor getLightHighlightColor()
{
    return stepColor(getHighlightColor(), 170);
}

wxColor getCaptionColor()
{
    return stepColor(getBaseColor(), 25);
}

wxBrush getBaseBrush()
{
    return wxBrush(getBaseColor());

}

wxPen getBorderPen()
{
    return wxPen(getBorderColor());
}



#if defined(__WXMSW__)
void* getHdcFrom(wxDC& dc)
{
    #if wxCHECK_VERSION(2,9,0)
    wxMSWDCImpl* msw_dc = (wxMSWDCImpl*)dc.GetImpl();
    return (void*)msw_dc->GetHDC();
    #else
    return (void*)dc.GetHDC();
    #endif
}
#else
void* getHdcFrom(wxDC& dc)
{
    return (void*)0;
}
#endif



};  // namespace kcl


