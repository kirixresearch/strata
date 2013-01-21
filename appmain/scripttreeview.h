/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-06-20
 *
 */


#ifndef __APP_SCRIPTTREEVIEW_H
#define __APP_SCRIPTTREEVIEW_H


class TreeView;

class TreeViewItem : public ScriptHostBase
{
friend class TreeView;

    BEGIN_KSCRIPT_CLASS("TreeViewItem", TreeViewItem)
        KSCRIPT_GUI_METHOD("constructor", TreeViewItem::constructor)
        KSCRIPT_GUI_METHOD("setLabel", TreeViewItem::setLabel)
        KSCRIPT_GUI_METHOD("getLabel", TreeViewItem::getLabel)
        KSCRIPT_GUI_METHOD("setBitmap", TreeViewItem::setBitmap)
        KSCRIPT_GUI_METHOD("getParent", TreeViewItem::getParent)
        KSCRIPT_GUI_METHOD("getChildCount", TreeViewItem::getChildCount)
        KSCRIPT_GUI_METHOD("getFirstChild", TreeViewItem::getFirstChild)
        KSCRIPT_GUI_METHOD("getNextChild", TreeViewItem::getNextChild)
        KSCRIPT_GUI_METHOD("getLastChild", TreeViewItem::getLastChild)
        KSCRIPT_GUI_METHOD("getPreviousSibling", TreeViewItem::getPreviousSibling)
        KSCRIPT_GUI_METHOD("getNextSibling", TreeViewItem::getNextSibling)
        KSCRIPT_GUI_METHOD("hasChildren", TreeViewItem::hasChildren)
        KSCRIPT_GUI_METHOD("isExpanded", TreeViewItem::isExpanded)
        KSCRIPT_GUI_METHOD("isSelected", TreeViewItem::isSelected)
        KSCRIPT_GUI_METHOD("isVisible", TreeViewItem::isVisible)
    END_KSCRIPT_CLASS()
    
public:

    TreeViewItem();
    ~TreeViewItem();
    
    const wxTreeItemId getId();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void getLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void setBitmap(kscript::ExprEnv* env, kscript::Value* retval);
    void getParent(kscript::ExprEnv* env, kscript::Value* retval);
    void getChildCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getFirstChild(kscript::ExprEnv* env, kscript::Value* retval);
    void getNextChild(kscript::ExprEnv* env, kscript::Value* retval);
    void getLastChild(kscript::ExprEnv* env, kscript::Value* retval);
    void getPreviousSibling(kscript::ExprEnv* env, kscript::Value* retval);
    void getNextSibling(kscript::ExprEnv* env, kscript::Value* retval);
    void hasChildren(kscript::ExprEnv* env, kscript::Value* retval);
    void isExpanded(kscript::ExprEnv* env, kscript::Value* retval);
    void isSelected(kscript::ExprEnv* env, kscript::Value* retval);
    void isVisible(kscript::ExprEnv* env, kscript::Value* retval);

private:

    TreeView* m_owner;
    
    wxTreeItemIdValue m_cookie;     // used in wxTreeCtrl::GetFirstChild() and
                                    // wxTreeCtrl::GetNextChild() methods
    wxTreeItemId m_id;
    wxString m_label;
    wxBitmap m_bitmap;
};


class TreeView : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("TreeView", TreeView, FormControl)
        KSCRIPT_GUI_METHOD("constructor", TreeView::constructor)
        KSCRIPT_GUI_METHOD("setRootItem", TreeView::setRootItem)
        KSCRIPT_GUI_METHOD("getRootItem", TreeView::getRootItem)
        KSCRIPT_GUI_METHOD("addItem", TreeView::addItem)
        KSCRIPT_GUI_METHOD("deleteItem", TreeView::deleteItem)
        KSCRIPT_GUI_METHOD("clear", TreeView::clear)
        KSCRIPT_GUI_METHOD("editItemLabel", TreeView::editItemLabel)
        KSCRIPT_GUI_METHOD("scrollToItem", TreeView::scrollToItem)
        KSCRIPT_GUI_METHOD("expandItem", TreeView::expandItem)
        KSCRIPT_GUI_METHOD("collapseItem", TreeView::collapseItem)
        KSCRIPT_GUI_METHOD("toggleItemExpanded", TreeView::toggleItemExpanded)
        KSCRIPT_GUI_METHOD("toggleItemSelected", TreeView::toggleItemSelected)
        KSCRIPT_GUI_METHOD("selectItem", TreeView::selectItem)
        KSCRIPT_GUI_METHOD("deselectItem", TreeView::deselectItem)
        KSCRIPT_GUI_METHOD("deselectAllItems", TreeView::deselectAllItems)
        KSCRIPT_GUI_METHOD("getItemCount", TreeView::getItemCount)
        KSCRIPT_GUI_METHOD("getSelectedItem", TreeView::getSelectedItem)
        KSCRIPT_GUI_METHOD("getSelectedItems", TreeView::getSelectedItems)
        
        // deprecated
        KSCRIPT_GUI_METHOD("add", TreeView::addItem)          // renamed addItem()
        KSCRIPT_GUI_METHOD("delete", TreeView::deleteItem)    // renamed deleteItem()
    END_KSCRIPT_CLASS()
    
public:

    TreeView();
    ~TreeView();
    void realize();
    
    wxTreeCtrl* getWxTreeCtrl();
    int getImageListIdx(const wxBitmap& bitmap);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setRootItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getRootItem(kscript::ExprEnv* env, kscript::Value* retval);
    void addItem(kscript::ExprEnv* env, kscript::Value* retval);
    void deleteItem(kscript::ExprEnv* env, kscript::Value* retval);
    void clear(kscript::ExprEnv* env, kscript::Value* retval);
    void editItemLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void scrollToItem(kscript::ExprEnv* env, kscript::Value* retval);
    void expandItem(kscript::ExprEnv* env, kscript::Value* retval);
    void collapseItem(kscript::ExprEnv* env, kscript::Value* retval);
    void toggleItemExpanded(kscript::ExprEnv* env, kscript::Value* retval);
    void toggleItemSelected(kscript::ExprEnv* env, kscript::Value* retval);
    void selectItem(kscript::ExprEnv* env, kscript::Value* retval);
    void deselectItem(kscript::ExprEnv* env, kscript::Value* retval);
    void deselectAllItems(kscript::ExprEnv* env, kscript::Value* retval);
    void getItemCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedItems(kscript::ExprEnv* env, kscript::Value* retval);

    void setDropData();
    void onDataDropped();
    
private:

    void onEvent(wxEvent& event);

private:

    TreeViewItem* m_root;
    wxImageList* m_imagelist;
    wxTreeCtrl* m_ctrl;
};


#endif  // __APP_SCRIPTTREEVIEW_H

