/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2002-02-19
 *
 */


#ifndef H_KCL_BUTTON_H
#define H_KCL_BUTTON_H


#include <wx/wx.h>
#include <kl/signal.h>


class wxFont;


namespace kcl
{


class Button : public wxControl
{
public:

    //for overall alignment of bitmap and text in the button
    enum
    {
        alignCenter = 0,
        alignLeft   = 1,
        alignRight  = 2,
        alignTop    = 3,
        alignBottom = 4
    };

    enum
    {
        bitmapLeft   = 0,
        bitmapRight  = 1,
        bitmapTop    = 2,
        bitmapBottom = 3
    };

    enum
    {
        modeFlat       = 0,
        mode3d         = 1,
        modeCompatible = 2,
        modeXP         = 3
    };

    enum
    {
        colorNormalFg   = 0,
        colorNormalBg   = 1,
        colorHoverFg    = 2,
        colorHoverBg    = 3,
        colorSelectedFg = 4,
        colorSelectedBg = 5
    };
    
    enum CustomRenderFlags
    {
        buttonEnabled   = 1 << 0,
        buttonDepressed = 1 << 1,
    };
    
    static int getDefaultMode();
    static void setDefaultMode(int default_mode);

    Button(wxWindow* parent,
           wxWindowID id = -1,
           const wxPoint& position = wxDefaultPosition,
           const wxSize& size = wxDefaultSize,
           const wxString& text = wxEmptyString,
           const wxBitmap& bitmap = wxNullBitmap,
           const wxBitmap& disabled_bitmap = wxNullBitmap);

    bool SetBackgroundColour(const wxColour& color);

    int getHeight();
    int getWidth();
    bool getDepressed();

    virtual void setText(const wxString& text);
    virtual void setBitmap(const wxBitmap& bitmap);
    virtual wxBitmap getBitmap();
    virtual wxString getText();
    
    wxColor getColor(int color_idx);
    wxBitmap getDropDownBitmap();
    
    void setFont(const wxFont& font);
    void setTextVisible(bool state);
    void setBitmapVisible(bool state);
    void setToggleMode(bool state);
    void setDepressed(bool state);
    void setDropDown(bool state);
    void setRightClickAllowed(bool state);
    void setIndent(bool state);
    void setBitmapLocation(int location);
    void setAlignment(int align);
    void setDimensions(int width, int height);
    void setTooltip(const wxString& tip);
    void setMode(int state);
    void setSpacer(int spacer);
    void setXOffset(int x_offset); // only applies for left and right alignment
    void setColor(int color_idx, const wxColour& color);
    void makeMinSize();

    bool Enable(bool state = true);

public: // signals

    // custom rendering hook
    xcm::signal3<Button*, wxDC*, int /* flags */> sigCustomRender;

    // button pressed signal
    //     parameters = this, button number (1 = left, 2 = middle, 3 = right)
    xcm::signal2<Button*, int> sigButtonClicked;

protected:

    virtual void onDropDownClicked() { }
    virtual void onButtonClicked() { }

private:

    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onMove(wxMoveEvent& event);
    void onMouseButtonDown(wxMouseEvent& event);
    void onMouseButtonUp(wxMouseEvent& event);
    void onMouseEnterWindow(wxMouseEvent& event);
    void onMouseLeaveWindow(wxMouseEvent& event);
    void onMouseMotion(wxMouseEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    bool SendClickEvent();
    void drawButtonFrame(wxDC& dc,
                         int x,
                         int y,
                         int width,
                         int height,
                         bool pushed_in,
                         bool button_down,
                         bool mouse_over);

private:

    wxFont  m_font;
    wxPen   m_darkgrey_pen;
    wxPen   m_grey_pen;
        
    wxBrush m_bk_brush;
    wxPen   m_bk_pen;
    wxPen   m_hover_pen;
    wxBrush m_hover_brush;
    wxPen   m_sel_pen;
    wxBrush m_sel_brush;

    wxBitmap m_bitmap;
    wxBitmap m_disabled_bitmap;
    wxBitmap m_shadow_bitmap;
    wxBitmap m_downarrow_bitmap;

    wxString m_text;
    wxCoord  m_text_width;
    wxCoord  m_text_height;
    
    int  m_mode;
    int  m_alignment;
    int  m_bitmap_location;
    int  m_spacer;
    int  m_x_off;

    bool m_text_visible;
    bool m_bitmap_visible;
    bool m_button_down;
    bool m_dropdown_down;
    bool m_pushed_in;
    bool m_mouse_over;
    bool m_enabled;
    bool m_toggle_mode;
    bool m_right_click_allowed;
    bool m_drop_down;
    bool m_indent;

    // this window does not accept focus, and should
    // not be included in tab traversal lists
    bool AcceptsFocus() const { return FALSE; }

    wxBitmap getDisabledBitmap();
    wxBitmap getShadowBitmap();

    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(kcl::Button)
};



}; // namespace kcl


#endif

