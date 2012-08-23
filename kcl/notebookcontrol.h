/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Aaron L. Williams
 * Created:  2002-04-09
 *
 */


#ifndef __KCL_NOTEBOOKCONTROL_H
#define __KCL_NOTEBOOKCONTROL_H


#include <wx/wx.h>
#include <vector>
#include "button.h"


namespace kcl
{


class NotebookPage
{
public:
    wxWindow* m_window;
    wxString m_text;
    wxBitmap m_bitmap;
    wxRect m_hitrect;
    int m_tab_offset;
    int m_tab_width;

    NotebookPage()
    {
        m_window = NULL;
        m_text = wxT("");
        m_tab_offset = 0;
        m_tab_width = 0;
    }

    NotebookPage(const NotebookPage& c)
    {
        m_window = c.m_window;
        m_text = c.m_text;
        m_bitmap = c.m_bitmap;
        m_hitrect = c.m_hitrect;
        m_tab_offset = c.m_tab_offset;
        m_tab_width = c.m_tab_width;
    }
};

enum
{
    notebookScrollOff = 0,
    notebookScrollOn = 1,
    notebookScrollAuto = 2
};

class NotebookControl : public wxControl
{

public:

    NotebookControl(wxWindow* parent,
                    wxWindowID id = -1,
                    const wxPoint& position = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = wxCLIP_CHILDREN | wxNO_BORDER);

    virtual ~NotebookControl();

    void setScrollMode(int scroll_mode);

    void setExtraPanel(wxPanel* extra_panel);
    wxPanel* getExtraPanel();
    void setTabHeight(int tab_height);

    bool create(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
        const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxT("notebook"));

    bool addPage(wxWindow* window, const wxString& text, const wxBitmap& bitmap = wxNullBitmap, bool select = false);
    bool insertPage(int index, wxWindow* window, const wxString& text, const wxBitmap& bitmap = wxNullBitmap, bool select = false);
    bool deletePage(int page);
    bool deleteAllPages();


    wxWindow* getPage(int page);
    int getPageCount();

    bool setSelection(int page);
    int getSelection();

    bool setPageText(int page, const wxString& text);
    wxString getPageText(int page);

    bool setPageBitmap(int page, const wxBitmap& bitmap);
    wxBitmap getPageBitmap(int page);

    void setInactiveBackground(const wxBrush& brush);
    void setActiveBackground(const wxBrush& brush);

    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onMouse(wxMouseEvent& event);

    void onScrollLeft(wxCommandEvent& event);
    void onScrollRight(wxCommandEvent& event);

public: // signals 

    xcm::signal1<int> sigActivePageChanged;
    xcm::signal1<int> sigTabRightClicked;

private:

    std::vector<NotebookPage> m_pages;
    
    int m_notebook_tab_start;
    int m_active_index;
    int m_tab_height;
    int m_user_tab_height;
    int m_total_tab_width;
    int m_extra_panel_width;
    int m_xoff;
    int m_current_scroll_page;
    int m_scroll_mode;

    wxBrush m_brush_inactive;
    wxBrush m_brush_active;
    wxPanel* m_extra_panel;
    wxPanel* m_tabscroll_panel;

    kcl::Button* m_scroll_left_button;
    kcl::Button* m_scroll_right_button;


    void drawTab(wxDC& dc, int tab);
    void calcTabHeight();
    void calcTabWidths();
    void doSizing();


    DECLARE_EVENT_TABLE()
};



};


#endif

