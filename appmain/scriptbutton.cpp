/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2006-12-07
 *
 */


#include "appmain.h"
#include "scriptapp.h"
#include "scriptgui.h"
#include "scriptbitmap.h"
#include "scriptbutton.h"


// (CLASS) Button
// Category: Control
// Derives: FormControl
// Description: A class that represents a button control.
// Remarks: The Button class represents a button control which can
//     be added to a window.  It can be clicked by the user using the mouse,
//     or activated by the keyboard if the control has keyboard focus.  When
//     the button is pushed, the |click| event is fired.

Button::Button()
{
    m_ctrl = NULL;
}

Button::~Button()
{

}

// (CONSTRUCTOR) Button.constructor
// Description: Creates a new Button object.
//
// Syntax: Button(text : String,
//                x_pos : Integer,
//                y_pos : Integer,
//                width : Integer,
//                height : Integer)
//
// Remarks: Creates a new Button with the given |text| at the 
//     position specified by |x_pos| and |y_pos|, and having 
//     dimensions specified by |width| and |height|.
//
// Param(text): The |text| to be shown on the control.
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void Button::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // add some properties
    getMember(L"click")->setObject(Event::createObject(env));

    // set default values
    wxString label = wxEmptyString;
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
    if (param_count > 0)
        label = env->getParam(0)->getString();
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
        // create the control
        m_ctrl = new wxButton(getApp()->getTempParent(),
                              -1,
                              label,
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
        // create the control
        m_ctrl = new wxButton(getApp()->getTempParent(),
                              -1,
                              label,
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

void Button::realize()
{
    m_ctrl->Reparent(m_form_wnd);

    if (!m_enabled)
        m_ctrl->Enable(false);

    listenEvent(wxEVT_COMMAND_BUTTON_CLICKED);
}


// (METHOD) Button.setLabel
// Description: Sets the label text on the button.
//
// Syntax: function Button.setLabel(text : String)
//
// Remarks: Sets the button's label text to the string specified
//     in the |text| parameter.
//
// Param(text): The new label text

void Button::setLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0 && isControlValid())
    {
        wxString label = env->getParam(0)->getString();
        m_ctrl->SetLabel(label);
    }
}

// (METHOD) Button.getLabel
// Description: Returns the button's current label text.
//
// Syntax: function Button.getLabel() : String
//
// Remarks: Returns the button's current label text.
//
// Returns: The button's current label text.

void Button::getLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    std::wstring label = towstr(m_ctrl->GetLabel());
    retval->setString(label);
}

// (METHOD) Button.setDefault
// Description: Sets whether the button is a form's 'default' button
//
// Syntax: function Button.setDefault()
//
// Remarks: Sets a button as a form's 'default' button.  A default
//     button is activated whenever the enter key is pressed when a form
//     is focused.

void Button::setDefault(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        retval->setNull();
        m_ctrl->SetDefault();
    }
}

// (EVENT) Button.click
// Description: Fired when the button is clicked.

void Button::onEvent(wxEvent& event)
{
    invokeJsEvent(L"click");
}




// (CLASS) BitmapButton
// Category: Control
// Derives: FormControl
// Description: A class that represents a bitmap button control.
// Remarks: The BitmapButton class represents a button control which can
//     be added a window.  It can be clicked by the user using the mouse,
//     or activated by the keyboard if the control has keyboard focus.
//     This type of button can only display a bitmap without any label.
//     When the button is pushed, the |click| event is fired.

BitmapButton::BitmapButton()
{
    m_ctrl = NULL;
}

BitmapButton::~BitmapButton()
{

}

