/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2006-06-20
 *
 */


#include "appmain.h"
#include "scriptapp.h"
#include "scriptgui.h"
#include "scriptmenu.h"
#include "scriptgraphics.h"
#include "toolbars.h"


// these id's are used internally by the Application class
enum
{
    ID_AppExit = 8001,
    ID_EventStart,
    ID_EventEnd,
};


// -- Event class implementation --


// (CLASS) Event
// Category: Application
// Description: A class that represents an event.
// Remarks: The Event class represents an event.

Event::Event()
{
}

Event::~Event()
{
    std::vector<EventSink*>::iterator it;
    for (it = m_sinks.begin(); it != m_sinks.end(); ++it)
        delete *it;
}

// (CONSTRUCTOR) Event.constructor
//
// Description: Creates a new Event.
//
// Syntax: Event()
//
// Remarks: Creates a new Event.

void Event::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    initComponent(env);
}

// (METHOD) Event.connect
//
// Description: Connects an event to a function.
//
// Syntax: function Event.connect(func : Function)
// Syntax: function Event.connect(object : Object,
//                                func : Function)
//
// Remarks: Connects an event to a function specified by |func| or 
//     by |object| and |func|. For example, for a size event,
//     this.size.connect(onFormResized) will connect the size event
//     to the onFormResized function, such that onFormResized will
//     be invoked when the size event occurs.  Similarly,
//     this.size.connect(this, onFormResized) will connect the
//     size event to the onFormResized function using the |this| pointer
//     specified in the |object| parameter
//
// Param(func): The function to which to connect the event.
// Param(class): The object instance to be used as |this| when invoking
//     the event handler

void Event::connect(kscript::ExprEnv* env, kscript::Value* retval)
{
    int param_count = env->getParamCount();

    if (param_count == 1)
    {
        connectInternal(env->getParam(0));
    }
     else if (param_count == 2)
    {
        connectInternal(env->getParam(0), env->getParam(1));
    }
}

// (METHOD) Event.disconnect
//
// Description: Disconnects a specific event handler
//
// Syntax: function Event.disconnect(func : Function) : Boolean
// Syntax: function Event.disconnect(object : Object,
//                                   func : Function) : Boolean
//
// Remarks: Calling this method disconnects the specified event
//     handler from the event object.  When the event is subsequently
//     fired, the specified handler will no longer be invoked.  If
//     the handler was found and removed by the disconnect() method,
//     true is returned.  If the handler was not found, false is returned.
// Returns: True if the event handler was successfully found and removed,
//     false otherwise

void Event::disconnect(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(false);
    
    if (env->getParamCount() < 1)
        return;

    std::vector<EventSink*>::iterator it;
    for (it = m_sinks.begin(); it != m_sinks.end(); ++it)
    {
        if (env->getParamCount() >= 2)
        {
            if (env->getParam(0)->isObject() &&
                env->getParam(0)->getObject() == (*it)->obj &&
                env->getParam(1)->isFunction() &&
                env->getParam(1)->getFunction() == (*it)->method.getFunction())
            {
                delete (*it);
                m_sinks.erase(it);
                retval->setBoolean(true);
                return;
            }
        }
         else
        {
            if (env->getParam(0)->isFunction() &&
                env->getParam(0)->getFunction() == (*it)->method.getFunction())
            {
                delete (*it);
                m_sinks.erase(it);
                retval->setBoolean(true);
                return;
            }
        }
    }
}

// (METHOD) Event.disconnectAll
//
// Description: Disconnects all event handlers.
//
// Syntax: function Event.disconnectAll()
//
// Remarks: Disconnects all event handlers from the event.  When the
//     event is subsequently fired, any previous event handlers
//     will no longer be triggered
   
void Event::disconnectAll(kscript::ExprEnv* env, kscript::Value* retval)
{
    std::vector<EventSink*>::iterator it;
    for (it = m_sinks.begin(); it != m_sinks.end(); ++it)
        delete *it;
    m_sinks.clear();
}

