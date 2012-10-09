/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams; Benjamin I. Williams
 * Created:  2006-12-08
 *
 */


#include "appmain.h"
#include "appcontroller.h"
#include "scriptapp.h"
#include "scriptgui.h"
#include "scriptcombo.h"


// ComboBox class implementation

// (CLASS) ComboBox
// Category: Control
// Derives: FormControl
// Description: A class that represents a combo box control.
// Remarks: The ComboBox class represents a combo box control.

ComboBox::ComboBox()
{
    m_ctrl = NULL;
}

ComboBox::~ComboBox()
{
}

// (CONSTRUCTOR) ComboBox.constructor
// Description: Creates a new ComboBox.
//
// Syntax: ComboBox(x_pos : Integer,
//                  y_pos : Integer,
//                  width : Integer,
//                  height : Integer)
//
// Remarks: Creates a new ComboBox at the position specified by 
//     |x_pos| and |y_pos|, and having dimensions specified by |width| 
//     and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void ComboBox::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // add some properties
    kscript::Value obj;
    
    getMember(L"selectionChanged")->setObject(Event::createObject(env));
    getMember(L"textChanged")->setObject(Event::createObject(env));
    getMember(L"enterPressed")->setObject(Event::createObject(env));

    // set default values
    wxString text = wxEmptyString;
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
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
        // create the control
        m_ctrl = new wxComboBox(getApp()->getTempParent(),
                                -1,
                                wxEmptyString,
                                wxDefaultPosition,
                                wxDefaultSize,
                                0,
                                NULL,
                                wxCB_DROPDOWN);
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
        m_ctrl = new wxComboBox(getApp()->getTempParent(),
                                -1,
                                wxEmptyString,
                                wxPoint(m_x, m_y),
                                wxSize(m_width, m_height),
                                0,
                                NULL,
                                wxCB_DROPDOWN);
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

void ComboBox::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (!m_enabled)
        m_ctrl->Enable(false);
    
    listenEvent(wxEVT_COMMAND_COMBOBOX_SELECTED);
    listenEvent(wxEVT_COMMAND_TEXT_UPDATED);
    listenEvent(wxEVT_COMMAND_TEXT_ENTER);
}

// (METHOD) ComboBox.addItem
// Description: Adds a text item to the combo box.
//
// Syntax: function ComboBox.addItem(text : String) : Integer
//
// Remarks: Adds an item, specified by |text|, to the list
//     of items in the combo box.
//
// Param(text): The |text| to add to the list of combo box items.
// Returns: Returns the index of the item that was added.

// ----------
// ( METHOD ) ComboBox.add
// DEPRECATED: Use ComboBox.addItem() instead of ComboBox.add()

void ComboBox::addItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    wxString s = towx(env->getParam(0)->getString());
    int idx = m_ctrl->Append(s);
    retval->setInteger(idx);
}

// (METHOD) ComboBox.clear
// Description: Clears the combo box of all items.
//
// Syntax: function ComboBox.clear()
//
// Remarks: Clears the combo box of all items.

void ComboBox::clear(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->Clear();
}

// (METHOD) ComboBox.deleteItem
// Description: Deletes an item with a specified index from the combo box.
//
// Syntax: function ComboBox.deleteItem(index : Integer)
//
// Remarks: Deletes an item with a specified |index| from the combo box.
//
// Param(index): The |index| of the item to delete from the list of 
//     items in the combo box.

// ----------
// ( METHOD ) ComboBox.delete
// DEPRECATED: Use ComboBox.deleteItem() instead of ComboBox.delete()

void ComboBox::deleteItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    int idx = env->getParam(0)->getInteger();
    m_ctrl->Delete(idx);
}

// (METHOD) ComboBox.findItem
// Description: Finds the index of a string in the combo box.
//
// Syntax: function ComboBox.findItem(text : String) : Integer
//
// Remarks: Finds the |text| item in the combo box and returns
//     it's index if it's found and -1 if it isn't found.
//
// Param(text): The |text| to find in the combo box.
// Returns: Returns the index of the |text| that's found in the
//     combo box and -1 if the text isn't found.

