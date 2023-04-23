/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-22
 *
 */


#ifndef H_APP_SCRIPTLISTVIEW_H
#define H_APP_SCRIPTLISTVIEW_H


class ListView;

class ListViewItem : public ScriptHostBase
{
friend class ListView;

    BEGIN_KSCRIPT_CLASS("ListViewItem", ListViewItem)
        KSCRIPT_GUI_METHOD("constructor", ListViewItem::constructor)
        KSCRIPT_GUI_METHOD("getIndex", ListViewItem::getIndex)
        KSCRIPT_GUI_METHOD("setText", ListViewItem::setText)
        KSCRIPT_GUI_METHOD("getText", ListViewItem::getText)
        KSCRIPT_GUI_METHOD("setColumnText", ListViewItem::setColumnText)
        KSCRIPT_GUI_METHOD("getColumnText", ListViewItem::getColumnText)
        KSCRIPT_GUI_METHOD("setBitmap", ListViewItem::setBitmap)
        KSCRIPT_GUI_METHOD("getBitmap", ListViewItem::getBitmap)
    END_KSCRIPT_CLASS()
    
public:

    ListViewItem();
    ~ListViewItem();
    
    const long getId();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void getIndex(kscript::ExprEnv* env, kscript::Value* retval);
    
    int getIndexInternal();
    wxString getColumnTextInternal(int col_idx);
    
    wxString getTextInternal();
    void setTextInternal(const wxString& text);
    
    size_t getColumnCountInternal();
    
    void setText(kscript::ExprEnv* env, kscript::Value* retval);
    void getText(kscript::ExprEnv* env, kscript::Value* retval);
    void setBitmap(kscript::ExprEnv* env, kscript::Value* retval);
    void getBitmap(kscript::ExprEnv* env, kscript::Value* retval);
    void setColumnText(kscript::ExprEnv* env, kscript::Value* retval);
    void getColumnText(kscript::ExprEnv* env, kscript::Value* retval);

private:

    ListView* m_owner;
    
    long m_id;
    wxString m_text;
    wxBitmap m_bitmap;
    std::vector<wxString> m_columns;
};


    
    
class ListView : public FormControl
{
friend class ListViewItem;

public:

    BEGIN_KSCRIPT_DERIVED_CLASS("ListView", ListView, FormControl)

        KSCRIPT_GUI_METHOD("constructor", ListView::constructor)
        KSCRIPT_GUI_METHOD("addItem", ListView::addItem)
        KSCRIPT_GUI_METHOD("getItem", ListView::getItem)
        KSCRIPT_GUI_METHOD("addColumn", ListView::addColumn)
        KSCRIPT_GUI_METHOD("deleteColumn", ListView::deleteColumn)
        KSCRIPT_GUI_METHOD("clear", ListView::clear)
        KSCRIPT_GUI_METHOD("deleteItem", ListView::deleteItem)
        KSCRIPT_GUI_METHOD("deselectItem", ListView::deselectItem)
        KSCRIPT_GUI_METHOD("deselectAllItems", ListView::deselectAllItems)
        KSCRIPT_GUI_METHOD("findItem", ListView::findItem)
        KSCRIPT_GUI_METHOD("insertItem", ListView::insertItem)
        KSCRIPT_GUI_METHOD("setView", ListView::setView)
        KSCRIPT_GUI_METHOD("getView", ListView::getView)
        KSCRIPT_GUI_METHOD("setLabelEdit", ListView::setLabelEdit)
        KSCRIPT_GUI_METHOD("getLabelEdit", ListView::getLabelEdit)
        KSCRIPT_GUI_METHOD("getItemCount", ListView::getItemCount)
        KSCRIPT_GUI_METHOD("getItems", ListView::getItems)
        KSCRIPT_GUI_METHOD("getSelectedIndex", ListView::getSelectedIndex)
        KSCRIPT_GUI_METHOD("getSelectedItem", ListView::getSelectedItem)
        KSCRIPT_GUI_METHOD("getSelectedItems", ListView::getSelectedItems)
        KSCRIPT_GUI_METHOD("isItemSelected", ListView::isItemSelected)
        KSCRIPT_GUI_METHOD("selectItem", ListView::selectItem)
        KSCRIPT_GUI_METHOD("sortItems", ListView::sortItems)

        KSCRIPT_CONSTANT_INTEGER("LargeIcon",   LargeIcon)
        KSCRIPT_CONSTANT_INTEGER("SmallIcon",   SmallIcon)
        KSCRIPT_CONSTANT_INTEGER("Details",     Details)
        KSCRIPT_CONSTANT_INTEGER("List",        List)
        KSCRIPT_CONSTANT_INTEGER("AlignLeft",   AlignLeft)
        KSCRIPT_CONSTANT_INTEGER("AlignCenter", AlignCenter)
        KSCRIPT_CONSTANT_INTEGER("AlignRight",  AlignRight)

    END_KSCRIPT_CLASS()

public:
    
    enum
    {
        LargeIcon = 1,
        SmallIcon = 2,
        Details = 3,
        List = 4
    };
    
    enum
    {
        AlignLeft = 1,
        AlignCenter = 2,
        AlignRight = 3
    };
      
public:

    ListView();
    ~ListView();
    
    void realize();
    
    wxListCtrl* getWxListCtrl();
    int getImageListIdx(const wxBitmap& bitmap);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void addItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getItem(kscript::ExprEnv* env, kscript::Value* retval);
    void deleteItem(kscript::ExprEnv* env, kscript::Value* retval);
    void deselectItem(kscript::ExprEnv* env, kscript::Value* retval);
    void deselectAllItems(kscript::ExprEnv* env, kscript::Value* retval);
    void findItem(kscript::ExprEnv* env, kscript::Value* retval);
    void insertItem(kscript::ExprEnv* env, kscript::Value* retval);
    void clear(kscript::ExprEnv* env, kscript::Value* retval);
    void getItemCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getItems(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedItems(kscript::ExprEnv* env, kscript::Value* retval);
    void isItemSelected(kscript::ExprEnv* env, kscript::Value* retval);
    void selectItem(kscript::ExprEnv* env, kscript::Value* retval);
    void sortItems(kscript::ExprEnv* env, kscript::Value* retval);
    
    void addColumn(kscript::ExprEnv* env, kscript::Value* retval);
    void deleteColumn(kscript::ExprEnv* env, kscript::Value* retval);
    
    void setView(kscript::ExprEnv* env, kscript::Value* retval);
    void getView(kscript::ExprEnv* env, kscript::Value* retval);

    void setLabelEdit(kscript::ExprEnv* env, kscript::Value* retval);
    void getLabelEdit(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(wxEvent& evt);

private:

    wxImageList* m_imagelist;
    wxListCtrl* m_ctrl;
    bool m_allow_label_edit;
    std::vector<ListViewItem*> m_items;
};


#endif  // __APP_SCRIPTLISTVIEW_H

