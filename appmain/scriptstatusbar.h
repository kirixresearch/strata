/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-08-03
 *
 */


#ifndef __APP_SCRIPTSTATUSBAR_H
#define __APP_SCRIPTSTATUSBAR_H


class StatusBarItem : public FormComponent
{
friend class StatusBar;

    BEGIN_KSCRIPT_CLASS("StatusBarItem", StatusBarItem)
        KSCRIPT_GUI_METHOD("constructor", StatusBarItem::constructor)
        KSCRIPT_GUI_METHOD("setText", StatusBarItem::setText)
        KSCRIPT_GUI_METHOD("getText", StatusBarItem::getText)
        KSCRIPT_GUI_METHOD("setWidth", StatusBarItem::setWidth)
        KSCRIPT_GUI_METHOD("setStretchProportion", StatusBarItem::setStretchProportion)
    END_KSCRIPT_CLASS()
    
public:

    StatusBarItem();
    ~StatusBarItem();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setText(kscript::ExprEnv* env, kscript::Value* retval);
    void getText(kscript::ExprEnv* env, kscript::Value* retval);
    void setWidth(kscript::ExprEnv* env, kscript::Value* retval);
    void setStretchProportion(kscript::ExprEnv* env, kscript::Value* retval);

private:

    wxStatusBar* m_owner;
    
    wxString m_text;
    int m_width;
    int m_idx;
};


class StatusBar : public FormComponent
{
    BEGIN_KSCRIPT_CLASS("StatusBar", StatusBar)
        KSCRIPT_GUI_METHOD("constructor", StatusBar::constructor)
        KSCRIPT_GUI_METHOD("add", StatusBar::add)
    END_KSCRIPT_CLASS()
    
public:

    StatusBar();
    ~StatusBar();
    void realize();
    
    wxStatusBar* getWxStatusBar();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void add(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    std::vector<StatusBarItem*> m_items;
    wxStatusBar* m_ctrl;
};


#endif

