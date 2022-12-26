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
#include "appcontroller.h"
#include "scriptapp.h"
#include "scriptgui.h"
#include "scriptmenu.h"


// MenuItem class implementation

// (CLASS) MenuItem
// Category: Form
// Description: A class that represents items on a menu.
// Remarks: The MenuItem class represents items on a menu.

MenuItem::MenuItem()
{
    m_id = -1;
    m_label = wxT("");
    m_help_str = wxT("");
    m_ctrl = NULL;
}

MenuItem::~MenuItem()
{
    g_app->getAppController()->getScriptCommandRouter()->unregisterCommandReceiver(m_id);
}

const int MenuItem::getId()
{
    return m_id;
}

const wxString MenuItem::getWxLabel()
{
    return m_label;
}

wxMenuItem* MenuItem::getWxMenuItem()
{
    return m_ctrl;
}

void MenuItem::setId(int id)
{
    m_id = id;
}

void MenuItem::setWxMenuItem(wxMenuItem* item)
{
    m_ctrl = item;
    m_id = item->GetId();
    m_label = item->GetItemLabelText();
    m_help_str = item->GetHelp();
}

void MenuItem::setWxMenu(wxMenu* menu)
{
    // if we've already created a control, that means we've already
    // added this menuitem to a different menu, so don't do anything
    if (m_ctrl)
        return;
    
    // create the control
    m_ctrl = new wxMenuItem(menu,
                            m_id,
                            m_label,
                            m_help_str);
}


// (EVENT) MenuItem.click
// Description: Fired when the menu item is clicked.

void MenuItem::onEvent(wxEvent& event)
{
    invokeJsEvent(L"click");
}



// (CONSTRUCTOR) MenuItem.constructor
// Description: Creates a new menu item.
//
// Syntax: MenuItem()
// Syntax: MenuItem(label : String,
//                  help : String)
//
// Remarks: Creates a new menu item having label text specified by 
//     |label| and help text specified by |help|.
//
// Param(label): The text that appears on the menu item.
// Param(help): The help string associated with the item.

void MenuItem::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // add some properties
    kscript::Value obj;
    
    getMember(L"click")->setObject(Event::createObject(env));

    // if a label was supplied, use it
    m_label = env->getParam(0)->getString();
    
    // if a help string was supplied, use it
    m_help_str = env->getParam(1)->getString();
    
    // set the ID for this menu
    m_id = getUniqueScriptCommandId();
    
    g_app->getAppController()->getScriptCommandRouter()->registerCommandReceiver(m_id, this);
}

// (METHOD) MenuItem.setEnabled
// Description: Enables or disables a menu item.
//
// Syntax: function MenuItem.setEnabled()
// Syntax: function MenuItem.setEnabled(enabled : Boolean)
//
// Remarks: Enables or disables a menu item.  If |enabled|
//     isn't specified, the menu item is enabled; otherwise the 
//     menu item is enabled if |enabled| is true and disabled if 
//     |enabled| is false.
//
// Param(enabled): A flag which determines whether a menu item is
//     enabled or not.

void MenuItem::setEnabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    bool enabled = true;
    
    if (env->getParamCount() > 0 &&
        env->getParam(0)->getBoolean() == false)
    {
        enabled = false;
    }

    if (m_ctrl)
        m_ctrl->Enable(enabled);
}

// (METHOD) MenuItem.setLabel
// Description: Sets the label for a menu item.
//
// Syntax: function MenuItem.setLabel(label : String)
//
// Remarks: Sets the menu item text, specified by |label|.
//
// Param(label): The new text for the menu item.

void MenuItem::setLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_label = env->getParam(0)->getString();

    if (m_ctrl)
        m_ctrl->SetItemLabel(m_label);
}

// (METHOD) MenuItem.getLabel
// Description: Returns the text of the menu item.
//
// Syntax: function MenuItem.getLabel() : String
//
// Remarks: Returns the text of the menu item.
//
// Returns: The text of the menu item.