// (CONSTRUCTOR) BitmapButton.constructor
// Description: Creates a new BitmapButton object.
//
// Syntax: BitmapButton(bitmap : Bitmap,
//                      x_pos : Integer,
//                      y_pos : Integer,
//                      width : Integer,
//                      height : Integer)
//
// Remarks: Creates a new BitmapButton with the given |bitmap|
//     at the position specified by |x_pos| and |y_pos|, and
//     having dimensions specified by |width| and |height|.
//
// Param(bitmap): The |bitmap| to be shown on the control.
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void BitmapButton::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);
    
    // add some properties
    getMember(L"click")->setObject(Event::createObject(env));
    
    // set default values
    wxBitmap bitmap;
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
    if (param_count > 0)
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->isKindOf(L"Bitmap"))
        {
            Bitmap* b = (Bitmap*)obj;
            bitmap = b->getWxBitmap();
        }
    }
    if (param_count > 1)
        m_x = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_y = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_width = env->getParam(3)->getInteger();
    if (param_count > 4)
        m_height = env->getParam(4)->getInteger();
    
    // eliminate ASSERT failures
    if (!bitmap.IsOk())
        bitmap = GETBMP(gf_blank_16);
    
    if (param_count < 2)
    {
        // create the control
        m_ctrl = new wxBitmapButton(getApp()->getTempParent(),
                                    -1,
                                    bitmap,
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
        // create the control
        m_ctrl = new wxBitmapButton(getApp()->getTempParent(),
                                    -1,
                                    bitmap,
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

void BitmapButton::realize()
{
    m_ctrl->Reparent(m_form_wnd);

    if (!m_enabled)
        m_ctrl->Enable(false);

    listenEvent(wxEVT_COMMAND_BUTTON_CLICKED);
}


// (METHOD) BitmapButton.setBitmap
// Description: Sets the label text on the button.
//
// Syntax: function BitmapButton.setBitmap(bitmap : Bitmap)
//
// Remarks: Sets the button's bitmap to the bitmap specified
//     in the |bitmap| parameter.
//
// Param(bitmap): The new bitmap

void BitmapButton::setBitmap(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0 && isControlValid())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->isKindOf(L"Bitmap"))
        {
            Bitmap* b = (Bitmap*)obj;
            m_ctrl->SetBitmapLabel(b->getWxBitmap());
        }
    }
}

// (METHOD) BitmapButton.getBitmap
// Description: Returns the button's current bitmap.
//
// Syntax: function BitmapButton.getBitmap() : Bitmap
//
// Remarks: Returns the button's current bitmap.
//
// Returns: The button's current bitmap.

void BitmapButton::getBitmap(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    Bitmap* bitmap_obj = Bitmap::createObject(env);
    bitmap_obj->setWxBitmap(m_ctrl->GetBitmapLabel());
    retval->setObject(bitmap_obj);
}

// (METHOD) BitmapButton.setDefault
// Description: Sets whether the button is a form's 'default' button
//
// Syntax: function BitmapButton.setDefault()
//
// Remarks: Sets a button as a form's 'default' button.  A default
//     button is activated whenever the enter key is pressed when a form
//     is focused.

void BitmapButton::setDefault(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        retval->setNull();
        m_ctrl->SetDefault();
    }
}

// (EVENT) BitmapButton.click
// Description: Fired when the button is clicked.

void BitmapButton::onEvent(wxEvent& event)
{
    invokeJsEvent(L"click");
}




// RadioButton class implementation

// (CLASS) RadioButton
// Category: Control
// Derives: FormControl
// Description: A class that represents a radio button control.
// Remarks: The RadioButton class represents a radio button control.

RadioButton::RadioButton() 
{
    m_id = -1;
    //m_owner = NULL;
    m_ctrl = NULL;
}

RadioButton::~RadioButton()
{
}

int RadioButton::getId() const
{
    return m_id;
}

wxRadioButton* RadioButton::getWxRadioButton()
{
    return m_ctrl;
}

/*
void RadioButton::setOwner(RadioButtonGroup* owner)
{
    m_owner = owner;
}
*/

// (CONSTRUCTOR) RadioButton.constructor
// Description: Creates a new RadioButton.
//
// Syntax: RadioButton(text : String,
//                     x_pos : Integer,
//                     y_pos : Integer,
//                     width : Integer,
//                     height : Integer)
//
// Remarks: Creates a new RadioButton with the given |text| at the 
//     position specified by |x_pos| and |y_pos|, and having dimensions 
//     specified by |width| and |height|.
//
// Param(text): The |text| of the radio button.
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void RadioButton::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // add some properties 
    getMember(L"click")->setObject(Event::createObject(env));

    // set the ID for this control
    m_id = getUniqueScriptControlId();
    
    // set default values
    wxString label = wxEmptyString;
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();

    // get user input values
    if (param_count > 0)
        label = env->getParam(0)->getString();
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
        // create the control
        m_ctrl = new wxRadioButton(getApp()->getTempParent(),
                                   -1,
                                   label,
                                   wxDefaultPosition,
                                   wxDefaultSize,
                                   wxRB_SINGLE);
        m_wnd = m_ctrl;

        // if no sizing parameters were specified, used wx's default
        // parameters, since they are equal to the exact size of the text
        wxSize s = m_ctrl->GetSize();
        m_width = s.GetWidth();
        m_height = s.GetHeight();
    }
     else
    {
        // create the control
        m_ctrl = new wxRadioButton(getApp()->getTempParent(),
                                   -1,
                                   label,
                                   wxPoint(m_x, m_y),
                                   wxSize(m_width, m_height),
                                   wxRB_SINGLE);
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

void RadioButton::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (!m_enabled)
        m_ctrl->Enable(false);
    
    listenEvent(wxEVT_COMMAND_RADIOBUTTON_SELECTED);
}

