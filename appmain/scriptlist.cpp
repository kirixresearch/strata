/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-07
 *
 */


#include "appmain.h"
#include "scriptgui.h"
#include "scriptlist.h"


// -- ListBox class implementation --

// (CLASS) ListBox
// Category: Control
// Derives: FormControl
// Description: A class that represents a list box control.
// Remarks: The ListBox class represents a list box control.

// (EVENT) ListBox.itemSelect
// Description: Fired when a listbox item is selected.
// Param(index): The index of the listbox item that is selected.

// (EVENT) ListBox.itemActivate
// Description: Fired when a listbox item is activated.
// Param(index): The index of the listbox item that is activated.

zListBox::zListBox()
{
    m_ctrl = NULL;
}

zListBox::~zListBox()
{

}

// (CONSTRUCTOR) ListBox.constructor
// Description: Creates a new ListBox control.
//
// Syntax: ListBox(x_pos : Integer,
//                 y_pos : Integer,
//                 width : Integer,
//                 height : Integer)
//
// Remarks: Creates a new ListBox control at the position specified by 
//     |x_pos| and |y_pos|, and having dimensions specified by |width| 
//     and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void zListBox::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // initialize event objects
    getMember(L"itemActivate")->setObject(Event::createObject(env));
    getMember(L"itemSelect")->setObject(Event::createObject(env));
    
    // -- set default values --
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
        wxArrayString empty_array;
        m_ctrl = new wxListBox(getApp()->getTempParent(),
                               -1,
                               wxDefaultPosition,
                               wxDefaultSize,
                               empty_array,
                               wxLB_EXTENDED);
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
        wxArrayString empty_array;
        m_ctrl = new wxListBox(getApp()->getTempParent(),
                               -1,
                               wxPoint(m_x, m_y),
                               wxSize(m_width, m_height),
                               empty_array,
                               wxLB_EXTENDED);
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

void zListBox::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (!m_enabled)
        m_ctrl->Enable(false);
    
    listenEvent(wxEVT_COMMAND_LISTBOX_SELECTED);
    listenEvent(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED);
}

// (METHOD) ListBox.addItem
// Description: Adds text to the listbox.
//
// Syntax: function ListBox.addItem(text : String) : Integer
//
// Remarks: Adds the |text| to the listbox and returns
//     the index of the |text| item that was added.
//
// Param(text): The |text| to add to the listbox.
//
// Returns: Returns the index of the |text| item that was added.  If
//     an error is encountered during the call, -1 is returned.

void zListBox::addItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(-1);
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    wxString s = towx(env->getParam(0)->getString());
    int idx = m_ctrl->Append(s);
    retval->setInteger(idx);
}

// (METHOD) ListBox.clear
// Description: Clears the listbox of all items.
//
// Syntax: function ListBox.clear()
//
// Remarks: Clears the listbox of all items.

void zListBox::clear(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    m_ctrl->Clear();
}

// (METHOD) ListBox.deleteItem
// Description: Deletes an item from the listbox.
//
// Syntax: function ListBox.deleteItem(index : Integer) : Boolean
//
// Remarks: Deletes an item at the specified |index| from
//     the listbox.
//
// Param(index): The index of the item to delete.
// Returns: True if the function succeeded, false otherwise
void zListBox::deleteItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    int idx = env->getParam(0)->getInteger();
    if (idx >= 0 && idx < (int)m_ctrl->GetCount())
    {
        m_ctrl->Delete(idx);
        retval->setBoolean(true);
    }
}

// (METHOD) ListBox.deselectItem
//
// Description: Deselects the item at the specified item index.
//
// Syntax: function ListBox.deselectItem(index : Integer)
//
// Remarks: Deselects the item at the specified |index|.

void zListBox::deselectItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
    
    int idx = env->getParam(0)->getInteger();
    if (idx < 0 || idx >= (int)m_ctrl->GetCount())
        return;
    
    m_ctrl->Deselect(idx);
}

// (METHOD) ListBox.deselectAllItems
//
// Description: Deselects all the items in the listbox.
//
// Syntax: function ListBox.deselectAllItems()
//
// Remarks: Deselects all the items in the listbox.

void zListBox::deselectAllItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;
    
    // get the selected item indexes
    wxArrayInt sels;
    int count = m_ctrl->GetSelections(sels);
    if (count < 1)
        return;
    
    // deselect the selected item indexes
    int i, idx;
    for (i = 0; i < count; ++i)
    {
        idx = sels.Item(i);
        m_ctrl->Deselect(idx);
    }
}

