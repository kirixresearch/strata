/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-08-03
 *
 */


#include "appmain.h"
#include "scriptgui.h"
#include "scriptslider.h"


// -- Slider class implementation --

// (CLASS) Slider
// Category: Control
// Derives: FormControl
// Description: A class that represents a slider control.
// Remarks: The Slider class represents a slider control.
//
// Property(Slider.Horizontal):   A flag representing that a horizontal slider should be created.
// Property(Slider.Vertical):     A flag representing that a vertical slider should be created.

// (EVENT) Slider.scroll
// Description: Fired when the slider moves.
// Param(value): The new slider position.

// (EVENT) Slider.scrollTop
// Description: Fired when the slider moves to the minimum value
//     of its range.
// Param(value): The new slider position.

// (EVENT) Slider.scrollBottom
// Description: Fired when the slider moves to the maximum value
//     of its range.

// (EVENT) Slider.scrollUp
// Description: Fired when the slider moves a single interval
//     towards its minimum value.
// Param(value): The new slider position.

// (EVENT) Slider.scrollDown
// Description: Fired when the slider moves a single interval
//     towards its maximum value.
// Param(value: The new slider position.

// (EVENT) Slider.scrollPageUp
// Description: Fired when the slider moves a page interval
//     towards its minimum value.
// Param(value): The new slider position.

// (EVENT) Slider.scrollPageDown
// Description: Fired when the slider moves a page interval
//     towards its maximum value.
// Param(value): The new slider position.

// (EVENT) Slider.track
// Description: Fired when the slider is being moved
//     by dragging it with the mouse
// Param(value): The new slider position.

// (EVENT) Slider.trackRelease
// Description: Fired when the slider is released
//     from being dragged by the mouse
// Param(value): The new slider position.


Slider::Slider()
{
    m_ctrl = NULL;
}

Slider::~Slider()
{

}

// (CONSTRUCTOR) Slider.constructor
// Description: Creates a new Slider.
//
// Syntax: Slider(orientation : Integer,
//                x_pos : Integer,
//                y_pos : Integer,
//                width : Integer,
//                height : Integer)
//
// Remarks: Creates a new Slider object with the specified |orientation|
//     at the position specified by |x_pos| and |y_pos|, and having
//     dimensions specified by |width| and |height|.  If |orientation| is
//     unspecified or is Slider.Horizontal, a horizontal Slider object will
//     be created.  If |orientation| is Slider.Vertical, a vertical Slider
//     object will be created.
//
// Param(orientation): Determines the manner in which to
//     create the Slider object, depending on whether it's
//     Slider.Horizontal or Slider.Vertical.
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void Slider::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- initialize the component --
    initComponent(env);

    // -- add some properties --
    kscript::Value obj;
    
    getMember(L"scroll")->setObject(Event::createObject(env));
    getMember(L"scrollTop")->setObject(Event::createObject(env));
    getMember(L"scrollBottom")->setObject(Event::createObject(env));
    getMember(L"scrollUp")->setObject(Event::createObject(env));
    getMember(L"scrollDown")->setObject(Event::createObject(env));
    getMember(L"scrollPageUp")->setObject(Event::createObject(env));
    getMember(L"scrollPageDown")->setObject(Event::createObject(env));
    getMember(L"track")->setObject(Event::createObject(env));
    getMember(L"trackRelease")->setObject(Event::createObject(env));
 
    // -- default style --
    int orientation = Slider::Horizontal;
    
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

    if (param_count < 2)
    {
        // -- create the control --
        m_ctrl = new wxSlider(getApp()->getTempParent(),
                              -1,
                              50,
                              0,
                              100,
                              wxDefaultPosition,
                              wxDefaultSize,
                                (orientation == Slider::Vertical)
                                    ? wxSL_VERTICAL
                                    : wxSL_HORIZONTAL);
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
        m_ctrl = new wxSlider(getApp()->getTempParent(),
                              -1,
                              50,
                              0,
                              100,
                              wxPoint(m_x, m_y),
                              wxSize(m_width, m_height),
                                (orientation == Slider::Vertical)
                                    ? wxSL_VERTICAL
                                    : wxSL_HORIZONTAL);
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

void Slider::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (!m_enabled)
        m_ctrl->Enable(false);

    listenEvent(wxEVT_SCROLL_TOP);
    listenEvent(wxEVT_SCROLL_BOTTOM);
    listenEvent(wxEVT_SCROLL_LINEUP);
    listenEvent(wxEVT_SCROLL_LINEDOWN);
    listenEvent(wxEVT_SCROLL_PAGEUP);
    listenEvent(wxEVT_SCROLL_PAGEDOWN);
    listenEvent(wxEVT_SCROLL_THUMBTRACK);
    listenEvent(wxEVT_SCROLL_THUMBRELEASE);
}

// (METHOD) Slider.setValue
// Description: Sets the position of the slider.
//
// Syntax: function Slider.setValue(position : Integer)
//
// Remarks: Sets the |position| of the slider.
//
// Param(position): The |position| to which to set the slider.

void Slider::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    int val = env->getParam(0)->getInteger();
    m_ctrl->SetValue(val);
}

// (METHOD) Slider.getValue
// Description: Gets the position of the slider.
//
// Syntax: function Slider.getValue() : Integer
//
// Remarks: Returns the position of the slider.
//
// Returns: Returns the position of the slider.