void MenuItem::getLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_label));
}

// (METHOD) MenuItem.setHelpString
// Description: Sets the help string for the menu item.
//
// Syntax: function MenuItem.setHelpString(help : String)
//
// Remarks: Sets the menu item help string specified by |help|.
//
// Param(help): The help string associated with the menu item.

void MenuItem::setHelpString(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_help_str = env->getParam(0)->getString();

    if (m_ctrl)
        m_ctrl->SetHelp(m_help_str);
}

// (METHOD) MenuItem.getHelpString
// Description: Returns the help string associated with the menu item.
//
// Syntax: function MenuItem.getHelpString() : String
//
// Remarks: Returns the help string associated with the menu item.
//
// Returns: The help string associated with the menu item.

void MenuItem::getHelpString(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_help_str));
}




// zMenu class implementation

// (CLASS) Menu
// Category: Form
// Description: A class that represents a menu.
// Remarks: The Menu class represents a menu, which is a vertical list of menu items.

zMenu::zMenu()
{
    m_id = -1;
    m_enabled = true;
    m_ctrl = NULL;
    m_owner = NULL;
}

zMenu::~zMenu()
{

}

const int zMenu::getId()
{
    return m_id;
}

const wxString zMenu::getTitle()
{
    return m_title;
}

wxMenu* zMenu::getWxMenu()
{
    return m_ctrl;
}

void zMenu::setWxMenu(wxMenu* menu)
{
    m_ctrl = menu;
}

void zMenu::setMenuOwner(kscript::ValueObject* owner)
{
    m_owner = owner;
}

// (CONSTRUCTOR) Menu.constructor
// Description: Creates a new menu.
//
// Syntax: Menu()
// Syntax: Menu(title : String)
//
// Remarks: Creates a new menu item having text specified by |title|.
//
// Param(title): The title of the menu.

void zMenu::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // if a title was supplied, use it
    m_title = env->getParam(0)->getString();
    
    // set the ID for this menu
    m_id = getUniqueScriptCommandId();
    
    // create the control
    m_ctrl = new wxMenu;
}

size_t zMenu::getJsMenuItemCount()
{
    return m_ctrl->GetMenuItemCount();
}

MenuItemInfo zMenu::getJsMenuItemObject(int idx)
{
    if (!m_ctrl)
        return MenuItemInfo();
    
    size_t item_count = getJsMenuItemCount();

    // if the requested index is out of range
    // return NULL
    if (idx < 0 || (size_t)idx >= item_count)
        return MenuItemInfo();
    
    wxMenuItem* real_wxitem = m_ctrl->FindItemByPosition((size_t)idx);
    wxMenu* real_wxsubmenu = real_wxitem->GetSubMenu();


    if ((size_t)idx < m_items.size() && m_items[idx].isOk())
    {
        // double check to make sure we have
        // the right wxMenuItem object
                        
        if (real_wxsubmenu != NULL &&
            m_items[idx].submenu != NULL &&
            real_wxsubmenu == m_items[idx].submenu->getWxMenu())
        {
            return m_items[idx];
        }
        
        if (real_wxitem != NULL &&
            m_items[idx].item != NULL &&
            real_wxitem == m_items[idx].item->getWxMenuItem())
        {
            return m_items[idx];
        }
    
        // menu item was changed by someone else; dispose of the
        // old zMenu and/or MenuItem object and make a fresh one
        
        if (m_items[idx].item != NULL)
        {
            m_items[idx].item->baseUnref();
            m_items[idx].item = NULL;
        }
        
        if (m_items[idx].submenu != NULL)
        {
            m_items[idx].submenu->baseUnref();
            m_items[idx].submenu = NULL;
        }
        
        // continue on
    }

    if (item_count != m_items.size())
        m_items.resize(item_count);
    
    MenuItemInfo new_item_info;
    
    if (real_wxsubmenu)
    {
        zMenu* new_submenu = zMenu::createObject(getParser());
        new_submenu->baseRef();
        new_submenu->setMenuOwner((kscript::ValueObject*)(this));
        new_submenu->setWxMenu(real_wxsubmenu);

        new_item_info.type = MenuItemInfo::typeSubmenu;
        new_item_info.submenu = new_submenu;
    }
     else
    {
        MenuItem* new_item = MenuItem::createObject(getParser());
        new_item->baseRef();
        new_item->setWxMenuItem(real_wxitem);
        new_item->setWxMenu(m_ctrl);
        
        new_item_info.type = MenuItemInfo::typeItem;
        new_item_info.item = new_item;
    }
    
    m_items[idx] = new_item_info;
    return m_items[idx];
}

