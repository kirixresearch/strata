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
#include "scriptstatusbar.h"


// -- StatusBarItem class implementation --

// (CLASS) StatusBarItem
// Category: Form
// Description: A class that represents an item on a statusbar.
// Remarks: The StatusBarItem represents an item on a statusbar. 

zStatusBarItem::zStatusBarItem()
{
    m_owner = NULL;
    m_text = wxT("");
    m_width = 50;
    m_idx = -1;
}

zStatusBarItem::~zStatusBarItem()
{

}

// (CONSTRUCTOR) StatusBarItem.constructor
// Description: Creates a new status bar item.
//
// Syntax: StatusBar()
// Syntax: StatusBar(width : Integer,
//                   text : String)
//
// Remarks: Creates a new status bar item with a fixed |width|
//      and default |text|, if they are specified.
//
// Param(width): The fixed |width| of the newly created item.
// Param(text): The default |text| for the newly created item.

void zStatusBarItem::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    if (env->getParamCount() > 0)
        m_width = env->getParam(0)->getInteger();
    if (env->getParamCount() > 1)
        m_text = env->getParam(1)->getString();
}

// (METHOD) StatusBarItem.setText
// Description: Sets the status bar item's text.
//
// Syntax: function StatusBarItem.setText(text : String)
//
// Remarks: Sets the status bar item's |text|.
//
// Param(text): The |text| to which to set the text of the status bar.

void zStatusBarItem::setText(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_text = env->getParam(0)->getString();
    
    if (m_owner)
    {
        m_owner->SetStatusText(m_text, m_idx);
    }
}

// (METHOD) StatusBarItem.getText
// Description: Gets the text of the status bar item.
//
// Syntax: function StatusBarItem.getText() : String
//
// Remarks: Returns the text of the status bar item.
//
// Returns: Returns the text of the status bar item.

void zStatusBarItem::getText(kscript::ExprEnv* env, kscript::Value* retval)
{
    std::wstring text = towstr(m_text);
    retval->setString(text);
}

// (METHOD) StatusBarItem.setWidth
// Description: Sets the width of the status bar item.
//
// Syntax: function StatusBarItem.setWidth(width : Integer)
//
// Remarks: Sets the |width| of the status bar item.
//
// Param(width): The |width| to which to set the width of the status bar item.

void zStatusBarItem::setWidth(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_width = env->getParam(0)->getInteger();
}

// TODO: should we include the following setStretchProportion method, 
// or should the end user implement the equivalent with sizers?

// (METHOD) StatusBarItem.setStretchProportion
// Description: Sets the proportional size of the status bar item.
//
// Syntax: function StatusBarItem.setStretchProportion(size : Integer)
//
// Remarks: Sets the proportional |size| of the status bar item
//     relative to all other proportional items.
//
// Param(value): The proportional |size| of the status bar item, relative
//     to all other proportional items.

void zStatusBarItem::setStretchProportion(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_width = env->getParam(0)->getInteger() * -1;
}


// -- StatusBar class implementation --

// (CLASS) StatusBar
// Category: Form
// Description: A class that represents a status bar.
// Remarks: The StatusBar class represents a status bar.

zStatusBar::zStatusBar()
{
    m_ctrl = NULL;
}

zStatusBar::~zStatusBar()
{

}

wxStatusBar* zStatusBar::getWxStatusBar()
{
    return m_ctrl;
}

// (CONSTRUCTOR) StatusBar.constructor
// Description: Creates a new status bar.
//
// Syntax: StatusBar()
//
// Remarks: Creates a new status bar.

void zStatusBar::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    m_ctrl = new wxStatusBar(getApp()->getTempParent(), -1);
    m_wnd = m_ctrl;
}

void zStatusBar::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    int size = m_items.size();
    int* widths = new int[size];
    
    int i = 0;
    std::vector<zStatusBarItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        (*it)->m_owner = m_ctrl;
        (*it)->m_idx = i;
        
        widths[i++] = (*it)->m_width;
    }
    
    // create the status bar boxes
    if (size > 0)
        m_ctrl->SetFieldsCount(size, widths);
     else
        m_ctrl->SetFieldsCount(1);

    // now that we've created the status bar boxes,
    // let's add any default text that was set

    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        m_ctrl->SetStatusText((*it)->m_text, (*it)->m_idx);
    }
    
    // -- normally we would call m_ctrl->Reparent() here, but because the
    //    status bar's parent must be FormFrame and m_form_wnd is the wxPanel
    //    inside FormFrame, we must call Reparent() in Form::setStatusBar() --
    
    if (!m_enabled)
    {
        m_ctrl->Enable(false);
    }
    
    delete[] widths;
}

// (METHOD) StatusBar.add
// Description: Adds a status bar item to the status bar.
//
// Syntax: function StatusBar.add(item : StatusBarItem)
//
// Remarks: Adds a status bar |item| to the status bar.
//
// Param(item):  The status bar |item| to add to the status bar.

void zStatusBar::add(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj)
    {
        if (obj->getClassName() == L"StatusBarItem")
        {
            zStatusBarItem* item = (zStatusBarItem*)obj;
            m_items.push_back(item);
        }
    }
}

