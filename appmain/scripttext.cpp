/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-24
 *
 */


#include "appmain.h"
#include "scriptgui.h"
#include "scripttext.h"


// TextBox class implementation

// (CLASS) TextBox
// Category: Control
// Derives: FormControl
// Description: A class that represents a text box control.
// Remarks: The TextBox class represents a text box control.

TextBox::TextBox()
{
    m_ctrl = NULL;
    m_max_length = 0;
    m_password_mode = false;
}

TextBox::~TextBox()
{

}

// (CONSTRUCTOR) TextBox.constructor
// Description: Creates a new TextBox.
//
// Syntax: TextBox(text : String,
//                 x_pos : Integer,
//                 y_pos : Integer,
//                 width : Integer,
//                 height : Integer)
//
// Remarks: Creates a new TextBox control, having initial text specified by
//     |text|, at the position specified by |x_pos| and |y_pos|, and having
//     dimensions specified by |width| and |height|.
//
// Param(text): The initial text of the control.
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void TextBox::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // add some properties
    getMember(L"textChanged")->setObject(Event::createObject(env));
    getMember(L"enterPressed")->setObject(Event::createObject(env));
    getMember(L"maxLengthExceeded")->setObject(Event::createObject(env));

    // set default values
    wxString text = wxEmptyString;
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();

    // get user input values
    if (param_count > 0)
        text = towx(env->getParam(0)->getString());
    if (param_count > 1)
        m_x = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_y = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_width = env->getParam(3)->getInteger();
    if (param_count > 4)
        m_height = env->getParam(4)->getInteger();

    // store the original style for the text control (we need to do this
    // because in other functions we actually destroy and recreate
    // the text control (to make sure it is displayed correctly)
    m_orig_style = wxTE_PROCESS_ENTER;
    
    if (m_password_mode)
        m_orig_style |= wxTE_PASSWORD;
    
    if (param_count < 2)
    {
        // create the control
        m_ctrl = new wxTextCtrl(getApp()->getTempParent(),
                                -1,
                                text,
                                wxDefaultPosition,
                                wxDefaultSize,
                                m_orig_style);
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
        m_ctrl = new wxTextCtrl(getApp()->getTempParent(),
                                -1,
                                text,
                                wxPoint(m_x, m_y),
                                wxSize(m_width, m_height),
                                m_orig_style);
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

void TextBox::realize()
{
    m_ctrl->Reparent(m_form_wnd);

    if (!m_enabled)
        m_ctrl->Enable(false);

    listenEvent(wxEVT_COMMAND_TEXT_UPDATED);
    listenEvent(wxEVT_COMMAND_TEXT_ENTER);
    listenEvent(wxEVT_COMMAND_TEXT_MAXLEN);
}

// (METHOD) TextBox.clear
// Description: Clears the textbox of all text.
//
// Syntax: function TextBox.clear()
//
// Remarks: Clears the textbox of all text.

void TextBox::clear(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->Clear();
}

// (METHOD) TextBox.setText
// Description: Sets the text for the textbox.
//
// Syntax: function TextBox.setText(text : String)
//
// Remarks: Sets the text for the textbox from |text|.
//
// Param(text): The text to which to set the textbox text.

void TextBox::setText(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    wxString text = towx(env->getParam(0)->getString());
    m_ctrl->SetValue(text);
}

// (METHOD) TextBox.getText
// Description: Gets the text of the textbox.
//
// Syntax: funciton TextBox.getText() : String
//
// Remarks: Gets the text of the textbox.
//
// Returns: Returns the text of the textbox.

void TextBox::getText(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }

#ifdef _UNICODE
    wxString str = m_ctrl->GetValue();
    retval->setString(str.c_str(), str.Length());
#else
    std::wstring text = towstr(m_ctrl->GetValue());
    retval->setString(text);
#endif
}

