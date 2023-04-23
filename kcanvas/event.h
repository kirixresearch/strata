/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Canvas Control
 * Author:   Aaron L. Williams
 * Created:  2006-11-01
 *
 */


#ifndef H_KCANVAS_EVENT_H
#define H_KCANVAS_EVENT_H


#include <wx/timer.h>
#include "property.h"


namespace kcanvas
{


// Canvas Event Classes

enum
{
    EVENT_NO_PHASE = 0,
    EVENT_CAPTURING_PHASE = 1,
    EVENT_AT_TARGET = 2,
    EVENT_BUBBLING_PHASE = 3
};


class EventHandlerMap
{
public:
    IEventHandlerPtr m_handler;
    wxString m_name;
    bool m_capture_phase;
};


class EventTarget : public IEventTarget
{
    XCM_CLASS_NAME("kcanvas.EventTarget")
    XCM_BEGIN_INTERFACE_MAP(EventTarget)
        XCM_INTERFACE_ENTRY(IEventTarget)
    XCM_END_INTERFACE_MAP()

public:

    EventTarget();
    EventTarget(const EventTarget& c);
    virtual ~EventTarget();

    bool addEventHandler(const wxString& name,
                         IEventHandlerPtr handler,
                         bool capture_phase = false);
    bool removeEventHandler(const wxString& name,
                            IEventHandlerPtr handler,
                            bool capture_phase = false);
    void removeAllEventHandlers();
    void dispatchEvent(IEventPtr evt);

private:

    std::vector<EventHandlerMap> m_event_handlers;
};


class EventHandler : public IEventHandler
{
    XCM_CLASS_NAME("kcanvas.EventHandler")
    XCM_BEGIN_INTERFACE_MAP(EventHandler)
        XCM_INTERFACE_ENTRY(IEventHandler)
    XCM_END_INTERFACE_MAP()

    XCM_IMPLEMENT_SIGNAL1(sigEvent, IEventPtr)

public:

    EventHandler();
    EventHandler(const EventHandler& c);
    virtual ~EventHandler();

    void handleEvent(IEventPtr evt);
};


class EventTimer : public wxTimer,
                   public IEventTimer
{
    XCM_CLASS_NAME("kcanvas.EventTimer")
    XCM_BEGIN_INTERFACE_MAP(EventTimer)
        XCM_INTERFACE_ENTRY(IEventTimer)
    XCM_END_INTERFACE_MAP()

public:

    EventTimer();
    EventTimer(const EventTimer& c);
    virtual ~EventTimer();

    void setEvent(IEventPtr evt);
    IEventPtr getEvent();

    void setInterval(int interval);
    int getInterval();

    void start();
    void stop();

public:

    // override wxTimer function for custom behavior
    void Notify();

private:

    // event to fire, and interval to fire event
    IEventPtr m_event;
    int m_interval;
};


class Event : public IEvent
{
    XCM_CLASS_NAME("kcanvas.Event")
    XCM_BEGIN_INTERFACE_MAP(Event)
        XCM_INTERFACE_ENTRY(IEvent)
    XCM_END_INTERFACE_MAP()

public:

    Event();
    Event(const Event& c);
    virtual ~Event();

    static IEventPtr create(const wxString& name,
                            IEventTargetPtr target,
                            bool async = false);
    IEventPtr clone();
    
    void setName(const wxString& name);
    const wxString& getName();

    void setTarget(IEventTargetPtr target);
    IEventTargetPtr getTarget();

    void setTimeStamp(long timestamp);
    long getTimeStamp();

    void setPhase(int phase);
    int getPhase();

    void async(bool async = false);
    bool isAsync();

    void handle(bool handled = true);
    bool isHandled();

    void stopPropogation();
    bool isCancelled();

private:

