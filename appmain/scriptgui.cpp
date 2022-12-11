/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-06-20
 *
 */


#include "appmain.h"
#include "toolbars.h"
#include "scriptapp.h"
#include "scriptgui.h"
#include "scriptgraphics.h"
#include "scriptmenu.h"
#include "scripttoolbar.h"
#include "scriptstatusbar.h"
#include "scriptbitmap.h"
#include "scriptlayout.h"
#include "scriptfont.h"
#include <wx/evtloop.h>
#include <kl/thread.h>


// utility functions

int getUniqueScriptControlId()
{
    static int id = ID_FirstScriptControlId;
    return id++;
}

int getUniqueScriptCommandId()
{
    static int id = ID_FirstScriptCommandId;
    return id++;
}



BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_FORMEVENTHANDLER_DESTROYSELF, 0)
END_DECLARE_EVENT_TYPES()


// FormEventHandler class implementation

DEFINE_EVENT_TYPE(wxEVT_FORMEVENTHANDLER_DESTROYSELF)

FormEventHandler::FormEventHandler(FormComponent* owner)
{
    m_owner = owner;
}

FormEventHandler::~FormEventHandler()
{
    int i = 5;
}

void FormEventHandler::safeDestroy()
{
    // any further event processing should be avoided
    m_owner = NULL;
    
    // destroy |this| later and in the main thread
    Connect(wxID_ANY,
            wxEVT_FORMEVENTHANDLER_DESTROYSELF,
            (wxObjectEventFunction)
            (wxEventFunction)
            &FormEventHandler::onEvent,
            NULL,
            this);
                     
    wxCommandEvent evt;
    evt.SetEventType(wxEVT_FORMEVENTHANDLER_DESTROYSELF);
    ::wxPostEvent(this, evt);
}

void FormEventHandler::onEvent(wxEvent& evt)
{
    if (evt.GetEventType() == wxEVT_FORMEVENTHANDLER_DESTROYSELF)
    {
        if (!wxPendingDelete.Member(this))
            wxPendingDelete.Append(this);
        return;
    }
    
    if (m_owner)
        m_owner->onEvent(evt);
}

void FormEventHandler::onFormClose(bool* allow)   // only used for Form components
{
    if (m_owner)
    {
        *allow = ((Form*)m_owner)->onFormFrameClose();
    }
}

void FormEventHandler::onFormDestructing()        // only used for Form components
{
    if (m_owner)
    {
        ((Form*)m_owner)->onFormFrameDestructing();
    }
}


// FormComponent class implementation

FormComponent::FormComponent()
{
    m_form_wnd = NULL;
    m_wnd = NULL;
    m_x = 0;
    m_y = 0;
    m_width = 0;
    m_height = 0;
    m_enabled = true;
    m_evt_handler = new FormEventHandler(this);
}

FormComponent::~FormComponent()
{
    m_evt_handler->safeDestroy();
}


void FormComponent::setFormWindow(wxWindow* wnd)
{
    m_form_wnd = wnd;
}


wxWindow* FormComponent::getWindow()
{
    return m_wnd;
}

void FormComponent::listenEvent(wxWindow* wnd, int event_id, int id, int id2)
{
    if (id2 != -1)
    {
        wnd->Connect(id,
                     id2,
                     event_id,
                     (wxObjectEventFunction)
                     (wxEventFunction)
                     &FormEventHandler::onEvent,
                     NULL,
                     m_evt_handler);
    }
     else
    {
        wnd->Connect(id,
                     event_id,
                     (wxObjectEventFunction)
                     (wxEventFunction)
                     &FormEventHandler::onEvent,
                     NULL,
                     m_evt_handler);
    }
}

void FormComponent::listenEvent(int event_id, int id, int id2)
{
    listenEvent(m_wnd, event_id, id, id2);
}



void FormComponent::deinitializeControl()
{
    m_wnd = NULL;
}










// FormPanel class

class FormPanel : public wxPanel
{
public:

    FormPanel(wxWindow* parent,
              wxWindowID id,
              const wxPoint& pos,
              const wxSize& size)
                  : wxPanel(parent, id, pos, size)
    {
    }

    ~FormPanel()
    {
    }

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(FormPanel, wxPanel)
END_EVENT_TABLE()




// FormFrame class implementation

BEGIN_EVENT_TABLE(FormFrame, wxFrame)
    EVT_CLOSE(FormFrame::onClose)
    EVT_PAINT(FormFrame::onPaint)
    EVT_MENU(ID_DoDestroy, FormFrame::onDoDestroy)
END_EVENT_TABLE()


FormFrame::FormFrame(Form* owner,
                     wxWindow* parent,
                     wxWindowID id,
                     const wxString& caption,
                     const wxPoint& pos,
                     const wxSize& size)
                      : wxFrame(parent, id, caption,
                                pos, size,
                                wxDEFAULT_FRAME_STYLE |
                                wxNO_FULL_REPAINT_ON_RESIZE |
                                wxFRAME_FLOAT_ON_PARENT)
{
    //m_owner = owner;
    //m_owner->baseRef();
    
    m_panel = new FormPanel(this, -1,
                            wxDefaultPosition,
                            wxSize(0,0));
    m_form_enabled = true;
}

FormFrame::~FormFrame()
{
    sigFormDestructing();

   // if (m_owner)
   // {
   //     m_owner->onFormFrameDestructing();
   //     m_owner->baseUnref();
   // }
}

wxPanel* FormFrame::getPanel()
{
    return m_panel;
}

void FormFrame::enableForm(bool enabled)
{
    #ifdef WIN32
        // disabling the window this way allows the
        // window to be moved even while the script is
        // busy.  This provides a more fluid feel
        ::EnableWindow((HWND)m_panel->GetHandle(), enabled ? TRUE : FALSE);
    #else
        // eventually we should find a solution for this
        // on linux, but right now it causes ugly disabling
        //Enable(enabled);
    #endif
    
    m_form_enabled = enabled;
}


void FormFrame::onPaint(wxPaintEvent& evt)
{
    evt.Skip();
}

void FormFrame::onSize(wxSizeEvent& evt)
{
    m_panel->SetSize(GetClientSize());
}

void FormFrame::onClose(wxCloseEvent& evt)
{
    if (!m_form_enabled)
        return;

/*
    if (m_owner)
    {
        bool allow = m_owner->onFormFrameClose();
        if (!allow)
            return;
    }

    Destroy();
*/

    bool allow = true;
    sigFormClose(&allow);
    if (allow)
    {
        Destroy();
    }
}

void FormFrame::safeOwnerDestroy()
{
    //if (m_owner)
    //{
    //    m_owner->baseUnref();
    //    m_owner = NULL;
    //}

    // fire an event to ourselves
    wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, ID_DoDestroy);
    ::wxPostEvent(this, e);
}

void FormFrame::onDoDestroy(wxCommandEvent& evt)
{
    Destroy();
}




// FormControl class implementation

// (CLASS) FormControl
// Category: Form
// Description: A class that represents a base form control component from 
//     which other controls derive.
// Remarks: The form control component encapsulates functions common to form
//     controls such as position, size, focus, layout, and update functions.

FormControl::FormControl()
{
    m_last_mouse_x = -1;
    m_last_mouse_y = -1;
}

FormControl::~FormControl()
{
}

// (CONSTRUCTOR) FormControl.constructor
// Description: Creates a new FormControl object.
//
// Syntax: FormControl()
//
// Remarks: Creates a new FormControl object.

void FormControl::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    initComponent(env);
    
    // PaintEvent needs to be created differently, 
    // because it's not a real js class
    PaintEvent* paint_evt = new PaintEvent;
    paint_evt->setParser(env->getParser());
    getParser()->bindObject(paint_evt);
    
    // add form control properties
    getMember(L"sizeChanged")->setObject(Event::createObject(env));
    getMember(L"paint")->setObject(paint_evt);
    getMember(L"mouseMove")->setObject(Event::createObject(env));
    getMember(L"mouseEnter")->setObject(Event::createObject(env));
    getMember(L"mouseLeave")->setObject(Event::createObject(env));
    getMember(L"mouseWheel")->setObject(Event::createObject(env));
    getMember(L"mouseLeftDoubleClick")->setObject(Event::createObject(env));
    getMember(L"mouseLeftDown")->setObject(Event::createObject(env));
    getMember(L"mouseLeftUp")->setObject(Event::createObject(env));
    getMember(L"mouseMiddleDoubleClick")->setObject(Event::createObject(env));
    getMember(L"mouseMiddleDown")->setObject(Event::createObject(env));
    getMember(L"mouseMiddleUp")->setObject(Event::createObject(env));
    getMember(L"mouseRightDoubleClick")->setObject(Event::createObject(env));
    getMember(L"mouseRightDown")->setObject(Event::createObject(env));
    getMember(L"mouseRightUp")->setObject(Event::createObject(env));
}

// (METHOD) FormControl.setFocus
// Description: Sets the focus to the form control.
//
// Syntax: function FormControl.setFocus()
//
// Remarks: Sets the focus to the form control.

void FormControl::setFocus(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        m_wnd->SetFocus();
    }
}