// ----------
// ( METHOD ) ComboBox.findString
// DEPRECATED: Use ComboBox.findItem() instead of ComboBox.findString()

void ComboBox::findItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    wxString s = towx(env->getParam(0)->getString());
    int idx = m_ctrl->FindString(s);
    retval->setInteger(idx);
}

// (METHOD) ComboBox.getItemCount
// Description: Gets the number of items in the combo box.
//
// Syntax: function ComboBox.getItemCount() : Integer
//
// Remarks: Returns the number of items in the combo box.
//
// Returns: Returns the number of items in the combo box.

// ----------
// ( METHOD ) ComboBox.getCount
// DEPRECATED: Use ComboBox.getItemCount() instead of ComboBox.getCount()

void ComboBox::getItemCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setInteger(m_ctrl->GetCount());
}

// (METHOD) ComboBox.getSelectedIndex
// Description: Gets the index of the selected item in the combo box.
//
// Syntax: function ComboBox.getSelectedIndex() : Integer
//
// Remarks: Returns the index of the selected item in the combo box.
//
// Returns: Returns the index of the selected item in the combo box.

// ----------
// ( METHOD ) ComboBox.getSelection
// DEPRECATED: Use ComboBox.getSelectedIndex() instead of ComboBox.getSelection()

void ComboBox::getSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setInteger(m_ctrl->GetSelection());
}

// (METHOD) ComboBox.getItem
// Description:  Gets an item from the combo box based on the
//     specified index.
//
// Syntax: function ComboBox.getItem(index : Integer) : String
//
// Remarks: Returns the text item at the specified |index|.  If |index|
//     isn't specified, then the currently selected string is returned,
//     which may include the item that's currently being entered in
//     the combo box before it actually becomes part of the list.
//
// Param(index): The |index| of the item in the combo box to return.
// Returns: The text item at the specified |index|.

// ----------
// ( METHOD ) ComboBox.getString
// DEPRECATED: Use ComboBox.getItem() instead of ComboBox.getString()

void ComboBox::getItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    if (env->getParamCount() == 0)
    {
        retval->setString(towstr(m_ctrl->GetValue()));
    }
     else
    {
        int idx = env->getParam(0)->getInteger();
        retval->setString(towstr(m_ctrl->GetString(idx)));
    }
}

// (METHOD) ComboBox.getValue
// Description:  Gets the current text in the combo box.
//
// Syntax: function ComboBox.getValue() : String
//
// Remarks: Returns the current text in the combo box.

void ComboBox::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    retval->setString(towstr(m_ctrl->GetValue()));
}

// (METHOD) ComboBox.insertItem
// Description: Inserts a text item at a specified index.
//
// Syntax: function ComboBox.insertItem(text : String,
//                                      index : Integer)
//
// Remarks: Inserts a |text| item at a specified |index|.
//
// Param(text): The |text| item to add to the combo box.
// Param(index): The |index| at which to insert the |text|
//     in the combo box.

// ----------
// ( METHOD ) ComboBox.insertItem
// DEPRECATED: Use ComboBox.insertItem() instead of ComboBox.insert()

void ComboBox::insertItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 2 || !isControlValid())
        return;

    wxString s = towx(env->getParam(0)->getString());
    int idx = env->getParam(1)->getInteger();
    m_ctrl->Insert(s, idx);
}

// (METHOD) ComboBox.selectItem
// Description: Selects a combo box item at the specified index.
//
// Syntax: function ComboBox.selectItem(index : Integer)
//
// Remarks: Selects a combo box item at the specified |index|.
//
// Param(index): The |index| of the item to select in the combo box.

// ----------
// ( METHOD ) ComboBox.select
// DEPRECATED: Use ComboBox.selectItem() instead of ComboBox.select()

void ComboBox::selectItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    int idx = env->getParam(0)->getInteger();
    m_ctrl->Select(idx);
}

