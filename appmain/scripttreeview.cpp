/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-07
 *
 */


#include "appmain.h"
#include "scriptapp.h"
#include "scriptgui.h"
#include "scriptbitmap.h"
#include "scripttreeview.h"


// class for storing data in tree items

class TreeViewItemData : public wxTreeItemData
{
public:

    TreeViewItemData() { m_item = NULL; }
    
    ~TreeViewItemData()
    {
        if (m_item)
            m_item->baseUnref();
    }
    
    void setItem(TreeViewItem* item)
    {
        if (m_item)
            m_item->baseUnref();
            
        m_item = item;
        
        if (m_item)
            m_item->baseRef();
    }
    
    TreeViewItem* getItem() const
    {
        return m_item;
    }
    
public:

    TreeViewItem* m_item;
};




// TreeViewItem class implementation

// (CLASS) TreeViewItem
// Category: Control
// Description: A class that represents items on a tree view control.
// Remarks: The TreeViewItem represents items on a tree view control.

TreeViewItem::TreeViewItem()
{
    m_owner = NULL;
    m_cookie = NULL;
    m_id = NULL;
    m_label = wxEmptyString;
    m_bitmap = wxNullBitmap;
}

TreeViewItem::~TreeViewItem()
{
}

wxTreeItemId TreeViewItem::getId() const
{
    return m_id;
}

// (CONSTRUCTOR) TreeViewItem.constructor
// Description: Creates a new TreeViewItem.
//
// Syntax: TreeViewItem(label : String,
//                      bitmap : Bitmap)
//
// Remarks: Creates a new TreeViewItem with a given |label| and |bitmap|.
//
// Param(label): The label that displays for the item.
// Param(bitmap): The bitmap that displays for the item.

void TreeViewItem::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
        m_label = env->getParam(0)->getString();

    if (env->getParamCount() > 1)
    {
        kscript::ValueObject* obj = env->getParam(1)->getObject();
        if (obj->getClassName() == L"Bitmap")
        {
            Bitmap* b = (Bitmap*)obj;
            m_bitmap = b->getWxBitmap();
        }
    }
}

// (METHOD) TreeViewItem.setLabel
// Description: Sets the label for a treeview item.
//
// Syntax: function TreeViewItem.setLabel(label : String)
//
// Remarks: Sets the treeview item text, specified by |label|.
//
// Param(label): The new text for the treeview item.

void TreeViewItem::setLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() == 0)
        return;
    
    m_label = env->getParam(0)->getString();
    
    // the item has been added to a TreeView, so we can
    // set the label for the item in the wxTreeCtrl
    if (m_id.IsOk() && m_owner != NULL)
    {
        wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
        wxtree->SetItemText(m_id, m_label);
    }
}

// (METHOD) TreeViewItem.getLabel
// Description: Returns the text of the treeview item.
//
// Syntax: function TreeViewItem.getLabel() : String
//
// Remarks: Returns the text of the treeview item.
//
// Returns: The text of the treeview item.

void TreeViewItem::getLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has been added to a TreeView, so we can
    // get the label from the item in the wxTreeCtrl
    if (m_id.IsOk() && m_owner != NULL)
    {
        wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
        m_label = wxtree->GetItemText(m_id);
    }
    
    retval->setString(towstr(m_label));
}

// (METHOD) TreeViewItem.setBitmap
// Description: Sets a bitmap for the treeview item.
//
// Syntax: function TreeViewItem.setBitmap(bitmap : Bitmap)
//
// Remarks: Sets the treeview item's bitmap, specified by |bitmap|, that will be shown for
// the treeview item.
//
// Param(bitmap): The bitmap to show for the treeview item.


void TreeViewItem::setBitmap(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() == 0)
        return;
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Bitmap")
    {
        Bitmap* b = (Bitmap*)obj;
        m_bitmap = b->getWxBitmap();
    }
    
    // the item has been added to a TreeView, so we can
    // set the bitmap for the item in the wxTreeCtrl
    if (m_id.IsOk() && m_owner != NULL)
    {
        wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
        int bmp_idx = m_owner->getImageListIdx(m_bitmap);
        wxtree->SetItemImage(m_id, bmp_idx);
        
    }
}

// (METHOD) TreeViewItem.getParent
// Description: Gets the parent of a treeview item.
//
// Syntax: function TreeViewItem.getParent() : TreeViewItem
//
// Remarks: Gets the parent of a treeview item and returns it.  If the treeview
//     item doesn't have a parent, the function returns an empty treeview item.
//
// Returns: Returns the treeview item parent.

