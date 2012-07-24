/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-07-10
 *
 */


#include "appmain.h"
#include "scriptgui.h"
#include "scriptfont.h"


// -- utility function --

static void updateValues(const wxFont& font,
                         int* pointsize,
                         int* family,
                         int* style,
                         wxString* facename)
{
    *facename = towstr(font.GetFaceName());
    *pointsize = font.GetPointSize();
    *family = zFont::Default;
    *style = zFont::Normal;
    
    switch (font.GetFamily())
    {
        default:
        case wxFONTFAMILY_DEFAULT:
            (*family) = zFont::Default;
            break;
        case wxFONTFAMILY_ROMAN:
            (*family) = zFont::Roman;
            break;
        case wxFONTFAMILY_SWISS:
            (*family) = zFont::Swiss;
            break;
        case wxFONTFAMILY_MODERN:
            (*family) = zFont::Modern;
            break;
    }
    
    (*style) = zFont::Normal;
    if (font.GetStyle() == wxFONTSTYLE_ITALIC)
        (*style) |= zFont::Italic;
    if (font.GetWeight() == wxFONTWEIGHT_BOLD)
        (*style) |= zFont::Bold;
    if (font.GetUnderlined())
        (*style) |= zFont::Underlined;
}

static wxFont constructFont(int family, int pointsize, int style, wxString facename)
{
    int wx_pointsize = 12;
    int wx_family = wxFONTFAMILY_DEFAULT;
    int wx_style  = wxFONTSTYLE_NORMAL;
    int wx_weight = wxFONTWEIGHT_NORMAL;
    bool wx_underlined = false;
    
    if (pointsize > 0)
        wx_pointsize = pointsize;
    
    switch (family)
    {
        default:
        case zFont::Default:
            wx_family = wxFONTFAMILY_DEFAULT;
            break;
        case zFont::Roman:
            wx_family = wxFONTFAMILY_ROMAN;
            break;
        case zFont::Swiss:
            wx_family = wxFONTFAMILY_SWISS;
            break;
        case zFont::Modern:
            wx_family = wxFONTFAMILY_MODERN;
            break;
    }
    
    if (style & zFont::Bold)
        wx_weight = wxFONTWEIGHT_BOLD;
        
    if (style & zFont::Italic)
        wx_style = wxFONTSTYLE_ITALIC;
        
    if (style & zFont::Underlined)
        wx_underlined = true;
    
    // create the font
    wxFont font = wxFont(wx_pointsize, wx_family, wx_style, wx_weight, wx_underlined, facename);
    if (!font.IsOk())
        font = *wxNORMAL_FONT;

    return font;
}




// -- Font class implementation --

// (CLASS) Font
// Category: Graphics
// Description: A class that represents a font.
// Remarks: The Font class encapsulates a font.
//
// Property(Font.Default):      A flag representing the default font family.
// Property(Font.Roman):        A flag representing the Roman font family.
// Property(Font.Swiss):        A flag representing the Swiss font family.
// Property(Font.Modern):       A flag representing the Modern font family.
// Property(Font.Normal):       A flag representing the normal font style.
// Property(Font.Bold):         A flag representing the bold font style.
// Property(Font.Italic):       A flag representing the italicized font style.
// Property(Font.Underlined):   A flag representing the underlined font style.

zFont::zFont()
{
    m_font = *wxNORMAL_FONT;
}

zFont::~zFont()
{
}

wxFont& zFont::getWxFont()
{
    return m_font;
}

void zFont::setWxFont(const wxFont& font)
{
    if (m_font.IsOk())
        m_font = font;
         else
        m_font = *wxNORMAL_FONT;

    updateValues(m_font, &m_pointsize, &m_family, &m_style, &m_facename);
}

// (CONSTRUCTOR) Font.constructor
// Description: Creates a new Font object.
//
// Syntax: Font(family : String,
//              pointsize : Integer,
//              style : Integer,
//              facename : String)
//
// Remarks: Creates a new font with the specified |family|,
//     |pointsize|, |style|, and |facename|, where the family is
//     one of Font.Default, Font.Roman, Font.Swiss, or Font.Modern,
//     and style is one of Font.Normal, Font.Bold, Font.Italic, and Font.Underlined.
//
// Param(family): The |family| of the new font. One of Font.Default,
//     Font.Roman, Font.Swiss or Font.Modern.
// Param(pointsize): The |pointsize| of the new font.
// Param(style): The |style| of the new font. Combination of Font.Normal,
//     Font.Bold, Font.Italic, and Font.Underlined.
// Param(facename): The |facename| of the new font.

