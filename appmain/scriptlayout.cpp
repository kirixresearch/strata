/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-05
 *
 */


#include "appmain.h"
#include "scriptgui.h"
#include "scriptlayout.h"


// -- LayoutItem class implementation --

// note: even though LayoutItem has a binding, it's not used on the
// script interface for the Layout classes, so don't document this
// for now; however, this may change if we expose it so it as a
// return item to allow the item.expand().center() style calls.
LayoutItem::LayoutItem()
{
    m_added = false;
    m_realized = false;
    m_layout = NULL;
    m_component = NULL;
    m_spacer_x = 0;
    m_spacer_y = 0;
    m_proportion = 0;
    m_border = 0;
    m_flags = 0;
}

LayoutItem::~LayoutItem()
{
    if (m_layout)
    {
        m_layout->baseUnref();
    }
    
    if (m_component)
    {
        m_component->baseUnref();
    }
}

    
// -- Layout class implementation --

// (CLASS) Layout
// Category: Layout
// Description: A class that allows controls to be positioned and sized
//     automatically.
// Remarks: Layout class that allows controls to be positioned and sized
//     automatically.
//
// Property(Layout.Horizontal):    A flag representing that a horizontal layout should be created.
// Property(Layout.Vertical):      A flag representing that a vertical layout should be created.
// Property(Layout.Top):           A flag representing that a layout item should have padding on the top.
// Property(Layout.Bottom):        A flag representing that a layout item should have padding on the bottom.
// Property(Layout.Left):          A flag representing that a layout item should have padding on the left.
// Property(Layout.Right):         A flag representing that a layout item should have padding on the right.
// Property(Layout.All):           A flag representing that a layout item should have padding on all sides.
// Property(Layout.Expand):        A flag representing that a layout item should be expanded to fill the layout (layout items are expanded vertically for horizontal layouts and expanded horizontally for vertical layouts).
// Property(Layout.Center):        A flag representing that a layout item should be centered in the layout (layout items are centered vertically for horizontal layouts and centered horizontally for vertical layouts).

Layout::Layout()
{
    m_sizer = NULL;
    m_form_wnd = NULL;
}

Layout::~Layout()
{
    std::vector<LayoutItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        delete(*it);
    }
}


void Layout::setFormWindow(wxWindow* wnd)
{
    m_form_wnd = wnd;
}

void Layout::deinitializeControl()
{
    m_form_wnd = NULL;
    std::vector<LayoutItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        if ((*it)->m_layout)
        {
            (*it)->m_layout->deinitializeControl();
        }
         else if ((*it)->m_component)
        {
            (*it)->m_component->deinitializeControl();
        }
    }
}


wxSizer* Layout::getSizer()
{
    return m_sizer;
}

void Layout::add(LayoutItem* item)
{
    m_items.push_back(item);
    
    if (m_sizer != NULL)
        realizeChildren();
}

void Layout::insert(LayoutItem* item, size_t idx)
{
    // -- the index is greater than or equal to the size of the vector,
    //    so just push back the item --
    if (idx >= m_items.size())
    {
        add(item);
        return;
    }
       
    std::vector<LayoutItem*>::iterator insert_it;
    insert_it = m_items.begin() + idx;
    m_items.insert(insert_it, item);
    
    if (m_sizer != NULL)
        realizeChildren();
}

void Layout::clear()
{
    int idx = 0;
    std::vector<LayoutItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        // reset this variable just in case the LayoutItem is reused
        (*it)->m_added = false;
        
        if (m_sizer)
            m_sizer->Show(idx, false);
        
        idx++;
    }
    
    m_items.clear();
}

