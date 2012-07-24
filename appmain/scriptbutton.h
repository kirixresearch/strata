/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-07
 *
 */


#ifndef __APP_SCRIPTBUTTON_H
#define __APP_SCRIPTBUTTON_H


class Button : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("Button", Button, FormControl)
        KSCRIPT_GUI_METHOD("constructor", Button::constructor)
        KSCRIPT_GUI_METHOD("setLabel", Button::setLabel)
        KSCRIPT_GUI_METHOD("getLabel", Button::getLabel)
        KSCRIPT_GUI_METHOD("setDefault", Button::setDefault)
    END_KSCRIPT_CLASS()

public:

    Button();
    ~Button();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void getLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void setDefault(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    void onEvent(wxEvent& event);

private:

    wxButton* m_ctrl;
};


class BitmapButton : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("BitmapButton", BitmapButton, FormControl)
        KSCRIPT_GUI_METHOD("constructor", BitmapButton::constructor)
        KSCRIPT_GUI_METHOD("setBitmap", BitmapButton::setBitmap)
        KSCRIPT_GUI_METHOD("getBitmap", BitmapButton::getBitmap)
        KSCRIPT_GUI_METHOD("setDefault", BitmapButton::setDefault)
    END_KSCRIPT_CLASS()

public:

    BitmapButton();
    ~BitmapButton();
    void realize();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setBitmap(kscript::ExprEnv* env, kscript::Value* retval);
    void getBitmap(kscript::ExprEnv* env, kscript::Value* retval);
    void setDefault(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    void onEvent(wxEvent& event);

private:

    wxBitmapButton* m_ctrl;
};


class RadioButton : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("RadioButton", RadioButton, FormControl)
        KSCRIPT_GUI_METHOD("constructor", RadioButton::constructor)
        KSCRIPT_GUI_METHOD("setValue", RadioButton::setValue)
        KSCRIPT_GUI_METHOD("getValue", RadioButton::getValue)
    END_KSCRIPT_CLASS()

public:

    RadioButton();
    ~RadioButton();
    void realize();
    
    const int getId();
    wxRadioButton* getWxRadioButton();
    //void setOwner(RadioButtonGroup* owner);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(wxEvent& event);

private:

    int m_id;
    //RadioButtonGroup* m_owner;
    wxRadioButton* m_ctrl;
};


/*
class RadioButtonGroup : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("RadioButtonGroup", RadioButtonGroup, FormControl)
        KSCRIPT_GUI_METHOD("constructor", RadioButtonGroup::constructor)
        KSCRIPT_GUI_METHOD("add", RadioButtonGroup::add)
    END_KSCRIPT_CLASS()

public:

    RadioButtonGroup();
    ~RadioButtonGroup();
    void realize();
    
    void handleSelection(int id);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void add(kscript::ExprEnv* env, kscript::Value* retval);

private:
    
    std::vector<RadioButton*> m_items;
    int m_orientation;
    wxBoxSizer* m_sizer;
    wxPanel* m_ctrl;
};
*/


class CheckBox : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("CheckBox", CheckBox, FormControl)
        KSCRIPT_GUI_METHOD("constructor", CheckBox::constructor)
        KSCRIPT_GUI_METHOD("setLabel", CheckBox::setLabel)
        KSCRIPT_GUI_METHOD("getLabel", CheckBox::getLabel)
        KSCRIPT_GUI_METHOD("setValue", CheckBox::setValue)
        KSCRIPT_GUI_METHOD("getValue", CheckBox::getValue)
    END_KSCRIPT_CLASS()
    
public:
    
    CheckBox();
    ~CheckBox();
    void realize();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void getLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void setValue(kscript::ExprEnv* env, kscript::Value* retval);
    void getValue(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    void onEvent(wxEvent& event);

private:

    wxCheckBox* m_ctrl;
};

#endif