void TreeViewItem::getParent(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView,
    // return an empty TreeViewItem
    if (!m_id.IsOk() || m_owner == NULL)
    {
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    wxTreeItemId parent_id = wxtree->GetItemParent(m_id);
    
    if (!parent_id.IsOk())
    {
        // couldn't find a parent item, return an empty TreeViewItem
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }

    // get the TreeViewItemData associated with the root tree item
    TreeViewItemData* data = (TreeViewItemData*)wxtree->GetItemData(parent_id);
    
    if (data)
    {
        // return the TreeViewItem from the TreeViewItemData
        kscript::Value val;
        val.setObject(data->getItem());
        retval->setValue(val);
    }
}

// (METHOD) TreeViewItem.getChildCount
// Description: Returns the number of children of the treeview item.
//
// Syntax: function TreeViewItem.getChildCount(all_children : Boolean) : Integer
//
// Remarks: Gets the number of children of the treeview item.  If |all_children| is
//     true, then the function counts all child items recursively and returns the
//     total number of children.  If |all_children| is false, then the function
//     only counts the immediate children of the item and returns this number.
//     If |all_children| isn't specified, the function returns the count of all
//     children.
//
// Param(all_children) : Flag denoting whether to return all children (true) or
//     just the immediate, first-level children (false).
//
// Returns: Returns the number of children of the treeview item.

void TreeViewItem::getChildCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView, return 0
    if (!m_id.IsOk() || m_owner == NULL)
    {
        retval->setInteger(0);
        return;
    }
    
    bool recursive = true;
    
    if (env->getParamCount() > 0)
        recursive = env->getParam(0)->getBoolean();
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    size_t child_count = wxtree->GetChildrenCount(m_id, recursive);
    retval->setInteger(child_count);
}

// (METHOD) TreeViewItem.getFirstChild
// Description: Gets the first child of the treeview item.
//
// Syntax: function TreeViewItem.getFirstChild() : TreeViewItem
//
// Remarks: Gets the first child of the treeview item and returns it. If
//     the function can't find a first child, the function returns an empty
//     treeview item.
//
// Returns: Returns the first child of a treeview item.

void TreeViewItem::getFirstChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView,
    // return an empty TreeViewItem
    if (!m_id.IsOk() || m_owner == NULL)
    {
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    wxTreeItemId child_id = wxtree->GetFirstChild(m_id, m_cookie);
    
    if (!child_id.IsOk())
    {
        // couldn't find a child item, return an empty TreeViewItem
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }

    // get the TreeViewItemData associated with the child item
    TreeViewItemData* data = (TreeViewItemData*)wxtree->GetItemData(child_id);
    
    if (data)
    {
        // return the TreeViewItem from the TreeViewItemData
        kscript::Value val;
        val.setObject(data->getItem());
        retval->setValue(val);
    }
}

// (METHOD) TreeViewItem.getNextChild
// Description: Gets the next child of the treeview item.
//
// Syntax: function TreeViewItem.getNextChild() : TreeViewItem
//
// Remarks: Gets the next child of this item.  The next child is the child
//     after the last child that was returned from this function or from
//     getFirstChild().  If the function can't find the next item, it returns
//     an empty treeview item.
//
// Returns: Returns the next child of a treeview item after the last item that 
//     was returned from this function of from getFirstChild().

void TreeViewItem::getNextChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView,
    // return an empty TreeViewItem

    if (!m_id.IsOk() || m_owner == NULL)
    {
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    wxTreeItemId child_id = wxtree->GetNextChild(m_id, m_cookie);
    
    if (!child_id.IsOk())
    {
        // couldn't find a child item, return an empty TreeViewItem

        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }

    // get the TreeViewItemData associated with the child item
    TreeViewItemData* data = (TreeViewItemData*)wxtree->GetItemData(child_id);
    
    if (data)
    {
        // return the TreeViewItem from the TreeViewItemData

        kscript::Value val;
        val.setObject(data->getItem());
        retval->setValue(val);
    }
}

// (METHOD) TreeViewItem.getLastChild
// Description: Gets the last child of the treeview item.
//
// Syntax: function TreeViewItem.getLastChild() : TreeViewItem
//
// Remarks: Gets the last child of the treeview item and returns it. If
//     the function can't find a last child, the function returns an empty
//     treeview item.
//
// Returns: Returns the last child of a treeview item.