void Layout::realizeChildren()
{
    if (!m_sizer)
        return;

    size_t idx = 0;
    std::vector<LayoutItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        idx++;
        LayoutItem* item = *it;

        // -- we cannot add the same item to a sizer twice --
        if (item->m_added)
            continue;
            
        // -- if the item is a sizer, add a sizer --
        if (item->m_layout)
        {
            Layout* layout = item->m_layout;

            if (!item->m_realized)
            {
                layout->setFormWindow(m_form_wnd);
                layout->realize();
                item->m_realized = true;
            }
            
            m_sizer->Insert(idx-1,
                            layout->getSizer(),
                            item->m_proportion, 
                            item->m_flags,
                            item->m_border);
            item->m_added = true;
            continue;
        }
        
        // -- if the item is a component, add a component --
        if (item->m_component)
        {
            FormComponent* component = item->m_component;
        
            if (!item->m_realized)
            {
                component->setFormWindow(m_form_wnd);
                component->realize();
                item->m_realized = true;
            }
            
            // we need to call Fit() and SetMinSize()
            // here for the following reasons...
            //
            // 1) Fit() -- if the window is actually a form and its proportion
            //             is set to 0, we need to make sure it gets resized
            //             based on the size hints of the sizer
            //
            // 2) SetMinSize() -- if we've called setSize() on an object and
            //                    add it to a sizer and its proportion is set
            //                    to 0, the sizer will set its size according
            //                    to the minimum size
            wxWindow* w = component->getWindow();
            w->Fit();
            w->SetMinSize(w->GetSize());
            
            m_sizer->Insert(idx-1,
                            component->getWindow(),
                            item->m_proportion,
                            item->m_flags,
                            item->m_border);
            item->m_added = true;
            continue;
        }
        
        // -- if the item is neither a sizer nor a component, add a spacer --
        if (!item->m_layout && !item->m_component)
        {
            m_sizer->Insert(idx-1,
                            item->m_spacer_x,
                            item->m_spacer_y,
                            item->m_proportion,
                            item->m_flags,
                            item->m_border);
            item->m_added = true;
            continue;
        }
    }

    m_sizer->Layout();
}

int Layout::getIndex(kscript::ValueObject* object)
{
    int idx = 0;
    bool item_found = false;
    
    // -- try to get the index of the item in our vector --
    std::vector<LayoutItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); ++it)
    {
        LayoutItem* item = (*it);
        
        if (item->m_layout)
        {
            // -- we've found the item we're looking for --
            if (object == item->m_layout)
            {
                item_found = true;
                break;
            }
        }
        
        if (item->m_component)
        {
            // -- we've found the item we're looking for --
            if (object == item->m_component)
            {
                item_found = true;
                break;
            }
        }
        
        idx++;
    }
    
    if (!item_found)
        return -1;
        
    return idx;
}

// (CONSTRUCTOR) Layout.constructor
// Description: Creates a new Layout object.
//
// Syntax: Layout()
//
// Remarks: Creates a new layout class.

// TODO: elsewhere, we have empty contructors; 1) do these do anything?
// 2) we need to make sure we document them consistently - sometimes they're
// documented and sometimes not.

void Layout::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}

// (METHOD) Layout.show
// Description: Shows or hides the contents of the layout object.
//
// Syntax: function Layout.show(item : FormComponent, 
//                              flag : Boolean)
// Syntax: function Layout.show(item : Layout, 
//                              flag : Boolean)
//
// Remarks: If |flag| is true, this function shows |item|.
//     If |flag| is false, this function hides |item|.
//
// Param(item): The form component or layout object to be shown or hidden
//     in this layout.
// Param(flag): The |flag| determines whether or not to show the contents
//     of the layout object.  If |flag| is true, the contents of the layout
//     object are show; if |flag| is false, they are hidden.

void Layout::show(kscript::ExprEnv* env, kscript::Value* retval)
{
    int idx = 0;
    bool show = true;
    kscript::ValueObject* obj = NULL;
    
    // -- get the parameters the user has passed --
    if (env->getParamCount() > 0)
        obj = env->getParam(0)->getObject();
    if (env->getParamCount() > 1)
        show = env->getParam(1)->getBoolean();

    // -- we couldn't get the bound object, bail out --
    if (!obj)
        return;

    idx = getIndex(obj);
    
    // -- we couldn't find the item in the sizer, bail out --
    if (idx == -1)
        return;
    
    if (!m_sizer)
        return;
    
    m_sizer->Show(idx, show);
}

