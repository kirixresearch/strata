/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2008-03-12
 *
 */


#ifndef H_KCANVAS_CANVASPDF_H
#define H_KCANVAS_CANVASPDF_H


#include "../supp/libharu/include/hpdf.h" 


#define USE_WXPDF 0


#if USE_WXPDF==1
class wxPdfDocument;
#endif


namespace kcanvas
{


// Canvas PDF Classes

#if USE_WXPDF==0


class CanvasPDF : public Canvas
{
    XCM_CLASS_NAME_NOREFCOUNT("kcanvas.CanvasPDF")
    XCM_BEGIN_INTERFACE_MAP(CanvasPDF)
        XCM_INTERFACE_CHAIN(Canvas)
    XCM_END_INTERFACE_MAP()

public:

    CanvasPDF();
    virtual ~CanvasPDF();

    static ICanvasPtr create();

public:

    void addPage(int width, int height);
    bool saveAs(const wxString& name);

public:

    // ICanvas
    ICanvasPtr clone();
    void copy(ICanvasPtr canvas);

    bool isPrinting() const;

    bool setPen(const Pen& pen);
    bool setPen(const kcanvas::Properties& properties);

    bool setBrush(const Brush& brush);
    bool setBrush(const kcanvas::Properties& properties);

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
                          const wxDirection& direction);
                           
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

private:

    // clipping region update function
    void updateClippingRectangle();

    // functions for saving and restoring the graphics state
    bool saveState() const;
    bool restoreState() const;

    // initializes the pdf graphics state based
    // on the drawing settings
    bool initializeGraphicsState() const;
    bool initializeTextState() const;
    bool initializePen() const;
    bool initializeBrush() const;
    bool initializeFont() const;
    bool initializeClippingRegion() const;

    // conversion functions
    double mtopdf_x(int x) const;
    double mtopdf_y(int y) const;
    
    int pdftom_x(double x) const;
    int pdftom_y(double y) const;
    
private:

    HPDF_Doc m_document;
    HPDF_Page m_page;

    // drawing settings
    Color m_fg_color;
    Color m_bg_color;
    Color m_text_color;
    wxString m_fontname;
    int m_fontsize;
    int m_pen_width;

    // clipping rectangles
    std::vector<wxRect> m_clipping_rects;
    wxRect m_clipping_rect;
    bool m_clipping_active;
};


#endif
#if USE_WXPDF==1


class CanvasPDF : public Canvas
{
    XCM_CLASS_NAME_NOREFCOUNT("kcanvas.CanvasPDF")
    XCM_BEGIN_INTERFACE_MAP(CanvasPDF)
        XCM_INTERFACE_CHAIN(Canvas)
    XCM_END_INTERFACE_MAP()

public:

    CanvasPDF(wxPaperSize format = wxPAPER_LETTER,
              int orientation = wxPORTRAIT);
    virtual ~CanvasPDF();

    static ICanvasPtr create(wxPaperSize format = wxPAPER_LETTER,
                             int orientation = wxPORTRAIT);

public:

    // ICanvas
    ICanvasPtr clone();
    void copy(ICanvasPtr canvas);

    bool isPrinting() const;

    bool setPen(const Pen& pen);
    bool setPen(const kcanvas::Properties& properties);

    bool setBrush(const Brush& brush);
    bool setBrush(const kcanvas::Properties& properties);

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
                          const wxDirection& direction);
                           
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

public:

    void addPage(int width, int height);
    bool saveAs(const wxString& name);

private:

    // clipping region update function
    void updateClippingRectangle();

    // conversion functions
    double mtopdf_x(int x) const;
    double mtopdf_y(int y) const;
    
    int pdftom_x(double x) const;
    int pdftom_y(double y) const;
    
private:

    wxPdfDocument* m_document;

    // clipping rectangles
    std::vector<wxRect> m_clipping_rects;
    bool m_clipping_active;
};

#endif


}; // namespace kcanvas


#endif

