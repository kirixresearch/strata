/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-09
 *
 */


#include "appmain.h"
#include "scriptgui.h"
#include "scriptspin.h"
#include "scriptapp.h"


// -- SpinButton class implementation --

// (CLASS) SpinButton
// Category: Control
// Derives: FormControl
// Description: A class that represents a spin button control.
// Remarks: The SpinButton class represents a spin button control.

SpinButton::SpinButton()
{
    m_ctrl = NULL;
}

SpinButton::~SpinButton()
{

}

// (CONSTRUCTOR) SpinButton.constructor
// Description: Creates a new SpinButton.
//
// Syntax: SpinButton(x_pos : Integer,
//                    y_pos : Integer,
//                    width : Integer,
//                    height : Integer)
//
// Remarks: Creates a new SpinButton at the position specified by 
//     |x_pos| and |y_pos|, and having dimensions specified by |width| 
//     and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void SpinButton::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- initialize the component --
    initComponent(env);

    // -- add some properties --
    kscript::Value obj;
    
    getMember(L"changed")->setObject(Event::createObject(env));
    getMember(L"spinUp")->setObject(Event::createObject(env));
    getMember(L"spinDown")->setObject(Event::createObject(env));
    
    // -- set default values --
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // -- get user input values --
    if (param_count > 0)
        m_x = env->getParam(0)->getInteger();
    if (param_count > 1)
        m_y = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_width = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_height = env->getParam(3)->getInteger();

    if (param_count < 1)
    {
        // -- create the control --
        m_ctrl = new wxSpinButton(getApp()->getTempParent(),
                                  -1,
                                  wxDefaultPosition,
                                  wxDefaultSize);
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
        m_ctrl = new wxSpinButton(getApp()->getTempParent(),
                                  -1,
                                  wxPoint(m_x, m_y),
                                  wxSize(m_width, m_height));
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

void SpinButton::realize()
{
    m_ctrl->Reparent(m_form_wnd);

    if (!m_enabled)
        m_ctrl->Enable(false);

    listenEvent(wxEVT_SCROLL_LINEUP);
    listenEvent(wxEVT_SCROLL_LINEDOWN);
    listenEvent(wxEVT_SCROLL_THUMBTRACK);
}

// (METHOD) SpinButton.setValue
// Description: Sets the value of the spin button.
//
// Syntax: function SpinButton.setValue(number : Integer)
//
// Remarks: Sets the value of the spin button to |number|.
//
// Param(number): The value to which to set the spin button value.

void SpinButton::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    int val = env->getParam(0)->getInteger();
    m_ctrl->SetValue(val);
}

// (METHOD) SpinButton.getValue
// Description: Gets the value of the spin button.
//
// Syntax: function SpinButton.getValue() : Integer
//
// Remarks: Returns the current value of the spin button.
//
// Returns: Returns the current value of the spin button.

void SpinButton::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int val = m_ctrl->GetValue();
    retval->setInteger(val);
}

// (METHOD) SpinButton.setRange
// Description: Sets the minimum and maximum range of the spin button.
//
// Syntax: function SpinButton.setRange(minimum : Integer,
//                                      maximum : Integer)
//
// Remarks: Sets the |minimum| and |maximum| range of the spin button,
//     limiting the value the spin button can have to this range.
//
// Param(minimum): The |minimum| value the spin button value can have.
// Param(maximum): The |maximum| value the spin button value can have.

void SpinButton::setRange(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 2 || !isControlValid())
        return;
        
    int min = env->getParam(0)->getInteger();
    int max = env->getParam(1)->getInteger();
    m_ctrl->SetRange(min, max);
}

// (METHOD) SpinButton.getMin
// Description: Gets the minimum value the spin button can have.
//
// Syntax: function SpinButton.getMin() : Integer
//
// Remarks: Returns the minimum value the spin button can have.
//
// Returns: Returns the minimum value the spin button can have.

void SpinButton::getMin(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int min = m_ctrl->GetMin();
    retval->setInteger(min);
}

// (METHOD) SpinButton.getMax
// Description: Gets the maximum value the spin button can have.
//
// Syntax: function SpinButton.getMax() : Integer
//
// Remarks: Returns the maximum value the spin button can have.
//
// Returns: Returns the maximum value the spin button can have.

