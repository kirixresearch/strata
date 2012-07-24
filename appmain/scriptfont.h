/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-07-10
 *
 */


#ifndef __APP_SCRIPTFONT_H
#define __APP_SCRIPTFONT_H


class zFont : public ScriptHostBase
{
public:

    BEGIN_KSCRIPT_CLASS("Font", zFont)

        KSCRIPT_GUI_METHOD("constructor", zFont::constructor)
        KSCRIPT_GUI_METHOD("setPointSize", zFont::setPointSize)
        KSCRIPT_GUI_METHOD("setFamily", zFont::setFamily)
        KSCRIPT_GUI_METHOD("setStyle", zFont::setStyle)
        KSCRIPT_GUI_METHOD("setFaceName", zFont::setFaceName)
        KSCRIPT_GUI_METHOD("getPointSize", zFont::getPointSize)
        KSCRIPT_GUI_METHOD("getFamily", zFont::getFamily)
        KSCRIPT_GUI_METHOD("getStyle", zFont::getStyle)
        KSCRIPT_GUI_METHOD("getFaceName", zFont::getFaceName)
        
        KSCRIPT_CONSTANT_INTEGER("Default",    Default)
        KSCRIPT_CONSTANT_INTEGER("Roman",      Roman)
        KSCRIPT_CONSTANT_INTEGER("Swiss",      Swiss)
        KSCRIPT_CONSTANT_INTEGER("Modern",     Modern)
        
        KSCRIPT_CONSTANT_INTEGER("Normal",     Normal)
        KSCRIPT_CONSTANT_INTEGER("Bold",       Bold)
        KSCRIPT_CONSTANT_INTEGER("Italic",     Italic)
        KSCRIPT_CONSTANT_INTEGER("Underlined", Underlined)

    END_KSCRIPT_CLASS()

public:

    enum FontFamily
    {
        Default,
        Roman,
        Swiss,
        Modern
    };
    
    enum FontStyle
    {
        Normal     = 0x00,
        Bold       = 0x01,
        Italic     = 0x02,
        Underlined = 0x04
    };

public:

    zFont();
    ~zFont();
    
    wxFont& getWxFont();
    void setWxFont(const wxFont& font);
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setPointSize(kscript::ExprEnv* env, kscript::Value* retval);
    void setFamily(kscript::ExprEnv* env, kscript::Value* retval);
    void setStyle(kscript::ExprEnv* env, kscript::Value* retval);
    void setFaceName(kscript::ExprEnv* env, kscript::Value* retval);
    void getPointSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getFamily(kscript::ExprEnv* env, kscript::Value* retval);
    void getStyle(kscript::ExprEnv* env, kscript::Value* retval);
    void getFaceName(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    wxFont m_font;
    
    int m_pointsize;
    int m_family;
    int m_style;
    wxString m_facename;
};


#endif  // __APP_SCRIPTFONT_H


