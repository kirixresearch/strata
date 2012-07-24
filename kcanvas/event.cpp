/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2006-11-01
 *
 */


#include "kcanvas_int.h"
#include "event.h"


namespace kcanvas
{


EventTarget::EventTarget()
{
}

EventTarget::EventTarget(const EventTarget& c)
{
    m_event_handlers = c.m_event_handlers;
}

EventTarget::~EventTarget()
{
}

bool EventTarget::addEventHandler(const wxString& name,
                                  IEventHandlerPtr handler,
                                  bool capture_phase)
{
    // see if an event listener has already been added;
    // if so, we're done
    std::vector<EventHandlerMap>::iterator it, it_end;
    it_end = m_event_handlers.end();

    for (it = m_event_handlers.begin(); it != it_end; ++it)
    {
        if (it->m_name == name &&
            it->m_handler == handler &&
            it->m_capture_phase == capture_phase)
        {
            return false;
        }
    }
    
    // map the event listener to an event
    EventHandlerMap e;
    e.m_handler = handler;
    e.m_name = name;
    e.m_capture_phase = capture_phase;

    m_event_handlers.push_back(e);
    return true;
}

bool EventTarget::removeEventHandler(const wxString& name,
                                     IEventHandlerPtr handler,
                                     bool capture_phase)
{
    // if an event listener has been added, remove it; note:
    // since the handler with the given name and phase can
    // only be added once, we need only remove the first one
    // we find with the same criteria we used to add it
    std::vector<EventHandlerMap>::iterator it, it_end;
    it_end = m_event_handlers.end();

    for (it = m_event_handlers.begin(); it != it_end; ++it)
    {
        if (it->m_name == name &&
            it->m_handler == handler &&
            it->m_capture_phase == capture_phase)
        {
            m_event_handlers.erase(it);
            return true;
        }
    }
    
    // no event listener to delete; return false
    return false;
}

void EventTarget::removeAllEventHandlers()
{
    m_event_handlers.clear();
}

void EventTarget::dispatchEvent(IEventPtr evt)
{
    // if the event's been cancelled, we're done
    if (evt->isCancelled())
        return;

    // create a copy of the listeners so we can iterate
    // through them completely in case a given listener 
    // adds or removes other listeners from this target
    std::vector<EventHandlerMap> handlers;
    handlers = m_event_handlers;

    // get the event phase; if the event is not in the capture
    // phase, the target phase, or the bubble phase, we're done
    int phase = evt->getPhase();
    bool capture_phase = (phase == EVENT_CAPTURING_PHASE);
    if (phase != EVENT_CAPTURING_PHASE &&
        phase != EVENT_AT_TARGET &&
        phase != EVENT_BUBBLING_PHASE)
    {
        return;
    }

    // look through the event listeners
    std::vector<EventHandlerMap>::iterator it, it_end;
    it_end = handlers.end();

    for (it = handlers.begin(); it != it_end; ++it)
    {
        // see if the event listener is mapped to a particular 
        // event name and event capture phase; if it is, 
        // handle the event and set the handled flag to
        // true to indicate the event has been handled
        if (it->m_name == evt->getName() &&
            it->m_capture_phase == capture_phase)
        {
            it->m_handler->handleEvent(evt);
            evt->handle(true);
        }
    }
}

EventHandler::EventHandler()
{
}

EventHandler::EventHandler(const EventHandler& c)
{
}

EventHandler::~EventHandler()
{
}

void EventHandler::handleEvent(IEventPtr evt)
{
    sigEvent().fire(evt);
}

EventTimer::EventTimer() : wxTimer()
{
    // default interval: 100 milliseconds
    m_interval = 100;
}

EventTimer::EventTimer(const EventTimer& c)
{
    m_event = c.m_event;
    m_interval = c.m_interval;
}

EventTimer::~EventTimer()
{
}

void EventTimer::setEvent(IEventPtr evt)
{
    // set the event to fire
    m_event = evt;
}

IEventPtr EventTimer::getEvent()
{
    // return the event being fired
    return m_event;
}

void EventTimer::setInterval(int interval)
{
    // set the interval to fire the event
    m_interval = interval;
}

int EventTimer::getInterval()
{
    // return the interval that the event is to be fired
    return m_interval;
}

void EventTimer::start()
{
    // start sending the event
    Start(m_interval);
}

void EventTimer::stop()
{
    // stop sending the event
    Stop();
}

void EventTimer::Notify()
{
    // if we don't have an event to fire, we're done
    if (m_event.isNull())
        return;

    // dispatch the event to the target
    IEventTargetPtr target = m_event->getTarget();
    target->dispatchEvent(m_event);
}

Event::Event()
{
    m_timestamp = 0;
    m_phase = EVENT_NO_PHASE;
    m_async = false;
    m_handled = false;
    m_cancelled = false;
}

Event::Event(const Event& c)
{
    // copy the parameters that give the event it's
    // identity but not those that store information
    // about how it's been handled; this allows cloned
    // events to get a "fresh" start
    m_name = c.m_name;
    m_target = c.m_target;
    m_timestamp = c.m_timestamp;
    m_phase = EVENT_NO_PHASE;
    m_async = c.m_async;
    m_handled = false;
    m_cancelled = false;
}

Event::~Event()
{
}

IEventPtr Event::create(const wxString& name,
                        IEventTargetPtr target,
                        bool async)
{
    Event* e = new Event;
    e->setName(name);
    e->setTarget(target);
    e->setTimeStamp(::wxGetUTCTime());
    e->async(async);
    return static_cast<IEvent*>(e);
}

IEventPtr Event::clone()
{
    Event* e = new Event(*this);
    return static_cast<IEvent*>(e);
}

void Event::setName(const wxString& name)
{
    m_name = name;
}

const wxString& Event::getName()
{
    return m_name;
}

void Event::setTarget(IEventTargetPtr target)
{
    m_target = target;
}

IEventTargetPtr Event::getTarget()
{
    return m_target;
}

void Event::setTimeStamp(long timestamp)
{
    m_timestamp = timestamp;
}

long Event::getTimeStamp()
{
    return m_timestamp;
}

void Event::setPhase(int phase)
{
    m_phase = phase;
}

int Event::getPhase()
{
    return m_phase;
}

void Event::async(bool async)
{
    m_async = async;
}

bool Event::isAsync()
{
    return m_async;
}

void Event::handle(bool handled)
{
    m_handled = handled;
}

bool Event::isHandled()
{
    return m_handled;
}

void Event::stopPropogation()
{
    m_cancelled = true;
}

bool Event::isCancelled()
{
    return m_cancelled;
}

MouseEvent::MouseEvent()
{
    m_x = 0;
    m_y = 0;
    m_client_x = 0;
    m_client_y = 0;
    m_rotation = 0;
    m_altdown = false;
    m_ctrldown = false;
    m_shiftdown = false;
}

MouseEvent::MouseEvent(const MouseEvent& c) : Event(c)
{
    m_x = c.m_x;
    m_y = c.m_y;
    m_client_x = c.m_client_x;
    m_client_y = c.m_client_y;
    m_rotation = c.m_rotation;
    m_altdown = c.m_altdown;
    m_ctrldown = c.m_ctrldown;
    m_shiftdown = c.m_shiftdown;
}

MouseEvent::~MouseEvent()
{
}

IEventPtr MouseEvent::create(const wxString& name,
                             IEventTargetPtr target,
                             bool async)
{
    MouseEvent* e = new MouseEvent;
    e->setName(name);
    e->setTarget(target);
    e->setTimeStamp(::wxGetUTCTime());
    e->async(async);
    return static_cast<IEvent*>(e);
}

IEventPtr MouseEvent::clone()
{
    MouseEvent* e = new MouseEvent(*this);
    return static_cast<IEvent*>(e);
}

void MouseEvent::setX(int x)
{
    m_x = x;
}

int MouseEvent::getX()
{
    return m_x;
}

void MouseEvent::setY(int y)
{
    m_y = y;
}

int MouseEvent::getY()
{
    return m_y;
}

void MouseEvent::setClientX(int x)
{
    m_client_x = x;
}

int MouseEvent::getClientX()
{
    return m_client_x;
}

void MouseEvent::setClientY(int y)
{
    m_client_y = y;
}

int MouseEvent::getClientY()
{
    return m_client_y;
}

void MouseEvent::setRotation(int r)
{
    m_rotation = r;
}

int MouseEvent::getRotation()
{
    return m_rotation;
}

void MouseEvent::setAltDown(bool key_down)
{
    m_altdown = key_down;
}

bool MouseEvent::isAltDown()
{
    return m_altdown;
}

void MouseEvent::setCtrlDown(bool key_down)
{
    m_ctrldown = key_down;
}

bool MouseEvent::isCtrlDown()
{
    return m_ctrldown;
}

void MouseEvent::setShiftDown(bool key_down)
{
    m_shiftdown = key_down;
}

bool MouseEvent::isShiftDown()
{
    return m_shiftdown;
}

KeyEvent::KeyEvent()
{
    m_x = 0;
    m_y = 0;
    m_client_x = 0;
    m_client_y = 0;
    m_keycode = 0;
    m_altdown = false;
    m_ctrldown = false;
    m_shiftdown = false;
}

KeyEvent::KeyEvent(const KeyEvent& c) : Event(c)
{
    m_x = c.m_x;
    m_y = c.m_y;
    m_client_x = c.m_client_x;
    m_client_y = c.m_client_y;
    m_keycode = c.m_keycode;
    m_altdown = c.m_altdown;
    m_ctrldown = c.m_ctrldown;
    m_shiftdown = c.m_shiftdown;
}

KeyEvent::~KeyEvent()
{
}

IEventPtr KeyEvent::create(const wxString& name,
                           IEventTargetPtr target,
                           bool async)
{
    KeyEvent* e = new KeyEvent;
    e->setName(name);
    e->setTarget(target);
    e->setTimeStamp(::wxGetUTCTime());
    e->async(async);
    return static_cast<IEvent*>(e);
}

IEventPtr KeyEvent::clone()
{
    KeyEvent* e = new KeyEvent(*this);
    return static_cast<IEvent*>(e);
}

void KeyEvent::setX(int x)
{
    m_x = x;
}

int KeyEvent::getX()
{
    return m_x;
}

void KeyEvent::setY(int y)
{
    m_y = y;
}

int KeyEvent::getY()
{
    return m_y;
}

void KeyEvent::setClientX(int x)
{
    m_client_x = x;
}

int KeyEvent::getClientX()
{
    return m_client_x;
}

void KeyEvent::setClientY(int y)
{
    m_client_y = y;
}

int KeyEvent::getClientY()
{
    return m_client_y;
}

void KeyEvent::setKeyCode(int keycode)
{
    m_keycode = keycode;
}

int KeyEvent::getKeyCode()
{
    return m_keycode;
}

void KeyEvent::setAltDown(bool key_down)
{
    m_altdown = key_down;
}

bool KeyEvent::isAltDown()
{
    return m_altdown;
}

void KeyEvent::setCtrlDown(bool key_down)
{
    m_ctrldown = key_down;
}

bool KeyEvent::isCtrlDown()
{
    return m_ctrldown;
}

void KeyEvent::setShiftDown(bool key_down)
{
    m_shiftdown = key_down;
}

bool KeyEvent::isShiftDown()
{
    return m_shiftdown;
}

NotifyEvent::NotifyEvent()
{
    m_allowed = true;
}

NotifyEvent::NotifyEvent(const NotifyEvent& c) : Event(c)
{
    m_properties = c.m_properties;
    m_allowed = m_allowed;
}

NotifyEvent::~NotifyEvent()
{
}

IEventPtr NotifyEvent::create(const wxString& name,
                              IEventTargetPtr target,
                              bool async)
{
    NotifyEvent* e = new NotifyEvent;
    e->setName(name);
    e->setTarget(target);
    e->setTimeStamp(::wxGetUTCTime());
    e->async(async);
    return static_cast<IEvent*>(e);
}

IEventPtr NotifyEvent::clone()
{
    NotifyEvent* e = new NotifyEvent(*this);
    return static_cast<IEvent*>(e);
}

void NotifyEvent::veto(bool veto)
{
    m_allowed = !veto;
}

bool NotifyEvent::isAllowed()
{
    return m_allowed;
}

void NotifyEvent::addProperty(const wxString& prop_name, const PropertyValue& value)
{
    m_properties.add(prop_name, value);
}

bool NotifyEvent::getProperty(const wxString& prop_name, PropertyValue& value)
{
    return m_properties.get(prop_name, value);
}

DragDropEvent::DragDropEvent()
{
    m_x = 0;
    m_y = 0;
}

DragDropEvent::DragDropEvent(const DragDropEvent& c)
{
    m_data = c.m_data;
    m_x = c.m_x;
    m_y = c.m_y;
}

DragDropEvent::~DragDropEvent()
{
}

IEventPtr DragDropEvent::create(const wxString& name,
                                IEventTargetPtr target,
                                bool async)
{
    DragDropEvent* e = new DragDropEvent;
    e->setName(name);
    e->setTarget(target);
    e->setTimeStamp(::wxGetUTCTime());
    e->async(async);
    return static_cast<IEvent*>(e);
}
                                
IEventPtr DragDropEvent::clone()
{
    DragDropEvent* e = new DragDropEvent(*this);
    return static_cast<IEvent*>(e);
}

void DragDropEvent::setX(int x)
{
    m_x = x;
}

int DragDropEvent::getX()
{
    return m_x;
}

void DragDropEvent::setY(int y)
{
    m_y = y;
}

int DragDropEvent::getY()
{
    return m_y;
}

void DragDropEvent::setData(IComponentPtr component)
{
    m_data = component;
}

IComponentPtr DragDropEvent::getData()
{
    return m_data;
}


}; // namespace kcanvas