void TreeViewItem::getLastChild(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView,
    // return an empty TreeViewItem

    if (!m_id.IsOk() || m_owner == NULL)
    {
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    wxTreeItemId child_id = wxtree->GetLastChild(m_id);
    
    if (!child_id.IsOk())
    {
        // couldn't find a child item, return an empty TreeViewItem
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }

    // get the TreeViewItemData associated with the child item
    TreeViewItemData* data = (TreeViewItemData*)wxtree->GetItemData(child_id);
    
    if (data)
    {
        // return the TreeViewItem from the TreeViewItemData
        kscript::Value val;
        val.setObject(data->getItem());
        retval->setValue(val);
    }
}

// (METHOD) TreeViewItem.getPreviousSibling
// Description: Gets the treeview item right before this item.
//
// Syntax: function TreeViewItem.getPreviousSibling() : TreeViewItem
//
// Remarks: Gets the treeview item right before this item, which is the
//     treeview item that has the same parent as the current item and is
//     right before this item in the tree.  If the function can't find
//     the previous child item, it returns an empty treeview item.
//
// Returns: Returns the treeview item right before this item.

void TreeViewItem::getPreviousSibling(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView,
    //  return an empty TreeViewItem
    if (!m_id.IsOk() || m_owner == NULL)
    {
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    wxTreeItemId sibling_id = wxtree->GetPrevSibling(m_id);
    
    if (!sibling_id.IsOk())
    {
        // couldn't find the sibling item, return an empty TreeViewItem
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }

    // get the TreeViewItemData associated with the sibling item
    TreeViewItemData* data = (TreeViewItemData*)wxtree->GetItemData(sibling_id);
    
    if (data)
    {
        // return the TreeViewItem from the TreeViewItemData
        kscript::Value val;
        val.setObject(data->getItem());
        retval->setValue(val);
    }
}

// (METHOD) TreeViewItem.getNextSibling
// Description: Gets the treeview item right after this item.
//
// Syntax: function TreeViewItem.getNextSibling() : TreeViewItem
//
// Remarks: Gets the treeview item right after this item, which is the
//     treeview item that has the same parent as the current item and is
//     right after this item in the tree.  If the function can't find
//     the next child item, it returns an empty treeview item.
//
// Returns: Returns the treeview item right after this item.

void TreeViewItem::getNextSibling(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView,
    // return an empty TreeViewItem
    if (!m_id.IsOk() || m_owner == NULL)
    {
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    wxTreeItemId sibling_id = wxtree->GetNextSibling(m_id);
    
    if (!sibling_id.IsOk())
    {
        // couldn't find the sibling item, return an empty TreeViewItem
        kscript::Value val;
        env->createObject(L"TreeViewItem", &val);
        retval->setValue(val);
        return;
    }

    // get the TreeViewItemData associated with the sibling item
    TreeViewItemData* data = (TreeViewItemData*)wxtree->GetItemData(sibling_id);
    
    if (data)
    {
        // return the TreeViewItem from the TreeViewItemData
        kscript::Value val;
        val.setObject(data->getItem());
        retval->setValue(val);
    }
}

// (METHOD) TreeViewItem.hasChildren
// Description: Indicates whether a treeview item has children or not.
//
// Syntax: function TreeViewItem.hasChildren() : Boolean
//
// Remarks: Indicates whether a treeview item has children or not.  If
//     the treeview item has children, the function returns true; otherwise,
//     it returns false.  If the treeview item is not part of the treeview, 
//     the function also returns false.
//
// Returns: Returns true if the item has children, and false otherwise.

void TreeViewItem::hasChildren(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView, bail out
    if (!m_id.IsOk() || m_owner == NULL)
    {
        retval->setBoolean(false);
        return;
    }
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    retval->setBoolean(wxtree->ItemHasChildren(m_id));
}

// (METHOD) TreeViewItem.isExpanded
// Description: Indicates whether a treeview item is expanded or not.
//
// Syntax: function TreeViewItem.isExpanded() : Boolean
//
// Remarks: Indicates whether a treeview item is expanded so that it's children
//     are visible, if present, or whether the treeview item is collapsed so
//     that children are hidden.  If the treeview item is expanded, the function 
//     returns true.  If the treeview item is collapsed, the function returns false.
//     If the treeview item is not part of the treeview, the function also returns 
//     false.
//
// Returns: Returns true if the item is expanded, and false if it is collapsed.

void TreeViewItem::isExpanded(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView, bail out
    if (!m_id.IsOk() || m_owner == NULL)
    {
        retval->setBoolean(false);
        return;
    }
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    retval->setBoolean(wxtree->IsExpanded(m_id));
}

// (METHOD) TreeViewItem.isSelected
// Description: Indicates whether a treeview item is selected or not.
//
// Syntax: function TreeViewItem.isSelected() : Boolean
//
// Remarks: Indicates whether a treeview item is selected or not.  If the treeview 
//     item is selected, the function returns true.  If the treeview item is not
//     selected, the function returns false.  If the treeview item is not part
//     of the treeview, the function also returns false.
//
// Returns: Returns true if the item is selected, and false otherwise.

void TreeViewItem::isSelected(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView, bail out
    if (!m_id.IsOk() || m_owner == NULL)
    {
        retval->setBoolean(false);
        return;
    }
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    retval->setBoolean(wxtree->IsSelected(m_id));
}

// (METHOD) TreeViewItem.isVisible
// Description: Indicates whether a treeview item is visible or not.
//
// Syntax: function TreeViewItem.isVisible() : Boolean
//
// Remarks: Indicates whether a treeview item is visible or not.  For example, a
//     treeview item may not be visible if it's outside the window or it's the child
//     of a collapsed parent.  If the treeview item is visible, the function returns
//     true.  If the treeview item is not visible, the function returns false.  If the
//     item is not part of the treeview, the function also returns false.
//
// Returns: Returns true if the item is visible, and false otherwise.

void TreeViewItem::isVisible(kscript::ExprEnv* env, kscript::Value* retval)
{
    // the item has not been added to a TreeView, bail out
    if (!m_id.IsOk() || m_owner == NULL)
    {
        retval->setBoolean(false);
        return;
    }
    
    wxTreeCtrl* wxtree = m_owner->getWxTreeCtrl();
    retval->setBoolean(wxtree->IsVisible(m_id));
}


// TreeView class implementation

// (CLASS) TreeView
// Category: Control
// Derives: FormControl
// Description: A class that represents a tree view control.
// Remarks: The TreeView class represents a tree view control.

TreeView::TreeView()
{
    m_root = NULL;
    m_imagelist = NULL;
    m_ctrl = NULL;
}

TreeView::~TreeView()
{
    if (m_imagelist)
        delete m_imagelist;
}

wxTreeCtrl* TreeView::getWxTreeCtrl()
{
    return m_ctrl;
}

int TreeView::getImageListIdx(const wxBitmap& bitmap)
{
    if (!bitmap.Ok())
        return 0;

    int idx = 0;
    int count = m_imagelist->GetImageCount();
    
    for (idx = 0; idx < count; ++idx)
    {
        if (m_imagelist->GetBitmap(idx).IsSameAs(bitmap))
            return idx;
    }
    
    idx = m_imagelist->Add(bitmap);
    return idx;
}

// (CONSTRUCTOR) TreeView.constructor
// Description: Creates a new TreeView.
//
// Syntax: TreeView(x_pos : Integer,
//                  y_pos : Integer,
//                  width : Integer,
//                  height : Integer)
//
// Remarks: Creates a new TreeView control at the position given by
//     |x_pos| and |y_pos| with dimensions given by |width| and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void TreeView::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // add some properties
    getMember(L"keyDown")->setObject(Event::createObject(env));
    getMember(L"selectionChanged")->setObject(Event::createObject(env));
    getMember(L"selectionChanging")->setObject(Event::createObject(env));
    getMember(L"itemBeginLabelEdit")->setObject(Event::createObject(env));
    getMember(L"itemEndLabelEdit")->setObject(Event::createObject(env));
    getMember(L"itemCollapsed")->setObject(Event::createObject(env));
    getMember(L"itemCollapsing")->setObject(Event::createObject(env));
    getMember(L"itemExpanded")->setObject(Event::createObject(env));
    getMember(L"itemExpanding")->setObject(Event::createObject(env));
    getMember(L"itemDelete")->setObject(Event::createObject(env));
    getMember(L"itemActivate")->setObject(Event::createObject(env));
    getMember(L"itemRightClick")->setObject(Event::createObject(env));
    
    // set default values
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
        m_ctrl = new wxTreeCtrl(getApp()->getTempParent(),
                                -1,
                                wxDefaultPosition,
                                wxDefaultSize,
                                wxTR_HAS_BUTTONS |
                                wxTR_LINES_AT_ROOT |
                                wxTR_MULTIPLE |
                                wxTR_EDIT_LABELS);
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
        m_ctrl = new wxTreeCtrl(getApp()->getTempParent(),
                                -1,
                                wxPoint(m_x, m_y),
                                wxSize(m_width, m_height),
                                wxTR_HAS_BUTTONS |
                                wxTR_LINES_AT_ROOT |
                                wxTR_MULTIPLE |
                                wxTR_EDIT_LABELS);
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

void TreeView::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (!m_enabled)
        m_ctrl->Enable(false);

    listenEvent(wxEVT_COMMAND_TREE_KEY_DOWN);
    listenEvent(wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT);
    listenEvent(wxEVT_COMMAND_TREE_END_LABEL_EDIT);
    listenEvent(wxEVT_COMMAND_TREE_ITEM_EXPANDED);
    listenEvent(wxEVT_COMMAND_TREE_ITEM_EXPANDING);
    listenEvent(wxEVT_COMMAND_TREE_ITEM_COLLAPSED);
    listenEvent(wxEVT_COMMAND_TREE_ITEM_COLLAPSING);
    listenEvent(wxEVT_COMMAND_TREE_SEL_CHANGED);
    listenEvent(wxEVT_COMMAND_TREE_SEL_CHANGING);
    listenEvent(wxEVT_COMMAND_TREE_DELETE_ITEM);
    listenEvent(wxEVT_COMMAND_TREE_ITEM_ACTIVATED);
    listenEvent(wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK);
}


// (METHOD) TreeView.setRootItem
// Description: Sets the root item of the treeview.
//
// Syntax: function TreeView.setRootItem(item : TreeViewItem)
//
// Remarks: Sets the root item of the treeview from the specified |item|.
//
// Param(item): The item to use as the root item.

void TreeView::setRootItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (obj->getClassName() != L"TreeViewItem")
        return;

    TreeViewItem* item = (TreeViewItem*)obj;
    m_root = item;
    
    int bmp_idx = -1;
    if (item->m_bitmap.Ok())
    {
        if (!m_imagelist)
        {
            // if our tree control doesn't have an imagelist, create one
            m_imagelist = new wxImageList(item->m_bitmap.GetWidth(),
                                          item->m_bitmap.GetHeight(),
                                          true,
                                          0);
            m_ctrl->SetImageList(m_imagelist);
        }
        
        bmp_idx = getImageListIdx(item->m_bitmap);
    }
    
    // set the item's m_owner variable to this TreeView
    item->m_owner = this;
    
    item->m_id = m_ctrl->AddRoot(item->m_label,
                                 bmp_idx);
                                
    // associate the TreeViewItem class with the wxTreeItemId so we can
    // retrive the TreeViewItem class when an event is fired

    TreeViewItemData* data = new TreeViewItemData;
    data->setItem(item);
    m_ctrl->SetItemData(item->m_id, data);
}

// (METHOD) TreeView.getRootItem
// Description: Gets the root item of the treeview.
//
// Syntax: function TreeView.getRootItem() : TreeViewItem
//
// Remarks: Gets the root item of the treeview.
//
// Returns: Returns the root item of the treeview.

void TreeView::getRootItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    wxTreeItemId root_id = m_ctrl->GetRootItem();
    
    if (!root_id.IsOk())
    {
        // couldn't find a root item on the wxTreeCtrl, so set
        // the return value to null and bail out
        retval->setNull();
        return;
    }

    // get the TreeViewItemData associated with the root tree item
    TreeViewItemData* data = (TreeViewItemData*)m_ctrl->GetItemData(root_id);
    
    if (data)
    {
        // return the TreeViewItem from the TreeViewItemData
        kscript::Value val;
        val.setObject(data->getItem());
        retval->setValue(val);
    }
}