void zMenu::insertObjectIntoMenu(size_t pos, kscript::ValueObject* _obj)
{
    // get the menu item count before we insert the item
    size_t item_count = getJsMenuItemCount();

    // make sure we take into account numbers
    // greater than the number of items in the menu
    if (pos > item_count || pos == -1)
        pos = item_count;
        
    kscript::ValueObject* obj = (kscript::ValueObject*)_obj;
    
    MenuItemInfo info;
    
    if (obj->getClassName() == L"Menu")
    {
        zMenu* menu = (zMenu*)obj;
        menu->setMenuOwner((kscript::ValueObject*)(this));
        menu->baseRef();
        
        m_ctrl->Insert(pos,
                       menu->getId(),
                       menu->getTitle(),
                       menu->getWxMenu());

        info.type = MenuItemInfo::typeSubmenu;
        info.submenu = menu;
        
        item_count++;
        
        // dynamically resize vector so we insert at the right location
        if (item_count != m_items.size())
            m_items.resize(item_count);
        
        m_items[pos] = info;
    }
     else if (obj->getClassName() == L"MenuItem")
    {
        MenuItem* item = (MenuItem*)obj;
        item->setWxMenu(m_ctrl);
        item->baseRef();
        
        if (item->getWxMenuItem()->IsSeparator())
        {
            m_ctrl->InsertSeparator(pos);

            info.type = MenuItemInfo::typeSeparator;
        }
         else
        {
            m_ctrl->Insert(pos, item->getWxMenuItem());
        
            info.type = MenuItemInfo::typeItem;
            info.item = item;
        }
        
        item_count++;
        
        // dynamically resize vector so we insert at the right location
        if (item_count != m_items.size())
            m_items.resize(item_count);
        
        m_items[pos] = info;
    }
}


// (METHOD) Menu.add
// Description: Adds a menu or a menu item to a menu.
//
// Syntax: function Menu.add(item : Menu)
// Syntax: function Menu.add(item : MenuItem)
//
// Remarks: Adds a menu or menu item specified by |item| to 
//     the menu.
//
// Param(item): The menu or menu item to add to the menu.

// (METHOD) Menu.insert
// Description: Inserts a menu or menu item into a menu.
//
// Syntax: function Menu.insert(item : Menu, 
//                              index : Integer)
// Syntax: function Menu.insert(item : MenuItem, 
//                              index : Integer)
//
// Remarks: Inserts a menu or menu item specified by |item| into
//     the menu at a given |index|.  If no |index| is specified,
//     the menu or menu item is inserted at the end of the menu.
//
// Param(item): The menu or menu item to insert into the menu.
// Param(index): The location where |item| is inserted.

void zMenu::insert(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;
    
    int idx;
    if (env->getParamCount() > 1)
    {
        idx = env->getParam(1)->getInteger();
        if (idx < 0)
            idx = 0;
    }
     else
    {
        idx = (int)getJsMenuItemCount();
    }

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    insertObjectIntoMenu(idx, obj);
}


// (METHOD) Menu.addSeparator
// Description: Adds a separator to a menu.
//
// Syntax: function Menu.addSeparator()
//
// Remarks: Adds a separator to the menu.