// (METHOD) Event.isConnected
//
// Description: Determines if the event is handled
//
// Syntax: function Event.isConnected()
//
// Remarks: This method allows the caller to determine if the event
//     is handled.  If the event is handled by one or more handlers,
//     the function will return true.
// Returns: True if the event is handled, false otherwise

void Event::isConnected(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(getSinkCountInternal() > 0 ? true : false);
}

// (METHOD) Event.getSinkCount
//
// Description: Returns the number of event handlers
//
// Syntax: function Event.getSinkCount()
//
// Remarks: This method returns the number of event handlers (sinks)
//     attached to the event.  This normally corresponds to the number
//     of times the connect() method was called
// Returns: An integer value indicating the number of event handlers

void Event::getSinkCount(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(getSinkCountInternal());
}



// (METHOD) Event.fire
//
// Description: Connects an event to a function.
//
// Syntax: function Event.fire(sender : Object,
//                             event_args : Object)
//
// Remarks: Fires an event.  The parameters passed to this method
//     will be passed on to the event sink(s).
// Param(sender): A reference to the sender of the event
// Param(event_args): An object containing event arguments specific
//     to the event

void Event::zfire(kscript::ExprEnv* env, kscript::Value* retval)
{
    kscript::Value sender;
    kscript::Value event_args;
    
    if (env->getParamCount() > 0)
        sender.setValue(env->getParam(0));
    if (env->getParamCount() > 1)
        event_args.setValue(env->getParam(1));
    
    fire(env, sender.getObject(), &event_args);
}



void Event::connectInternal(kscript::Value* func_value)
{
    EventSink* sink = new EventSink;
    func_value->eval(NULL, &sink->method);
    m_sinks.push_back(sink);
}

void Event::connectInternal(kscript::Value* obj, kscript::Value* func_value)
{
    if (!obj->isObject())
        return;
    
    // prevent infinite loops by programs adding
    // too many event handlers/sinks
    if (m_sinks.size() >= 50000)
        return;
        
    EventSink* sink = new EventSink;
    sink->obj = obj->getObject();
    sink->obj->registerEventListener(sink);
    func_value->eval(NULL, &sink->method);
    m_sinks.push_back(sink);
}


void Event::fire(kscript::ExprEnv* env,
                 kscript::ValueObject* sender,
                 kscript::Value* event_args)
{
    kscript::ExprParser* parser = env->getParser();
    kscript::Value retval;

    kscript::Value* vparams[2];

    vparams[0] = new kscript::Value;
    vparams[1] = new kscript::Value;

    if (sender)
    {
        vparams[0]->setObject(sender);
    }

    if (event_args)
    {
        event_args->eval(env, vparams[1]);
    }

    this->baseRef();

    kscript::Value object;
    
    // use this type of for loop instead of iterator
    // loop in case an event handler gets added or
    // removed during the 'invoke' below
    for (size_t i = 0; i < m_sinks.size(); ++i)
    {
        EventSink* sink = m_sinks[i];
        
        if (!sink->enabled)
            continue;
            
        if (sink->obj)
        {
            object.setObject(sink->obj);
        }
        
        parser->invoke(env,
                       &sink->method,
                       &object,
                       &retval,
                       vparams,
                       2);
    }

    delete vparams[0];
    delete vparams[1];
    
    this->baseUnref();
}

size_t Event::getSinkCountInternal() const
{
    return m_sinks.size();
}



// -- PaintEvent specialization --