    wxString m_name;
    IEventTargetPtr m_target;
    long m_timestamp;
    int m_phase;
    bool m_async;
    bool m_handled;
    bool m_cancelled;
};


class MouseEvent : public Event,
                   public IMouseEvent
{
    XCM_CLASS_NAME("kcanvas.MouseEvent")
    XCM_BEGIN_INTERFACE_MAP(MouseEvent)
        XCM_INTERFACE_ENTRY(IMouseEvent)
        XCM_INTERFACE_CHAIN(Event)
    XCM_END_INTERFACE_MAP()

public:

    MouseEvent();
    MouseEvent(const MouseEvent& c);
    virtual ~MouseEvent();

    static IEventPtr create(const wxString& name,
                            IEventTargetPtr target,
                            bool async = false);
    IEventPtr clone();

    void setX(int x);
    int getX();
    
    void setY(int y);
    int getY();
    
    void setClientX(int x);
    int getClientX();
    
    void setClientY(int y);
    int getClientY();

    void setRotation(int r);
    int getRotation();

    void setAltDown(bool key_down);
    bool isAltDown();
    
    void setCtrlDown(bool key_down);
    bool isCtrlDown();
    
    void setShiftDown(bool key_down);
    bool isShiftDown();
    
private:

    int m_x;
    int m_y;
    int m_client_x;
    int m_client_y;
    int m_rotation;
    
    bool m_altdown;
    bool m_ctrldown;
    bool m_shiftdown;
};


class KeyEvent : public Event,
                 public IKeyEvent
{
    XCM_CLASS_NAME("kcanvas.KeyEvent")
    XCM_BEGIN_INTERFACE_MAP(KeyEvent)
        XCM_INTERFACE_ENTRY(IKeyEvent)
        XCM_INTERFACE_CHAIN(Event)
    XCM_END_INTERFACE_MAP()

public:

    KeyEvent();
    KeyEvent(const KeyEvent& c);
    virtual ~KeyEvent();

    static IEventPtr create(const wxString& name,
                            IEventTargetPtr target,
                            bool async = false);
    IEventPtr clone();

    void setX(int x);
    int getX();
    
    void setY(int y);
    int getY();

    void setClientX(int x);
    int getClientX();
    
    void setClientY(int y);
    int getClientY();
    
    void setKeyCode(int keycode);
    int getKeyCode();

    void setAltDown(bool key_down);
    bool isAltDown();
    
    void setCtrlDown(bool key_down);
    bool isCtrlDown();
    
    void setShiftDown(bool key_down);
    bool isShiftDown();

private:

    int m_x;
    int m_y;
    int m_client_x;
    int m_client_y;
    int m_keycode;
    
    bool m_altdown;
    bool m_ctrldown;
    bool m_shiftdown;
};


class NotifyEvent : public Event,
                    public INotifyEvent
{
    XCM_CLASS_NAME("kcanvas.NotifyEvent")
    XCM_BEGIN_INTERFACE_MAP(NotifyEvent)
        XCM_INTERFACE_ENTRY(INotifyEvent)
        XCM_INTERFACE_CHAIN(Event)
    XCM_END_INTERFACE_MAP()

public:

    NotifyEvent();
    NotifyEvent(const NotifyEvent& c);
    virtual ~NotifyEvent();

    static IEventPtr create(const wxString& name,
                            IEventTargetPtr target,
                            bool async = false);
    IEventPtr clone();

    void veto(bool veto = true);
    bool isAllowed();
    
    void addProperty(const wxString& prop_name, const PropertyValue& value);
    bool getProperty(const wxString& prop_name, PropertyValue& value);
    
private:

    Properties m_properties;
    bool m_allowed;
};


class DragDropEvent : public Event,
                      public IDragDropEvent
{
    XCM_CLASS_NAME("kcanvas.DragDropEvent")
    XCM_BEGIN_INTERFACE_MAP(DragDropEvent)
        XCM_INTERFACE_ENTRY(IDragDropEvent)
        XCM_INTERFACE_CHAIN(Event)
    XCM_END_INTERFACE_MAP()

public:

    DragDropEvent();
    DragDropEvent(const DragDropEvent& c);
    virtual ~DragDropEvent();

    static IEventPtr create(const wxString& name,
                            IEventTargetPtr target,
                            bool async = false);
    IEventPtr clone();

    void setX(int x);
    int getX();
    
    void setY(int y);
    int getY();

    void setData(IComponentPtr component);
    IComponentPtr getData();
    
private:

    IComponentPtr m_data;
    int m_x;
    int m_y;
};


}; // namespace kcanvas


#endif