// (METHOD) ComboBox.setItem
// Description: Sets the text of a combo box item at the specified index.
//
// Syntax: function ComboBox.setItem(index : Integer,
//                                   text : String)
//
// Remarks: Sets the new |text| of a combo box item at a specified |index|.
//
// Param(index): The |index| of the item in the combo box for which to
//     change the |text|.
// Param(text): The new |text| for the item at the specified |index|.

// ----------
// ( METHOD ) ComboBox.setString
// DEPRECATED: Use ComboBox.setItem() instead of ComboBox.setString()

void ComboBox::setItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 2 || !isControlValid())
        return;

    int idx = env->getParam(0)->getInteger();
    wxString s = towx(env->getParam(1)->getString());

    m_ctrl->SetString(idx, s);
}

// (METHOD) ComboBox.setValue
// Description:  Sets the current text in the combo box.
//
// Syntax: function ComboBox.setValue(text : String)
//
// Remarks: Sets the current text in the combo box.
//
// Param(text): The |text| to set in the combo box.

void ComboBox::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
    
    wxString s = towx(env->getParam(0)->getString());
    
    m_ctrl->SetValue(s);
}

// (EVENT) ComboBox.selectionChanged
// Description: Fired when the combobox's selection has changed.
// Param(text): The value of the combo box.

// (EVENT) ComboBox.textChanged
// Description: Fired when combobox's text has changed.
// Param(text): The value of the combo box.

// (EVENT) ComboBox.enterPressed
// Description: Fired when enter has been pressed inside the combobox.
// Param(text): The value of the combo box.

void ComboBox::onEvent(wxEvent& event)
{
    wxCommandEvent& evt = (wxCommandEvent&)event;
    std::wstring text = towstr(evt.GetString());

    kscript::Value* event_args = new kscript::Value;
    event_args->setObject();
    
    if (event.GetEventType() == wxEVT_COMMAND_COMBOBOX_SELECTED)
    {
        event_args->getMember(L"text")->setString(text);
        invokeJsEvent(L"selectionChanged", event_args);
        return;
    }
    
    if (event.GetEventType() == wxEVT_COMMAND_TEXT_UPDATED)
    {
        event_args->getMember(L"text")->setString(text);
        invokeJsEvent(L"textChanged", event_args);
        return;
    }
    
    if (event.GetEventType() == wxEVT_COMMAND_TEXT_ENTER)
    {
        // -- this event doesn't include the string in it, so we'll
        //    just get it using GetValue() so that we're consistent --
        if (isControlValid())
            text = towstr(m_ctrl->GetValue());

        event_args->getMember(L"text")->setString(text);
        invokeJsEvent(L"enterPressed", event_args);
        return;
    }
}



// -- ChoiceBox class implementation --

// (CLASS) ChoiceBox
// Category: Control
// Derives: FormControl
// Description: A class that represents a choice box control.
// Remarks: The ChoiceBox class represents a choice box control.

// (EVENT) ChoiceBox.selectionChanged
// Description: Fired when the choicebox's selection has changed.
// Param(text): The value of the choice box.

ChoiceBox::ChoiceBox()
{
    m_ctrl = NULL;
}

ChoiceBox::~ChoiceBox()
{

}

// (CONSTRUCTOR) ChoiceBox.constructor
// Description: Creates a new ChoiceBox.
//
// Syntax: ChoiceBox(x_pos : Integer,
//                   y_pos : Integer,
//                   width : Integer,
//                   height : Integer)
//
// Remarks: Creates a new ChoiceBox at the position specified by 
//     |x_pos| and |y_pos|, and having dimensions specified by |width| 
//     and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void ChoiceBox::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // add some properties
    kscript::Value obj;
    
    getMember(L"selectionChanged")->setObject(Event::createObject(env));

    // set default values
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
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
        // create the control
        m_ctrl = new wxChoice(getApp()->getTempParent(),
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
        m_ctrl = new wxChoice(getApp()->getTempParent(),
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

void ChoiceBox::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (!m_enabled)
    {
        m_ctrl->Enable(false);
    }
    
    listenEvent(wxEVT_COMMAND_CHOICE_SELECTED);
}

// (METHOD) ChoiceBox.addItem
// Description: Adds a text item to the choice box.
//
// Syntax: function ChoiceBox.addItem(text : String) : Integer
//
// Remarks: Adds an item, specified by |text|, to the list
//     of items in the choice box.
//
// Param(text): The |text| to add to the list of choice box items.
// Returns: Returns the index of the item that was added.

// ----------
// ( METHOD ) ChoiceBox.add
// DEPRECATED: Use ChoiceBox.addItem() instead of ChoiceBox.add()

void ChoiceBox::addItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    wxString s = towx(env->getParam(0)->getString());
    int idx = m_ctrl->Append(s);
    retval->setInteger(idx);
}