// (METHOD) TextBox.appendText
// Description: Appends text to the text of the textbox.
//
// Syntax: function TextBox.appendText(text : String)
//
// Remarks: Appends the text, specified by |text|, to the the
//     existing text of the textbox.
//
// Param(text): The text to append to the existing text in the textbox.

void TextBox::appendText(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    wxString s = towx(env->getParam(0)->getString());
    size_t len = s.Length();

#ifdef WIN32
    // just using AppendText() somehow clips the text
    // when added to the control.  The reason for this
    // is unknown.  Therefore we use the following
    // workaround for win32 systems
    wxString text = m_ctrl->GetValue();
    text += s;
    m_ctrl->Freeze();
    m_ctrl->SetValue(text);
    m_ctrl->ShowPosition(text.Length());
    m_ctrl->Thaw();
#else
     m_ctrl->AppendText(s);
#endif
}

// (METHOD) TextBox.select
// Description: Selects text in the textbox.
//
// Syntax: function TextBox.select(index : Integer,
//                                 count : Integer)
//
// Remarks: Selects text in the textbox, starting at the position specified
//     by |index| and continuing for the number of characters specified by
//     |count|.
//
// Param(index): The index of the character to start the selection.
// Param(count): The number of characters to select.

void TextBox::select(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;
        
    int from = 0;
    int len = 0;

    if (env->getParamCount() < 2)
        return;
        
    from = env->getParam(0)->getInteger();
    len = env->getParam(1)->getInteger();
    
    m_ctrl->SetSelection(from, from+len);
    m_ctrl->SetFocus();
}

// (METHOD) TextBox.selectAll
// Description: Selects all the text in the textbox.
//
// Syntax: function TextBox.selectAll()
//
// Remarks: Selects all the text in the textbox.

void TextBox::selectAll(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;
    
    m_ctrl->SetSelection(-1, -1);
    m_ctrl->SetFocus();
}

// (METHOD) TextBox.getSelectedText
// Description: Gets the selected text in the textbox.
//
// Syntax: function TextBox.getSelectedText() : String
//
// Remarks: Gets the selected text in the textbox and returns it as a string.
//
// Returns: Returns the selected text in the textbox.

void TextBox::getSelectedText(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    retval->setString(m_ctrl->GetStringSelection());
}

// (METHOD) TextBox.getLineText
// Description: Gets the text on a specified line number.
//
// Syntax: function TextBox.getLineText(line : Integer) : String
//
// Remarks: Gets the text on line, specified by |line|, and returns the
//     the text as a string.
//
// Returns: Returns the string of text on a given line.

void TextBox::getLineText(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    wxString text = wxEmptyString;
    int line = 0;
    
    if (env->getParamCount() > 0 && env->getParam(0)->isNumber())
        line = env->getParam(0)->getInteger();
        
    if (m_ctrl->IsMultiLine())
    {
        text = m_ctrl->GetLineText(line);
    }
     else
    {
        text = m_ctrl->GetValue();
    }

    retval->setString(towstr(text));
}

// (METHOD) TextBox.getLineLength
// Description: Gets the number of characters on a line.
//
// Syntax: function TextBox.getLineLength(line : Integer) : Integer
//
// Remarks: Returns the number of characters on a given |line| in the textbox.
//
// Returns: Returns the number of characters on a line.

void TextBox::getLineLength(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    int line_length = 0;
    int line = 0;
    
    if (env->getParamCount() > 0 && env->getParam(0)->isNumber())
        line = env->getParam(0)->getInteger();
        
    if (m_ctrl->IsMultiLine())
    {
        line_length = m_ctrl->GetLineLength(line);
    }
     else
    {
        line_length = m_ctrl->GetValue().Length();
    }

    retval->setInteger(line_length);
}

// (METHOD) TextBox.getLineCount
// Description: Gets the number of lines in a textbox.
//
// Syntax: function TextBox.getLineCount() : Integer
//
// Remarks: Returns the number of lines in the textbox.  If the textbox is
//     not a multiline textbox, the function returns 1.
//
// Returns: Returns the number of lines in the textbox.