void PaintEvent::fire(kscript::ExprEnv* env,
                 kscript::ValueObject* sender,
                 kscript::Value* event_args)
{
    Graphics* graphics = (Graphics*)event_args->getMember(L"graphics")->getObject();
    graphics->baseRef();
    
    if (!graphics->m_wnd)
    {
        graphics->decrementPaintCount();
        graphics->baseUnref();
        return;
    }
    
    
    wxDC* dc = graphics->m_dc;
    WXWidget handle = graphics->m_wnd->GetHandle();
    
    Event::fire(env, sender, event_args);
    
    // this is still running in the thread
    // but (at least on win32) these functions
    // should be callable from threads
    
    if (graphics->m_wnd)
    {
        #ifdef WIN32
            // win32 optimized version
            HWND hwnd = (HWND)handle;
            HDC destdc = ::GetDC(hwnd);
            RECT rect;
            
            ::GetClientRect(hwnd, &rect);
            ::BitBlt(destdc, 0, 0, rect.right+1, rect.bottom+1, (HDC)kcl::getHdcFrom(*dc), 0, 0, SRCCOPY);
            ::ReleaseDC(hwnd, destdc);
        #else
            ::wxMutexGuiEnter();
            wxSize size = graphics->m_wnd->GetClientSize();
            wxClientDC client_dc(graphics->m_wnd);
            client_dc.Blit(0, 0, size.x, size.y, dc, 0, 0);
            ::wxMutexGuiLeave();
        #endif
    }
    
    
    graphics->decrementPaintCount();
    graphics->baseUnref();
}


// -- Timer class implementation --

// (CLASS) Timer
// Category: Application
// Description: A class for firing events at a specified interval.
// Remarks: The Timer class represents a timer for firing events
//     at a specified interval.

// (EVENT) Timer.tick
// Description: Fired each time a timer interval has passed.

zTimer::zTimer()
{
    // default interval: 100 milliseconds
    m_interval = 100;
}

zTimer::~zTimer()
{
}

// (CONSTRUCTOR) Timer.constructor
// Description: Creates a new Timer object.
//
// Syntax: Timer()
//
// Remarks: Creates a new Timer object.

void zTimer::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    initComponent(env);
    
    getMember(L"tick")->setObject(Event::createObject(env));
    m_tick_event.setValue(getMember(L"tick"));
}

// (METHOD) Timer.isRunning
// Description: Returns true if the timer is running and false otherwise.
//
// Syntax: function Timer.isRunning() : Boolean
//
// Remarks: This function indicates whether or not the timer is currently
//     running, and therefore, firing timer events.  Returns true if the
//     timer is running and false otherwise.
//
// Returns: True if the timer is running and false otherwise.

void zTimer::isRunning(kscript::ExprEnv* env, kscript::Value* retval)
{
    bool running = IsRunning();
    retval->setBoolean(running);
}

// (METHOD) Timer.setInterval
// Description: Set the timer event interval.
//
// Syntax: function Timer.setInterval(interval : Integer)
//
// Remarks: This function sets the time |interval| in milliseconds
//     at which the timer tick event is to be fired.
//
// Param(interval): The time interval in milliseconds at which the
//     the timer tick event is to be fired.

void zTimer::setInterval(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1)
        return;

    // if we're currently running, stop running
    bool running = IsRunning();
    if (running)
        Stop();

    // get the input parameter
    int interval = env->getParam(0)->getInteger();
    
    // if the specified interval is less than zero,
    // set the interval to the default
    if (interval < 0)
        interval = 100;

    // set the interval to fire the event
    m_interval = interval;

    // if we were previously running, restart with
    // the new interval
    if (running)
        Start(m_interval);
}

// (METHOD) Timer.getInterval
// Description: Gets the timer event interval.
//
// Syntax: function Timer.getInterval() : Integer
//
// Remarks: This function returns the time |interval| in milliseconds
//     at which the timer tick event is to be fired.
//
// Returns: The time interval in milliseconds at which the
//     the timer tick event is to be fired.

void zTimer::getInterval(kscript::ExprEnv* env, kscript::Value* retval)
{
    // return the interval that the event is to be fired
    retval->setInteger(m_interval);
}

// (METHOD) Timer.start
// Description: Starts the timer
//
// Syntax: function Timer.start()
//
// Remarks: This function starts the timer, so that the tick
//     event is fired at a periodic interval.