// (METHOD) TreeView.addItem
// Description: Adds a treeview item to the treeview.
//
// Syntax: function TreeView.addItem(item : TreeViewItem,
//                                   parent : TreeViewItem)
// Remarks: Adds a treeview item, specified by |item|, to the
//     node specified by |parent|.  If |parent| isn't specified,
//     then |item| is added to the root item of the treeview.
//
// Param(item): The item to add to the treeview.
// Param(parent): The node to which to add the item.

// ----------
// ( METHOD ) TreeView.add
// DEPRECATED: Use TreeView.addItem() instead of TreeView.add()

void TreeView::addItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (!obj->isKindOf(L"TreeViewItem"))
        return;

    // if the tree doesn't have a root item yet, we need to add one,
    // so the item that was going to be added is now going
    // to become the root node
    if (!m_root)
    {
        setRootItem(env, retval);
        return;
    }
        
    TreeViewItem* item = (TreeViewItem*)obj;
    
    int bmp_idx = -1;
    if (item->m_bitmap.Ok())
    {
        if (!m_imagelist)
        {
            // if our tree control doesn't have an imagelist, create one

            m_imagelist = new wxImageList(item->m_bitmap.GetWidth(),
                                          item->m_bitmap.GetHeight(),
                                          true,
                                          0);
            m_ctrl->SetImageList(m_imagelist);
        }
        
        bmp_idx = getImageListIdx(item->m_bitmap);
    }
    
    // if no second parameter is specified, add this item to the root

    wxTreeItemId parent_id = m_root->getId();
    
    // (optional) the second parameter is the item
    // to which this item will be added

    if (env->getParamCount() > 1)
    {
        kscript::ValueObject* obj2 = env->getParam(1)->getObject();
        
        // only accept a TreeViewItem object as the parameter
        if (obj2->isKindOf(L"TreeViewItem"))
        {
            TreeViewItem* parent_item = (TreeViewItem*)obj2;
            parent_id = parent_item->getId();
        }
    }
    
    // set the item's m_owner variable to this TreeView
    item->m_owner = this;
    
    item->m_id = m_ctrl->AppendItem(parent_id,
                                    item->m_label,
                                    bmp_idx);
                                    
    // associate the TreeViewItem class with the wxTreeItemId so we can
    // retrive the TreeViewItem class when an event is fired
    TreeViewItemData* data = new TreeViewItemData;
    data->setItem(item);
    m_ctrl->SetItemData(item->m_id, data);
}