void TextBox::getLineCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    if (m_ctrl->IsMultiLine())
        retval->setInteger(m_ctrl->GetNumberOfLines());
         else
        retval->setInteger(1);
}

// (METHOD) TextBox.getMaxLength
// Description: Gets the maximum number of characters a textbox can have.
//
// Syntax: function TextBox.getMaxLength() : Integer
//
// Remarks: Returns the maximum number of characters the textbox can have.
//     If the textbox doesn't have a maximum length, the function returns 0.
//
// Returns: Returns the maximum number of characters the textbox can have.

void TextBox::getMaxLength(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    retval->setInteger(m_max_length);
}

// (METHOD) TextBox.setMaxLength
// Description: Sets the maximum number of characters a textbox can have
//
// Syntax: function TextBox.setMaxLength(length : Integer)
//
// Remarks: Sets the maximum number of characters the textbox can have to |length|.
//
// Param(length): The maximum number of characters the texbox can have.

void TextBox::setMaxLength(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0 && env->getParam(0)->isNumber() && isControlValid())
    {
        unsigned long max_length = env->getParam(0)->getInteger();
        m_ctrl->SetMaxLength(max_length);
        m_max_length = max_length;
    }
}

// (METHOD) TextBox.getPasswordMode
// Description: Returns the state of the password mode for the textbox.
//
// Syntax: function TextBox.getPasswordMode() : Boolean
//
// Remarks: When a textbox is in password mode, the text is substituted with
//     a non-descript character such as an asterisk so that the text can't be
//     read.  This function returns true if password mode is on for the textbox,
//     and false if the password mode is off.
//
// Returns: Returns true if the password mode is on for the textbox, and
//     false otherwise.

void TextBox::getPasswordMode(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    retval->setBoolean(m_password_mode);
}

// (METHOD) TextBox.setPasswordMode
// Description: Sets the state of the password mode for the textbox.
//
// Syntax: function TextBox.setPasswordMode(flag : Boolean)
//
// Remarks: When a textbox is in password mode, the text is substituted with
//     a non-descript character such as an asterisk so that the text can't be
//     read.  This function turns the password mode on if |flag| is true and
//     off if |flag| is false.
//
// Param(flag) : A flag to turn the password mode on (true) or off (false).

void TextBox::setPasswordMode(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
        m_password_mode = env->getParam(0)->getBoolean();
         else
        m_password_mode = true;
        
    if (m_wnd)
    {
        long style = m_wnd->GetWindowStyleFlag();
        
        if (m_password_mode)
            style |= wxTE_PASSWORD;
             else
            style &= ~wxTE_PASSWORD;
        
        wxString text = m_ctrl->GetValue();
        wxWindow* parent = m_ctrl->GetParent();
        
        // find out if this control was added to a sizer (for use below)
        wxSizer* sizer = m_ctrl->GetContainingSizer();
            
        // right now, in order to reset the style of the wxTextCtrl,
        // we have to destroy it and recreate it, otherwise, the new
        // style doesn't work... perhaps we can fix this at some point
        
        // set the style for the control
        wxTextCtrl* new_ctrl = new wxTextCtrl(parent,
                                              -1,
                                              text,
                                              wxPoint(m_x, m_y),
                                              wxSize(m_width, m_height),
                                              style);
        
        // the control was part of a sizer; make sure we remove
        // the old control and add the new control to the sizer
        if (sizer)
        {
            sizer->Replace(m_ctrl, new_ctrl);
            sizer->Layout();
        }
        
        // destroy the old control and store
        // the new control as a member variable
        m_ctrl->Destroy();
        m_ctrl = new_ctrl;
        m_wnd = m_ctrl;
    }
}