// (METHOD) Layout.layout
// Description: Positions and sizes the contents of the layout object.
//
// Syntax: function Layout.layout()
//
// Remarks: This function positions and sizes the contents of the layout object.

// TODO: I believe this is redundant and handled by a layout function on
//     the form.  Should this be public?

void Layout::layout(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_sizer)
        m_sizer->Layout();
}






// -- BoxLayout class implementation --

// (CLASS) BoxLayout
// Category: Layout
// Derives: Layout
// Description: A class that allows controls to be positioned and sized
//     automatically in a horizontal or vertical row.
// Remarks: Layout class that allows controls to be positioned and sized
//     automatically in a horizontal or vertical row.

// TODO: box sizer seems like it has a lot of elements that should be
//     on the layout class; for example: add() - all layout sizers
//     have elements that are added to it, and the flags are the same
//     (that's why they're Layout.Expand, etc, and not BoxLayout.Expand).
//     So it seems like some of these functions should be moved to layout
//     and then the specific layout classes determine more specific
//     behavior.

BoxLayout::BoxLayout()
{
    m_orientation = Layout::Horizontal;
}

BoxLayout::~BoxLayout()
{

}

// (CONSTRUCTOR) BoxLayout.constructor
// Description: Creates a new BoxLayout object.
//
// Syntax: BoxLayout()
// Syntax: BoxLayout(orientation : Integer)
//
// Remarks: Creates a new BoxLayout object.  If |orientation| is
//     unspecified or is Layout.Horizontal, the layout object will
//     position and size its contained elements horizontally.  
//     If |orientation| is Layout.Vertical,  the layout object will
//     position and size its contained elements vertically.
//
// Param(orientation): Determines the manner in which to position and
//     size the layout object's components, depending on whether it's
//     Layout.Horizontal or Layout.Vertical.

void BoxLayout::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_orientation = Layout::Horizontal;
    
    if (env->getParamCount() > 0)
        m_orientation = env->getParam(0)->getInteger();

    // if the user used an incorrect value for the orientation flag,
    // go back to the default setting (horizontal)
    if (m_orientation != Layout::Horizontal &&
        m_orientation != Layout::Vertical)
    {
        m_orientation = Layout::Horizontal;
    }
}

void BoxLayout::realize()
{
    if (m_orientation == Layout::Horizontal)
        m_sizer = new wxBoxSizer(wxHORIZONTAL);
         else
        m_sizer = new wxBoxSizer(wxVERTICAL);

    realizeChildren();
}


// (METHOD) BoxLayout.add
// Description: Adds a form component or another layout object to the
//     layout object for it to position and size automatically.
//
// Syntax: function BoxLayout.add(item : FormComponent,
//                                proportion : Integer,
//                                layout_flags : Integer,
//                                border_size : Integer)
//
// Syntax: function BoxLayout.add(item : Layout,
//                                proportion : Integer,
//                                layout_flags : Integer,
//                                border_size : Integer)
//
// Remarks: Adds a form component |item| or another layout |item| to the
//     layout object for it to position and size automatically, based
//     on the rules specified by |proportion|, |layout_flags|, and
//     |border_size|.  |Proportion| determines the size of this item
//     relative to the total proportions of the items. So, if this item
//     has proportion of 1 and there is another item with a proportion
//     of 2, this item will occupy 1/3 of the overall space of the
//     layout object (1 / 1 + 2), subject to the layout behavior specified
//     in |layout_flags|.  |Layout_flags| is a combination of Layout.Top,
//     Layout.Bottom, Layout.Left, Layout.Right, Layout.All, Layout.Expand,
//     and Layout.Center, and Layout.Shaped.  |Border_size| determines the
//     space around the contained elements, subject to the |layout_flags|.
//
// Param(item): The form component or layout object to add to be
//     positioned and sized by this layout object.
// Param(proportion): The relative size of this item to all the other items,
//     subject to the |layout_flags|.
// Param(layout_flags): Combination of Layout.Top, Layout.Bottom, Layout.Left, 
//     Layout.Right, Layout.All, Layout.Expand, and Layout.Center, and Layout.Shaped.
// Param(border_size):  The space around the contained elements, subject to 
//     the |layout_flags|.