// (METHOD) RadioButton.setValue
// Description: Selects or deselects the radio button.
//
// Syntax: function RadioButton.setValue(flag : Boolean)
//
// Remarks: Selects the radio button if |flag| is true and
//     deselects the radio button if |flag| is false.
//
// Param(flag): Value indicating whether to select or deselect
//     the radio button.

// TODO: other setValue/getValue() is usually used for non-boolean
//     values; make sure the API follows consistent rules for
//     boolean values

void RadioButton::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        m_ctrl->SetValue(env->getParam(0)->getBoolean());
    }
}

// (METHOD) RadioButton.getValue
// Description: Gets whether or not the radio button is selected.
//
// Syntax: function RadioButton.getValue() : Boolean
//
// Remarks: Returns true if the radio button is selected and
//     false if the radio button is deselected.
//
// Returns: Returns true if the radio button is selected and
//     false if the radio button is deselected.

void RadioButton::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setBoolean(m_ctrl->GetValue());
}

// (EVENT) RadioButton.click
// Description: Fired when the radio button is clicked.

void RadioButton::onEvent(wxEvent& event)
{
    invokeJsEvent(L"click");

    /*
    if (m_owner)
        m_owner->handleSelection(m_id);
    */
}




/*
// RadioButtonGroup class implementation

// ( CLASS ) RadioButtonGroup
// Category: Control
// Description: A class that represents a group of radio buttons.
// Remarks: The RadioButtonGroup class represents a group of radio buttons.

RadioButtonGroup::RadioButtonGroup()
{
    m_orientation = Layout::Horizontal;
}

RadioButtonGroup::~RadioButtonGroup()
{

}

// ( CONSTRUCTOR ) RadioButtonGroup.constructor
// Description: Creates a new RadioButtonGroup object.
//
// Syntax: RadioButtonGroup(orientation : Integer)
//
// Remarks: Creates a new RadioButtonGroup object. If |orientation|
//     has the value Layout.Horizontal, then a horizontal radio
//     button group will be created; if |orientation| has the value
//     Layout.Vertical, then a vertical radio button group will be
//     created.  If |orientation| isn't specified, a horizontal
//     radio button group will be created.
//
// Param(orientation): Specifies whether a horizontal or vertical
//     radio button group will be created.

// TODO: most other control constructors have an (x,y) position
// and a (width,height) size.  It's not clear here how to
// set the position and size of the group in which the elements
// are layed out.

void RadioButtonGroup::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // set default values
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();

    // get user input values
    if (param_count > 0)
        m_orientation = env->getParam(0)->getInteger();
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
        // create the control
        m_ctrl = new wxPanel(getApp()->getTempParent(),
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
        // create the control
        m_ctrl = new wxPanel(getApp()->getTempParent(),
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
    
    // if the user used an incorrect value for the orientation flag,
    // go back to the default setting (horizontal)
    if (m_orientation != Layout::Horizontal &&
        m_orientation != Layout::Vertical)
    {
        m_orientation = Layout::Horizontal;
    }
}

void RadioButtonGroup::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (m_orientation == Layout::Vertical)
        m_sizer = new wxBoxSizer(wxVERTICAL);
     else
        m_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    std::vector<RadioButton*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        if (it != m_items.begin())
            m_sizer->AddSpacer(8);
        
        // realize the radio button
        RadioButton* rb = (*it);
        rb->realize();
        
        // add the radio button to the sizer
        wxRadioButton* wx_rb = rb->getWxRadioButton();
        wx_rb->Reparent(m_ctrl);
        m_sizer->Add(wx_rb);
    }
    
    m_ctrl->SetSizer(m_sizer);
    m_ctrl->Layout();
    
    if (!m_enabled)
        m_ctrl->Enable(false);
    
    // update our size
    wxSize s = m_sizer->GetMinSize();
    wxSize s2 = m_ctrl->GetSize();
    m_ctrl->SetSize(s);
    m_width = s.GetWidth();
    m_height = s.GetHeight();
}

// ( METHOD ) RadioButtonGroup.add
// Description: Adds a RadioButton object to the RadioButtonGroup.
//
// Syntax: function RadioButtonGroup.add(item : RadioButton)
//
// Remarks: Adds the RadioButton |item| to the RadioButtonGroup.
//
// Param(item): The RadioButton |item| to add to the RadioButtonGroup.

void RadioButtonGroup::add(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->isKindOf(L"RadioButton"))
    {
        RadioButton* rb = (RadioButton*)obj;
        rb->setOwner(this);
        m_items.push_back(rb);
    }
}

void RadioButtonGroup::handleSelection(int id)
{
    // this function makes sure that all other radio buttons
    // are deselected when a radio button is selected
    std::vector<RadioButton*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        RadioButton* rb = (*it);
        if (rb->getId() != id)
            rb->getWxRadioButton()->SetValue(false);
    }
}
*/