void SpinButton::getMax(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int max = m_ctrl->GetMax();
    retval->setInteger(max);
}


// (EVENT) SpinButton.changed
// Description: Fired when the spin button's value is changed.
// Param(value): The new value of the spin button.

// (EVENT) SpinButton.spinUp
// Description: Fired when the menu item is clicked.
// Param(value): The new value of the spin button.

// (EVENT) SpinButton.spinDown
// Description: Fired when the menu item is clicked.
// Param(value): The new value of the spin button.

void SpinButton::onEvent(wxEvent& event)
{
    wxCommandEvent& evt = (wxCommandEvent&)event;
    int val = evt.GetInt();
    
    kscript::Value* event_args = new kscript::Value;
    event_args->setObject();
    event_args->getMember(L"value")->setInteger(val);

    if (event.GetEventType() == wxEVT_SCROLL_THUMBTRACK)
    {
        invokeJsEvent(L"changed", event_args);
    }
     else if (event.GetEventType() == wxEVT_SCROLL_LINEUP)
    {
        invokeJsEvent(L"spinUp", event_args);
    }
     else if (event.GetEventType() == wxEVT_SCROLL_LINEDOWN)
    {
        invokeJsEvent(L"spinDown", event_args);
    }
}




// -- SpinBox class implementation --

// (CLASS) SpinBox
// Category: Control
// Derives: FormControl
// Description: A class that represents a spin box control.
// Remarks: The SpinBox class represents a spin box control.

SpinBox::SpinBox()
{
    m_ctrl = NULL;
}

SpinBox::~SpinBox()
{

}

// (CONSTRUCTOR) SpinBox.constructor
// Description: Creates a new SpinBox.
//
// Syntax: SpinBox(x_pos : Integer,
//                 y_pos : Integer,
//                 width : Integer,
//                 height : Integer)
//
// Remarks: Creates a new SpinBox at the position specified by 
//     |x_pos| and |y_pos|, and having dimensions specified by 
//     |width| and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.


void SpinBox::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- initialize the component --
    initComponent(env);

    // -- add some properties --
    getMember(L"changed")->setObject(Event::createObject(env));
    getMember(L"spinUp")->setObject(Event::createObject(env));
    getMember(L"spinDown")->setObject(Event::createObject(env));
    getMember(L"textChanged")->setObject(Event::createObject(env));
    getMember(L"enterPressed")->setObject(Event::createObject(env));
    
    // -- set default values --
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // -- get user input values --
    if (param_count > 0)
        m_x = env->getParam(0)->getInteger();
    if (param_count > 1)
        m_y = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_width = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_height = env->getParam(3)->getInteger();

    if (param_count < 1)
    {
        // -- create the control --
        m_ctrl = new wxSpinCtrl(getApp()->getTempParent(),
                                -1,
                                wxEmptyString,
                                wxDefaultPosition,
                                wxDefaultSize);
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
        m_ctrl = new wxSpinCtrl(getApp()->getTempParent(),
                                -1,
                                wxEmptyString,
                                wxPoint(m_x, m_y),
                                wxSize(m_width, m_height));
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

void SpinBox::realize()
{
    // store old variables before we destroy the control
    int val = m_ctrl->GetValue();
    int min = m_ctrl->GetMin();
    int max = m_ctrl->GetMax();
    long style = m_ctrl->GetWindowStyle();
    
    // normally we would call m_ctrl->Reparent(m_form_wnd) here,
    // except that the Reparent() function only reparents the
    // wxSpinButton inside of the wxSpinCtrl and not the textbox
    // portion... so we need to destroy the only wxSpinCtrl and
    // recreate it with the right parent
    
    m_ctrl->Destroy();
    m_ctrl = new wxSpinCtrl(m_form_wnd,
                            -1,
                            wxEmptyString,
                            wxPoint(m_x, m_y),
                            wxSize(m_width, m_height),
                            style,
                            min,
                            max,
                            val);
    m_wnd = m_ctrl;
    
    if (!m_enabled)
        m_ctrl->Enable(false);

    listenEvent(wxEVT_SCROLL_LINEUP);
    listenEvent(wxEVT_SCROLL_LINEDOWN);
    listenEvent(wxEVT_SCROLL_THUMBTRACK);
    listenEvent(wxEVT_COMMAND_TEXT_UPDATED);
    listenEvent(wxEVT_COMMAND_TEXT_ENTER);
}


// (METHOD) SpinBox.setValue
// Description: Sets the value of the spin box.
//
// Syntax: function SpinBox.setValue(number : Integer)
//
// Remarks: Sets the value of the spin box to |number|.
//
// Param(number): The value to which to set the spin box value.

void SpinBox::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() == 0)
        return;
        
    int val = env->getParam(0)->getInteger();
    m_ctrl->SetValue(val);
}

