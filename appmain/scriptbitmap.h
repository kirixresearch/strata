/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-01-03
 *
 */


#ifndef __APP_SCRIPTBITMAP_H
#define __APP_SCRIPTBITMAP_H


#include "scriptgui.h"


class Bitmap : public ScriptHostBase
{
public:

    BEGIN_KSCRIPT_CLASS("Bitmap", Bitmap)

        KSCRIPT_GUI_METHOD("constructor", Bitmap::constructor)
        KSCRIPT_GUI_METHOD("loadFile", Bitmap::loadFile)
        KSCRIPT_GUI_METHOD("saveFile", Bitmap::saveFile)
        KSCRIPT_GUI_METHOD("setHeight", Bitmap::setHeight)
        KSCRIPT_GUI_METHOD("getHeight", Bitmap::getHeight)
        KSCRIPT_GUI_METHOD("setWidth", Bitmap::setWidth)
        KSCRIPT_GUI_METHOD("getWidth", Bitmap::getWidth)
        KSCRIPT_GUI_METHOD("isOk", Bitmap::isOk)
        //KSCRIPT_GUI_METHOD("setColorDepth", Bitmap::setColorDepth)
        //KSCRIPT_GUI_METHOD("getColorDepth", Bitmap::getColorDepth)
        
        KSCRIPT_CONSTANT_INTEGER("FormatBMP", FormatBMP)
        KSCRIPT_CONSTANT_INTEGER("FormatGIF", FormatGIF)
        KSCRIPT_CONSTANT_INTEGER("FormatXPM", FormatXPM)
        KSCRIPT_CONSTANT_INTEGER("FormatPNG", FormatPNG)
        KSCRIPT_CONSTANT_INTEGER("FormatJPG", FormatJPG)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        FormatDefault = 1,
        FormatGIF = 2,
        FormatBMP = 3,
        FormatPNG = 4,
        FormatXPM = 5,
        FormatJPG = 6
    };

public:

    Bitmap();
    ~Bitmap();
    
    wxBitmap& getWxBitmap();
    void setWxBitmap(const wxBitmap& bmp);
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void loadFile(kscript::ExprEnv* env, kscript::Value* retval);
    void saveFile(kscript::ExprEnv* env, kscript::Value* retval);
    void setHeight(kscript::ExprEnv* env, kscript::Value* retval);
    void getHeight(kscript::ExprEnv* env, kscript::Value* retval);
    void setWidth(kscript::ExprEnv* env, kscript::Value* retval);
    void getWidth(kscript::ExprEnv* env, kscript::Value* retval);
    void isOk(kscript::ExprEnv* env, kscript::Value* retval);

private:

    wxBitmap m_bitmap;
};


#endif
