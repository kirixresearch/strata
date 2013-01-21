/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-24
 *
 */


#ifndef __APP_SCRIPTTEXT_H
#define __APP_SCRIPTTEXT_H


class TextBox : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("TextBox", TextBox, FormControl)
        KSCRIPT_GUI_METHOD("constructor", TextBox::constructor)
        KSCRIPT_GUI_METHOD("clear", TextBox::clear)
        KSCRIPT_GUI_METHOD("setText", TextBox::setText)
        KSCRIPT_GUI_METHOD("getText", TextBox::getText)
        KSCRIPT_GUI_METHOD("appendText", TextBox::appendText)
        KSCRIPT_GUI_METHOD("select", TextBox::select)
        KSCRIPT_GUI_METHOD("selectAll", TextBox::selectAll)
        KSCRIPT_GUI_METHOD("getSelectedText", TextBox::getSelectedText)
        KSCRIPT_GUI_METHOD("getLineText", TextBox::getLineText)
        KSCRIPT_GUI_METHOD("getLineLength", TextBox::getLineLength)
        KSCRIPT_GUI_METHOD("getLineCount", TextBox::getLineCount)
        KSCRIPT_GUI_METHOD("getMaxLength", TextBox::getMaxLength)
        KSCRIPT_GUI_METHOD("setMaxLength", TextBox::setMaxLength)
        KSCRIPT_GUI_METHOD("getPasswordMode", TextBox::getPasswordMode)
        KSCRIPT_GUI_METHOD("setPasswordMode", TextBox::setPasswordMode)
        KSCRIPT_GUI_METHOD("getInsertionPoint", TextBox::getInsertionPoint)
        KSCRIPT_GUI_METHOD("setInsertionPoint", TextBox::setInsertionPoint)
        KSCRIPT_GUI_METHOD("setInsertionPointEnd", TextBox::setInsertionPointEnd)
        KSCRIPT_GUI_METHOD("setMultiline", TextBox::setMultiline)
        KSCRIPT_GUI_METHOD("isMultiline", TextBox::isMultiline)
    END_KSCRIPT_CLASS()

public:

    TextBox();
    ~TextBox();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void clear(kscript::ExprEnv* env, kscript::Value* retval);
    void setText(kscript::ExprEnv* env, kscript::Value* retval);
    void getText(kscript::ExprEnv* env, kscript::Value* retval);
    void appendText(kscript::ExprEnv* env, kscript::Value* retval);
    void select(kscript::ExprEnv* env, kscript::Value* retval);
    void selectAll(kscript::ExprEnv* env, kscript::Value* retval);
    void getSelectedText(kscript::ExprEnv* env, kscript::Value* retval);
    void getLineText(kscript::ExprEnv* env, kscript::Value* retval);
    void getLineLength(kscript::ExprEnv* env, kscript::Value* retval);
    void getLineCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getMaxLength(kscript::ExprEnv* env, kscript::Value* retval);
    void setMaxLength(kscript::ExprEnv* env, kscript::Value* retval);
    void getPasswordMode(kscript::ExprEnv* env, kscript::Value* retval);
    void setPasswordMode(kscript::ExprEnv* env, kscript::Value* retval);
    void getInsertionPoint(kscript::ExprEnv* env, kscript::Value* retval);
    void setInsertionPoint(kscript::ExprEnv* env, kscript::Value* retval);
    void setInsertionPointEnd(kscript::ExprEnv* env, kscript::Value* retval);
    void setMultiline(kscript::ExprEnv* env, kscript::Value* retval);
    void isMultiline(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(wxEvent& event);

private:

    long m_orig_style;
    unsigned long m_max_length;
    bool m_password_mode;
    wxTextCtrl* m_ctrl;
};





class Label : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("Label", Label, FormControl)
        KSCRIPT_GUI_METHOD("constructor", Label::constructor)
        KSCRIPT_GUI_METHOD("setLabel", Label::setLabel)
        KSCRIPT_GUI_METHOD("getLabel", Label::getLabel)
        KSCRIPT_GUI_METHOD("wrap", Label::wrap)
    END_KSCRIPT_CLASS()

public:

    Label();
    ~Label();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void getLabel(kscript::ExprEnv* env, kscript::Value* retval);
    void wrap(kscript::ExprEnv* env, kscript::Value* retval);

private:

    wxStaticText* m_ctrl;
};


#endif