// (METHOD) TreeView.deleteItem
// Description: Deletes a treeview item from the treeview.
//
// Syntax: function TreeView.deleteItem(item : TreeViewItem)
//
// Remarks: Deletes the treeview item, specified by |item|, from the treeview.
//
// Param(item): The item to delete from the treeview.

// ----------
// ( METHOD ) TreeView.delete
// DEPRECATED: Use TreeView.deleteItem() instead of TreeView.delete()

void TreeView::deleteItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (!obj->isKindOf(L"TreeViewItem"))
        return;
        
    TreeViewItem* item = (TreeViewItem*)obj;
    m_ctrl->Delete(item->getId());
}

// (METHOD) TreeView.clear
// Description: Clears the treeview of all items.
//
// Syntax: function TreeView.clear()
//
// Remarks: Clears the treeview of all items.

void TreeView::clear(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->DeleteAllItems();
}

// (METHOD) TreeView.editItemLabel
// Description:  Causes a treeview item to enter into editing mode.
//
// Syntax: function TreeView.editItemLabel(item : TreeViewItem)
//
// Remarks: This function triggers an event that causes a treeview
//     item, specified by |item|, to enter into editing mode.
//
// Param(item): The treeview item that should enter into editing mode.

void TreeView::editItemLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (!obj->isKindOf(L"TreeViewItem"))
        return;
        
    TreeViewItem* item = (TreeViewItem*)obj;
    m_ctrl->EditLabel(item->getId());
}