void zTimer::start(kscript::ExprEnv* env, kscript::Value* retval)
{
    // if we're already running, we're done
    if (IsRunning())
        return;

    // start the timer
    Start(m_interval);
}

// (METHOD) Timer.stop
// Description: Stops the timer
//
// Syntax: function Timer.stop()
//
// Remarks: This function stops the timer, so that the tick
//     event stops firing.

void zTimer::stop(kscript::ExprEnv* env, kscript::Value* retval)
{
    // if we're not running, we're done
    if (!IsRunning())
        return;

    // stop the timer
    Stop();
}

void zTimer::Notify()
{
    invokeJsEvent(&m_tick_event);
}


// -- Application class implementation --

// (CLASS) Application
// Category: Application
// Description: A class that represents a script application.
// Remarks: The application class represents a script application and
//     is primarily reponsible for starting and stopping the script
//     event loop.  In event-oriented programming, the event loop drives
//     the user interface aspects of the program, and coordinates the
//     connection between the program code and the user's actions.  Using
//     the Application object allows the usage of Forms and interactive
//     script programs.

Application::Application()
{    
    m_temp_parent = NULL;
    
    m_loop_condition_mutex = new wxMutex;
    m_loop_condition = new wxCondition(*m_loop_condition_mutex);
    
    m_exit = false;
    m_exit_modal = false;
    m_wakeup = false;
    
    m_evt_sink.m_app = this;
}

Application::~Application()
{
    sigDestructing();
    
    //delete m_temp_parent;
    delete m_loop_condition;
    delete m_loop_condition_mutex;
}

void Application::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}





// Application.execute(String) - executes script code
// 
//   Parameter 1: script code to execute

// NOTE: internal, don't document
// NOTE: this is slated for removal, use javascript eval()
 
void Application::execute(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() < 1 || !env->getParam(0)->isString())
    {
        return;
    }
    
    const wchar_t* code = env->m_eval_params[0]->getString();


    ScriptHost* outer_script_host = (ScriptHost*)env->getParser()->getExtraLong();


    ScriptHost* script_host = new ScriptHost;
    
    // add the outer script host's include paths
    const std::vector<ScriptHostInclude>& inc = outer_script_host->getIncludePaths();
    std::vector<ScriptHostInclude>::const_iterator it;
    for (it = inc.begin(); it != inc.end(); ++it)
        script_host->addIncludePath(*it);

    
    if (env->getParamCount() > 1)
    {
        kscript::Value* val = env->m_eval_params[1];
        if (val->isObject())
        {
            kscript::ValueObject* obj = val->getObject();
            
            size_t i, count = obj->getRawMemberCount();
            for (i = 0; i < count; ++i)
            {
                script_host->addValue(obj->getRawMemberName(i),
                                      *obj->getRawMemberByIdx(i));
            }
        }
    }



    retval->setObject(kscript::Object::createObject(env));

    if (script_host->compile(code))
    {
        script_host->run();
        retval->getMember(L"retval")->setValue(script_host->getRetval());
    }

    retval->getMember(L"errorCode")->setInteger(script_host->getErrorCode());
    retval->getMember(L"errorLine")->setInteger(script_host->getErrorLine());
    retval->getMember(L"errorFile")->setString(script_host->getErrorFile().c_str());
    retval->getMember(L"errorString")->setString(script_host->getErrorString().c_str());
    
    delete script_host;
}

// (METHOD) Application.run
//
// Description: Starts the event loop for a form.
//
// Syntax: static function Application.run()
//
// Remarks: Starts the event loop for a form.  When using forms in a script,
//     this function must be called before the form can register events.