// (METHOD) ChoiceBox.clear
// Description: Clears the choice box of all items.
//
// Syntax: function ChoiceBox.clear()
//
// Remarks: Clears the choice box of all items.

void ChoiceBox::clear(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->Clear();
}

// (METHOD) ChoiceBox.deleteItem
// Description: Deletes an item with a specified index from the choice box.
//
// Syntax: function ChoiceBox.deleteItem(index : Integer)
//
// Remarks: Deletes an item with a specified |index| from the choice box.
//
// Param(index): The |index| of the item to delete from the list of 
//     items in the choice box.

// ----------
// ( METHOD ) ChoiceBox.delete
// DEPRECATED: Use ChoiceBox.deleteItem() instead of ChoiceBox.delete()

void ChoiceBox::deleteItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    int idx = env->getParam(0)->getInteger();
    m_ctrl->Delete(idx);
}

// (METHOD) ChoiceBox.findItem
// Description: Finds the index of a string in the choice box.
//
// Syntax: function ChoiceBox.findItem(text : String) : Integer
//
// Remarks: Finds the |text| item in the choice box and returns
//     it's index if it's found and -1 if it isn't found.
//
// Param(text): The |text| to find in the choice box.
// Returns: Returns the index of the |text| that's found in the
//     choice box and -1 if the text isn't found.

// ----------
// ( METHOD ) ChoiceBox.findString
// DEPRECATED: Use ChoiceBox.findItem() instead of ChoiceBox.findString()

void ChoiceBox::findItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    wxString s = towx(env->getParam(0)->getString());
    int idx = m_ctrl->FindString(s);
    retval->setInteger(idx);
}

// (METHOD) ChoiceBox.getItemCount
// Description: Gets the number of items in the choice box.
//
// Syntax: function ChoiceBox.getItemCount() : Integer
//
// Remarks: Returns the number of items in the choice box.
//
// Returns: Returns the number of items in the choice box.

// ----------
// ( METHOD ) ChoiceBox.getCount
// DEPRECATED: Use ChoiceBox.getItemCount() instead of ChoiceBox.getCount()

void ChoiceBox::getItemCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setInteger(m_ctrl->GetCount());
}

// (METHOD) ChoiceBox.getSelectedIndex
// Description: Gets the index of the selected item in the choice box.
//
// Syntax: function ChoiceBox.getSelectedIndex() : Integer
//
// Remarks: Returns the index of the selected item in the choice box.
//
// Returns: Returns the index of the selected item in the choice box.

// ----------
// ( METHOD ) ChoiceBox.getSelection
// DEPRECATED: Use ChoiceBox.getSelectedIndex() instead of ChoiceBox.getSelection()

void ChoiceBox::getSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setInteger(m_ctrl->GetSelection());
}

// (METHOD) ChoiceBox.getItem
// Description:  Gets an item from the choice box based on the
//     specified index.
//
// Syntax: function ChoiceBox.getItem(index : Integer) : String
//
// Remarks: Returns the text item at the specified |index|.  If |index|
//     isn't specified, then the currently selected string is returned,
//     which may include the item that's currently being entered in
//     the choice box before it actually becomes part of the list.
//
// Param(index): The |index| of the item in the choice box to return.
// Returns: The text item at the specified |index|.