// (METHOD) TreeView.scrollToItem
// Description: Scrolls the treeview control to a given treeview item.
//
// Syntax: function TreeView.scrollToItem(item : TreeViewItem)
//
// Remarks: Scrolls the treeview control to a treeview item, which is 
//     specified by |item|.
//
// Param(item): The item to which the treeview control should scroll.

void TreeView::scrollToItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (!obj->isKindOf(L"TreeViewItem"))
        return;
        
    TreeViewItem* item = (TreeViewItem*)obj;
    m_ctrl->ScrollTo(item->getId());
}

// (METHOD) TreeView.expandItem
// Description: Expands a treeview item in the treeview.
//
// Syntax: function TreeView.expandItem(item : TreeViewItem)
//
// Remarks: Expands a treeview item, specified by |item|, so that it shows
//     its children, if they exist.
//
// Param(item): The treeview item to expand.

void TreeView::expandItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (!obj->isKindOf(L"TreeViewItem"))
        return;
        
    TreeViewItem* item = (TreeViewItem*)obj;
    m_ctrl->Expand(item->getId());
}

// (METHOD) TreeView.collapseItem
// Description: Collapses a treeview item in the treeview.
//
// Syntax: function TreeView.collapseItem(item : TreeViewItem)
//
// Remarks: Collapses a treeview item, specified by |item|, so that it
//     no longer shows any children it may have.
//
// Param(item): The treeview item to collapse.

void TreeView::collapseItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (!obj->isKindOf(L"TreeViewItem"))
        return;
        
    TreeViewItem* item = (TreeViewItem*)obj;
    m_ctrl->Collapse(item->getId());
}

// (METHOD) TreeView.toggleItemExpanded
// Description: Expands or collapses a treeview item.
//
// Syntax: function TreeView.toggleItemExpanded(item : TreeViewItem)
//
// Remarks: Expands or collapses a treeview item, depending on whether
//     it is currently expanded or collapsed.  If the treeview item is
//     currently expanded, the function will collapse it.  If the treeview
//     item is currently collapsed, the function will expand it.
//
// Param(item): The treeview item to expand or collapse.

void TreeView::toggleItemExpanded(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (!obj->isKindOf(L"TreeViewItem"))
        return;
        
    TreeViewItem* item = (TreeViewItem*)obj;
    m_ctrl->Toggle(item->getId());
}

// (METHOD) TreeView.toggleItemSelected
// Description: Selects or deselects a treeview item.
//
// Syntax: function TreeView.toggleItemSelected(item : TreeViewItem)
//
// Remarks: Selects or deselects a treeview item, depending on whether
//     it is currently selected or deselected.  If the treeview item is
//     currently selected, the function will deselect it.  If the treeview
//     item is currently deselected, the function will select it.
//
// Param(item): The treeview item to select or deselect.

void TreeView::toggleItemSelected(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (!obj->isKindOf(L"TreeViewItem"))
        return;
        
    TreeViewItem* item = (TreeViewItem*)obj;
    m_ctrl->ToggleItemSelection(item->getId());
}

// (METHOD) TreeView.selectItem
// Description: Selects a treeview item in the treeview.
//
// Syntax: function TreeView.selectItem(item : TreeViewItem)
//
// Remarks: Selects a treeview item, specified by |item|.
//
// Param(item): The treeview item to select.

void TreeView::selectItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (!obj->isKindOf(L"TreeViewItem"))
        return;
        
    TreeViewItem* item = (TreeViewItem*)obj;
    m_ctrl->SelectItem(item->getId());
}

// (METHOD) TreeView.deselectItem
// Description: Deselects a treeview item in the treeview.
//
// Syntax: function TreeView.deselectItem(item : TreeViewItem)
//
// Remarks: Deselects a treeview item, specified by |item|.
//
// Param(item): The treeview item to deselect.