// TODO: need better explanation of what the sizer flags and border size 
//     parameters and how they relate to the proportion parameter.

void BoxLayout::add(kscript::ExprEnv* env, kscript::Value* retval)
{
    // get the object to insert in the sizer
    size_t param_count = env->getParamCount();
    
    // if no parameters were passed, bail out
    if (param_count < 1)
        return;
        
    kscript::Value* param1 = env->getParam(0);
    kscript::Value* param2 = env->getParam(1);
    kscript::Value* param3 = env->getParam(2);
    kscript::Value* param4 = env->getParam(3);

    // if the first parameter is not an object, bail out
    if (!param1->isObject())
        return;

    kscript::ValueObject* param1_obj = param1->getObject();
    
    if (param1_obj == this)
        return;
    
    // create a new LayoutItem and add it to the FormSizer;
    // the FormSizer destructor will handle the cleanup
    LayoutItem* item = new LayoutItem();
    item->m_layout = NULL;
    item->m_component = NULL;

    if (param1_obj != NULL)
    {
        if (param1_obj->isKindOf(L"BoxLayout") ||
            param1_obj->isKindOf(L"BorderBoxLayout"))
        {
            item->m_layout = (Layout*)param1_obj;
            item->m_layout->baseRef();
        }
         else if (param1_obj->isKindOf(L"FormControl"))
        {
            item->m_component = (FormComponent*)param1_obj;
            item->m_component->baseRef();
        }
         else
        {
            delete item;
            return;
        }
    }
    
    // -- handle additional parameters --
    if (param_count > 1)
        item->m_proportion = param2->getInteger();
    
    if (param_count > 2)
        item->m_flags = param3->getInteger();
    
    if (param_count > 3)
        item->m_border = param4->getInteger();

    // -- add the item --
    Layout::add(item);
}

// (METHOD) BoxLayout.addSpacer
// Description: Adds a pixel size spacer to the layout object.
//
// Syntax: BoxLayout.addSpacer(size : Integer)
//
// Remarks: Adds a spacer with the specified pixel |size| to the
//     layout object.  If the layout object has a horizontal
//     layout, then |size| is the width of the spacer.  If
//     the layout object has a vertical layout, then |size|
//     is the height of the spacer.
//
// Param(size): The pixel |size| of the spacer to add to the layout
//     object.  If the layout object has a horizontal layout,
//     then |size| is the width in pixels of the spacer.  If the 
//     layout object has a vertical layout, then |size| is the height 
//     in pixels of the spacer.


void BoxLayout::addSpacer(kscript::ExprEnv* env, kscript::Value* retval)
{
    // create a new LayoutItem and add it to the FormSizer
    // the FormSizer destructor will handle the cleanup
    
    LayoutItem* item = new LayoutItem;
    item->m_proportion = 0;
    
    if (m_orientation == Layout::Horizontal)
    {
        item->m_spacer_x = 5;
        item->m_spacer_y = 1;
    }
     else if (m_orientation == Layout::Vertical)
    {
        item->m_spacer_x = 1;
        item->m_spacer_y = 5;
    }
    

    // get the object to add to the sizer
    size_t param_count = env->getParamCount();
    kscript::Value* param1 = env->getParam(0);
    
    if (param_count > 0)
    {
        if (m_orientation == Layout::Horizontal)
        {
            item->m_spacer_x = param1->getInteger();
            item->m_spacer_y = 1;
        }
         else if (m_orientation == Layout::Vertical)
        {
            item->m_spacer_x = 1;
            item->m_spacer_y = param1->getInteger();
        }
    }
 
    // -- add the item --
    Layout::add(item);
}