// (METHOD) TextBox.getInsertionPoint
// Description: Gets the insertion point for the textbox.
//
// Syntax: function TextBox.getInsertionPoint() : Integer
//
// Remarks: Returns the insertion point for the textbox, which is
//     the position in the textbox text where the cursor is located and
//     the index where text will be inserted.
//
// Returns: Returns the insertion point for the textbox.

void TextBox::getInsertionPoint(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
        
    retval->setInteger(m_ctrl->GetInsertionPoint());
}

// (METHOD) TextBox.setInsertionPoint
// Description: Sets the insertion point for the textbox.
//
// Syntax: function TextBox.setInsertionPoint(index : Integer)
//
// Remarks: Sets the insertion point in the textbox to the offset
//     given by |index|.  The insertion point is the position in the 
//     textbox text where the cursor is located and the index where text 
//     will be inserted.
//
// Param(index): The index of the character in the textbox where the insertion 
//     point should be positioned

void TextBox::setInsertionPoint(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0 && env->getParam(0)->isNumber() && isControlValid())
    {
        m_ctrl->SetInsertionPoint(env->getParam(0)->getInteger());
    }
}

void TextBox::setInsertionPointEnd(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
        m_ctrl->SetInsertionPointEnd();
}

// (METHOD) TextBox.setMultiline
// Description: Sets the multiline mode for the textbox.
//
// Syntax: function TextBox.setMultiline()
// Syntax: function TextBox.setMultiline(flag : Boolean)
//
// Remarks: Sets the multiline mode of the textbox.  If |flag|
//     is true, then the textbox multiline mode will be turned
//     on, causing the textbox text to wrap.  If |flag| is false,
//     then the textbox multiple mode will be turned off, so that
//     the textbox text will not wrap.
//
// Param(flag): A flag to turn the textbox multiline mode on or off.

void TextBox::setMultiline(kscript::ExprEnv* env, kscript::Value* retval)
{
    // get present style
    long style = m_orig_style;

    // remove any existing multiline style
    style &= ~wxTE_MULTILINE;

    if (env->getParamCount() == 0 ||
        env->getParam(0)->getBoolean() == true)
    {
        // the process enter flag is set by default for
        // multiline text controls (an assert failure
        // will be thrown if we don't do this here)
        style &= ~wxTE_PROCESS_ENTER;
        
        // add the multiline flag to the style
        style |= wxTE_MULTILINE;
    }

    wxWindow* parent = m_ctrl->GetParent();
    wxString text = m_ctrl->GetValue();
    
    // find out if this control was added to a sizer (for use below)
    wxSizer* sizer = m_ctrl->GetContainingSizer();
        
    // right now, in order to reset the style of the wxTextCtrl,
    // we have to destroy it and recreate it, otherwise, the new
    // style doesn't work... perhaps we can fix this at some point
    
    // set the style for the control
    wxTextCtrl* new_ctrl = new wxTextCtrl(parent,
                                          -1,
                                          text,
                                          wxPoint(m_x, m_y),
                                          wxSize(m_width, m_height),
                                          style);
    
    // the control was part of a sizer; make sure we remove
    // the old control and add the new control to the sizer
    if (sizer)
    {
        sizer->Replace(m_ctrl, new_ctrl);
        sizer->Layout();
    }
    
    // destroy the old control and store
    // the new control as a member variable
    m_ctrl->Destroy();
    m_ctrl = new_ctrl;
    m_wnd = m_ctrl;
}

// (METHOD) TextBox.isMultiline
// Description: Gets the multiline mode for the textbox.
//
// Syntax: function TextBox.isMultiline() : Boolean
//
// Remarks: Returns true if the textbox multiline mode is on and
//     false if it is turned off.
//
// Returns: Returns true if the textbox multiline mode is on and
//     false if it is turned off.

void TextBox::isMultiline(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setBoolean(false);
        return;
    }
    
    retval->setBoolean(m_ctrl->IsMultiLine());
}



// (EVENT) TextBox.textChanged
// Description: Fired when the text is updated in the control.
// Param(text): New value of the text box.