void TreeView::deselectItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !isControlValid())
        return;

    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    // only accept a TreeViewItem object as the parameter
    if (!obj->isKindOf(L"TreeViewItem"))
        return;
        
    TreeViewItem* item = (TreeViewItem*)obj;
    m_ctrl->UnselectItem(item->getId());
}

// (METHOD) TreeView.deselectAllItems
// Description: Deselects all items in the treeview.
//
// Syntax: function TreeView.deselectAllItems()
//
// Remarks: Deselects all items in the treeview.

void TreeView::deselectAllItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->UnselectAll();
}

// (METHOD) TreeView.getItemCount
// Description: Returns the number of items in the treeview.
//
// Syntax: function TreeView.getItemCount() : Integer
//
// Remarks: Returns the number of items in the treeview.
//
// Returns: Returns the number of items in the treeview.

void TreeView::getItemCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    retval->setInteger(m_ctrl->GetCount());
}

// (METHOD) TreeView.getSelectedItem
// Description: Gets selected treeview item from the treeview.
//
// Syntax: function TreeView.getSelectedItem() : TreeViewItem
//
// Remarks: Gets the selected treeview item from the treeview.  If the
//     treeview doesn't have a selection, the function returns null.
//
// Returns:  Returns the selected treeview item, if a treeview item is selected.
//     Otherwise, the function returns null.

// TODO: elsewhere, we return an empty TreeViewItem node if it can't be found
// or doesn't exists.  Seems like we should make these functions behavior consistent
// one way or another.

void TreeView::getSelectedItem(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

    wxTreeItemId id;
    
    // we need this bit of code here because wx will throw an ASSERT
    // failure if GetSelections() is called without the wxTR_MULTIPLE
    // flag specified and vice versa for GetSelection()

    if (m_ctrl->GetWindowStyle() & wxTR_MULTIPLE)
    {
        wxArrayTreeItemIds a;
        size_t count = m_ctrl->GetSelections(a);
        
        if (count > 0)
            id = a.Item(0);
    }
     else
    {
        id = m_ctrl->GetSelection();
    }
    
    if (!id.IsOk())
    {
        retval->setNull();
        return;
    }
       
    // get the TreeViewItemData associated with this wxTreeItemId
    TreeViewItemData* data = (TreeViewItemData*)m_ctrl->GetItemData(id);
    
    if (data)
    {
        // return the TreeViewItem from the TreeViewItemData
        kscript::Value val;
        val.setObject(data->getItem());
        retval->setValue(val);
    }
     else
    {
        retval->setNull();
    }
}

// (METHOD) TreeView.getSelectedItems
// Description: Gets an array of the selected treeview items from the treeview.
//
// Syntax:  function TreeView.getSelectedItems() : Array(TreeViewItem)
//
// Remarks: Gets an array of the selected treeview items from the treeview.
//
// Returns: Returns an array of the selected treeview items in the treeview.

void TreeView::getSelectedItems(kscript::ExprEnv* env, kscript::Value* retval)
{
    // set our return type to be an array
    retval->setArray(env);

    if (!isControlValid())
        return;

    // we need this bit of code here because wx will throw an ASSERT
    // failure if GetSelection() is called without the wxTR_SINGLE
    // flag specified and vice versa for GetSelections()

    if (m_ctrl->GetWindowStyle() & wxTR_SINGLE)
    {
        wxTreeItemId id = m_ctrl->GetSelection();
        
        if (!id.IsOk())
            return;
            
        // get the TreeViewItemData associated with this wxTreeItemId
        TreeViewItemData* data = (TreeViewItemData*)m_ctrl->GetItemData(id);
        
        kscript::Value val;
        val.setObject(data->getItem());
        retval->appendMember(&val);
        return;
    }

    wxArrayTreeItemIds a;
    size_t count = m_ctrl->GetSelections(a);
    size_t i = 0;

    for (i = 0; i < count; ++i)
    {
        wxTreeItemId id = a.Item(i);
    
        if (!id.IsOk())
            continue;
           
        // get the TreeViewItemData associated with this wxTreeItemId
        TreeViewItemData* data = (TreeViewItemData*)m_ctrl->GetItemData(id);
        
        if (data)
        {
            // add the TreeViewItem to the array
            kscript::Value val;
            val.setObject(data->getItem());
            retval->appendMember(&val);
        }
    }
}




// (EVENT) TreeView.keyDown
// Description: Fired when the tool bar item is clicked.
// Param(keyCode): Key code of the key pressed.
// Param(altDown): True if the Alt key is pressed, false otherwise.
// Param(controlDown): True if the Control key is pressed, false otherwise.
// Param(shiftDown): True if the Shift key is pressed, false otherwise.

// (EVENT) TreeView.selectionChanging
// Description: Fired when the tree's item selection is about to change.

// (EVENT) TreeView.selectionChanged
// Description: Fired when the tree's item selection has changed.

