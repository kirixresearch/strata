/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-06-20
 *
 */


#ifndef H_APP_SCRIPTGUI_H
#define H_APP_SCRIPTGUI_H


#include "../kscript/kscript.h"
#include "scripthost.h"
#include "scriptapp.h"


// forward declarations

class FormComponent;
class FormPanel;
class zMenu;
class MenuBar;
class ToolBar;
class Layout;


// utility functions

int getUniqueScriptControlId();
int getUniqueScriptCommandId();



class wxModalEventLoop;

class FormFrame : public wxFrame
{
    friend class Form;
    
    enum
    {
        ID_DoDestroy = 20000
    };

public:

    FormFrame(Form* owner,
              wxWindow* parent,
              wxWindowID id,
              const wxString& caption,
              const wxPoint& pos,
              const wxSize& size);
    ~FormFrame();

    wxPanel* getPanel();
    void enableForm(bool enabled);
    void safeOwnerDestroy();
    
private:

    void onSize(wxSizeEvent& evt);
    void onClose(wxCloseEvent& evt);
    void onPaint(wxPaintEvent& evt);
    void onDoDestroy(wxCommandEvent& evt);

private:

    Form* m_owner;
    FormPanel* m_panel;
    bool m_form_enabled;
    
    xcm::signal1<bool*> sigFormClose;
    xcm::signal0 sigFormDestructing;

    DECLARE_EVENT_TABLE()
};


class FormEventHandler : public wxEvtHandler,
                         public xcm::signal_sink
{
    friend Form;

public:

    FormEventHandler(FormComponent* owner);
    ~FormEventHandler();
    void safeDestroy();
    
    void onEvent(wxEvent& evt);

    void onFormClose(bool* allow); // only used for Form components
    void onFormDestructing();      // only used for Form components

public:

    FormComponent* m_owner;
};


class FormComponent : public ScriptHostBase
{
public:
    
    FormComponent();
    ~FormComponent();

    void setFormWindow(wxWindow* wnd);
    wxWindow* getWindow();

    virtual void realize() { }
    virtual void deinitializeControl();

    bool isControlValid() const { return m_wnd ? true : false; }
    
protected:

    void listenEvent(int wx_event, int id = -1, int id2 = -1);
    void listenEvent(wxWindow* wnd, int wx_event, int id = -1, int id2 = -1);

public:

    wxWindow* m_form_wnd;
    wxWindow* m_wnd;
    wxFont m_font;
    int m_x;
    int m_y;
    int m_min_width;
    int m_min_height;
    int m_max_width;
    int m_max_height;
    int m_cliwidth;
    int m_cliheight;
    int m_width;
    int m_height;
    bool m_enabled;

protected:

    FormEventHandler* m_evt_handler;
};


// TODO: The getMember() function creates a new object if it can't be found, 
// and this is used casually in many constructors to define important variables, 
// such as those used in events.  This has two drawbacks: 1) it's prone to errors that 
// can't be detected at runtime, such as misspellings, because the object is 
// implicitly created, and 2) the definition of the class is spread out between 
// the header and implementation file, with the functions being specified using 
// macro bindings, but the constants and variables defined implicitly in the 
// constructor binding implementation, making it hard to see the entire interface 
// of a function at a glance, and therefore difficult to document.

// TODO (cont): One partial solution to handle these "constructor" bindings is to 
// define the binding strings using constants in the header file.  This provides 
// compile time checking and also allows definitions to be changed quickly.  Compare 
// the current problem of changing the events from the "onMouseLeftDown" nomenclature 
// to the "mouseLeftDown" - every instance has to be checked carefully, and if it
// isn't, the wrong member variable will be implicitly created - compare this with 
// defining them with the constant
//
// wxString MOUSE_LEFT_DOWN = wxT("mouseLeftDown");

// where the constant could be used safely.  If these bindings where moved near the
// class definition, it might just also provide the whole definition.

// TODO: the class hierarchy seems confused here.  Form derives from FormControl?

