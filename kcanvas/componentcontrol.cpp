/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2007-07-11
 *
 */


#include "kcanvas_int.h"
#include "componentcontrol.h"


namespace kcanvas
{


CompControl::CompControl(wxControl* control)
{
    // properties
    initProperties(m_properties);

    // add event handlers
    addEventHandler(EVENT_KEY, &CompControl::onKey);
    addEventHandler(EVENT_MOUSE_LEFT_DCLICK, &CompControl::onMouse);
    addEventHandler(EVENT_MOUSE_LEFT_DOWN, &CompControl::onMouse);
    addEventHandler(EVENT_MOUSE_LEFT_UP, &CompControl::onMouse);
    addEventHandler(EVENT_MOUSE_RIGHT_DCLICK, &CompControl::onMouse);
    addEventHandler(EVENT_MOUSE_RIGHT_DOWN, &CompControl::onMouse);
    addEventHandler(EVENT_MOUSE_RIGHT_UP, &CompControl::onMouse);
    addEventHandler(EVENT_MOUSE_MOTION, &CompControl::onMouse);
    addEventHandler(EVENT_MOUSE_OVER, &CompControl::onMouse);

    // save the control
    m_control = control;

    // make sure the canvas is the control's parent and that it is 
    // initially hidden
    if (m_control)
    {
        ICanvasControlPtr canvas_control = getCanvas();
        if (!canvas_control.isNull())
            m_control->Reparent(canvas_control->getWindow());
        
        m_control->Show(false);
    }

    // initialize the cache
    m_cache_bmp = wxNullBitmap;
    m_cache_width = 0;
    m_cache_height = 0;
    
    // editing state
    m_editing = false;
}

CompControl::~CompControl()
{
    // TODO: uncommenting the following causes the program to crash, probably because
    // the canvas is handling the deleting of the control; not sure if the following
    // is even necessary, but we'll leave it in, but commented out, so we can look
    // more into the issue

    //if (m_control)
    //    m_control->Destroy();
}

IComponentPtr CompControl::create(wxControl* control)
{
    return static_cast<IComponent*>(new CompControl(control));
}

void CompControl::initProperties(Properties& properties)
{
    Component::initProperties(properties);
    properties.add(PROP_COMP_TYPE, COMP_TYPE_CONTROL);
}

IComponentPtr CompControl::clone()
{
    CompControl* c = new CompControl(NULL);
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void CompControl::copy(IComponentPtr component)
{
    Component::copy(component);
    clearCache();
        
    // TODO: create a new control from the old one, or
    // is there a better approach?
}

void CompControl::addProperty(const wxString& prop_name, const PropertyValue& value)
{
    // add the property and clear the cache
    Component::addProperty(prop_name, value);
    clearCache();
}

void CompControl::addProperties(const Properties& properties)
{
    // add the properties and clear the cache
    Component::addProperties(properties);
    clearCache();
}

void CompControl::removeProperty(const wxString& prop_name)
{
    // remove the property and clear the cache
    Component::removeProperty(prop_name);
    clearCache();
}

bool CompControl::setProperty(const wxString& prop_name, const PropertyValue& value)
{
    // try to set the component property; if the property is 
    // unchanged, we're done; return false
    if (!Component::setProperty(prop_name, value))
        return false;
        
    // if the property has changed, clear the cache; return true
    clearCache();
    return true;
}

bool CompControl::setProperties(const Properties& properties)
{
    // try to set the component properties; if the properties are 
    // unchanged, we're done; return false
    if (!Component::setProperties(properties))
        return false;
    
    // if the properties have changed, clear the cache; return true
    clearCache();
    return true;
}

void CompControl::render(const wxRect& rect)
{
    // if we don't have a control, we're done
    if (!m_control)
        return;

    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // if the width or height is zero, we're done
    int w, h;
    getSize(&w, &h);
    if (w == 0 || h == 0)
        return;

    // TODO: render a background

    // if we're editing, we're done; the control is showing, so it
    // will take care of it's own rendering
    if (isEditing())
        return;

    if (!m_cache_bmp.IsOk())
    {
        // create a bitmap and select it into a memory dc
        m_cache_bmp.Create(w, h, -1);
        
        wxMemoryDC memdc;
        memdc.SelectObject(m_cache_bmp);
    
        // TODO: we may need a better way to find the position; this is relying
        // on an implementation detail of the canvas; namely, that a component
        // is drawn at a position by shifting the draw origin to the beginning
        // of the component; do we need a render event that carries the position
        // of the component at the time of render, and if so, so should make a
        // focus event that works the same way?
        int x, y;
        canvas->getDrawOrigin(&x, &y);
        x = canvas->mtod_x(x);
        y = canvas->mtod_y(y);

        int w, h;
        w = canvas->mtod_x(getWidth());
        h = canvas->mtod_y(getHeight());
    
        // move the control into position and show it
        m_control->Move(x,y);
        m_control->SetSize(w, h);
        m_control->Show(true);

        // refresh it and hide it again; TODO: this technique works as long as the control
        // is entirely on the screen; secondly, it produces a momentary display of the
        // control, which is undesirable; the answer may be to create a parent control
        // that contains this control, and override the parent control to use a buffered
        // paint dc, set the dc so it doesn't flush on destruction, then blit the control 
        // from the buffer; this may allow us to display controls up to the size of the
        // client area, which should be good enough
        m_control->Refresh();
        m_control->Update();
        m_control->Show(false);

        // blit the control from the screen onto the memory dc and save the bitmap
        m_control->GetSize(&w, &h);
        wxClientDC dc(m_control);
        memdc.Blit(0, 0, w, h, &dc, 0, 0);

        //m_cache_bmp.SetSize(w, h);
        memdc.SelectObject(wxNullBitmap);
    }

    wxImage image = m_cache_bmp.ConvertToImage();
    canvas->drawImage(image, 0, 0, getWidth(), getHeight());
}

void CompControl::beginEdit()
{
    if (!m_control)
        return;

    ICanvasPtr canvas = getCanvas();
    if (canvas.isNull())
        return;

    // find out where we are so we can move the control to this position;
    // TODO: we may need a better way to find the position; this is relying
    // on an implementation detail of the canvas; namely, that a component
    // is drawn at a position by shifting the draw origin to the beginning
    // of the component; do we need a focus event that carries the position
    // of the component at the time of focus, and if so, so should make a
    // render event that works the same way?
    int x, y;
    canvas->getDrawOrigin(&x, &y);
    x = canvas->mtod_x(x);
    y = canvas->mtod_y(y);

    int w, h;
    w = canvas->mtod_x(getWidth());
    h = canvas->mtod_y(getHeight());

    // move the control into position and show it
    m_control->Move(x,y);
    m_control->SetSize(w, h);
    m_control->Show(true);
    m_control->Update();
    m_control->Refresh();

    m_editing = true;
}

void CompControl::endEdit(bool accept)
{
    // if we aren't editing, we're done
    if (!isEditing())
        return;

    // reset the bitmap
    m_cache_bmp = wxNullBitmap;

    // get the width and height
    int w, h;
    getSize(&w, &h);

    // create a bitmap and select it into a memory dc
    m_cache_bmp.Create(w, h, -1);
    
    wxMemoryDC memdc;
    memdc.SelectObject(m_cache_bmp);
    
    // blit the control from the screen onto the memory dc and save the bitmap
    m_control->GetSize(&w, &h);
    wxClientDC dc(m_control);
    memdc.Blit(0, 0, w, h, &dc, 0, 0);

    //m_cache_bmp.SetSize(w, h);
    memdc.SelectObject(wxNullBitmap);

    // hide the control and end the editing
    m_control->Show(false);
    m_editing = false;
}

bool CompControl::isEditing() const
{
    return m_editing;
}

bool CompControl::canCut() const
{
    return false;
}

bool CompControl::canCopy() const
{
    return false;
}

bool CompControl::canPaste() const
{
    return false;
}

void CompControl::cut()
{
}

void CompControl::copy()
{
}

void CompControl::paste()
{
}

void CompControl::selectAll()
{
}

void CompControl::selectNone()
{
}

void CompControl::clear(bool text)
{
}

void CompControl::onKey(IEventPtr evt)
{
    // if we're not in the at-target phase,
    // we're done
    if (evt->getPhase() != EVENT_AT_TARGET)
        return;

    IKeyEventPtr key_evt = evt;
    if (key_evt.isNull())
        return;
}

void CompControl::onMouse(IEventPtr evt)
{
    // if we're not in the at-target phase,
    // we're done
    if (evt->getPhase() != EVENT_AT_TARGET)
        return;

    IMouseEventPtr mouse_evt = evt;
    if (mouse_evt.isNull())
        return;
}

void CompControl::clearCache()
{
    // if the width or height has changed, clear the bitmap
    int w, h;
    getSize(&w, &h);
    
    if (w != m_cache_width || h != m_cache_height)
    {
        m_cache_bmp = wxNullBitmap;
        m_cache_width = w;
        m_cache_height = h;
    }
}


}; // namespace kcanvas

