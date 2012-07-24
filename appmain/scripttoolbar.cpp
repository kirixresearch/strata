/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-21
 *
 */


#include "appmain.h"
#include "scripthost.h"
#include "scriptgui.h"
#include "scriptbitmap.h"
#include "scripttoolbar.h"


// -- ToolBarItem class implementation --

// (CLASS) ToolBarItem
// Category: Form
// Description: A class that represents an item on a toolbar.
// Remarks: The ToolBarItem represents an item on a toolbar.

ToolBarItem::ToolBarItem()
{
    m_id = -1;
    m_enabled = true;
    m_label = wxEmptyString;
    m_tooltip = wxEmptyString;
    m_help_str = wxEmptyString;
    m_bitmap = wxNullBitmap;
    m_disabled_bitmap = wxNullBitmap;
    
    m_separator = false;
    
    m_owner = NULL;
}

ToolBarItem::~ToolBarItem()
{

}

const int ToolBarItem::getId()
{
    return m_id;
}

// (EVENT) ToolBarItem.click
// Description: Fired when the tool bar item is clicked.

void ToolBarItem::onEvent(wxEvent& event)
{
    invokeJsEvent(L"click");
}

// (CONSTRUCTOR) ToolBarItem.constructor
// Description: Creates a new toolbar item.
//
// Syntax: ToolBarItem()
// Syntax: ToolBarItem(enabled_bitmap : Bitmap,
//                     disabled_bitmap : Bitmap,
//                     label : String,
//                     help : String)
//
// Remarks: Creates a new toolbar item with a given |label| and |help|
//     string.  The |enabled_bitmap| will show when the toolbar item is enabled, 
//     and the |disabled_bitmap| will show when the toolbar item is disabled.
//
// Param(enabled_bitmap): The bitmap that shows when the toolbar item is enabled.
// Param(disabled_bitmap): The bitmap that shows when the toolbar item is disabled.
// Param(label): The toolbar label.
// Param(help): The toolbar help text.

void ToolBarItem::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- initialize the component --
    initComponent(env);

    // -- add some properties --
    kscript::Value obj;
    
    getMember(L"click")->setObject(Event::createObject(env));

    // -- set the ID for this toolbar item --
    m_id = getUniqueScriptCommandId();

    // -- we need to create a default bitmap so that wxWidgets does not throw
    //    an assert failure if there is no bitmap for the toolbar item --
    //wxImage img(10,10, false);
    m_bitmap = wxBitmap(0,0);
    
    // -- get user input values --
    if (env->getParamCount() > 0)
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->getClassName() == L"Bitmap")
        {
            Bitmap* b = (Bitmap*)obj;
            m_bitmap = b->getWxBitmap();
        }
    }
    if (env->getParamCount() > 1)
    {
        kscript::ValueObject* obj = env->getParam(1)->getObject();
        if (obj->getClassName() == L"Bitmap")
        {
            Bitmap* b = (Bitmap*)obj;
            m_disabled_bitmap = b->getWxBitmap();
        }
    }
    if (env->getParamCount() > 2)
        m_label = env->getParam(2)->getString();
    if (env->getParamCount() > 3)
        m_help_str = env->getParam(3)->getString();
}

// (METHOD) ToolBarItem.setEnabled
// Description: Enables or disables a toolbar item.
//
// Syntax: function ToolBarItem.setEnabled()
// Syntax: function ToolBarItem.setEnabled(enabled : Boolean)
//
// Remarks: Enables or disables a toolbar item.  If |enabled|
//     isn't specified, the toolbar item is enabled; otherwise the 
//     toolbar item is enabled if |enabled| is true and disabled if 
//     |enabled| is false.
//
// Param(enabled): A flag which determines whether a toolbar item is
//     enabled or not.

void ToolBarItem::setEnabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_enabled = true;
    
    if (env->getParamCount() > 0 &&
        env->getParam(0)->getBoolean() == false)
    {
        m_enabled = false;
    }
}

// (METHOD) ToolBarItem.setLabel
// Description: Sets the label for a toolbar item.
//
// Syntax: function ToolBarItem.setLabel(label : String)
//
// Remarks: Sets the toolbar item text specified by |label|.
//
// Param(label): The new text for the toolbar item.

void ToolBarItem::setLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_label = towx(env->getParam(0)->getString());
}

// (METHOD) ToolBarItem.getLabel
// Description: Returns the text of the toolbar item.
//
// Syntax: function ToolBarItem.getLabel() : String
//
// Remarks: Returns the text of the toolbar item.
//
// Returns: The text of the toolbar item.

void ToolBarItem::getLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_label));
}

// (METHOD) ToolBarItem.setTooltip
// Description: Sets the tooltip text for the toolbar item.
//
// Syntax: function ToolBarItem.setTooltip(tooltip : String)
//
// Remarks: Sets the tooltip text, specified by |tooltip|, for the
//     toolbar item.
//
// Param(tooltip): The text to use when the tooltip shows.

void ToolBarItem::setTooltip(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_tooltip = towx(env->getParam(0)->getString());
}

// (METHOD) ToolBarItem.getTooltip
// Description: Returns the tooltip text for the toolbar item.
//
// Syntax: function ToolBarItem.getTooltip() : String
//
// Remarks: Returns the tooltip text for the toolbar item.
//
// Returns: The tooltip text for the toolbar item.

void ToolBarItem::getTooltip(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_tooltip));
}

// (METHOD) ToolBarItem.setHelpString
// Description: Sets the help string for the toolbar item.
//
// Syntax: function ToolBarItem.setHelpString(help : String)
//
// Remarks: Sets the toolbar item help string, specified by |help|.
//
// Param(help): The help string associated with the toolbar item.

