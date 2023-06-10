/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-12-06
 *
 */


#ifndef H_APP_SCRIPTAPP_H
#define H_APP_SCRIPTAPP_H


#include "scripthost.h"
#include "../kscript/kscript.h"
#include "../kscript/jsobject.h"
#include "../kscript/jsarray.h"
#include <queue>
#include <set>


// -- forwards --

class Application;
class Event;
class Form;
class FormComponent;
class ScriptHostBase;


// -- enums --

enum ScriptHostEventFlag
{
    eventFlagNone = 0,
    eventFlagProcessNow = 1 << 0,
    eventFlagNoGuiBlock = 1 << 1
};


// -- event classes --

class PendingEvent
{
public:

    PendingEvent()
    {
        app = NULL;
        jsevent = NULL;
        sender = NULL;
        args = NULL;
        flags = 0;
    }
    
    PendingEvent(const PendingEvent& c)
    {
        app = c.app;
        jsevent = c.jsevent;
        sender = c.sender;
        args = c.args;
        flags = c.flags;
    }
    
    PendingEvent& operator=(const PendingEvent& c)
    {
        if (&c != this)
        {
            app = c.app;
            jsevent = c.jsevent;
            sender = c.sender;
            args = c.args;
            flags = c.flags;
        }

        return *this;
    } 

    Application* app;
    Event* jsevent;
    kscript::ValueObject* sender;
    kscript::Value* args;
    unsigned int flags;
};


class EventSink : public kscript::ValueObjectEvents
{
public:

    bool enabled;
    kscript::ValueObject* obj;
    kscript::Value method;

    EventSink()
    {
        enabled = true;
        obj = NULL;
    }
        
    ~EventSink()
    {
        if (obj && (long long)obj != 1)
        {
            obj->unregisterEventListener(this);
        }
    }
    
    virtual void onDestroy(kscript::ValueObject*)
    {
        enabled = false;
        obj = NULL;
    }
};


class Event : public ScriptHostBase
{
    BEGIN_KSCRIPT_CLASS("Event", Event)
        KSCRIPT_METHOD("constructor", Event::constructor)
        KSCRIPT_METHOD("connect", Event::connect)
        KSCRIPT_METHOD("disconnect", Event::disconnect)
        KSCRIPT_METHOD("disconnectAll", Event::disconnectAll)
        KSCRIPT_METHOD("isConnected", Event::isConnected)
        KSCRIPT_METHOD("getSinkCount", Event::getSinkCount)
        KSCRIPT_METHOD("fire", Event::zfire)
    END_KSCRIPT_CLASS()

public:

    Event();
    ~Event();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void connect(kscript::ExprEnv* env, kscript::Value* retval);
    void disconnect(kscript::ExprEnv* env, kscript::Value* retval);
    void disconnectAll(kscript::ExprEnv* env, kscript::Value* retval);
    void isConnected(kscript::ExprEnv* env, kscript::Value* retval);
    void getSinkCount(kscript::ExprEnv* env, kscript::Value* retval);
    void zfire(kscript::ExprEnv* env, kscript::Value* retval);

public:
        
    virtual void fire(
                 kscript::ExprEnv* env,
                 kscript::ValueObject* sender,
                 kscript::Value* event_args);
                 
    size_t getSinkCountInternal() const;
    
    void connectInternal(kscript::Value* func_value);
    void connectInternal(kscript::Value* obj, kscript::Value* func_value);
    
private:

    std::vector<EventSink*> m_sinks;
};


// -- specialized PaintEvent --
class PaintEvent : public Event
{
public:

   void fire(kscript::ExprEnv* env,
             kscript::ValueObject* sender,
             kscript::Value* event_args);
};


// -- App class (for scripts/programs) --

class AppEvtHandler : public wxEvtHandler
{
public:

    AppEvtHandler();
    bool ProcessEvent(wxEvent& evt);

public:
    
    Application* m_app;
    int m_events_processing;
};


class Application : public kscript::ValueObject
{
    enum
    {
        ShutdownDefault = 1,
        ShutdownOff = 2,
        ShutdownAfterAllFormsClose = 3
    };

