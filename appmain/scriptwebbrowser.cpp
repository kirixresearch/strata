/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Aaron L. Williams; Benjamin I. Williams
 * Created:  2007-08-03
 *
 */


#include "appmain.h"
#include <wx/file.h>
#include "../webconnect/webcontrol.h"

#include "scriptgui.h"
#include "scriptwebbrowser.h"
#include "scriptwebdom.h"
#include <kl/thread.h>


// WebBrowser class implementation

// (CLASS) WebBrowser
// Category: Control
// Derives: FormControl
// Description: A class that represents a web browser control.
// Remarks: The WebBrowser class represents a web browser control.

// (EVENT) WebBrowser.navigated
// Description: Fired when the web control has been navigated to a new location.


WebBrowser::WebBrowser()
{
    m_ctrl = NULL;
}

WebBrowser::~WebBrowser()
{
    std::vector<wxString>::iterator it;
    for (it = m_tempfiles_to_delete.begin();
         it != m_tempfiles_to_delete.end(); ++it)
    {
        ::wxRemoveFile(*it);
    }
}

// (CONSTRUCTOR) WebBrowser.constructor
// Description: Creates a new WebBrowser control.
//
// Syntax: WebBrowser(x_pos : Integer,
//                y_pos : Integer,
//                width : Integer,
//                height : Integer)
//
// Remarks: Creates a new WebBrowser control at the position specified by 
//     |x_pos| and |y_pos|, and having dimensions specified by 
//     |width| and |height|.
//
// Param(x_pos): The x position of the control.
// Param(y_pos): The y position of the control.
// Param(width): The width of the control.
// Param(height): The height of the control.

void WebBrowser::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    // initialize the component
    FormControl::constructor(env, retval);
    
    // add some properties
    getMember(L"navigated")->setObject(Event::createObject(env));
    getMember(L"domContentLoaded")->setObject(Event::createObject(env));
    
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
}

void WebBrowser::realize()
{
    // create the control; please note that the control wants to be
    // created here, rather than in the constructor with reparenting
    // (as is usual with other control types).  The web control doesn't
    // work well on linux if it gets reparented
    m_ctrl = new wxWebControl(m_form_wnd,
                              -1,
                              wxPoint(m_x, m_y),
                              wxSize(m_width, m_height));
    m_wnd = m_ctrl;
    
    if (!m_enabled)
        m_ctrl->Enable(false);
    
    if (m_initial_nav.Length() > 0)
        m_ctrl->OpenURI(m_initial_nav);

    if (m_initial_html.Length() > 0)
        showHypertext(m_initial_html);

    listenEvent(wxEVT_WEB_LOCATIONCHANGE);
    listenEvent(wxEVT_WEB_DOMCONTENTLOADED);
    listenEvent(wxEVT_WEB_LEFTDOWN);
    listenEvent(wxEVT_WEB_MIDDLEDOWN);
    listenEvent(wxEVT_WEB_RIGHTDOWN);
    listenEvent(wxEVT_WEB_LEFTUP);
    listenEvent(wxEVT_WEB_MIDDLEUP);
    listenEvent(wxEVT_WEB_RIGHTUP);
    listenEvent(wxEVT_WEB_LEFTDCLICK);
}

// (METHOD) WebBrowser.navigate
// Description: Opens a URI for the web control.
//
// Syntax: function WebBrowser.navigate(location : String)
//
// Remarks: Opens the |location| in the web control.
//
// Param(location): The |location| to open in the web control.

void WebBrowser::navigate(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;

    if (isControlValid())
    { 
        wxString val = towx(env->getParam(0)->getString());
        m_ctrl->OpenURI(val);
    }
     else
    {
        m_initial_nav = towx(env->getParam(0)->getString());
    }
}