// (METHOD) ListBox.findItem
// Description: Finds a listbox item with the specified text.
//
// Syntax: function ListBox.findItem(text : String,
//                                   index : Integer) : Integer
//
// Remarks: Finds a listbox item with the specified |text| and returns
//     the zero-based index of the item that was found.  If the
//     specified |text| item can't be found, the function returns -1.
//
// Param(text): The |text| of the item to find.
// Param(index): (Optional) The zero-based |index| of the item to start at.
//     If this parameter is left blank or is -1, the find operation
//     will always start with the first item in the ListBox.
//
// Returns: Returns the zero-based index of the item if it's found, and
//     -1 if the item can't be found.

void zListBox::findItem(kscript::ExprEnv* env, kscript::Value* retval)
{    
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    retval->setInteger(-1);
    
    size_t param_count = env->getParamCount();
    if (param_count < 1)
        return;
    
    // get the find string
    wxString s = towx(env->getParam(0)->getString());
    
    // get the start index
    int start_idx = -1;
    if (param_count > 1)
        start_idx = env->getParam(1)->getInteger();
    
    // don't allow negative start indexes less than -1
    if (start_idx < -1)
        return;
    
    // the start index is exclusive, not inclusive,
    // so we need to increment this variable here (-1 will start
    // matching at index 0; 0 will start matching at index 1, etc.)
    start_idx++;
    
    // index out-of-bounds, bail out
    int item_count = m_ctrl->GetCount();
    if (start_idx < 0 || start_idx >= item_count)
        return;
    
    // since the wxListBox control doesn't all you to specify a start
    // index for the find, we'll use the wxArrayString class to do the find
    wxArrayString strs = m_ctrl->GetStrings();
    size_t idx, count = strs.Count();
    
    // start the find at the start index
    for (idx = start_idx; idx < count; ++idx)
    {
        if (s.CmpNoCase(strs.Item(idx)) == 0)
        {
            retval->setInteger((int)idx);
            return;
        }
    }
}


// (METHOD) ListBox.getSelectedIndex
// Description: Gets the index of the selected item in the listbox.
//
// Syntax: function ListBox.getSelectedIndex(index : Integer) : Integer
//
// Remarks: Returns the zero-based index of the first selected item
//          in the listbox after the start |index|.  If there
//          are no selected items, the function returns -1.
//
// Param(index): (Optional) The zero-based |index| of the item to start at.
//     If this parameter is left blank or is -1, the find operation
//     will always start with the first item in the listbox.
//
// Returns: Returns the zero-based index of the first selected item
//          in the listbox after the start |index|.  If there
//          are no selected items, the function returns -1.

void zListBox::getSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    retval->setInteger(-1);
    
    int param_count = env->getParamCount();
    
    // get the start index
    int start_idx = -1;
    if (param_count > 0)
        start_idx = env->getParam(0)->getInteger();
    
    // get the selected item indexes
    wxArrayInt sels;
    int i, count = m_ctrl->GetSelections(sels);
    if (count < 1)
        return;
    
    // if the start index is -1, return the first selection
    if (start_idx == -1)
    {
        retval->setInteger(sels.Item(0));
        return;
    }
    
    // iterate through the selected indexes to find the first selected
    // index after the starting index (exclusive)
    for (i = 0; i < count; ++i)
    {
        int sel_idx = sels.Item(i);
        if (sel_idx <= start_idx)
            continue;
        
        // get the first selected index after the start index (exclusive)
        retval->setInteger(sel_idx);
        break;
    }
}


// (METHOD) ListBox.getSelectedItem
// Description: Gets the text of the selected item in the listbox.
//
// Syntax: function ListBox.getSelectedItem(index : Integer) : String
//
// Remarks: Returns the text of the first selected item in the listbox
//          after the start |index|.  If there are no selected items,
//          the function returns null.
//
// Param(index): (Optional) The zero-based |index| of the item to start at.
//     If this parameter is left blank or is -1, the find operation
//     will always start with the first item in the listbox.
//
// Returns: Returns the text of the first selected item in the listbox
//          after the start |index|.  If there is no selected item or
//          an error occurs, null is returned.

