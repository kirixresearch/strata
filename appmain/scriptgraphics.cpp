/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2006-12-23
 *
 */


#include "appmain.h"
#include "scripthost.h"
#include "scriptgraphics.h"
#include "scriptgui.h"
#include "scriptbitmap.h"
#include "scriptfont.h"


// -- Graphics class implementation --

// (CLASS) Graphics
// Category: Graphics
// Description: A class that encapsulates drawing commands on a graphics object.
// Remarks: Encapsulates drawing commands on a graphics object.

Graphics::Graphics()
{
    m_dc = NULL;
    m_wnd = NULL;
    m_memdc = NULL;
    m_paint_count = 0;
}

Graphics::~Graphics()
{
    if (m_memdc)
    {
        m_memdc->SelectObject(wxNullBitmap);
        delete m_memdc;
    }
}

void Graphics::init()
{
    if (m_dc)
    {
        m_dc->SetPen(*wxBLACK_PEN);
        m_dc->SetBrush(*wxBLACK_BRUSH);
        m_dc->SetFont(*wxNORMAL_FONT);
    }
}


void Graphics::incrementPaintCount()
{
    XCM_AUTO_LOCK(m_obj_mutex);
    m_paint_count++;
}

void Graphics::decrementPaintCount()
{
    XCM_AUTO_LOCK(m_obj_mutex);
    m_paint_count--;
}

int Graphics::getPaintCount()
{
    XCM_AUTO_LOCK(m_obj_mutex);
    return m_paint_count;
}


void Graphics::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) Graphics.drawLine
// Description: Draws a line on the graphics object.
//
// Syntax: function Graphics.drawLine(x1 : Integer,
//                                    y1 : Integer,
//                                    x2 : Integer,
//                                    y2 : Integer)
//
// Remarks: Draws a line from the point (|x1|, |y1|) to the point (|x2|, |y2|).
//
// Param(x1): The x position of the start of the line.
// Param(y1): The y position of the start of the line.
// Param(x2): The x position of the end of the line.
// Param(y2): The y position of the end of the line.

void Graphics::drawLine(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc)
        return;
        
    if (env->getParamCount() < 4)
        return;
   
    m_dc->DrawLine(env->getParam(0)->getInteger(),
                   env->getParam(1)->getInteger(),
                   env->getParam(2)->getInteger(),
                   env->getParam(3)->getInteger());
}

// (METHOD) Graphics.drawCircle
// Description: Draws a circle on the graphics object.
//
// Syntax: function Graphics.drawCircle(x : Integer,
//                                      y : Integer,
//                                      r : Integer)
//
// Remarks: Draws a circle with the center at the point (|x|, |y|) and
//     a radius of |r|.
//
// Param(x): The |x| position of the center of the circle.
// Param(y): The |y| position of the center of the circle.
// Param(r): The radius of the circle.

void Graphics::drawCircle(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc)
        return;
               
    if (env->getParamCount() < 3)
        return;
   
    m_dc->DrawCircle(env->getParam(0)->getInteger(),
                     env->getParam(1)->getInteger(),
                     env->getParam(2)->getInteger());
}

// (METHOD) Graphics.drawArc
// Description: Draws an arc on the graphics object.
//
// Syntax: function Graphics.drawArc(x : Integer,
//                                   y : Integer,
//                                   x1 : Integer,
//                                   y1 : Integer,
//                                   x2 : Integer,
//                                   y2 : Integer)
//
// Remarks: Draws an arc of a circle, centered at the point (|x|, |y|),
//     starting from the point (|x1|, |y1|) on the edge of the circle 
//     and proceeding counter-clockwise to another point (|x2|, |y2|),
//     also on the edge of the circle.
//
// Param(x): The |x| position of the center of the circle which contains
//     the arc.
// Param(y): The |y| position of the center of the circle which contains
//     the arc.
// Param(x1): The x position of the start of the arc.
// Param(y1): The y position of the start of the arc.
// Param(x2): The x position of the end of the arc.
// Param(y2): The y position of the end of the arc.

void Graphics::drawArc(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc)
        return;
               
    if (env->getParamCount() < 6)
        return;
   
    m_dc->DrawArc(env->getParam(0)->getInteger(),
                  env->getParam(1)->getInteger(),
                  env->getParam(2)->getInteger(),
                  env->getParam(3)->getInteger(),
                  env->getParam(4)->getInteger(),
                  env->getParam(5)->getInteger());
}

