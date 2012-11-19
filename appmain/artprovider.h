/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   David Z. Williams
 * Created:  2007-04-03
 *
 */


#ifndef __CFW_ARTPROVIDER_H
#define __CFW_ARTPROVIDER_H


#include <wx/aui/aui.h>



class CfwDockArt : public wxAuiDefaultDockArt
{
public:

    void DrawPaneButton(wxDC& dc,
                        wxWindow* window,
                        int button,
                        int button_state,
                        const wxRect& rect,
                        wxAuiPaneInfo& pane);

    void DrawCaption(wxDC& dc,
                     wxWindow *WXUNUSED(window),
                     const wxString& text,
                     const wxRect& rect,
                     wxAuiPaneInfo& pane);
};


class CfwTabArt : public wxAuiDefaultTabArt
{
public:

    void DrawBackground(
                 wxDC& dc,
                 wxWindow* wnd,
                 const wxRect& rect);

    void DrawButton(
                 wxDC& dc,
                 wxWindow* wnd,
                 const wxRect& in_rect,
                 int bitmap_id,
                 int button_state,
                 int orientation,
                 wxRect* out_rect);

    void DrawTab(wxDC& dc,
                 wxWindow* wnd,
                 const wxAuiNotebookPage& page,
                 const wxRect& in_rect,
                 int close_button_state,
                 wxRect* out_tab_rect,
                 wxRect* out_button_rect,
                 int* x_extent);

    void SetSizingInfo(const wxSize& tab_ctrl_size,
                       size_t tab_count);
                                  
    wxSize GetTabSize(
                 wxDC& dc,
                 wxWindow* WXUNUSED(wnd),
                 const wxString& caption,
                 const wxBitmap& bitmap,
                 bool WXUNUSED(active),
                 int close_button_state,
                 int* x_extent);

    int GetIndentSize();

    wxAuiTabArt* Clone();
};



#endif  // __CFW_ARTPROVIDER_H