// (METHOD) FormControl.show
// Description: Shows or hides the form control.
//
// Syntax: function FormControl.show(flag : Boolean)
//
// Remarks: Shows the form control if the |flag| is true.  Hides the
//     form control if the |flag| is false.  If |flag| isn't specified,
//     the form control is shown.
//
// Param(flag): A |flag| which indicates whether to show or hide
//     the form control.

void FormControl::show(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        bool do_show = true;
        
        if (env->getParamCount() > 0)
            do_show = env->getParam(0)->getBoolean();
            
        m_wnd->Show(do_show);
    }
}

// (METHOD) FormControl.setEnabled
// Description: Enables or disables the form control.
//
// Syntax: function FormControl.setEnabled(flag : Boolean)
//
// Remarks: Enables the form control if |flag| is true. Disables
//     the form control if |flag| is false.  If |flag| isn't specified,
//     the form control is disabled.
//
// Param(flag): A |flag| which indicates whether or not to enable the
//     form control.

void FormControl::setEnabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        m_enabled = true;
        
        if (env->getParamCount() > 0)
            m_enabled = env->getParam(0)->getBoolean();

        m_wnd->Enable(m_enabled);
    }
}

// (METHOD) FormControl.getEnabled
// Description: Indicates whether or not a form control is enabled.
//
// Syntax: function FormControl.getEnabled() : Boolean
//
// Remarks: Returns true if a form control is enabled.  Returns false
//     if a form control is not enabled.
//
// Returns: Returns true if a form control is enabled, and false otherwise.

void FormControl::getEnabled(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        m_enabled = m_wnd->IsEnabled();
    }

    retval->setBoolean(m_enabled);
}

// (METHOD) FormControl.setPosition
// Description: Sets the position of a form control relative to the
//     the form control's parent.
//
// Syntax: function FormControl.setPosition(point : Point)
// Syntax: function FormControl.setPosition(x : Integer,
//                                          y : Integer)
//
// Remarks: Sets the position of a form control relative its parent
//      from the specified |point| or (|x|, |y|) position.
//
// Param(point): The position to which to set the form control.
// Param(x): The |x| portion of the position to which to set the form control.
// Param(y): The |y| portion of the position to which to set the form control.

void FormControl::setPosition(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
    {
        // incorrect syntax used; leave
        return;
    }
    
    int new_x = -1;
    int new_y = -1;
    int new_w = -1;
    int new_h = -1;
    
    if (env->getParamCount() == 1 && env->getParam(0)->isObject())
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->getClassName() == L"Point")
        {
            new_x = obj->getMember(L"x")->getInteger();
            new_y = obj->getMember(L"y")->getInteger();
        }
         else
        {
            // incorrect syntax used; leave
            return;
        }
    }
     else if (env->getParamCount() >= 2)
    {
        new_x = env->getParam(0)->getInteger();
        new_y = env->getParam(1)->getInteger();
        
        if (env->getParamCount() >= 4)
        {
            new_w = env->getParam(2)->getInteger();
            new_h = env->getParam(3)->getInteger();
        }
    }

    setPositionInt(new_x, new_y, new_w, new_h);
}

void FormControl::setPositionInt(int new_x, int new_y, int new_w, int new_h)
{
    if (new_x != -1)
        m_x = new_x;
    if (new_y != -1)
        m_y = new_y;
    if (new_w != -1)
        m_width = new_w;
    if (new_h != -1)
        m_height = new_h;
    
    if (isControlValid())
    {
        if (new_w == -1 || new_y == -1)
        {
            m_wnd->Move(wxPoint(new_x, new_y));
        }
         else
        {
            m_wnd->SetSize(new_x, new_y, new_w, new_h);
        }
    }
}

// (METHOD) FormControl.getPosition
// Description: Gets the position of a form control.
//
// Syntax: function FormControl.getPosition() : Point
//
// Remarks: Returns the position of a form control, relative
//     to its parent.
//
// Returns: Returns the position of a form control, relative
//     to its parent.

void FormControl::getPosition(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        m_wnd->GetPosition(&m_x, &m_y);
    }

    kscript::Value val;
    env->createObject(L"Point", &val);
    val.getMember(L"x")->setInteger(m_x);
    val.getMember(L"y")->setInteger(m_y);
    retval->setValue(val);
}

// (METHOD) FormControl.setSize
// Description: Sets the size of a form control.
//
// Syntax: function FormControl.setSize(size : Size)
// Syntax: function FormControl.setSize(width : Integer,
//                                      height : Integer)
//
// Remarks: Sets the size of a form control to the specified
//     |size| or |width| and |height| values.
//
// Param(size): The |size| to which to set the form control.
// Param(width): The |width| to which to set the form control.
// Param(height): The |height| to which to set the form control.

void FormControl::setSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 ||
        env->getParamCount() > 2)
    {
        // incorrect syntax used; leave
        return;
    }
    
    if (env->getParamCount() == 1)
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->getClassName() == L"Size")
        {
            m_width = obj->getMember(L"width")->getInteger();
            m_height = obj->getMember(L"height")->getInteger();
        }
         else
        {
            // incorrect syntax used; leave
            return;
        }
    }
     else if (env->getParamCount() == 2)
    {
        m_width = env->getParam(0)->getInteger();
        m_height = env->getParam(1)->getInteger();
    }

    if (isControlValid())
    {
        m_wnd->SetSize(wxSize(m_width, m_height));
    }
}

// (METHOD) FormControl.getSize
// Description: Gets the size of the form control.
//
// Syntax: function FormControl.getSize() : Size
//
// Remarks: Returns the size of the form control.
//
// Returns: Returns the size of the form control.

void FormControl::getSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        m_wnd->GetSize(&m_width, &m_height);
    }

    kscript::Value val;
    env->createObject(L"Size", &val);
    val.getMember(L"width")->setInteger(m_width);
    val.getMember(L"height")->setInteger(m_height);
    retval->setValue(val);
}

// (METHOD) FormControl.setMinSize
// Description: Sets the minimum size of a form control.
//
// Syntax: function FormControl.setMinSize(size : Size)
// Syntax: function FormControl.setMinSize(width : Integer,
//                                         height : Integer)
//
// Remarks: Sets the minimum size of a form control to the specified
//     |size| or |width| and |height| values.
//
// Param(size): The minimum |size| to which to set the form control.
// Param(width): The minimum |width| to which to set the form control.
// Param(height): The minimum |height| to which to set the form control.


void FormControl::setMinSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 ||
        env->getParamCount() > 2)
    {
        // incorrect syntax used; leave
        return;
    }
    
    if (env->getParamCount() == 1)
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->getClassName() == L"Size")
        {
            m_min_width = obj->getMember(L"width")->getInteger();
            m_min_height = obj->getMember(L"height")->getInteger();
        }
         else
        {
            // incorrect syntax used; leave
            return;
        }
    }
     else if (env->getParamCount() == 2)
    {
        m_min_width =  env->getParam(0)->getInteger();
        m_min_height =  env->getParam(1)->getInteger();
    }

    if (isControlValid())
    {
        m_wnd->SetMinSize(wxSize(m_min_width, m_min_height));
    }
}

// (METHOD) FormControl.getMinSize
// Description: Gets the minimum size of the form control.
//
// Syntax: function FormControl.getMinSize() : Size
//
// Remarks: Returns the minimum size of the form control.
//
// Returns: Returns the minimum size of the form control.

void FormControl::getMinSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        wxSize s = m_wnd->GetMinSize();
        m_min_width = s.GetWidth();
        m_min_height = s.GetHeight();
    }

    kscript::Value val;
    env->createObject(L"Size", &val);
    val.getMember(L"width")->setInteger(m_min_width);
    val.getMember(L"height")->setInteger(m_min_height);
    retval->setValue(val);
}

// (METHOD) FormControl.setMaxSize
// Description: Sets the maximum size of a form control.
//
// Syntax: function FormControl.setMaxSize(size : Size)
// Syntax: function FormControl.setMaxSize(width : Integer,
//                                         height : Integer)
//
// Remarks: Sets the maximum size of a form control to the specified
//     |size| or |width| and |height| values.
//
// Param(size): The maximum |size| to which to set the form control.
// Param(width): The maximum |width| to which to set the form control.
// Param(height): The maximum |height| to which to set the form control.

void FormControl::setMaxSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 ||
        env->getParamCount() > 2)
    {
        // incorrect syntax used; leave
        return;
    }
    
    if (env->getParamCount() == 1)
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->getClassName() == L"Size")
        {
            m_max_width = obj->getMember(L"width")->getInteger();
            m_max_height = obj->getMember(L"height")->getInteger();
        }
         else
        {
            // incorrect syntax used; leave
            return;
        }
    }
     else if (env->getParamCount() == 2)
    {
        m_max_width =  env->getParam(0)->getInteger();
        m_max_height =  env->getParam(1)->getInteger();
    }

    if (isControlValid())
    {
        m_wnd->SetMaxSize(wxSize(m_max_width, m_max_height));
    }
}

// (METHOD) FormControl.getMaxSize
// Description: Gets the maximum size of the form control.
//
// Syntax: function FormControl.getMaxSize() : Size
//
// Remarks: Returns the maximum size of the form control.
//
// Returns: Returns the maximum size of the form control.