// (METHOD) WebBrowser.showHypertext
// Description: Displays hypertext in a the browser control
//
// Syntax: function WebBrowser.showHypertext(html_text : String) : Boolean
//
// Remarks: Display's the text passed in the |html_text| parameter in the
//     browser control.  The text is stored in a temporary file which is
//     removed after the control has been destroyed.
//
// Param(html_text): The HTML text that the control should display
// Returns: |true| if the operation succeeded, |false| otherwise

void WebBrowser::showHypertext(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
        
    if (env->getParamCount() < 1)
        return;
    
    if (!isControlValid())
    {
        m_initial_html = towx(env->getParam(0)->getString());
        return;
    }
    
    retval->setBoolean(showHypertext(towx(env->getParam(0)->getString())));
}

bool WebBrowser::showHypertext(const wxString& html)
{
    std::wstring s_temp_filename = xf_get_temp_filename(L"tmpd", L"html");
    wxString temp_filename = towx(s_temp_filename);
    
    // create a file that only the local user is able to read and write to
    wxFile file;
    if (!file.Create(temp_filename, false, wxS_IRUSR | wxS_IWUSR))
        return false;
    
    // delete this file when the WebBrowser control is destroyed
    m_tempfiles_to_delete.push_back(temp_filename);
    
    // write the text out to the file in utf8
    file.Write(html, wxConvUTF8);
    
    file.Close();
    
    wxString url = filenameToUrl(temp_filename);

    if (isControlValid())
        m_ctrl->OpenURI(url);
    
    return true;
}


// (METHOD) WebBrowser.waitUntilReady
// Description: Waits for the web control's current load cycle to finish
//
// Syntax: function WebBrowser.waitUntilReady(max_milliseconds : Number)
//
// Remarks: Calling this method will cause execution to wait until the
//     web browser has finished loading the web document.  It is normally
//     invoked after a call to navigate() is issued.  The function may return
//     before all images are loaded, but ensures that the web pages DOM document
//     is fully present.   The optional parameter, |max_milliseconds|,
//     specifies the maximum amount of time the call should wait before
//     returning.  If no parameter is specified, the method will wait indefinitely.
//
// Param(max_milliseconds): (optional) The number of milliseconds to wait
//     before returning


void WebBrowser::waitUntilReady(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;
        
    wxStopWatch sw;
    long max_ms = -1;

    if (env->getParamCount() > 0)
    {
        max_ms = env->getParam(0)->getInteger();
        if (max_ms == 0)
            return;
    }
    
    if (max_ms >= 0)
        sw.Start();
    
    while (!m_ctrl->IsContentLoaded())
    {
        kl::Thread::sleep(20);
        ::wxSafeYield();
        
        if (max_ms > 0)
        {
            if (sw.Time() > max_ms)
                return;
        }
    }
}




// (METHOD) WebBrowser.getLocation
// Description: Gets the current URI for the web control.
//
// Syntax: function WebBrowser.getLocation() : String
//
// Remarks: Returns the current URI for the web control.
//
// Returns: Returns the current URI for the web control.

void WebBrowser::getLocation(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setString(L"");
        return;
    }

    wxString val = m_ctrl->GetCurrentURI();
    retval->setString(val);
}

// (METHOD) WebBrowser.goForward
// Description: Opens the next URI in the list of navigated URIs.
//
// Syntax: function WebBrowser.goForward()
//
// Remarks: Opens the next URI in the list of navigated URIs.

void WebBrowser::goForward(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;
        
    m_ctrl->GoForward();
}

// (METHOD) WebBrowser.goBack
// Description: Opens the previous URI in the list of navigated URIs.
//
// Syntax: function WebBrowser.goBack()
//
// Remarks: Opens the previous URI in the list of navigated URIs.

void WebBrowser::goBack(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->GoBack();
}

// (METHOD) WebBrowser.reload
// Description: Reloads the current URI in the web control.
//
// Syntax: function WebBrowser.reload()
//
// Remarks: Reloads the current URI in the web control.

void WebBrowser::reload(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->Reload();
}

// (METHOD) WebBrowser.stop
// Description: Stops the web control from loading a URI that's currently
//     being loaded.
//
// Syntax: function WebBrowser.stop()
//
// Remarks: Stops the web control from loading a URI that's currently
//     being loaded.