// (METHOD) Graphics.drawEllipticArc
// Description: Draws an elliptic arc on the graphics object.
//
// Syntax: function Graphics.drawEllipticArc(x : Integer,
//                                           y : Integer,
//                                           width : Integer,
//                                           height : Integer,
//                                           deg1 : Integer,
//                                           deg2 : Integer)
//
// Remarks: Draws an elliptic arc contained on an ellipse that is
//     bounded by a rectangle of a given |width| and |height| with an 
//     upper-left corner at the point (|x|, |y|), and where the arc of 
//     the bounded ellipse starts at an angle of |deg1| degrees relative
//     to a standard, positive x-axis and ends at an angle of |deg2| degrees
//     relative to the same axis.
//
// Param(x): The upper-left |x| position of the bounding rectangle.
// Param(y): The upper-left |y| position of the bounding rectangle.
// Param(width): The |width| of the bounding rectangle.
// Param(height): The |height| of the bounding rectangle.
// Param(deg1): The start of the arc in degrees, relative to a standard,
//     positive x-axis.
// Param(deg2): The end of the arc in degrees, relative to a standard,
//     positive x-axis.

void Graphics::drawEllipticArc(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc)
        return;
               
    if (env->getParamCount() < 6)
        return;
   
    m_dc->DrawEllipticArc(env->getParam(0)->getInteger(),
                          env->getParam(1)->getInteger(),
                          env->getParam(2)->getInteger(),
                          env->getParam(3)->getInteger(),
                          env->getParam(4)->getDouble(),
                          env->getParam(5)->getDouble());
}

// (METHOD) Graphics.drawImage
// Description: Draws an image on the graphics object.
//
// Syntax: function Graphics.drawImage(bitmap : Bitmap,
//                                     x : Integer,
//                                     y : Integer,
//                                     width : Integer,
//                                     height : Integer)
// Syntax: function Graphics.drawImage(graphics : Graphics,
//                                     x : Integer,
//                                     y : Integer,
//                                     width : Integer,
//                                     height : Integer)
//
// Remarks: Draws a |bitmap| at the position given by (|x|, |y|),
//     with a given |width| and |height|.
//
// Param(bitmap): The |bitmap| to draw.
// Param(graphics): The |graphics| object to blit to this graphics object.
// Param(x): The upper-left |x| position of the bitmap.
// Param(y): The upper-left |y| position of the bitmap.
// Param(width): The |width| of the bitmap.
// Param(height): The |height| of the bitmap.

void Graphics::drawImage(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc)
        return;

    size_t param_count = env->getParamCount();
    
    if (param_count < 1 || env->getParam(0)->getType() != kscript::Value::typeObject)
        return;

    
    int dest_x = (param_count >= 2) ? env->getParam(1)->getInteger() : 0;
    int dest_y = (param_count >= 3) ? env->getParam(2)->getInteger() : 0;
    int width = (param_count >= 4) ? env->getParam(3)->getInteger() : -1;
    int height = (param_count >= 5) ? env->getParam(4)->getInteger() : -1;

    

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    std::wstring class_name = obj->getClassName();
    if (class_name == L"Bitmap")
    {
        
    }
     else if (class_name == L"Graphics")
    {
        if (width == -1 || height == -1)
        {
            if (m_wnd)
            {
                wxSize cli_size = m_wnd->GetClientSize();
                if (width == -1)
                    width = cli_size.x;
                if (height == -1)
                    height = cli_size.y;
            }
             else
            {
                width = 0;
                height = 0;
            }
        }
    
    
        Graphics* graphics = (Graphics*)obj;
        if (graphics->m_dc)
        {
            m_dc->Blit(dest_x, dest_y, width, height, graphics->m_dc, 0, 0);
        }
    }
    
}

// (METHOD) Graphics.drawRectangle
// Description: Draws a rectangle on the graphics object.
//
// Syntax: function Graphics.drawRectangle(x : Integer,
//                                         y : Integer,
//                                         width : Integer,
//                                         height : Integer)
//
// Remarks: Draws a rectangle with an upper-left at the position
//     (|x|, |y|) and with a given |width| and |height|.
//
// Param(x): The upper-left |x| position of the rectangle.
// Param(y): The upper-left |y| position of the rectangle.
// Param(width): The |width| of the rectangle.
// Param(height): The |height| of the rectangle.

void Graphics::drawRectangle(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc)
        return;
               
    if (env->getParamCount() < 4)
        return;
   
    m_dc->DrawRectangle(env->getParam(0)->getInteger(),
                        env->getParam(1)->getInteger(),
                        env->getParam(2)->getInteger(),
                        env->getParam(3)->getInteger());
}

// (METHOD) Graphics.drawText
// Description: Draws text on the graphics object.
//
// Syntax: function Graphics.drawText(text : String,
//                                    x : Integer,
//                                    y : Integer)
//
// Remarks: Draws the specified |text| at the position (|x|, |y|),
//     using the currently set font.
//
// Param(text): The |text| to draw.
// Param(x): The upper-left |x| position of the text.
// Param(y): The upper-left |y| position of the text.

