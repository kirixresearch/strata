/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  GUI Control Library
 * Author:   Aaron L. Williams
 * Created:  2003-04-08
 *
 */


#ifndef __KCL_RULERCONTROL_H
#define __KCL_RULERCONTROL_H


#define RULER_MODEL_PPI 1440
#define RULER_SCREEN_PPI 96


namespace kcl
{

class RulerGuide
{
public:

    int m_pos;
    wxBitmap m_bitmap;

    RulerGuide()
    {
        m_pos = 0;
    }

    RulerGuide(const RulerGuide& c)
    {
        m_pos = c.m_pos;
        m_bitmap = c.m_bitmap;
    }

    RulerGuide& operator=(const RulerGuide& c)
    {
        m_pos = c.m_pos;
        m_bitmap = c.m_bitmap;
        return *this;
    }

    bool operator==(const RulerGuide& c)
    {
        if (m_pos == c.m_pos &&
            m_bitmap.IsSameAs(c.m_bitmap))
        {
            return true;
        }

        return false;
    }
};



class RulerControl : public wxControl     
{
    
public:

    RulerControl(wxWindow* parent,
                 wxWindowID id = -1,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize);
                 
    virtual ~RulerControl();

    void setScale(int scale);
    int getScale();

    void setDimensions(int width, int length, int offset);
    void getDimensions(int* width, int* length, int* offset);

    void setOrigin(int origin);
    int getOrigin();

    void setTickInterval(int number, int major, int minor);
    void getTickInterval(int* number, int* major, int* minor);

    void addGuide(const RulerGuide& guide);
    void removeGuide(const RulerGuide& guide);
    RulerGuide getGuideByIdx(int guide_idx);
    int getGuideCount();

    void render();
    void repaint(wxDC* dc = NULL);

    void onSize(wxSizeEvent& evt);
    void onPaint(wxPaintEvent& evt);
    void onMouse(wxMouseEvent& evt);

public: // signals

    xcm::signal2<RulerControl*, int> sigMoveGuideStart;
    xcm::signal2<RulerControl*, int> sigMoveGuideEnd;
    xcm::signal2<RulerControl*, int> sigMoveGuide;

    // --signal for returning focus to another window --
    xcm::signal0 sigReturnFocus;
    
private:

    enum
    {
        actionNone = 0,
        actionMoveOrigin = 1
    };

    // -- Bitmap variables --
    wxBitmap m_bmp;
    wxMemoryDC m_memdc;
    wxBrush m_bkbrush;
    wxPen m_bkpen;
    int m_bmp_alloc_width;
    int m_bmp_alloc_height;

    // -- Mouse event variables --
    int m_mouse_action;             // action indicator
    int m_mouse_xlast;              // last x pos of the mouse
    int m_mouse_ylast;              // last y pos of the mouse
    int m_mouse_x;                  // x pos of the mouse
    int m_mouse_y;                  // y pos of the mouse
    int m_action_x;                 // x pos of the mouse at start of action
    int m_action_y;                 // y pos of the mouse at start of action

    int m_scale;                    // scale in dpi between model and screen

    int m_length;                   // length of ruler in screen coordinates
    int m_width;                    // width of ruler in screen coordinates
    int m_offset;                   // beginning of ruler scale in screen coordinates

    int m_origin;                   // location of origin in model coordinates
    int m_origin_old;               // old location of origin in model coordinates

    int m_number_tick_interval;     // interval between numbers in model coordinates
    int m_major_tick_interval;      // interval between major ticks in model coordinates
    int m_minor_tick_interval;      // interval between minor ticks in model coordinates

    std::vector<RulerGuide> m_guides;

    void allocBitmap(int width, int height);

    int mtod(int x);                // model to screen (device) conversion function
    int dtom(int x);                // screen (device) to model conversion function

    DECLARE_EVENT_TABLE()
};



};  // namespace kcl


#endif
