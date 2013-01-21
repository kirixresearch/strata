/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-05
 *
 */


#ifndef __APP_SCRIPTLAYOUT_H
#define __APP_SCRIPTLAYOUT_H


class Layout;
class FormComponent;


class LayoutItem
{
public:

    LayoutItem();
    ~LayoutItem();

public:

    bool m_added;
    bool m_realized;
    Layout* m_layout;
    FormComponent* m_component;
    int m_spacer_x;
    int m_spacer_y;
    int m_proportion;
    int m_border;
    int m_flags;
};




class Layout : public ScriptHostBase
{    
public:

    BEGIN_KSCRIPT_CLASS("Layout", Layout)

        KSCRIPT_GUI_METHOD("constructor", Layout::constructor)
        KSCRIPT_GUI_METHOD("show", Layout::show)
        KSCRIPT_GUI_METHOD("layout", Layout::layout)
        
        KSCRIPT_CONSTANT_INTEGER("Horizontal",  Horizontal)
        KSCRIPT_CONSTANT_INTEGER("Vertical",    Vertical)
        KSCRIPT_CONSTANT_INTEGER("Top",         Top)
        KSCRIPT_CONSTANT_INTEGER("Bottom",      Bottom)
        KSCRIPT_CONSTANT_INTEGER("Left",        Left)
        KSCRIPT_CONSTANT_INTEGER("Right",       Right)
        KSCRIPT_CONSTANT_INTEGER("All",         All)
        KSCRIPT_CONSTANT_INTEGER("Expand",      Expand)
        KSCRIPT_CONSTANT_INTEGER("AlignCenter", AlignCenter) // DEPRECATED - don't document
        KSCRIPT_CONSTANT_INTEGER("Center",      Center)
        KSCRIPT_CONSTANT_INTEGER("Shaped",      Shaped)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Horizontal = wxHORIZONTAL,
        Vertical = wxVERTICAL,
        Top = wxTOP,
        Bottom = wxBOTTOM,
        Left = wxLEFT,
        Right = wxRIGHT,
        All = wxALL,
        Expand = wxEXPAND,
        AlignCenter = wxALIGN_CENTER, /* DEPRECATED */
        Center = wxALIGN_CENTER,
        Shaped = wxSHAPED
    };

public:

    Layout();
    ~Layout();

    void setFormWindow(wxWindow* wnd);
    wxSizer* getSizer();

    virtual void realize() { }

    void add(LayoutItem* item);
    void insert(LayoutItem* item, size_t idx);
    void clear();
    
    void deinitializeControl();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void show(kscript::ExprEnv* env, kscript::Value* retval);
    void layout(kscript::ExprEnv* env, kscript::Value* retval);

protected:

    void realizeChildren();
    int getIndex(kscript::ValueObject* object);

protected:

    std::vector<LayoutItem*> m_items;
    wxWindow* m_form_wnd;
    wxSizer* m_sizer;
};



class BoxLayout : public Layout
{
    BEGIN_KSCRIPT_DERIVED_CLASS("BoxLayout", BoxLayout, Layout)
        KSCRIPT_GUI_METHOD("constructor", BoxLayout::constructor)
        KSCRIPT_GUI_METHOD("add", BoxLayout::add)
        KSCRIPT_GUI_METHOD("addSpacer", BoxLayout::addSpacer)
        KSCRIPT_GUI_METHOD("addStretchSpacer", BoxLayout::addStretchSpacer)
        KSCRIPT_GUI_METHOD("insert", BoxLayout::insert)
        KSCRIPT_GUI_METHOD("insertSpacer", BoxLayout::insertSpacer)
        KSCRIPT_GUI_METHOD("insertStretchSpacer", BoxLayout::insertStretchSpacer)
        KSCRIPT_GUI_METHOD("clear", BoxLayout::clear)
    END_KSCRIPT_CLASS()

public:

    BoxLayout();
    ~BoxLayout();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void add(kscript::ExprEnv* env, kscript::Value* retval);
    void addSpacer(kscript::ExprEnv* env, kscript::Value* retval);
    void addStretchSpacer(kscript::ExprEnv* env, kscript::Value* retval);
    void insert(kscript::ExprEnv* env, kscript::Value* retval);
    void insertSpacer(kscript::ExprEnv* env, kscript::Value* retval);
    void insertStretchSpacer(kscript::ExprEnv* env, kscript::Value* retval);
    void clear(kscript::ExprEnv* env, kscript::Value* retval);

protected:

    int m_orientation;
};




class BorderBoxLayout : public BoxLayout
{
    BEGIN_KSCRIPT_DERIVED_CLASS("BorderBoxLayout", BorderBoxLayout, BoxLayout)
        KSCRIPT_GUI_METHOD("constructor", BorderBoxLayout::constructor)
    END_KSCRIPT_CLASS()

public:

    BorderBoxLayout();
    ~BorderBoxLayout();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    wxString m_label;
};






#endif