// TODO: need methods for foreground and background color text (unless
//     we want to use the color of the pen and brush - which is how we
//     do it in kcanvas)

// TODO: need getTextExtent()

void Graphics::drawText(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc)
        return;
               
    if (env->getParamCount() < 3)
        return;
    
    m_dc->DrawText(env->getParam(0)->getString(),
                   env->getParam(1)->getInteger(),
                   env->getParam(2)->getInteger());
}

// (METHOD) Graphics.setPen
// Description: Sets the current pen to use when drawing the foreground
//     on the graphics object.
//
// Syntax: function Graphics.setPen(pen : Pen)
//
// Remarks: Sets the current |pen| to use when drawing the foreground
//     on the graphics object.
//
// Param(pen): The |pen| to use when drawing the foreground
//     on the graphics object.

void Graphics::setPen(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc || 
         env->getParamCount() < 1 ||
         env->getParam(0)->getType() != kscript::Value::typeObject)
    {
        return;
    }
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Pen")
    {
        Pen* pen = (Pen*)obj;
        m_dc->SetPen(pen->getWxPen());
    }
}

// (METHOD) Graphics.setBrush
// Description: Sets the current brush to use when drawing the background
//     on the graphics object.
//
// Syntax: function Graphics.setBrush(brush : Brush)
//
// Remarks: Sets the current |brush| to use when drawing the background
//     on the graphics object.
//
// Param(brush): The |brush| to use when drawing the foreground
//     on the graphics object.

void Graphics::setBrush(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc || 
         env->getParamCount() < 1 ||
         env->getParam(0)->getType() != kscript::Value::typeObject)
    {
        return;
    }
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Brush")
    {
        Brush* brush = (Brush*)obj;
        m_dc->SetBrush(brush->getWxBrush());
    }
}

// (METHOD) Graphics.setFont
// Description: Sets the current font to use when drawing text
//     on the graphics object.
//
// Syntax: function Graphics.setFont(font : Font)
//
// Remarks: Sets the current |font| to use when drawing text
//     on the graphics object.
//
// Param(font): The |font| to use when drawing text 
//     on the graphics object.

void Graphics::setFont(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc || 
         env->getParamCount() < 1 ||
         env->getParam(0)->getType() != kscript::Value::typeObject)
    {
        return;
    }
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Font")
    {
        zFont* font = (zFont*)obj;
        m_dc->SetFont(font->getWxFont());
    }
}

// (METHOD) Graphics.setTextForeground
// Description: Sets the foreground color to use when drawing text
//     on the graphics object.
//
// Syntax: function Graphics.setTextForeground(color : Color)
//
// Remarks: Sets the current |color| to use when drawing text
//     on the graphics object.
//
// Param(color): The |color| to use when drawing text 
//     on the graphics object.

void Graphics::setTextForeground(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc || 
         env->getParamCount() < 1 ||
         env->getParam(0)->getType() != kscript::Value::typeObject)
    {
        return;
    }
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Color")
    {
        Color* c = (Color*)obj;
        
        wxColour color(c->getMember(L"red")->getInteger(),
                       c->getMember(L"green")->getInteger(),
                       c->getMember(L"blue")->getInteger());
        m_dc->SetTextForeground(color);
    }
}

// (METHOD) Graphics.setTextBackground
// Description: Sets the background color to use when drawing text
//     on the graphics object.
//
// Syntax: function Graphics.setTextBackground(color : Color)
//
// Remarks: Sets the current |color| to use when drawing text
//     on the graphics object.
//
// Param(color): The |color| to use when drawing text 
//     on the graphics object.

void Graphics::setTextBackground(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_dc || 
         env->getParamCount() < 1 ||
         env->getParam(0)->getType() != kscript::Value::typeObject)
    {
        return;
    }
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Color")
    {
        Color* c = (Color*)obj;
        
        wxColour color(c->getMember(L"red")->getInteger(),
                       c->getMember(L"green")->getInteger(),
                       c->getMember(L"blue")->getInteger());
        m_dc->SetTextBackground(color);
    }
}

// (METHOD) Graphics.fromBitmap
// Description: Creates a new graphics object from a specified bitmap.
//
// Syntax: static function Graphics.fromBitmap(bitmap : Bitmap)
//
// Remarks: Creates a new graphics object from a specified |bitmap|.
// Returns: Returns a new graphics object.

// TODO: this seems like it should be done in the constructor, following
//     the pattern of many of the rest of the API.