// ----------
// ( METHOD ) ChoiceBox.getString
// DEPRECATED: Use ChoiceBox.getItem() instead of ChoiceBox.getString()

void ChoiceBox::getItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    if (env->getParamCount() == 0)
    {
        retval->setString(towstr(m_ctrl->GetStringSelection()));
    }
     else
    {
        int idx = env->getParam(0)->getInteger();
        retval->setString(towstr(m_ctrl->GetString(idx)));
    }
}

// (METHOD) ChoiceBox.getValue
// Description:  Gets the current text in the choice box.
//
// Syntax: function ChoiceBox.getValue() : String
//
// Remarks: Returns the current text in the choice box.

void ChoiceBox::getValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    retval->setString(towstr(m_ctrl->GetStringSelection()));
}

// (METHOD) ChoiceBox.insertItem
// Description: Inserts a text item at a specified index.
//
// Syntax: function ChoiceBox.insertItem(text : String,
//                                       index : Integer)
//
// Remarks: Inserts a |text| item at a specified |index|.
//
// Param(text): The |text| item to add to the choice box.
// Param(index): The |index| at which to insert the |text|
//     in the choice box.

// ----------
// ( METHOD ) ChoiceBox.insert
// DEPRECATED: Use ChoiceBox.insertItem() instead of ChoiceBox.insert()

void ChoiceBox::insertItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 2 || !isControlValid())
        return;

    wxString s = towx(env->getParam(0)->getString());
    int idx = env->getParam(1)->getInteger();
    m_ctrl->Insert(s, idx);
}

// (METHOD) ChoiceBox.selectItem
// Description: Selects a choice box item at the specified index.
//
// Syntax: function ChoiceBox.selectItem(index : Integer)
//
// Remarks: Selects a choice box item at the specified |index|.
//
// Param(index): The |index| of the item to select in the choice box.

// ----------
// ( METHOD ) ChoiceBox.select
// DEPRECATED: Use ChoiceBox.selectItem() instead of ChoiceBox.select()

void ChoiceBox::selectItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    int idx = env->getParam(0)->getInteger();
    m_ctrl->Select(idx);
}

// (METHOD) ChoiceBox.setItem
// Description: Sets the text of a choice box item at the specified index.
//
// Syntax: function ChoiceBox.setItem(index : Integer,
//                                    text : String)
//
// Remarks: Sets the new |text| of a choice box item at a specified |index|.
//
// Param(index): The |index| of the item in the choice box for which to
//     change the |text|.
// Param(text): The new |text| for the item at the specified |index|.

// ----------
// ( METHOD ) ChoiceBox.setString
// DEPRECATED: Use ChoiceBox.setItem() instead of ChoiceBox.setString()

void ChoiceBox::setItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 2 || !isControlValid())
        return;

    int idx = env->getParam(0)->getInteger();
    wxString s = towx(env->getParam(1)->getString());

    m_ctrl->SetString(idx, s);
}

// (METHOD) ChoiceBox.setValue
// Description:  Sets the current text in the choice box.
//
// Syntax: function ChoiceBox.setValue(text : String) : Boolean
//
// Remarks: Sets the current text in the choice box.  If the text item
//          doesn't exist in the choice box, the function will do nothing
//          and will return false.
//
// Param(text): The |text| item to set in the choice box.

void ChoiceBox::setValue(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
    {
        retval->setBoolean(false);
        return;
    }
    
    wxString s = towx(env->getParam(0)->getString());
    
    bool res = m_ctrl->SetStringSelection(s);
    retval->setBoolean(res);
}

void ChoiceBox::onEvent(wxEvent& event)
{
    if (event.GetEventType() == wxEVT_COMMAND_CHOICE_SELECTED)
    {
        wxCommandEvent& evt = (wxCommandEvent&)event;
        std::wstring text = towstr(evt.GetString());

        kscript::Value* event_args = new kscript::Value;
        event_args->setObject();
        event_args->getMember(L"text")->setString(text);
        invokeJsEvent(L"selectionChanged", event_args);
    }
}