void WebBrowser::stop(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->Stop();
}


// (METHOD) WebBrowser.canCutSelection
// Description: Indicates if a there's a selection in the web control 
//     that can be cut.
//
// Syntax: function WebBrowser.canCutSelection() : Boolean
//
// Remarks: Returns true if there's a selection in the web control that can
//     be cut, and false otherwise.
//
// Returns: Returns true if there's a selection in the web control that can
//     be cut, and false otherwise.

void WebBrowser::canCutSelection(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setBoolean(false);
        return;
    }
    
    bool val = m_ctrl->CanCutSelection();
    retval->setBoolean(val);
}

// (METHOD) WebBrowser.canCopySelection
// Description: Indicates if a there's a selection in the web control 
//     that can be copied.
//
// Syntax: function WebBrowser.canCopySelection() : Boolean
//
// Remarks: Returns true if there's a selection in the web control that can
//     be copied, and false otherwise.
//
// Returns: Returns true if there's a selection in the web control that can
//     be copied, and false otherwise.

void WebBrowser::canCopySelection(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setBoolean(false);
        return;
    }

    bool val = m_ctrl->CanCopySelection();
    retval->setBoolean(val);
}

// (METHOD) WebBrowser.canCopyLinkLocation
// Description: Indicates if a the current link location can be copied.
//
// Syntax: function WebBrowser.canCopyLinkLocation() : Boolean
//
// Remarks: Returns true if the current link location can be copied, and
//     false otherwise.
//
// Returns: Returns true if the current link location can be copied, and
//     false otherwise.

void WebBrowser::canCopyLinkLocation(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setBoolean(false);
        return;
    }

    bool val = m_ctrl->CanCopyLinkLocation();
    retval->setBoolean(val);
}

// (METHOD) WebBrowser.canCopyImageLocation
// Description: Indicates if a the current image location can be copied.
//
// Syntax: function WebBrowser.canCopyImageLocation() : Boolean
//
// Remarks: Returns true if the current image location can be copied, and
//     false otherwise.
//
// Returns: Returns true if the current image location can be copied, and
//     false otherwise.

void WebBrowser::canCopyImageLocation(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setBoolean(false);
        return;
    }

    bool val = m_ctrl->CanCopyImageLocation();
    retval->setBoolean(val);
}

// (METHOD) WebBrowser.canCopyImageContents
// Description: Indicates if a the current image contents can be copied.
//
// Syntax: function WebBrowser.canCopyImageContents() : Boolean
//
// Remarks: Returns true if the current image contents can be copied, and
//     false otherwise.
//
// Returns: Returns true if the current image contents can be copied, and
//     false otherwise.

void WebBrowser::canCopyImageContents(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setBoolean(false);
        return;
    }

    bool val = m_ctrl->CanCopyImageContents();
    retval->setBoolean(val);
}

// (METHOD) WebBrowser.canPaste
// Description: Indicates in the current contents of the system 
//     clipboard can be pasted.
//
// Syntax: function WebBrowser.canPaste() : Boolean
//
// Remarks: Returns true if the current contents of the system 
//     clipboard can be pasted, and false otherwise.

void WebBrowser::canPaste(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
    {
        retval->setBoolean(false);
        return;
    }

    bool val = m_ctrl->CanPaste();
    retval->setBoolean(val);
}

// (METHOD) WebBrowser.cutSelection
// Description: Cuts the current selection.
//
// Syntax: function WebBrowser.cutSelection()
//
// Remarks: Cuts the current selection.

void WebBrowser::cutSelection(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->CutSelection();
}

// (METHOD) WebBrowser.copySelection
// Description: Copies the current selection to the system clipboard.
//
// Syntax: function WebBrowser.copySelection()
//
// Remarks: Copies the current selection to the system clipboard.

void WebBrowser::copySelection(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->CopySelection();
}