void Graphics::fromBitmap(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || env->getParam(0)->getType() != kscript::Value::typeObject)
    {
        retval->setNull();
        return;
    }
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (!obj->isKindOf(L"Bitmap"))
    {
        retval->setNull();
        return;
    }
    
    Bitmap* bitmap = (Bitmap*)obj;
    

    Graphics* g = Graphics::createObject(env);
    g->m_bmp = bitmap->getWxBitmap();
    g->m_memdc = new wxMemoryDC;
    g->m_memdc->SelectObject(g->m_bmp);
    g->m_dc = g->m_memdc;
    g->init();
    
    retval->setObject(g);
}

// TODO: formalize the names of colors.  Instead of red, green, etc.,
//     we should probably use Color.Red, Color.Green, etc, in keeping
//     with the pattern of using the object prefix.

wxColor getColorFromText(const wchar_t* text)
{
    wxString s = text;
    s.MakeLower();
    
    if (s == "black")
        return *wxBLACK;
     else if (s == "white")
        return *wxWHITE;
     else if (s == "red")
        return *wxRED;
     else if (s == "blue")
        return *wxBLUE;
     else if (s == "green")
        return *wxGREEN;
     else if (s == "cyan")
        return *wxCYAN;
     else if (s == "grey" || s == "gray")
        return wxColor(128,128,128);
     else if (s == "lightgrey" || s == "lightgray")
        return wxColor(192,192,192);
     
     return *wxBLACK;
}


// -- Pen class implementation --

// (CLASS) Pen
// Category: Graphics
// Description: A class that encapsulates a pen object, which is used for 
//     drawing the foreground on a graphics object.
// Remarks: Encapsulates a pen object, which is used for drawing
//     the foreground on a graphics object.

Pen::Pen()
{
}

Pen::~Pen()
{
}

wxPen& Pen::getWxPen()
{
    return m_pen;
}

// (CONSTRUCTOR) Pen.constructor
// Description: Creates a new Pen object.
//
// Syntax: Pen(color : Color, thickness : Number)
// Syntax: Pen(name : String, thickness : Number)
//
// Remarks: Creates a new Pen object with the specified |color| or
//     with the specified color |name|.
//
// Param(color): The |color| to use to draw the foreground.
// Param(name): The |name| of the color to use to draw the foreground.

void Pen::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    size_t param_count = env->getParamCount();
    
    if (param_count < 1)
    {
        m_pen = *wxBLACK_PEN;
        return;
    }
        

    int thickness = 1;
    if (env->getParamCount() > 1)
    {
        thickness = env->getParam(1)->getInteger();
    }



    wxColor color = *wxBLACK;
    
    if (env->getParam(0)->isString())
    {
        m_pen = wxPen(getColorFromText(env->getParam(0)->getString()), thickness);
        return;
    }
     else if (env->getParam(0)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        
        if (obj->getClassName() == L"Color")
        {
            Color* col = (Color*)obj;
            color = wxColor(col->getMember(L"red")->getInteger(),
                            col->getMember(L"green")->getInteger(),
                            col->getMember(L"blue")->getInteger());
        }
    }

    m_pen = wxPen(color, thickness);
}

// -- Brush class implementation --

// (CLASS) Brush
// Category: Graphics
// Description: A class that encapsulates a brush object, which is used for 
//     drawing the background on a graphics object.
// Remarks: Encapsulates a brush object, which is used for drawing
//     the background on a graphics object.

Brush::Brush()
{
}

Brush::~Brush()
{
}

wxBrush& Brush::getWxBrush()
{
    return m_brush;
}

// (CONSTRUCTOR) Brush.constructor
// Description: Creates a new Brush object.
//
// Syntax: Brush(color : Color)
// Syntax: Brush(name : String)
//
// Remarks: Creates a new Brush object with the specified |color| or
//     with the specified color |name|.
//
// Param(color): The |color| to use to draw the background.
// Param(name): The |name| of the color to use to draw the background.

void Brush::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    size_t param_count = env->getParamCount();
    
    if (param_count < 1)
    {
        m_brush = *wxBLACK_BRUSH;
        return;
    }
        

    wxColor color = *wxBLACK;
    
    if (env->getParam(0)->isString())
    {
        m_brush = wxBrush(getColorFromText(env->getParam(0)->getString()));
        return;
    }
     else if (env->getParam(0)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        
        if (obj->getClassName() == L"Color")
        {
            Color* col = (Color*)obj;
            color = wxColor(col->getMember(L"red")->getInteger(),
                            col->getMember(L"green")->getInteger(),
                            col->getMember(L"blue")->getInteger());
        }
    }
    
    
    m_brush = wxBrush(color);
}



