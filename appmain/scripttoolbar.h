/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-21
 *
 */


#ifndef __APP_SCRIPTTOOLBAR_H
#define __APP_SCRIPTTOOLBAR_H


class ToolBarItem : public ScriptHostBase
{
friend class ToolBar;

    BEGIN_KSCRIPT_CLASS("ToolBarItem", ToolBarItem)
        KSCRIPT_GUI_METHOD("constructor", ToolBarItem::constructor)
        KSCRIPT_GUI_METHOD("setEnabled", ToolBarItem::setEnabled)
        KSCRIPT_GUI_METHOD("setLabel", ToolBarItem::setLabel)
        KSCRIPT_GUI_METHOD("getLabel", ToolBarItem::getLabel)
        KSCRIPT_GUI_METHOD("setTooltip", ToolBarItem::setTooltip)
        KSCRIPT_GUI_METHOD("getTooltip", ToolBarItem::getTooltip)
        KSCRIPT_GUI_METHOD("setHelpString", ToolBarItem::setHelpString)
        KSCRIPT_GUI_METHOD("getHelpString", ToolBarItem::getHelpString)
        KSCRIPT_GUI_METHOD("setBitmap", ToolBarItem::setBitmap)
        KSCRIPT_GUI_METHOD("setDisabledBitmap", ToolBarItem::setDisabledBitmap)
    END_KSCRIPT_CLASS()
    
public:

    ToolBarItem();
    ~ToolBarItem();
    
    const int getId();
    void setWxToolBar(wxToolBar* toolbar);
    void onEvent(wxEvent& event);
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setEnabled(kscript::ExprEnv* env, kscript::Value* retval);
    void setLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void getLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void setTooltip(kscript::ExprEnv* env, kscript::Value* retval);
    void getTooltip(kscript::ExprEnv* env, kscript::Value* retval);
    void setHelpString(kscript::ExprEnv* env, kscript::Value* retval);
    void getHelpString(kscript::ExprEnv* env, kscript::Value* retval);
    void setBitmap(kscript::ExprEnv* env, kscript::Value* retval);
    void getBitmap(kscript::ExprEnv* env, kscript::Value* retval);
    void setDisabledBitmap(kscript::ExprEnv* env, kscript::Value* retval);
    void getDisabledBitmap(kscript::ExprEnv* env, kscript::Value* retval);

private:

    int m_id;
    bool m_enabled;
    wxString m_label;
    wxString m_tooltip;
    wxString m_help_str;
    wxBitmap m_bitmap;
    wxBitmap m_disabled_bitmap;
    
    bool m_separator; // this is set to true if the item is a separator
    
    wxToolBar* m_owner;
};


class ToolBar : public FormComponent
{
    BEGIN_KSCRIPT_CLASS("ToolBar", ToolBar)
        KSCRIPT_GUI_METHOD("constructor", ToolBar::constructor)
        KSCRIPT_GUI_METHOD("add", ToolBar::add)
        KSCRIPT_GUI_METHOD("addSeparator", ToolBar::addSeparator)
    END_KSCRIPT_CLASS()
    
public:

    ToolBar();
    ~ToolBar();
    void realize();
    
    wxToolBar* getWxToolBar();
    ToolBarItem* getToolBarItemFromId(int id);
    void addItemToWxToolBar(ToolBarItem* item);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void add(kscript::ExprEnv* env, kscript::Value* retval);
    void addSeparator(kscript::ExprEnv* env, kscript::Value* retval);

private:

    std::vector<ToolBarItem*> m_items;
    wxToolBar* m_ctrl;
    bool m_realized;
};


#endif


