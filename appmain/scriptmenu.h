/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-06
 *
 */


#ifndef H_APP_SCRIPTMENU_H
#define H_APP_SCRIPTMENU_H


class MenuItem : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("MenuItem", MenuItem)
        KSCRIPT_GUI_METHOD("constructor", MenuItem::constructor)
        KSCRIPT_GUI_METHOD("setEnabled", MenuItem::setEnabled)
        KSCRIPT_GUI_METHOD("setLabel", MenuItem::setLabel)
        KSCRIPT_GUI_METHOD("getLabel", MenuItem::getLabel)
        KSCRIPT_GUI_METHOD("setHelpString", MenuItem::setHelpString)
        KSCRIPT_GUI_METHOD("getHelpString", MenuItem::getHelpString)
    END_KSCRIPT_CLASS()
    
public:

    MenuItem();
    ~MenuItem();
    
    int getId() const;
    wxString getWxLabel() const;
    wxMenuItem* getWxMenuItem() const;

    void setId(int id);
    void setWxMenuItem(wxMenuItem* item);
    void setWxMenu(wxMenu* menu);
    void onEvent(wxEvent& event);
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setEnabled(kscript::ExprEnv* env, kscript::Value* retval);
    void setLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void getLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void setHelpString(kscript::ExprEnv* env, kscript::Value* retval);
    void getHelpString(kscript::ExprEnv* env, kscript::Value* retval);

private:

    int m_id;
    wxString m_label;
    wxString m_help_str;
    wxMenuItem* m_ctrl;
};



class MenuItemInfo
{
public:

    enum
    {
        typeInvalid = 0,
        typeSeparator = 1,
        typeItem = 2,
        typeSubmenu = 3
    };
    
public:

    MenuItemInfo()
    {
        type = typeInvalid;
        item = NULL;
        submenu = NULL;
    }
    
    MenuItemInfo(const MenuItemInfo& c)
    {
        type = c.type;
        item = c.item;
        submenu = c.submenu;
    }
    
    MenuItemInfo& operator=(const MenuItemInfo& c)
    {
        type = c.type;
        item = c.item;
        submenu = c.submenu;
        return *this;
    }

    bool isOk() const
    {
        return (type != typeInvalid) ? true : false;
    }
    
public:

    int type;
    MenuItem* item;
    zMenu* submenu;
};


class zMenu : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("Menu", zMenu)
        KSCRIPT_GUI_METHOD("constructor", zMenu::constructor)
        KSCRIPT_GUI_METHOD("add", zMenu::insert) // (add method is the same as insert)
        KSCRIPT_GUI_METHOD("addSeparator", zMenu::insertSeparator) // (addSeparator method is the same as insertSeparator)
        KSCRIPT_GUI_METHOD("getMenuItem", zMenu::getMenuItem)
        KSCRIPT_GUI_METHOD("getMenuItems", zMenu::getMenuItems)
        KSCRIPT_GUI_METHOD("getMenuItemCount", zMenu::getMenuItemCount)
        KSCRIPT_GUI_METHOD("findMenuItem", zMenu::findMenuItem)
        KSCRIPT_GUI_METHOD("getLabel", zMenu::getLabel)
        KSCRIPT_GUI_METHOD("insert", zMenu::insert)
        KSCRIPT_GUI_METHOD("insertSeparator", zMenu::insertSeparator)
        KSCRIPT_GUI_METHOD("popup", zMenu::popup)        
        KSCRIPT_GUI_METHOD("remove", zMenu::remove)
        KSCRIPT_GUI_METHOD("setEnabled", zMenu::setEnabled)
        KSCRIPT_GUI_METHOD("setLabel", zMenu::setLabel)
    END_KSCRIPT_CLASS()

public:

    zMenu();
    ~zMenu();

    int getId() const;
    wxString getTitle() const;
    
    void setWxMenu(wxMenu* menu);
    wxMenu* getWxMenu();
    
    size_t getJsMenuItemCount();
    MenuItemInfo getJsMenuItemObject(int idx);
    
    void setMenuOwner(kscript::ValueObject* owner);
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getMenuItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getMenuItems(kscript::ExprEnv* env, kscript::Value* retval);
    void getMenuItemCount(kscript::ExprEnv* env, kscript::Value* retval);
    void findMenuItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void insert(kscript::ExprEnv* env, kscript::Value* retval);
    void insertSeparator(kscript::ExprEnv* env, kscript::Value* retval);
    void popup(kscript::ExprEnv* env, kscript::Value* retval);
    void remove(kscript::ExprEnv* env, kscript::Value* retval);
    void setEnabled(kscript::ExprEnv* env, kscript::Value* retval);
    void setLabel(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void insertObjectIntoMenu(size_t pos, kscript::ValueObject* obj);
    
private:

    int m_id;
    bool m_enabled;
    wxString m_title;
    wxMenu* m_ctrl;
    
    // -- the owner can either be a menubar or another menu --
    kscript::ValueObject* m_owner;
    
    std::vector<MenuItemInfo> m_items;
};


class MenuBar : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("MenuBar", MenuBar)
        KSCRIPT_GUI_METHOD("constructor", MenuBar::constructor)
        KSCRIPT_GUI_METHOD("add", MenuBar::insert) // (add method is the same as insert)
        KSCRIPT_GUI_METHOD("getMenu", MenuBar::getMenu)
        KSCRIPT_GUI_METHOD("getMenus", MenuBar::getMenus)
        KSCRIPT_GUI_METHOD("getMenuCount", MenuBar::getMenuCount)
        KSCRIPT_GUI_METHOD("findMenu", MenuBar::findMenu)
        KSCRIPT_GUI_METHOD("insert", MenuBar::insert)
        KSCRIPT_GUI_METHOD("remove", MenuBar::remove)
    END_KSCRIPT_CLASS()
    
public:

    MenuBar();
    ~MenuBar();

    wxMenuBar* getWxMenuBar();
    void setWxMenuBar(wxMenuBar* menu);
    
    void clear();
    zMenu* getJsMenuObject(kscript::ExprEnv* env, int idx);
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getMenu(kscript::ExprEnv* env, kscript::Value* retval);
    void getMenus(kscript::ExprEnv* env, kscript::Value* retval);
    void getMenuCount(kscript::ExprEnv* env, kscript::Value* retval);
    void findMenu(kscript::ExprEnv* env, kscript::Value* retval);
    void insert(kscript::ExprEnv* env, kscript::Value* retval);
    void remove(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    std::vector<zMenu*> m_menus;
    wxMenuBar* m_ctrl;
};

#endif

