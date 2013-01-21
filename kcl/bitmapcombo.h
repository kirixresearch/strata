/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams
 * Created:  2007-11-14
 *
 */


#ifndef __KCL_BITMAPCOMBO_H
#define __KCL_BITMAPCOMBO_H


#include <wx/combo.h>
#include <wx/listctrl.h>
#include <map>


namespace kcl
{


class BitmapComboEvent : public wxNotifyEvent
{
public:

    BitmapComboEvent(wxEventType command_type = wxEVT_NULL,
                     int win_id = 0)
            : wxNotifyEvent(command_type, win_id)
    {

    }

    BitmapComboEvent(const BitmapComboEvent& c) : wxNotifyEvent(c)
    {

    }
};




class BitmapComboPopup : public wxListCtrl,
                         public wxComboPopup
{
public:

    enum BitmapComboPopupOptions
    {
        ShowItemBitmap = 0x01
    };
    
    // wxComboPopup overrides
    
    virtual void Init();
    virtual bool Create(wxWindow* parent);
    
    virtual wxWindow* GetControl();
    
    virtual void SetStringValue(const wxString& s);
    virtual wxString GetStringValue() const;
    
    void PaintComboControl(wxDC& dc, const wxRect& rect);

public:

    void setOptions(int options);
    void setDefaultBitmap(const wxBitmap& bitmap);
    
    void setItemBitmap(int idx, const wxBitmap& bitmap);
    wxBitmap getItemBitmap(int idx);
    wxBitmap getBitmap();
    
    void insert(const wxString& label,
                const wxBitmap& bitmap,
                int idx = -1);
    int getSelection() const;
    
private:

    void onItemSelected(wxListEvent& evt);
    void onMouseMove(wxMouseEvent& evt);
    void onSize(wxSizeEvent& evt);

private:
    
    wxBitmap m_default_bmp;          // default bitmap to show in the control
    int m_absolute_max_popup_height; // absolute maximum popup height
    int m_selection_idx;             // selected item index
    int m_options;                   // options flag for this control
    
    // for some reason there are artifacts in the bitmaps we try to get
    // from the imagelist, so we'll store our own list of bitmaps mapped
    // to the imagelist index
    std::map<int,wxBitmap> m_bitmaps;
    
    DECLARE_EVENT_TABLE()
};




class BitmapComboControl : public wxComboCtrl
{
public:

    BitmapComboControl(wxWindow* parent,
                       wxWindowID id = wxID_ANY,
                       BitmapComboPopup* popup = NULL);
    
    void setOverlayText(const wxString& val);
    wxString getOverlayText() const;
    
    void setSingleClickSelect(bool select = true);
    void setDefaultBitmap(const wxBitmap& bitmap);
    void setShowBitmap(bool show = true);
    
    void setItemBitmap(size_t idx, const wxBitmap& bitmap);
    
    // wxBitmapComboBox compatibility
    void SetItemBitmap(size_t idx, const wxBitmap& bitmap) { setItemBitmap(idx,bitmap); }
    
    wxBitmap getItemBitmap(size_t idx) const;
    wxBitmap getBitmap() const;
    
    // pass through functions for BitmapComboPopup
    void Insert(const wxString& label,
                const wxBitmap& bitmap,
                size_t idx);
    int FindString(const wxString& string,
                   bool case_sensitive = false);
    int GetSelection() const;
    
    // wxComboCtrl overrides
    void SetValue(const wxString& value);
    wxString GetValue() const;
    
    // event handlers
    void onMouseLeftDown(wxMouseEvent& evt);
    void onMouseLeftUp(wxMouseEvent& evt);
    void onMouseMove(wxMouseEvent& evt);
    void onSetFocus(wxFocusEvent& evt);
    void onKillFocus(wxFocusEvent& evt);
    void onSize(wxSizeEvent& evt);

private:

    void recalcCustomPaintWidth();
    void showOverlayText(bool show);
    bool isOverlayTextShown() const;
    
private:

    BitmapComboPopup* m_popup;
    wxString m_overlay_text;
    bool m_dragging;
    bool m_had_focus;
    bool m_show_bitmap;
    bool m_single_click_select;
    bool m_in_mouse_left_down;
    bool m_overlay_text_shown;
    int m_cli_width;
    int m_cli_height;
    int m_drag_x_threshold;
    int m_drag_y_threshold;
    wxPoint m_action_pos;
    
    DECLARE_EVENT_TABLE()
};




};  // namespace kcl




// event machinery

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_KCLBITMAPCOMBO_BEGIN_DRAG, 2200)
END_DECLARE_EVENT_TYPES()


typedef void (wxEvtHandler::*wxKclBitmapComboEventFunction)(kcl::BitmapComboEvent&);


#define EVT_KCLBITMAPCOMBO_BEGIN_DRAG(winid, fn)              DECLARE_EVENT_TABLE_ENTRY(wxEVT_KCLBITMAPCOMBO_BEGIN_DRAG, winid, -1, (wxObjectEventFunction)(wxEventFunction)(wxKclBitmapComboEventFunction)&fn, NULL),


#endif  // __KCL_BITMAPCOMBO_H



