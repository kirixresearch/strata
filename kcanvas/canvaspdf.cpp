/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2008-03-12
 *
 */


#include "kcanvas_int.h"
#include "canvas.h"
#include "canvaspdf.h"
#include "property.h"


namespace kcanvas
{


class convertToString
{
public:

    convertToString(const wxString& str)
    {
        m_s = new char[str.Length()+10];
        const wchar_t* wc = str.wc_str();
        size_t i = 0;
        while (*wc)
        {
            unsigned int ch = *wc;
            if (ch > 255)
                ch = '?';
            m_s[i++] = (char)(unsigned char)ch;
            wc++;
        }
        m_s[i] = 0;
    }
    
    ~convertToString()
    {
        delete[] m_s;
    }
    
    operator const char*() const { return (const char*)m_s; }
 
private:

    char* m_s;
};



#if USE_WXPDF==0


// pdf-related constants; TODO: rename these
const int PDF_INTERFACE_DPI = CANVAS_MODEL_DPI;     // currently 1440
const int PDF_INTERNAL_DPI = 72;                    // points


void error_handler(HPDF_STATUS   error_no,
                   HPDF_STATUS   detail_no,
                   void         *user_data)
{
}

CanvasPDF::CanvasPDF()
{
    // create a default PDF document
    m_document = HPDF_New(error_handler, NULL);

    // turn on compression
    HPDF_SetCompressionMode(m_document, HPDF_COMP_ALL);

    // show canvas in single, continuous column
    HPDF_SetPageLayout(m_document, HPDF_PAGE_LAYOUT_ONE_COLUMN);

    // clipping state
    m_clipping_active = false;

    // default values
    m_bg_color = COLOR_WHITE;
    m_fg_color = COLOR_BLACK;
    m_text_color = COLOR_BLACK;
    m_fontname = wxT("Helvetica");
    m_fontsize = 10;
    m_pen_width = 1;
}

CanvasPDF::~CanvasPDF()
{
    HPDF_Free(m_document);
}

ICanvasPtr CanvasPDF::create()
{
    return static_cast<ICanvas*>(new CanvasPDF());
}

void CanvasPDF::addPage(int width, int height)
{
    // add a page
    m_page = HPDF_AddPage(m_document);

    // set the page size
    HPDF_Page_SetWidth(m_page, mtopdf_y(width));
    HPDF_Page_SetHeight(m_page, mtopdf_y(height));
}

bool CanvasPDF::saveAs(const wxString& name)
{
    HPDF_SaveToFile(m_document, convertToString(name));
    
    // TODO: check to see if the file was created; for
    // now, simply return true
    return true;
}

ICanvasPtr CanvasPDF::clone()
{
    CanvasPDF* c = new CanvasPDF;
    c->copy(this);

    return static_cast<ICanvas*>(c);
}

void CanvasPDF::copy(ICanvasPtr canvas)
{
    // copy the base canvas properties
    Canvas::copy(canvas);

    // TODO: copy additional pdf info
}

bool CanvasPDF::isPrinting() const
{
    return true;
}

bool CanvasPDF::setPen(const Pen& pen)
{
    m_fg_color = pen.getColor();
    m_pen_width = pen.getWidth();

    return true;
}

bool CanvasPDF::setPen(const kcanvas::Properties& properties)
{
    // used throughout
    kcanvas::PropertyValue value;    

    if (properties.get(kcanvas::PROP_LINE_WIDTH, value))
        m_pen_width = value.getInteger();

    if (properties.get(kcanvas::PROP_COLOR_FG, value))
        m_fg_color = value.getColor();

    return true;
}

bool CanvasPDF::setBrush(const Brush& brush)
{
    // TODO: set style (for transparent style)

    m_bg_color = brush.getColor();
    return true;
}

bool CanvasPDF::setBrush(const kcanvas::Properties& properties)
{
    // TODO: set style (for transparent style)

    // used throughout
    kcanvas::PropertyValue value;

    // set the brush color
    if (properties.get(kcanvas::PROP_COLOR_BG, value))
        m_bg_color = value.getColor();

    return true;
}

bool CanvasPDF::setFont(const Font& font)
{
    // TODO: for non-standard fonts, we should embed the
    // font; for now just use the default fonts; default
    // fonts are as follows:
    /*
        Courier
        Courier-Bold
        Courier-Oblique
        Courier-BoldOblique
        Helvetica
        Helvetica-Bold
        Helvetica-Oblique
        Helvetica-BoldOblique
        Times-Roman
        Times-Bold
        Times-Italic
        Times-BoldItalic
        Symbol
        ZapfDingbats
    */


    // get the font family; not all families have
    // corresponding default PDF types, so for these,
    // use HELVETICA
    wxString fontname = wxT("Helvetica");

/*
    // TODO: need to set the fontname based on the facename
    // since families in kcanvas::Font aren't yet implemented

    int f = font.GetFamily();
   
    if (f == wxFONTFAMILY_DEFAULT)
        fontname = wxT("Helvetica");
        
    if (f == wxFONTFAMILY_SWISS)
        fontname = wxT("Helvetica");
        
    if (f == wxFONTFAMILY_ROMAN)
        fontname = wxT("Times");

    if (f == wxFONTFAMILY_MODERN)
        fontname = wxT("Courier");
        
    if (f == wxFONTFAMILY_TELETYPE)
        fontname = wxT("Courier");

    if (f == wxFONTFAMILY_DECORATIVE) // TODO: not a good match
        fontname = wxT("Symbol");
      
    if (f == wxFONTFAMILY_SCRIPT) // TODO: not a good match
        fontname = wxT("Helvetica");
*/

    // set the style string
    wxString style;
    
    if (font.getWeight() == kcanvas::FONT_WEIGHT_BOLD)
    {
        if (fontname == wxT("Courier"))
            style += wxT("Bold");
            
        if (fontname == wxT("Helvetica"))
            style += wxT("Bold");
            
        if (fontname == wxT("Times"))
            style += wxT("Bold");
    }

    if (font.getStyle() == kcanvas::FONT_STYLE_ITALIC)
    {
        if (fontname == wxT("Courier"))
            style += wxT("Oblique");
            
        if (fontname == wxT("Helvetica"))
            style += wxT("Oblique");
            
        if (fontname == wxT("Times"))
            style += wxT("Italic");
    }

    if (font.getUnderscore() == kcanvas::FONT_UNDERSCORE_LINE)
    {
        // TODO: handle underline
    }
    
    if (style.Length() > 0)
    {
        fontname += wxT("-") + style;
    }
    else
    {
        if (fontname == wxT("Times"))
            fontname += wxT("-Roman");
    }

    // get the font size
    int size = font.getSize();

    // save the font name and size
    m_fontname = fontname;
    m_fontsize = size;

    return true;
}

bool CanvasPDF::setTextBackground(const Color& color)
{
    m_bg_color = color;
    return true;
}

bool CanvasPDF::setTextForeground(const Color& color)
{
    m_text_color = color;
    return true;
}

void CanvasPDF::drawImage(const wxImage& image,
                           int x, int y, int w, int h)
{
    // if we don't have an image, we're done
    if (!image.Ok())
        return;

    // initialize the graphics state
    (void)saveState();
    (void)initializeGraphicsState();

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    x = x + draw_origin_x;
    y = y + draw_origin_y;


    // the origin for the PDF page is in the lower-left, but
    // for the canvas in the upper-left; transform the y coordinate
    // manually; TODO: may want to simply apply a coorinate
    // transformation in addPage() that would automatically convert
    // between the canvas and the pdf coordinate systems
    y = getHeight() - y - h;


    // get the raw data
    unsigned char* image_buf = image.GetData();
    size_t bits_per_component = CHAR_BIT;

    // convert the raw contents of the bitmap into a buffer; set the
    // color space to RGB (color_space = HPDF_CS_DEVICE_RGB)
    HPDF_Image pdf_image = HPDF_LoadRawImageFromMem(m_document,
                                                    image_buf,
                                                    image.GetWidth(),
                                                    image.GetHeight(),
                                                    HPDF_CS_DEVICE_RGB,
                                                    bits_per_component);

    HPDF_Page_DrawImage(m_page, pdf_image,
                        mtopdf_x(x), mtopdf_y(y), mtopdf_x(w), mtopdf_y(h));


    // restore the graphics state
    (void)restoreState();
}

void CanvasPDF::drawCheckBox(int x, int y, int w, int h, bool checked)
{
    // TODO: implement
}

void CanvasPDF::drawCircle(int x, int y, int r)
{
    // initialize the graphics state
    (void)saveState();
    (void)initializeGraphicsState();

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    x = x + draw_origin_x;
    y = y + draw_origin_y;


    // the origin for the PDF page is in the lower-left, but
    // for the canvas in the upper-left; transform the y coordinate
    // manually; TODO: may want to simply apply a coorinate
    // transformation in addPage() that would automatically convert
    // between the canvas and the pdf coordinate systems
    y = getHeight() - y;


    HPDF_Page_Circle(m_page, mtopdf_x(x), mtopdf_y(y), mtopdf_x(r));
    HPDF_Page_FillStroke(m_page);

    // restore the graphics state
    (void)restoreState();
}

void CanvasPDF::drawEllipse(int x, int y, int w, int h)
{
    // initialize the graphics state
    (void)saveState();
    (void)initializeGraphicsState();

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    // HPDF_Page_Ellipse draws ellipses from the center given 
    // by (x0,y0), a horizontal radius of (rw), and a vertical 
    // radius of (ry)
    int rx = w/2;
    int ry = h/2;
    int x0 = x + rx + draw_origin_x;
    int y0 = y + ry + draw_origin_y;


    // the origin for the PDF page is in the lower-left, but
    // for the canvas in the upper-left; transform the y coordinate
    // manually; TODO: may want to simply apply a coorinate
    // transformation in addPage() that would automatically convert
    // between the canvas and the pdf coordinate systems
    y0 = getHeight() - y0;


    HPDF_Page_MoveTo(m_page, mtopdf_x(x0), mtopdf_y(y0));
    HPDF_Page_Ellipse(m_page, mtopdf_x(x0), mtopdf_y(y0), mtopdf_x(rx), mtopdf_y(ry));
    HPDF_Page_FillStroke(m_page);

    // restore the graphics state
    (void)restoreState();
}

void CanvasPDF::drawLine(int x1, int  y1, int x2, int y2)
{
    // initialize the graphics state
    (void)saveState();
    (void)initializeGraphicsState();

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    x1 = x1 + draw_origin_x;
    y1 = y1 + draw_origin_y;
    x2 = x2 + draw_origin_x;
    y2 = y2 + draw_origin_y;


    // the origin for the PDF page is in the lower-left, but
    // for the canvas in the upper-left; transform the y coordinate
    // manually; TODO: may want to simply apply a coorinate
    // transformation in addPage() that would automatically convert
    // between the canvas and the pdf coordinate systems
    y1 = getHeight() - y1;
    y2 = getHeight() - y2;


    HPDF_Page_MoveTo(m_page, mtopdf_x(x1), mtopdf_y(y1));
    HPDF_Page_LineTo(m_page, mtopdf_x(x2), mtopdf_y(y2));
    HPDF_Page_Stroke(m_page);

    // restore the graphics state
    (void)restoreState();
}

void CanvasPDF::drawPoint(int x, int y)
{
    // TODO: implement
}

void CanvasPDF::drawPolygon(int n, wxPoint points[], int x, int y)
{
    // if we don't have any points, we're done
    if (n < 1)
        return;

    // initialize the graphics state
    (void)saveState();
    (void)initializeGraphicsState();

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    for (int i = 0; i < n; ++i)
    {
        x = points[i].x + x + draw_origin_x;
        y = points[i].y + y + draw_origin_y;


        // the origin for the PDF page is in the lower-left, but
        // for the canvas in the upper-left; transform the y coordinate
        // manually; TODO: may want to simply apply a coorinate
        // transformation in addPage() that would automatically convert
        // between the canvas and the pdf coordinate systems
        y = getHeight() - y;    


        if (i == 0)
        {
            // move to the first point
            HPDF_Page_MoveTo(m_page, x, y);
            continue;
        }
        
        // draw a line to each subsequent point
        HPDF_Page_LineTo(m_page, x, y);
    }

    // close the path and draw
    HPDF_Page_ClosePathFillStroke(m_page);

    // restore the graphics state
    (void)restoreState();
}

void CanvasPDF::drawGradientFill(const wxRect& rect,
                                 const Color& start_color,
                                 const Color& end_color,
                                 const wxDirection& direction)
{
    // TODO: implement
}

void CanvasPDF::drawRectangle(int x, int y, int w, int h)
{
    // initialize the graphics state
    (void)saveState();
    (void)initializeGraphicsState();

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    x = x + draw_origin_x;
    y = y + draw_origin_y;


    // the origin for the PDF page is in the lower-left, but
    // for the canvas in the upper-left; transform the y coordinate
    // manually; TODO: may want to simply apply a coorinate
    // transformation in addPage() that would automatically convert
    // between the canvas and the pdf coordinate systems
    y = getHeight() - y - h;


    HPDF_Page_Rectangle(m_page, mtopdf_x(x), mtopdf_y(y), mtopdf_x(w), mtopdf_y(h));
    HPDF_Page_FillStroke(m_page);

    // restore the graphics state
    (void)restoreState();
}

void CanvasPDF::drawText(const wxString& text, int x, int y)
{
    // initialize the graphics state
    (void)saveState();
    (void)initializeTextState();

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    x = x + draw_origin_x;
    y = y + draw_origin_y;

    // the HPDF_Page_ShowText() function positions text using
    // the top of the base-line of the letters; however, the 
    // canvas uses the top of the text (very nearly the ascent, 
    // if not the same), so to render text properly, we have to 
    // add on the ascent

    // get the x height and ascent and convert them from font 
    // units into canvas units; see the GetCharHeight function 
    // for more info about converting between the two units
    HPDF_Font font = HPDF_Page_GetCurrentFont(m_page);
    HPDF_INT ascent = HPDF_Font_GetAscent(font);
    HPDF_REAL size = HPDF_Page_GetCurrentFontSize(m_page);
    y += ((double)ascent*size*PDF_INTERFACE_DPI)/72000;


    // the origin for the PDF page is in the lower-left, but
    // for the canvas in the upper-left; transform the y coordinate
    // manually; TODO: may want to simply apply a coorinate
    // transformation in addPage() that would automatically convert
    // between the canvas and the pdf coordinate systems
    y = getHeight() - y;


    // draw the text
    HPDF_Page_BeginText(m_page);
    HPDF_Page_MoveTextPos(m_page, mtopdf_x(x), mtopdf_y(y));
    HPDF_Page_ShowText(m_page, convertToString(text));
    HPDF_Page_EndText(m_page);

    // restore the graphics state
    (void)restoreState();
}

bool CanvasPDF::getTextExtent(const wxString& text,
                              int* x,
                              int* y,
                              int* descent,
                              int* external) const
{
    // initialize the graphics state
    (void)saveState();
    (void)initializeTextState();

    // for the x-extent, get the string width
    *x = pdftom_x(HPDF_Page_TextWidth(m_page, convertToString(text)));

    // for the y-extent, use the character height
    *y = getCharHeight();

    // if the descent is requested, set it
    if (descent)
    {
        // convert directly into canvas units;
        // 1000 = 1pt; @ 72pt per inch, we have 72000 font
        // units per inch; so convert to canvas units as
        // follows:

        // calculate the descent
        HPDF_Font font = HPDF_Page_GetCurrentFont(m_page);
        HPDF_REAL size = HPDF_Page_GetCurrentFontSize(m_page);
        *descent = ((double)HPDF_Font_GetDescent(font)*size*PDF_INTERFACE_DPI)/72000;
    }

    // if external is requested, simply return zero;
    // we don't know what this value is right now        
    if (external)
        *external = 0;

    // restore the graphics state
    (void)restoreState();
    return true;
}

bool CanvasPDF::getPartialTextExtents(const wxString& text,
                                      wxArrayInt& widths) const
{
    // note: getPartialTextExtents() returns an array of the width of
    // each character plus previous characters; i.e., an array of the 
    // widths of each substring of the string that begins with the first 
    // character of the string; before the current implementation, we 
    // calculated the average width of each character and used that to 
    // determine the partial text extents, but the results were more 
    // approximate than the current method; for reasonably-sized strings, 
    // the current, more-precise method should be fast enough

    // initialize the graphics state
    (void)saveState();
    (void)initializeTextState();

    // clear out the widths array
    widths.clear();
    
    // calculate the partial text extents
    int length = text.Length();
    for (int i = 0; i < length; ++i)
    {
        wxString text_substr = text.SubString(0, i);

        int width = pdftom_x(HPDF_Page_TextWidth(m_page, convertToString(text_substr)));
        widths.push_back(width);
    }

    // restore the graphics state
    (void)restoreState();
    return true;
}

int CanvasPDF::getCharHeight() const
{
    // TODO: verify if this is the best way to calculate
    // the character height

    // initialize the graphics state
    (void)saveState();
    (void)initializeTextState();

    // used throughout
    int height = 0;

    // font unit: 1000 = 1 pt    
    HPDF_Font font = HPDF_Page_GetCurrentFont(m_page);
    HPDF_INT ascent = HPDF_Font_GetAscent(font);
    HPDF_INT descent = HPDF_Font_GetDescent(font);

    height += ascent;
    height += -1*descent;

    // convert directly into canvas units;
    // 1000 = 1pt; @ 72pt per inch, we have 72000 font
    // units per inch; so convert to canvas units as
    // follows:
    HPDF_REAL size = HPDF_Page_GetCurrentFontSize(m_page);
    height = ((double)height*size*PDF_INTERFACE_DPI)/72000;

    // restore the graphics state
    (void)restoreState();

    // return height
    return height;
}

int CanvasPDF::getCharWidth() const
{
    // initialize the graphics state
    (void)saveState();
    (void)initializeTextState();

    // used throughout
    int width = 0;

    // font unit: 1000 = 1 pt; use an average of
    // capital letter widths
    HPDF_Font font = HPDF_Page_GetCurrentFont(m_page);

    // TODO: should we be using an X here -- is this function even used? BIW 12/2/2009
    HPDF_INT unicode_width = HPDF_Font_GetUnicodeWidth(font, 'X');
    width += unicode_width;

    // convert directly into canvas units;
    // 1000 = 1pt; @ 72pt per inch, we have 72000 font
    // units per inch; so convert to canvas units as
    // follows:
    HPDF_REAL size = HPDF_Page_GetCurrentFontSize(m_page);    
    width = ((double)width*size*PDF_INTERFACE_DPI)/72000;

    // restore the graphics state
    (void)restoreState();

    // return width
    return width;
}

void CanvasPDF::addClippingRectangle(int x, int y, int w, int h)
{
    // if the width or height are less than zero, fix the 
    // rectangle so it expressed in positive coordinates
    if (w < 0) {x = x + w; w = -w;}
    if (h < 0) {y = y + h; h = -h;}

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    // transform the rectangle coordinates from model coordinates 
    // to device coordinates
    int x1, y1, x2, y2;
    x1 = mtopdf_x(x + draw_origin_x);
    y1 = mtopdf_y(y + draw_origin_y);
    x2 = mtopdf_x(x + w + draw_origin_x);
    y2 = mtopdf_y(y + h + draw_origin_y);

    // add the rectangle to the list of rectangles and set
    // the clipping rectangle
    wxRect rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    m_clipping_rects.push_back(rect);

    updateClippingRectangle();
}

void CanvasPDF::removeClippingRectangle()
{
    // remove the last added clipping rectangle
    if (m_clipping_rects.size() > 0)
        m_clipping_rects.pop_back();

    updateClippingRectangle();
}

void CanvasPDF::removeAllClippingRectangles()
{
    m_clipping_rects.clear();
    updateClippingRectangle();
}

void CanvasPDF::updateClippingRectangle()
{
    // if we don't have any clipping rectangles, we're done                    
    if (m_clipping_rects.empty())
    {
        m_clipping_active = false;
        return;
    }

    // iterate through the vector of rectangles and aggregate
    // them into a single rectangle
    std::vector<wxRect>::iterator it, it_end;
    it_end = m_clipping_rects.end();

    wxRect rect = m_clipping_rects[0];
    wxRegion region(rect.x, rect.y, rect.width, rect.height);

    for (it = m_clipping_rects.begin(); it != it_end; ++it)
    {
        region.Intersect(it->x, it->y, it->width, it->height);
    }

    // set the aggregated clipping region; TODO: to be more precise, we
    // can actually use a clipping path; then, this will work, even if the 
    // renderer/canvas interface is updated to support non-rectangular
    // clipping regions
    m_clipping_active = true;
    m_clipping_rect = region.GetBox();
}

bool CanvasPDF::saveState() const
{
    // save the default graphics state
    HPDF_Page_GSave(m_page);
    return true;
}

bool CanvasPDF::restoreState() const
{
    // restore teh default graphics state
    HPDF_Page_GRestore(m_page);
    return true;
}

bool CanvasPDF::initializeGraphicsState() const
{
    if (!initializePen())
        return false;

    if (!initializeBrush())
        return false;

    if (!initializeClippingRegion())
        return false;

    return true;
}

bool CanvasPDF::initializeTextState() const
{
    if (!initializeFont())
        return false;

    if (!initializeClippingRegion())
        return false;

    return true;
}

bool CanvasPDF::initializePen() const
{
    // set the join style
    if (HPDF_OK != HPDF_Page_SetLineJoin(m_page, HPDF_MITER_JOIN))
        return false;

    // set the cap style
    if (HPDF_OK != HPDF_Page_SetLineCap(m_page, HPDF_BUTT_END))
        return false;

    // set the line width
    if (HPDF_OK != HPDF_Page_SetLineWidth(m_page, mtopdf_x(m_pen_width)))
        return false;

    // set the pen color
    HPDF_STATUS status = HPDF_Page_SetRGBStroke(m_page, 
                                                (float)m_fg_color.Red()/255, 
                                                (float)m_fg_color.Green()/255,
                                                (float)m_fg_color.Blue()/255);

    if (status != HPDF_OK)
        return false;

    return true;
}

bool CanvasPDF::initializeBrush() const
{
    HPDF_STATUS status = HPDF_Page_SetRGBFill(m_page,
                                              (float)m_bg_color.Red()/255,
                                              (float)m_bg_color.Green()/255,
                                              (float)m_bg_color.Blue()/255);

    if (status != HPDF_OK)
        return false;
    
    return true;
}

bool CanvasPDF::initializeFont() const
{
    //HPDF_Encoder encoder = HPDF_GetEncoder(m_document, "ISO8859-1");
    
    // get the font
    HPDF_Font fonthandle = HPDF_GetFont(m_document, convertToString(m_fontname), "WinAnsiEncoding");

    if (!fonthandle)
        return false;

    // set the font
    HPDF_Page_BeginText(m_page);
    HPDF_Page_SetRGBFill(m_page, (float)m_text_color.Red()/255,
                                 (float)m_text_color.Green()/255,
                                 (float)m_text_color.Blue()/255);
    HPDF_Page_SetTextRenderingMode(m_page, HPDF_FILL);
    HPDF_STATUS status = HPDF_Page_SetFontAndSize(m_page, fonthandle, m_fontsize);
    HPDF_Page_EndText(m_page);

    if (status != HPDF_OK)
        return false;

    return true;
}

bool CanvasPDF::initializeClippingRegion() const
{
    /*
    // end previous clipping region
    HPDF_Page_Eoclip(page)

    // create clipping rectangle
    HPDF_Page_Rectangle(page, x, y, w, h); - creates a path
    HPDF_Page_Clip(page) - clips a path
    HPDF_Page_EndPath(page) - ends a path
    */

    // get the clipping rectangle
    wxRect clipping_rect = m_clipping_rect;


    // the origin for the PDF page is in the lower-left, but
    // for the canvas in the upper-left; transform the y coordinate
    // manually; TODO: may want to simply apply a coorinate
    // transformation in addPage() that would automatically convert
    // between the canvas and the pdf coordinate systems
    clipping_rect.y = mtopdf_y(getHeight()) - clipping_rect.y - clipping_rect.height;


    // create the clipping region
    HPDF_Page_Rectangle(m_page,
                        clipping_rect.x,
                        clipping_rect.y,
                        clipping_rect.width,
                        clipping_rect.height);

    HPDF_Page_Clip(m_page);

    // close the clipping region
    HPDF_Page_EndPath(m_page);

    return true;
}

double CanvasPDF::mtopdf_x(int x) const
{
    return ((double)x*(double)PDF_INTERNAL_DPI)/(double)PDF_INTERFACE_DPI;
}

double CanvasPDF::mtopdf_y(int y) const
{
    return ((double)y*(double)PDF_INTERNAL_DPI)/(double)PDF_INTERFACE_DPI;
}

int CanvasPDF::pdftom_x(double x) const
{
    double x_model = x*((double)PDF_INTERFACE_DPI/(double)PDF_INTERNAL_DPI);
    double diff = x_model - (int)x_model;
    if (diff >= 0.5)
        return (int)x_model + 1;
    else
        return (int)x_model;
}

int CanvasPDF::pdftom_y(double y) const
{
    double y_model = y*((double)PDF_INTERFACE_DPI/(double)PDF_INTERNAL_DPI);
    double diff = y_model - (int)y_model;
    if (diff >= 0.5)
        return (int)y_model + 1;
    else
        return (int)y_model;
}



#endif
#if USE_WXPDF==1



// pdf-related constants; TODO: rename these
const int PDF_INTERFACE_DPI = CANVAS_MODEL_DPI;     // currently 1440
const int PDF_INTERNAL_DPI = 72;                    // points


CanvasPDF::CanvasPDF(wxPaperSize format, int orientation)
{
    // create a default PDF document, using points as the unit of measure
    m_document = new wxPdfDocument(orientation, wxString(_T("pt")), format);
    
    // turn auto page break off; we'll create the pages manually
    m_document->SetAutoPageBreak(false);
    
    // make sure compression is on
    m_document->SetCompression(true);
    
    // set the cell margin to zero so text is drawn
    // precisely where it is specified
    m_document->SetCellMargin(0);
    
    // turn font subsetting on, so that if we add font embedding, we'll
    // only be embedding the glyphs that are used, saving space; set
    // the default font to Arial        
    m_document->SetFontSubsetting(true);
    m_document->SetFont(wxT("ARIAL"), wxT(""), 8);
    
    // clipping state
    m_clipping_active = false;
}

CanvasPDF::~CanvasPDF()
{
    delete m_document;
}

ICanvasPtr CanvasPDF::create(wxPaperSize format, int orientation)
{
    return static_cast<ICanvas*>(new CanvasPDF(format, orientation));
}

void CanvasPDF::addPage(int width, int height)
{
    if (!m_document)
        return;

    m_document->AddPage();
}

bool CanvasPDF::saveAs(const wxString& name)
{
    if (!m_document)
        return false;

    m_document->SaveAsFile(name);
    
    // TODO: check to see if the file was created; for
    // now, simply return true
    return true;
}

ICanvasPtr CanvasPDF::clone()
{
    CanvasPDF* c = new CanvasPDF;
    c->copy(this);

    return static_cast<ICanvas*>(c);
}

void CanvasPDF::copy(ICanvasPtr canvas)
{
    // copy the base canvas properties
    Canvas::copy(canvas);

    // TODO: copy additional pdf info
}

bool CanvasPDF::isPrinting() const
{
    return true;
}

bool CanvasPDF::setPen(const Pen& pen)
{
    if (!m_document)
        return false;

    m_document->SetDrawColor(pen.getColor());
    m_document->SetLineWidth(mtopdf_x(pen.getWidth()));

    return true;
}

bool CanvasPDF::setPen(const kcanvas::Properties& properties)
{
    if (!m_document)
        return false;

    wxPen pen;

    // note: this function sets the pen from a property list
    kcanvas::PropertyValue value;

    // set the join and cap style; TODO: determine if we should
    // expose these as properties; if not, we can set these globally
    // in the constructor, provided we take care to reset them if
    // the pen is set via the other setPen() function    
    pen.SetCap(wxCAP_BUTT);
    pen.SetJoin(wxJOIN_MITER);
    
    // set the line width
    if (properties.get(kcanvas::PROP_LINE_WIDTH, value))
        pen.SetWidth(value.getInteger());
    
    // set the pen color
    if (properties.get(kcanvas::PROP_COLOR_FG, value))
        pen.SetColour(value.getColor());

    // TODO: add support for the transparent pen; perhaps if
    // the foreground color property doesn't exist, set the 
    // transparent pen?

    // set the pen width and color
    m_document->SetLineWidth(pen.GetWidth());
    m_document->SetDrawColor(pen.GetColour());
    
    return true;
}

bool CanvasPDF::setBrush(const Brush& brush)
{
    if (!m_document)
        return false;

    m_document->SetFillColor(brush.GetColor());
    return true;
}

bool CanvasPDF::setBrush(const kcanvas::Properties& properties)
{
    if (!m_document)
        return false;

    wxBrush brush;

    // note: this function sets the brush from a property list
    kcanvas::PropertyValue value;
    
    // set the brush color
    if (properties.get(kcanvas::PROP_COLOR_BG, value))
        brush.SetColour(value.getColor());

    // TODO: add support for the transparent brush; perhaps if
    // the background color property doesn't exist, set the 
    // transparent brush?

    // set the fill color
    m_document->SetFillColor(brush.GetColour());
    return true;
}

bool CanvasPDF::setFont(const Font& font)
{
    if (!m_document)
        return false;

    // TODO: for non-standard fonts, we should embed the
    // font; for now just use the default fonts

    // get the font family; not all families have
    // corresponding default PDF types, so for these,
    // use ARIAL
    wxString family = wxT("ARIAL");
    
/*
    // TODO: need to set the fontname based on the facename
    // since families in kcanvas::Font aren't yet implemented    
    
    int f = font.GetFamily();
    
    if (f == wxFONTFAMILY_DEFAULT)
        family = wxT("ARIAL");
        
    if (f == wxFONTFAMILY_SWISS)
        family = wxT("ARIAL");
        
    if (f == wxFONTFAMILY_ROMAN)
        family = wxT("TIMES");
        
    if (f == wxFONTFAMILY_MODERN)
        family = wxT("COURIER");
        
    if (f == wxFONTFAMILY_TELETYPE)
        family = wxT("COURIER");

    if (f == wxFONTFAMILY_DECORATIVE) // TODO: not a good match
        family = wxT("SYMBOL");
      
    if (f == wxFONTFAMILY_SCRIPT) // TODO: not a good match
        family = wxT("ARIAL");
*/

    // determine the font style and set the style string: B for bold,
    // I for italic, U for underlined, in any combination
    wxString style;
    style += (font.getWeight() == kcanvas::FONT_WEIGHT_BOLD ? wxT("B") : wxT(""));
    style += (font.getStyle() == kcanvas::FONT_STYLE_ITALIC ? wxT("I") : wxT(""));
    style += (font.getUnderscore() == kcanvas::FONT_UNDERSCORE_LINE ? wxT("U") : wxT(""));

    // get the font size
    int size = font.getSize();
    
    // BEGIN PROBLEM
    
    // TODO: this is terrible, but I couldn't find the problem or any other 
    // solution; for some reason, on the first page of a document only, and only
    // when clipping regions are active, after the first cell of a table is drawn
    // the rest of the cells will be drawn with the default font, which is set
    // in the constructor, without regard to what the font is actually set to in
    // this function; to make sure of this, I explicity set the font right before 
    // the m_document->Cell() call in drawText() with 
    // m_document->SetFont(wxT("ARIAL"), wxT(""), 10) and also checked the size of 
    // the font before and after m_document->Cell(), even though the font size
    // was correctly listed as 10, the default font, with a size of 5 was being
    // used to draw the cells; it seems that this problem happens as long as multiple 
    // cells are drawn in a row with the same font (face, style, size); however, if 
    // there's variation from cell to cell, such as in the font style, the fonts draw 
    // correctly; so, this kludge here forces toggles the bold setting of whatever font
    // we're setting, "draws" some blank text, then sets the correct font values,
    // clearing the way for the m_document->Cell() call in drawText()
    
    // additional note: not necessarily limited to the first page
    
    bool bold = (font.getWeight() == kcanvas::FONT_WEIGHT_BOLD);
    m_document->SetFont(family, bold ? wxT("") : wxT("B"), (double)size);
    m_document->Cell(0,0,wxT(""));

    // END PROBLEM; when this is fixed, delete the lines above; the rest
    // of the function is correct

    // set the font
    m_document->SetFont(family, style, (double)size);
    return true;
}

bool CanvasPDF::setTextBackground(const Color& color)
{
    if (!m_document)
        return false;

    m_document->SetFillColor(towxcolor(color));
    return true;
}

bool CanvasPDF::setTextForeground(const Color& color)
{
    if (!m_document)
        return false;

    m_document->SetTextColor(towxcolor(color));
    return true;
}

void CanvasPDF::drawImage(const wxImage& image,
                           int x, int y, int w, int h)
{
    if (!m_document)
        return;

    // if we don't have an image, we're done
    if (!image.Ok())
        return;

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    x = x + draw_origin_x;
    y = y + draw_origin_y;

    m_document->Image(wxT(""),
                      image,
                      mtopdf_x(x),
                      mtopdf_y(y));
}

void CanvasPDF::drawCheckBox(int x, int y, int w, int h, bool checked)
{
    // TODO: fill out
    if (!m_document)
        return;
}

void CanvasPDF::drawCircle(int x, int y, int r)
{
    if (!m_document)
        return;

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    x = x + draw_origin_x;
    y = y + draw_origin_y;

    m_document->Circle(mtopdf_x(x),
                       mtopdf_y(y),
                       mtopdf_x(r),
                       wxPDF_STYLE_FILLDRAW);
}

void CanvasPDF::drawEllipse(int x, int y, int w, int h)
{
    if (!m_document)
        return;

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    int rx = w/2;
    int ry = h/2;
    int x0 = x + rx + draw_origin_x;
    int y0 = y + ry + draw_origin_y;

    m_document->Ellipse(mtopdf_x(x0),
                        mtopdf_y(y0),
                        mtopdf_x(rx),
                        mtopdf_y(ry),
                        wxPDF_STYLE_FILLDRAW);
}

void CanvasPDF::drawLine(int x1, int  y1, int x2, int y2)
{
    if (!m_document)
        return;

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    x1 = x1 + draw_origin_x;
    y1 = y1 + draw_origin_y;
    x2 = x2 + draw_origin_x;
    y2 = y2 + draw_origin_y;

    m_document->Line(mtopdf_x(x1),
                     mtopdf_y(y1),
                     mtopdf_x(x2),
                     mtopdf_y(y2));
}

void CanvasPDF::drawPoint(int x, int y)
{
    // TODO: implement
    if (!m_document)
        return;
}

void CanvasPDF::drawPolygon(int n, wxPoint points[], int x, int y)
{
    if (!m_document)
        return;

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    wxPdfArrayDouble x0, y0;
    for (int i = 0; i < n; ++i)
    {
        x0.Add(mtopdf_x(points[i].x + x + draw_origin_x));
        y0.Add(mtopdf_y(points[i].y + y + draw_origin_y));
    }
    
    m_document->Polygon(x0,
                        y0,
                        wxPDF_STYLE_FILLDRAW);
}

void CanvasPDF::drawGradientFill(const wxRect& rect,
                                 const Color& start_color,
                                 const Color& end_color,
                                 const wxDirection& direction)
{
    if (!m_document)
        return;

    // determine the gradient
    Color c1, c2;
    wxPdfLinearGradientType gradient_direction;
    if (direction == wxEAST)
    {
        c1 = start_color;
        c2 = end_color;
        gradient_direction = wxPDF_LINEAR_GRADIENT_HORIZONTAL;
    }
    
    if (direction == wxWEST)
    {
        c1 = end_color;
        c2 = start_color;
        gradient_direction = wxPDF_LINEAR_GRADIENT_HORIZONTAL;
    }
    
    if (direction== wxNORTH)
    {
        c1 = start_color;
        c2 = end_color;
        gradient_direction = wxPDF_LINEAR_GRADIENT_VERTICAL;
    }
    
    if (direction == wxSOUTH)
    {
        c1 = end_color;
        c2 = start_color;
        gradient_direction = wxPDF_LINEAR_GRADIENT_VERTICAL;
    }

    int gradient = m_document->LinearGradient(towxcolor(c1),
                                              towxcolor(c2),
                                              gradient_direction);

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    wxRect r = rect;
    r.x = r.x + draw_origin_x;
    r.y = r.y + draw_origin_y;

    // draw the gradient                                
    m_document->SetFillGradient(mtopdf_x(r.x),
                                mtopdf_y(r.y),
                                mtopdf_x(r.width),
                                mtopdf_y(r.height),
                                gradient);
}

void CanvasPDF::drawRectangle(int x, int y, int w, int h)
{
    if (!m_document)
        return;

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    x = x + draw_origin_x;
    y = y + draw_origin_y;

    m_document->Rect(mtopdf_x(x),
                     mtopdf_y(y),
                     mtopdf_x(w),
                     mtopdf_y(h),
                     wxPDF_STYLE_FILLDRAW);
}

void CanvasPDF::drawText(const wxString& text, int x, int y)
{
    if (!m_document)
        return;

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    x = x + draw_origin_x;
    y = y + draw_origin_y;

    // the Cell() function positions text using the
    // top of the lower-case letters (the x height);
    // however, the canvas uses the top of the text
    // (very nearly the ascent, if not the same), so
    // to render text properly, we have to add on
    // the difference between the x height and the
    // ascent

    // get the x height and ascent and convert them 
    // from font units into canvas units; see the
    // GetCharHeight function for more info about
    // converting between the two units
    wxPdfFontDescription font;
    font = m_document->GetFontDescription();
    y += (font.GetAscent()*PDF_INTERFACE_DPI)/7200;
    y -= (font.GetXHeight()*PDF_INTERFACE_DPI)/7200;
    
    // TODO: for some reason, adding in the descender
    // yields better results; find out why
    y += (-1*font.GetDescent()*PDF_INTERFACE_DPI)/7200;

    // set the position of the text and render it
    m_document->SetXY(mtopdf_x(x), mtopdf_y(y));
    m_document->Cell(0, 0, text);
}

bool CanvasPDF::getTextExtent(const wxString& text,
                              int* x,
                              int* y,
                              int* descent,
                              int* external) const
{
    if (!m_document)
        return false;

    // for the x-extent, get the string width
    *x = pdftom_x(m_document->GetStringWidth(text));

    // for the y-extent, use the character height
    *y = getCharHeight();

    // if the descent is requested, set it
    if (descent)
    {
        // convert directly into canvas units;
        // 1000 = 1pt; @ 72pt per inch, we have 7200 font
        // units per inch; so convert to canvas units as
        // follows:
        wxPdfFontDescription font;
        font = m_document->GetFontDescription();
        *descent = (font.GetDescent()*PDF_INTERFACE_DPI)/7200;
    }

    // if external is requested, simply return zero;
    // we don't know what this value is right now        
    if (external)
        *external = 0;
    
    return true;
}

bool CanvasPDF::getPartialTextExtents(const wxString& text,
                                      wxArrayInt& widths) const
{
    if (!m_document)
        return false;

    // note: getPartialTextExtents() returns an array
    // of the width of each character plus previous
    // characters; i.e., an array of the widths of
    // each substring of the string that begins with
    // with the first character of the string; before
    // the current implementation, we calculated the
    // average width of each character and used that
    // to determine the partial text extents, but the
    // results were more approximate than the current
    // method; for reasonably-sized strings, the
    // current, more-precise method should be fast
    // enough

    // clear out the widths array
    widths.clear();
    
    // calculate the partial text extents
    int length = text.Length();
    for (int i = 0; i < length; ++i)
    {
        wxString t = text.SubString(0, i);
        int width = pdftom_x(m_document->GetStringWidth(t));
        widths.push_back(width);
    }

    return true;
}

int CanvasPDF::getCharHeight() const
{
    int height = 0;

    // font unit: 1000 = 1 pt
    wxPdfFontDescription font;
    font = m_document->GetFontDescription();
    height += font.GetAscent();
    height += -1*font.GetDescent();

    // convert directly into canvas units;
    // 1000 = 1pt; @ 72pt per inch, we have 7200 font
    // units per inch; so convert to canvas units as
    // follows:
    height = (height*PDF_INTERFACE_DPI)/7200;

    // return height
    return height;
}

int CanvasPDF::getCharWidth() const
{
    int width = 0;

    // font unit: 1000 = 1 pt; use the missing width,
    // for the character width, which is the default
    // width used for unkown glyph widths when 
    // calculating text extents
    wxPdfFontDescription font;
    font = m_document->GetFontDescription();
    width += font.GetMissingWidth();
    
    // convert directly into canvas units;
    // 1000 = 1pt; @ 72pt per inch, we have 7200 font
    // units per inch; so convert to canvas units as
    // follows:
    width = (width*PDF_INTERFACE_DPI)/7200;
    return width;
}

void CanvasPDF::addClippingRectangle(int x, int y, int w, int h)
{
    // if the width or height are less than zero, fix the 
    // rectangle so it expressed in positive coordinates
    if (w < 0) {x = x + w; w = -w;}
    if (h < 0) {y = y + h; h = -h;}

    // get the drawing origin
    int draw_origin_x, draw_origin_y;
    getDrawOrigin(&draw_origin_x, &draw_origin_y);

    // transform the rectangle coordinates from model coordinates 
    // to device coordinates
    int x1, y1, x2, y2;
    x1 = mtopdf_x(x + draw_origin_x);
    y1 = mtopdf_y(y + draw_origin_y);
    x2 = mtopdf_x(x + w + draw_origin_x);
    y2 = mtopdf_y(y + h + draw_origin_y);

    // add the rectangle to the list of rectangles and set
    // the clipping rectangle
    wxRect rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    m_clipping_rects.push_back(rect);

    updateClippingRectangle();
}

void CanvasPDF::removeClippingRectangle()
{
    // remove the last added clipping rectangle
    if (m_clipping_rects.size() > 0)
        m_clipping_rects.pop_back();

    updateClippingRectangle();
}

void CanvasPDF::removeAllClippingRectangles()
{
    m_clipping_rects.clear();
    updateClippingRectangle();
}

void CanvasPDF::updateClippingRectangle()
{
    // if we already have a clipping rectangle, destroy it
    if (m_clipping_active && m_document)
        m_document->UnsetClipping();

    // if we don't have any clipping rectangles, we're done                    
    if (m_clipping_rects.empty())
    {
        m_clipping_active = false;
        return;
    }

    // iterate through the vector of rectangles and aggregate
    // them into a single rectangle
    std::vector<wxRect>::iterator it, it_end;
    it_end = m_clipping_rects.end();

    wxRect rect = m_clipping_rects[0];
    wxRegion region(rect.x, rect.y, rect.width, rect.height);

    for (it = m_clipping_rects.begin(); it != it_end; ++it)
    {
        region.Intersect(it->x, it->y, it->width, it->height);
    }

    // set the aggregated clipping region; TODO: to be more precise, we
    // can actually use a clipping path; then, this will work, even if the 
    // renderer/canvas interface is updated to support non-rectangular
    // clipping regions
    m_clipping_active = true;
    wxRect clipping_rect = region.GetBox();
    
    if (m_document)
    {
        m_document->ClippingRect(clipping_rect.x,
                                 clipping_rect.y,
                                 clipping_rect.width,
                                 clipping_rect.height,
                                 false);
    }
}

double CanvasPDF::mtopdf_x(int x) const
{
    return ((double)x*(double)PDF_INTERNAL_DPI)/(double)PDF_INTERFACE_DPI;
}

double CanvasPDF::mtopdf_y(int y) const
{
    return ((double)y*(double)PDF_INTERNAL_DPI)/(double)PDF_INTERFACE_DPI;
}

int CanvasPDF::pdftom_x(double x) const
{
    double x_model = x*((double)PDF_INTERFACE_DPI/(double)PDF_INTERNAL_DPI);
    double diff = x_model - (int)x_model;
    if (diff >= 0.5)
        return (int)x_model + 1;
    else
        return (int)x_model;
}

int CanvasPDF::pdftom_y(double y) const
{
    double y_model = y*((double)PDF_INTERFACE_DPI/(double)PDF_INTERNAL_DPI);
    double diff = y_model - (int)y_model;
    if (diff >= 0.5)
        return (int)y_model + 1;
    else
        return (int)y_model;
}

#endif

}; // namespace kcanvas