class FormControl : public FormComponent
{
    BEGIN_KSCRIPT_CLASS("FormControl", FormControl)
        KSCRIPT_GUI_METHOD("constructor", FormControl::constructor)
        KSCRIPT_GUI_METHOD("setFocus", FormControl::setFocus)
        KSCRIPT_GUI_METHOD("show", FormControl::show)
        KSCRIPT_GUI_METHOD("setEnabled", FormControl::setEnabled)
        KSCRIPT_GUI_METHOD("getEnabled", FormControl::getEnabled)
        KSCRIPT_GUI_METHOD("setPosition", FormControl::setPosition)
        KSCRIPT_GUI_METHOD("getPosition", FormControl::getPosition)
        KSCRIPT_GUI_METHOD("setSize", FormControl::setSize)
        KSCRIPT_GUI_METHOD("getSize", FormControl::getSize)
        KSCRIPT_GUI_METHOD("setMaxSize", FormControl::setMaxSize)
        KSCRIPT_GUI_METHOD("getMaxSize", FormControl::getMaxSize)
        KSCRIPT_GUI_METHOD("setMinSize", FormControl::setMinSize)
        KSCRIPT_GUI_METHOD("getMinSize", FormControl::getMinSize)
        KSCRIPT_GUI_METHOD("setClientSize", FormControl::setClientSize)
        KSCRIPT_GUI_METHOD("getClientSize", FormControl::getClientSize)        
        KSCRIPT_GUI_METHOD("setBackgroundColor", FormControl::setBackgroundColor)
        KSCRIPT_GUI_METHOD("getBackgroundColor", FormControl::getBackgroundColor)
        KSCRIPT_GUI_METHOD("setForegroundColor", FormControl::setForegroundColor)
        KSCRIPT_GUI_METHOD("getForegroundColor", FormControl::getForegroundColor)
        KSCRIPT_GUI_METHOD("setFont", FormControl::setFont)
        KSCRIPT_GUI_METHOD("getFont", FormControl::getFont)
        KSCRIPT_GUI_METHOD("invalidate", FormControl::invalidate)
        KSCRIPT_GUI_METHOD("update", FormControl::update)
        KSCRIPT_GUI_METHOD("refresh", FormControl::refresh)
        KSCRIPT_GUI_METHOD("captureMouse", FormControl::captureMouse)
        KSCRIPT_GUI_METHOD("releaseMouse", FormControl::releaseMouse)
        KSCRIPT_GUI_METHOD("disablePaint", FormControl::disablePaint)
        KSCRIPT_GUI_METHOD("enablePaint", FormControl::enablePaint)
        KSCRIPT_GUI_METHOD("getNativeHandle", FormControl::getNativeHandle)
    END_KSCRIPT_CLASS()

public:

    FormControl();
    ~FormControl();
    
    void setPositionInt(int x, int y, int w, int h);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void show(kscript::ExprEnv* env, kscript::Value* retval);
    void setBackgroundColor(kscript::ExprEnv* env, kscript::Value* retval);
    void getBackgroundColor(kscript::ExprEnv* env, kscript::Value* retval);
    void setForegroundColor(kscript::ExprEnv* env, kscript::Value* retval);
    void getForegroundColor(kscript::ExprEnv* env, kscript::Value* retval);
    void setPosition(kscript::ExprEnv* env, kscript::Value* retval);
    void getPosition(kscript::ExprEnv* env, kscript::Value* retval);
    void setSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getSize(kscript::ExprEnv* env, kscript::Value* retval);
    void setMinSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getMinSize(kscript::ExprEnv* env, kscript::Value* retval);
    void setMaxSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getMaxSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getMousePosition(kscript::ExprEnv* env, kscript::Value* retval);
    void setClientSize(kscript::ExprEnv* env, kscript::Value* retval);
    void getClientSize(kscript::ExprEnv* env, kscript::Value* retval);
    void setEnabled(kscript::ExprEnv* env, kscript::Value* retval);
    void getEnabled(kscript::ExprEnv* env, kscript::Value* retval);
    void setFont(kscript::ExprEnv* env, kscript::Value* retval);
    void getFont(kscript::ExprEnv* env, kscript::Value* retval);
    void setFocus(kscript::ExprEnv* env, kscript::Value* retval);
    void invalidate(kscript::ExprEnv* env, kscript::Value* retval);
    void update(kscript::ExprEnv* env, kscript::Value* retval);
    void refresh(kscript::ExprEnv* env, kscript::Value* retval);
    void captureMouse(kscript::ExprEnv* env, kscript::Value* retval);
    void releaseMouse(kscript::ExprEnv* env, kscript::Value* retval);
    void disablePaint(kscript::ExprEnv* env, kscript::Value* retval);
    void enablePaint(kscript::ExprEnv* env, kscript::Value* retval);
    void getNativeHandle(kscript::ExprEnv* env, kscript::Value* retval);
    
protected:

    void onEvent(wxEvent& evt);
    virtual void fireOnPaint() { }
    
    int m_last_mouse_x;
    int m_last_mouse_y;
};