void zFont::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // default values
    m_pointsize = 12;
    m_family = zFont::Default;
    m_style = zFont::Normal;
    m_facename = wxEmptyString;

    size_t param_count = env->getParamCount();
    
    // -- get font family --
    if (param_count > 0)
    {
        kscript::Value* param = env->getParam(0);
        if (param->isInteger())
        {
            int family = param->getInteger();
            if (family == zFont::Default ||
                family == zFont::Roman   ||
                family == zFont::Swiss   ||
                family == zFont::Modern)
            {
                m_family = family;
            }
        }
    }
    
    // -- get font point size --
    if (param_count > 1)
    {
        kscript::Value* param = env->getParam(1);
        if (param->isInteger() && param->getInteger() > 0)
            m_pointsize = param->getInteger();
    }
    
    // -- get font style --
    if (param_count > 2)
    {
        kscript::Value* param = env->getParam(2);
        if (param->isInteger())
        {
            int style = param->getInteger();
            if (style & zFont::Bold)
                m_style |= zFont::Bold;
            if (style & zFont::Italic)
                m_style |= zFont::Italic;
            if (style & zFont::Underlined)
                m_style |= zFont::Underlined;
        }
    }
    
    // -- get font facename --
    if (param_count > 3)
    {
        kscript::Value* param = env->getParam(3);
        if (param->isString())
            m_facename = param->getString();
    }
    
    // -- create the font --
    m_font = constructFont(m_family, m_pointsize, m_style, m_facename);
        
    // -- make sure our stored values match --
    updateValues(m_font, &m_pointsize, &m_family, &m_style, &m_facename);
}


// (METHOD) Font.setPointSize
// Description: Sets the new point size of the font.
//
// Syntax: function Font.setPointSize(size : Integer)
//
// Remarks: Sets the new |size| of the font in points.
//
// Param(size): The new |size| of the font in points.

void zFont::setPointSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;
    
    kscript::Value* param = env->getParam(0);
    param = env->getParam(0);
    if (param->isInteger() && param->getInteger() > 0)
        m_pointsize = param->getInteger();
    
    // -- create the font --
    m_font = constructFont(m_family, m_pointsize, m_style, m_facename);
        
    // -- make sure our stored values match --
    updateValues(m_font, &m_pointsize, &m_family, &m_style, &m_facename);
}

// (METHOD) Font.setFamily
// Description: Sets the new family of the font.
//
// Syntax: function Font.setFamily(family : String)
//
// Remarks: Sets the new |family| of the font, where |family| is
//     one of Font.Default, Font.Roman, Font.Swiss, or Font.Modern.
//
// Param(family): The new |family| of the font, where |family| is
//     one of Font.Default, Font.Roman, Font.Swiss, or Font.Modern.

void zFont::setFamily(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;

    kscript::Value* param = env->getParam(0);
    param = env->getParam(0);
    if (param->isInteger())
    {
        m_family = zFont::Default;
        
        int family = param->getInteger();
        if (family == zFont::Default ||
            family == zFont::Roman   ||
            family == zFont::Swiss   ||
            family == zFont::Modern)
        {
            m_family = family;
        }
    }
    
    // -- create the font --
    m_font = constructFont(m_family, m_pointsize, m_style, m_facename);
        
    // -- make sure our stored values match --
    updateValues(m_font, &m_pointsize, &m_family, &m_style, &m_facename);
}

// (METHOD) Font.setStyle
// Description: Sets the new style of the font.
//
// Syntax: function Font.setStyle(style : Integer)
//
// Remarks: Sets the new |style| of the font to a combination
//     of Font.Normal, Font.Bold, Font.Italic, and Font.Underlined.
//
// Param(style): The new |style| of the font, which is a combination
//     Font.Normal, Font.Bold, Font.Italic, and Font.Underlined.

void zFont::setStyle(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;
    
    kscript::Value* param = env->getParam(0);
    param = env->getParam(0);
    if (param->isInteger())
    {
        m_style = zFont::Normal;
        
        int style = param->getInteger();
        if (style & zFont::Bold)
            m_style |= zFont::Bold;
        if (style & zFont::Italic)
            m_style |= zFont::Italic;
        if (style & zFont::Underlined)
            m_style |= zFont::Underlined;
    }
    
    // -- create the font --
    m_font = constructFont(m_family, m_pointsize, m_style, m_facename);
        
    // -- make sure our stored values match --
    updateValues(m_font, &m_pointsize, &m_family, &m_style, &m_facename);
}

// (METHOD) Font.setFaceName
// Description: Sets the new facename of the font.
//
// Syntax: function Font.setFaceName(facename : String)
//
// Remarks: Sets the new |facename| of the font.
//
// Param(facename): The new |facename| of the font.

void zFont::setFaceName(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;
    
    kscript::Value* param = env->getParam(0);
    param = env->getParam(0);
    if (param->isString())
        m_facename = param->getString();
    
    // -- create the font --
    m_font = constructFont(m_family, m_pointsize, m_style, m_facename);
        
    // -- make sure our stored values match --
    updateValues(m_font, &m_pointsize, &m_family, &m_style, &m_facename);
}

// (METHOD) Font.getPointSize
// Description: Gets the point size of the font.
//
// Syntax: function Font.getPointSize()
//
// Remarks: Gets the point size of the font.

void zFont::getPointSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(m_pointsize);
}

// (METHOD) Font.getFamily
// Description: Gets the family of the font.
//
// Syntax: function Font.getFamily()
//
// Remarks: Gets the family of the font.

void zFont::getFamily(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(m_family);
}

// (METHOD) Font.getStyle
// Description: Gets the style of the font.
//
// Syntax: function Font.getStyle()
//
// Remarks: Gets the style of the font.

void zFont::getStyle(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(m_style);
}

// (METHOD) Font.getFaceName
// Description: Gets the facename of the font.
//
// Syntax: function Font.getFaceName()
//
// Remarks: Gets the facename of the font.

void zFont::getFaceName(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_facename));
}




