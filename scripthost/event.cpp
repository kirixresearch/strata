/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#include "scripthost.h"
#include "event.h"


namespace scripthost
{


// TODO: Please note that Event and ScriptHostBase aren't yet entirely ported from appmain


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



}; // namespace scripthost