    BEGIN_KSCRIPT_CLASS("Application", Application)
        KSCRIPT_GUI_METHOD("constructor", Application::constructor)
        KSCRIPT_METHOD("setShutdownStyle", Application::setShutdownStyle)
        KSCRIPT_METHOD("execute", Application::execute)
        KSCRIPT_METHOD("run", Application::run)
        KSCRIPT_METHOD("exit", Application::exit)
        KSCRIPT_STATIC_METHOD("getStartupPath", Application::getStartupPath)

        KSCRIPT_CONSTANT_INTEGER("ShutdownDefault",            ShutdownDefault)
        KSCRIPT_CONSTANT_INTEGER("ShutdownOff",                ShutdownOff)
        KSCRIPT_CONSTANT_INTEGER("ShutdownAfterAllFormsClose", ShutdownAfterAllFormsClose)
    END_KSCRIPT_CLASS()

public:

    friend class AppEvtHandler;

    Application();
    ~Application();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void setShutdownStyle(kscript::ExprEnv* env, kscript::Value* retval);
    void execute(kscript::ExprEnv* env, kscript::Value* retval);
    void run(kscript::ExprEnv* env, kscript::Value* retval);
    void exit(kscript::ExprEnv* env, kscript::Value* retval);
    static void getStartupPath(kscript::ExprEnv* env, void* param, kscript::Value* retval);
    
public:

    void wakeUpQueue();
    void exitModal();
    
    void postEvent(Event* evt,
                   kscript::ValueObject* sender,
                   kscript::Value* event_args = NULL,
                   unsigned event_flags = 0);
    void processEvents(kscript::ExprEnv* env);

    void processWxEvent(wxEvent& evt);

    wxWindow* getTempParent();
    
    void registerForm(Form* f);
    void unregisterForm(Form* f);

public: // signals

    xcm::signal0 sigDestructing;
    
public:

    wxWindow* m_temp_parent;
    
    wxCondition* m_loop_condition;
    wxMutex* m_loop_condition_mutex;
        
    wxMutex m_queue_mutex;
    std::queue<PendingEvent> m_queue;

    wxMutex m_forms_mutex;
    std::set<Form*> m_forms;

    AppEvtHandler m_evt_sink;

    wxMutex m_obj_mutex;
    bool m_exit;
    bool m_exit_modal;
    bool m_wakeup;

    int m_shutdown_mode;
};





// -- Script Command Event Router --
// this class routes messages from menus to any script object
// that wants to receive them

class ScriptCommandEventRouter : public wxEvtHandler
{
public:

    bool ProcessEvent(wxEvent& evt);
    
    void registerCommandReceiver(int id, ScriptHostBase* comp);
    void unregisterCommandReceiver(int id);
    
private:

    std::map<int, ScriptHostBase*> m_map;
    kl::mutex m_map_mutex;
};






class zTimer : public ScriptHostBase,
               public wxTimer
{
    // note: starting a timer has to take place in the main
    // thread, so start() is a GUI method; use GUI methods
    // on the other methods as a safeguard

    BEGIN_KSCRIPT_CLASS("Timer", zTimer)
        KSCRIPT_METHOD("constructor", zTimer::constructor)
        KSCRIPT_GUI_METHOD("isRunning", zTimer::isRunning)
        KSCRIPT_GUI_METHOD("setInterval", zTimer::setInterval)
        KSCRIPT_GUI_METHOD("getInterval", zTimer::getInterval)
        KSCRIPT_GUI_METHOD("start", zTimer::start)
        KSCRIPT_GUI_METHOD("stop", zTimer::stop)
    END_KSCRIPT_CLASS()

public:

    zTimer();
    ~zTimer();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void isRunning(kscript::ExprEnv* env, kscript::Value* retval);
    void setInterval(kscript::ExprEnv* env, kscript::Value* retval);
    void getInterval(kscript::ExprEnv* env, kscript::Value* retval);
    void start(kscript::ExprEnv* env, kscript::Value* retval);
    void stop(kscript::ExprEnv* env, kscript::Value* retval);

public:

    // override wxTimer function for custom behavior
    void Notify();

private:

    int m_interval;
    ApplicationWeakReference m_app;
    kscript::Value m_tick_event;
};




#endif