// (METHOD) BoxLayout.addStretchSpacer
// Description: Adds a stretch spacer to the layout object.
//
// Syntax: BoxLayout.addStretchSpacer(size : Integer)
//
// Remarks: Adds a spacer with the specified proportional |size| 
//     to the layout object.
//
// Param(size): The proportion of the space the stretch spacer
//     will take up relative to all the other proportionally
//     sized items in the layout object.

void BoxLayout::addStretchSpacer(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- create a new LayoutItem and add it to the FormSizer;
    //    the FormSizer destructor will handle the cleanup --
    LayoutItem* item = new LayoutItem();
    item->m_proportion = 1;

    // -- get the object to add to the sizer --
    size_t param_count = env->getParamCount();
    kscript::Value* param1 = env->getParam(0);

    if (param_count > 0)
    {
        item->m_proportion = param1->getInteger();
    }
 
    // -- add the item --
    Layout::add(item);
}

// (METHOD) BoxLayout.insert
// Description: Inserts a form component or another layout object into the
//     layout object for it to position and size automatically.
//
// Syntax: function BoxLayout.insert(index : Integer | FormComponent | Layout,
//                                   item : FormComponent | Layout,
//                                   proportion : Integer,
//                                   layout_flags : Integer,
//                                   border_size : Integer)
//
// Remarks: Inserts a form component |item| or another layout |item| into
//     the layout object for it to position and size automatically, based
//     on the rules specified by |proportion|, |layout_flags|, and |border_size|.  
//     The |item| is inserted at the specified |index| or else before the |index| 
//     of the specified FormComponent or Layout item.  |Proportion| determines 
//     the size of this item relative to the total proportions of the items. So, 
//     if this item has proportion of 1 and there is another item with a proportion 
//     of 2, this item will occupy 1/3 of the overall space of the layout object 
//     (1 / 1 + 2), subject to the layout behavior specified in |layout_flags|.  
//     |Layout_flags| is a combination of Layout.Top, Layout.Bottom, Layout.Left, 
//     Layout.Right, Layout.All, Layout.Expand, and Layout.Center, and Layout.Shaped.  
//     |Border_size| determines the space around the contained elements, subject 
//     to the |layout_flags|.
//
// Param(index): If |index| is an integer, then it's the zero-based |index| where 
//     the item is to be inserted in the layout object.  If |index| is a FormComponent
//     or Layout object, it's the |index| of the FormComponent or Layout object before
//     which the |item| will be inserted.
// Param(item): The form component or layout object to add to be
//     positioned and sized by this layout object.
// Param(proportion): The relative size of this item to all the other items,
//     subject to the |layout_flags|.
// Param(layout_flags): Combination of Layout.Top, Layout.Bottom, Layout.Left, 
//     Layout.Right, Layout.All, Layout.Expand, and Layout.Center, and Layout.Shaped.
// Param(border_size):  The space around the contained elements, subject to 
//     the |layout_flags|.