// (METHOD) Menu.insertSeparator(index : Integer)
// Description: Inserts a separator into a menu.
//
// Syntax: function Menu.insert(item : Menu, 
//                              index : Integer)
// Syntax: function Menu.insert(item : MenuItem, 
//                              index : Integer)
//
// Remarks: Inserts a separator into the menu at a given |index|.
//     If no |index| is specified, the separator is inserted
//     at the end of the menu.
//
// Param(index): The location where the separator is inserted.

void zMenu::insertSeparator(kscript::ExprEnv* env, kscript::Value* retval)
{
    int idx;
    if (env->getParamCount() > 0)
    {
        idx = env->getParam(0)->getInteger();
        if (idx < 0)
            idx = 0;
    }
     else
    {
        idx = (int)getJsMenuItemCount();
    }
    
    MenuItem* item = MenuItem::createObject(env);
    item->setId(wxID_SEPARATOR);
    
    insertObjectIntoMenu(idx, item);
}

// (METHOD) Menu.remove
// Description: Removes a Menu or MenuItem from the menu.
//
// Syntax: function Menu.remove(index : Integer)
// Syntax: function Menu.remove(item : Menu)
// Syntax: function Menu.remove(item : MenuItem)
//
// Remarks: Removes a menu or a menu item from the menu.  If the
//     input is an integer, then the item at the position specified
//     by |index| is removed.  If the input is a menu or menu item,
//     then the item is removed from the menu, wherever it is located.
//
// Param(index): The location of the item to remove.
// Param(item): The menu or menu item to remove.

void zMenu::remove(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;
    
    int idx = -1;
    
    // if they passed an object, find the index of that
    // object in our m_items array, so we can remove it down below
    if (env->getParam(0)->isObject())
    {
        std::wstring class_name = env->getParam(0)->getObject()->getClassName();
        if (class_name == L"Menu" || class_name == L"MenuItem")
        {
            kscript::ValueObject* obj = env->getParam(0)->getObject();
            
            int found_idx = 0;
            size_t i, count = getJsMenuItemCount();
            for (i = 0; i < count; ++i)
            {
                MenuItemInfo info = getJsMenuItemObject(i);
                if ((zMenu*)obj == info.submenu || (MenuItem*)obj == info.item)
                {
                    idx = found_idx;
                    break;
                }
                found_idx++;
            }
        }
    }

    if (idx == -1)
    {
        // an index was passed to us as a parameter
        idx = env->getParam(0)->getInteger();
    }

    
    // remove the object
    if (idx < 0 || (size_t)idx >= m_items.size())
        return;
        
    MenuItemInfo& info = m_items[idx];
    if (info.submenu)
    {
        info.submenu->baseUnref();
        info.submenu = NULL;
    }
    
    if (info.item)
    {
        info.item->baseUnref();
        info.item = NULL;
    }
        
    m_items.erase(m_items.begin() + idx);
    
    m_ctrl->Remove(m_ctrl->FindItemByPosition(idx));
}


// (METHOD) Menu.setEnabled
// Description: Enables or disables a menu.
//
// Syntax: function Menu.setEnabled()
// Syntax: function Menu.setEnabled(enabled : Boolean)
//
// Remarks: Enables or disables a menu.  If |enabled| isn't
//     specified, the menu is enabled; otherwise the menu is
//     enabled if |enabled| is true and disabled if |enabled| 
//     is false.
//
// Param(enabled): A flag which determines whether a menu is
//     enabled or not.