void FormControl::getMaxSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        wxSize s = m_wnd->GetMaxSize();
        m_max_width = s.GetWidth();
        m_max_height = s.GetHeight();
    }

    kscript::Value val;
    env->createObject(L"Size", &val);
    val.getMember(L"width")->setInteger(m_max_width);
    val.getMember(L"height")->setInteger(m_max_height);
    retval->setValue(val);
}

// (METHOD) FormControl.setClientSize
// Description: Sets the client size of a form control.
//
// Syntax: function FormControl.setClientSize(size : Size)
// Syntax: function FormControl.setClientSize(width : Integer,
//                                            height : Integer)
//
// Remarks: Sets the client size of a form control to the specified
//     |size| or |width| and |height| values.
//
// Param(size): The client |size| to which to set the form control.
// Param(width): The client |width| to which to set the form control.
// Param(height): The client |height| to which to set the form control.

void FormControl::setClientSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 ||
        env->getParamCount() > 2)
    {
        // incorrect syntax used; leave
        return;
    }
    
    if (env->getParamCount() == 1)
    {
        kscript::ValueObject* obj = env->getParam(0)->getObject();
        if (obj->getClassName() == L"Size")
        {
            m_cliwidth = obj->getMember(L"width")->getInteger();
            m_cliheight = obj->getMember(L"height")->getInteger();
        }
         else
        {
            // incorrect syntax used; leave
            return;
        }
    }
     else if (env->getParamCount() == 2)
    {
        m_cliwidth = env->getParam(0)->getInteger();
        m_cliheight = env->getParam(1)->getInteger();
    }

    if (isControlValid())
    {
        m_wnd->SetClientSize(wxSize(m_cliwidth, m_cliheight));
    }
}

// (METHOD) FormControl.getClientSize
// Description: Gets the client size of the form control.
//
// Syntax: function FormControl.getClientSize() : Size
//
// Remarks: Returns the client size of the form control.
//
// Returns: Returns the client size of the form control.

void FormControl::getClientSize(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (isControlValid())
    {
        m_wnd->GetClientSize(&m_cliwidth, &m_cliheight);
    }

    kscript::Value val;
    env->createObject(L"Size", &val);
    val.getMember(L"width")->setInteger(m_cliwidth);
    val.getMember(L"height")->setInteger(m_cliheight);
    retval->setValue(val);
}

// (METHOD) FormControl.setBackgroundColor
// Description: Sets the background color of the form control.
//
// Syntax: function FormControl.setBackgroundColor(color : Color)
//
// Remarks: Sets the background color of the form control to the
//     specified |color|.
//
// Param(color): The |color| to which to set the background of the
//     specified form control.

void FormControl::setBackgroundColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxWindow* action_wnd = m_wnd;
    if (isKindOf(L"Form"))
        action_wnd = ((Form*)this)->m_form_panel;
    if (!action_wnd)
        return;
        
    wxColor color;
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    if (obj->getClassName() == L"Color")
    {
        Color* col = (Color*)obj;
        color = wxColor(col->getMember(L"red")->getInteger(),
                        col->getMember(L"green")->getInteger(),
                        col->getMember(L"blue")->getInteger());
        action_wnd->SetBackgroundColour(color);
    }
}

// (METHOD) FormControl.getBackgroundColor
// Description: Gets the background color of the form control.
//
// Syntax: function FormControl.getBackgroundColor() : Color
//
// Remarks: Returns the background color of the form control.
//
// Returns: Returns the background color of the form control.

void FormControl::getBackgroundColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxWindow* action_wnd = m_wnd;
    if (isKindOf(L"Form"))
        action_wnd = ((Form*)this)->m_form_panel;
        
    Color* col = Color::createObject(env);
    
    wxColor color(255,255,255);
    
    if (action_wnd)
    {
        color = action_wnd->GetBackgroundColour();
    }
    
    col->getMember(L"red")->setInteger(color.Red());
    col->getMember(L"green")->setInteger(color.Green());
    col->getMember(L"blue")->setInteger(color.Blue());
        
    retval->setObject(col);
}

// (METHOD) FormControl.setForegroundColor
// Description: Sets the foreground color of the form control.
//
// Syntax: function FormControl.setForegroundColor(color : Color)
//
// Remarks: Sets the foreground color of the form control to the
//     specified |color|.
//
// Param(color): The |color| to which to set the foreground of the
//     specified form control.

void FormControl::setForegroundColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxWindow* action_wnd = m_wnd;
    if (isKindOf(L"Form"))
        action_wnd = ((Form*)this)->m_form_panel;
    if (!action_wnd)
        return;
        
    wxColor color;
    
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    
    if (obj->getClassName() == L"Color")
    {
        Color* col = (Color*)obj;
        color = wxColor(col->getMember(L"red")->getInteger(),
                        col->getMember(L"green")->getInteger(),
                        col->getMember(L"blue")->getInteger());
        action_wnd->SetForegroundColour(color);
    }
}

// (METHOD) FormControl.getForegroundColor
// Description: Gets the foreground color of the form control.
//
// Syntax: function FormControl.getForegroundColor() : Color
//
// Remarks: Returns the foreground color of the form control.
//
// Returns: Returns the foreground color of the form control.

void FormControl::getForegroundColor(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxWindow* action_wnd = m_wnd;
    if (isKindOf(L"Form"))
        action_wnd = ((Form*)this)->m_form_panel;
    if (!action_wnd)
        return;
        
    Color* col = Color::createObject(env);
    
    wxColor color(0,0,0);
    
    if (action_wnd)
    {
        color = action_wnd->GetForegroundColour();
    }
    
    col->getMember(L"red")->setInteger(color.Red());
    col->getMember(L"green")->setInteger(color.Green());
    col->getMember(L"blue")->setInteger(color.Blue());
        
    retval->setObject(col);
}

// (METHOD) FormControl.setFont
// Description: Sets the default font for the text of the form control.
//
// Syntax: function FormControl.setFont(font : Font)
//
// Remarks: Sets the default |font| for the text of the form control.
//
// Param(font): The default |font| to use when rendering the text of the form control.

void FormControl::setFont(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxWindow* action_wnd = m_wnd;
    if (isKindOf(L"Form"))
        action_wnd = ((Form*)this)->m_form_panel;
        
    if (env->getParamCount() < 1)
        return;
            
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"Font")
    {
        zFont* font = (zFont*)obj;
        
        m_font = font->getWxFont();
        
        if (action_wnd)
        {
            action_wnd->SetFont(m_font);
        }
    }
}

// (METHOD) FormControl.getFont
// Description: Gets the default font for the text of form control.
//
// Syntax: function FormControl.getFont() : Font
//
// Remarks: Returns the default font used for the text of the form control.
//
// Returns: Returns the default font used for the text of the form control.

void FormControl::getFont(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxWindow* action_wnd = m_wnd;
    if (isKindOf(L"Form"))
        action_wnd = ((Form*)this)->m_form_panel;

        
    wxFont wx_font = *wxNORMAL_FONT;
    
    if (action_wnd)
    {
        wx_font = action_wnd->GetFont();
    }

    zFont* font = zFont::createObject(env);
    font->setWxFont(wx_font);
    retval->setObject(font);
}

// (METHOD) FormControl.invalidate
// Description: Invalidates a form control, which will cause it to be repainted
//     on the next paint event.
//
// Syntax: function FormControl.invalidate()
//
// Remarks: Invalidates a form control, which will cause it to be repainted
//     on the next paint event.

void FormControl::invalidate(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_wnd)
    {
        m_wnd->Refresh(true);
    }
}

// (METHOD) FormControl.update
// Description: Updates a form control, which will immediately repaint any
//     invalid areas.
//
// Syntax: function FormControl.update()
//
// Remarks: Updates a form control, which will immediately repaint any
//     invalid areas.

void FormControl::update(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_wnd)
    {
        m_wnd->Update();
    }
}

// (METHOD) FormControl.refresh
// Description: Refreshes a form control, which immediately repaints the
//     entire form control.
//
// Syntax: function FormControl.refresh()
//
// Remarks: Refreshes a form control, which immediately repaints the
//     entire form control.

void FormControl::refresh(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_wnd)
    {
        fireOnPaint();
    }
}

// (METHOD) FormControl.captureMouse
// Description: Captures the mouse on this form control.
//
// Syntax: function FormControl.captureMouse()
//
// Remarks: Captures the mouse on this form control.

void FormControl::captureMouse(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_wnd)
    {
        m_wnd->CaptureMouse();
    }
}

// (METHOD) FormControl.releaseMouse
// Description: Releases the mouse from being captured on this form control.
//
// Syntax: function FormControl.releaseMouse()
//
// Remarks: Releases the mouse from being captured on this form control.

void FormControl::releaseMouse(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_wnd)
    {
        if (wxWindow::GetCapture() == m_wnd)
        {
            m_wnd->ReleaseMouse();
        }
    }
}


