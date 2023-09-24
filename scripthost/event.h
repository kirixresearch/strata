/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-11-23
 *
 */


#ifndef H_SCRIPTHOST_EVENT_H
#define H_SCRIPTHOST_EVENT_H


namespace scripthost
{




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



} // namespace scripthost


#endif
