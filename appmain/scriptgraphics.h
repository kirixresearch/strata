/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2006-12-23
 *
 */


#ifndef H_APP_SCRIPTGRAPHICS_H
#define H_APP_SCRIPTGRAPHICS_H


#include "scriptgui.h"


class Bitmap;

class Graphics : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("Graphics", Graphics)
        KSCRIPT_GUI_METHOD("constructor", Graphics::constructor)
        KSCRIPT_GUI_METHOD("drawLine", Graphics::drawLine)
        KSCRIPT_GUI_METHOD("drawCircle", Graphics::drawCircle)
        KSCRIPT_GUI_METHOD("drawArc", Graphics::drawArc)
        KSCRIPT_GUI_METHOD("drawEllipticArc", Graphics::drawEllipticArc)
        KSCRIPT_GUI_METHOD("drawImage", Graphics::drawImage)
        KSCRIPT_GUI_METHOD("drawRectangle", Graphics::drawRectangle)
        KSCRIPT_GUI_METHOD("drawText", Graphics::drawText)
        KSCRIPT_GUI_METHOD("setPen", Graphics::setPen)
        KSCRIPT_GUI_METHOD("setBrush", Graphics::setBrush)
        KSCRIPT_GUI_METHOD("setFont", Graphics::setFont)
        KSCRIPT_GUI_METHOD("setTextForeground", Graphics::setTextForeground)
        KSCRIPT_GUI_METHOD("setTextBackground", Graphics::setTextBackground)
        KSCRIPT_STATIC_METHOD("fromBitmap", Graphics::fromBitmap)
    END_KSCRIPT_CLASS()

public:

    Graphics();
    ~Graphics();

    void init();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void drawLine(kscript::ExprEnv* env, kscript::Value* retval);
    void drawCircle(kscript::ExprEnv* env, kscript::Value* retval);
    void drawArc(kscript::ExprEnv* env, kscript::Value* retval);
    void drawEllipticArc(kscript::ExprEnv* env, kscript::Value* retval);
    void drawImage(kscript::ExprEnv* env, kscript::Value* retval);
    void drawRectangle(kscript::ExprEnv* env, kscript::Value* retval);
    void drawText(kscript::ExprEnv* env, kscript::Value* retval);
    static void fromBitmap(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    void setPen(kscript::ExprEnv* env, kscript::Value* retval);
    void setBrush(kscript::ExprEnv* env, kscript::Value* retval);
    void setFont(kscript::ExprEnv* env, kscript::Value* retval);
    void setTextForeground(kscript::ExprEnv* env, kscript::Value* retval);
    void setTextBackground(kscript::ExprEnv* env, kscript::Value* retval);

    void incrementPaintCount();
    void decrementPaintCount();
    int getPaintCount();
    
public:

    wxDC* m_dc;
    wxWindow* m_wnd;
    
    wxBitmap m_bmp;
    wxMemoryDC* m_memdc;
    
    kl::mutex m_obj_mutex;
    int m_paint_count;
};



class Pen : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("Pen", Pen)
        KSCRIPT_GUI_METHOD("constructor", Pen::constructor)
    END_KSCRIPT_CLASS()

public:

    Pen();
    ~Pen();
    
    wxPen& getWxPen();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

private:

    wxPen m_pen;
};



class Brush : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("Brush", Brush)
        KSCRIPT_GUI_METHOD("constructor", Brush::constructor)
    END_KSCRIPT_CLASS()

public:

    Brush();
    ~Brush();
    
    wxBrush& getWxBrush();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

private:

    wxBrush m_brush;
};




#endif