// (METHOD) FormControl.disablePaint
// Description: Disables the window from redrawing itself.
//
// Syntax: function FormControl.disablePaint()
//
// Remarks: Calling disablePaint causes the window not to redraw.  This
//     can be useful in controlling flicker when dealing with form elements.
//     Each disablePaint call should have a corresponding enablePaint call.


void FormControl::disablePaint(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_wnd)
        m_wnd->Freeze();
}


// (METHOD) FormControl.enablePaint
// Description: Enables the window to redraw itself.
//
// Syntax: function FormControl.enablePaint()
//
// Remarks: Reenables window redraw which was masked by a call to disablePaint.
//     Each enablePaint call should have a corresponding disablePaint call.


void FormControl::enablePaint(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_wnd)
        m_wnd->Thaw();
}


// (METHOD) FormControl.getMousePosition
// Description: Gets the mouse position relative to this form control.
//
// Syntax: function FormControl.getMousePosition() : Point
//
// Remarks: Returns the mouse position relative to this form control.
//
// Returns: Returns the mouse position relative to this form control.

void FormControl::getMousePosition(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_wnd)
    {
        retval->setNull();
        return;
    }

    wxPoint pt = m_wnd->ScreenToClient(::wxGetMousePosition());

    Point* p = Point::createObject(env);
    p->getMember(L"x")->setInteger(pt.x);
    p->getMember(L"y")->setInteger(pt.y);
    retval->setObject(p);
}


// (METHOD) FormControl.getNativeHandle
// Description: Gets the native handle of the window/control
//
// Syntax: function FormControl.getNativeHandle() : Number
//
// Remarks: Returns the native handle of the window or control.  On Windows,
//     this is the value of the window's HWND handle.
//
// Returns: Returns the handle as an numeric value

void FormControl::getNativeHandle(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger((long)(void*)m_wnd->GetHandle());
}


// (EVENT) FormControl.mouseMove
// Description: Fired when the mouse is moved inside the control window.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseEnter
// Description: Fired when the mouse enters the control window.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseLeave
// Description: Fired when the mouse leaves the control window.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseWheel
// Description: Fired when the mouse wheel is moved.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseLeftDoubleClick
// Description: Fired when the left mouse button is double-clicked.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseMiddleDoubleClick
// Description: Fired when the middle mouse button is double-clicked.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseRightDoubleClick
// Description: Fired when the right mouse button is double-clicked.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseLeftDown
// Description: Fired when the left mouse button is down.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseMiddleDown
// Description: Fired when the middle mouse button is down.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseRightDown
// Description: Fired when the right mouse button is down.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseLeftUp
// Description: Fired when the left mouse button is released.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseMiddleUp
// Description: Fired when the middle mouse button is released.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.mouseRightUp
// Description: Fired when the right mouse button is released.
// Param(x): The x position of the mouse when the event was fired.
// Param(y): The y position of the mouse when the event was fired.
// Param(altDown): True if the Alt key was down when the event was fired.
// Param(controlDown): True if the Control key was down when the event was fired.
// Param(shiftDown): True if the Shift key was down when the event was fired.
// Param(leftDown): True if the left mouse button was down when the event was fired.
// Param(middleDown): True if the middle mouse button was down when the event was fired.
// Param(rightDown): True if the right mouse button was down when the event was fired.

// (EVENT) FormControl.sizeChanged
// Description: Fired when the form control size is changed.

// (EVENT) FormControl.paint
// Description: Fired when the form control is painted.
// Param(graphics): The graphics object of the form that is painted.

void FormControl::onEvent(wxEvent& event)
{
    wxEventType event_type = event.GetEventType();
    
    
    if (event_type == wxEVT_SIZE)
    {
        //m_form->onSize((wxSizeEvent&)event);
        invokeJsEvent(L"sizeChanged", NULL, eventFlagNoGuiBlock);
        return;
    }
     else if (event_type == wxEVT_COMMAND_MENU_SELECTED)
    {
        return;
    }
    
    
    if (event_type == wxEVT_MOTION ||
        event_type == wxEVT_ENTER_WINDOW ||
        event_type == wxEVT_LEAVE_WINDOW ||
        event_type == wxEVT_MOUSEWHEEL ||
        event_type == wxEVT_LEFT_DCLICK ||
        event_type == wxEVT_LEFT_DOWN ||
        event_type == wxEVT_LEFT_UP ||
        event_type == wxEVT_MIDDLE_DCLICK ||        
        event_type == wxEVT_MIDDLE_DOWN ||
        event_type == wxEVT_MIDDLE_UP ||
        event_type == wxEVT_RIGHT_DCLICK ||        
        event_type == wxEVT_RIGHT_DOWN ||
        event_type == wxEVT_RIGHT_UP)
    {
        // this whole chunk of code is devoted to mouse events
        
        wxMouseEvent& evt = (wxMouseEvent&)event;
        kscript::Value* event_args = new kscript::Value;
        event_args->setObject();
        event_args->getMember(L"altDown")->setBoolean(evt.AltDown());
        event_args->getMember(L"commandDown")->setBoolean(evt.CmdDown());
        event_args->getMember(L"shiftDown")->setBoolean(evt.ShiftDown());
        event_args->getMember(L"leftDown")->setBoolean(evt.LeftIsDown());
        event_args->getMember(L"middleDown")->setBoolean(evt.MiddleIsDown());
        event_args->getMember(L"rightDown")->setBoolean(evt.RightIsDown());
        event_args->getMember(L"scrollDistance")->setInteger(evt.GetWheelRotation());
        event_args->getMember(L"pixelsPerScroll")->setInteger(evt.GetWheelDelta());
        event_args->getMember(L"linesPerScroll")->setInteger(evt.GetLinesPerAction());
        event_args->getMember(L"x")->setInteger(evt.m_x);
        event_args->getMember(L"y")->setInteger(evt.m_y);
        
        if (event_type == wxEVT_MOTION)
        {
            // this comparison takes care of extra mouse down events being
            // fired.  I'm not certain if this is related to wx's wxUSE_MOUSEEVENT_HACK;
            // see wx/msw/window.cpp line 5205 or so for more information
            if (evt.m_x != m_last_mouse_x ||
                evt.m_y != m_last_mouse_y)
            {
                invokeJsEvent(L"mouseMove", event_args, eventFlagNoGuiBlock);
                m_last_mouse_x = evt.m_x;
                m_last_mouse_y = evt.m_y;
            }
        }
        else if (event_type == wxEVT_ENTER_WINDOW)
        {
            invokeJsEvent(L"mouseEnter", event_args, eventFlagNoGuiBlock);
            //m_last_mouse_x = -1;
            //m_last_mouse_y = -1;
        }
        else if (event_type == wxEVT_LEAVE_WINDOW)
        {
            invokeJsEvent(L"mouseLeave", event_args, eventFlagNoGuiBlock);
        }
        else if (event_type == wxEVT_MOUSEWHEEL)
            invokeJsEvent(L"mouseWheel", event_args);
        else if (event_type == wxEVT_LEFT_DCLICK)
            invokeJsEvent(L"mouseLeftDoubleClick", event_args);
        else if (event_type == wxEVT_LEFT_DOWN)
            invokeJsEvent(L"mouseLeftDown", event_args, eventFlagNoGuiBlock);
        else if (event_type == wxEVT_LEFT_UP)
            invokeJsEvent(L"mouseLeftUp", event_args);
        else if (event_type == wxEVT_MIDDLE_DCLICK)
            invokeJsEvent(L"mouseMiddleDoubleClick", event_args);
        else if (event_type == wxEVT_MIDDLE_DOWN)
            invokeJsEvent(L"mouseMiddleDown", event_args);
        else if (event_type == wxEVT_MIDDLE_UP)
            invokeJsEvent(L"mouseMiddleUp", event_args);
        else if (event_type == wxEVT_RIGHT_DCLICK)
            invokeJsEvent(L"mouseRightDoubleClick", event_args);
        else if (event_type == wxEVT_RIGHT_DOWN)
            invokeJsEvent(L"mouseRightDown", event_args);
        else if (event_type == wxEVT_RIGHT_UP)
            invokeJsEvent(L"mouseRightUp", event_args);
        else
        {
            // this event type isn't handeled
            delete event_args;
        }
    }
}




// Form class implementation

// (CLASS) Form
// Category: Form
// Derives: FormControl
// Description: A class that represents a form on which controls can be placed.
// Remarks: Represents a form on which controls can be placed.

Form::Form()
{
    m_form_frame = NULL;
    m_form_panel = NULL;
    m_menubar = NULL;
    m_toolbar = NULL;
    m_layout = NULL;
    m_first_time = true;
    m_modal_result = DialogResult::Cancel;
    m_showdialog_run = false;
    m_showdialog_running = false;
}

Form::~Form()
{
    deinitializeControl();
}






wxWindow* Form::getFormPanel()
{
    return m_form_panel;
}


// (CONSTRUCTOR) Form.constructor
// Description: Creates a new Form.
//
// Syntax: Form(caption : String,
//              x_pos : Integer,
//              y_pos : Integer,
//              width : Integer,
//              height : Integer)
//
// Remarks: Creates a new Form with the specified |caption|,
//     at the position specified by |x_pos| and |y_pos|, and 
//     having dimensions specified by |width| and |height|.
//
// Param(caption): The |caption| of the form.
// Param(x_pos): The x position of the form.
// Param(y_pos): The y position of the form.
// Param(width): The width of the form.
// Param(height): The height of the form.

