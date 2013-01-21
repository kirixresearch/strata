/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-06-16
 *
 */


#include "kscript.h"
#include "jsobject.h"
#include "jsstring.h"
#include "jsnumber.h"
#include "jsboolean.h"


namespace kscript
{


// (CLASS) Object
// Category: Core
// Description: A generic object class and container.
// Remarks: A generic object class and container.

Object::Object()
{
}

Object::~Object()
{
}

// (CONSTRUCTOR) Object.constructor
// Description: Creates a new Object.
//
// Syntax: Object()
// Syntax: Object(value : Primitive)
//
// Remarks: Creates a new instance of an object.  If |value| is supplied, the
//     constructor creates a new Boolean, Number or String object representing 
//     the |value|.
//
// Param(value): A primitive boolean, number or string value.

void Object::constructor(ExprEnv* env, Value* retval)
{
}

void Object::staticConstructor(ExprEnv* env, void* param, Value* retval)
{
    if (env->m_param_count > 0)
    {
        Value* p0 = env->getParam(0);
        
        if (p0->isObject())
        {
            p0->eval(env, retval);
        }
         else if (p0->isNull() || p0->isUndefined())
        {
            Object* obj = Object::createObject(env);
            retval->setObject(obj);
        }
         else if (p0->isString())
        {
            String* obj = String::createObject(env);
            obj->val.setString(p0);
            retval->setObject(obj);
        }
         else if (p0->isNumber())
        {
            Number* obj = Number::createObject(env);
            p0->eval(NULL, &obj->val);
            retval->setObject(obj);
        }
         else if (p0->getType() == Value::typeBoolean)
        {
            Boolean* obj = Boolean::createObject(env);
            obj->val.setBoolean(p0->getBoolean());
            retval->setObject(obj);
        }
         else
        {
            Object* obj = Object::createObject(env);
            retval->setObject(obj);
        }
    }
     else
    {
        Object* obj = Object::createObject(env);
        retval->setObject(obj);
    }
}

bool Object::customConstructor(int class_id, ExprEnv* env, void* param, Value* retval)
{
    if (class_id != Object::staticGetClassId())
        return false;
        
    if (env->m_param_count > 0)
    {
        staticConstructor(env, param, retval);
        Value* vthis = env->getThis();
        if (retval->isObject() && vthis)
        {
            // see comment in parse.cpp (search for mutate)
            bool* mutate = (bool*)param;
            *mutate = true;
            
            vthis->setObject(retval->getObject());
        }
         
        return true;
    }
    
    return false;
}

// (METHOD) Object.toString
// Description: Returns a string that represents the object.
//
// Syntax: function Object.toString()
//
// Remarks: Returns a string that represents the object.
//
// Returns: Returns a string that represents the object.

void Object::toString(ExprEnv* env, void* param, Value* retval)
{
    Value* thisval = env->getThis();
    if (!thisval)
    {
        retval->setString(L"[object Object]");
        return;
    }
    
    std::wstring res = L"[object ";
    res += thisval->getObject()->getClassName();
    res += L"]";
    retval->setString(res.c_str());
}

// (METHOD) Object.toLocaleString
// Description: Returns a string that represents the object.
//
// Syntax: function Object.toLocaleString()
//
// Remarks: Returns a string that represents the object. Identical to Object.toString().
//
// Returns: Returns a string that represents the object.

void Object::toLocaleString(ExprEnv* env, void* param, Value* retval)
{
    toString(env, param, retval);
}

void Object::toString(Value* retval)
{
    std::wstring res = L"[object ";
    res += getClassName();
    res += L"]";
    retval->setString(res.c_str());
}

// (METHOD) Object.valueOf
// Description: Returns the 'primitive' value of the object.
//
// Syntax: function Object.valueOf()
//
// Remarks: Returns the 'primitive' value of the object.
//
// Returns: Returns the 'primitive' value of the object.

void Object::valueOf(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = env->getThis();
    if (vthis)
    {
        vthis->toPrimitive(retval);
    }
     else
    {
        retval->setNull();
    }
}



// (METHOD) Object.hasOwnProperty
// Description: Checks for the existence of an object property
//
// Syntax: function Object.hasOwnProperty(property : String) : Boolean
//
// Remarks: Checks for the existence of an object property.  Object
//     and prototype chains are not checked, therefore the method
//     will only return true if the immediate object |this| has a
//     a property
//
// Returns: Returns true of the object |this| has property |property|

void Object::hasOwnProperty(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = env->getThis();
    if (!vthis || !vthis->isObject() || env->getParamCount() < 1)
    {
        retval->setBoolean(false);
        return;
    }
    
    ValueObject* obj = vthis->getObject();
    retval->setBoolean(obj->lookupMember(env->getParam(0)->getString(), false) ? true : false);
}

void Object::propertyIsEnumerable(ExprEnv* env, void* param, Value* retval)
{
    retval->setBoolean(false);
    
    Value* vthis = env->getThis();
    if (!vthis || !vthis->isObject() || env->getParamCount() < 1 || !env->getParam(0)->isObject())
        return;
    ValueObject* thisobj = vthis->getObject();
    
    
    if (env->getParamCount() < 1)
        return;
        
    std::wstring prop = env->getParam(0)->getString();
    
    retval->setBoolean(thisobj->getMemberExists(prop));
    
/*
    retval->setBoolean(false);
    
    if (env->getParamCount() < 1)
        return;
        
    std::wstring prop = env->getParam(0)->getString();
    
    
    std::map<std::wstring, Value*>::iterator it;
    it = m_members_map.find(prop);
    
    if (it != m_members_map.end())
        retval->setBoolean(true);
*/
}

void Object::isPrototypeOf(ExprEnv* env, void* param, Value* retval)
{
    retval->setBoolean(false);
    
    Value* vthis = env->getThis();
    if (!vthis || !vthis->isObject() || env->getParamCount() < 1 || !env->getParam(0)->isObject())
        return;
    
    ValueObject* obj = env->getParam(0)->getObject();
    
    Value* proto = NULL;
    if (obj->hasPrototype())
        proto = obj->getPrototype();
        
    while (proto)
    {
        if (proto->getObject() == vthis->getObject())
        {
            retval->setBoolean(true);
            return;
        }
            
        if (proto->getObject()->hasPrototype())
            proto = proto->getObject()->getPrototype();
             else
            proto = NULL;
    }
}


};

