/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2009-12-18
 *
 */


#ifndef H_KCANVAS_GRAPHICSOBJ_H
#define H_KCANVAS_GRAPHICSOBJ_H


namespace kcanvas
{


// Canvas Graphics Object Classes

// color definition
class Color
{
public:

    Color();
    Color(unsigned char red, 
          unsigned char green, 
          unsigned char blue);
    Color(unsigned long color);
    Color(const wxString& color);
    virtual ~Color();
    
    Color(const Color& c);
    Color& operator=(const Color& c);
    Color& operator=(unsigned long c);
    Color& operator=(const wxString& c);
    bool operator==(const Color& c) const;
    bool operator!=(const Color& c) const;

    void Set(unsigned char red,
             unsigned char green,
             unsigned char blue);
    void Set(unsigned long color);
    void Set(const wxString& color);

    unsigned char Red() const;
    unsigned char Green() const;
    unsigned char Blue() const;

    wxString GetAsString() const;

private:

    void init();

private:

    // TODO: can easily consolidate variables
    unsigned char m_alpha;
    unsigned char m_red;
    unsigned char m_green;
    unsigned char m_blue;
};


// pen definition
class Pen
{
public:

    Pen();
    Pen(const Color& color,
        int width = 1,
        int style = wxSOLID);
    virtual ~Pen();
    
    Pen(const Pen& c);    
    Pen& operator=(const Pen& c);
    bool operator==(const Pen& c) const;
    bool operator!=(const Pen& c) const;

    void setColor(const Color& color);
    Color getColor() const;

    void setWidth(int width);
    int getWidth() const;
    
    void setStyle(int style);
    int getStyle() const;
    
    void setCap(int cap);
    int getCap() const;
    
    void setJoin(int miter);
    int getJoin() const;

private:

    Color m_color;
    int m_width;
    int m_style;
    int m_cap;
    int m_miter;
};


// brush definition
class Brush
{
public:

    Brush();
    Brush(const Color& color,
          int style = wxSOLID);
    virtual ~Brush();

    Brush(const Brush& c);
    Brush& operator=(const Brush& c);
    bool operator==(const Brush& c) const;
    bool operator!=(const Brush& c) const;

    void setColor(const Color& color);
    Color getColor() const;

    void setStyle(int style);
    int getStyle() const;

private:

    Color m_color;
    int m_style;
};


// font definition
class Font
{
public:

    Font();
    Font(const wxString& facename,
         const wxString& style = FONT_STYLE_NORMAL,
         const wxString& weight = FONT_WEIGHT_NORMAL,
         const wxString& underscore = FONT_UNDERSCORE_NORMAL,
         int size = 10);
    virtual ~Font();

    Font(const Font& c);
    Font& operator=(const Font& c);
    bool operator==(const Font& c) const;
    bool operator!=(const Font& c) const;

    void setFaceName(const wxString& facename);
    wxString getFaceName() const;
    
    void setFamilyName(const wxString& family);
    wxString getFamily() const;
    
    void setStyle(const wxString& style);
    wxString getStyle() const;
    
    void setWeight(const wxString& weight);
    wxString getWeight() const;
    
    void setUnderscore(const wxString& underscore);
    wxString getUnderscore() const;
    
    void setSize(int size);
    int getSize() const;

private:

    wxString m_facename;
    wxString m_family;
    wxString m_style;
    wxString m_weight;
    wxString m_underscore;
    int m_size;
};


}; // namespace kcanvas


#endif