void Form::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // call base class constructor
    FormControl::constructor(env, retval);

    // set default values
    wxString caption = wxT("");
    m_x = 100;
    m_y = 100;
    m_width = 800;
    m_height = 600;
    
    // get user input values
    if (env->getParamCount() > 0)
    {
        caption = env->getParam(0)->getString();
        m_caption = caption;
    }
    
    if (env->getParamCount() > 1)
        m_x = env->getParam(1)->getInteger();
        
    if (env->getParamCount() > 2)
        m_y = env->getParam(2)->getInteger();
    if (env->getParamCount() > 3)
        m_width = env->getParam(3)->getInteger();
    if (env->getParamCount() > 4)
        m_height = env->getParam(4)->getInteger();

    m_form_frame = new FormFrame(this,
                           g_app->getMainWindow(),
                           -1,
                           caption,
                           wxPoint(m_x, m_y),
                           wxSize(m_width, m_height));

    m_form_frame->sigFormClose.connect(m_evt_handler, &FormEventHandler::onFormClose);
    m_form_frame->sigFormDestructing.connect(m_evt_handler, &FormEventHandler::onFormDestructing);

    m_wnd = m_form_frame;
    m_form_panel = m_form_frame->getPanel();

    //set default icon
    m_form_frame->SetIcon(g_app->getMainFrame()->getFrameWindow()->GetIcon());

    // listen for events
    //listenEvent(wxEVT_CLOSE_WINDOW);
    listenEvent(wxEVT_COMMAND_MENU_SELECTED, 30000, 31999);
    listenEvent(m_form_panel, wxEVT_SIZE);
    listenEvent(m_form_panel, wxEVT_MOTION);
    listenEvent(m_form_panel, wxEVT_ENTER_WINDOW);
    listenEvent(m_form_panel, wxEVT_LEAVE_WINDOW);
    listenEvent(m_form_panel, wxEVT_MOUSEWHEEL);
    listenEvent(m_form_panel, wxEVT_LEFT_DCLICK);
    listenEvent(m_form_panel, wxEVT_LEFT_DOWN);
    listenEvent(m_form_panel, wxEVT_LEFT_UP);
    listenEvent(m_form_panel, wxEVT_MIDDLE_DCLICK);
    listenEvent(m_form_panel, wxEVT_MIDDLE_DOWN);
    listenEvent(m_form_panel, wxEVT_MIDDLE_UP);
    listenEvent(m_form_panel, wxEVT_RIGHT_DCLICK);
    listenEvent(m_form_panel, wxEVT_RIGHT_DOWN);
    listenEvent(m_form_panel, wxEVT_RIGHT_UP);
}

void Form::realize()
{
    // this is called only when a form is added to another form
    m_form_panel->Reparent(m_form_wnd);
    
    if (m_form_wnd->IsKindOf(CLASSINFO(FormPanel)))
    {
        m_form_panel->SetSize(m_x, m_y, m_width, m_height);
    }
     else
    {
        m_x = 0;
        m_y = 0;
        m_form_panel->SetSize(m_x, m_y, m_width, m_height);
    }
    
    m_form_frame->m_owner = NULL;
    m_form_frame->sigFormClose.disconnect();
    m_form_frame->sigFormDestructing.disconnect();
    m_form_frame->Destroy();
    
    m_form_panel->Show();
    m_form_frame = NULL;
    m_wnd = m_form_panel;
}

void Form::deinitializeControl()
{
    // form frame is being destroyed, maybe not by our
    // own hands.  We need to release our references to
    // other objects
    
    std::vector<FormComponent*>::iterator it;
    for (it = m_form_elements.begin(); it != m_form_elements.end(); ++it)
    {
        ((FormComponent*)(*it))->deinitializeControl();
        (*it)->baseUnref();
    }
    m_form_elements.clear();
    

    if (m_layout)
    {
        m_layout->deinitializeControl();
        m_layout->baseUnref();
        m_layout = NULL;
    }
    
    Application* app = getApp();
    if (app)
    {
        app->unregisterForm(this);
    }
    
    m_wnd = NULL;
}


void Form::setSite(IDocumentSitePtr site)
{
    m_site = site;
    
    m_form_panel->SetSize(0, 0, m_width, m_height);
    m_x = 0;
    m_y = 0;
    
    if (m_site.isOk())
    {
        m_site->setCaption(m_caption);
    }
}

void Form::onEvent(wxEvent& evt)
{
    if (evt.GetEventType() == wxEVT_SIZE)
    {
        if (m_form_frame)
        {
            m_form_frame->onSize((wxSizeEvent&)evt);
        }
        
        if (0 == invokeJsEvent(L"sizeChanged", NULL, eventFlagNoGuiBlock))
        {
            if (m_form_panel)
            {
                m_form_panel->Layout();
            }
        }
        
        return;
    }
    
    FormControl::onEvent(evt);
}


bool Form::onFormFrameClose()
{
    if (m_showdialog_running)
    {
        // if there is a modal dialog running, cause
        // it to exit and return cancel
        m_modal_result = DialogResult::Cancel;
        getApp()->exitModal();
        
        // don't allow default processing, as the form will
        // get destroyed later by the modal loop
        return false;
    }
    
    // allow default processing (Destroy)
    return true;
}


void Form::onFormFrameDestructing()
{
    deinitializeControl();
    
    m_form_frame = NULL;
    m_form_panel = NULL;
    m_form_wnd = NULL;
    m_wnd = NULL;
}

void Form::enableForm(bool enabled)
{
    if (!m_form_frame)
        return;
 
    m_form_frame->enableForm(enabled);
}

// (METHOD) Form.setCaption
// Description: Sets the caption for the form.
//
// Syntax: function Form.setCaption(text : String)
//
// Remarks: Sets the form's caption to |text|.
//
// Param(text): The |text| to which to set the form's caption.

void Form::setCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_caption = env->getParam(0)->getString();
    
    if (m_form_frame)
    {
        m_form_frame->SetTitle(m_caption);
    }
    
    if (m_site)
    {
        m_site->setCaption(m_caption);
    }
}

// (METHOD) Form.getCaption
// Description: Gets the form's caption.
//
// Syntax: function Form.getCaption() : String
//
// Remarks: Returns the form's caption.
//
// Returns: Returns the form's caption.

void Form::getCaption(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setString(towstr(m_caption));
}

// (METHOD) Form.setMenuBar
// Description: Sets the MenuBar object to use for the form.
//
// Syntax: function Form.setMenuBar(object : MenuBar)
//
// Remarks: Sets the MenuBar |object| to use for the form's menu bar.
//
// Param(object): The MenuBar |object| to use for the form's menu bar.

void Form::setMenuBar(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"MenuBar")
    {
        MenuBar* m = (MenuBar*)obj;
        m_form_frame->SetMenuBar(m->getWxMenuBar());
        
        // store the menubar for menuitem lookup
        m_menubar = m;
    }
}

// (METHOD) Form.setToolBar
// Description: Sets the ToolBar object to use for the form.
//
// Syntax: function Form.setToolBar(object : ToolBar)
//
// Remarks: Sets the ToolBar |object| to use for the form's toolbar.
//
// Param(object): The ToolBar |object| to use for the form's toolbar.

void Form::setToolBar(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"ToolBar")
    {
        ToolBar* t = (ToolBar*)obj;
        t->setFormWindow(m_form_frame);
        t->realize();
        m_form_frame->SetToolBar(t->getWxToolBar());
        
        m_toolbar = t;
    }
}

// (METHOD) Form.setStatusBar
// Description: Sets the StatusBar object to use for the form.
//
// Syntax: function Form.setStatusBar(object : StatusBar)
//
// Remarks: Sets the StatusBar |object| to use for the form's status bar.
//
// Param(object): The StatusBar |object| to use for the form's status bar.

void Form::setStatusBar(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->getClassName() == L"StatusBar")
    {
        zStatusBar* s = (zStatusBar*)obj;
        s->setFormWindow(m_form_frame);
        s->realize();
        
        // we are calling Reparent() here because the StatusBar class
        // doesn't have access to the FormFrame object because m_form_wnd
        // is actually the wxPanel inside of FormFrame
        s->getWxStatusBar()->Reparent(m_form_frame);

        s->realize();
        m_form_frame->SetStatusBar(s->getWxStatusBar());
    }
}

// (METHOD) Form.setLayout
// Description: Sets the layout object to use to position and size the
//     form's controls.
//
// Syntax: function Form.setLayout(object : Layout)
//
// Remarks: Sets the layout |object| to use to position and size the
//     form's controls.
//
// Param(object): The layout |object| to use to position and size the
//     form's controls.

