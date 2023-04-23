/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-01-08
 *
 */
 

#ifndef H_APP_SCRIPTLIST_H
#define H_APP_SCRIPTLIST_H


class zListBox : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("ListBox", zListBox, FormControl)
        KSCRIPT_GUI_METHOD("constructor", zListBox::constructor)
        KSCRIPT_GUI_METHOD("addItem", zListBox::addItem)
        KSCRIPT_GUI_METHOD("clear", zListBox::clear)
        KSCRIPT_GUI_METHOD("deleteItem", zListBox::deleteItem)
        KSCRIPT_GUI_METHOD("deselectItem", zListBox::deselectItem)
        KSCRIPT_GUI_METHOD("deselectAllItems", zListBox::deselectAllItems)
        KSCRIPT_GUI_METHOD("findItem", zListBox::findItem)
        KSCRIPT_GUI_METHOD("getSelectedIndex", zListBox::getSelectedIndex)
        KSCRIPT_GUI_METHOD("getSelectedItem", zListBox::getSelectedItem)
        KSCRIPT_GUI_METHOD("getSelectedItems", zListBox::getSelectedItems)
        KSCRIPT_GUI_METHOD("getItem", zListBox::getItem)
        KSCRIPT_GUI_METHOD("getItems", zListBox::getItems)
        KSCRIPT_GUI_METHOD("getItemCount", zListBox::getItemCount)
        KSCRIPT_GUI_METHOD("insertItem", zListBox::insertItem)
        KSCRIPT_GUI_METHOD("isItemSelected", zListBox::isItemSelected)        
        KSCRIPT_GUI_METHOD("setItem", zListBox::setItem)
        KSCRIPT_GUI_METHOD("selectItem", zListBox::selectItem)
        KSCRIPT_GUI_METHOD("sortItems", zListBox::sortItems)
        
        // deprecated
        KSCRIPT_GUI_METHOD("getSelected", zListBox::isItemSelected)    // renamed isItemSelected()
        KSCRIPT_GUI_METHOD("setSelected", zListBox::setSelected)       // replaced by selectItem() and deselectItem()
        KSCRIPT_GUI_METHOD("insert", zListBox::insertItem)             // renamed insertItem()
    END_KSCRIPT_CLASS()

public:

    zListBox();
    ~zListBox();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void addItem(kscript::ExprEnv* env, kscript::Value* retval);
    void clear(kscript::ExprEnv* env, kscript::Value* retval);
    void deleteItem(kscript::ExprEnv* env, kscript::Value* retval);
    void deselectItem(kscript::ExprEnv* env, kscript::Value* retval);
    void deselectAllItems(kscript::ExprEnv* env, kscript::Value* retval);
    void findItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedItems(kscript::ExprEnv* env, kscript::Value* retval);
    void getItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getItems(kscript::ExprEnv* env, kscript::Value* retval);
    void getItemCount(kscript::ExprEnv* env, kscript::Value* retval);
    void insertItem(kscript::ExprEnv* env, kscript::Value* retval);
    void isItemSelected(kscript::ExprEnv* env, kscript::Value* retval);
    void setItem(kscript::ExprEnv* env, kscript::Value* retval);
    void selectItem(kscript::ExprEnv* env, kscript::Value* retval);
    void sortItems(kscript::ExprEnv* env, kscript::Value* retval);
    
    // deprecated
    void setSelected(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    void onEvent(wxEvent& evt);

private:

    wxListBox* m_ctrl;
};


#endif