// (EVENT) TreeView.itemBeginLabelEdit
// Description: Fired when a tree item's label editing begins.

// (EVENT) TreeView.itemEndLabelEdit
// Description: Fired when a tree item's label editing ends.

// (EVENT) TreeView.itemCollapsing
// Description: Fired when a tree item is about to be collapsed.

// (EVENT) TreeView.itemCollapsed
// Description: Fired after a tree item is collapsed.

// (EVENT) TreeView.itemExpanding
// Description: Fired when a tree item is about to be expanded.

// (EVENT) TreeView.itemExpanded
// Description: Fired after a tree item is collapsed.

// (EVENT) TreeView.itemDelete
// Description: Fired when a tree item is deleted.

// (EVENT) TreeView.itemActivate
// Description: Fired when a tree item is activated.

// (EVENT) TreeView.itemRightClick
// Description: Fired when a tree item is right-clicked.


void TreeView::onEvent(wxEvent& event)
{
    wxEventType event_type = event.GetEventType();
    
    if (event_type == wxEVT_COMMAND_TREE_KEY_DOWN)
    {
        wxTreeEvent& tevt = (wxTreeEvent&)event;
        
        // we need to figure out how we are going to handle key codes,
        // since we do not want to have a direct pass-through to
        // wxWidgets's key codes
        kscript::Value* event_args = new kscript::Value;
        event_args->getMember(L"keyCode")->setInteger(tevt.GetKeyCode());
        event_args->getMember(L"altDown")->setBoolean(::wxGetKeyState(WXK_ALT) ? true : false);
        event_args->getMember(L"controlDown")->setBoolean(::wxGetKeyState(WXK_CONTROL) ? true : false);
        event_args->getMember(L"shiftDown")->setBoolean(::wxGetKeyState(WXK_SHIFT) ? true : false);
        
        invokeJsEvent(L"keyDown", event_args);
        return;
    }
     else if (event_type == wxEVT_COMMAND_TREE_SEL_CHANGING ||
              event_type == wxEVT_COMMAND_TREE_SEL_CHANGED ||
              event_type == wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT ||
              event_type == wxEVT_COMMAND_TREE_END_LABEL_EDIT ||
              event_type == wxEVT_COMMAND_TREE_ITEM_COLLAPSING ||
              event_type == wxEVT_COMMAND_TREE_ITEM_COLLAPSED ||
              event_type == wxEVT_COMMAND_TREE_ITEM_EXPANDING ||
              event_type == wxEVT_COMMAND_TREE_ITEM_EXPANDED ||
              event_type == wxEVT_COMMAND_TREE_DELETE_ITEM ||
              event_type == wxEVT_COMMAND_TREE_ITEM_ACTIVATED ||
              event_type == wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK)
    {
        wxTreeEvent& tree_event = (wxTreeEvent&)event;
        wxTreeItemId treeitem_id = tree_event.GetItem();
        
        if (!treeitem_id.IsOk())
            return;
            
        TreeViewItemData* data = NULL;
        
        if (isControlValid())
            data = (TreeViewItemData*)m_ctrl->GetItemData(treeitem_id);

        if (data)
        {
            kscript::Value* event_args = new kscript::Value;
            event_args->getMember(L"item")->setObject(data->getItem());
        
            if (event_type == wxEVT_COMMAND_TREE_SEL_CHANGING)
                invokeJsEvent(L"selectionChanging", event_args);
                 else if (event_type == wxEVT_COMMAND_TREE_SEL_CHANGED)
                invokeJsEvent(L"selectionChanged", event_args);
                 else if (event_type == wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT)
                invokeJsEvent(L"itemBeginLabelEdit", event_args);
                 else if (event_type == wxEVT_COMMAND_TREE_END_LABEL_EDIT)
                invokeJsEvent(L"itemEndLabelEdit", event_args);
                 else if (event_type == wxEVT_COMMAND_TREE_ITEM_COLLAPSING)
                invokeJsEvent(L"itemCollapsing", event_args);
                 else if (event_type == wxEVT_COMMAND_TREE_ITEM_COLLAPSED)
                invokeJsEvent(L"itemCollapsed", event_args);
                 else if (event_type == wxEVT_COMMAND_TREE_ITEM_EXPANDING)
                invokeJsEvent(L"itemExpanding", event_args);
                 else if (event_type == wxEVT_COMMAND_TREE_ITEM_EXPANDED)
                invokeJsEvent(L"itemExpanded", event_args);
                 else if (event_type == wxEVT_COMMAND_TREE_DELETE_ITEM)
                invokeJsEvent(L"itemDelete", event_args);
                 else if (event_type == wxEVT_COMMAND_TREE_ITEM_ACTIVATED)
                invokeJsEvent(L"itemActivate", event_args);
                 else if (event_type == wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK)
                invokeJsEvent(L"itemRightClick", event_args);
        }
        
        return;
    }
}
