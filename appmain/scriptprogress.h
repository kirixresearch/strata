/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-10
 *
 */


#ifndef H_APP_SCRIPTPROGRESS_H
#define H_APP_SCRIPTPROGRESS_H


class ProgressBar : public FormControl
{
friend class ProgressBarTimer;
    
public:

    BEGIN_KSCRIPT_DERIVED_CLASS("ProgressBar", ProgressBar, FormControl)

        KSCRIPT_GUI_METHOD("constructor", ProgressBar::constructor)
        KSCRIPT_GUI_METHOD("getIndeterminate", ProgressBar::getIndeterminate)
        KSCRIPT_GUI_METHOD("getMaximum", ProgressBar::getMaximum)
        KSCRIPT_METHOD    ("getValue", ProgressBar::getValue)
        KSCRIPT_GUI_METHOD("setIndeterminate", ProgressBar::setIndeterminate)
        KSCRIPT_GUI_METHOD("setMaximum", ProgressBar::setMaximum)
        KSCRIPT_GUI_METHOD("setValue", ProgressBar::setValue)
        
        KSCRIPT_CONSTANT_INTEGER("Horizontal",  Horizontal)
        KSCRIPT_CONSTANT_INTEGER("Vertical",    Vertical)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Horizontal = wxGA_HORIZONTAL,
        Vertical = wxGA_VERTICAL
    };

public:

    ProgressBar();
    ~ProgressBar();
    
    void realize();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setMaximum(kscript::ExprEnv* env, kscript::Value* retval);
    void getMaximum(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void setIndeterminate(kscript::ExprEnv* env, kscript::Value* retval);
    void getIndeterminate(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    void onEvent(wxEvent& event);
    
private:

    wxGauge* m_ctrl;
    wxTimer* m_timer;
    int m_value;
    bool m_indeterminate;
};


#endif

