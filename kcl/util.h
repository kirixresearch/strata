/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Benjamin I. Williams
 * Created:  2006-11-16
 *
 */


#ifndef H_KCL_UTIL_H
#define H_KCL_UTIL_H


namespace kcl
{

#ifdef __WXGTK__
const int DEFAULT_BORDER = wxSUNKEN_BORDER;
#else
const int DEFAULT_BORDER = wxBORDER_THEME;
#endif

double blendColor(double fg, double bg, double alpha);
wxColor stepColor(const wxColor& c, int ialpha);
wxColor getBaseColor();

wxColor getBorderColor();
wxColor getHighlightColor();
wxColor getLightHighlightColor();
wxColor getCaptionColor();
wxBrush getBaseBrush();
wxPen getBorderPen();

void* getHdcFrom(wxDC& dc); // retrieve HDC from wxDC

};  // namespace kcl


#endif