void zListBox::getSelectedItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setNull();
    
    int param_count = env->getParamCount();
    
    // get the start index
    int start_idx = -1;
    if (param_count > 0)
        start_idx = env->getParam(0)->getInteger();
    
    // get the selected item indexes
    wxArrayInt sels;
    int i, count = m_ctrl->GetSelections(sels);
    if (count < 1)
        return;
    
    // if the start index is -1, return the first selection
    if (start_idx == -1)
    {
        retval->setString(towstr(m_ctrl->GetString(sels.Item(0))));
        return;
    }
    
    // iterate through the selected indexes to find the first selected
    // item after the starting index (exclusive)
    for (i = 0; i < count; ++i)
    {
        int sel_idx = sels.Item(i);
        if (sel_idx <= start_idx)
            continue;
        
        // get the first selected item after the start index (exclusive)
        retval->setString(towstr(m_ctrl->GetString(sel_idx)));
        break;
    }
}


// (METHOD) ListBox.getSelectedItems
// Description: Returns the selected items in the listbox.
//
// Syntax: function ListBox.getSelectedItems() : Array(String)
//
// Remarks: Returns an Array object containing all selected items
//     in the listbox. If there are no selected items or an error occurs,
//     an empty array is returned.
//
// Returns: An Array of strings

void zListBox::getSelectedItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    // set our return type to be an array
    retval->setArray(env);
    
    if (!isControlValid())
        return;

    // get the selected item indexes
    wxArrayInt sels;
    int count = m_ctrl->GetSelections(sels);
    if (count < 1)
        return;
    
    // add the selected strings to the array
    int i, idx;
    for (i = 0; i < count; ++i)
    {
        idx = sels.Item(i);
        kscript::Value val;
        val.setString(towstr(m_ctrl->GetString(idx)));
        retval->appendMember(&val);
    }
}


// (METHOD) ListBox.getItem
// Description: Gets the listbox item at the specified position.
//
// Syntax: function ListBox.getItem(index : Integer) : String
//
// Remarks: Returns the item at the specified, zero-based |index| item
//     in the listbox. If the index isn't specified, the function returns
//     the current item in the listbox.
//
// Param(index): The zero-based |index| of the item to return.
//
// Returns: Returns the item at the specified |index|.

void zListBox::getItem(kscript::ExprEnv* env, kscript::Value* retval)
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


// (METHOD) ListBox.getItems
// Description: Gets an array of all listbox items.
//
// Syntax: function ListBox.getItems() : Array(String)
//
// Remarks: Returns an array of strings containing all items in
//     the listbox.
//
// Returns: An array of strings.

void zListBox::getItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setArray(env);
    size_t i, count = m_ctrl->GetCount();
    for (i = 0; i < count; ++i)
    {
        kscript::Value v;
        v.setString(towstr(m_ctrl->GetString(i)));
        retval->setMemberI(i, &v);
    }
}


// (METHOD) ListBox.getItemCount
// Description: Gets the number of items in the listbox.
//
// Syntax: function ListBox.getItemCount() : Integer
//
// Remarks: Returns the number of items in the listbox.
// Returns: An integer representing the number of items in the listbox.

void zListBox::getItemCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setInteger(m_ctrl->GetCount());
}

// (METHOD) ListBox.insertItem
// Description: Inserts a text item into the listbox at a specified position.
//
// Syntax: function ListBox.insertItem(text : String,
//                                     index : Integer)
//
// Remarks: Inserts the |text| into the listbox at the position specified
//     by |index|.
//
// Param(text): The |text| to insert into the listbox.
// Param(index): The |index| where the text should be inserted.

// ----------
// ( METHOD ) ListBox.insert
// DEPRECATED: Use ListBox.insertItem() instead of ListBox.insert()

void zListBox::insertItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    if (env->getParamCount() < 2 || !isControlValid())
        return;
        
    wxString s = towx(env->getParam(0)->getString());
    int idx = env->getParam(1)->getInteger();
    if (idx < 0 || idx >= (int)m_ctrl->GetCount())
        m_ctrl->Append(s);
         else
        m_ctrl->Insert(s, idx);
        
    retval->setBoolean(true);
}

// (METHOD) ListBox.isItemSelected
// Description: Indicates whether or not the specified item is selected
//
// Syntax: function ListBox.isItemSelected(index : Integer) : Boolean
//
// Remarks: Indicates whether or not the item at the specified |index| is
//     selected. If the item is selected, the function returns true. If
//     the item is not selected, the function returns false.
//
// Param(index): The |index| of the item being checked for selection.
//
// Returns: Returns true if the item at the specified |index| is selected,
//     and false otherwise.  If an error is encountered during the call,
//     null is returned.

// ----------
// ( METHOD ) ListBox.getSelected
// DEPRECATED: Use ListBox.isItemSelected() instead of ListBox.getSelected()