// CheckBox class implementation

// (CLASS) CheckBox
// Category: Control
// Derives: FormControl
// Description: A class that represents a check box control.
// Remarks: The CheckBox class represents a check box control.

CheckBox::CheckBox()
{
    m_ctrl = NULL;
}

CheckBox::~CheckBox()
{

}

// (CONSTRUCTOR) CheckBox.constructor
// Description: Creates a new CheckBox object.
//
// Syntax: CheckBox(text : String,
//                  x_pos : Integer,
//                  y_pos : Integer,
//                  width : Integer,
//                  height : Integer)
//
// Remarks: Creates a new CheckBox object.
//
// Param(text): The |text| to be shown next to the control.
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The |width| of the control.
// Param(height): The |height| of the control.

void CheckBox::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // add some properties
    getMember(L"click")->setObject(Event::createObject(env));

    // set default values
    wxString label = wxEmptyString;
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
    if (param_count > 0)
        label = env->getParam(0)->getString();
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
        // create the control
        m_ctrl = new wxCheckBox(getApp()->getTempParent(),
                                -1,
                                label,
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
        // create the control
        m_ctrl = new wxCheckBox(getApp()->getTempParent(),
                                -1,
                                label,
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

void CheckBox::realize()
{
    m_ctrl->Reparent(m_form_wnd);

    if (!m_enabled)
    {
        m_ctrl->Enable(false);
    }

    listenEvent(wxEVT_COMMAND_CHECKBOX_CLICKED);
}

// (METHOD) CheckBox.setLabel
// Description: Sets the text associated with the check box control.
//
// Syntax: function CheckBox.setLabel(text : String)
//
// Remarks: Sets the |text| associated with the check box control.
//
// Param(text): The |text| to draw next to the control.

void CheckBox::setLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    wxString label = env->getParam(0)->getString();
    m_ctrl->SetLabel(label);
}

// (METHOD) CheckBox.getLabel
// Description: Gets the text associated with the check box control.
//
// Syntax: function CheckBox.getLabel() : String
//
// Remarks: Returns the text associated with the check box control.
//
// Returns: Returns the text associated with the check box control.

void CheckBox::getLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    std::wstring label = towstr(m_ctrl->GetLabel());
    retval->setString(label);
}

// (METHOD) CheckBox.setValue
// Description: Turns the check in the check box on or off.
//
// Syntax: function CheckBox.setValue(flag : Boolean)
//
// Remarks: Turns the check in the check box on if |flag|
//     is true. Turns the check in the check box off if
//     |flag| is false.
//
// Param(flag): The state to which to set the check in the check box.
//     If |flag| is true, the check box will show the check, and
//     if |flag| is false, the check box will not show the check.

void CheckBox::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0 && isControlValid())
    {
        bool checked = env->getParam(0)->getBoolean();
        m_ctrl->SetValue(checked);
    }
}

// (METHOD) CheckBox.getValue
// Description: Gets the visible state of the check in the check box control.
//
// Syntax: function CheckBox.getValue() : Boolean
//
// Remarks: Returns true if the check in the check box control is visible.
//     Returns false if the check in the check box control is not visible.
//
// Returns: Returns true if the check in the check box control is visible,
//     and false otherwise.

void CheckBox::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setBoolean(m_ctrl->GetValue());
}

// (EVENT) CheckBox.click
// Description: Fired when the check box is clicked.

void CheckBox::onEvent(wxEvent& event)
{
    if (event.GetEventType() == wxEVT_COMMAND_CHECKBOX_CLICKED)
    {
        invokeJsEvent(L"click");
    }
}