void Slider::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int val = m_ctrl->GetValue();
    retval->setInteger(val);
}

// (METHOD) Slider.setRange
// Description: Sets the minimum and maximum value the slider can have.
//
// Syntax: function Slider.setRange(minimum : Integer,
//                                  maximum : Integer)
//
// Remarks: Sets the |minimum| and |maximum| value the slider can have.
//
// Param(minimum): The |minimum| value the slider can have.
// Param(maximum): The |maximum| value the slider can have.

void Slider::setRange(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 2 || !isControlValid())
        return;

    int min = env->getParam(0)->getInteger();
    int max = env->getParam(1)->getInteger();
    m_ctrl->SetRange(min, max);
}

// (METHOD) Slider.getMin
// Description: Gets the minimum value of the slider.
//
// Syntax: function Slider.getMin() : Integer
//
// Remarks: Returns the minimum value the slider can have.
//
// Returns: Returns the minimum value the slider can have.

void Slider::getMin(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int val = m_ctrl->GetMin();
    retval->setInteger(val);
}

// (METHOD) Slider.getMax
// Description: Gets the maximum value of the slider.
//
// Syntax: function Slider.getMax() : Integer
//
// Remarks: Returns the maximum value the slider can have.
//
// Returns: Returns the maximum value the slider can have.

void Slider::getMax(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int val = m_ctrl->GetMax();
    retval->setInteger(val);
}

// (METHOD) Slider.setPageSize
// Description: Sets the number of steps the slider moves on page up or page down.
//
// Syntax: function Slider.setPageSize(increment : Integer)
//
// Remarks: Sets the number of steps the slider moves on page up or page down.
//
// Param(increment): The number of steps the slider moves on a page up or page down.

void Slider::setPageSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    int val = env->getParam(0)->getInteger();
    m_ctrl->SetPageSize(val);
}

// (METHOD) Slider.getPageSize
// Description: Gets the number of steps the slider moves on page up or page down.
//
// Syntax: function Slider.getPageSize() : Integer
//
// Remarks: Returns the number of steps the slider moves on page up or page down.
//
// Returns: Returns the number of steps the slider moves on page up or page down.

void Slider::getPageSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int val = m_ctrl->GetPageSize();
    retval->setInteger(val);
}

// (METHOD) Slider.setLineSize
// Description: Sets the number of steps the slider moves when the slider 
//     bar is moved.
//
// Syntax: function Slider.setLineSize(steps : Integer)
//
// Remarks: Sets the number of steps the slider moves when the slider 
//     bar is moved.
//
// Param(steps): The number of |steps| the slider moves when the slider
//     bar is moved.

void Slider::setLineSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    int val = env->getParam(0)->getInteger();
    m_ctrl->SetLineSize(val);
}

// (METHOD) Slider.getLineSize
// Description: Gets the number of steps the slider moves when the slider
//     bar is moved.
//
// Syntax: function Slider.getLineSize() : Integer
//
// Remarks: Returns the number of steps the slider moves when the slider 
//     bar is moved.
//
// Returns: Returns the number of steps the slider moves when the slider 
//     bar is moved.

void Slider::getLineSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    int val = m_ctrl->GetLineSize();
    retval->setInteger(val);
}

void Slider::onEvent(wxEvent& event)
{
    wxCommandEvent& evt = (wxCommandEvent&)event;
    int val = evt.GetInt();
    
    kscript::Value* event_args = new kscript::Value;
    event_args->setObject();
    event_args->getMember(L"value")->setInteger(val);

    kscript::Value* event_args2 = new kscript::Value;
    event_args2->setObject();
    event_args2->getMember(L"value")->setInteger(val);

    wxEventType event_type = evt.GetEventType();
    
    if (event_type == wxEVT_SCROLL_TOP ||
        event_type == wxEVT_SCROLL_BOTTOM ||
        event_type == wxEVT_SCROLL_LINEUP ||
        event_type == wxEVT_SCROLL_LINEDOWN ||
        event_type == wxEVT_SCROLL_PAGEUP ||
        event_type == wxEVT_SCROLL_PAGEDOWN ||
        event_type == wxEVT_SCROLL_THUMBTRACK ||
        event_type == wxEVT_SCROLL_THUMBRELEASE)
    {
        if (event_type == wxEVT_SCROLL_TOP)
            invokeJsEvent(L"scrollTop", event_args);
             else if (event_type == wxEVT_SCROLL_BOTTOM)
            invokeJsEvent(L"scrollBottom", event_args);
             else if (event_type == wxEVT_SCROLL_LINEUP)
            invokeJsEvent(L"scrollUp", event_args);
             else if (event_type == wxEVT_SCROLL_LINEDOWN)
            invokeJsEvent(L"scrollDown", event_args);
             else if (event_type == wxEVT_SCROLL_PAGEUP)
            invokeJsEvent(L"scrollPageUp", event_args);
             else if (event_type == wxEVT_SCROLL_PAGEDOWN)
            invokeJsEvent(L"scrollPageDown", event_args);
             else if (event_type == wxEVT_SCROLL_THUMBTRACK)
            invokeJsEvent(L"track", event_args);
             else if (event_type == wxEVT_SCROLL_THUMBRELEASE)
            invokeJsEvent(L"trackRelease", event_args);
        
        invokeJsEvent(L"scroll", event_args2);
    }
}