void zListBox::isItemSelected(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    int idx = env->getParam(0)->getInteger();
    if (idx < 0 || idx >= (int)m_ctrl->GetCount())
        return;
        
    retval->setBoolean(m_ctrl->IsSelected(idx));
}

// (METHOD) ListBox.selectItem
//
// Description: Selects the item at the specified item index.
//
// Syntax: function ListBox.selectItem(index : Integer)
//
// Remarks: Select the item at the specified |index|.

void zListBox::selectItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;
    
    int idx = env->getParam(0)->getInteger();
    if (idx < 0 || idx >= (int)m_ctrl->GetCount())
        return;
    
    m_ctrl->Select(idx);
}

// ( METHOD ) ListBox.setSelected
// DEPRECATED: Use ListBox.selectItem() and ListBox.deselectItem() instead of ListBox.setSelected()
//
// Description: Selects the item at the specified item index.
//
// Syntax: function ListBox.setSelected(index : Integer,
//                                      selected : Boolean) : Boolean
//
// Remarks: Select or deselects the item at the specified |index|, based
//     on the boolean value of the |selected| parameter.  If omitted, the
//     |selected| parameter will default to |true|.

void zListBox::setSelected(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    if (env->getParamCount() < 1 || !isControlValid())
        return;
        
    int idx = env->getParam(0)->getInteger();
    if (idx < 0 || idx >= (int)m_ctrl->GetCount())
        return;
        
    bool select = true;
    if (env->getParamCount() >= 2)
        select = env->getParam(1)->getBoolean();
    
    if (select)
    {
        m_ctrl->Select(idx);
    }
     else
    {
        m_ctrl->Deselect(idx);
    }
    
    retval->setBoolean(true);
}

// (METHOD) ListBox.setItem
// Description: Sets the text at the specified item index.
//
// Syntax: function ListBox.setItem(index : Integer,
//                                  text : String)
//
// Remarks: Sets the |text| of the item at the specified |index|.
//
// Param(index): The |index| of the listbox item on which to set the text.
// Param(text): The new |text| for the item at the specified |index|.
// Returns: True if the function succeeded, false otherwise

void zListBox::setItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 2 || !isControlValid())
        return;
        
    int idx = env->getParam(0)->getInteger();
    if (idx < 0 || idx >= (int)m_ctrl->GetCount())
        return;
        
    wxString s = towx(env->getParam(1)->getString());

    m_ctrl->SetString(idx, s);
    retval->setBoolean(true);
}


// (METHOD) ListBox.sortItems
// Description: Sorts the items in the listbox
//
// Syntax: function ListBox.sortItems(ascending : Boolean)
//
// Remarks: Sorts the items in the listbox.  The |ascending| parameter,
//     which is optional, specifies the sort order.  If this parameter
//     is not specified, an ascending sort order is assumed.
//
// Param(ascending): (Optional) Specifying |true| directs the operation
//     to sort the control ascending, |false| descending.

static int sortStringAsc(const wxString& first, const wxString& second)
{
    return first.CmpNoCase(second);
}

static int sortStringDesc(const wxString& first, const wxString& second)
{
    return second.CmpNoCase(first);
}

void zListBox::sortItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    bool ascending = true;
    
    if (env->getParamCount() >= 1)
        ascending = env->getParam(1)->getBoolean();
    
    // since the wxListBox control doesn't all you to sort items,
    // we'll use the wxArrayString class to do the sort
    wxArrayString strs = m_ctrl->GetStrings();
    strs.Sort(ascending ? sortStringAsc : sortStringDesc);
    
    // repopulate the wxListBox with the sotrted array of strings
    m_ctrl->Set(strs);
}

void zListBox::onEvent(wxEvent& evt)
{
    wxEventType event_type = evt.GetEventType();
    
    if (event_type == wxEVT_COMMAND_LISTBOX_SELECTED ||
        event_type == wxEVT_COMMAND_LISTBOX_DOUBLECLICKED)
    {
        wxCommandEvent& tevt = (wxCommandEvent&)evt;
        long idx = tevt.GetInt();
        
        kscript::Value* event_args = new kscript::Value;
        event_args->getMember(L"index")->setInteger(idx);
        
        if (event_type == wxEVT_COMMAND_LISTBOX_SELECTED)
            invokeJsEvent(L"itemSelect", event_args);
         else if (event_type == wxEVT_COMMAND_LISTBOX_DOUBLECLICKED)
            invokeJsEvent(L"itemActivate", event_args);
    }
}