void Application::run(kscript::ExprEnv* env, kscript::Value* retval)
{
/*
    if (m_forms.size() < 1)
    {
        // -- no forms, immediately exit --
        return;
    }
*/
    m_obj_mutex.Lock();
    m_exit = false;
    m_obj_mutex.Unlock();
    
    bool wakeup;
    
    // -- wait for events --
    while (1)
    {
        m_obj_mutex.Lock();
        if (m_exit || m_exit_modal)
        {
            // reset modal exit flag for possible
            // 'next time' modal dialog
            m_exit_modal = false;
            
            // exit flag, if set, remains set because somebody
            // called Application.exit()
            
            m_obj_mutex.Unlock();
            break;
        }
        wakeup = m_wakeup;
        m_wakeup = false;
        m_obj_mutex.Unlock();
        
        processEvents(env);

        if (env->getRuntimeError())
            return;
            
        if (env->isCancelled())
            return;

        if (!wakeup)
        {
            m_loop_condition_mutex->Lock();
            m_loop_condition->WaitTimeout(3000);
            m_loop_condition_mutex->Unlock();
        }
    }
}

// (METHOD) Application.exit
//
// Description: Exits the event loop and stops the script from running.
//
// Syntax: static function Application.exit()
//
// Remarks: Exists the event loop and stops the script from running.

void Application::exit(kscript::ExprEnv* env, kscript::Value* retval)
{
    std::set<Form*>::iterator it;
    std::set<Form*> forms = m_forms;
    for (it = forms.begin(); it != forms.end(); ++it)
    {
        (*it)->destroy();
    }
    
    
    // the exit sequence is a little complicated, but it has
    // its reasons.  As soon as this method is called, a message
    // is posted to the main thread's event loop;  The message,
    // when processed in the main thread, will set the m_exit flag
    // and then wake up the thread that App normally runs in;  After
    // the thread is woken up, the Application::run() method will stop
    // running.  This allows all call stacks in the main thread
    // to unwind properly
    
    wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED, ID_AppExit);
    ::wxPostEvent(&m_evt_sink, e);
}

void Application::exitModal()
{
    m_exit_modal = true;
    
    if (wxThread::IsMain())
    {
        wakeUpQueue();
    }
}

void Application::getStartupPath(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    ScriptHost* script_host = (ScriptHost*)env->getParser()->getExtraLong();
    
    retval->setString(towstr(script_host->getStartupPath()));
}


wxWindow* Application::getTempParent()
{
    if (!m_temp_parent)
    {
        m_temp_parent = new wxWindow(g_app->getMainWindow(),
                                 -1,
                                 wxPoint(0,0),
                                 wxSize(0,0));
        m_temp_parent->Show(false);
    }
    
    return m_temp_parent;
}

void Application::registerForm(Form* f)
{
    m_forms.insert(f);
}

void Application::unregisterForm(Form* f)
{
    m_forms.erase(f);
}


// wakeUpQueue() tells the script's event loop to stop
// waiting and process pending events

void Application::wakeUpQueue()
{
    m_obj_mutex.Lock();
    m_wakeup = true;
    m_obj_mutex.Unlock();
    
    if (m_loop_condition_mutex->TryLock() == wxMUTEX_BUSY)
    {
        // script thread is currently working, so
        // wake it up the next time around
        return;
    }
    
    m_loop_condition->Signal();
    m_loop_condition_mutex->Unlock();
}


void Application::postEvent(Event* evt,
                    kscript::ValueObject* sender,
                    kscript::Value* event_args,
                    unsigned int event_flags)
{
    PendingEvent p;
    p.jsevent = evt;
    p.sender = sender;
    p.args = event_args;
    p.flags = event_flags;
    if (p.sender)
        p.sender->baseRef();
    
    m_queue_mutex.Lock();
    m_queue.push(p);
    m_queue_mutex.Unlock();

    wakeUpQueue();
}



