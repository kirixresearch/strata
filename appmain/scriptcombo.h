/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-08
 *
 */


#ifndef __APP_SCRIPTCOMBO_H
#define __APP_SCRIPTCOMBO_H


class ComboBox : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("ComboBox", ComboBox, FormControl)
        KSCRIPT_GUI_METHOD("constructor", ComboBox::constructor)
        KSCRIPT_GUI_METHOD("addItem", ComboBox::addItem)
        KSCRIPT_GUI_METHOD("clear", ComboBox::clear)
        KSCRIPT_GUI_METHOD("deleteItem", ComboBox::deleteItem)
        KSCRIPT_GUI_METHOD("findItem", ComboBox::findItem)
        KSCRIPT_GUI_METHOD("getItemCount", ComboBox::getItemCount)
        KSCRIPT_GUI_METHOD("getSelectedIndex", ComboBox::getSelectedIndex)
        KSCRIPT_GUI_METHOD("getItem", ComboBox::getItem)
        KSCRIPT_GUI_METHOD("getValue", ComboBox::getValue)
        KSCRIPT_GUI_METHOD("insertItem", ComboBox::insertItem)
        KSCRIPT_GUI_METHOD("selectItem", ComboBox::selectItem)
        KSCRIPT_GUI_METHOD("setItem", ComboBox::setItem)
        KSCRIPT_GUI_METHOD("setValue", ComboBox::setValue)
        
        // deprecated
        KSCRIPT_GUI_METHOD("getSelection", ComboBox::getSelectedIndex)    // renamed getSelectedIndex()
        KSCRIPT_GUI_METHOD("add", ComboBox::addItem)                      // renamed addItem()
        KSCRIPT_GUI_METHOD("delete", ComboBox::deleteItem)                // renamed deleteItem()
        KSCRIPT_GUI_METHOD("findString", ComboBox::findItem)              // renamed findItem()
        KSCRIPT_GUI_METHOD("getCount", ComboBox::getItemCount)            // renamed getItemCount()
        KSCRIPT_GUI_METHOD("getString", ComboBox::getItem)                // renamed getItem()
        KSCRIPT_GUI_METHOD("insert", ComboBox::insertItem)                // renamed insertItem()
        KSCRIPT_GUI_METHOD("select", ComboBox::selectItem)                // renamed selectItem()
        KSCRIPT_GUI_METHOD("setString", ComboBox::setItem)                // renamed setItem()
    END_KSCRIPT_CLASS()

public:

    ComboBox();
    ~ComboBox();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void addItem(kscript::ExprEnv* env, kscript::Value* retval);
    void clear(kscript::ExprEnv* env, kscript::Value* retval);
    void deleteItem(kscript::ExprEnv* env, kscript::Value* retval);
    void findItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getItemCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval);
    void getItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void insertItem(kscript::ExprEnv* env, kscript::Value* retval);
    void selectItem(kscript::ExprEnv* env, kscript::Value* retval);
    void setItem(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(wxEvent& event);

private:

    wxComboBox* m_ctrl;
};


class ChoiceBox : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("ChoiceBox", ChoiceBox, FormControl)
        KSCRIPT_GUI_METHOD("constructor", ChoiceBox::constructor)
        KSCRIPT_GUI_METHOD("addItem", ChoiceBox::addItem)
        KSCRIPT_GUI_METHOD("clear", ChoiceBox::clear)
        KSCRIPT_GUI_METHOD("deleteItem", ChoiceBox::deleteItem)
        KSCRIPT_GUI_METHOD("findItem", ChoiceBox::findItem)
        KSCRIPT_GUI_METHOD("getItemCount", ChoiceBox::getItemCount)
        KSCRIPT_GUI_METHOD("getSelectedIndex", ChoiceBox::getSelectedIndex)
        KSCRIPT_GUI_METHOD("getItem", ChoiceBox::getItem)
        KSCRIPT_GUI_METHOD("getValue", ChoiceBox::getValue)
        KSCRIPT_GUI_METHOD("insertItem", ChoiceBox::insertItem)
        KSCRIPT_GUI_METHOD("selectItem", ChoiceBox::selectItem)
        KSCRIPT_GUI_METHOD("setItem", ChoiceBox::setItem)
        KSCRIPT_GUI_METHOD("setValue", ChoiceBox::setValue)
        
        // deprecated
        KSCRIPT_GUI_METHOD("getSelection", ChoiceBox::getSelectedIndex)  // renamed getSelectedIndex()
        KSCRIPT_GUI_METHOD("add", ChoiceBox::addItem)                    // renamed addItem()
        KSCRIPT_GUI_METHOD("delete", ChoiceBox::deleteItem)              // renamed deleteItem()
        KSCRIPT_GUI_METHOD("findString", ChoiceBox::findItem)            // renamed findItem()
        KSCRIPT_GUI_METHOD("getCount", ChoiceBox::getItemCount)          // renamed getItemCount()
        KSCRIPT_GUI_METHOD("getString", ChoiceBox::getItem)              // renamed getItem()
        KSCRIPT_GUI_METHOD("insert", ChoiceBox::insertItem)              // renamed insertItem()
        KSCRIPT_GUI_METHOD("select", ChoiceBox::selectItem)              // renamed selectItem()
        KSCRIPT_GUI_METHOD("setString", ChoiceBox::setItem)              // renamed setItem()
    END_KSCRIPT_CLASS()

public:

    ChoiceBox();
    ~ChoiceBox();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void addItem(kscript::ExprEnv* env, kscript::Value* retval);
    void clear(kscript::ExprEnv* env, kscript::Value* retval);
    void deleteItem(kscript::ExprEnv* env, kscript::Value* retval);
    void findItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getItemCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedIndex(kscript::ExprEnv* env, kscript::Value* retval);
    void getItem(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    void insertItem(kscript::ExprEnv* env, kscript::Value* retval);
    void selectItem(kscript::ExprEnv* env, kscript::Value* retval);
    void setItem(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(wxEvent& event);

private:

    wxChoice* m_ctrl;
};

#endif