class Form : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("Form", Form, FormControl)
        KSCRIPT_GUI_METHOD("constructor", Form::constructor)
        KSCRIPT_GUI_METHOD("setCaption", Form::setCaption)
        KSCRIPT_GUI_METHOD("getCaption", Form::getCaption)
        KSCRIPT_GUI_METHOD("setMenuBar", Form::setMenuBar)
        KSCRIPT_GUI_METHOD("setToolBar", Form::setToolBar)
        KSCRIPT_GUI_METHOD("setStatusBar", Form::setStatusBar)
        KSCRIPT_GUI_METHOD("setLayout", Form::setLayout)
        KSCRIPT_GUI_METHOD("add", Form::add)
        KSCRIPT_GUI_METHOD("show", Form::show)
        KSCRIPT_GUI_METHOD("layout", Form::layout)
        KSCRIPT_GUI_METHOD("close", Form::close)
        KSCRIPT_GUI_METHOD("center", Form::center)
        KSCRIPT_GUI_METHOD("getInnerNativeHandle", Form::getInnerNativeHandle)
        KSCRIPT_METHOD("showDialog", Form::showDialog)
        KSCRIPT_METHOD("endDialog", Form::endDialog)
    END_KSCRIPT_CLASS()

public:

    Form();
    ~Form();
    
    void realize();
    void setSite(IDocumentSitePtr site);
    
    void destroy();

    wxWindow* getFormPanel();
    
    void enableForm(bool enabled);

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void getCaption(kscript::ExprEnv* env, kscript::Value* retval);
    void setMenuBar(kscript::ExprEnv* env, kscript::Value* retval);
    void setToolBar(kscript::ExprEnv* env, kscript::Value* retval);
    void setStatusBar(kscript::ExprEnv* env, kscript::Value* retval);
    void setLayout(kscript::ExprEnv* env, kscript::Value* retval);
    void add(kscript::ExprEnv* env, kscript::Value* retval);
    void show(kscript::ExprEnv* env, kscript::Value* retval);
    void showDialog(kscript::ExprEnv* env, kscript::Value* retval);
    void endDialog(kscript::ExprEnv* env, kscript::Value* retval);
    void layout(kscript::ExprEnv* env, kscript::Value* retval);
    void close(kscript::ExprEnv* env, kscript::Value* retval);
    void center(kscript::ExprEnv* env, kscript::Value* retval);
    void getInnerNativeHandle(kscript::ExprEnv* env, kscript::Value* retval);

    void onEvent(wxEvent& evt);
    void onFormFrameDestructing();
    bool onFormFrameClose();

public:

    void deinitializeControl();
    
public:

    MenuBar* m_menubar;
    ToolBar* m_toolbar;
    wxString m_caption;
    std::vector<zMenu*> m_popup_menus;
    std::vector<FormComponent*> m_form_elements;
    
    FormFrame* m_form_frame;
    wxPanel* m_form_panel;
    IDocumentSitePtr m_site;  // site, if any
            
    bool m_showdialog_run;      // true of showDialog has ever been invoked
    bool m_showdialog_running;  // true of showDialog is running
    
    int m_modal_result;

    Layout* m_layout;
    bool m_first_time;
};


class Graphics;
class Control : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("Control", Control, FormControl)
        KSCRIPT_GUI_METHOD("constructor", Control::constructor)
    END_KSCRIPT_CLASS()

public:

    Control();
    ~Control();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(wxEvent& evt);
    virtual void fireOnPaint();
    
    void deinitializeControl();
    
private:

    wxMemoryDC m_memdc;
    wxBitmap m_bmp;
    Graphics* m_graphics;
};


// NOTE: This class exists here because it is now possible to create
//       modal forms which return a DialogResult constant
class DialogResult : public kscript::ValueObject
{    
public:

    BEGIN_KSCRIPT_CLASS("DialogResult", DialogResult)

        KSCRIPT_METHOD("constructor", DialogResult::constructor)
        
        KSCRIPT_CONSTANT_INTEGER("OK",     Ok)
        KSCRIPT_CONSTANT_INTEGER("Ok",     Ok)
        KSCRIPT_CONSTANT_INTEGER("Cancel", Cancel)
        KSCRIPT_CONSTANT_INTEGER("Abort",  Abort)
        KSCRIPT_CONSTANT_INTEGER("Retry",  Retry)
        KSCRIPT_CONSTANT_INTEGER("Ignore", Ignore)
        KSCRIPT_CONSTANT_INTEGER("Yes",    Yes)
        KSCRIPT_CONSTANT_INTEGER("No",     No)
        KSCRIPT_CONSTANT_INTEGER("YesNo",  YesNo)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Ok     = 1 << 0,
        Cancel = 1 << 1,
        Abort  = 1 << 2,
        Retry  = 1 << 3,
        Ignore = 1 << 4,
        Yes    = 1 << 5,
        No     = 1 << 6,
        YesNo  = (1 << 5) | (1 << 6)
    };