void zMenu::setEnabled(kscript::ExprEnv* env, kscript::Value* retval)
{
//  TODO: needs to be implemented
/*
    m_enabled = true;
    
    if (env->getParamCount() > 0 &&
        env->getParam(0)->getBoolean() == false)
    {
        m_enabled = false;
    }

    // if the menu hasn't been added to a menubar or another menu yet,
    // we can't officially make it enabled or disabled

    if (!m_owner)
        return;
        
    if (m_owner->getClassName() == L"MenuBar")
    {
        MenuBar* mb = (MenuBar*)m_owner;
        int pos = mb->getMenuPositionFromId(m_id);
        
        // the menu was not found
        if (pos == -1)
            return;
            
        mb->getWxMenuBar()->EnableTop(pos, m_enabled);
    }
     else if (m_owner->getClassName() == L"Menu")
    {
        zMenu* m = (zMenu*)m_owner;
        m->getWxMenu()->Enable(m_id, m_enabled);
    }
    */
}

// (METHOD) Menu.setLabel
// Description: Sets the label for a menu.
//
// Syntax: function Menu.setLabel(label : String)
//
// Remarks: Sets the menu text specified by |label|.
//
// Param(label): The new next for the menu.

void zMenu::setLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxString label = env->getParam(0)->getString();
    m_ctrl->SetTitle(label);
}

// (METHOD) Menu.findMenuItem
// Description: Finds the location of a submenu or menu item in the menu.
//
// Syntax: function Menu.findMenuItem(caption : String) : Integer
//
// Remarks: Finds the location of a submenu or menu item with a particular
//     |caption| in the menu and returns the location where the submenu or
//     menu item is located.  If a submenu or menu item with the |caption|
//     is not found, then the function returns -1.
//
// Param(caption): The text of the submenu or menu item to find.
// Returns: The zero-based index of the submenu or menu item with 
//     a particular |caption| or -1 if a submenu or menu item with
//     a |caption| does not exist.

void zMenu::findMenuItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setInteger(-1);
        return;
    }
    
    wxString label = env->getParam(0)->getString();
    if (label.IsEmpty())
    {
        retval->setInteger(-1);
        return;
    }
    
    // find the submenu or menu item based on its label
    size_t i, count = getJsMenuItemCount();
    for (i = 0; i < count; ++i)
    {
        MenuItemInfo info = getJsMenuItemObject(i);
        if (info.type == MenuItemInfo::typeItem)
        {
            if (label.CmpNoCase(info.item->getWxLabel()) == 0)
            {
                retval->setInteger(i);
                return;
            }
        }
         else if (info.type == MenuItemInfo::typeSubmenu)
        {
            if (label.CmpNoCase(info.submenu->getTitle()) == 0)
            {
                retval->setInteger(i);
                return;
            }
        }
    }
    
    // the submenu or menu item with this label could not be found
    retval->setInteger(-1);
}

// (METHOD) Menu.getMenuItem
// Description: Returns a menu or menu item that is in the menu.
//
// Syntax: function Menu.getMenuItem(index : Integer) : MenuItem
// Syntax: function Menu.getMenuItem(index : Integer) : Menu
//
// Remarks: Returns the submenu or menu item at the specified |index|.
//
// Returns: The menu or menu item at the specified zero-based |index|
//     in the menu.  If |index| is less than 0, greater than the number
//     of menu items in the menu or is unspecified, the function returns null.

void zMenu::getMenuItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
    
    int idx = env->getParam(0)->getInteger();

    if (idx < 0 || (size_t)idx >= getJsMenuItemCount())
    {
        retval->setNull();
        return;
    }

    MenuItemInfo info = getJsMenuItemObject(idx);
    if (info.type == MenuItemInfo::typeItem && info.item != NULL)
    {
        retval->setObject(info.item);
        return;
    }
     else if (info.type == MenuItemInfo::typeSubmenu && info.submenu != NULL)
    {
        retval->setObject(info.submenu);
        return;
    }
    
    retval->setNull();
}

// (METHOD) MenuBar.getMenuItems
// Description: Returns an array of menus and menu items that are in the menu.
//
// Syntax: function MenuBar.getMenuItems() : Array(Menu/MenuItem)
//
// Remarks: Returns an array of menus and menu items that are in the menu.
//
// Returns: An array of menus and menu items that are in the menu.

