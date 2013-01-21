/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-08-03
 *
 */


#ifndef __APP_SCRIPTSLIDER_H
#define __APP_SCRIPTSLIDER_H


class Slider : public FormControl
{    
public:

    BEGIN_KSCRIPT_DERIVED_CLASS("Slider", Slider, FormControl)

        KSCRIPT_GUI_METHOD("constructor", Slider::constructor)
        KSCRIPT_GUI_METHOD("setValue", Slider::setValue)
        KSCRIPT_GUI_METHOD("getValue", Slider::getValue)
        KSCRIPT_GUI_METHOD("setRange", Slider::setRange)
        KSCRIPT_GUI_METHOD("getMin", Slider::getMin)
        KSCRIPT_GUI_METHOD("getMax", Slider::getMax)
        KSCRIPT_GUI_METHOD("setPageSize", Slider::setPageSize)
        KSCRIPT_GUI_METHOD("getPageSize", Slider::getPageSize)
        KSCRIPT_GUI_METHOD("setLineSize", Slider::setLineSize)
        KSCRIPT_GUI_METHOD("getLineSize", Slider::getLineSize)

        KSCRIPT_CONSTANT_INTEGER("Horizontal",  Horizontal)
        KSCRIPT_CONSTANT_INTEGER("Vertical",    Vertical)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Horizontal = wxSL_HORIZONTAL,
        Vertical = wxSL_VERTICAL
    };

public:

    Slider();
    ~Slider();
    void realize();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void setRange(kscript::ExprEnv* env, kscript::Value* retval);
    void getMin(kscript::ExprEnv* env, kscript::Value* retval);
    void getMax(kscript::ExprEnv* env, kscript::Value* retval);
    void setPageSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getPageSize(kscript::ExprEnv* env, kscript::Value* retval);
    void setLineSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getLineSize(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(wxEvent& event);
    
private:

    wxSlider* m_ctrl;
};


#endif
