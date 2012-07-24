/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-10
 *
 */


#include "appmain.h"
#include "scriptgui.h"
#include "scriptprogress.h"


class ProgressBarTimer : public wxTimer
{
public:
    
    ProgressBarTimer(ProgressBar* progress)
    {
        m_progress = progress;
    }
    
    void Notify()
    {
        if (!m_progress)
            return;
        
        m_progress->m_ctrl->Pulse();
    }
    
private:
    
    ProgressBar* m_progress;
};

// -- ProgressBar class implementation --

// (CLASS) ProgressBar
// Category: Control
// Derives: FormControl
// Description: A class that represents a progress bar control.
// Remarks: The ProgressBar class represents a progress bar control.
//
// Property(ProgressBar.Horizontal):   A flag representing that a horizontal progress bar should be created.
// Property(ProgressBar.Vertical):     A flag representing that a vertical progress bar should be created.

ProgressBar::ProgressBar()
{
    m_ctrl = NULL;
    m_indeterminate = false;
    m_value = 0;
    
    // -- create a timer that will be used if
    m_timer = new ProgressBarTimer(this);
}

ProgressBar::~ProgressBar()
{
    delete m_timer;
}


// (CONSTRUCTOR) ProgressBar.constructor
// Description: Creates a new ProgressBar.
//
// Syntax: ProgressBar(orientation : Integer,
//                     x_pos : Integer,
//                     y_pos : Integer,
//                     width : Integer,
//                     height : Integer)
//
// Remarks: Creates a new ProgressBar object with the specified |orientation|
//     at the position specified by |x_pos| and |y_pos|, and having
//     dimensions specified by |width| and |height|.  If |orientation| is
//     unspecified or is ProgressBar.Horizontal, a horizontal ProgressBar
//     object will be created.  If |orientation| is ProgressBar.Vertical,
//     a vertical ProgressBar object will be created.
//
// Param(orientation): Determines the manner in which to
//     create the ProgressBar object, depending on whether
//     it's ProgressBar.Horizontal or ProgressBar.Vertical.
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void ProgressBar::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- initialize the component --
    initComponent(env);

    // -- default progress value --
    m_value = 0;
    
    // -- default style --
    int orientation = ProgressBar::Horizontal;
    
    // -- set default values --
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // -- get user input values --
    if (param_count > 0)
        orientation = env->getParam(0)->getInteger();
    if (param_count > 1)
        m_x = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_y = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_width = env->getParam(3)->getInteger();
    if (param_count > 4)
        m_height = env->getParam(4)->getInteger();

    if (param_count < 1)
    {
        // -- create the control --
        m_ctrl = new wxGauge(getApp()->getTempParent(),
                             -1,
                             100,
                             wxDefaultPosition,
                             wxDefaultSize,
                                (orientation == ProgressBar::Vertical)
                                    ? wxGA_VERTICAL
                                    : wxGA_HORIZONTAL);
        m_wnd = m_ctrl;

        // if no sizing parameters were specified, used wx's default
        // parameters, since they are equal to the exact size of the text
        wxSize s = m_ctrl->GetSize();
        m_width = s.GetWidth();
        m_height = s.GetHeight();
    }
     else
    {
        // -- create the control --
        m_ctrl = new wxGauge(getApp()->getTempParent(),
                             -1,
                             100,
                             wxPoint(m_x, m_y),
                             wxSize(m_width, m_height),
                                (orientation == ProgressBar::Vertical)
                                    ? wxGA_VERTICAL
                                    : wxGA_HORIZONTAL);
        m_wnd = m_ctrl;

        // make sure we update the m_width and m_height member
        // variables based on the actual size of the control
        wxSize s = m_ctrl->GetSize();
        if (m_width != s.GetWidth())
            m_width = s.GetWidth();
        if (m_height != s.GetHeight())
            m_height = s.GetHeight();
    }
}

void ProgressBar::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (!m_enabled)
        m_ctrl->Enable(false);
    
    if (m_indeterminate)
        m_timer->Start(70);
         else
        m_ctrl->SetValue(m_value);
}

// (METHOD) ProgressBar.setMaximum
// Description: Sets the maximum value the progress bar can have.
//
// Syntax: function ProgressBar.setMaximum(number : Integer)
//
// Remarks: Sets the maximum value the progress bar can have.
//
// Param(number): The maximum value the progress bar can have.

void ProgressBar::setMaximum(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    int val = env->getParam(0)->getInteger();
    m_ctrl->SetRange(val);
}

// (METHOD) ProgressBar.getMaximum
// Description: Gets the maximum value the progress bar can have.
//
// Syntax: function ProgressBar.getMaximum() : Integer
//
// Remarks: Returns the maximum value the progress bar can have.
//
// Returns: Returns the maximum value the progress bar can have.

void ProgressBar::getMaximum(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int val = m_ctrl->GetRange();
    retval->setInteger(val);
}

// (METHOD) ProgressBar.setValue
// Description: Sets the current value of the progress bar.
//
// Syntax: function ProgressBar.setValue(number : Integer)
//
// Remarks: Sets the current value of the progress bar.
//
// Param(number): The current value of the progress bar.

void ProgressBar::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
    
    m_value = env->getParam(0)->getInteger();
    m_indeterminate = false;
    
    if (m_ctrl)
    {
        m_ctrl->SetValue(m_value);
    }
}

// (METHOD) ProgressBar.getValue
// Description: Gets the current value of the progress bar.
//
// Syntax: function ProgressBar.getValue() : Integer
//
// Remarks: Returns the current value of the progress bar.
//
// Returns: Returns the current value of the progress bar.

void ProgressBar::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_indeterminate)
    {
        // we're in pulse mode and we don't want to return
        // an indeterminate value
        retval->setInteger(0);
        return;
    }

    retval->setInteger(m_value);
}

// (METHOD) ProgressBar.setIndeterminate
// Description: Sets the indeterminate mode of the progress bar.
//
// Syntax: function ProgressBar.setIndeterminate(flag : Boolean)
//
// Remarks: Turns on the indeterminate mode if the |flag| is set to true.
//     Turns off the indeterminate mode if the |flag| is set to false.
//
// Param(flag): A |flag| indicating whether the progress bar should be
//     in indeterminate mode or not.

void ProgressBar::setIndeterminate(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        m_indeterminate = true;
    }
     else
    {   
        m_indeterminate = env->getParam(0)->getBoolean();
    }
    
    
    if (m_indeterminate)
    {
        m_timer->Start(30);
    }
     else
    {
        if (m_timer->IsRunning())
            m_timer->Stop();
    }
    
    
    if (isControlValid())
    {
        if (!m_indeterminate)
        {
            m_ctrl->SetValue(0);
            m_value = 0;
        }
    }
}

// (METHOD) ProgressBar.getIndeterminate
// Description: Indicates whether the progress bar is in indeterminate mode or not.
//
// Syntax: function ProgressBar.getIndeterminate() : Boolean
//
// Remarks: Indicates whether the progress bar is in indeterminate mode or not.  If
//     the progress bar is in indeterminate mode, the function returns true.  If the
//     progress bar is not in indeterminate mode, the function returns false.
//
// Returns: Returns true if the progress bar is in indeterminate mode, and false otherwise.

void ProgressBar::getIndeterminate(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(m_indeterminate);
}

void ProgressBar::onEvent(wxEvent& evt)
{
}



