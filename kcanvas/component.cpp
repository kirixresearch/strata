/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2003-08-21
 *
 */


#include "kcanvas_int.h"
#include "canvas.h"
#include "property.h"
#include "event.h"
#include "util.h"


namespace kcanvas
{


// component implementation
Component::Component()
{
    // canvas to which component belongs; set to null
    // initially
    m_canvas = NULL;

    // parent of the component in the canvas hiearchy;
    // set to null initially, and set to the component's
    // parent by the canvas
    m_parent = NULL;

    // absolute position and size of the component;
    // set by the canvas
    m_absolute_x = 0;
    m_absolute_y = 0;
    m_absolute_w = 0;
    m_absolute_h = 0;

    // initialize the properties with those from the
    // property enumeration
    initProperties(m_properties);

    // add event handlers
    addEventHandler(EVENT_RENDER, &Component::onRender);
}

Component::~Component()
{
    removeAll();
}

IComponentPtr Component::create()
{
    return static_cast<IComponent*>(new Component);
}

void Component::initProperties(Properties& properties)
{
    // enumerate the default properties
    Properties props;
    props.add(PROP_COMP_TYPE, COMP_TYPE_COMPONENT);
    props.add(PROP_VISIBLE, true);
    props.add(PROP_ORIGIN_X, 0);
    props.add(PROP_ORIGIN_Y, 0);
    props.add(PROP_SIZE_W, 0);
    props.add(PROP_SIZE_H, 0);
    props.add(PROP_COLOR_FG, Color(COLOR_BLACK));
    props.add(PROP_COLOR_BG, Color(COLOR_WHITE));
    props.add(PROP_LINE_WIDTH, 1);
    props.add(PROP_BORDER_TOP_STYLE, BORDER_STYLE_NONE);
    props.add(PROP_BORDER_BOTTOM_STYLE, BORDER_STYLE_NONE);
    props.add(PROP_BORDER_LEFT_STYLE, BORDER_STYLE_NONE);
    props.add(PROP_BORDER_RIGHT_STYLE, BORDER_STYLE_NONE);
    properties = props;
}

IComponentPtr Component::clone()
{
    // note: clones the given component; this used to use a copy 
    // constructor, but this caused some problems with copying the 
    // event handlers because the event handlers couldn't readily 
    // be copied because in most cases, they contain pointers to the 
    // component they are added, and it's difficult to know how to 
    // reset these; as a result, the alternatives were to 1) add the
    // event handlers in the copy constructor, which duplicates the
    // list in the regular copy constructor, possibly causing
    // inconsistencies and bugs; 2) create a separate init() function
    // to factor out the event handler creation and add this to both
    // constructors, creating too much machinary when taken alongside 
    // both copy constructors, and the creation-related create(), 
    // clone() and copy() functions; and 3) use the basic, non-copy
    // constructor to create a stock component with the normal event
    // handlers, then copy the rest of the properties with the copy
    // function; we follow the third option below, which is a little
    // clunky in that properties are first created in the constructor,
    // then reset in the copy operation, but it's the simplest option
    // from the code standpoint

    Component* c = new Component;
    c->copy(this);

    return static_cast<IComponent*>(c);
}

void Component::copy(IComponentPtr component)
{
    // note: this function copies the properties and children
    // components of the input components to this component;
    // the children are not cloned - only their references are 
    // copied

    // if the input component is null, we're done
    if (component.isNull())
        return;

    // copy the properties
    Component* c = static_cast<Component*>(component.p);
    m_properties = c->m_properties;

    // copy the references to the children of the
    // component we're copying
    m_components = c->m_components;
}

wxString Component::getId()
{
    // if we don't have a unique id, generate one
    if (m_id.length() == 0)
        m_id = getUniqueString();

    return m_id;
}

wxString Component::getType()
{
    PropertyValue value;
    getProperty(PROP_COMP_TYPE, value);
    return value.getString();
}

bool Component::add(IComponentPtr component)
{
    // note: this function adds a child component to this component;
    // it doesn't set the parent of the child since the canvas 
    // establishes the parent-child and we don't know whether this 
    // component will be part of the canvas hierarchy or not (it might 
    // be used for something else, such as a canvas history or for 
    // packaging up components for drag&drop)

    // don't allow null children
    if (component.isNull())
        return false;

    // a component cannot contain itself as a child component
    IComponentPtr comp = this;
    if (comp == component)
        return false;

    // if the component already exists, don't add it; return false
    std::vector<IComponentPtr>::iterator it;
    it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
        return false;

    // add the component
    m_components.push_back(component);

    // if the canvas member isn't null, set the canvas member
    // of the children to the same canvas as this component;
    // this will allow multiple components to have a particular
    // component as a child, but will only allow it to be
    // owned by one canvas at a time
    if (m_canvas != NULL)
    {
        std::vector<IComponentPtr> child_components;
        std::vector<IComponentPtr>::iterator it, it_end;

        getAllSubComponents(child_components, component);
        child_components.push_back(component);

        it_end = child_components.end();
        for (it = child_components.begin(); it != it_end; ++it)
        {
            if (it->isNull())
                continue;

            Component* c = static_cast<Component*>(it->p);
            c->m_canvas = m_canvas;
        }
    }
    
    return true;
}

bool Component::remove(IComponentPtr component)
{
    // if the child is null, there's nothing
    // to remove; return false
    if (component.isNull())
        return false;

    std::vector<IComponentPtr>::iterator it, it_end;

    // if we can't find the component to remove, return false
    it = std::find(m_components.begin(), m_components.end(), component);
    if (it == m_components.end())
        return false;

    // set the canvas and parent members of the component 
    // being removed and all its sub-components to NULL
    std::vector<IComponentPtr> children;
    getAllSubComponents(children, component);
    children.push_back(component);

    it_end = children.end();
    for (it = children.begin(); it != it_end; ++it)
    {
        Component* c = static_cast<Component*>(it->p);
        c->m_canvas = NULL;
        c->m_parent = NULL;
    }

    // remove the component and return true
    m_components.erase(it);
    return true;
}

void Component::removeAll()
{
    // set the canvas and parent members of all the 
    // sub-components being removed to NULL
    std::vector<Component*> children;
    getAllSubComponents(children, this);
    
    std::vector<Component*>::iterator it, it_end;
    it_end = children.end();
    
    for (it = children.begin(); it != it_end; ++it)
    {
        (*it)->m_canvas = NULL;
        (*it)->m_parent = NULL;
    }

    // remove the sub components
    m_components.clear();
}

void Component::getComponents(std::vector<IComponentPtr>& components) const
{
    components = m_components;
}

void Component::addProperty(const wxString& prop_name, const PropertyValue& value)
{
    m_properties.add(prop_name, value);
}

void Component::addProperties(const Properties& properties)
{
    m_properties.add(properties);
}

void Component::removeProperty(const wxString& prop_name)
{
    m_properties.remove(prop_name);
}

bool Component::setProperty(const wxString& prop_name, const PropertyValue& value)
{
    return m_properties.set(prop_name, value);
}

bool Component::setProperties(const Properties& properties)
{
    return m_properties.set(properties);
}

bool Component::getProperty(const wxString& prop_name, PropertyValue& value) const
{
    return m_properties.get(prop_name, value);
}

bool Component::getProperties(Properties& properties) const
{
    return m_properties.get(properties);
}

Properties& Component::getPropertiesRef()
{
    return m_properties;
}

void Component::setOrigin(int x, int y)
{
    setProperty(PROP_ORIGIN_X, x);
    setProperty(PROP_ORIGIN_Y, y);
}

void Component::getOrigin(int* x, int* y) const
{
    PropertyValue value;

    getProperty(PROP_ORIGIN_X, value);
    *x = value.getInteger();
    
    getProperty(PROP_ORIGIN_Y, value);
    *y = value.getInteger();
}

void Component::setSize(int w, int h)
{
    setProperty(PROP_SIZE_W, w);
    setProperty(PROP_SIZE_H, h);
}

void Component::getSize(int* w, int* h) const
{
    PropertyValue value;

    getProperty(PROP_SIZE_W, value);
    *w = value.getInteger();
    
    getProperty(PROP_SIZE_H, value);
    *h = value.getInteger();
}

void Component::setWidth(int width)
{
    setProperty(PROP_SIZE_W, width);
}

int Component::getWidth() const
{
    PropertyValue value;
    getProperty(PROP_SIZE_W, value);
    return value.getInteger();
}

void Component::setHeight(int height)
{
    setProperty(PROP_SIZE_H, height);
}

int Component::getHeight() const
{
    PropertyValue value;
    getProperty(PROP_SIZE_H, value);
    return value.getInteger();
}

void Component::visible(bool visible)
{
    // note: sets the visible property; if visible is true and 
    // the component is in the canvas hierarchy, then it will 
    // be part of the layout; if visible is false, then the 
    // component won't be part of the layout, even if it's 
    // part of the canvas hierarchy
    
    setProperty(PROP_VISIBLE, visible);
}

bool Component::isVisible() const
{
    // note: returns true if the visible property is set
    // to true; returns false otherwise

    PropertyValue value;
    getProperty(PROP_VISIBLE, value);
    return value.getBoolean();
}

bool Component::contains(int x, int y)
{
    // note: returns true if the point is contained within 
    // the local coordinates of the component; returns false 
    // otherwise

    int w, h;
    getSize(&w, &h);
   
    if (x >= 0 && x <= w && y >= 0 && y <= h)
        return true;
        
    return false;
}

bool Component::intersects(const wxRect& rect)
{
    // note: returns false if the bounds of the rectangle fall 
    // outside the local coordinates of the component; returns 
    // true otherwise

    int w, h;
    getSize(&w, &h);
    
    return (rect.Intersects(wxRect(0,0,w,h)));
}

void Component::extends(wxRect& rect)
{
    // note: this function returns the extent of the content 
    // relative to the origin and size of the component; by default, 
    // the function will return the size of the component; however, 
    // in the case of certain components, such as a text component, 
    // it can be used to return the the actual size of the text 
    // content so the component can be resized or invalidated 
    // properly

    rect.x = 0; rect.y = 0;
    getSize(&rect.width, &rect.height);
}

void Component::render(const wxRect& rect)
{
}

bool Component::addEventHandler(const wxString& name,
                                IEventHandlerPtr handler,
                                bool capture_phase)
{
    return m_event_target.addEventHandler(name, handler, capture_phase);
}

bool Component::removeEventHandler(const wxString& name,
                                   IEventHandlerPtr handler,
                                   bool capture_phase)
{
    return m_event_target.removeEventHandler(name, handler, capture_phase);
}

void Component::removeAllEventHandlers()
{
    m_event_target.removeAllEventHandlers();
}

void Component::dispatchEvent(IEventPtr evt)
{
    // note: when we dispatch an event on a target, we want it 
    // to go to the canvas to handle the capture, at-target, and 
    // bubbling phase; when the canvas it handling the at-target 
    // phase, it calls dispatchEvent() on the target, which in this
    // case, is this function, which started the process; however
    // we can tell where we are in the process by the event phase:
    // when the event first starts out, the phase is set to
    // EVENT_NO_PHASE, because the canvas hasn't yet started to
    // direct the event; a simple check for this flag will tell
    // us whether to send the event to the canvas to begin processing
    // the event, or the canvas is now calling teh dispatchEvent
    // and we need to send the event to the handlers     

    // if the event is at the beginning of it's journey, send
    // it to the canvas, and we're done
    if (evt->getPhase() == EVENT_NO_PHASE)
    {
        IEventTargetPtr target = static_cast<IEventTarget*>(m_canvas);
        if (!target.isNull())
            target->dispatchEvent(evt);

        return;
    }

    // if the event isn't at the beginning of it's journey,
    // we need to handle the event with the handlers
    m_event_target.dispatchEvent(evt);
}

void Component::setCanvas(ICanvasPtr canvas)
{
    if (canvas.isNull())
    {
        m_canvas = NULL;
        return;
    }
    
    m_canvas = static_cast<Canvas*>(canvas.p);
}

ICanvasPtr Component::getCanvas()
{
    if (m_canvas == NULL)
        return xcm::null;

    return static_cast<ICanvas*>(m_canvas);
}

void Component::setParent(IComponentPtr component)
{
    if (component.isNull())
    {
        m_parent = NULL;
        return;
    }

    m_parent = static_cast<Component*>(component.p);
}

IComponentPtr Component::getParent()
{
    if (m_parent == NULL)
        return xcm::null;

    return static_cast<IComponent*>(m_parent);
}

void Component::invalidate(const wxRect& rect)
{
    // fire an invalidate event, which will notify handlers
    // that a part of the component needs to be redrawn

    // if we don't have a valid canvas, we're done
    if (m_canvas == NULL)
        return;

    // if the component isn't visible, there's nothing visible
    // to invalidate, so do nothing
    if (!isVisible())
        return;

    // if no rectangle is specified, invalidate the whole
    // component, plus a small area around the component;
    // otherwise use the specified area
    wxRect update_rect = rect;
    int x = m_canvas->dtom_x(10); // 10 pixel buffer
    
    if (update_rect.IsEmpty())
        update_rect = wxRect(-x, -x, getWidth() + 2*x, getHeight() + 2*x);
    
    // fire the event asynchronously
    INotifyEventPtr notify_evt;
    notify_evt = NotifyEvent::create(EVENT_INVALIDATE, this, true);
    notify_evt->addProperty(EVENT_PROP_X, (int)update_rect.x);
    notify_evt->addProperty(EVENT_PROP_Y, (int)update_rect.y);
    notify_evt->addProperty(EVENT_PROP_W, (int)update_rect.width);
    notify_evt->addProperty(EVENT_PROP_H, (int)update_rect.height);
    dispatchEvent(notify_evt);
}

void Component::onRender(IEventPtr evt)
{
    // if we're not in the at-target phase,
    // we're done
    if (evt->getPhase() != EVENT_AT_TARGET)
        return;

    // unpack the render rectangle from the event
    INotifyEventPtr notify_evt = evt;
    PropertyValue x, y, w, h;
    notify_evt->getProperty(EVENT_PROP_X, x);
    notify_evt->getProperty(EVENT_PROP_Y, y);
    notify_evt->getProperty(EVENT_PROP_W, w);
    notify_evt->getProperty(EVENT_PROP_H, h);
    wxRect rect(x.getInteger(), y.getInteger(), w.getInteger(), h.getInteger());
    
    // call the render function
    render(rect);
}

} // namespace kcanvas