// processEvents() runs in the script's thread
void Application::processEvents(kscript::ExprEnv* env)
{
    PendingEvent evt;
    while (1)
    {
        m_queue_mutex.Lock();
        evt.jsevent = NULL;
        if (!m_queue.empty())
        {
            evt = m_queue.front();
            m_queue.pop();
        }
        m_queue_mutex.Unlock();
        if (!evt.jsevent)
            return;
        
        if ((evt.flags & eventFlagNoGuiBlock) == 0)
        {
            wxCommandEvent e1(wxEVT_COMMAND_MENU_SELECTED, ID_EventStart);
            e1.SetExtraLong((long)evt.jsevent);
            ::wxPostEvent(&m_evt_sink, e1);
        }
        

        evt.jsevent->fire(env, evt.sender, evt.args);

        if (evt.args)
            delete evt.args;
        if (evt.sender)
            evt.sender->baseUnref();

        if ((evt.flags & eventFlagNoGuiBlock) == 0)
        {
            wxCommandEvent e2(wxEVT_COMMAND_MENU_SELECTED, ID_EventEnd);
            e2.SetExtraLong((long)evt.jsevent);
            ::wxPostEvent(&m_evt_sink, e2);
        }
    }

}



AppEvtHandler::AppEvtHandler()
{
    m_app = NULL;
    m_events_processing = 0;
}


// ProcessEvent() processes events send from other threads
// it run's in the main thread and works on gui-thread-only tasks
bool AppEvtHandler::ProcessEvent(wxEvent& _evt)
{
    wxCommandEvent& evt = (wxCommandEvent&)_evt;
    int id = evt.GetId();
 
    if (!m_app)
        return false;
    
    if (id == ID_EventStart)
    {
        m_events_processing++;
        
        if (m_events_processing == 1)
        {
            // a gui blocking event is running, so disable all form windows
            std::set<Form*>::const_iterator it, it_end = m_app->m_forms.end();
            for (it = m_app->m_forms.begin(); it != it_end; ++it)
            {
                (*it)->enableForm(false);
            }
        }
        
        return true;
    }
     else if (id == ID_EventEnd)
    {
        m_events_processing--;
        
        if (m_events_processing == 0)
        {
            // all gui-blocking events are done, so reenable all form windows
            std::set<Form*>::const_iterator it, it_end = m_app->m_forms.end();
            for (it = m_app->m_forms.begin(); it != it_end; ++it)
            {
                (*it)->enableForm(true);
            }
        }
        
        return true;
    }
     else if (id == ID_AppExit)
    {
        m_app->m_exit = true;
        m_app->wakeUpQueue();
    }

    return false;
}


// -- ScriptCommandEventRouter implementation --

void ScriptCommandEventRouter::registerCommandReceiver(int id, ScriptHostBase* comp)
{
    // this may not be happening in the main thread
    // so lock the mutex
    XCM_AUTO_LOCK(m_map_mutex);
    m_map[id] = comp;
}

void ScriptCommandEventRouter::unregisterCommandReceiver(int id)
{
    // this may not be happening in the main thread
    // so lock the mutex
    XCM_AUTO_LOCK(m_map_mutex);
    m_map.erase(id);
}


bool ScriptCommandEventRouter::ProcessEvent(wxEvent& evt)
{
    if (evt.IsCommandEvent())
    {
        wxCommandEvent& command_evt = (wxCommandEvent&)evt;
        int id = command_evt.GetId();
        
        if (id >= ID_FirstScriptCommandId && id < ID_FirstScriptControlId &&
            command_evt.GetEventType() == wxEVT_COMMAND_MENU_SELECTED)
        {
            // this always happens in the main thread
            // but lock the mutex to prevent concurrent access
            // from the register/unregister methods
            XCM_AUTO_LOCK(m_map_mutex);
            
            std::map<int, ScriptHostBase*>::iterator it;
            
            it = m_map.find(id);
            if (it != m_map.end())
            {
                if (it->second)
                {
                    it->second->onEvent(evt);
                    return true;
                }
            }
        }
    }
    
    return wxEvtHandler::ProcessEvent(evt);
}

