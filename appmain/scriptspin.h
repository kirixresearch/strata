/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-09
 *
 */


#ifndef __APP_SCRIPTSPIN_H
#define __APP_SCRIPTSPIN_H


class SpinButton : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("SpinButton", SpinButton, FormControl)
        KSCRIPT_GUI_METHOD("constructor", SpinButton::constructor)
        KSCRIPT_GUI_METHOD("setValue", SpinButton::setValue)
        KSCRIPT_GUI_METHOD("getValue", SpinButton::getValue)
        KSCRIPT_GUI_METHOD("setRange", SpinButton::setRange)
        KSCRIPT_GUI_METHOD("getMin", SpinButton::getMin)
        KSCRIPT_GUI_METHOD("getMax", SpinButton::getMax)
    END_KSCRIPT_CLASS()

public:

    SpinButton();
    ~SpinButton();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void setRange(kscript::ExprEnv* env, kscript::Value* retval);
    void getMin(kscript::ExprEnv* env, kscript::Value* retval);
    void getMax(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(wxEvent& event);

private:

    wxSpinButton* m_ctrl;
};



class SpinBox : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("SpinBox", SpinBox, FormControl)
        KSCRIPT_GUI_METHOD("constructor", SpinBox::constructor)
        KSCRIPT_GUI_METHOD("setValue", SpinBox::setValue)
        KSCRIPT_GUI_METHOD("getValue", SpinBox::getValue)
        KSCRIPT_GUI_METHOD("setRange", SpinBox::setRange)
        KSCRIPT_GUI_METHOD("getMin", SpinBox::getMin)
        KSCRIPT_GUI_METHOD("getMax", SpinBox::getMax)
    END_KSCRIPT_CLASS()

public:

    SpinBox();
    ~SpinBox();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void setRange(kscript::ExprEnv* env, kscript::Value* retval);
    void getMin(kscript::ExprEnv* env, kscript::Value* retval);
    void getMax(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(wxEvent& event);

private:

    wxSpinCtrl* m_ctrl;
};




#endif