void zMenu::getMenuItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setArray(env);

    size_t i, menu_count = getJsMenuItemCount();
    for (i = 0; i < menu_count; ++i)
    {
        MenuItemInfo info = getJsMenuItemObject(i);

        if (info.isOk())
        {
            kscript::Value val;
            
            // set each object in the return array
            if (info.type == MenuItemInfo::typeSubmenu)
                val.setObject(info.submenu);
                 else
                val.setObject(info.item);
            
            retval->appendMember(&val);
        }
    }
}

// (METHOD) Menu.getMenuItemCount
// Description: Returns the number of menu items in the menu.
//
// Syntax: function Menu.getMenuItemCount() : Integer
//
// Remarks: Returns the number of submenus and menu items in the menu.
//
// Returns: The number of submenus and menu items in the menu.

void zMenu::getMenuItemCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_ctrl)
    {
        retval->setInteger(0);
        return;
    }

        
    retval->setInteger(m_ctrl->GetMenuItemCount());
}

// (METHOD) Menu.getLabel
// Description: Returns the text of the menu.
//
// Syntax: function Menu.getLabel() : String
//
// Remarks: Returns the text of the menu.
//
// Returns: The text of the menu.

void zMenu::getLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxString label = m_ctrl->GetTitle();
    retval->setString(label);
}

// (METHOD) Menu.popup
// Description: Creates a menu at a given position on a form.
//
// Syntax: function Menu.popup(form : Form,
//                             point : Point)
// 
// Remarks: Creates a menu on a |form| at at a given |point|.
//
// Param(form): The Form object on which the menu is created.
// Param(point): The position where the menu is created.

void zMenu::popup(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    
    if (env->getParamCount() == 0)
        return;
    
    
    if (!env->getParam(0)->isObject())
        return;
    
    wxWindow* wnd = NULL;
    
    // get the wxWindow* from the Form object
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->isKindOf(L"FormControl"))
    {
        FormControl* f = (FormControl*)obj;
        wnd = f->getWindow();
    }

    if (!wnd)
        return;

    // if a Point parameter was passed, get its x and y coordinates
    // and do the popup at those coordinates
    if (env->getParamCount() > 1)
    {
        int x = -1, y = -1;
    
        obj = env->getParam(1)->getObject();
        if (obj->isKindOf(L"Point"))
        {
            x = obj->getMember(L"x")->getInteger();
            y = obj->getMember(L"y")->getInteger();
        }
        
        if (x >= 0 && y >= 0)
            wnd->PopupMenu(m_ctrl, x, y);
             else
            wnd->PopupMenu(m_ctrl);
    }
     else
    {
        wnd->PopupMenu(m_ctrl);
    }
}




// MenuBar class implementation

// (CLASS) MenuBar
// Category: Form
// Description: A class that represents a menu bar.
// Remarks: The MenuBar class represents a menu bar.

MenuBar::MenuBar()
{
    m_ctrl = NULL;
}

MenuBar::~MenuBar()
{
    clear();
}

void MenuBar::clear()
{
    std::vector<zMenu*>::iterator it;
    for (it = m_menus.begin(); it != m_menus.end(); ++it)
    {
        if (*it)
        {
            (*it)->baseUnref();
        }
    }
    
    m_menus.clear();
}

wxMenuBar* MenuBar::getWxMenuBar()
{
    return m_ctrl;
}

void MenuBar::setWxMenuBar(wxMenuBar* menu)
{
    if (m_ctrl != NULL)
        delete m_ctrl;
        
    m_ctrl = menu;
    
    clear();
    m_menus.resize(m_ctrl->GetMenuCount(), NULL);
}