// (METHOD) WebBrowser.copyLinkLocation
// Description: Copies the current link location to the system clipboard.
//
// Syntax: function WebBrowser.copyLinkLocation()
//
// Remarks: Copies the current link location to the system clipboard.

void WebBrowser::copyLinkLocation(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->CopyLinkLocation();
}

// (METHOD) WebBrowser.copyImageLocation
// Description: Copies the current image location to the system clipboard.
//
// Syntax: function WebBrowser.copyImageLocation()
//
// Remarks: Copies the current image location to the system clipboard.

void WebBrowser::copyImageLocation(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->CopyImageLocation();
}

// (METHOD) WebBrowser.copyImageContents
// Description: Copies the current image contents to the system clipboard.
//
// Syntax: function WebBrowser.copyImageContents()
//
// Remarks: Copies the current image contents to the system clipboard.

void WebBrowser::copyImageContents(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->CopyImageContents();
}

// (METHOD) WebBrowser.paste
// Description: Pastes the contents of the system clipboard to the web control.
//
// Syntax: function WebBrowser.paste()
//
// Remarks: Pastes the contents of the system clipboard to the web control.

void WebBrowser::paste(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->Paste();
}

// (METHOD) WebBrowser.selectAll
// Description: Selects all the selectable content of the web control.
//
// Syntax: function WebBrowser.selectAll()
//
// Remarks: Selects all the selectable content of the web control.

void WebBrowser::selectAll(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->SelectAll();
}

// (METHOD) WebBrowser.selectNone
// Description: Deselects all the selected content of the web control.
//
// Syntax: function WebBrowser.selectNone()
//
// Remarks: Deselects all the selected content of the web control.

void WebBrowser::selectNone(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!isControlValid())
        return;

    m_ctrl->SelectNone();
}


void WebBrowser::getDOMDocument(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
  
    if (!isControlValid())
        return;
  
    wxDOMDocument doc = m_ctrl->GetDOMDocument();
    if (!doc.IsOk())
        return;
        
    WebDOMDocument* ret = WebDOMDocument::createObject(env);
    ret->m_node = doc;
    
    retval->setObject(ret);
}



void WebBrowser::onEvent(wxEvent& event)
{
    wxCommandEvent& evt = (wxCommandEvent&)event;
    int val = evt.GetInt();

    kscript::Value* event_args = new kscript::Value;
    event_args->setObject();
    event_args->getMember(L"value")->setInteger(val);

    if (event.GetEventType() == wxEVT_WEB_LOCATIONCHANGE)
    {
        invokeJsEvent(L"navigated", event_args);
    }
    else if (event.GetEventType() == wxEVT_WEB_DOMCONTENTLOADED)
    {
        invokeJsEvent(L"domContentLoaded", event_args);
    }
     else if (event.GetEventType() == wxEVT_WEB_LEFTDOWN)
    {
        invokeJsEvent(L"mouseLeftDown", event_args);
    }
     else if (event.GetEventType() == wxEVT_WEB_MIDDLEDOWN)
    {
        invokeJsEvent(L"mouseMiddleDown", event_args);
    }
     else if (event.GetEventType() == wxEVT_WEB_RIGHTDOWN)
    {
        invokeJsEvent(L"mouseRightDown", event_args);
    }
     else if (event.GetEventType() == wxEVT_WEB_LEFTUP)
    {
        invokeJsEvent(L"mouseLeftUp", event_args);
    }
     else if (event.GetEventType() == wxEVT_WEB_MIDDLEUP)
    {
        invokeJsEvent(L"mouseMiddleUp", event_args);
    }
     else if (event.GetEventType() == wxEVT_WEB_RIGHTUP)
    {
        invokeJsEvent(L"mouseRightUp", event_args);
    }
     else if (event.GetEventType() == wxEVT_WEB_LEFTDCLICK)
    {
        invokeJsEvent(L"mouseLeftDoubleClick", event_args);
    }
     else
    {
        // this event type is not handled
        delete event_args;
    }
}

