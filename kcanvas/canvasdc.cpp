/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2009-05-21
 *
 */


#include "kcanvas_int.h"
#include "canvas.h"
#include "canvasdc.h"
#include "util.h"


namespace kcanvas
{


inline bool isPrinterDC(wxDC* dc)
{
    if (!dc)
        return false;

#ifdef WIN32
    if (dc->IsKindOf(CLASSINFO(wxPrinterDC)))
        return true;
#endif

#if wxUSE_POSTSCRIPT
    if (dc->IsKindOf(CLASSINFO(wxPostScriptDC)))
        return true;
#endif

    return false;
}


// canvas implementation
CanvasDC::CanvasDC()
{
    // set the device context to NULL; implementations that
    // want to utilize the default canvas drawing functions
    // must provide a suitable DC to the canvas using setDC();
    // the reason for this is that a canvas implementation,
    // such as the PDF canvas, doesn't always output to a DC,
    // but instead may override the drawing functions to output
    // to something else; as a result, it's inefficient to
    // always initialize the canvas with a memory DC when it
    // might not even be needed
    m_draw_dc = NULL;

    // set the font; deleted in destructor
    m_draw_font = new wxFont(10, wxSWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
}

CanvasDC::~CanvasDC()
{
    delete m_draw_font;
}

ICanvasPtr CanvasDC::create()
{
    return static_cast<ICanvas*>(new CanvasDC);
}

void CanvasDC::destroy()
{
    // since a canvas isn't reference counted, we need some way
    // to destroy a canvas if it isn't destroyed by an owner
    // object; for example, CanvasControl is automatically
    // destroyed because it also derives from wxControl;
    // however CanvasPDF and CanvasPrintout aren't automatically
    // destroyed, so we need a way to allow for this
    delete this;
}

ICanvasPtr CanvasDC::clone()
{
    CanvasDC* c = new CanvasDC;
    c->copy(this);

    return static_cast<ICanvas*>(c);
}

void CanvasDC::copy(ICanvasPtr canvas)
{    
    if (canvas.isNull())
        return;

    // copy the base canvas elements
    Canvas::copy(canvas);
}

bool CanvasDC::setPen(const Pen& pen)
{
    if (!m_draw_dc)
        return false;

    wxPen wxpen;
    wxpen.SetColour(towxcolor(pen.getColor()));
    wxpen.SetWidth(pen.getWidth());
    wxpen.SetStyle(pen.getStyle());

    #if wxCHECK_VERSION(2,9,0)
        wxpen.SetCap((wxPenCap)pen.getCap());
        wxpen.SetJoin((wxPenJoin)pen.getJoin());
    #else
        wxpen.SetCap(pen.getCap());
        wxpen.SetJoin(pen.getJoin());
    #endif

    // set the draw pen
    m_draw_dc->SetPen(wxpen);
    return true;
}

bool CanvasDC::setPen(const Properties& properties)
{
    if (!m_draw_dc)
        return false;

    wxPen pen;

    // note: this function sets the pen from a property list
    PropertyValue value;

    // set the join and cap style; TODO: determine if we should
    // expose these as properties; if not, we can set these globally
    // in the constructor, provided we take care to reset them if
    // the pen is set via the other setPen() function    
    pen.SetCap(wxCAP_BUTT);
    pen.SetJoin(wxJOIN_MITER);
    
    // set the line width
    if (properties.get(PROP_LINE_WIDTH, value))
        pen.SetWidth(wxMax(1, mtod_x(value.getInteger())));
    
    // set the pen color
    if (properties.get(PROP_COLOR_FG, value))
    {
        // if we have a null color, set the pen to transparent;
        // otherwise, set the style to solid
        const Color& c = value.getColor();
        if (c == COLOR_NULL)
        {
            pen.SetColour(*wxBLACK);
            pen.SetStyle(wxPENSTYLE_TRANSPARENT);
        }
         else
        {
            pen.SetColour(towxcolor(c));
            pen.SetStyle(wxPENSTYLE_SOLID);
        }
    }

    // set the dc pen
    m_draw_dc->SetPen(pen);
    return true;
}

bool CanvasDC::setBrush(const Brush& brush)
{
    if (!m_draw_dc)
        return false;

    wxBrush wxbrush;
    wxbrush.SetColour(towxcolor(brush.getColor()));
    wxbrush.SetStyle(brush.getStyle());

    // set the draw brush    
    m_draw_dc->SetBrush(wxbrush);
    return true;
}

bool CanvasDC::setBrush(const Properties& properties)
{
    if (!m_draw_dc)
        return false;

    wxBrush brush;

    // note: this function sets the brush from a property list
    PropertyValue value;
    
    // set the brush color
    if (properties.get(PROP_COLOR_BG, value))
    {
        // if we have a null color, set the brush to transparent
        // otherwise, set the style to solid
        const Color& c = value.getColor();
        if (c == COLOR_NULL)
        {
            brush.SetColour(*wxBLACK);
            brush.SetStyle(wxBRUSHSTYLE_TRANSPARENT);
        }
         else
        {
            brush.SetColour(towxcolor(c));
            brush.SetStyle(wxBRUSHSTYLE_SOLID);
        }
    }

    // set the brush
    m_draw_dc->SetBrush(brush);
    return true;
}

bool CanvasDC::setFont(const Font& font)
{
    if (!m_draw_dc)
        return false;

    // if the font isn't ok, don't try to set anything
    if (!m_draw_font->IsOk())
        return false;

    // set the cached font from the font settings
    m_draw_font->SetFaceName(font.getFaceName());
    m_draw_font->SetPointSize(font.getSize());
    
    wxString style = font.getStyle();
    if (style == kcanvas::FONT_STYLE_NORMAL)
        m_draw_font->SetStyle(wxFONTSTYLE_NORMAL);
    if (style == kcanvas::FONT_STYLE_ITALIC)
        m_draw_font->SetStyle(wxFONTSTYLE_ITALIC);

    wxString weight = font.getWeight();
    if (weight == kcanvas::FONT_WEIGHT_NORMAL)
        m_draw_font->SetWeight(wxFONTWEIGHT_NORMAL);
    if (weight == kcanvas::FONT_WEIGHT_BOLD)
        m_draw_font->SetWeight(wxFONTWEIGHT_BOLD);    

    wxString underscore = font.getUnderscore();
    if (underscore == kcanvas::FONT_UNDERSCORE_NORMAL)
        m_draw_font->SetUnderlined(false);
    if (underscore == kcanvas::FONT_UNDERSCORE_LINE)
        m_draw_font->SetUnderlined(true);

    // get the zoom
    int zoom = getZoom();

    // if the zoom is active, scale the font
    if (zoom != 100)
    {
        // adjust the font for zoom size
        wxSize size = m_draw_font->GetPixelSize();
        double scale = static_cast<double>(zoom)/100;
        size.Scale(scale, scale);
        m_draw_font->SetPixelSize(size);
    }

#ifdef WIN32

    // wxFont under WIN32 is implemented incorrectly.  It
    // uses screen dimensions to determine DPI and thus
    // generates the wrong font size.  We compensate here
    // by adjusting the pixel size of the font; note: in
    // previous implementations (textrender), we created
    // a native font, but here we don't want to create
    // a font since this function is called fairly often

    if (isPrinterDC(m_draw_dc))
    {
        int canvas_scale_x, canvas_scale_y;
        getScale(&canvas_scale_x, &canvas_scale_y);

        wxSize size = m_draw_font->GetPixelSize();
        double scale_x = static_cast<double>(canvas_scale_x)/CANVAS_SCREEN_DPI;
        double scale_y = static_cast<double>(canvas_scale_y)/CANVAS_SCREEN_DPI;
        size.Scale(scale_x, scale_y);
        m_draw_font->SetPixelSize(size);
    }
#endif

    // set the font
    m_draw_dc->SetFont(*m_draw_font);
    return true;
}

bool CanvasDC::setTextBackground(const Color& color)
{
    if (!m_draw_dc)
        return false;

    m_draw_dc->SetTextBackground(towxcolor(color));
    return true;
}

bool CanvasDC::setTextForeground(const Color& color)
{
    if (!m_draw_dc)
        return false;

    m_draw_dc->SetTextForeground(towxcolor(color));
    return true;
}

void CanvasDC::drawImage(const wxImage& image,
                        int x, int y, int w, int h)
{
    // TODO: handle transparent images

    if (!m_draw_dc)
        return;

    if (!image.Ok())
        return;

    x += m_draw_origin_x;
    y += m_draw_origin_y;

    wxImage image_copy = image;
    wxBitmap bitmap(image_copy.Scale(mtod_x(w), mtod_y(h), wxIMAGE_QUALITY_HIGH));
    m_draw_dc->DrawBitmap(bitmap, mtod_x(x), mtod_y(y), false);
}

void CanvasDC::drawCheckBox(int x, int y, int w, int h, bool checked)
{
    if (!m_draw_dc)
        return;

    // TODO: need to reimplement; commented out when underiving Canvas 
    // from wxControl; need a way to get the renderer
    
    /*

    // note: draw a checkbox centered inside the rectangle defined
    // by x, y, w, h

    // if we haven't created the checked checkbox bitmap, do so now    
    if (!m_bmp_checkbox_checked.IsOk())
    {
        m_bmp_checkbox_checked.Create(50, 50, -1);
        wxMemoryDC memdc;
        memdc.SelectObject(m_bmp_checkbox_checked);
        wxRendererNative::Get().DrawCheckBox(this, memdc, wxRect(0,0,50,50), wxCONTROL_CHECKED);
        
        int xc, yc, wc, hc;
        getBitmapContentArea(m_bmp_checkbox_checked, COLOR_BLACK, &xc, &yc, &wc, &hc);
        memdc.Blit(0, 0, wc, hc, &memdc, xc, yc);

        // commented out because it doesn't build on linux
        //m_bmp_checkbox_checked.SetSize(wc, hc);
    }

    // if we haven't created the unchecked checkbox bitmap, do so now
    if (!m_bmp_checkbox_unchecked.IsOk())
    {
        m_bmp_checkbox_unchecked.Create(50, 50, -1);
        wxMemoryDC memdc;
        memdc.SelectObject(m_bmp_checkbox_unchecked);
        wxRendererNative::Get().DrawCheckBox(this, memdc, wxRect(0,0,50,50), 0);

        int xc, yc, wc, hc;
        getBitmapContentArea(m_bmp_checkbox_unchecked, COLOR_BLACK, &xc, &yc, &wc, &hc);
        memdc.Blit(0, 0, wc, hc, &memdc, xc, yc);

        // commented out because it doesn't build on linux
        //m_bmp_checkbox_unchecked.SetSize(wc, hc);
    }

    // calculate the offset to add to the draw origin to center the bitmap
    // in the given region
    int x_offset = 0;
    int y_offset = 0;

    if (checked)
    {
        x_offset = (w - dtom_x(m_bmp_checkbox_checked.GetWidth()))/2;
        y_offset = (h - dtom_y(m_bmp_checkbox_checked.GetHeight()))/2;
    }
    else
    {
        x_offset = (w - dtom_x(m_bmp_checkbox_unchecked.GetWidth()))/2;
        y_offset = (h - dtom_y(m_bmp_checkbox_unchecked.GetHeight()))/2;
    }

    // if the input width is less than zero, set the x_offset to zero
    if (w < 0)
        x_offset = 0;

    // if the input height is less than zero, set the y_offset to zero        
    if (y < 0)
        y_offset = 0;

    // set the draw origin
    x += m_draw_origin_x;
    y += m_draw_origin_y;

    // draw the bitmap]
    if (checked)
    {
        wxImage image = m_bmp_checkbox_checked.ConvertToImage();
        drawImage(image, x + x_offset, y + y_offset, wc, hc);
    }
    else
    {
        wxImage image = m_bmp_checkbox_unchecked.ConvertToImage();
        drawImage(image, x + x_offset, y + y_offset, wc, hc);
    }
    
    */
}

void CanvasDC::drawCircle(int x, int y, int r)
{
    if (!m_draw_dc)
        return;

    x += m_draw_origin_x;
    y += m_draw_origin_y;

    m_draw_dc->DrawCircle(mtod_x(x), mtod_y(y), mtod_x(r));
}

void CanvasDC::drawEllipse(int x, int y, int w, int h)
{
    if (!m_draw_dc)
        return;

    x += m_draw_origin_x;
    y += m_draw_origin_y;

    m_draw_dc->DrawEllipse(mtod_x(x), mtod_y(y), mtod_x(w), mtod_y(h));
}

void CanvasDC::drawLine(int x1, int y1, int x2, int y2)
{
    if (!m_draw_dc)
        return;

    x1 = mtod_x(x1 + m_draw_origin_x);
    y1 = mtod_y(y1 + m_draw_origin_y);
    x2 = mtod_x(x2 + m_draw_origin_x);
    y2 = mtod_y(y2 + m_draw_origin_y);

    m_draw_dc->DrawLine(x1, y1, x2, y2);
}

void CanvasDC::drawPoint(int x, int y)
{
    if (!m_draw_dc)
        return;

    x += m_draw_origin_x;
    y += m_draw_origin_y;

    m_draw_dc->DrawPoint(mtod_x(x), mtod_y(y));
}

void CanvasDC::drawPolygon(int n, wxPoint points[], int x, int y)
{
    if (!m_draw_dc)
        return;

    wxPoint* copy = new wxPoint[n];
    for (int i = 0; i < n; ++i)
    {
        copy[i].x = mtod_x(points[i].x + m_draw_origin_x);
        copy[i].y = mtod_y(points[i].y + m_draw_origin_y);
    }

    m_draw_dc->DrawPolygon(n, copy, mtod_x(x), mtod_y(y));
    delete[] copy;
}

void CanvasDC::drawGradientFill(const wxRect& rect,
                              const Color& start_color,
                              const Color& end_color,
                              const wxDirection& direction)
{
    if (!m_draw_dc)
        return;

    // if the width or height of the rectangle is zero,
    // don't draw anything
    if (rect.width == 0 || rect.height == 0)
        return;

    // convert the rectangle to device coordinates
    int x1, y1, x2, y2;
    x1 = mtod_x(rect.x + m_draw_origin_x);
    y1 = mtod_y(rect.y + m_draw_origin_y);
    x2 = mtod_x(rect.x + rect.width + m_draw_origin_x);
    y2 = mtod_y(rect.y + rect.height + m_draw_origin_y);

    wxRect dc_rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    m_draw_dc->GradientFillLinear(dc_rect,
                                  towxcolor(start_color),
                                  towxcolor(end_color),
                                  direction);
}

void CanvasDC::drawRectangle(int x, int y, int w, int h)
{
    if (!m_draw_dc)
        return;

    // if the width or height of the rectangle is zero,
    // don't draw anything
    if (w == 0 || h == 0)
        return;

    // note: rectangles with positive widths and heights seem
    // to draw exactly; ones with negative widths or heights used
    // to be off by a pixel, even when the absolute coordinates
    // (x1, y1, x2, y2) are the same as those of the line, both
    // before and after conversion; this was fixed, but not tested
    // with the following code; TODO: test - any changes should also
    // be reflected in clipping rectangle code

    // if the width or height are less than zero, fix the 
    // rectangle so it expressed in positive coordinates
    if (w < 0) {x = x + w; w = -w;}
    if (h < 0) {y = y + h; h = -h;}

    // convert the rectangle to device coordinates
    int x1, y1, x2, y2;
    x1 = mtod_x(x + m_draw_origin_x);
    y1 = mtod_y(y + m_draw_origin_y);
    x2 = mtod_x(x + w + m_draw_origin_x);
    y2 = mtod_y(y + h + m_draw_origin_y);

    m_draw_dc->DrawRectangle(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

void CanvasDC::drawText(const wxString& text, int x, int y)
{
    if (!m_draw_dc)
        return;

    // if we don't have a valid font, don't draw text
    if (!m_draw_font->IsOk())
        return;

    x += m_draw_origin_x;
    y += m_draw_origin_y;

    m_draw_dc->DrawText(text, mtod_x(x), mtod_y(y));
}

bool CanvasDC::getTextExtent(const wxString& text,
                           int* x,
                           int* y,
                           int* descent,
                           int* external) const
{
    // TODO: need to make the model dimensions this function returns 
    // depend only on scale_x, scale_y and zoom, even if the device 
    // context changes so that algorithms that rely on text extent will 
    // work the same on the screen and on printers; note: so far, we
    // haven't run into many problems, so this may be ok; research
    
    if (!m_draw_dc)
        return false;

    int d, e;
    m_draw_dc->GetTextExtent(text, x, y, &d, &e);

    if (descent != NULL)
    {
        *descent = d;
        *descent = dtom_y(*descent);
    }
    
    if (external != NULL)
    {
        *external = e;
        *external = dtom_y(*external);
    }

    *x = dtom_x(*x);
    *y = dtom_y(*y);
    
    return true;
}

bool CanvasDC::getPartialTextExtents(const wxString& text,
                                   wxArrayInt& widths) const
{
    // TODO: need to make the model dimensions this function returns 
    // depend only on scale_x, scale_y and zoom, even if the device 
    // context changes so that algorithms that rely on text extent will 
    // work the same on the screen and on printers; note: so far, we
    // haven't run into many problems, so this may be ok; research

    if (!m_draw_dc)
        return false;

    // get the partial text extents of the text
    bool retval = m_draw_dc->GetPartialTextExtents(text, widths);
    
    // convert the widths to model coordinates
    wxArrayInt::iterator it, it_end;
    it_end = widths.end();
    for (it = widths.begin(); it != it_end; ++it)
    {
        *it = dtom_x(*it);
    }
    
    return retval;
}

int CanvasDC::getCharHeight() const
{
    // TODO: need to make the model dimensions this function returns 
    // depend only on scale_x, scale_y and zoom, even if the device 
    // context changes so that algorithms that rely on text extent will 
    // work the same on the screen and on printers; note: so far, we
    // haven't run into many problems, so this may be ok; research

    if (!m_draw_dc)
        return 0;

    return dtom_y(m_draw_dc->GetCharHeight());
}

int CanvasDC::getCharWidth() const
{
    // TODO: need to make the model dimensions this function returns 
    // depend only on scale_x, scale_y and zoom, even if the device 
    // context changes so that algorithms that rely on text extent will 
    // work the same on the screen and on printers; note: so far, we
    // haven't run into many problems, so this may be ok; research

    if (!m_draw_dc)
        return 0;

    return dtom_x(m_draw_dc->GetCharWidth());
}

void CanvasDC::addClippingRectangle(int x, int y, int w, int h)
{
    // if the width or height are less than zero, fix the 
    // rectangle so it expressed in positive coordinates
    if (w < 0) {x = x + w; w = -w;}
    if (h < 0) {y = y + h; h = -h;}

    // transform the rectangle coordinates from model coordinates 
    // to device coordinates
    int x1, y1, x2, y2;
    x1 = mtod_x(x + m_draw_origin_x);
    y1 = mtod_y(y + m_draw_origin_y);
    x2 = mtod_x(x + w + m_draw_origin_x);
    y2 = mtod_y(y + h + m_draw_origin_y);

    // add the rectangle to the list of rectangles and set
    // the clipping rectangle
    wxRect rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    m_clipping_rects.push_back(rect);

    updateClippingRectangle();
}

void CanvasDC::removeClippingRectangle()
{
    // remove the last added clipping rectangle
    if (m_clipping_rects.size() > 0)
        m_clipping_rects.pop_back();

    updateClippingRectangle();
}

void CanvasDC::removeAllClippingRectangles()
{
    // remove all the clipping rectangles
    m_clipping_rects.clear();
    updateClippingRectangle();
}

void CanvasDC::setDC(wxDC* dc)
{
    // set the dc
    m_draw_dc = dc;
    
    // update the clipping rectangle
    updateClippingRectangle();
}

wxDC* CanvasDC::getDC()
{
    return m_draw_dc;
}

void CanvasDC::updateClippingRectangle()
{
    if (!m_draw_dc)
        return;

    // destroy the old clipping region
    m_draw_dc->DestroyClippingRegion();

    if (m_clipping_rects.empty())
        return;

    // iterate through the vector of rectangles and aggregate
    // them into a single rectangle
    std::vector<wxRect>::iterator it, it_end;
    it_end = m_clipping_rects.end();

    wxRect clipping_rect = m_clipping_rects[0];
    for (it = m_clipping_rects.begin(); it != it_end; ++it)
    {
        clipping_rect.Intersect(*it);
    }

    // set the aggregated clipping region
    m_draw_dc->SetClippingRegion(clipping_rect);
}


}; // namespace kcanvas

