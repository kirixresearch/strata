/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2003-05-14
 *
 */


#ifndef __XCM_SIGNAL_H
#define __XCM_SIGNAL_H


#include <vector>


namespace xcm
{


class connection_base
{
public:
    connection_base()
    {
        m_ref = 0;
    }

    virtual ~connection_base()
    {
    }

    virtual void deactivate() = 0;
    virtual bool isActive() = 0;

    void ref()
    {
        m_ref++;
    }
    void unref()
    {
        if (--m_ref == 0)
            delete this;
    }

private:
    int m_ref;
};


class signal_sink
{
public:

    void addToDelete(connection_base* connection)
    {
        to_delete.push_back(connection);
        connection->ref();
    }
    
protected:

    ~signal_sink()
    {
        disconnectAllSignals();
    }

    void disconnectAllSignals()
    {
        std::vector<connection_base*>::iterator it;
        for (it = to_delete.begin(); it != to_delete.end(); it++)
        {
            (*it)->deactivate();
            (*it)->unref();
        }
        to_delete.clear();
    }
    
private:
    std::vector<connection_base*> to_delete;
};


class connection_base0 : public connection_base
{
public:
    virtual void doCall() = 0;
};
template<class ct>
class connection0 : public connection_base0
{
public:
    void (ct::*pMethod)();
    ct* pObj;

    void doCall()
    {
        if (pObj)
        {
            (pObj->*pMethod)();
        }
    }
    void deactivate()
    {
        pObj = NULL;
    }
    bool isActive()
    {
        return pObj ? true : false;
    }
};
class connection_callable0 : public connection_base0
{
public:
    void (*pCallable)();

    void doCall()
    {
        if (pCallable)
        {
            (*pCallable)();
        }
    }
    void deactivate()
    {
        pCallable = NULL;
    }
    bool isActive()
    {
        return pCallable ? true : false;
    }
};
class signal0 : public signal_sink
{
public:
    std::vector<connection_base0*> connections;
    ~signal0()
    {
        disconnect();
    }
    void fire()
    {
        for (size_t i = 0; i < connections.size(); ++i)
            connections[i]->doCall();
    }
    void operator()()
    {
        for (size_t i = 0; i < connections.size(); ++i)
            connections[i]->doCall();
    }
    template<class ct>
    void connect(ct* pObj, void (ct::*pMethod)())
    {
        connection0<ct>* pConnection = new connection0<ct>;
        pConnection->ref();
        pConnection->pMethod = pMethod;
        pConnection->pObj = pObj;
        pObj->addToDelete(pConnection);
        connections.push_back(pConnection);
    }
    void connect(void (*pFunction)())
    {
        connection_callable0* pConnection = new connection_callable0;
        pConnection->ref();
        pConnection->pCallable = pFunction;
        connections.push_back(pConnection);
    }
    void disconnect()
    {
        for (size_t i = 0; i < connections.size(); ++i)
            connections[i]->unref();
        connections.clear();
    }
    int isActive()
    {
        for (size_t i = 0; i < connections.size(); ++i)
        {
            if (connections[i]->isActive())
                return true;
        }
        return false;
    }
};




#define XCM_SIGNAL_IMPL(ARG_COUNT, TEMPLATE_ARGS, DEF_ARGS, DEFP_ARGS, CALL_ARGS)\
template<TEMPLATE_ARGS>\
class connection_base##ARG_COUNT : public connection_base\
{\
public:\
    virtual void doCall(DEF_ARGS) = 0;\
};\
template<class ct, TEMPLATE_ARGS>\
class connection##ARG_COUNT : public connection_base##ARG_COUNT<DEF_ARGS>\
{\
public:\
    void (ct::*pMethod)(DEF_ARGS);\
    ct* pObj;\
    void doCall(DEFP_ARGS)\
    {\
        if (pObj)\
        {\
            (pObj->*pMethod)(CALL_ARGS);\
        }\
    }\
    void deactivate()\
    {\
        pObj = NULL;\
    }\
    bool isActive()\
    {\
        return pObj ? true : false;\
    }\
};\
template<TEMPLATE_ARGS> \
class connection_callable##ARG_COUNT : public connection_base##ARG_COUNT<DEF_ARGS>\
{\
public:\
    void (*pCallable)(DEF_ARGS);\
    void doCall(DEFP_ARGS)\
    {\
        if (pCallable)\
        {\
            (*pCallable)(CALL_ARGS);\
        }\
    }\
    void deactivate()\
    {\
        pCallable = NULL;\
    }\
    bool isActive()\
    {\
        return pCallable ? true : false;\
    }\
};\
template<TEMPLATE_ARGS>\
class signal##ARG_COUNT : public signal_sink\
{\
public:\
    std::vector<connection_base##ARG_COUNT<DEF_ARGS>*> connections;\
    ~signal##ARG_COUNT()\
    {\
        disconnect();\
    }\
    void fire(DEFP_ARGS)\
    {\
        for (size_t i = 0; i < connections.size(); ++i)\
            connections[i]->doCall(CALL_ARGS);\
    }\
    void operator()(DEFP_ARGS)\
    {\
        for (size_t i = 0; i < connections.size(); ++i)\
            connections[i]->doCall(CALL_ARGS);\
    }\
    template<class ct>\
    void connect(ct* pObj, void (ct::*pMethod)(DEF_ARGS))\
    {\
        connection##ARG_COUNT<ct, DEF_ARGS>* pConnection = new connection##ARG_COUNT<ct,DEF_ARGS>;\
        pConnection->ref();\
        pConnection->pMethod = pMethod;\
        pConnection->pObj = pObj;\
        pObj->addToDelete(pConnection);\
        connections.push_back(pConnection);\
    }\
    void connect(void (*pFunction)(DEF_ARGS))\
    {\
        connection_callable##ARG_COUNT<DEF_ARGS>* pConnection = new connection_callable##ARG_COUNT<DEF_ARGS>;\
        pConnection->ref();\
        pConnection->pCallable = pFunction;\
        connections.push_back(pConnection);\
    }\
    void disconnect()\
    {\
        for (size_t i = 0; i < connections.size(); ++i) \
            connections[i]->unref(); \
        connections.clear(); \
    }\
    bool isActive()\
    {\
        for (size_t i = 0; i < connections.size(); ++i) \
        {\
            if (connections[i]->isActive())\
                return true;\
        }\
        return false;\
    }\
};






#define XCMCOMMA ,


XCM_SIGNAL_IMPL(1,
    class p1t,
    p1t,
    p1t p1,
    p1
)


XCM_SIGNAL_IMPL(2,
    class p1t XCMCOMMA class p2t,
    p1t XCMCOMMA p2t,
    p1t p1 XCMCOMMA p2t p2,
    p1 XCMCOMMA p2
)

XCM_SIGNAL_IMPL(3,
    class p1t XCMCOMMA class p2t XCMCOMMA class p3t,
    p1t XCMCOMMA p2t XCMCOMMA p3t,
    p1t p1 XCMCOMMA p2t p2 XCMCOMMA p3t p3,
    p1 XCMCOMMA p2 XCMCOMMA p3
)

XCM_SIGNAL_IMPL(4,
    class p1t XCMCOMMA class p2t XCMCOMMA class p3t XCMCOMMA class p4t,
    p1t XCMCOMMA p2t XCMCOMMA p3t XCMCOMMA p4t,
    p1t p1 XCMCOMMA p2t p2 XCMCOMMA p3t p3 XCMCOMMA p4t p4,
    p1 XCMCOMMA p2 XCMCOMMA p3 XCMCOMMA p4
)



};




#endif
