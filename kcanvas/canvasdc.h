/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2009-05-21
 *
 */


#ifndef __KCANVAS_CANVASDC_H
#define __KCANVAS_CANVASDC_H


#include "event.h"


namespace kcanvas
{


// Canvas Device Context Implementation

class CanvasDC : public Canvas
{
    XCM_CLASS_NAME_NOREFCOUNT("kcanvas.CanvasDC")
    XCM_BEGIN_INTERFACE_MAP(CanvasDC)
        XCM_INTERFACE_CHAIN(Canvas)
    XCM_END_INTERFACE_MAP()

public:

    CanvasDC();
    virtual ~CanvasDC();

    static ICanvasPtr create();
    void destroy();

    // canvas clone/copy functions
    ICanvasPtr clone();
    void copy(ICanvasPtr canvas);

    // drawing related functions
    bool setPen(const Pen& pen);
    bool setPen(const Properties& properties);

    bool setBrush(const Brush& brush);
    bool setBrush(const Properties& properties);

    bool setFont(const Font& font);
    bool setTextBackground(const Color& color);
    bool setTextForeground(const Color& color);

    void drawImage(const wxImage& image,
                    int x, int y, int w, int h);

    void drawCheckBox(int x, int y, int w, int h, bool checked = true);
    void drawCircle(int x, int y, int r);
    void drawEllipse(int x, int y, int w, int h);
    void drawLine(int x1, int  y1, int x2, int y2);
    void drawPoint(int x, int y);
    void drawPolygon(int n, wxPoint points[], int x = 0, int y = 0);
    void drawGradientFill(const wxRect& rect,
                          const Color& start_color,
                          const Color& end_color,
                          const wxDirection& direction = wxSOUTH);
    void drawRectangle(int x, int y, int w, int h);
    void drawText(const wxString& text, int x, int y);
    
    bool getTextExtent(const wxString& text,
                       int* x,
                       int* y,
                       int* descent = NULL,
                       int* external = NULL) const;
                       
    bool getPartialTextExtents(const wxString& text,
                               wxArrayInt& widths) const;
                               
    int getCharHeight() const;
    int getCharWidth() const;

    void addClippingRectangle(int x, int y, int w, int h);
    void removeClippingRectangle();
    void removeAllClippingRectangles();

protected:

    void setDC(wxDC* dc);
    wxDC* getDC();

private:

    // clipping region update function
    void updateClippingRectangle();

private:

    // clipping rectangles
    std::vector<wxRect> m_clipping_rects;

    // variables for current state of drawing options for 
    // device context
    wxDC* m_draw_dc;

    // cache of the font used to draw
    wxFont* m_draw_font;
    
    // TODO: reactivate
    //wxBitmap m_bmp_checkbox_checked;
    //wxBitmap m_bmp_checkbox_unchecked;
};


}; // namespace kcanvas


#endif