zMenu* MenuBar::getJsMenuObject(kscript::ExprEnv* env, int idx)
{
    if (!m_ctrl)
        return NULL;
    
    size_t menu_count = m_ctrl->GetMenuCount();

    // if the requested index is out of range
    // return NULL
    if (idx < 0 || (size_t)idx >= menu_count)
        return NULL;
    
    wxMenu* real_wxsubmenu = m_ctrl->GetMenu((size_t)idx);


    if ((size_t)idx < m_menus.size() && m_menus[idx])
    {
        // double check to make sure we have
        // the right wxMenu object
        
        if (real_wxsubmenu == m_menus[idx]->getWxMenu())
        {
            return m_menus[idx];
        }
         else
        {
            // menu bar was changed by someone else;
            // dispose of the old zMenu object and make a fresh one
            
            m_menus[idx]->baseUnref();
            m_menus[idx] = NULL;
            
            // continue on
        }
    }

    if (menu_count != m_menus.size())
        m_menus.resize(menu_count);
    
    zMenu* menu = zMenu::createObject(env);
    menu->baseRef();
    menu->setMenuOwner((kscript::ValueObject*)(this));
    menu->setWxMenu(real_wxsubmenu);
    m_menus[idx] = menu;
    
    return menu;
}

// (CONSTRUCTOR) MenuBar.constructor
// Description: Constructs a new menu bar.
//
// Syntax: MenuBar()
//
// Remarks: Constructs a new menu bar.

void MenuBar::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    m_ctrl = new wxMenuBar;
}

// (METHOD) MenuBar.add
// Description: Adds a menu to a menu bar.
//
// Syntax: function MenuBar.add(item : Menu)
//
// Remarks: Adds a menu bar specified by |item| to the menu.
//
// Param(item): The menu to add to the menu bar.

// (METHOD) MenuBar.insert
// Description: Inserts a menu into a menu bar.
//
// Syntax: function MenuBar.insert(item : Menu, 
//                                 index : Integer)
//
// Remarks: Inserts a menu specified by |item| into the menu bar
//     at a given |index|.  If no |index| is specified,
//     the menu or menu item is inserted at the end of the menu.
//
// Param(item): The menu to insert into the menu bar.
// Param(index): The location where |item| is inserted.


void MenuBar::insert(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    if (obj->getClassName() == L"Menu")
    {
        zMenu* menu = (zMenu*)obj;
        menu->setMenuOwner((kscript::ValueObject*)(this));
        
        int menu_count = m_ctrl->GetMenuCount();
        int idx;
        if (env->getParamCount() > 1)
            idx = env->getParam(1)->getInteger();
             else
            idx = menu_count;
        
        if (idx < 0)
            idx = 0;
            
        menu->baseRef();
        
        if (idx >= menu_count)
        {
            m_ctrl->Append(menu->getWxMenu(), menu->getTitle());
            m_menus.push_back(menu);
        }
         else
        {
            m_ctrl->Insert((size_t)idx, menu->getWxMenu(), menu->getTitle());
            m_menus.insert(m_menus.begin() + idx, menu);
        }
    }
}

// (METHOD) MenuBar.findMenu
// Description: Finds the location of a menu in the menu bar.
//
// Syntax: function MenuBar.findMenu(caption : String) : Integer
//
// Remarks: Finds the location of a menu with a particular |caption|
//     in the menu bar and returns the location where the menu is
//     located.  If a menu with the |caption| is not found, then
//     the function returns -1.
//
// Param(caption): The text of the menu to find.
// Returns: The zero-based index of the menu with a particular |caption| 
//     or -1 if a menu with a |caption| does not exist.

void MenuBar::findMenu(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    if (env->getParamCount() < 1)
    {
        retval->setInteger(-1);
        return;
    }
    
    int res = m_ctrl->FindMenu(env->getParam(0)->getString());
    
    if (res == wxNOT_FOUND)
        retval->setInteger(-1);
         else
        retval->setInteger(res);
}

// (METHOD) MenuBar.getMenu
// Description: Returns a menu in the menu bar.
//
// Syntax: function MenuBar.getMenu(index : Integer) : Menu
//
// Remarks: Returns the menu in the menu bar at the specified |index|.
//
// Returns: The menu at the specified zero-based |index| in the menu bar.
//     If |index| is less than 0, greater than the number of menus in the
//     menu bar or is unspecified, the function returns null.