// (EVENT) TextBox.enterPressed
// Description: Fired when enter is pressed in the control.

// (EVENT) TextBox.maxLengthExceeded
// Description: Fired when the maximum length of the text control is exceeded.

void TextBox::onEvent(wxEvent& event)
{
    wxCommandEvent& evt = (wxCommandEvent&)event;
    std::wstring text = towstr(evt.GetString());
    
    kscript::Value* event_args = new kscript::Value;
    event_args->setObject();
    event_args->getMember(L"text")->setString(text);
    
    if (event.GetEventType() == wxEVT_COMMAND_TEXT_UPDATED)
        invokeJsEvent(L"textChanged", event_args);
    else if (event.GetEventType() == wxEVT_COMMAND_TEXT_ENTER)
        invokeJsEvent(L"enterPressed", event_args);
    else if (event.GetEventType() == wxEVT_COMMAND_TEXT_MAXLEN)
        invokeJsEvent(L"maxLengthExceeded", event_args);
}


// Label class implementation

// (CLASS) Label
// Category: Control
// Derives: FormControl
// Description: A class that represents a label control.
// Remarks: The Label class represents a label control.

Label::Label()
{
    m_ctrl = NULL;
}

Label::~Label()
{

}

// (CONSTRUCTOR) Label.constructor
// Description: Creates a new Label.
//
// Syntax: Label(text : String,
//               x_pos : Integer,
//               y_pos : Integer,
//               width : Integer,
//               height : Integer)
//
// Remarks: Creates a new Label, having the label specified by |text|, at the 
//     position specified by |x_pos| and |y_pos|, and having the dimensions
//     specified by |width| and |height|.
//
// Param(text): The text of the label.
// Param(x_pos): The x position of the label.
// Param(y_pos): The y position of the label.
// Param(width): The width of the label.
// Param(height): The height of the label.

void Label::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // set default values
    wxString label = wxEmptyString;
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
    if (param_count > 0)
        label = towx(env->getParam(0)->getString());
    if (param_count > 1)
        m_x = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_y = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_width = env->getParam(3)->getInteger();
    if (param_count > 4)
        m_height = env->getParam(4)->getInteger();

    if (param_count < 2)
    {
        // create the control
        m_ctrl = new wxStaticText(getApp()->getTempParent(),
                                  -1,
                                  label,
                                  wxDefaultPosition,
                                  wxDefaultSize);
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
        m_ctrl = new wxStaticText(getApp()->getTempParent(),
                                  -1,
                                  label,
                                  wxPoint(m_x, m_y),
                                  wxSize(m_width, m_height));
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

void Label::realize()
{
    m_ctrl->Reparent(m_form_wnd);

    if (!m_enabled)
    {
        m_ctrl->Enable(false);
    }
}

// (METHOD) Label.setLabel
// Description: Sets the text of the label.
//
// Syntax: function Label.setLabel(text : String)
//
// Remarks: Sets the text of the label.
//
// Param(text): The text to which to set the label's text.

void Label::setLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0 && isControlValid())
    {
        wxString label = towx(env->getParam(0)->getString());
        m_ctrl->SetLabel(label);
    }
}

// (METHOD) Label.getLabel
// Description: Gets the text of the label.
//
// Syntax: function Label.getLabel() : String
//
// Remarks: Gets the text of the label.
//
// Returns: Returns the text of the label.

void Label::getLabel(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setNull();
        return;
    }
    
    std::wstring label = towstr(m_ctrl->GetLabel());
    retval->setString(label);
}

// (METHOD) Label.wrap
// Description: Sets the width to which to wrap the label's text
//
// Syntax: function Label.wrap(width : Integer)
//
// Remarks: Sets the |width| to which wrap the label's text.
//
// Param(width): The width to which to wrap the label's text.

void Label::wrap(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0 && isControlValid())
    {
        int width = env->getParam(0)->getInteger();
        m_ctrl->Wrap(width);
    }
}