void BoxLayout::insert(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- get the parameter count --
    size_t param_count = env->getParamCount();
    
    // if no parameters were passed, bail out
    if (param_count < 2)
        return;
        
    kscript::Value* param1 = env->getParam(0);
    kscript::Value* param2 = env->getParam(1);
    kscript::Value* param3 = env->getParam(2);
    kscript::Value* param4 = env->getParam(3);
    kscript::Value* param5 = env->getParam(4);


    int insert_idx = -1;
    if (param1->isObject())
    {
        // -- if the first parameter is an object, get the object's index --
        kscript::ValueObject* param1_obj = param1->getObject();
        insert_idx = getIndex(param1_obj);
    }
     else
    {
        insert_idx = param1->getInteger();
    }
    
    // -- create a new LayoutItem and add it to the FormSizer;
    //    the FormSizer destructor will handle the cleanup --
    LayoutItem* item = new LayoutItem();
    item->m_layout = NULL;
    item->m_component = NULL;

    kscript::ValueObject* param2_obj = param2->getObject();
    if (param2_obj != NULL)
    {
        if (param2_obj->getClassName() == L"BoxLayout" ||
            param2_obj->getClassName() == L"BorderBoxLayout")
        {
            item->m_layout = (Layout*)param2_obj;
            item->m_layout->baseRef();
        }
         else if (param2_obj->isKindOf(L"FormControl"))
        {
            item->m_component = (FormComponent*)param2_obj;
            item->m_component->baseRef();
        }
         else
        {
            delete item;
            return;
        }
    }

    if (param_count > 2)
        item->m_proportion = param3->getInteger();
    
    if (param_count > 3)
        item->m_flags = param4->getInteger();
    
    if (param_count > 4)
        item->m_border = param5->getInteger();

    // -- the item should be inserted at the end of the vector --
    if (insert_idx == -1 || (size_t)insert_idx >= m_items.size())
    {
        Layout::add(item);
        return;
    }

    Layout::insert(item, insert_idx);
}

// (METHOD) BoxLayout.insertSpacer
// Description: Inserts a pixel size spacer into the layout object.
//
// Syntax: BoxLayout.insertSpacer(index : Integer | FormComponent | Layout,
//                                size : Integer)
//
// Remarks: Inserts a spacer with the specified pixel |size| to the
//     layout object at the specified |index|, where the |index|
//     is the zero-based |index| if it is an integer, or else
//     the zero-based |index| of the specified FormComponent or
//     Layout object before which the spacer will be inserted.
//     If the layout object has a horizontal layout, then |size| 
//     is the width of the spacer.  If the layout object has a 
//     vertical layout, then |size| is the height of the spacer.
//
// Param(index): If |index| is an integer, then it's the zero-based |index| 
//     where the spacer is to be inserted in the layout object.  If |index| 
//     is a FormComponent or Layout object, it's the |index| of the FormComponent 
//     or Layout object before which the spacer will be inserted.
// Param(size): The pixel |size| of the spacer to add to the layout object.  If 
//     the layout object has a horizontal layout, then |size| is the width in pixels
//     of the spacer.  If the layout object has a vertical layout, then |size| is the 
//     height in pixels of the spacer.

void BoxLayout::insertSpacer(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- get the parameter count --
    size_t param_count = env->getParamCount();
    kscript::Value* param1 = env->getParam(0);
    kscript::Value* param2 = env->getParam(1);

    // -- we need both paramaters --
    if (param1 == NULL || param2 == NULL || param_count < 2)
        return;

    int insert_idx = -1;
    if (param1->isObject())
    {
        // -- if the first parameter is an object, get the object's index --
        kscript::ValueObject* param1_obj = param1->getObject();
        insert_idx = getIndex(param1_obj);
    }
     else
    {
        insert_idx = param1->getInteger();
    }
    
    // -- create a new LayoutItem and add it to the FormSizer;
    //    the FormSizer destructor will handle the cleanup --
    LayoutItem* item = new LayoutItem();
    item->m_proportion = 0;
    item->m_spacer_x = (m_orientation == Layout::Horizontal) ? param2->getInteger() : 1;
    item->m_spacer_y = (m_orientation == Layout::Vertical) ? param2->getInteger() : 1;
    
    // -- the item should be inserted at the end of the vector --
    if (insert_idx == -1 || (size_t)insert_idx >= m_items.size())
    {
        Layout::add(item);
        return;
    }

    // -- add the item --
    Layout::insert(item, insert_idx);
}