// (METHOD) SpinBox.getValue
// Description: Gets the value of the spin box.
//
// Syntax: function SpinBox.getValue() : Integer
//
// Remarks: Returns the current value of the spin box.
//
// Returns: Returns the current value of the spin box.

void SpinBox::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    int val = m_ctrl->GetValue();
    retval->setInteger(val);
}

// (METHOD) SpinBox.setRange
// Description: Sets the minimum and maximum range of the spin box.
//
// Syntax: function SpinBox.setRange(minimum : Integer,
//                                   maximum : Integer)
//
// Remarks: Sets the |minimum| and |maximum| range of the spin box,
//     limiting the value the spin box can have to this range.
//
// Param(minimum): The |minimum| value the spin box value can have.
// Param(maximum): The |maximum| value the spin box value can have.

void SpinBox::setRange(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 2)
        return;
        
    int min = env->getParam(0)->getInteger();
    int max = env->getParam(1)->getInteger();
    m_ctrl->SetRange(min, max);
}

// (METHOD) SpinBox.getMin
// Description: Gets the minimum value the spin box can have.
//
// Syntax: function SpinBox.getMin() : Integer
//
// Remarks: Returns the minimum value the spin box can have.
//
// Returns: Returns the minimum value the spin box can have.

void SpinBox::getMin(kscript::ExprEnv* env, kscript::Value* retval)
{
    int min = m_ctrl->GetMin();
    retval->setInteger(min);
}

// (METHOD) SpinBox.getMax
// Description: Gets the maximum value the spin box can have.
//
// Syntax: function SpinBox.getMax() : Integer
//
// Remarks: Returns the maximum value the spin box can have.
//
// Returns: Returns the maximum value the spin box can have.

void SpinBox::getMax(kscript::ExprEnv* env, kscript::Value* retval)
{
    int max = m_ctrl->GetMax();
    retval->setInteger(max);
}




// (EVENT) SpinBox.changed
// Description: Fired when the spin button's value is changed.
// Param(value): The new value of the spin box.

// (EVENT) SpinBox.spinUp
// Description: Fired when the menu item is clicked.
// Param(value): The new value of the spin box.

// (EVENT) SpinBox.spinDown
// Description: Fired when the menu item is clicked.
// Param(value): The new value of the spin box.

void SpinBox::onEvent(wxEvent& event)
{
    wxCommandEvent& evt = (wxCommandEvent&)event;
    
    kscript::Value* event_args = new kscript::Value;
    event_args->setObject();

    if (event.GetEventType() == wxEVT_SCROLL_THUMBTRACK)
    {
        int val = evt.GetInt();
        event_args->getMember(L"value")->setInteger(val);
        invokeJsEvent(L"changed", event_args);
    }
     else if (event.GetEventType() == wxEVT_SCROLL_LINEUP)
    {
        int val = evt.GetInt();
        event_args->getMember(L"value")->setInteger(val);
        invokeJsEvent(L"spinUp", event_args);
    }
     else if (event.GetEventType() == wxEVT_SCROLL_LINEDOWN)
    {
        int val = evt.GetInt();
        event_args->getMember(L"value")->setInteger(val);
        invokeJsEvent(L"spinDown", event_args);
    }

    if (event.GetEventType() == wxEVT_COMMAND_TEXT_UPDATED)
    {
        std::wstring text = towstr(evt.GetString());
        event_args->getMember(L"text")->setString(text);
        invokeJsEvent(L"textChanged", event_args);
    }
    
    if (event.GetEventType() == wxEVT_COMMAND_TEXT_ENTER)
    {
        std::wstring text = towstr(evt.GetString());
        event_args->getMember(L"text")->setString(text);
        invokeJsEvent(L"enterPressed", event_args);
    }
}