void ToolBarItem::setHelpString(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_help_str = towx(env->getParam(0)->getString());
}

// (METHOD) ToolBarItem.getHelpString
// Description: Returns the help string associated with the toolbar item.
//
// Syntax: function ToolBarItem.getHelpString() : String
//
// Remarks: Returns the help string associated with the toolbar item.
//
// Returns: The help string associated with the toolbar item.

void ToolBarItem::getHelpString(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_help_str));
}

// (METHOD) ToolBarItem.setBitmap
// Description: Sets a bitmap for the toolbar item.
//
// Syntax: function ToolBarItem.setBitmap(bitmap : Bitmap)
//
// Remarks: Sets the toolbar item's bitmap, specified by |bitmap|, that will be shown when
// the toolbar item is enabled.
//
// Param(bitmap): The bitmap to show on the toolbar when the toolbar is enabled.

void ToolBarItem::setBitmap(kscript::ExprEnv* env, kscript::Value* retval)
{
    Bitmap* b = (Bitmap*)(env->getParam(0)->getObject());
    if (b)
    {
        m_bitmap = b->getWxBitmap();
    }
}

// (METHOD) ToolBarItem.setDisabledBitmap
// Description: Sets a bitmap for the toolbar item.
//
// Syntax: function ToolBarItem.setDisabledBitmap(bitmap : Bitmap)
//
// Remarks: Sets the toolbar item's, specified by |bitmap|, that will be shown when
// the toolbar item is disabled.
//
// Param(bitmap): The bitmap to show on the toolbar when the toolbar is disabled.

void ToolBarItem::setDisabledBitmap(kscript::ExprEnv* env, kscript::Value* retval)
{
    Bitmap* b = (Bitmap*)(env->getParam(0)->getObject());
    if (b)
    {
        m_disabled_bitmap = b->getWxBitmap();
    }
}




// -- ToolBar class implementation --

// (CLASS) ToolBar
// Category: Form
// Description: A class that represents a toolbar.
// Remarks: The ToolBar class represents a toolbar.

ToolBar::ToolBar()
{
    m_ctrl = NULL;
    m_realized = false;
}

ToolBar::~ToolBar()
{

}

wxToolBar* ToolBar::getWxToolBar()
{
    return m_ctrl;
}

ToolBarItem* ToolBar::getToolBarItemFromId(int id)
{
    // -- find the menuitem based on it's id --
    std::vector<ToolBarItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        if ((*it)->getId() == id)
            return (*it);
    }
    
    return NULL;
}

void ToolBar::addItemToWxToolBar(ToolBarItem* item)
{
    // -- if the item is a separator, add it to the toolbar --
    if (item->m_separator)
    {
        m_ctrl->AddSeparator();
        return;
    }

    // -- add the toolbaritem to the toolbar --
    m_ctrl->AddTool(item->m_id,
                    item->m_label,
                    item->m_bitmap,
                    item->m_disabled_bitmap,
                    wxITEM_NORMAL,
                    item->m_tooltip,
                    item->m_help_str);
}

// (CONSTRUCTOR) ToolBar.constructor
// Description: Creates a new toolbar.
//
// Syntax: ToolBar()
//
// Remarks: Creates a new toolbar.

void ToolBar::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- initialize the component --
    initComponent(env);

    m_ctrl = new wxToolBar(getApp()->getTempParent(),
                           -1,
                           wxDefaultPosition,
                           wxDefaultSize);
    m_wnd = m_ctrl;
}

void ToolBar::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    // -- get the toolbar's bitmap size --
    wxSize s = m_ctrl->GetToolBitmapSize();
    
    std::vector<ToolBarItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        ToolBarItem* item = (*it);

        // -- get the toolbaritems's bitmap size --
        wxSize s2 = wxSize(item->m_bitmap.GetWidth(),
                           item->m_bitmap.GetHeight());

        // -- if the toolbaritem's bitmap size is greater than the toolbar's
        //    bitmap size, increase the toolbar's bitmaps size to match --                           
        s.IncTo(s2);
        m_ctrl->SetToolBitmapSize(s);
    }
    
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        ToolBarItem* item = (*it);
        addItemToWxToolBar(item);
    }

    // -- realize must be called on the wxToolBar after
    //    all items have been added to it --
    m_ctrl->Realize();
        
    if (!m_enabled)
    {
        m_ctrl->Enable(false);
    }
    
    m_realized = true;
}

// (METHOD) ToolBar.add
// Description: Adds a toolbar item to the toolbar.
//
// Syntax: function ToolBar.add(item : ToolBarItem)
//
// Remarks: Adds a toolbar |item| to the toolbar.
//
// Param(item):  The toolbar item to add to the toolbar.

void ToolBar::add(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    if (obj->getClassName() == L"ToolBarItem")
    {
        ToolBarItem* item = (ToolBarItem*)obj;
        if (item)
        {
            m_items.push_back(item);
        }
        
        // -- if we've already created and shown the toolbar, add the item
        //    and call Realize() on the wxToolBar so the item is shown --
        if (m_realized)
        {
            addItemToWxToolBar(item);
            m_ctrl->Realize();
        }
    }
}

void ToolBar::addSeparator(kscript::ExprEnv* env, kscript::Value* retval)
{
    ToolBarItem* item = ToolBarItem::createObject(env);
    item->m_separator = true;
    m_items.push_back(item);
    
    // -- if we've already created and shown the toolbar, add the item
    //    and call Realize() on the wxToolBar so the item is shown --
    if (m_realized)
    {
        addItemToWxToolBar(item);
        m_ctrl->Realize();
    }
}