void MenuBar::getMenu(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setNull();
        return;
    }
    
    int idx = env->getParam(0)->getInteger();

    if (idx < 0 || (size_t)idx >= m_menus.size())
    {
        retval->setNull();
        return;
    }

    zMenu* menu = getJsMenuObject(env, idx);
    retval->setObject(m_menus[idx]);
}

// (METHOD) MenuBar.getMenus
// Description: Returns an array of menus that are in the menu bar.
//
// Syntax: function MenuBar.getMenus() : Array(Menu)
//
// Remarks: Returns an array of menus that are in the menu bar.
//
// Returns: An array of menus that are in the menu bar.

void MenuBar::getMenus(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setArray(env);

    size_t i, menu_count = m_menus.size();
    for (i = 0; i < menu_count; ++i)
    {
        // set each object in the return array
        kscript::Value val;
        val.setObject(getJsMenuObject(env, i));
        retval->appendMember(&val);
    }
}

// (METHOD) MenuBar.getMenuCount
// Description: Returns the number of menus in the menu bar.
//
// Syntax: function MenuBar.getMenuCount() : Integer
//
// Remarks: Returns the number of menus in the menu bar.
//
// Returns: The number of menus in the menu bar.

void MenuBar::getMenuCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_ctrl)
    {
        retval->setInteger(0);
        return;
    }
        
    retval->setInteger(m_ctrl->GetMenuCount());
}


// (METHOD) MenuBar.remove
//
// Description: Removes a menu from the menu bar.
//
// Syntax: function MenuBar.remove(item : Menu) : Boolean
// Syntax: function MenuBar.remove(index : Integer) : Boolean
//
// Remarks: Removes a menu from the menu bar that is explicitly
//     denoted by |item| or else by an position given by |index|.
//     The function returns true if the menu is removed, false otherwise.
//
// Returns: Returns true if the menu is removed, and false otherwise.

void MenuBar::remove(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    int idx = -1;
    
    // if they passed an object, find the index of that
    // object in our m_items array, so we can remove it down below
    if (env->getParam(0)->isObject())
    {
        std::wstring class_name = env->getParam(0)->getObject()->getClassName();
        if (class_name == L"Menu")
        {
            kscript::ValueObject* obj = env->getParam(0)->getObject();
            
            int found_idx = 0;
            std::vector<zMenu*>::iterator it;
            for (it = m_menus.begin(); it != m_menus.end(); ++it)
            {
                if ((zMenu*)obj == *it)
                {
                    idx = found_idx;
                    break;
                }
                found_idx++;
            }
        }
    }

    if (idx == -1)
    {
        // an index was passed to us as a parameter
        idx = env->getParam(0)->getInteger();
    }

    
    // remove the object
    if (idx < 0 || (size_t)idx >= m_menus.size())
    {
        retval->setBoolean(false);
        return;
    }
        
    zMenu* menu = m_menus[idx];
    if (menu)
    {
        menu->baseUnref();
    }
    
    m_ctrl->Remove(idx);
    m_menus.erase(m_menus.begin() + idx);
    
    retval->setBoolean(true);
}





/*
MenuItem* MenuBar::getMenuItemFromId(int id)
{
    // look for the selected menuitem in the menubar's menus
    std::vector<zMenu*>::iterator it;
    for (it = m_menus.begin(); it != m_menus.end(); ++it)
    {
        MenuItem* mi = (*it)->getMenuItemFromId(id);
        if (mi)
            return mi;
    }
    
    return NULL;
}

int MenuBar::getMenuPositionFromId(int id)
{
    int pos = 0;

    std::vector<zMenu*>::iterator it;
    for (it = m_menus.begin(); it != m_menus.end(); ++it)
    {
        if ((*it)->getId() == id)
            return pos;
            
        pos++;
    }
    
    return -1;
}
*/

