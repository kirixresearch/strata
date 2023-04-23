/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-08-03
 *
 */


#ifndef H_APP_SCRIPTSTATUSBAR_H
#define H_APP_SCRIPTSTATUSBAR_H


class zStatusBarItem : public FormComponent
{
friend class zStatusBar;

    BEGIN_KSCRIPT_CLASS("StatusBarItem", zStatusBarItem)
        KSCRIPT_GUI_METHOD("constructor", zStatusBarItem::constructor)
        KSCRIPT_GUI_METHOD("setText", zStatusBarItem::setText)
        KSCRIPT_GUI_METHOD("getText", zStatusBarItem::getText)
        KSCRIPT_GUI_METHOD("setWidth", zStatusBarItem::setWidth)
        KSCRIPT_GUI_METHOD("setStretchProportion", zStatusBarItem::setStretchProportion)
    END_KSCRIPT_CLASS()
    
public:

    zStatusBarItem();
    ~zStatusBarItem();
    
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


class zStatusBar : public FormComponent
{
    BEGIN_KSCRIPT_CLASS("StatusBar", zStatusBar)
        KSCRIPT_GUI_METHOD("constructor", zStatusBar::constructor)
        KSCRIPT_GUI_METHOD("add", zStatusBar::add)
    END_KSCRIPT_CLASS()
    
public:

    zStatusBar();
    ~zStatusBar();
    void realize();
    
    wxStatusBar* getWxStatusBar();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void add(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    std::vector<zStatusBarItem*> m_items;
    wxStatusBar* m_ctrl;
};


#endif