public:

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
};


class Point : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("Point", Point)
        KSCRIPT_GUI_METHOD("constructor", Point::constructor)
    END_KSCRIPT_CLASS()

public:

    Point();
    ~Point();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
};


class Size : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("Size", Size)
        KSCRIPT_GUI_METHOD("constructor", Size::constructor)
    END_KSCRIPT_CLASS()

public:

    Size();
    ~Size();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
};


class Color : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("Color", Color)
        KSCRIPT_GUI_METHOD("constructor", Color::constructor)
    END_KSCRIPT_CLASS()

public:

    Color();
    ~Color();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
};


class zBusyCursor : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("BusyCursor", zBusyCursor)
        KSCRIPT_GUI_METHOD("constructor", zBusyCursor::constructor)
    END_KSCRIPT_CLASS()

public:

    zBusyCursor();
    ~zBusyCursor();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    AppBusyCursor* m_bc;
};


class Line : public FormControl
{
public:

    BEGIN_KSCRIPT_DERIVED_CLASS("Line", Line, FormControl)

        KSCRIPT_GUI_METHOD("constructor", Line::constructor)
        
        KSCRIPT_CONSTANT_INTEGER("Horizontal",    Horizontal)
        KSCRIPT_CONSTANT_INTEGER("Vertical",      Vertical)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Horizontal,
        Vertical
    };

public:

    Line();
    ~Line();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

private:

    wxStaticLine* m_ctrl;
};


class BorderBox : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("BorderBox", BorderBox, FormControl)
        KSCRIPT_GUI_METHOD("constructor", BorderBox::constructor)
    END_KSCRIPT_CLASS()
    
public:
    
    BorderBox();
    ~BorderBox();
    void realize();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);

private:

    wxStaticBox* m_ctrl;
};


class PictureBox : public FormControl
{
    BEGIN_KSCRIPT_DERIVED_CLASS("PictureBox", PictureBox, FormControl)
        KSCRIPT_GUI_METHOD("constructor", PictureBox::constructor)
        KSCRIPT_GUI_METHOD("setImage", PictureBox::setImage)
        KSCRIPT_GUI_METHOD("setImageFromFile", PictureBox::setImageFromFile)
    END_KSCRIPT_CLASS()

public:

    PictureBox();
    ~PictureBox();
    void realize();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setImage(kscript::ExprEnv* env, kscript::Value* retval);
    void setImageFromFile(kscript::ExprEnv* env, kscript::Value* retval);

private:

    void onEvent(wxEvent& event);

private:

    wxStaticBitmap* m_ctrl;
    wxBitmap m_bmp;
    wxImage m_img;
};


/*
class FormDataObject : public kscript::ValueObject,
                       public wxCustomDataObject
{
    BEGIN_KSCRIPT_CLASS("DragAndDropData", FormDataObject)
        KSCRIPT_GUI_METHOD("constructor", FormDataObject::constructor)
        KSCRIPT_GUI_METHOD("add", FormDataObject::add)
        KSCRIPT_GUI_METHOD("delete", FormDataObject::zdelete)
        KSCRIPT_GUI_METHOD("clear", FormDataObject::clear)
        KSCRIPT_GUI_METHOD("getCount", FormDataObject::getCount)
        KSCRIPT_GUI_METHOD("getItem", FormDataObject::getItem)
    END_KSCRIPT_CLASS()

public:

    FormDataObject();
    ~FormDataObject();
    
    void* GetData() const;
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void add(kscript::ExprEnv* env, kscript::Value* retval);
    void zdelete(kscript::ExprEnv* env, kscript::Value* retval);
    void clear(kscript::ExprEnv* env, kscript::Value* retval);
    void getCount(kscript::ExprEnv* env, kscript::Value* retval);
    void getItem(kscript::ExprEnv* env, kscript::Value* retval);

private:

    std::map<std::wstring, kscript::ValueObject*> m_old_items;
    std::map<std::wstring, kscript::ValueObject*> m_items;
};


class FormDropTarget : public wxDropTarget
{
public:
    
    FormDropTarget(FormControl* owner);
    ~FormDropTarget();

    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
    wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    bool OnDrop(wxCoord x, wxCoord y);
    void OnLeave();
    
private:

    FormControl* m_owner;

    // different pieces of data that can be set
    std::vector<TreeViewItem*> m_treeitems;
    wxString m_text;
};


*/


#endif