void Form::setLayout(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::ValueObject* obj = env->getParam(0)->getObject();

    if (!obj)
        return;
        
    if ((obj->getClassName() == L"BoxLayout") ||
        (obj->getClassName() == L"BorderBoxLayout"))
    {
        Layout* layout = (Layout*)obj;
        
        layout->setFormWindow(m_form_panel);
        layout->realize();
        
        m_form_panel->SetSizer(layout->getSizer());
        m_form_panel->Layout();
        
        m_layout = layout;
        m_layout->baseRef();
    }
}

// (METHOD) Form.add
// Description: Adds a form control to the form.
//
// Syntax: function Form.add(control : FormControl)
//
// Remarks: Adds a form |control| to the form.
//
// Param(control): The form |control| to add to the form.

void Form::add(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;
        
    kscript::Value* object = env->getParam(0);
    if (!object || !object->isObject())
        return;

    kscript::ValueObject* bobj = object->getObject();
    if (bobj->isKindOf(L"FormControl"))
    {
        FormComponent* component = (FormComponent*)bobj;

        component->baseRef();
        m_form_elements.push_back(component);
        
        component->setFormWindow(m_form_panel);
        component->realize();
    }
}    

// (METHOD) Form.show
// Description: Shows or hides the form.
//
// Syntax: function Form.show(flag : Boolean)
//
// Remarks: Shows the form if the |flag| is true.  Hides the
//     form if the |flag| is false.  If |flag| isn't specified,
//     the form is shown.
//
// Param(flag): A |flag| which indicates whether to show or hide
//     the form.

// TODO: even though the override is necessary for the implementation,
//     the end effect to the user appears identical to FormControl::show();
//     do we need to expose the override in the documentation?

void Form::show(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_first_time)
    {
        m_first_time = false;
        
        Application* app = getApp();
        if (app)
        {
            app->registerForm(this);
        }
    }
    
    FormControl::show(env, retval);
}



class DialogLoopInMainThread : public wxEvtHandler
{
public:

    enum
    {
        ID_BeginDialogLoop = 8001,
        ID_EndDialogLoop = 8002,
        ID_ShowDialog = 8003
    };
    
    DialogLoopInMainThread(wxWindow* dialog_wnd)
    {
        m_loop_running = false;
        m_dialog_wnd = dialog_wnd;
        m_modal_loop = NULL;
    }
    
    bool ProcessEvent(wxEvent& evt)
    {
        if (evt.GetId() == ID_BeginDialogLoop)
        {
            wxCommandEvent e(0, ID_ShowDialog);
            ::wxPostEvent(this, e);
            
            m_modal_loop = new wxModalEventLoop(m_dialog_wnd);
            m_loop_running = true;
            m_modal_loop->Run();
            delete m_modal_loop;
            
            m_dialog_wnd->Destroy();
                            
            m_loop_running = false;
        }
         else if (evt.GetId() == ID_EndDialogLoop)
        {
            m_modal_loop->Exit();
        }
         else if (evt.GetId() == ID_ShowDialog)
        {
            m_dialog_wnd->Show();
        }

        
        return true;
    }
    
    bool IsRunning() const
    {
        return m_loop_running;
    }
    
public:

    bool m_loop_running;
    wxWindow* m_dialog_wnd;
    wxModalEventLoop* m_modal_loop;
};

// (METHOD) Form.showDialog
// Description: Displays the form as a modal dialog.
//
// Syntax: function Form.showDialog() : Integer
//
// Remarks: Calling showDialog() displays the form as a modal dialog.
//     Modal in this sense means that all mouse and keyboard input to
//     all background windows will be disabled until the user is finished
//     with the modal dialog form.  The dialog's modality can be ended
//     by calling endDialog(|return_code|).  Standard return codes are
//     DialogResult.OK and DialogResult.Cancel.  Once endDialog() is called
//     by the program, the showDialog() will return the value passed
//     as a parameter to endDialog().
//
// Returns: The integer value passed to endDialog()

void Form::showDialog(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    // don't allow showDialog() to be run twice
    if (m_showdialog_run)
        return;
    
    m_showdialog_run = true;
    m_showdialog_running = true;
    
    // start a modal loop in the main thread
    
    DialogLoopInMainThread* loop = new DialogLoopInMainThread(m_form_frame);
    wxCommandEvent e(0, DialogLoopInMainThread::ID_BeginDialogLoop);
    ::wxPostEvent(loop, e);
    
    // wait until it's running
    while (!loop->IsRunning())
        kl::thread::sleep(50);
    
    // ...meanwhile, start a new Application event pump in
    // the script's thread
    kscript::Value myret;
    getApp()->run(env, &myret);
    
    if (m_form_frame)
    {
        wxCommandEvent e2(0, DialogLoopInMainThread::ID_EndDialogLoop);
        ::wxPostEvent(loop, e2);
    }
    
    // wait until it's not running
    while (loop->IsRunning())
        kl::thread::sleep(50);
    
    if (!wxPendingDelete.Member(loop))
        wxPendingDelete.Append(loop);

    retval->setInteger(m_modal_result);
    
    // reset it for next time
    m_modal_result = DialogResult::Cancel;
    m_showdialog_running = false;
}


// (METHOD) Form.endDialog
// Description: Displays the form as a modal dialog.
//
// Syntax: function Form.endDialog(return_code : Integer) : Boolean
//
// Remarks: Calling endDialog() ends a modal dialog.  The value passed
//     in the |return_code| parameter is returned by showDialog().
//
// Returns: True if the call was successful, false otherwise.

void Form::endDialog(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (!m_showdialog_run)
        return;
        
    if (env->getParamCount() > 0)
        m_modal_result = env->getParam(0)->getInteger();
         else
        m_modal_result = DialogResult::Cancel;
    
    getApp()->exitModal();
    retval->setBoolean(true);
}



// (METHOD) Form.layout
// Description: Updates the position and size of the form's controls
//     using the form's layout object.
//
// Syntax: function Form.layout()
//
// Remarks: Updates the position and size of the form's controls
//     using the form's layout object, if it is specified.  If no
//     layout object is specified, the function does nothing.

void Form::layout(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_form_panel)
        return;
    
    m_form_panel->Layout();
}

// (METHOD) Form.close
// Description: Closes the form.
//
// Syntax: function Form.close()
//
// Remarks: Closes the form.

// TODO: how is this effectively different from Application.exit()

void Form::close(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_wnd)
    {
        if (m_site)
        {
            g_app->getMainFrame()->closeSite(m_site);
            m_wnd = NULL;
        }
         else
        {
            m_wnd->Destroy();
            m_wnd = NULL;
        }
    }
}

// (METHOD) Form.center
// Description: Centers a form relative to its parent.
//
// Syntax: function Form.center()
//
// Remarks: Centers a form relative to its parent.

void Form::center(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_form_frame)
    {
        m_form_frame->Center();
    }
}


// (METHOD) Form.getInnerNativeHandle
// Description: Gets the native handle of the content area of a form
//
// Syntax: function Form.getInnerNativeHandle() : Number
//
// Remarks: Returns the native handle of the content area of a form.
//     On Windows, this is the value of the content area's HWND handle.
//
// Returns: Returns the handle as an numeric value

void Form::getInnerNativeHandle(kscript::ExprEnv* env, kscript::Value* retval)
{
    
    if (!m_form_panel)
    {
        retval->setInteger(0);
        return;
    }
    
    retval->setInteger((long)(void*)m_form_panel->GetHandle());
}


void Form::destroy()
{
    if (!m_form_frame)
        return;
        
    std::vector<FormComponent*>::iterator it;
    for (it = m_form_elements.begin(); it != m_form_elements.end(); ++it)
    {
        ((FormComponent*)(*it))->deinitializeControl();
        (*it)->baseUnref();
    }
    m_form_elements.clear();
    
    if (m_layout)
    {
        m_layout->deinitializeControl();
        m_layout->baseUnref();
        m_layout = NULL;
    }
    
    
    FormFrame* form_frame = m_form_frame;
    m_form_frame = NULL;
    m_wnd = NULL;
    
    form_frame->safeOwnerDestroy();
}




// BusyCursor class implementation

class EndBusyCursor : public wxEvtHandler
{
public:
    bool ProcessEvent(wxEvent& evt)
    {
        AppBusyCursor* bc = (AppBusyCursor*)((wxCommandEvent&)evt).GetExtraLong();
        delete bc;
        wxPendingDelete.Append(this);
        return true;
    }
};

zBusyCursor::zBusyCursor()
{
    m_bc = NULL;
}

// TODO: need to document

zBusyCursor::~zBusyCursor()
{
    // end busy cursor, but from main thread
    if (m_bc)
    {
        EndBusyCursor* e = new EndBusyCursor;
        wxCommandEvent evt;
        evt.SetExtraLong((long)m_bc);
        ::wxPostEvent(e, evt);
    }
}

void zBusyCursor::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    m_bc = new AppBusyCursor;
}


// Control class implementation

// (CLASS) Control
// Category: Control
// Derives: FormControl
// Description: A class that represents a control that can be placed
//     on a form.
// Remarks: The Control class represents a control that can be placed
//     on a form.

// TODO: fill the description and remarks out more.

Control::Control()
{
    m_graphics = NULL;
}

Control::~Control()
{
    if (m_graphics)
        m_graphics->baseUnref();
}