// (METHOD) BoxLayout.insertStretchSpacer
// Description: Inserts a stretch spacer into the layout object.
//
// Syntax: BoxLayout.insertStretchSpacer(index : Integer | FormComponent | Layout,
//                                       size : Integer)
//
// Remarks: Inserts a stretch spacer with the specified proportional 
//     |size| into the layout object at the given |index| if |index|
//     is an Integer or before the |index| of the given FormComponent 
//     or LayoutComponent
//
// Param(index): If |index| is an integer, then it's the zero-based |index| 
//     where the spacer is to be inserted in the layout object.  If |index| 
//     is a FormComponent or Layout object, it's the |index| of the FormComponent 
//     or Layout object before which the spacer will be inserted.
// Param(size): The proportion of the space the stretch spacer
//     will take up relative to all the other proportionally
//     sized items in the layout object.

void BoxLayout::insertStretchSpacer(kscript::ExprEnv* env, kscript::Value* retval)
{
    // -- get the parameter count --
    size_t param_count = env->getParamCount();
    kscript::Value* param1 = env->getParam(0);
    kscript::Value* param2 = env->getParam(1);

    // -- we need both paramaters --
    if (param1 == NULL || param2 == NULL || param_count < 2)
        return;

    int insert_idx = -1;
    if (param1->isObject())
    {
        // -- if the first parameter is an object, get the object's index --
        kscript::ValueObject* param1_obj = param1->getObject();
        insert_idx = getIndex(param1_obj);
    }
     else
    {
        insert_idx = param1->getInteger();
    }
    
    // -- create a new LayoutItem and add it to the FormSizer;
    //    the FormSizer destructor will handle the cleanup --
    LayoutItem* item = new LayoutItem();
    item->m_proportion = 1;
    item->m_proportion = param2->getInteger();
    
    // -- the item should be inserted at the end of the vector --
    if (insert_idx == -1 || (size_t)insert_idx >= m_items.size())
    {
        Layout::add(item);
        return;
    }

    // -- add the item --
    Layout::insert(item, insert_idx);
}

// (METHOD) BoxLayout.clear
// Description: Removes all items from the Layout object.
//
// Syntax: function BoxLayout.clear()
//
// Remarks: Removes all items from the Layout object.

void BoxLayout::clear(kscript::ExprEnv* env, kscript::Value* retval)
{
    Layout::clear();
}


// -- BorderBoxLayout class implementation --

// (CLASS) BorderBoxLayout
// Category: Layout
// Derives: BoxLayout
// Description: A class that draws a box around controls that it positions
//     and sizes automatically.
// Remarks: Layout class that draws a box around controls that it positions
//     and sizes automatically.

BorderBoxLayout::BorderBoxLayout()
{
    m_orientation = Layout::Horizontal;
    m_label = wxEmptyString;
}

BorderBoxLayout::~BorderBoxLayout()
{
}

// (CONSTRUCTOR) BorderBoxLayout.constructor
// Description: Creates a new BorderBoxLayout object.
//
// Syntax: BorderBoxLayout()
// Syntax: BorderBoxLayout(orientation : Integer,
//                         text : String)
//
// Remarks: Creates a new BorderBoxLayout object with the
//     specified |orientation| and |text| label.  If |orientation| is
//     unspecified or is Layout.Horizontal, the layout object will
//     position and size its contained elements horizontally.  
//     If |orientation| is Layout.Vertical,  the layout object will
//     position and size its contained elements vertically.
//
// Param(orientation): Determines the manner in which to position and
//     size the layout object's components, depending on whether it's
//     Layout.Horizontal or Layout.Vertical.
// Param(text): The |text| to use for the BorderBoxLayout label.

void BorderBoxLayout::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 1)
        m_label = env->getParam(1)->getString();

    BoxLayout::constructor(env, retval);
}

void BorderBoxLayout::realize()
{
    if (m_orientation == Layout::Horizontal)
    {
        m_sizer = new wxStaticBoxSizer(wxHORIZONTAL,
                                       m_form_wnd,
                                       m_label);
    }
    else
    {
        m_sizer = new wxStaticBoxSizer(wxVERTICAL,
                                       m_form_wnd,
                                       m_label);
    }

    realizeChildren();
}

