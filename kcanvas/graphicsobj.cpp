/*!
 *
 * Copyright (c) 2009-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2009-12-18
 *
 */


#include "kcanvas_int.h"
#include "graphicsobj.h"

#include "../kl/include/kl/regex.h"
#include "../kl/include/kl/string.h"


namespace kcanvas
{


Color::Color()
{
    init();
}

Color::Color(unsigned char red, 
             unsigned char green, 
             unsigned char blue)
{
    m_red = red;
    m_green = green;
    m_blue = blue;
    m_alpha = 0;
}

Color::Color(unsigned long color)
{
    Set(color);
}

Color::Color(const wxString& color)
{
    Set(color);
}

Color::~Color()
{
}

Color::Color(const Color& c)
{
    m_red = c.m_red;
    m_green = c.m_green;
    m_blue = c.m_blue;
    m_alpha = c.m_alpha;
}

Color& Color::operator=(const Color& c)
{
    if (this == &c)
        return *this;
        
    m_red = c.m_red;
    m_green = c.m_green;
    m_blue = c.m_blue;
    m_alpha = c.m_alpha;
    
    return *this;    
}

Color& Color::operator=(unsigned long c)
{
    Set(c);
    return *this;
}

Color& Color::operator=(const wxString& c)
{
    Set(c);
    return *this;
}

bool Color::operator==(const Color& c) const
{
    if (this == &c)
        return true;
        
    if (m_red != c.m_red)
        return false;
        
    if (m_green != c.m_green)
        return false;
        
    if (m_blue != c.m_blue)
        return false;

    if (m_alpha != c.m_alpha)
        return false;
        
    return true;
}

bool Color::operator!=(const Color& c) const
{
    return !(*this == c);
}

void Color::Set(unsigned char red,
                unsigned char green, 
                unsigned char blue)
{
    m_red = red;
    m_green = green;
    m_blue = blue;
    m_alpha = 0;
}

void Color::Set(unsigned long color)
{
    // color bits are set (red, green, blue) from high to low order;
    // highest order digits are reserved for an alpha channel and 
    // currently only set to non-zero for the null color

    m_red = (0xff & (color >> 16));
    m_green = (0xff & (color >> 8));
    m_blue = (0xff & color);
    m_alpha = (COLOR_NULL == color ? 255 : 0);
}

void Color::Set(const wxString& color)
{
    // takes strings of the form "rgb(#,#,#)" where
    // 0 <= # <= 255
    
    // initalize the colors
    init();

    wxString c = color;
    
    // remove leading/trailing spaces
    c.Trim(true);
    c.Trim(false);

    // parse out the r,g,b values
    std::wstring expr = kl::towstring((const wxChar*)c.c_str());
    static const klregex::wregex regex_rgb(L"^rgb\\s*\\(\\s*(?<red>\\d+)\\s*,\\s*(?<green>\\d+)\\s*,\\s*(?<blue>\\d+)\\s*\\)");

    klregex::wmatch matchres;
    if (!regex_rgb.search(expr, matchres))
        return;

    const klregex::wsubmatch& red_match = matchres[L"red"];
    const klregex::wsubmatch& green_match = matchres[L"green"];
    const klregex::wsubmatch& blue_match = matchres[L"blue"];

    m_alpha = 0;

    if (red_match.isValid())
        m_red = kl::wtoi(red_match.str().c_str()) % 256;
    
    if (green_match.isValid())
        m_green = kl::wtoi(green_match.str().c_str()) % 256;

    if (blue_match.isValid())
        m_blue = kl::wtoi(blue_match.str().c_str()) % 256;
}

unsigned char Color::Red() const
{
    return m_red;
}

unsigned char Color::Green() const
{
    return m_green;
}

unsigned char Color::Blue() const
{
    return m_blue;
}

wxString Color::GetAsString() const
{
    return wxString::Format(wxT("rgb(%d,%d,%d)"), m_red, m_green, m_blue);
}

void Color::init()
{
    // null/invalid color
    m_red = 255;
    m_green = 255;
    m_blue = 255;
    m_alpha = 255;
}

Pen::Pen()
{
    m_color = COLOR_BLACK;
    m_width = 1;
    m_style = wxSOLID;
    m_cap = wxCAP_BUTT;
    m_miter = wxJOIN_MITER;
}

Pen::Pen(const Color& color, int width, int style)
{
    m_color = color;
    m_width = width;
    m_style = style;
    m_cap = wxCAP_BUTT;
    m_miter = wxJOIN_MITER;    
}

Pen::~Pen()
{
}

Pen::Pen(const Pen& c)
{
    m_color = c.m_color;
    m_width = c.m_width;
    m_style = c.m_style;
    m_cap = c.m_cap;
    m_miter = c.m_miter;
}

Pen& Pen::operator=(const Pen& c)
{
    if (this == &c)
        return *this;

    m_color = c.m_color;
    m_width = c.m_width;
    m_style = c.m_style;
    m_cap = c.m_cap;
    m_miter = c.m_miter;

    return *this;
}

bool Pen::operator==(const Pen& c) const
{
    if (this == &c)
        return true;
        
    if (m_color != c.m_color)
        return false;
        
    if (m_width != c.m_width)
        return false;
        
    if (m_style != c.m_style)
        return false;
        
    if (m_cap != c.m_cap)
        return false;
        
    if (m_miter != c.m_miter)
        return false;
        
    return true;
}

bool Pen::operator!=(const Pen& c) const
{
    return !(*this == c);
}

void Pen::setColor(const Color& color)
{
    m_color = color;
}

Color Pen::getColor() const
{
    return m_color;
}

void Pen::setWidth(int width)
{
    m_width = width;
}

int Pen::getWidth() const
{
    return m_width;
}

void Pen::setStyle(int style)
{
    m_style = style;
}

int Pen::getStyle() const
{
    return m_style;
}

void Pen::setCap(int cap)
{
    m_cap = cap;
}

int Pen::getCap() const
{
    return m_cap;
}

void Pen::setJoin(int miter)
{
    m_miter = miter;
}

int Pen::getJoin() const
{
    return m_miter;
}


Brush::Brush()
{
    m_color = COLOR_BLACK;
    m_style = wxSOLID;
}

Brush::Brush(const Color& color, int style)
{
    m_color = color;
    m_style = style;
}

Brush::~Brush()
{
}

Brush::Brush(const Brush& c)
{
    m_color = c.m_color;
    m_style = c.m_style;
}

Brush& Brush::operator=(const Brush& c)
{
    if (this == &c)
        return *this;

    m_color = c.m_color;
    m_style = c.m_style;

    return *this;
}

bool Brush::operator==(const Brush& c) const
{
    if (this == &c)
        return true;

    if (m_color != c.m_color)
        return false;

    if (m_style != c.m_style)
        return false;

    return true;
}

bool Brush::operator!=(const Brush& c) const
{
    return !(*this == c);
}

void Brush::setColor(const Color& color)
{
    m_color = color;
}

Color Brush::getColor() const
{
    return m_color;
}

void Brush::setStyle(int style)
{
    m_style = style;
}

int Brush::getStyle() const
{
    return m_style;
}


Font::Font()
{
#ifdef WIN32
    m_facename = wxT("Arial");
    m_family = wxT("");    
#else 
    m_facename = wxT("Bitstream Vera Sans");
    m_family = wxT("");    
#endif

    m_style = FONT_STYLE_NORMAL;                  
    m_weight = FONT_WEIGHT_NORMAL;
    m_underscore = FONT_UNDERSCORE_NORMAL;
    m_size = 10;
}

Font::Font(const wxString& facename,
           const wxString& style,
           const wxString& weight,
           const wxString& underscore,
           int size)
{
    m_facename = facename;
    m_family = wxT("");
    m_style = style;
    m_weight = weight;
    m_underscore = underscore;
    m_size = size;
}

Font::~Font()
{
}

Font::Font(const Font& c)
{
    m_facename = c.m_facename;
    m_family = c.m_family;
    m_style = c.m_style;
    m_weight = c.m_weight;
    m_underscore = c.m_underscore;
    m_size = c.m_size;
}

Font& Font::operator=(const Font& c)
{
    if (this == &c)
        return *this;

    m_facename = c.m_facename;
    m_family = c.m_family;
    m_style = c.m_style;
    m_weight = c.m_weight;
    m_underscore = c.m_underscore;
    m_size = c.m_size;

    return *this;
}

bool Font::operator==(const Font& c) const
{
    if (this == &c)
        return true;

    if (m_facename != c.m_facename)
        return false;

    if (m_family != c.m_family)
        return false;

    if (m_style != c.m_style)
        return false;

    if (m_weight != c.m_weight)
        return false;

    if (m_underscore != c.m_underscore)
        return false;

    if (m_size != c.m_size)
        return false;

    return true;
}

bool Font::operator!=(const Font& c) const
{
    return !(*this == c);
}

void Font::setFaceName(const wxString& facename)
{
    m_facename = facename;
}

wxString Font::getFaceName() const
{
    return m_facename;
}

void Font::setFamilyName(const wxString& family)
{
    m_family = family;
}

wxString Font::getFamily() const
{
    return m_family;
}

void Font::setStyle(const wxString& style)
{
    m_style = style;
}

wxString Font::getStyle() const
{
    return m_style;
}

void Font::setWeight(const wxString& weight)
{
    m_weight = weight;
}

wxString Font::getWeight() const
{
    return m_weight;
}

void Font::setUnderscore(const wxString& underscore)
{
    m_underscore = underscore;
}

wxString Font::getUnderscore() const
{
    return m_underscore;
}

void Font::setSize(int size)
{
    m_size = size;
}

int Font::getSize() const
{
    return m_size;
}


}; // namespace kcanvas