void Control::deinitializeControl()
{
    m_wnd = NULL;
    if (m_graphics)
    {
        m_graphics->m_dc = NULL;
        m_graphics->m_wnd = NULL;
    }
}


// (CONSTRUCTOR) Control.constructor
// Description: Creates a new Control.
//
// Syntax: Control(x_pos : Integer,
//                 y_pos : Integer,
//                 width : Integer,
//                 height : Integer)
//
// Remarks: Creates a new Control at the position specified by 
//     |x_pos| and |y_pos|, and having dimensions specified by 
//     |width| and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void Control::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    FormControl::constructor(env, retval);
     
    // set default values
    m_x = 0;
    m_y = 0;
    m_width = 100;
    m_height = 100;
    
    // get user input values
    if (env->getParamCount() > 0)
        m_x = env->getParam(0)->getInteger();
    if (env->getParamCount() > 1)
        m_y = env->getParam(1)->getInteger();
    if (env->getParamCount() > 2)
        m_width = env->getParam(2)->getInteger();
    if (env->getParamCount() > 3)
        m_height = env->getParam(3)->getInteger();

    // create the control
    m_wnd = new wxWindow(getApp()->getTempParent(),
                          -1,
                          wxPoint(m_x, m_y),
                          wxSize(m_width, m_height));
                          
    m_wnd->SetBackgroundColour(*wxWHITE);
}

void Control::realize()
{
    m_wnd->Reparent(m_form_wnd);

    if (!m_enabled)
    {
        m_wnd->Enable(false);
    }

    wxSize size = m_wnd->GetClientSize();


    bool zero_size = false;
    
    if (size.x <= 0)
    {
        size.x = 1;
        zero_size = true;
    }
    
    if (size.y <= 0)
    {
        size.y = 1;
        zero_size = true;
    }


    
    wxBitmap bmp;
    bmp.Create(size.x, size.y);
    
    m_memdc.SelectObject(bmp);
    m_bmp = bmp;

    m_memdc.SetBrush(*wxWHITE_BRUSH);
    m_memdc.SetPen(*wxWHITE_PEN);
    m_memdc.SetPen(*wxBLACK_PEN);
    m_memdc.DrawRectangle(0, 0, 1000, 1000);

    listenEvent(wxEVT_PAINT);
    listenEvent(wxEVT_ERASE_BACKGROUND); // because we are double-buffered
    listenEvent(wxEVT_SIZE);
    listenEvent(wxEVT_MOTION);
    listenEvent(wxEVT_ENTER_WINDOW);
    listenEvent(wxEVT_LEAVE_WINDOW);
    listenEvent(wxEVT_MOUSEWHEEL);
    listenEvent(wxEVT_LEFT_DCLICK);
    listenEvent(wxEVT_LEFT_DOWN);
    listenEvent(wxEVT_LEFT_UP);
    listenEvent(wxEVT_MIDDLE_DOWN);
    listenEvent(wxEVT_MIDDLE_UP);
    listenEvent(wxEVT_RIGHT_DOWN);
    listenEvent(wxEVT_RIGHT_UP);
    
    if (!zero_size)
        fireOnPaint();
}


void Control::fireOnPaint()
{
    if (!m_wnd || !m_memdc.IsOk())
        return;
        
    if (!m_graphics)
    {
        m_graphics = Graphics::createObject(getParser());
        m_graphics->m_dc = &m_memdc;
        m_graphics->m_wnd = m_wnd;
        m_graphics->baseRef();
    }
    
    m_graphics->init();
    
    if (m_graphics->getPaintCount() > 0)
        return;
        
    m_graphics->incrementPaintCount();
    
    // clear out the off-screen image
    wxSize size = m_wnd->GetClientSize();
    m_memdc.SetBrush(*wxWHITE_BRUSH);
    m_memdc.SetPen(*wxWHITE_PEN);
    m_memdc.DrawRectangle(0, 0, size.x+1, size.y+1);
    m_memdc.SetPen(*wxBLACK_PEN);
    
    kscript::Value* event_args = new kscript::Value;
    event_args->setObject();
    event_args->getMember(L"graphics")->setObject(m_graphics);
    
    invokeJsEvent(L"paint", event_args, eventFlagNoGuiBlock);
}

void Control::onEvent(wxEvent& _evt)
{
    FormControl::onEvent(_evt);
    
    wxCommandEvent& evt = (wxCommandEvent&)_evt;
    int val = evt.GetInt();

    if (evt.GetEventType() == wxEVT_SIZE)
    {
        wxSize size = m_wnd->GetClientSize();
        if (size.x > m_bmp.GetWidth() ||
            size.y > m_bmp.GetHeight())
        {
            wxBitmap bmp;
            bmp.Create(size.x, size.y);
            
            m_memdc.SelectObject(bmp);
            m_bmp = bmp;
        }
        
        m_width = size.x;
        m_height = size.y;
    }
     else if (evt.GetEventType() == wxEVT_PAINT)
    {
        wxPaintDC dc(m_wnd);
        
        wxSize size = m_wnd->GetClientSize();
        dc.Blit(0, 0, size.x, size.y, &m_memdc, 0, 0);
    }
     else if (evt.GetEventType() == wxEVT_ERASE_BACKGROUND)
    {
        // empty (because 'Control' is double-buffered
    }
}



// DialogResult class implementation

// TODO: see note in header

// (CLASS) DialogResult
// Category: Dialog
// Description: A class that represents a dialog result.
// Remarks: Class encapsulates a dialog result.
//
// Property(DialogResult.Ok):      A flag that is used to add an OK button to an alert message.  If the OK button is clicked, this is also the return value for the alert message.
// Property(DialogResult.Cancel):  A flag that is used to add a Cancel button to an alert message.  If the Cancel button is clicked, this is also the return value for the alert message.
// Property(DialogResult.Yes):     A flag that is used to add a Yes button to an alert message.  If the Yes button is clicked, this is also the return value for the alert message.
// Property(DialogResult.No):      A flag that is used to add a No button to an alert message.  If the No button is clicked, this is also the return value for the alert message.
// Property(DialogResult.YesNo):   A flag that is used to add Yes and No buttons to an alert message.
// Property(DialogResult.Abort):   A flag representing that the Abort button was clicked in a dialog.
// Property(DialogResult.Retry):   A flag representing that the Retry button was clicked in a dialog.
// Property(DialogResult.Ignore):  A flag representing that the Ignore button was clicked in a dialog.

void DialogResult::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// Point class implementation

// (CLASS) Point
// Category: Graphics
// Description: A class that represents a location as an (x,y) pair.
// Remarks: The Point class represents a location as an (x,y) pair.
//
// Property(x): An Integer representing the |x| portion of the coordinate.
// Property(y): An Integer representing the |y| portion of the coordinate.

Point::Point()
{
}

Point::~Point()
{
}

// (CONSTRUCTOR) Point.constructor
// Description: Creates a new Point object.
//
// Syntax: Point(x : Integer,
//               y : Integer)
//
// Remarks: Creates a new Point object.
//
// Param(x): The |x| position of the point.
// Param(y): The |y| position of the point.

void Point::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::Value* x_member = getMember(L"x");
    kscript::Value* y_member = getMember(L"y");
    
    x_member->setInteger(0);
    y_member->setInteger(0);
    
    if (env->getParamCount() > 0)
        x_member->setInteger(env->getParam(0)->getInteger());
        
    if (env->getParamCount() > 1)
        y_member->setInteger(env->getParam(1)->getInteger());
}

// -Size class implementation

// (CLASS) Size
// Category: Graphics
// Description: A class that represents a size as a (width,height) pair.
// Remarks: The Size class represents a size as a (width,height) pair.
//
// Property(width): An Integer representing the |width| portion of the size.
// Property(height): An Integer representing the |height| portion of the size.

Size::Size()
{
}

Size::~Size()
{
}

// (CONSTRUCTOR) Size.constructor
// Description: Creates a new Size object.
//
// Syntax: Size(width : Integer,
//              height : Integer)
//
// Remarks: Creates a new Size object.
//
// Param(width): The |width| portion of the size.
// Param(height): The |height| portion of the size.

void Size::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::Value* width_member = getMember(L"width");
    kscript::Value* height_member = getMember(L"height");
    
    width_member->setInteger(0);
    height_member->setInteger(0);
    
    if (env->getParamCount() > 0)
        width_member->setInteger(env->getParam(0)->getInteger());
        
    if (env->getParamCount() > 1)
        height_member->setInteger(env->getParam(1)->getInteger());
}

// Color class implementation

// (CLASS) Color
// Category: Graphics
// Description: A class that represents a color as a combination of red, green,
//     and blue values.
// Remarks: The Color class represents a color as a combination of red, green,
//     and blue values.
//
// Property(red): An Integer representing the |red| portion of the color.
// Property(green): An Integer representing the |green| portion of the color.
// Property(blue): An Integer representing the |blue| portion of the color.

Color::Color()
{
}

Color::~Color()
{
}

// (CONSTRUCTOR) Color.constructor
// Description: Creates a new Color object.
//
// Syntax: Color(red : Integer,
//               green : Integer,
//               blue : Integer)
//
// Remarks: Creates a new Color object.
//
// Param(red): The |red| portion of the color.
// Param(green): The |green| portion of the color.
// Param(blue): The |blue| portion of the color.

void Color::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::Value* red_member = getMember(L"red");
    kscript::Value* green_member = getMember(L"green");
    kscript::Value* blue_member = getMember(L"blue");
    
    red_member->setInteger(0);
    green_member->setInteger(0);
    blue_member->setInteger(0);
    
    if (env->getParamCount() > 0)
        red_member->setInteger(env->getParam(0)->getInteger());
        
    if (env->getParamCount() > 1)
        green_member->setInteger(env->getParam(1)->getInteger());
        
    if (env->getParamCount() > 2)
        blue_member->setInteger(env->getParam(2)->getInteger());
}



// Line class implementation

// (CLASS) Line
// Category: Control
// Derives: FormControl
// Description: A class to draw a line on a form to separate controls.
// Remarks: Draws a line on a form to separate controls.
//
// Property(Line.Horizontal):   A flag representing that a horizontal line should be created.
// Property(Line.Vertical):     A flag representing that a vertical line should be created.

Line::Line()
{
}

Line::~Line()
{
}

// (CONSTRUCTOR) Line.constructor
// Description: Creates a new Line object.
//
// Syntax: Line(orientation : Integer,
//              x_pos : Integer,
//              y_pos : Integer,
//              width : Integer,
//              height : Integer)
//
// Remarks: Creates a new Line object with the specified |orientation|, 
//     at the position specified by |x_pos| and |y_pos|, and having dimensions 
//     specified by |width| and |height|, where |orientation| is either
//     Line.Horizontal or Line.Vertical.
//
// Param(orientation): The |orientation| of the line. Either
//     Line.Horizontal or Line.Vertical.
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void Line::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);
 
    // default style
    int orientation = Line::Horizontal;
    
    // set default values
    m_x = 0;
    m_y = 0;
    m_width = -1;
    m_height = -1;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
    if (param_count > 0)
        orientation = env->getParam(0)->getInteger();
    if (param_count > 1)
        m_x = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_y = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_width = env->getParam(3)->getInteger();
    if (param_count > 4)
        m_height = env->getParam(4)->getInteger();
    
    // no width was specified, assign a default width
    if (orientation == Line::Horizontal && m_width == -1)
        m_width = 100;
        
    // no height was specified, assign a default width
    if (orientation == Line::Vertical && m_height == -1)
        m_height = 100;

    // create the control
    m_ctrl = new wxStaticLine(getApp()->getTempParent(),
                              -1,
                              wxPoint(m_x, m_y),
                              wxSize(m_width, m_height),
                                (orientation == Line::Vertical)
                                  ? wxLI_VERTICAL
                                  : wxLI_HORIZONTAL);
    m_wnd = m_ctrl;

    // make sure we update the m_width and m_height member
    // variables based on the actual size of the control
    wxSize s = m_ctrl->GetSize();
    if (m_width != s.GetWidth())
        m_width = s.GetWidth();
    if (m_height != s.GetHeight())
        m_height = s.GetHeight();
}

void Line::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (!m_enabled)
        m_ctrl->Enable(false);
}


// BorderBox class implementation

// (CLASS) BorderBox
// Category: Control
// Derives: FormControl
// Description: A class to draw a box on a form to group controls.
// Remarks: Draws a box on a form to group controls.

BorderBox::BorderBox()
{
    m_ctrl = NULL;
}

BorderBox::~BorderBox()
{
}

// (CONSTRUCTOR) BorderBox.constructor
// Description: Creates a new BorderBox.
//
// Syntax: BorderBox(text : String,
//                   x_pos : Integer,
//                   y_pos : Integer,
//                   width : Integer,
//                   height : Integer)
//
// Remarks: Creates a new BorderBox having the specified caption
//     |text|, at the position specified by |x_pos| and |y_pos|, and 
//     having dimensions specified by |width| and |height|.
//
// Param(text): The caption to show for the control.
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void BorderBox::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // set default values
    wxString label = wxEmptyString;
    m_x = 0;
    m_y = 0;
    m_width = 100;
    m_height = 100;
    
    size_t param_count = env->getParamCount();
    
    // get user input values
    if (param_count > 0)
        label = env->getParam(0)->getString();
    if (param_count > 1)
        m_x = env->getParam(1)->getInteger();
    if (param_count > 2)
        m_y = env->getParam(2)->getInteger();
    if (param_count > 3)
        m_width = env->getParam(3)->getInteger();
    if (param_count > 4)
        m_height = env->getParam(4)->getInteger();

    // create the control
    m_ctrl = new wxStaticBox(getApp()->getTempParent(),
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

void BorderBox::realize()
{
    m_ctrl->Reparent(m_form_wnd);

    if (!m_enabled)
        m_ctrl->Enable(false);
}




// PictureBox class implementation

// (CLASS) PictureBox
// Category: Control
// Derives: FormControl
// Description: A class to draw a bitmap on a form.
// Remarks: Draws a bitmap on a form.

PictureBox::PictureBox()
{
    m_ctrl = NULL;
}

PictureBox::~PictureBox()
{
}

// (CONSTRUCTOR) PictureBox.constructor
// Description: Creates a new PictureBox.
//
// Syntax: PictureBox(x_pos : Integer,
//                    y_pos : Integer,
//                    width : Integer,
//                    height : Integer)
//
// Remarks: Creates a new PictureBox at the position specified
//     by |x_pos| and |y_pos|, and having dimensions specified by 
//     |width| and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void PictureBox::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    initComponent(env);

    // set default values
    m_x = 0;
    m_y = 0;
    m_width = 100;
    m_height = 100;
    
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

    // create the control
    m_ctrl = new wxStaticBitmap(getApp()->getTempParent(),
                                -1,
                                wxNullBitmap,
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

void PictureBox::realize()
{
    m_ctrl->Reparent(m_form_wnd);
    
    if (m_img.Ok())
    {
        wxImage img = m_img.Scale(m_width, m_height);
        m_bmp = wxBitmap(img);
    }
     else
    {
        m_bmp = wxNullBitmap;
    }
    
    m_ctrl->SetBitmap(m_bmp);

    if (!m_enabled)
        m_ctrl->Enable(false);

    listenEvent(wxEVT_SIZE);
}

// (METHOD) PictureBox.setImage
// Description: Sets the image to show in the picture box from
//     a specified bitmap.
//
// Syntax: function PictureBox.setImage(bitmap : Bitmap)
//
// Remarks: Displays an image in a picture box from a specified |bitmap|.
//
// Param(bitmap): The |bitmap| to show in the picture box.

void PictureBox::setImage(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::ValueObject* obj = env->getParam(0)->getObject();
    if (obj->isKindOf(L"Bitmap"))
    {
        Bitmap* b = (Bitmap*)obj;
        wxBitmap bbmp = b->getWxBitmap();
        
        if (!bbmp.Ok())
            return;
            
        // we have to do this to copy the bitmap
        wxBitmap bmp = bbmp.GetSubBitmap(wxRect(0,
                                                0,
                                                bbmp.GetWidth(),
                                                bbmp.GetHeight()));
        m_img = bmp.ConvertToImage();
        
        if (m_ctrl)
        {
            wxSize clisize = m_ctrl->GetClientSize();
            wxImage img = m_img.Scale(clisize.x, clisize.y);
            m_bmp = wxBitmap(img);
            m_ctrl->SetBitmap(m_bmp);
        }
    }
}

// (METHOD) PictureBox.setImageFromFile
// Description: Sets the image to show in the picture box from
//     a specified file.
//
// Syntax: function PictureBox.setImageFromFile(file : String) : Boolean
//
// Remarks: Displays an image in a picture box from a specified |file|.
//     Returns true if the |file| was successfully loaded, and
//     false otherwise.
//
// Param(file): The |file| storing the image to display in the picture box.
//
// Returns: Returns true if the |file| was successfully loaded, and
//     false otherwise.

void PictureBox::setImageFromFile(kscript::ExprEnv* env, kscript::Value* retval)
{
    wxString fname = env->getParam(0)->getString();

    if (!m_img.LoadFile(fname))
    {
        retval->setBoolean(false);
        return;
    }

    if (m_ctrl)
    {
        wxSize clisize = m_ctrl->GetClientSize();
        wxImage img = m_img.Scale(clisize.x, clisize.y);
        m_bmp = wxBitmap(img);
        m_ctrl->SetBitmap(m_bmp);
    }

    retval->setBoolean(true);
}

void PictureBox::onEvent(wxEvent& event)
{
    if (event.GetEventType())
    {
        // stretch the bitmap
        m_form_wnd->Freeze();

        wxSize clisize = m_ctrl->GetClientSize();

        if (clisize.x <= 0)
            clisize.x = 20;
        if (clisize.y <= 0)
            clisize.y = 20;

        if (m_img.IsOk())
        {
            wxImage img = m_img.Scale(clisize.x, clisize.y);
            m_bmp = wxBitmap(img);
            m_ctrl->SetBitmap(m_bmp);
        }

        m_form_wnd->Thaw();
    }
}



